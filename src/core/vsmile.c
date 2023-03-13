#include "vsmile.h"

bool VSmile_IntroEnabled = true;

static VSmile_t this;

bool VSmile_Init() {
  if (!Bus_Init()) return false;
  if (!CPU_Init()) return false;
  if (!PPU_Init()) return false;
  if (!SPU_Init()) return false;
  if (!Controller_Init()) return false;

  this.clockScale = 1.0f;

  return true;
}


void VSmile_Cleanup() {
  Controller_Cleanup();
  PPU_Cleanup();
  SPU_Cleanup();
  CPU_Cleanup();
  Bus_Cleanup();
}


void VSmile_RunFrame() {
  if (this.paused)
    return;

  while (true) {
    this.cyclesLeft += CYCLES_PER_LINE;
    while (this.cyclesLeft > 0) {
      int32_t cycles = CPU_Tick();
      SPU_Tick(cycles);
      this.cyclesLeft -= cycles;
    }

    // Tick these every scan line instead of every cycle.
    // Even though it's slightly less accurate, it's waaaay more efficient this way.
    Bus_Update(CYCLES_PER_LINE-this.cyclesLeft);
    Controller_Tick(CYCLES_PER_LINE-this.cyclesLeft);
    if (PPU_RenderLine())
      break;
  }
  Backend_PushBuffer();
}


void VSmile_Reset() {
  Bus_Reset();
  CPU_Reset();
  PPU_Reset();
}


void VSmile_LoadROM(const char* path) {
  Bus_LoadROM(path);
}


void VSmile_LoadSysRom(const char* path) {
  Bus_LoadSysRom(path);
}


void VSmile_SetRegion(uint8_t region) {
  GPIO_SetRegion(region);
}

void VSmile_SetIntroEnable(bool shouldShowIntro) {
  VSmile_IntroEnabled = shouldShowIntro;
  GPIO_SetIntroEnable(shouldShowIntro);
}

bool VSmile_GetIntroEnable() {
  return VSmile_IntroEnabled;
}


bool VSmile_GetPaused() {
  return this.paused;
}


void VSmile_SetPause(bool isPaused) {
  this.paused = isPaused;
}


float VSmile_GetClockScale() {
  return this.clockScale;
}


void VSmile_SetClockScale(float newScale) {
  this.clockScale = newScale;
}


void VSmile_Step() {
  this.paused = true;
  this.step = true;
}


void VSmile_Log(const char* message, ...) {
  if(message == NULL) {
    printf("[LOG] unknown log with NULL pointer thrown\n");
    return;
  }

  char buffer[256];
  va_list args;
  va_start(args, message);
  vsnprintf(buffer, sizeof(buffer), message, args);
  va_end(args);

  printf("[LOG] %s\n", buffer);
}


void VSmile_Warning(const char* message, ...) {
  if(message == NULL) {
    printf("\x1b[33m[WARNING]\x1b[0m unknown warning with NULL pointer thrown\n");
    return;
  }

  char buffer[256];
  va_list args;
  va_start(args, message);
  vsnprintf(buffer, sizeof(buffer), message, args);
  va_end(args);

  printf("\x1b[33m[WARNING]\x1b[0m %s\n", buffer);
}


void VSmile_Error(const char* message, ...) {
  if(message == NULL){
    printf("\x1b[31m[ERROR]\x1b[0m unknown error with NULL pointer thrown\n");
  } else {
    char buffer[256];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    printf("\x1b[31m[ERROR]\x1b[0m %s\n", buffer);
    // CPU_PrintCPUState();
  }

  VSmile_Cleanup();
  exit(0);
}
