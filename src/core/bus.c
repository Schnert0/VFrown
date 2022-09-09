#include "bus.h"
#include "vsmile.h"

static Bus_t this;

#define UART_CTRL  0x30
#define UART_STAT  0x31
#define UART_RESET 0x32
#define UART_TX    0x35
#define UART_RX    0x36

bool Bus_Init() {
  memset(&this, 0, sizeof(Bus_t));

  this.rxEmpty = true;

  this.io[0x00] = 0x001f; // 3d00 - GPIO Control
  this.io[0x01] = 0xffff; // 3d01-3d05 - IOA
  this.io[0x02] = 0xffff;
  this.io[0x03] = 0xffff;
  this.io[0x04] = 0xffff;
  this.io[0x05] = 0xffff;

  this.io[0x06] = 0x00ff; // 3d06-3d0a - IOB
  this.io[0x07] = 0x00ff;
  this.io[0x08] = 0x00ff;
  this.io[0x09] = 0x00ff;
  this.io[0x0a] = 0x00ff;

  this.io[0x0b] = 0xffff; // 3d0b-3d0f - IOC
  this.io[0x0c] = 0xffff;
  this.io[0x0d] = 0xffff;
  this.io[0x0e] = 0xffff;
  this.io[0x0f] = 0xffff;

  this.io[0x10] = 0x000f; // 3d10 - Timebase freq
  this.io[0x11] = 0x0;    // 3d11-3d1f
  this.io[0x12] = 0x0;
  this.io[0x13] = 0x0;
  this.io[0x14] = 0x0;
  this.io[0x15] = 0x0;
  this.io[0x16] = 0x0;
  this.io[0x17] = 0x0;
  this.io[0x18] = 0x0;
  this.io[0x19] = 0x0;
  this.io[0x1a] = 0x0;
  this.io[0x1b] = 0x0;
  this.io[0x1c] = 0x0;
  this.io[0x1d] = 0x0;
  this.io[0x1e] = 0x0;
  this.io[0x1f] = 0x0;

  this.io[0x20] = 0x4006; // 3d20 - System control
  this.io[0x21] = 0x3ffb; // 3d21 - IRQ control
  this.io[0x22] = 0x7fff; // 3d22 - IRQ Status
  this.io[0x23] = 0x003e; // 3d23 - Memory control
  this.io[0x24] = 0xffff; // 3d24 - Watchdog
  this.io[0x25] = 0x2002; // 3d25 - ADC Ctrl
  this.io[0x26] = 0x0;
  this.io[0x27] = 0x0;
  this.io[0x28] = 0xffff; // 3d28 - Sleep
  this.io[0x29] = 0x0080; // 3d29 - Wakeup source
  this.io[0x2a] = 0x00ff; // 3d2a - Wakeup delay
  this.io[0x2b] = 0x0001; // 3d2b - PAL/NTSC
  this.io[0x2c] = 0x1418; // 3d2c - PRNG1
  this.io[0x2d] = 0x1658; // 3d2d - PRNG2
  this.io[0x2e] = 0x0007; // 3d2e - FIQ source
  this.io[0x2f] = 0x003f; // 3d2f - DS
  this.io[0x30] = 0x00ef; // 3d30 - UART Control ?maybe wrong since init captured using uart
  this.io[0x31] = 0x0003; // 3d31 - UART Status ?same
  this.io[0x32] = 0x0;
  this.io[0x33] = 0x00ff; // 3d33 - UART Baud rate ?same
  this.io[0x34] = 0x00ff; // 3d34 - UART Baud rate ?same
  this.io[0x35] = 0x00ff; // 3d35 - UART TX

  // this.io[0x23] = 0x0028; // 3d23 - External Memory Ctrl
  // this.io[0x25] = 0x2000; // 3d25 - ADC Ctrl

  this.sysTimers = Timer_Init(SYSCLOCK / 4096, Bus_TickTimers, 0);
  Timer_Reset(this.sysTimers);

  this.watchdogTimer = Timer_Init(0, Bus_WatchdogWakeup, 0);

  for (int32_t i = 0; i < 4; i++) {
    this.adcTimers[i] = Timer_Init(0, Bus_DoADCConversion, i);
  }

  this.adcValues[0] = 0x0000;
  this.adcValues[1] = 0x0fff;
  this.adcValues[2] = 0x0000;
  this.adcValues[3] = 0x0000;

  return true;
}


