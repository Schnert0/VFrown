#include "uart.h"

static UART_t this;

bool UART_Init() {
  memset(&this, 0, sizeof(UART_t));

  return true;
}


void UART_Cleanup() {

}


void UART_SaveState() {
  Backend_WriteSave(&this, sizeof(UART_t));
}


void UART_LoadState() {
  Backend_ReadSave(&this, sizeof(UART_t));
}


void UART_Reset() {
  // Maybe wrong since init captured using uart
  this.ctrl   = 0x00ef;
  this.stat   = 0x0003;
  this.baudLo = 0x00ff;
  this.baudHi = 0x00ff;
  this.tx     = 0x00ff;

  this.rxHead = 0;
  this.rxTail = 0;
  this.rxEmpty = true;
}


void UART_Update(int32_t cycles) {
  // TODO: Use Timer objects for these instead
  if (this.txTimer > 0) {
    this.txTimer -= cycles;
    if (this.txTimer <= 0) {
      UART_TransmitTick();
    }
  }

  if (this.rxTimer > 0) {
    this.rxTimer -= cycles;
    if (this.rxTimer <= 0) {
      UART_RecieveTick();
    }
  }
}


uint16_t UART_Read(uint16_t addr) {
  switch (addr) {
  case 0x3d30: return this.ctrl;
  case 0x3d31: return this.stat;
  case 0x3d33: return this.baudLo;
  case 0x3d34: return this.baudHi;
  case 0x3d35: return this.txBuffer;
  case 0x3d36: return UART_GetRxBuffer();
  default:
    VSmile_Warning("Unknown read from UART address %04x %06x", addr, CPU_GetCSPC());
  }

  return 0x0000;
}


void UART_Write(uint16_t addr, uint16_t data) {
  switch (addr) {
  case 0x3d30: UART_SetCtrl(data); return;
  case 0x3d31: UART_SetStat(data); return;
  case 0x3d32: if (data & 1) UART_Reset(); return;
  case 0x3d33: this.baudLo = data & 0xff; UART_SetBaud(); return;
  case 0x3d34: this.baudHi = data & 0xff; UART_SetBaud(); return;
  case 0x3d35: UART_SetTxBuffer(data); return;
  default:
    VSmile_Warning("Unknown write to UART address %04x with %04x at %06x", addr, data, CPU_GetCSPC());
  }
}


void UART_SetCtrl(uint16_t data) {
  // printf("UART CTRL set to %04x at %06x\n", data, CPU_GetCSPC());
  if (!(data & 0x40)) {
    this.rxAvailable = false;
    this.rxBuffer = 0;
  }

  if ((data ^ this.ctrl) & 0x80) {
    if (data & 0x80) {
      this.stat |= 0x02;
    } else {
      this.stat &= ~0x42;
      this.txTimer = 0;
    }
  }

  this.ctrl = data;
}


void UART_SetStat(uint16_t data) {
  // printf("write to UART STAT with %04x at %06x\n", data, CPU_GetCSPC());
  // Unset Rx Ready and Tx Ready
  if (data & 0x01) this.stat &= ~0x01;
  if (data & 0x02) this.stat &= ~0x02;
  if (!(this.stat & 0x03)) // If both Rx Ready and Tx Ready are unset
    Misc_Write(0x3d22, 0x0100);      // Clear the UART IRQ flag
}


// void Uart_ResetUART(uint16_t data){
//   // if (data & 1) {
//   VSmile_Log("UART: reset signal sent at %06x\n", CPU_GetCSPC());
//   // }
// }


void UART_SetBaud() {
  uint32_t baudRate = SYSCLOCK / (16 * (0x10000 - this.baud));

  if (baudRate != this.baudRate) {
    this.baudRate = baudRate;
    // VSmile_Log("UART: BAUD set to %d at %06x", baudRate, CPU_GetCSPC());
  }
}


void UART_SetTxBuffer(uint16_t data) {
  // VSmile_Log("UART: Writing 0x%02x at %06x", data & 0xff, CPU_GetCSPC());
  this.txBuffer = data;
  if (this.ctrl & 0x80) { // If Tx is enabled
    this.stat &= ~0x02;   // Unset Tx Ready
    this.stat |=  0x40;   // Set Tx Busy
    this.txTimer = this.baudRate;  // Enable Transmit timer
  }
}


uint16_t UART_GetRxBuffer() {
  if (this.rxAvailable) { // If there is data available in the Rx buffer
    this.stat &= ~0x81; // Unset the Rx Buffer full flag and Rx Ready flags
    if (!this.rxEmpty) {
      this.rxBuffer = UART_PopRx();
      if (!this.rxEmpty) {
        if (this.rxTimer <= 0) {
          this.rxTimer = this.baudRate;
        }
      } else {
        this.rxAvailable = false;
      }
    }
  }

  // VSmile_Log("UART: Reading 0x%02x from Rx Buffer at %06x", this.rxBuffer, CPU_GetCSPC());

  return this.rxBuffer;
}


void UART_RxTimerReset() {
  this.rxTimer = SYSCLOCK / 9600;
}


void UART_TransmitTick() {
  // VSmile_Warning("UART: transmitting %02x to the controller\n", this.txBuffer);
  Controller_RecieveByte(this.txBuffer);

  this.stat |=  0x02;
  this.stat &= ~0x40;
  if (this.ctrl & 0x02) {
    Misc_SetIRQFlags(0x3d22, 0x0100);
  }
}


void UART_RecieveTick() {
  this.rxAvailable = true;
  // printf("UART: recieving data from controller\n");

  this.stat |= 0x81;
  if (this.ctrl & 0x01) {
    Misc_SetIRQFlags(0x3d22, 0x0100);
  }
}


bool UART_PushRx(uint8_t data) {
  // printf("push Rx %02x\n", data);

  if (!this.rxEmpty && (this.rxHead == this.rxTail)) {
    VSmile_Warning("Rx byte %02x discarded because FIFO is full", data);
    return false;
  }

  this.rxFifo[this.rxHead] = data;
  this.rxHead = (this.rxHead + 1) & 0xf;
  this.rxEmpty = false;
  this.rxTimer = this.baudRate;

  return true;
}


uint8_t UART_PopRx() {
  if (this.rxEmpty) {
    VSmile_Warning("returning 0x00 because Rx FIFO is empty");
    return 0x00;
  }

  uint8_t data = this.rxFifo[this.rxTail];
  this.rxTail = (this.rxTail + 1) & 0xf;
  this.rxEmpty = (this.rxHead == this.rxTail);

  // printf("pop Rx (%02x)\n", data);

  return data;
}
