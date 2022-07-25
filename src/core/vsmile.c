#include "vsmile.h"

static VSmile_t this;

bool VSmile_Init() {
  if (!Backend_Init()) return false;

  if (!Bus_Init()) return false;
  if (!CPU_Init()) return false;
  if (!PPU_Init()) return false;
  if (!SPU_Init()) return false;
  if (!Controller_Init()) return false;

  if (!TMB_Init(0, SYSCLOCK / 128)) return false;
  if (!TMB_Init(1, SYSCLOCK / 8))   return false;

  return true;
}


void VSmile_Cleanup() {
  TMB_Cleanup(1);
  TMB_Cleanup(0);

  Controller_Cleanup();
  PPU_Cleanup();
  SPU_Cleanup();
  CPU_Cleanup();
  Bus_Cleanup();

  Backend_Cleanup();
}


void VSmile_Run() {
  int32_t cyclesLeft = 0;
  while (Backend_GetInput()) {
    if (!this.paused || this.step) {
      cyclesLeft += CYCLES_PER_LINE;
      while (cyclesLeft > 0) {
        int32_t cycles = CPU_Tick();
        cyclesLeft -= cycles;

        SPU_Tick(cycles);
        Bus_UARTTick(cycles);
        Controller_Tick(cycles);
        TMB_Tick(0, cycles);
        TMB_Tick(1, cycles);
      }
      CPU_TestIRQ();

      PPU_RenderLine();

      uint16_t currLine = PPU_GetCurrLine();

      if (currLine == 240) {
        Bus_SetIRQFlags(0x2863, 0x0001);
      }

      if (currLine == Bus_Load(0x2836)) {
        Bus_SetIRQFlags(0x2863, 0x0002);
      }

      if (currLine >= LINES_PER_FIELD) {
        PPU_UpdateScreen();
        this.step = false;
      }

    } else {
      PPU_UpdateScreen();
    }
  }
}


void VSmile_Reset() {
  Bus_Reset();
  CPU_Reset();
  PPU_Reset();
}


void VSmile_LoadROM(const char* path) {
  Bus_LoadROM(path);
}


void VSmile_TogglePause() {
  this.paused ^= true;
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
    CPU_PrintCPUState();
  }

  VSmile_Cleanup();
  exit(0);
}