void Bus_Cleanup() {
  if (this.romBuffer)
    free(this.romBuffer);

  if (this.biosBuffer)
    free(this.biosBuffer);

  for (int32_t i = 0; i < 4; i++) {
    if (this.adcTimers[i])
      Timer_Cleanup(this.adcTimers[i]);
  }

  if (this.watchdogTimer)
    Timer_Cleanup(this.watchdogTimer);

  if (this.sysTimers)
    Timer_Cleanup(this.sysTimers);
}


void Bus_Reset() {
  this.rxHead = 0;
  this.rxTail = 0;
  this.rxEmpty = true;
}


void Bus_LoadROM(const char* filePath) {
  FILE* file = fopen(filePath, "rb");
  if (!file) {
    VSmile_Error("unable to load ROM - can't open \"%s\"", filePath);
    return;
  }

  // Get size of ROM
  fseek(file, 0, SEEK_END);
  this.romSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (this.romSize > BUS_SIZE*sizeof(uint16_t)) {
    VSmile_Error("file too large!");
    this.romSize = BUS_SIZE*sizeof(uint16_t);
  }

  if (this.romBuffer)
    free(this.romBuffer);

  this.romBuffer = malloc(this.romSize);
  fread(this.romBuffer, this.romSize, sizeof(uint16_t), file);

  fclose(file);
}


void Bus_LoadBIOS(const char* filePath) {
  FILE* file = fopen(filePath, "rb");
  if (!file) {
    VSmile_Warning("unable to load BIOS - can't open \"%s\"", filePath);
    return;
  }

  // Get size of ROM
  fseek(file, 0, SEEK_END);
  this.biosSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  // if (this.biosSize > BIOS_SIZE*sizeof(uint16_t)) {
  //   VSmile_Error("file too large!");
  //   this.biosSize = BIOS_SIZE*sizeof(uint16_t);
  // }

  if (this.biosBuffer)
    free(this.biosBuffer);

  this.biosBuffer = malloc(this.biosSize);
  fread(this.biosBuffer, this.biosSize, sizeof(uint16_t), file);

  fclose(file);
}


void Bus_Tick(int32_t cycles) {
  // System Timers
  Timer_Tick(this.sysTimers, cycles);

  // Watchdog
  Timer_Tick(this.watchdogTimer, cycles);

  // ADC Conversion
  Timer_Tick(this.adcTimers[0], cycles);
  Timer_Tick(this.adcTimers[1], cycles);
  Timer_Tick(this.adcTimers[2], cycles);
  Timer_Tick(this.adcTimers[3], cycles);

  // UART Transmit and Recieve
  // TODO: Use Timer objects for these instead
  if (this.txTimer > 0) {
    this.txTimer -= cycles;
    if (this.txTimer <= 0)
      Bus_TransmitTick();
  }

  if (this.rxTimer > 0) {
    this.rxTimer -= cycles;
    if (this.rxTimer <= 0)
      Bus_RecieveTick();
  }
}

void Bus_TickTimers(int32_t index) {
  uint16_t timerIrq = 0x0040; // 4096 hz
	this.timer2khz++;
	if (this.timer2khz == 2) {
		this.timer2khz = 0;
		timerIrq |= 0x0020; // 2048 hz
		this.timer1khz++;
		if (this.timer1khz == 2) {
			this.timer1khz = 0;
			timerIrq |= 0x0010; // 1024 hz
			this.timer4hz++;
			if (this.timer4hz == 256) {
				this.timer4hz = 0;
				timerIrq |= 0x0008; // 4 hz
			}
		}
  }
  Bus_SetIRQFlags(0x3d22, timerIrq);

  Timer_Reset(this.sysTimers);
}

