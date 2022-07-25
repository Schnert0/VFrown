#include "tmb.h"

static TMB_t tmb[2];

bool TMB_Init(uint32_t index, uint32_t interval) {
  TMB_t* this = &tmb[index];

  memset(tmb, 0, sizeof(TMB_t));
  this->interval = interval;
  this->time = interval;

  return true;
}


void TMB_Cleanup(uint32_t index) {
  // TMB_t* this = &tmb[index];
}


void TMB_Tick(uint32_t index, int32_t cycles) {
  TMB_t* this = &tmb[index];

  this->time -= cycles;
  if(this->time <= 0) {
    Bus_SetIRQFlags(0x3d22, 1 << index);
    this->time += this->interval;
  }
}


void TMB_SetInterval(uint32_t index, uint32_t interval) {
  TMB_t* this = &tmb[index];

  this->interval = interval;
  this->time = interval;
}
