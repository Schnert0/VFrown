#include "timer.h"

struct Timer_t* Timer_Init(int32_t resetValue, TimerFunc_t callback, int32_t index) {
  struct Timer_t* this = malloc(sizeof(Timer_t));
  memset(this, 0, sizeof(Timer_t));
  this->resetValue = resetValue;
  this->callback = callback;
  this->index = index;

  return this;
}


void Timer_Cleanup(struct Timer_t* this) {
  if (this)
    free(this);
}


void Timer_Tick(struct Timer_t* this, int32_t cycles) {
  if (this->ticksLeft > 0) {
    this->ticksLeft -= cycles;
    if (this->ticksLeft <= 0 && this->callback) {
      this->callback(this->index);
    }
  }
}


void Timer_Reset(struct Timer_t* this) {
  this->ticksLeft = this->resetValue;
}


void Timer_Adjust(struct Timer_t* this, int32_t newResetValue) {
  this->resetValue = newResetValue;
  this->ticksLeft = newResetValue;
}


void Timer_Stop(struct Timer_t* this) {
  this->ticksLeft = 0;
}
