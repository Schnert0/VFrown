#include "timers.h"

static Timers_t this;

bool Timers_Init() {
  memset(&this, 0, sizeof(Timers_t));

  this.sysTimers = Timer_Init(SYSCLOCK / 4096, Timers_TickSysTimers, 0);
  if (!this.sysTimers)
    return false;

  this.tmb[0] = Timer_Init(SYSCLOCK / 128, Timers_TickTMB, 0);
  if (!this.tmb[0])
    return false;

  this.tmb[1] = Timer_Init(SYSCLOCK / 8, Timers_TickTMB, 1);
  if (!this.tmb[1])
    return false;

  this.timerABSource = Timer_Init(0, Timers_TickAB, 0);
  if (!this.timerABSource)
    return false;

  this.timerCSource = Timer_Init(0, Timers_TickC, 0);
  if (!this.timerCSource)
    return false;

  Timer_Reset(this.sysTimers);
  Timer_Reset(this.tmb[0]);
  Timer_Reset(this.tmb[1]);

  return true;
}

void Timers_Cleanup() {
  if (this.timerABSource)
    Timer_Cleanup(this.timerCSource);

  if (this.timerABSource)
    Timer_Cleanup(this.timerABSource);

  if (this.tmb[1])
    Timer_Cleanup(this.tmb[1]);

  if (this.tmb[0])
    Timer_Cleanup(this.tmb[0]);

  if (this.sysTimers)
    Timer_Cleanup(this.sysTimers);
}


void Timers_SaveState() {
  Backend_WriteSave(&this, sizeof(Timers_t));
  Timer_SaveState(this.sysTimers);
  Timer_SaveState(this.tmb[0]);
  Timer_SaveState(this.tmb[1]);
  Timer_SaveState(this.timerABSource);
  Timer_SaveState(this.timerCSource);
}


void Timers_LoadState() {
  struct Timer_t* tmb[2];
  struct Timer_t* sysTimers = this.sysTimers;
  tmb[0] = this.tmb[0];
  tmb[1] = this.tmb[1];
  struct Timer_t* timerABSource = this.timerABSource;
  struct Timer_t* timerCSource = this.timerCSource;
  Backend_ReadSave(&this, sizeof(Timers_t));
  this.sysTimers = sysTimers;
  this.tmb[0] = tmb[0];
  this.tmb[1] = tmb[1];
  this.timerABSource = timerABSource;
  this.timerCSource = timerCSource;
  Timer_LoadState(this.sysTimers);
  Timer_LoadState(this.tmb[0]);
  Timer_LoadState(this.tmb[1]);
  Timer_LoadState(this.timerABSource);
  Timer_LoadState(this.timerCSource);
}


void Timers_Reset() {
  Timer_Adjust(this.tmb[0], SYSCLOCK / 128);
  Timer_Adjust(this.tmb[1], SYSCLOCK / 8);
  Timer_Adjust(this.timerABSource, 0);
  Timer_Adjust(this.timerCSource,  0);

  Timer_Reset(this.sysTimers);
  Timer_Reset(this.tmb[0]);
  Timer_Reset(this.tmb[1]);
  Timer_Reset(this.timerABSource);
  Timer_Reset(this.timerCSource);

  this.timebaseSetup = 0x000f;
}


void Timers_Update(int32_t cycles) {
  Timer_Tick(this.sysTimers,     cycles);
  Timer_Tick(this.tmb[0],        cycles);
  Timer_Tick(this.tmb[1],        cycles);
  Timer_Tick(this.timerABSource, cycles);
  Timer_Tick(this.timerCSource,  cycles);
}


uint16_t Timers_Read(uint16_t addr) {
  switch (addr) {
  case 0x3d10: return this.timebaseSetup;
  case 0x3d12: return this.timers[TIMERS_A].data;
  case 0x3d13: return this.timers[TIMERS_A].ctrl;
  case 0x3d14: return this.timers[TIMERS_A].enable;
  case 0x3d16: return this.timers[TIMERS_B].data;
  case 0x3d17: return this.timers[TIMERS_B].ctrl;
  case 0x3d18: return this.timers[TIMERS_B].enable;
  case 0x3d1c: return PPU_GetCurrLine();
  default: VSmile_Warning("unknown read from Timers address %04x\n", addr);
  }

  return 0x0000;
}


