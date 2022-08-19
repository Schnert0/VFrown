#ifndef BACKEND_SDL_H
#define BACKEND_SDL_H

#include "../common.h"
#include "backend.h"
#include <SDL2/SDL.h>
#include "../core/vsmile.h"

typedef struct SDLBackend_t {
  // Window
  SDL_Window*  window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Surface* surface;
  uint16_t* pixels;
  int32_t msCount;
  bool isFullscreen;

  // Timing
  clock_t initial, final;

  // Input
  uint16_t curr, prev;

  // Audio
  SDL_AudioDeviceID audioDevice;
  int16_t  audioBuffer[2];
  uint16_t audioLen;

  SDL_Texture* oscilloscopeTexture;
  uint16_t* oscilloscopePixels[16];
  int16_t prevSample[16];
  int16_t currOscilloscopeSample;
  bool isOscilloscopeView;
} SDLBackend_t;

bool SDLBackend_Init();
void SDLBackend_Cleanup();

// Window
void SDLBackend_UpdateWindow();
uint16_t* SDLBackend_GetScanlinePointer(uint16_t scanlineNum);
bool SDLBackend_RenderScanline();
void SDLBackend_ToggleFullscreen();

// Input
bool SDLBackend_GetInput();
uint32_t SDLBackend_GetButtonStates();
uint32_t SDLBackend_GetChangedButtons();

// Audio
void SDLBackend_InitAudioDevice();
void SDLBackend_PushAudioSample(int16_t leftSample, int16_t rightSample);
void SDLBackend_PushOscilloscopeSample(uint8_t ch, int16_t sample);

#endif // BACKEND_SDL_H
