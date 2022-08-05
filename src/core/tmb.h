#ifndef TMB_H
#define TMB_H

#include "../common.h"
#include "vsmile.h"

/*
 * Emulates the TMB interrupt timers
 */
typedef struct TMB_t {
  int32_t       time;
  uint32_t      interval;
} TMB_t;

bool TMB_Init(uint32_t index, uint32_t interval);
void TMB_Cleanup(uint32_t index);
void TMB_Tick(uint32_t index, int32_t cycles);
void TMB_SetInterval(uint32_t index, uint32_t interval);

#endif // TMB_H
