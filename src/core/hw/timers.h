#ifndef TIMERS_H
#define TIMERS_H

#include "../../common.h"
#include "../vsmile.h"

#define TIMERS_START 0x3d10
#define TIMERS_SIZE  0x0010


enum {
  TIMERS_A,
  TIMERS_B,

  NUM_ABTIMERS
};


typedef struct TMB_t {
  int32_t       time;
  uint32_t      interval;
} TMB_t;


typedef struct {
  uint16_t data;
  uint16_t ctrl;
  uint16_t enable;
} ABTimer_t;


typedef struct {
  struct Timer_t* sysTimers;
  uint16_t timer2khz, timer1khz, timer4hz;

  struct Timer_t* tmb[2];
  uint16_t timebaseSetup;

  struct Timer_t* timerABSource;
  struct Timer_t* timerCSource;
  uint16_t timerASetup, timerBSetup, timerBRate, timerBDiv;

  ABTimer_t timers[NUM_ABTIMERS]; // 0x3d12 - 0x3d19
} Timers_t;


bool Timers_Init();
void Timers_Cleanup();

void Timers_Reset();
void Timers_Update();

uint16_t Timers_Read(uint16_t addr);
void Timers_Write(uint16_t addr, uint16_t data);

void Timers_SetTimebase(uint16_t data);
void Timers_SetTimerACTRL(uint16_t data);
void Timers_UpdateTimerB();

void Timers_TickSysTimers();
void Timers_TickTMB();
void Timers_TickAB();
void Timers_TickA();
void Timers_TickC();


#endif // TIMERS_H
