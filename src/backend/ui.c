#include "ui.h"

static UI_t this;

static const char keymap[512] = {
  [SAPP_KEYCODE_LEFT_SHIFT]       = MU_KEY_SHIFT,
  [SAPP_KEYCODE_RIGHT_SHIFT]      = MU_KEY_SHIFT,
  [SAPP_KEYCODE_LEFT_CONTROL]     = MU_KEY_CTRL,
  [SAPP_KEYCODE_RIGHT_CONTROL]    = MU_KEY_CTRL,
  [SAPP_KEYCODE_LEFT_ALT]         = MU_KEY_ALT,
  [SAPP_KEYCODE_RIGHT_ALT]        = MU_KEY_ALT,
  [SAPP_KEYCODE_ENTER]            = MU_KEY_RETURN,
  [SAPP_KEYCODE_BACKSPACE]        = MU_KEY_BACKSPACE,
};

// static mu_Style vfrownDefault = {
//   /* font | size | padding | spacing | indent */
//   NULL, { 68, 10 }, 5, 4, 24,
//   /* title_height | scrollbar_size | thumb_size */
//   24, 12, 8,
//   {
//     { 230, 230, 230, 255 }, /* MU_COLOR_TEXT */
//     { 25,  25,  25,  255 }, /* MU_COLOR_BORDER */
//     { 50,  50,  50,  255 }, /* MU_COLOR_WINDOWBG */
//     { 25,  25,  25,  255 }, /* MU_COLOR_TITLEBG */
//     { 240, 240, 240, 255 }, /* MU_COLOR_TITLETEXT */
//     { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
//     { 75,  75,  75,  255 }, /* MU_COLOR_BUTTON */
//     { 95,  95,  95,  255 }, /* MU_COLOR_BUTTONHOVER */
//     { 115, 115, 115, 255 }, /* MU_COLOR_BUTTONFOCUS */
//     { 30,  30,  30,  255 }, /* MU_COLOR_BASE */
//     { 35,  35,  35,  255 }, /* MU_COLOR_BASEHOVER */
//     { 40,  40,  40,  255 }, /* MU_COLOR_BASEFOCUS */
//     { 43,  43,  43,  255 }, /* MU_COLOR_SCROLLBASE */
//     { 30,  30,  30,  255 }  /* MU_COLOR_SCROLLTHUMB */
//   }
// };


static void UI_Update();
static void UI_Render();

static int UI_GetTextWidth(mu_Font font, const char* text, int len);
static int UI_GetTextHeight(mu_Font font);

static void UI_RenderText(const char* text, mu_Vec2 pos, mu_Color color);
static void UI_RenderRect(mu_Rect dst, mu_Color color);
static void UI_RenderIcon(int32_t id, mu_Rect rect, mu_Color color);
static void UI_SetClipRect(mu_Rect rect);


bool UI_Init() {
  memset(&this, 0, sizeof(UI_t));

  mu_init(&this.ctx);
  this.ctx.text_width  = UI_GetTextWidth;
  this.ctx.text_height = UI_GetTextHeight;

  // Create gpu-side texture atlas for UI
  const uint32_t rgba8Size = ATLAS_WIDTH * ATLAS_HEIGHT * sizeof(uint32_t);
  uint32_t* rgba8Pixels = malloc(rgba8Size);
  for (int i = 0; i < ATLAS_WIDTH*ATLAS_HEIGHT; i++) {
      rgba8Pixels[i] = 0x00ffffff | (atlas_texture[i] << 24);
  }
  this.atlasImage = sg_make_image(&(sg_image_desc) {
    .width        = ATLAS_WIDTH,
    .height       = ATLAS_HEIGHT,
    .min_filter   = SG_FILTER_NEAREST,
    .mag_filter   = SG_FILTER_NEAREST,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .data = {
      .subimage[0][0] = {
        .ptr  = rgba8Pixels,
        .size = rgba8Size
      }
    }
  });
  free(rgba8Pixels);

  return true;
}


void UI_Cleanup() {

}


void UI_RunFrame() {
  UI_Update();
  UI_Render();
}


