#include "backend.h"

bool Backend_Init() {
  #ifdef BACKENDIMPL_SDL
  return SDLBackend_Init();
  #endif // BACKENDIMPL_SDL
}


void Backend_Cleanup() {
  #ifdef BACKENDIMPL_SDL
  SDLBackend_Cleanup();
  #endif // BACKENDIMPL_SDL
}


void Backend_UpdateWindow() {
  #ifdef BACKENDIMPL_SDL
  SDLBackend_UpdateWindow();
  #endif // BACKENDIMPL_SDL
}


uint16_t* Backend_GetScanlinePointer(uint16_t scanlineNum) {
  #ifdef BACKENDIMPL_SDL
  return SDLBackend_GetScanlinePointer(scanlineNum);
  #endif // BACKENDIMPL_SDL
}


bool Backend_GetInput() {
  #ifdef BACKENDIMPL_SDL
  return SDLBackend_GetInput();
  #endif // BACKENDIMPL_SDL
}


uint32_t Backend_GetButtonStates() {
  #ifdef BACKENDIMPL_SDL
  return SDLBackend_GetButtonStates();
  #endif // BACKENDIMPL_SDL
}


uint32_t Backend_GetChangedButtons() {
  #ifdef BACKENDIMPL_SDL
  return SDLBackend_GetChangedButtons();
  #endif // BACKENDIMPL_SDL
}


void Backend_InitAudioDevice() {
  #ifdef BACKENDIMPL_SDL
  SDLBackend_InitAudioDevice();
  #endif // BACKENDIMPL_SDL
}


void Backend_PushAudioSample(int16_t leftSample, int16_t rightSample) {
  #ifdef BACKENDIMPL_SDL
  SDLBackend_PushAudioSample(leftSample, rightSample);
  #endif // BACKENDIMPL_SDL
}