uint16_t Bus_Load(uint32_t addr) {
  if (addr < RAM_START+RAM_SIZE) {
    // if (addr == 0x2298)
      // printf("read from [2298] (%04x) at %06x\n", this.ram[0x2298], CPU_GetCSPC());
      // return this.ram[addr - RAM_START] | 0x0020;
    return this.ram[addr - RAM_START];
  }
  else if (addr < PPU_START+PPU_SIZE) {
    if (addr == 0x2838)
      return PPU_GetCurrLine();

    return this.ppu[addr - PPU_START];
  }
  else if (addr < SPU_START+SPU_SIZE) {
    return SPU_Read(addr);
  }
  else if (addr < 0x3d00) {
    // VSmile_Warning("read from internal memory location %04x", addr);
    return 0x0000;
  }
  else if (addr < IO_START+IO_SIZE+DMA_SIZE) {
    // if (addr != 0x3d21 && addr != 0x3d22)
    //   VSmile_Log("Read from IO address %04x at %06x", addr, CPU_GetCSPC());

    switch (addr) {
    case 0x3d01:
  	case 0x3d06:
  	case 0x3d0b: // GPIO
  		Bus_DoGPIO(addr, false);
  		return this.io[addr - IO_START];

  	case 0x3d02 ... 0x3d05:
  	case 0x3d07 ... 0x3d0a:
  	case 0x3d0c ... 0x3d0f:		// GPIO
      return this.io[addr - IO_START];
  		break;

    case 0x3d25:
      // printf("read from ADC CTRL (%04x) at %06x\n", this.io[addr - IO_START], CPU_GetCSPC());
      return this.io[addr - IO_START];
      break;

    case 0x3d27:
      // printf("read from ADC data (%04x) at %06x\n", this.io[addr - IO_START], CPU_GetCSPC());
      return this.io[addr - IO_START];
      break;


    case 0x3d2c:
    case 0x3d2d: {
      uint16_t val = this.io[addr - IO_START];
      uint16_t a = (val >> 14) & 1;
      uint16_t b = (val >> 13) & 1;
	    this.io[addr - IO_START] = ((val << 1) | (a ^ b)) & 0x7fff;
      return val;
    }


    case 0x3d2f: // Get DS
      return CPU_GetDataSegment();

    case 0x3d36: // UART RX Buffer
      return Bus_GetRxBuffer();
    }
    // printf("unknown read from IO port %04x at %06x\n", addr, CPU_GetCSPC());
    return this.io[addr - IO_START];
  }

  if ((this.romDecodeMode == 2) && this.biosBuffer && (addr >= BIOS_START) && (addr < BIOS_START + BIOS_SIZE)) {
    return this.biosBuffer[addr - BIOS_START];
  }

  if (this.romBuffer && (addr < BUS_SIZE)) {
    return this.romBuffer[addr];
  }

  return 0x0000;
}


