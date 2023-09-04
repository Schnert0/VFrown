#include "misc.h"

static Misc_t this;

bool Misc_Init() {
  memset(&this, 0, sizeof(Misc_t));

  this.watchdogTimer = Timer_Init(0, Misc_WatchdogWakeup, 0);

  for (int32_t i = 0; i < 4; i++) {
    this.adcTimers[i] = Timer_Init(0, Misc_DoADCConversion, i);
  }

  return true;
}


void Misc_Cleanup() {
  for (int32_t i = 0; i < 4; i++)
    Timer_Cleanup(this.adcTimers[i]);

  Timer_Cleanup(this.watchdogTimer);
}


void Misc_SaveState() {
  Backend_WriteSave(&this, sizeof(Misc_t));
  Timer_SaveState(this.watchdogTimer);
  for (int32_t i = 0; i < 4; i++) {
    Timer_SaveState(this.adcTimers[i]);
  }
}


void Misc_LoadState() {
  struct Timer_t* adcTimers[4];
  struct Timer_t* watchdogTimer = this.watchdogTimer;
  for (int32_t i = 0; i < 4; i++)
    adcTimers[i] = this.adcTimers[i];
  Backend_ReadSave(&this, sizeof(Misc_t));
  this.watchdogTimer = watchdogTimer;
  Timer_LoadState(this.watchdogTimer);
  for (int32_t i = 0; i < 4; i++) {
    this.adcTimers[i] = adcTimers[i];
    Timer_LoadState(this.adcTimers[i]);
  }
}


void Misc_Reset() {
  this.sysCtrl    = 0x4006;
  this.irqCtrl    = 0x3ffb;
  this.irqStat    = 0x7fff;
  this.extMemCtrl = 0x003e;
  // this.watchdog   = 0xffff;
  this.adcCtrl    = 0x2002;
  this.adcData    = 0x0000;
  this.sleepMode  = 0xffff;
  this.wakeupSrc  = 0x0080;
  this.wakeupTime = 0x00ff;
  this.tvSystem   = 0x0001;
  this.prng[0]    = 0x1418;
  this.prng[1]    = 0x1658;
  this.fiqSelect  = 0x0007;
  CPU_SetDataSegment(0x003f);

  this.adcValues[0] = 0x03ff;
  this.adcValues[1] = 0x03ff;
  this.adcValues[2] = 0x03ff;
  this.adcValues[3] = 0x03ff;

  Timer_Reset(this.watchdogTimer);
  Timer_Reset(this.adcTimers[0]);
  Timer_Reset(this.adcTimers[1]);
  Timer_Reset(this.adcTimers[2]);
  Timer_Reset(this.adcTimers[3]);
}

void Misc_Update(int32_t cycles) {
  // Watchdog
  Timer_Tick(this.watchdogTimer, cycles);

  for (int32_t i = 0; i < cycles; i++) {
    Misc_UpdatePRNG(0);
    Misc_UpdatePRNG(1);
  }

  // ADC Conversion
  Timer_Tick(this.adcTimers[0], cycles);
  Timer_Tick(this.adcTimers[1], cycles);
  Timer_Tick(this.adcTimers[2], cycles);
  Timer_Tick(this.adcTimers[3], cycles);
}


uint16_t Misc_Read(uint16_t addr) {
  switch (addr) {
  case 0x3d20: return this.sysCtrl;
  case 0x3d21: return this.irqCtrl;
  case 0x3d22: return this.irqStat;
  case 0x3d23: return this.extMemCtrl;
  case 0x3d25: return this.adcCtrl;
  case 0x3d26: return this.adcPad;
  case 0x3d27: return this.adcData;
  case 0x3d29: return this.wakeupSrc;
  case 0x3d2a: return this.wakeupTime;
  case 0x3d2b: return this.tvSystem;
  case 0x3d2c: return this.prng[0];
  case 0x3d2d: return this.prng[1];
  case 0x3d2e: return this.fiqSelect;
  case 0x3d2f: return CPU_GetDataSegment();
  default:
    VSmile_Warning("Read from unknown misc. peripheral %04x at %06x", addr, CPU_GetCSPC());
  }

  return 0x0000;
}


void Misc_Write(uint16_t addr, uint16_t data) {
  switch (addr) {
  case 0x3d20: this.sysCtrl = data;          return;
  case 0x3d21: this.irqCtrl = data; CPU_ActivatePendingIRQs(); return;
  case 0x3d22: this.irqStat &= ~data;        return;
  case 0x3d23: Misc_SetExtMemoryCtrl(data);  return;
  case 0x3d24: Misc_ClearWatchdog(data);     return;
  case 0x3d25: Misc_WriteADCCtrl(data);      return;
  case 0x3d26: Misc_DisableADCPad(data);     return;
  case 0x3d28: this.sleepMode = data;        return;
  case 0x3d29: this.wakeupSrc = data;        return;
  case 0x3d2a: this.wakeupTime = data;       return;
  case 0x3d2c: this.prng[0] = data & 0x7fff; return;
  case 0x3d2d: this.prng[1] = data & 0x7fff; return;
  case 0x3d2e: this.fiqSelect = data & 7;    return;
  case 0x3d2f: CPU_SetDataSegment(data);     return;
  default:
    VSmile_Warning("Write to unknown misc. peripheral %04x with %04x at %06x", addr, data, CPU_GetCSPC());
  }
}