void Timers_Write(uint16_t addr, uint16_t data) {
  switch (addr) {
  case 0x3d10:
    Timers_SetTimebase(data);
    return;

  case 0x3d11:
    // printf("timebase clear\n");
    this.timer2khz = 0;
    this.timer1khz = 0;
    this.timer4hz  = 0;
    return;

  case 0x3d12:
    // printf("timer A Data set to %04x at %06x\n", data, CPU_GetCSPC());
    this.timers[TIMERS_A].data = data;
    this.timerASetup = data;
    return;

  case 0x3d13:
    Timers_SetTimerACTRL(data);
    return;

  case 0x3d14:
    // printf("Timer A Enable set to %04x at %04x\n", data, CPU_GetCSPC());
    this.timers[TIMERS_A].enable = data;
    return;

  case 0x3d15:
    Bus_Store(0x3d22, 0x0800);
    return;

  case 0x3d16:
    // printf("timer B data set to %04x at %06x\n", data, CPU_GetCSPC());
    this.timers[TIMERS_B].data = data;
    this.timerBSetup = data;
    return;

  case 0x3d17:
    // printf("timer B CTRL set to %04x at %06x\n", data, CPU_GetCSPC());
    this.timers[TIMERS_B].ctrl = data;
    if (data == 1)
      Timers_UpdateTimerB();
    return;

  case 0x3d18:
    // printf("timer B enable set to %04x at %06x\n", data, CPU_GetCSPC());
    this.timers[TIMERS_B].enable = data & 1;
    if (data & 1) {
      Timers_UpdateTimerB();
    } else {
      Timer_Adjust(this.timerCSource, 0);
      Timer_Reset(this.timerCSource);
    }
    return;

  case 0x3d19:
    Bus_Store(0x3d22, 0x0400);
    return;

  default:
    VSmile_Warning("unknown read from Timers address %04x\n", addr);
  }
}


void Timers_SetTimebase(uint16_t data) {
  // printf("timebase set to %04x\n", data);

  if ((this.timebaseSetup & 0x3) != (data & 0x3)) {
    uint16_t hz = 8 << (data & 0x3);
    Timer_Adjust(this.tmb[0], SYSCLOCK / hz);
    Timer_Reset(this.tmb[0]);
    // printf("[BUS] TMB1 frequency set to %d Hzn", hz);
  }

  if ((this.timebaseSetup & 0xc) != (data & 0xc)) {
    uint16_t hz = 128 << ((data & 0xc) >> 2);
    Timer_Adjust(this.tmb[1], SYSCLOCK / hz);
    Timer_Reset(this.tmb[1]);
    // printf("[BUS] TMB2 freqency set to %d Hz", hz);
  }

  this.timebaseSetup = data;
}


void Timers_SetTimerACTRL(uint16_t data) {
  // printf("timer A CTRL set to %04x at %06x\n", data, CPU_GetCSPC());
  uint32_t timerARate = 0;
  switch (data & 7) {
  case 2:
    Timer_Adjust(this.timerABSource, SYSCLOCK / 32768);
    timerARate = 32768;
    break;

  case 3:
    Timer_Adjust(this.timerABSource, SYSCLOCK / 8192);
    timerARate = 8192;
    break;

  case 4:
    Timer_Adjust(this.timerABSource, SYSCLOCK / 4096);
    timerARate = 4096;
    break;

  default:
    Timer_Adjust(this.timerABSource, 0);
    break;
  }

  Timer_Reset(this.timerABSource);

  switch ((data >> 3) & 7) {
  case 0: this.timerBRate = timerARate / 2048; break;
  case 1: this.timerBRate = timerARate / 1024; break;
  case 2: this.timerBRate = timerARate /  256; break;
  case 3: this.timerBRate = 0;                 break;
  case 4: this.timerBRate = timerARate /    4; break;
  case 5: this.timerBRate = timerARate /    2; break;
  case 6: this.timerBRate = 1;                 break;
  case 7: this.timerBRate = 0;                 break;
  }
}

void Timers_UpdateTimerB() {
  switch (this.timers[TIMERS_B].ctrl) {
  case 2:  Timer_Adjust(this.timerCSource, SYSCLOCK / 32768); break;
  case 3:  Timer_Adjust(this.timerCSource, SYSCLOCK /  8192); break;
  case 4:  Timer_Adjust(this.timerCSource, SYSCLOCK /  4096); break;
  default: Timer_Adjust(this.timerCSource, 0);                break;
  }

  Timer_Reset(this.timerCSource);
  // printf("update timer B\n");
}


void Timers_TickSysTimers() {
  // printf("System Timers Tick\n");

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

  // printf("%04x\n", timerIrq);

  Misc_SetIRQFlags(0x3d22, timerIrq);

  Timer_Reset(this.sysTimers);
}


void Timers_TickTMB(uint32_t index) {
  // printf("TMB%d Tick\n", index+1);
  Misc_SetIRQFlags(0x3d22, 1 << index);

  Timer_Reset(this.tmb[index]);
}


void Timers_TickAB() {
  // printf("timer AB Tick\n");
  if (this.timerBRate) {
    this.timerBDiv++;
    if (this.timerBDiv >= this.timerBRate) {
      this.timerBDiv = 0;
      Timers_TickA();
    }
  }

  Timer_Reset(this.timerABSource);
}


void Timers_TickA() {
  // printf("timer A Tick\n");
  this.timers[TIMERS_A].data++; // Timer A Data
  if (!this.timers[TIMERS_A].data) {
      this.timers[TIMERS_A].data = this.timerASetup;
      Misc_SetIRQFlags(0x3d22, 0x0800);
  }
}


void Timers_TickC() {
  // printf("timer C Tick\n");
  this.timers[TIMERS_B].data++; // Timer B Data
  if (!this.timers[TIMERS_B].data) {
    this.timers[TIMERS_B].data = this.timerBSetup;
    Misc_SetIRQFlags(0x3d22, 0x0400);
  }

  Timer_Reset(this.timerCSource);
}