void Bus_Store(uint32_t addr, uint16_t data) {
  if ((addr & 0xffff) < 0x4000) {
    addr &= 0x3fff;
  }

  if (addr < RAM_START+RAM_SIZE) {
    // if (addr == 0x2298)
    //   printf("write to [2298] with %04x at %06x\n", data, CPU_GetCSPC());
    this.ram[addr - RAM_START] = data;
    return;
  }
  else if (addr < PPU_START+PPU_SIZE) {
    switch (addr) {
    case 0x2810: // Page 1 X scroll
    case 0x2816: // Page 2 X scroll
      this.ppu[addr - PPU_START] = (data & 0x01ff);
      break;

    case 0x2811: // Page 1 Y scroll
    case 0x2817: // Page 2 Y scroll
      this.ppu[addr - PPU_START] = (data & 0x00ff);
      break;

    case 0x2814:
    case 0x281a:
      // printf("layer %d tilemap address set to %04x at %06x\n", addr == 0x281a, data, CPU_GetCSPC());
      this.ppu[addr - PPU_START] = data;
      break;

    case 0x2820:
    case 0x2821:
      // printf("layer %d tile data segment set to %04x at %06x\n", addr == 0x2821, data, CPU_GetCSPC());
      this.ppu[addr - PPU_START] = data;
      break;

    case 0x2863: // PPU IRQ acknowledge
      this.ppu[addr - PPU_START] &= ~data;
      break;

    case 0x2870: this.ppu[addr - PPU_START] = data & 0x3fff; break;
    case 0x2871: this.ppu[addr - PPU_START] = data & 0x03ff; break;
    case 0x2872: { // Do PPU DMA
      data &= 0x03ff;
      if (data == 0)
        data = 0x0400;

      uint32_t src = this.ppu[0x2870 - PPU_START];
      uint32_t dst = this.ppu[0x2871 - PPU_START] + 0x2c00;
      for (uint32_t i = 0; i < data; i++)
        Bus_Store(dst+i, Bus_Load(src+i));
      this.ppu[0x2872 - PPU_START] = 0;
      Bus_SetIRQFlags(0x2863, 0x0004);
    } break;

    default:
      this.ppu[addr - PPU_START] = data;
    }
    // printf("write to PPU address %04x with %04x\n", addr, data);
    return;
  }
  else if (addr < SPU_START+SPU_SIZE) {
    SPU_Write(addr, data);
    return;
  }
  else if (addr < 0x3d00) {
    VSmile_Warning("write to internal memory location %04x with %04x", addr, data);
    return;
  }
  else if (addr < IO_START+IO_SIZE+DMA_SIZE) {
    // if (addr != 0x3d21 && addr != 0x3d22 && addr != 0x3d24)
    //   VSmile_Log("Write to IO address %04x with %04x at %06x", addr, data, CPU_GetCSPC());

    switch (addr) {
    case 0x3d01:
    case 0x3d06:
    case 0x3d0b:
      addr++;

    case 0x3d05:
    case 0x3d0a:
    case 0x3d0f:

    case 0x3d02 ... 0x3d04:	// port A
    case 0x3d07 ... 0x3d09:	// port B
    case 0x3d0c ... 0x3d0e:	// port C
      this.io[addr - IO_START] = data;
      Bus_DoGPIO(addr, true);
      break;

    case 0x3d10:
      if ((this.io[addr - IO_START] & 0x3) != (data & 0x3)) {
        uint16_t hz = 8 << (data & 0x3);
        TMB_SetInterval(0, SYSCLOCK / hz);
        //printf("[BUS] TMB1 frequency set to %d Hzn", hz);
      }

      if ((this.io[addr - IO_START] & 0xc) != (data & 0xc)) {
        uint16_t hz = 128 << ((data & 0xc) >> 2);
        TMB_SetInterval(1, SYSCLOCK / hz);
        //printf("[BUS] TMB2 freqency set to %d Hz", hz);
      }

      this.io[addr - IO_START] = data;
      break;

    case 0x3d21:
      this.io[addr - IO_START] = data;
      CPU_ActivatePendingIRQs();
      break;
    case 0x3d22: // IRQ ack
      this.io[addr - IO_START] &= ~data;
      break;

    case 0x3d23: // External memory ctrl
      // printf("set rom decode mode to %d\n", (data >> 6) & 0x3);
      this.romDecodeMode = (data >> 6) & 0x3;
      // printf("set ram decode mode to %d\n", (data >> 8) & 0xf);
      this.ramDecodeMode = (data >> 8) & 0xf;

      if ((data ^ this.io[addr - IO_START]) & 0x8000) {
        if (data & 0x8000)
          Timer_Adjust(this.watchdogTimer, SYSCLOCK * (0.75f)); // 750 ms
        else
          Timer_Adjust(this.watchdogTimer, 0);

        Timer_Reset(this.watchdogTimer);
      }

      this.io[addr - IO_START] = data;
      break;

    case 0x3d24: // Watchdog clear
      if (data == 0x55aa && (this.io[0x23] & 0x8000)) {
        Timer_Reset(this.watchdogTimer);
      }
      break;

    case 0x3d25: // ADC ctrl
      Bus_WriteADCCtrl(data);
      // printf("Write to ADC CTRL with %04x at %06x\n", data, CPU_GetCSPC());
      break;

    case 0x3d2e:
      CPU_SetFIQSource(data);
      this.io[addr - IO_START] = data;
      break;

    case 0x3d2f: // Set DS
      CPU_SetDataSegment(data);
      break;

      case 0x3d30: // UART Ctrl
        Bus_SetUARTCtrl(data);
        break;

      case 0x3d31: // UART Stat
        Bus_SetUARTStat(data);
        break;

      case 0x3d32: // UART Reset
      Bus_ResetUART(data);
        break;

      case 0x3d33: // UART BAUD1
      case 0x3d34: // UART BAUD2
        this.io[addr - IO_START] = data;
        Bus_SetUARTBAUD((this.io[0x3d34 - IO_START] << 8) | this.io[0x3d33 - IO_START]);
        break;

      case 0x3d35:
        Bus_SetTxBuffer(data);
        break;

      case 0x3d36: // UART RX Buffer
        // printf("RX write\n");
        break;

      case 0x3d37:
        // printf("write to UART RX FIFO control with %04x\n", data);
        break;

    case 0x3e00:
    case 0x3e01:
    case 0x3e03: this.io[addr - IO_START] = data; break;

    case 0x3e02: { // Do DMA
      uint32_t src = ((this.io[0x3e01 - IO_START] & 0x3f) << 16) | this.io[0x3e00 - IO_START];
      uint32_t dst = this.io[0x3e03 - IO_START] & 0x3fff;
      for(uint32_t i = 0; i < data; i++) {
          uint16_t transfer = Bus_Load(src+i);
          Bus_Store(dst+i, transfer);
      }
      this.io[0x3e02 - IO_START] = 0;
      } break;

    default:
      // printf("write to unknown IO port %04x with %04x at %06x\n", addr, data, CPU_GetCSPC());
      this.io[addr - IO_START] = data;
    }
    return;
  }
  else if (IN_RANGE(addr, 0x3e04, 0x4000 - 0x3e04)) {
    VSmile_Warning("write to internal memory location %04x with %04x", addr, data);
    return;
  }

  VSmile_Error("attempt to write to out of bounds location %06x with %04x", addr, data);
}


