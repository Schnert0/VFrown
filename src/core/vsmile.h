#ifndef VSMILE_H
#define VSMILE_H

#include "../common.h"

#include "bus.h"
#include "cpu.h"
#include "spu.h"
#include "ppu.h"
#include "hw/controller.h"

/*
 * Connects all components of the VSmile together and is in charge of the
 * initialization, cleanup, and the main emulation loop
 */
typedef struct VSmile_t {
  int32_t cyclesLeft;
  float clockScale;
  bool paused, step, introEnabled, romLoaded, crashed;
} VSmile_t;

bool VSmile_Init();
void VSmile_Cleanup();

void VSmile_RunFrame();
void VSmile_Reset();

void VSmile_SaveState();
void VSmile_LoadState();

void VSmile_LoadROM(const char* path);
void VSmile_LoadSysRom(const char* path);
void VSmile_SetRegion(uint8_t region);
void VSmile_SetIntroEnable(bool shouldShowIntro);
bool VSmile_GetIntroEnable();

bool VSmile_GetPaused();
void VSmile_SetPause(bool isPaused);
float VSmile_GetClockScale();
void VSmile_SetClockScale(float newSpeed);
void VSmile_Step();

void VSmile_Log(const char* message, ...);
void VSmile_Warning(const char* message, ...);
void VSmile_Error(const char* message, ...);

#endif // VSMILE_H
