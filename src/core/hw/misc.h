#ifndef MISC_H
#define MISC_H

#include "../../common.h"
#include "../vsmile.h"

#define MISC_START 0x3d20
#define MISC_SIZE  0x0010


typedef struct {
  struct Timer_t* watchdogTimer;
  struct Timer_t* adcTimers[4];
  uint16_t adcValues[4];

  uint16_t sysCtrl;    // 0x3d20
  uint16_t irqCtrl;    // 0x3d21
  uint16_t irqStat;    // 0x3d22
  uint16_t extMemCtrl; // 0x3d23
  uint16_t adcCtrl;    // 0x3d25
  uint16_t adcPad;     // 0x3d26
  uint16_t adcData;    // 0x3d27
  uint16_t sleepMode;  // 0x3d28
  uint16_t wakeupSrc;  // 0x3d29
  uint16_t wakeupTime; // 0x3d2a
  uint16_t tvSystem;   // 0x3d2b
  uint16_t prng[2];    // 0x3d2c - 0x3d2d
  uint16_t fiqSelect;  // 0x3d2e
} Misc_t;

bool Misc_Init();
void Misc_Cleanup();

void Misc_Reset();
void Misc_Update(int32_t cycles);

uint16_t Misc_Read(uint16_t addr);
void Misc_Write(uint16_t addr, uint16_t write);

void Misc_SetIRQFlags(uint16_t addr, uint16_t data);
void Misc_UpdatePRNG(uint8_t index);
void Misc_SetExtMemoryCtrl(uint16_t data);

void Misc_ClearWatchdog(uint16_t data);
void Misc_WatchdogWakeup(int32_t index);

void Misc_WriteADCCtrl(uint16_t data);
void Misc_DoADCConversion(int32_t index);
void Misc_DisableADCPad(uint16_t data);

#endif // INTERRUPTS_H