void Bus_SetIRQFlags(uint32_t address, uint16_t data) {
  switch (address) {
  case 0x2863:
    this.ppu[0x63] |= data;
    break;
  case 0x3d22:
    this.io[0x22] |= data;
    break;
  default:
    VSmile_Warning("unknown IRQ Acknowledge for %04x with data %04x", address, data);
  }

  CPU_ActivatePendingIRQs(); // Notify CPU that there might be IRQ's to handle
}


void Bus_DoGPIO(uint16_t addr, bool write) {
  uint32_t port    = (addr - 0x3d01) / 5;

  uint16_t buffer  = this.io[0x02 + 5*port];
  uint16_t dir     = this.io[0x03 + 5*port];
  uint16_t attr    = this.io[0x04 + 5*port];
  uint16_t special = this.io[0x05 + 5*port];

  uint16_t push = dir;
  uint16_t pull = (~dir) & (~attr);
  uint16_t what = (buffer & (push | pull));
  what ^= (dir & ~attr);
  what &= ~special;

  if (port == 1) { // Port B
    if (write)
      Bus_SetIOB(what, push &~ special);

    what = (what & ~pull);

    if (!write)
      what |= Bus_GetIOB(push &~ special) & pull;
  }
  else if (port == 2) { // Port C
    if (write)
      Bus_SetIOC(what, push &~ special);

    what = (what & ~pull);

    if (!write)
      what |= Bus_GetIOC(push &~ special) & pull;
  }

  this.io[0x01 + 5*port] = what;

}


