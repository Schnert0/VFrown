#ifndef UI_H
#define UI_H

#include "../common.h"
#include "../core/vsmile.h"
#include "backend.h"

#include "libs.h"
#include "lib/microui.h"
#include "lib/sokol_gfx.h"
#ifndef SOKOL_APP_IMPLEMENTATION
#include "lib/sokol_app.h"
#endif
#include "lib/sokol_gl.h"


typedef struct {
  mu_Context ctx;
  mu_Container window;
  sg_image atlasImage;
  bool textureEnabled;
} UI_t;

bool UI_Init();
void UI_Cleanup();
void UI_HandleEvent(sapp_event* event);
void UI_RunFrame();

#endif // UI_H
