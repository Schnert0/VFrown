#ifndef BACKEND_H
#define BACKEND_H

#include "../common.h"

#define BACKENDIMPL_SDL
#include "sdl.h"

bool Backend_Init();
void Backend_Cleanup();

// Window
void Backend_UpdateWindow();
uint16_t* Backend_GetScanlinePointer(uint16_t scanlineNum);

// Input
bool Backend_GetInput();
uint32_t Backend_GetButtonStates();
uint32_t Backend_GetChangedButtons();

// Audio
void Backend_InitAudioDevice();
void Backend_PushAudioSample(int16_t leftSample, int16_t rightSample);
void Backend_PlayAudio();

#endif // BACKEND_H