void Misc_SetIRQFlags(uint16_t addr, uint16_t data) {
  if (addr == 0x3d22) {
    this.irqStat |= data;
    CPU_ActivatePendingIRQs();
  } else if (addr == 0x2863) {
    PPU_SetIRQFlags(data);
  } else {
    VSmile_Warning("unknown IRQ Acknowledge for %04x with data %04x", addr, data);
  }

  // switch (address) {
  // case 0x2863:
  //   this.ppu[0x63] |= data;
  //   break;
  // case 0x3d22:
  //   this.io[0x22] |= data;
  //   break;
  // default:
  //   VSmile_Warning("unknown IRQ Acknowledge for %04x with data %04x", address, data);
  // }
  //
  // CPU_ActivatePendingIRQs(); // Notify CPU that there might be IRQ's to handle
}


void Misc_UpdatePRNG(uint8_t index) {
  uint16_t a = (this.prng[index] >> 14) & 1;
  uint16_t b = (this.prng[index] >> 13) & 1;
  this.prng[index] = ((this.prng[index] << 1) | (a ^ b)) & 0x7fff;
}


void Misc_SetExtMemoryCtrl(uint16_t data) {
  Bus_SetRomDecode((data >> 6) & 0x3);
  Bus_SetRamDecode((data >> 8) & 0xf);

  if ((data ^ this.extMemCtrl) & 0x8000) {
    if (data & 0x8000)
      Timer_Adjust(this.watchdogTimer, (SYSCLOCK/4)*3); // 750 ms
    else
      Timer_Adjust(this.watchdogTimer, 0);

    Timer_Reset(this.watchdogTimer);
  }

  this.extMemCtrl = data;
}


void Misc_ClearWatchdog(uint16_t data) {
  if (data == 0x55aa && (this.extMemCtrl & 0x8000)) {
    Timer_Adjust(this.watchdogTimer, (SYSCLOCK/4)*3); // 750 ms
    Timer_Reset(this.watchdogTimer);
    VSmile_Log("Watchdog reset");
  } else if (data != 0x55aa) {
    VSmile_Warning("Unexpected write to Watchdog clear (expected 0x55aa), got 0x%04x", data);
  }
}


void Misc_WatchdogWakeup(int32_t index) {
  VSmile_Log("Watchdog timer expired. restarting...");
  VSmile_Reset();
}


void Misc_WriteADCCtrl(uint16_t data) {
  // printf("write to ADC CTRL with %04x at %06x\n", data, CPU_GetCSPC());
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

  uint16_t prevADC = this.adcCtrl;
  this.adcCtrl = data & ~0x2000;

  // Reset Interrupt status
  if (data & prevADC & 0x2000) {
    this.adcCtrl &= ~0x2000;
    Bus_Store(0x3d22, 0x2000); // Reset interrupt in IRQ
    // printf("resetting interrupt status...\n");
  }

  if (this.adcCtrl & 1) { // ADE
    // printf("ADE\n");
    this.adcCtrl |= 0x2000;
    uint8_t channel = (data >> 4) & 3;

    if ((this.adcCtrl & 0x1000) && !(prevADC & 0x1000)) { // Conversion Request
      this.adcCtrl &= ~0x3000;
      this.adcData &= ~0x8000; // Clear ready bit

      uint32_t ticks = 16 << ((data >> 2) & 3);
      Timer_Adjust(this.adcTimers[channel], ticks);
      Timer_Reset(this.adcTimers[channel]);
      // printf("conversion requested\n");
    }

    if (data & 0x0400) { // 8KHz Auto Request
      this.adcData &= ~0x8000; // Clear ready bit
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


void Misc_DoADCConversion(int32_t index) {
  this.adcData = (this.adcValues[index] & 0x0fff) | 0x8000;

  this.adcCtrl |= 0x2000;
  if (this.adcCtrl & 0x0200) {
    Misc_SetIRQFlags(0x3d22, 0x2000);
    // printf("ADC interrupt set\n");
  }

  // printf("ADC conversion request complete. Output: %04x\n", this.io[0x27]);

  if (this.adcCtrl & 0x0400) // Auto Conversion
    Timer_Reset(this.adcTimers[index]);
}


void Misc_DisableADCPad(uint16_t data) {
  this.adcPad = data;
  for (int32_t i = 0; i < 4; i++) {
    if (data & (1 << i)) {
      Timer_Adjust(this.adcTimers[i], 0);
      Timer_Reset(this.adcTimers[i]);
    }
  }
}
