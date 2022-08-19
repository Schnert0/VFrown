#ifndef TIMER_H
#define TIMER_H

#include "../common.h"

typedef void (*TimerFunc_t)(int32_t index);

/*
 * Reusable timer
 */
typedef struct Timer_t {
  int32_t resetValue;
  int32_t ticksLeft;
  int32_t index;

  TimerFunc_t  callback;
} Timer_t;


struct Timer_t* Timer_Init(int32_t resetValue, TimerFunc_t callback, int32_t index);
void Timer_Cleanup(struct Timer_t* this);

void Timer_Tick(struct Timer_t* this, int32_t cycles);
void Timer_Reset(struct Timer_t* this);
void Timer_Adjust(struct Timer_t* this, int32_t newResetValue);
void Timer_Stop(struct Timer_t* this);

#endif // TIMER_H