uint16_t Bus_GetIOB(uint16_t mask) {
  return 0x00c8 | this.chipSelectMode;
}


void Bus_SetIOB(uint16_t data, uint16_t mask) {
  // printf("write to IOB (data: %04x, mask: %04x)\n", data, mask);
  if (mask & 7) {
    this.chipSelectMode = (data & 7);
    // printf("chip select set to %d\n", this.chipSelectMode);
  }
}


uint16_t Bus_GetIOC(uint16_t mask) {
  uint16_t data = 0x003f;
  // uint16_t data = 0x0020;
  data |= Controller_GetRequests();

  // printf("read from IOC (%04x) at %06x\n", data, CPU_GetCSPC());

  return data;
}


void Bus_SetIOC(uint16_t data, uint16_t mask) {
  // printf("write to IOC with %04x at %06x\n", data, CPU_GetCSPC());

  if ((mask >> 8) & 1) {
    Controller_SetSelect(0, (data >> 8) & 1);
  }

  if ((mask >> 9) & 1) {
    Controller_SetSelect(1, (data >> 9) & 1);
  }
}


void Bus_WatchdogWakeup(int32_t index) {
  VSmile_Log("Watchdog timer expired. restarting...\n");
  VSmile_Reset();
}


void Bus_WriteADCCtrl(uint16_t data) {
  // printf("write to ADC CTRL with %04x\n", data);
  // printf("interrupt status: %d\n", (data & (1 << 13)) != 0);
  // printf("request conversion: %d\n", (data & (1 << 12)) != 0);
  // printf("auto request: %d\n", (data & (1 << 10)) != 0);
  // printf("interrupt enable: %d\n", (data & (1 << 9)) != 0);
  // printf("VRT enable: %d\n", (data & (1 << 8)) != 0);
  // printf("channel: %d\n", (data >> 4) & 3);
  // printf("clock select: %d\n", 16 << ((data >> 2) & 3));
  // printf("CSB: %d\n", (data & (1 << 1)) != 0);
  // printf("ADE: %d\n\n", (data & (1 << 0)) != 0);

  // this.io[0x25] = data;
  // return;

  uint16_t prevADC = this.io[0x25];
  this.io[0x25] = data & ~0x2000;

  // Reset Interrupt status
  if (data & prevADC & 0x2000) {
    this.io[0x22] &= ~0x2000;
    // printf("resetting interrupt status...\n");
  }

  if (this.io[0x25] & 1) { // ADE
    this.io[0x25] |= 0x2000;
    uint8_t channel = (data >> 4) & 3;

    if ((data & 0x1000) && !(prevADC & 0x1000)) { // Conversion Request
      this.io[0x25] &= ~0x3000;
      this.io[0x27] &= ~0x8000;

      int32_t ticks = 16 << ((data >> 2) & 3);
      Timer_Adjust(this.adcTimers[channel], SYSCLOCK / ticks);
      Timer_Reset(this.adcTimers[channel]);
      // printf("conversion requested\n");
    }

    if (data & 0x0400) { // 8KHz Auto Request
      this.io[0x27] &= ~0x8000;
      Timer_Adjust(this.adcTimers[channel], SYSCLOCK / 8000);
      Timer_Reset(this.adcTimers[channel]);
      // printf("auto request enabled\n");
    }
  } else {
    for (int32_t i = 0; i < 4; i++) {
      Timer_Adjust(this.adcTimers[i], 0);
      Timer_Reset(this.adcTimers[i]);
    }
    // printf("conversion timers disabled\n");
  }
}


