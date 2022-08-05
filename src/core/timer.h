#ifndef TIMER_H
#define TIMER_H

#include "../common.h"

typedef void (*Func_t)();

/*
 * Reusable timer
 */
typedef struct Timer_t {
  int32_t resetValue;
  int32_t ticksLeft;
  Func_t  callback;
} Timer_t;


struct Timer_t* Timer_Init(int32_t resetValue, Func_t callback);
void Timer_Cleanup(struct Timer_t* this);

void Timer_Tick(struct Timer_t* this, int32_t cycles);
void Timer_Reset(struct Timer_t* this);
void Timer_Adjust(struct Timer_t* this, int32_t newResetValue);

#endif // TIMER_H
