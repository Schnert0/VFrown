#ifndef BACKEND_H
#define BACKEND_H

#include "lib/nuklear_def.h"

#include "../common.h"
#include "../core/vsmile.h"

#include "lib/sokol_app.h"
#include "lib/sokol_audio.h"
#include "lib/sokol_gfx.h"
#include "lib/sokol_glue.h"
#include "lib/sokol_gp.h"
#include "lib/sokol_nuklear.h"

bool Backend_Init();
void Backend_Cleanup();
void Backend_Update();

// Window
uint32_t* Backend_GetScanlinePointer(uint16_t scanlineNum);
bool Backend_RenderScanline();

// Input
bool Backend_GetInput();
uint32_t Backend_GetButtonStates();
uint32_t Backend_GetChangedButtons();

// Leds
uint32_t Backend_SetLedStates(uint8_t state);

// Audio
void Backend_InitAudioDevice(float* buffer, int32_t* count);
void Backend_PushBuffer();
void Backend_PushOscilloscopeSample(uint8_t ch, int16_t sample);
void Backend_HandleInput(int32_t keycode, int32_t eventType);

#endif // BACKEND_H
