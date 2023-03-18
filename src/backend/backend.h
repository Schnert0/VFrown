#ifndef BACKEND_H
#define BACKEND_H

#ifndef DNDEBUG
  #define SOKOL_LOG(s) VSmile_Error(s);
#endif // DNDEBUG

#include "lib/nuklear_def.h"

#include "font.xpm"

#include "../common.h"
#include "../core/vsmile.h"

#include "lib/sokol_app.h"
#include "lib/sokol_audio.h"
#include "lib/sokol_gfx.h"
#include "lib/sokol_glue.h"
#include "lib/sokol_gp.h"
#include "lib/sokol_nuklear.h"

typedef struct {
  float* sampleBuffer;
  int32_t* sampleCount;
  FILE* saveFile;

  float emulationSpeed;
  uint32_t pixelBuffer[240][320];
  int32_t currSampleX[16];
  int16_t prevSample[16];
  uint32_t drawColor;
  uint32_t currButtons;
  uint32_t prevButtons;
  uint32_t currLed;
  char title[256];
  bool showLeds;
  bool oscilloscopeEnabled;
} Backend_t;

bool Backend_Init();
void Backend_Cleanup();
void Backend_Update();

// Save states
void Backend_GetFileName(const char* path);
void Backend_WriteSave(void* data, uint32_t size);
void Backend_ReadSave(void* data, uint32_t size);
void Backend_SaveState();
void Backend_LoadState();

float Backend_GetSpeed();
void Backend_SetSpeed(float newSpeed);

// Window
uint32_t* Backend_GetScanlinePointer(uint16_t scanlineNum);
bool Backend_RenderScanline();

// Input
bool Backend_GetInput();
uint32_t Backend_GetButtonStates();
uint32_t Backend_GetChangedButtons();
void Backend_HandleInput(int32_t keycode, int32_t eventType);

// Leds
uint8_t Backend_SetLedStates(uint8_t state);
void Backend_RenderLeds();
void Backend_ShowLeds(bool shouldShowLeds);

// Audio
void Backend_InitAudioDevice(float* buffer, int32_t* count);
void Backend_PushBuffer();
void Backend_PushOscilloscopeSample(uint8_t ch, int16_t sample);
bool Backend_GetOscilloscopeEnabled();
void Backend_SetOscilloscopeEnabled(bool shouldShow);

// Drawing
void Backend_SetDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void Backend_SetDrawColor32(uint32_t color);
void Backend_SetPixel(int32_t x, int32_t y);
void Backend_DrawCircle(int32_t x, int32_t y, uint32_t radius);
void Backend_DrawText(int32_t x, int32_t y, const char* text, ...);
void Backend_DrawChar(int32_t x, int32_t y, char c);

#endif // BACKEND_H
