#ifndef VSMILE_H
#define VSMILE_H

#include "../common.h"

#include "bus.h"
#include "cpu.h"
#include "spu.h"
#include "ppu.h"
#include "controller.h"
#include "tmb.h"


/*
 * Connects all components of the VSmile together and is in charge of the
 * initialization, cleanup, and the main emulation loop
 */
typedef struct VSmile_t {
  bool paused, step;
} VSmile_t;

bool VSmile_Init();
void VSmile_Cleanup();

void VSmile_Run();
void VSmile_Reset();
void VSmile_LoadROM(const char* path);
void VSmile_LoadBIOS(const char* path);

void VSmile_TogglePause();
void VSmile_Step();

void VSmile_Log(const char* message, ...);
void VSmile_Warning(const char* message, ...);
void VSmile_Error(const char* message, ...);

#endif // VSMILE_H
