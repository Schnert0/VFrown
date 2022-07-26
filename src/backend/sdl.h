#ifndef BACKEND_SDL_H
#define BACKEND_SDL_H

#include "../common.h"
#include "backend.h"
#include <SDL2/SDL.h>
#include "../core/vsmile.h"

bool SDLBackend_Init();
void SDLBackend_Cleanup();

// Window
void SDLBackend_UpdateWindow();
uint16_t* SDLBackend_GetScanlinePointer(uint16_t scanlineNum);
void SDLBackend_ToggleFullscreen();

// Input
bool SDLBackend_GetInput();
uint32_t SDLBackend_GetButtonStates();
uint32_t SDLBackend_GetChangedButtons();

// Audio
void SDLBackend_InitAudioDevice();
void SDLBackend_PushAudioSample(int16_t leftSample, int16_t rightSample);
void SDLBackend_PlayAudio();


typedef struct SDLBackend_t {
  // Window
  SDL_Window*  window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Surface* surface;
  uint16_t* pixels;
  int32_t msCount;
  bool isFullscreen;

  // Input
  uint16_t curr, prev;

  // Audio
  SDL_AudioDeviceID audioDevice;
  int16_t  audioBuffer[65536];
  uint16_t audioLen;
} SDLBackend_t;

#endif // BACKEND_SDL_H