void UI_HandleEvent(sapp_event* event) {
  switch (event->type) {
  case SAPP_EVENTTYPE_MOUSE_DOWN:
    mu_input_mousedown(&this.ctx, (int)event->mouse_x, (int)event->mouse_y, (1 << event->mouse_button));
    break;
  case SAPP_EVENTTYPE_MOUSE_UP:
    mu_input_mouseup(&this.ctx, (int)event->mouse_x, event->mouse_y, (1 << event->mouse_button));
    break;
  case SAPP_EVENTTYPE_MOUSE_MOVE:
    mu_input_mousemove(&this.ctx, event->mouse_x, event->mouse_y);
    break;
  case SAPP_EVENTTYPE_MOUSE_SCROLL:
    // mu_input_mousewheel(&this.ctx, event->scroll_y);
    break;
  case SAPP_EVENTTYPE_KEY_DOWN:
    mu_input_keydown(&this.ctx, keymap[event->key_code & 511]);
    break;
  case SAPP_EVENTTYPE_KEY_UP:
    mu_input_keyup(&this.ctx, keymap[event->key_code & 511]);
    break;
  case SAPP_EVENTTYPE_CHAR: {
    char txt[2] = { (char)(event->char_code & 255), 0 };
    mu_input_text(&this.ctx, txt);
  } break;
  default: break;
  }
}


static const mu_Rect winRect = { 16, 16, 320, 450 };

static void UI_Update() {
  mu_begin(&this.ctx);
  if (mu_begin_window(&this.ctx, "Hello, World!", winRect)) {
    mu_label(&this.ctx,"This is a Test.");
    mu_end_window(&this.ctx);
  }
  mu_end(&this.ctx);
}


static void UI_Render() {
  sgl_end();
  sgl_texture(this.atlasImage);
  sgl_begin_quads();

  mu_Command* cmd = 0;
  while(mu_next_command(&this.ctx, &cmd)) {
    switch (cmd->type) {
    case MU_COMMAND_TEXT: UI_RenderText(cmd->text.str, cmd->text.pos, cmd->text.color); break;
    case MU_COMMAND_RECT: UI_RenderRect(cmd->rect.rect, cmd->rect.color); break;
    case MU_COMMAND_ICON: UI_RenderIcon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
    case MU_COMMAND_CLIP: UI_SetClipRect(cmd->clip.rect); break;
    }
  }
}


static int UI_GetTextWidth(mu_Font font, const char* text, int len) {
  if (len == -1) {
    len = strlen(text);
  }

  int width = 0;
  uint32_t i = 0;
  while (text[i] && i < len) {
    width += atlas[ATLAS_FONT + i].w;
    i++;
  }
  return width;
}

static int UI_GetTextHeight(mu_Font font) {
  return 18;
}