void Bus_DoADCConversion(int32_t index) {
  this.io[0x27] = (this.adcValues[index] & 0x0fff) | 0x8000;

  this.io[0x25] |= 0x2000;
  if (this.io[0x25] & 0x0200) {
    Bus_SetIRQFlags(0x3d22, 0x2000);
    // printf("ADC interrupt set\n");
  }

  // printf("ADC conversion request complete. Output: %04x\n", this.io[0x27]);

  if (this.io[0x25] & 0x0400) // Auto Conversion
    Timer_Reset(this.adcTimers[index]);
}


// TODO: CPU-Side UART should probably be delegated to its own file
void Bus_SetUARTCtrl(uint16_t data) {
  // printf("UART CTRL set to %04x at %06x\n", data, CPU_GetCSPC());
  if (!(data & 0x40)) {
    this.rxAvailable = false;
    this.rxBuffer = 0;
  }

  if ((data ^ this.io[UART_CTRL]) & 0x80) {
    if (data & 0x80) {
      this.io[UART_STAT] |= 0x02;
    } else {
      this.io[UART_STAT] &= ~0x42;
      this.txTimer = 0;
    }
  }

  this.io[UART_CTRL] = data;
}


void Bus_SetUARTStat(uint16_t data) {
  // printf("write to UART STAT with %04x at %06x\n", data, CPU_GetCSPC());
  // Unset Rx Ready and Tx Ready
  if (data & 0x01) this.io[UART_STAT] &= ~0x01;
  if (data & 0x02) this.io[UART_STAT] &= ~0x02;
  if (!(this.io[UART_STAT] & 0x03)) // If both Rx Ready and Tx Ready are unset
    Bus_Store(0x3d22, 0x0100);      // Clear the UART IRQ flag
}


void Bus_ResetUART(uint16_t data){
  // if (data & 1) {
  VSmile_Log("UART: reset signal sent at %06x\n", CPU_GetCSPC());
  // }
}


void Bus_SetUARTBAUD(uint16_t data) {
  uint32_t baudRate = SYSCLOCK / (16 * (0x10000 - data));

  if (baudRate != this.baudRate) {
    this.baudRate = baudRate;
    // VSmile_Log("UART: BAUD set to %d at %06x", baudRate, CPU_GetCSPC());
  }
}


void Bus_SetTxBuffer(uint16_t data) {
  // VSmile_Log("UART: Writing 0x%02x at %06x", data & 0xff, CPU_GetCSPC());
  this.txBuffer = data;
  if (this.io[UART_CTRL] & 0x80) { // If Tx is enabled
    this.io[UART_STAT] &= ~0x02;   // Unset Tx Ready
    this.io[UART_STAT] |=  0x40;   // Set Tx Busy
    this.txTimer = this.baudRate;  // Enable Transmit timer
  }
}


uint16_t Bus_GetRxBuffer() {
  if (this.rxAvailable) {        // If there is data available in the Rx buffer
    this.io[UART_STAT] &= ~0x81; // Unset the Rx Buffer full flag and Rx Ready flags
    if (!this.rxEmpty) {
      this.rxBuffer = Bus_PopRx();
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


void Bus_RxTimerReset() {
  this.rxTimer = SYSCLOCK / 960;
}


void Bus_TransmitTick() {
  // VSmile_Warning("UART: transmitting %02x to the controller\n", this.txBuffer);
  Controller_RecieveByte(this.txBuffer);

  this.io[UART_STAT] |=  0x02;
  this.io[UART_STAT] &= ~0x40;
  if (this.io[UART_CTRL] & 0x02) {
    Bus_SetIRQFlags(0x3d22, 0x0100);
  }
}


void Bus_RecieveTick() {
  this.rxAvailable = true;
  // printf("UART: recieving data from controller\n");

  this.io[UART_STAT] |= 0x81;
  if (this.io[UART_CTRL] & 0x01) {
    Bus_SetIRQFlags(0x3d22, 0x0100);
  }
}


bool Bus_PushRx(uint8_t data) {
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


uint8_t Bus_PopRx() {
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
