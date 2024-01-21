#ifndef UI_H
#define UI_H

#include "../common.h"
#include "../core/vsmile.h"
#include "backend.h"

#include "libs.h"
#include "lib/sokol_app.h"

#include "userSettings.h"

typedef struct {
  void* next;
  char* name;
  uint16_t value;
  bool isFrozen;
} MemWatchValue_t;

// typedef struct {
// } UI_t;

bool UI_Init();
void UI_Cleanup();
void UI_HandleEvent(sapp_event* event);
void UI_StartFrame();
void UI_RunFrame();
void UI_Render();
void UI_Toggle();

#endif // UI_H