static void UI_RenderText(const char* text, mu_Vec2 pos, mu_Color color) {
  sgl_c4b(color.r, color.g, color.b, color.a);
  uint32_t i = 0;
  mu_Rect dst = { .x = pos.x, .y = pos.y };
  while (text[i]) {
    mu_Rect src = atlas[ATLAS_FONT + text[i]];
    dst.w = src.w;
    dst.h = src.h;
    sgl_v2f_t2f(dst.x,       dst.y,       (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
    sgl_v2f_t2f(dst.x+dst.w, dst.y,       (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
    sgl_v2f_t2f(dst.x+dst.w, dst.y+dst.h, (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
    sgl_v2f_t2f(dst.x,       dst.y+dst.h, (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
    dst.x += dst.w;
    i++;
  }
}


static void UI_RenderRect(mu_Rect dst, mu_Color color) {
  sgl_c4b(color.r, color.g, color.b, color.a);
  mu_Rect src = atlas[ATLAS_WHITE];
  sgl_v2f_t2f(dst.x,       dst.y,       (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
  sgl_v2f_t2f(dst.x+dst.w, dst.y,       (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
  sgl_c4b(color.r>>1, color.g>>1, color.b>>1, color.a);
  sgl_v2f_t2f(dst.x+dst.w, dst.y+dst.h, (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
  sgl_v2f_t2f(dst.x,       dst.y+dst.h, (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
}


static void UI_RenderIcon(int32_t id, mu_Rect rect, mu_Color color) {
  mu_Rect src = atlas[id];
  int x = rect.x + (rect.w - src.w) / 2;
  int y = rect.y + (rect.h - src.h) / 2;
  mu_Rect dst = { x, y, src.w, src.h };
  sgl_c4b(color.r, color.g, color.b, color.a);
  sgl_v2f_t2f(dst.x,       dst.y,       (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
  sgl_v2f_t2f(dst.x+dst.w, dst.y,       (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y      )/(float)ATLAS_HEIGHT);
  sgl_c4b(color.r>>1, color.g>>1, color.b>>1, color.a);
  sgl_v2f_t2f(dst.x+dst.w, dst.y+dst.h, (float)(src.x+src.w)/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
  sgl_v2f_t2f(dst.x,       dst.y+dst.h, (float)(src.x      )/(float)ATLAS_WIDTH, (float)(src.y+src.h)/(float)ATLAS_HEIGHT);
}


static void UI_SetClipRect(mu_Rect rect) {
  sgl_end();
  sgl_scissor_rect(rect.x, rect.y, rect.w, rect.h, true);
  sgl_begin_quads();
}


// // System settings
// static int32_t emulationSpeed = 100;
// static int32_t clockSpeed = 100;
// static nk_bool emulationPaused = false;
// static nk_bool introShown = true;

// // Popup window settings
// static nk_bool showHelp = false;
// static nk_bool showAbout = false;
// static nk_bool showChannels = false;
// static nk_bool showLeds = false;
// static nk_bool chanEnable[16] = {
//   true, true, true, true,
//   true, true, true, true,
//   true, true, true, true,
//   true, true, true, true
// };
// static nk_bool oscilloscopeEnabled = false;

// // UI START //
// if (nk_begin(ctx, "V.Frown", nk_rect(0, 0, sapp_width(), 30), 0)) {

//   // MENU BAR //
//   nk_layout_row_static(ctx, 20, 64, 8);

//   // File
//   if (nk_menu_begin_label(ctx, "File", NK_TEXT_CENTERED, nk_vec2(60, 3*ITEM_HEIGHT))) {
//     nk_layout_row_dynamic(ctx, 25, 1);

//     // Drag and drop for now
//     // if (nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT)) {
//     //
//     // }
//     if (nk_menu_item_label(ctx, "Help", NK_TEXT_LEFT)) {
//       showHelp = true;
//     }
//     if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
//       showAbout = true;
//     }
//     if (nk_menu_item_label(ctx, "Quit", NK_TEXT_LEFT)) {
//       sapp_request_quit();
//     }

//     nk_menu_end(ctx);
//   }

//   // Emulation
//   if (nk_menu_begin_label(ctx, "System", NK_TEXT_CENTERED, nk_vec2(200, 9*ITEM_HEIGHT))) {
//     nk_layout_row_dynamic(ctx, 25, 1);

//     if (nk_menu_item_label(ctx, "Toggle FullScreen", NK_TEXT_LEFT)) {
//       sapp_toggle_fullscreen();
//     }

//     emulationPaused = VSmile_GetPaused();
//     nk_checkbox_label(ctx, "Paused",  &emulationPaused);
//     VSmile_SetPause(emulationPaused);

//     introShown = VSmile_GetIntroEnable();
//     nk_checkbox_label(ctx, "Intro Enabled", &introShown);
//     VSmile_SetIntroEnable(introShown);

//     if (nk_menu_item_label(ctx, "Step", NK_TEXT_LEFT)) {
//       VSmile_Step();
//     }
//     if (nk_menu_item_label(ctx, "Reset", NK_TEXT_LEFT)) {
//       VSmile_Reset();
//     }
//     if (nk_menu_item_label(ctx, "Save State", NK_TEXT_LEFT)) {
//       Backend_SaveState();
//     }
//     if (nk_menu_item_label(ctx, "Load State", NK_TEXT_LEFT)) {
//       Backend_LoadState();
//     }
//     emulationSpeed = (int32_t)(Backend_GetSpeed() * 100.0f);
//     char speedText[256];
//     snprintf((char*)&speedText, 256, "Emulation Speed (%d%%)", emulationSpeed);
//     nk_label(ctx, speedText, NK_TEXT_LEFT);
//     nk_slider_int(ctx, 10, &emulationSpeed, 200, 1);
//     if (nk_menu_item_label(ctx, "Set speed to 100%", NK_TEXT_LEFT)) {
//       emulationSpeed = 100;
//     }
//     Backend_SetSpeed(emulationSpeed / 100.0f);

//     nk_checkbox_label(ctx, "Show LEDs", &showLeds);
//     Backend_ShowLeds(showLeds);

//     nk_menu_end(ctx);
//   }

//   // Video
//   if (nk_menu_begin_label(ctx, "Video", NK_TEXT_CENTERED, nk_vec2(200, 7*ITEM_HEIGHT))) {
//     nk_layout_row_dynamic(ctx, 25, 1);
//     if (nk_menu_item_label(ctx, "Toggle Layer 0", NK_TEXT_LEFT)) {
//       PPU_ToggleLayer(0);
//     }
//     if (nk_menu_item_label(ctx, "Toggle Layer 1", NK_TEXT_LEFT)) {
//       PPU_ToggleLayer(1);
//     }
//     if (nk_menu_item_label(ctx, "Toggle Sprites", NK_TEXT_LEFT)) {
//       PPU_ToggleLayer(2);
//     }
//     if (nk_menu_item_label(ctx, "Toggle Sprite Outlines", NK_TEXT_LEFT)) {
//       PPU_ToggleSpriteOutlines();
//     }
//     if (nk_menu_item_label(ctx, "Toggle Flip Visual", NK_TEXT_LEFT)) {
//       PPU_ToggleFlipVisual();
//     }

//     nk_menu_end(ctx);
//   }

//   // Audio
//   if (nk_menu_begin_label(ctx, "Audio", NK_TEXT_CENTERED, nk_vec2(200, 2*ITEM_HEIGHT))) {
//     nk_layout_row_dynamic(ctx, 25, 1);
//     if (nk_menu_item_label(ctx, "Toggle Channels", NK_TEXT_LEFT)) {
//       showChannels = true;
//     }
//     nk_checkbox_label(ctx, "Oscilloscope View", &oscilloscopeEnabled);
//     Backend_SetOscilloscopeEnabled(oscilloscopeEnabled);

//     nk_menu_end(ctx);
//   }

//   // Advanced
//   if (nk_menu_begin_label(ctx, "Advanced", NK_TEXT_CENTERED, nk_vec2(200, 3*ITEM_HEIGHT))) {
//     nk_layout_row_dynamic(ctx, 25, 1);

//     float clockScale = VSmile_GetClockScale();
//     char speedText[256];
//     snprintf((char*)&speedText, 256, "Clock Speed (%.2f MHz)", (27000000.0f * clockScale) / 1000000.0f);
//     nk_label(ctx, speedText, NK_TEXT_LEFT);
//     clockSpeed = (int32_t)(clockScale * 100.0f);
//     nk_slider_int(ctx, 10, &clockSpeed, 200, 1);
//     if (nk_menu_item_label(ctx, "Set clock speed to 100%", NK_TEXT_LEFT)) {
//       clockSpeed = 100;
//     }
//     VSmile_SetClockScale(clockSpeed / 100.0f);

//     nk_menu_end(ctx);
//   }

//   // POPUP WINDOWS //

//   // About window
//   if (showAbout) {
//     static struct nk_rect aboutBox = { 20, 40, 600, 400 };
//     if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_CLOSABLE, aboutBox)) {
//         nk_layout_row_dynamic(ctx, 25, 1);
//         nk_label(ctx, "V.Frown - The Experimental V.Smile Emulator", NK_TEXT_LEFT);
//         nk_label(ctx, "By Ian S. (Schnert0) and co.", NK_TEXT_LEFT);
//         nk_label(ctx, "V.Frown emulator core is licensed under the MIT license.",  NK_TEXT_LEFT);
//         nk_label(ctx, "nuklear UI library is licensed under the public domain license.",  NK_TEXT_LEFT);
//         nk_label(ctx, "Sokol libraries are licensed under the zlib/libpng license.",  NK_TEXT_LEFT);
//         nk_label(ctx, "Sokol_GP is licensed under the MIT-0 license.",  NK_TEXT_LEFT);
//         nk_label(ctx, "ini.h is dual licensed under the MIT and public domain licenses.",  NK_TEXT_LEFT);
//         nk_label(ctx, "for source code and a full list of contributors, go to:", NK_TEXT_LEFT);
//         nk_label(ctx, "https://github.com/Schnert0/VFrown", NK_TEXT_LEFT);
//         nk_popup_end(ctx);
//     } else showAbout = false;
//   }

//   // Help window
//   if (showHelp) {
//     static struct nk_rect helpBox = { 20, 40, 600, 400 };
//     if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Help", NK_WINDOW_CLOSABLE, helpBox)) {
//         nk_layout_row_dynamic(ctx, 20, 1);
//         nk_label(ctx, "Drag and drop a ROM on the window to start playing!", NK_TEXT_LEFT);
//         nk_label(ctx, "~ - Toggle FullScreen", NK_TEXT_LEFT);
//         nk_label(ctx, "1 - Toggle Layer 0", NK_TEXT_LEFT);
//         nk_label(ctx, "2 - Toggle Layer 1", NK_TEXT_LEFT);
//         nk_label(ctx, "3 - Toggle Sprites", NK_TEXT_LEFT);
//         nk_label(ctx, "P - Pause", NK_TEXT_LEFT);
//         nk_label(ctx, "O - Step", NK_TEXT_LEFT);
//         // nk_label(ctx, "6 - Sprite Outlines", NK_TEXT_LEFT);
//         // nk_label(ctx, "7 - Sprite Flip Visualization", NK_TEXT_LEFT);
//         // nk_label(ctx, "8 - Sound Oscilloscope Visualization", NK_TEXT_LEFT);
//         nk_label(ctx, "0 - Reset", NK_TEXT_LEFT);
//         // nk_label(ctx, "F1 - Toggle LED Visualization", NK_TEXT_LEFT);
//         // nk_label(ctx, "F10 - Close Emulator", NK_TEXT_LEFT);
//         nk_label(ctx, "z - RED", NK_TEXT_LEFT);
//         nk_label(ctx, "x - YELLOW", NK_TEXT_LEFT);
//         nk_label(ctx, "c - GREEN", NK_TEXT_LEFT);
//         nk_label(ctx, "v - BLUE", NK_TEXT_LEFT);
//         nk_label(ctx, "a - HELP", NK_TEXT_LEFT);
//         nk_label(ctx, "s - EXIT", NK_TEXT_LEFT);
//         nk_label(ctx, "d - ABC", NK_TEXT_LEFT);
//         nk_label(ctx, "[space] - ENTER", NK_TEXT_LEFT);
//         nk_popup_end(ctx);
//     } else showHelp = false;
//   }

//   // Audio Channels window
//   if (showChannels) {
//     static struct nk_rect channelsBox = { 20, 40, 150, 400 };
//     if (nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Audio Channels", NK_WINDOW_CLOSABLE, channelsBox)) {
//         nk_layout_row_dynamic(ctx, 20, 1);
//         nk_checkbox_label(ctx, "Channel 0",  &chanEnable[0]);
//         nk_checkbox_label(ctx, "Channel 1",  &chanEnable[1]);
//         nk_checkbox_label(ctx, "Channel 2",  &chanEnable[2]);
//         nk_checkbox_label(ctx, "Channel 3",  &chanEnable[3]);
//         nk_checkbox_label(ctx, "Channel 4",  &chanEnable[4]);
//         nk_checkbox_label(ctx, "Channel 5",  &chanEnable[5]);
//         nk_checkbox_label(ctx, "Channel 6",  &chanEnable[6]);
//         nk_checkbox_label(ctx, "Channel 7",  &chanEnable[7]);
//         nk_checkbox_label(ctx, "Channel 8",  &chanEnable[8]);
//         nk_checkbox_label(ctx, "Channel 9",  &chanEnable[9]);
//         nk_checkbox_label(ctx, "Channel 10", &chanEnable[10]);
//         nk_checkbox_label(ctx, "Channel 11", &chanEnable[11]);
//         nk_checkbox_label(ctx, "Channel 12", &chanEnable[12]);
//         nk_checkbox_label(ctx, "Channel 13", &chanEnable[13]);
//         nk_checkbox_label(ctx, "Channel 14", &chanEnable[14]);
//         nk_checkbox_label(ctx, "Channel 15", &chanEnable[15]);
//         SPU_SetEnabledChannels(
//           (chanEnable[0]       ) | (chanEnable[1]  <<  1) | (chanEnable[2]  <<  2) | (chanEnable[3]  <<  3) |
//           (chanEnable[4]  <<  4) | (chanEnable[5]  <<  5) | (chanEnable[6]  <<  6) | (chanEnable[7]  <<  7) |
//           (chanEnable[8]  <<  8) | (chanEnable[9]  <<  9) | (chanEnable[10] << 10) | (chanEnable[11] << 11) |
//           (chanEnable[12] << 12) | (chanEnable[13] << 13) | (chanEnable[14] << 14) | (chanEnable[15] << 15)
//         );
//         nk_popup_end(ctx);
//     } else showChannels = false;
//   }

// }

// nk_end(ctx);
