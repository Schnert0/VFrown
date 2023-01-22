#ifndef UI_H
#define UI_H

#include "../common.h"
#include "../core/vsmile.h"
#include "backend.h"
#include "lib/sokol_nuklear.h"

typedef struct {
  size_t placeholderValue;
} UI_t;

bool UI_Init();
void UI_Cleanup();
void UI_RunFrame(struct nk_context* ctx);

#endif // UI_H
