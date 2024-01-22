#include "ui.h"
#include "backend.h"

// static UI_t this;

#define GET_NKCOLOR(r, g, b, a) (struct nk_color){ r, g, b, a }

enum {
  THEME_TEXT,
  THEME_HEADER,
  THEME_BORDER,
  THEME_WINDOW,
  THEME_BUTTON,
};

// UI Style //
static struct nk_color theme[NK_COLOR_COUNT] = {
  [NK_COLOR_TEXT]                    = GET_NKCOLOR(210, 210, 210, 255),
  [NK_COLOR_WINDOW]                  = GET_NKCOLOR( 57,  67,  71, 255),
  [NK_COLOR_HEADER]                  = GET_NKCOLOR( 51,  51,  56, 255),
  [NK_COLOR_BORDER]                  = GET_NKCOLOR( 46,  46,  46, 255),
  [NK_COLOR_BUTTON]                  = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_BUTTON_HOVER]            = GET_NKCOLOR( 58,  93, 121, 255),
  [NK_COLOR_BUTTON_ACTIVE]           = GET_NKCOLOR( 63,  98, 126, 255),
  [NK_COLOR_TOGGLE]                  = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_TOGGLE_HOVER]            = GET_NKCOLOR( 45,  53,  56, 255),
  [NK_COLOR_TOGGLE_CURSOR]           = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_SELECT]                  = GET_NKCOLOR( 57,  67,  61, 255),
  [NK_COLOR_SELECT_ACTIVE]           = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_SLIDER]                  = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_SLIDER_CURSOR]           = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_SLIDER_CURSOR_HOVER]     = GET_NKCOLOR( 53,  88, 116, 255),
  [NK_COLOR_SLIDER_CURSOR_ACTIVE]    = GET_NKCOLOR( 58,  93, 121, 255),
  [NK_COLOR_PROPERTY]                = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_EDIT]                    = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_EDIT_CURSOR]             = GET_NKCOLOR(210, 210, 210, 255),
  [NK_COLOR_COMBO]                   = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_CHART]                   = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_CHART_COLOR]             = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_CHART_COLOR_HIGHLIGHT]   = GET_NKCOLOR(255,   0,   0, 255),
  [NK_COLOR_SCROLLBAR]               = GET_NKCOLOR( 50,  58,  61, 255),
  [NK_COLOR_SCROLLBAR_CURSOR]        = GET_NKCOLOR( 48,  83, 111, 255),
  [NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = GET_NKCOLOR( 53,  88, 116, 255),
  [NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = GET_NKCOLOR( 58,  93, 121, 255),
  [NK_COLOR_TAB_HEADER]              = GET_NKCOLOR( 48,  83, 111, 255),
};
static bool themeUpdated = false;

// UI variables //
static struct nk_context* ctx;
static nk_bool showUI          = true;
static nk_bool showAbout       = false;
static nk_bool showEmuSettings = false;
static nk_bool showRegisters   = false;
static nk_bool showMemory      = false;
static nk_bool showMemWatch    = false;
static nk_bool showCPU         = false;

static nk_bool showIntro = true;
static nk_bool showLeds  = false;
static uint8_t currRegion = 0;
static uint8_t filterMode = SCREENFILTER_NEAREST;
static nk_bool keepAR;
static nk_bool showLayer[3] = { true, true, true };
static nk_bool showSpriteOutlines = false;
static nk_bool showSpriteFlip = false;

static char jumpAddressText[7];
static int  jumpAddressTextLen;

// static MemWatchValue_t* memWatches = NULL;


static const uint8_t regionValues[] = {
  REGION_US,         REGION_UK,
  REGION_FRENCH,     REGION_SPANISH,
  REGION_GERMAN,     REGION_ITALIAN,
  REGION_DUTCH,      REGION_POLISH,
  REGION_PORTUGUESE, REGION_CHINESE
};

static const char* regionText[] = {
  "US",         "UK",
  "French",     "Spanish",
  "German",     "Italian",
  "Dutch",      "Polish",
  "Portuguese", "Chinese"
};

static const char* filterText[] = {
  "Nearest", "Linear"
};


// Helper functions //

void fPercentSlider(const char* name, float min, float max, float step, float* value) {
  char text[256];
  float percent = ((*value) / max) * (max * 100.0f);
  snprintf((char*)&text, 256, "%s (%d%%)", name, (int)percent);
  nk_label(ctx, text, NK_TEXT_LEFT);
  nk_slider_float(ctx, min, value, max, step);
}


static char     updateText[5];
static int      updateTextLen;
static uint32_t holdAddr = 0xffffffff;
void IOReg(const char* name, uint16_t addr) {
  if (name)
    nk_label(ctx, name, NK_TEXT_LEFT);

  if (addr == holdAddr) {
    nk_flags flags = nk_edit_string(ctx, NK_EDIT_SIMPLE, updateText, &updateTextLen, 5, nk_filter_hex);
    if (flags & NK_EDIT_ACTIVATED) {
      holdAddr = addr;
      updateTextLen = 0;
      memset(updateText, 0, 5);
      Input_SetControlsEnable(false);
    }
    else if (flags & (NK_EDIT_COMMITED | NK_EDIT_DEACTIVATED)) {
      if (updateTextLen > 0)
        Bus_Store(addr, strtol(updateText, NULL, 16));
      holdAddr = 0xffffffff;
      Input_SetControlsEnable(true);
    }
  } else {
    char text[5];
    uint16_t value = Bus_Load(addr);
    snprintf((char*)&text, 5, "%04x", value);

    if (nk_button_label(ctx, text)) {
      if (holdAddr != 0xffffffff && updateTextLen > 0)
        Bus_Store(holdAddr, strtol(updateText, NULL, 16));
      holdAddr = addr;
      updateTextLen = 0;
      memset(updateText, 0, 5);
      Backend_SetControlsEnable(false);
    }

  }
}


void controllerMapping(const char* buttonName, char* mappingText, int* mappingLen) {
  nk_label(ctx, buttonName, NK_TEXT_LEFT);
  // nk_flags flags = nk_edit_string(ctx, NK_EDIT_SIMPLE, mappingText, mappingLen, 16, nk_filter_hex);
  // if (flags & (NK_EDIT_COMMITED | NK_EDIT_DEACTIVATED)) {
  //   Backend_UpdateButtonMapping(buttonName, mappingText, (*mappingLen));
  // }
}

static nk_bool openColorPicker = false;
static struct nk_colorf pickedColor;
static uint8_t colorToPick;
void chooseColor(struct nk_context* ctx, const char* name, uint8_t type) {
  struct nk_color color;
  switch (type) {
  case THEME_TEXT:   color = theme[NK_COLOR_TEXT];   break;
  case THEME_HEADER: color = theme[NK_COLOR_HEADER]; break;
  case THEME_BORDER: color = theme[NK_COLOR_BORDER]; break;
  case THEME_WINDOW: color = theme[NK_COLOR_WINDOW]; break;
  case THEME_BUTTON: color = theme[NK_COLOR_BUTTON]; break;
  }

  nk_layout_row_dynamic(ctx, 25, 3);
  nk_label(ctx, name, NK_TEXT_LEFT);
  nk_button_color(ctx, color);
  if (nk_button_label(ctx, "Choose...")) {
    openColorPicker = true;
    colorToPick = type;
    pickedColor.r = color.r/255.0f;
    pickedColor.g = color.g/255.0f;
    pickedColor.b = color.b/255.0f;
    pickedColor.a = 1.0f;
  }
}


void updateColor(uint8_t type, struct nk_color chosen) {
  // Adjust click and highlight color
  struct nk_color hover  = nk_rgba_u32(((nk_color_u32(chosen) & 0xefefefef) << 1) | 0xff010101);
  struct nk_color active = nk_rgba_u32(((nk_color_u32(chosen) & 0x3f3f3f3f) << 2) | 0xff030303);
  struct nk_color blank  = nk_rgba_u32(((nk_color_u32(chosen) & 0xfefefefe) >> 1) | 0xff000000);

  switch (type) {
  case THEME_TEXT:   theme[NK_COLOR_TEXT]   = chosen; break;
  case THEME_HEADER: theme[NK_COLOR_HEADER] = chosen; break;
  case THEME_BORDER: theme[NK_COLOR_BORDER] = chosen; break;

  case THEME_BUTTON: {
    theme[NK_COLOR_BUTTON]        = chosen;
    theme[NK_COLOR_BUTTON_HOVER]  = hover;
    theme[NK_COLOR_BUTTON_ACTIVE] = active;

    theme[NK_COLOR_TOGGLE]        = chosen;
    theme[NK_COLOR_TOGGLE_HOVER]  = hover;
    theme[NK_COLOR_TOGGLE_CURSOR] = active;

    theme[NK_COLOR_SLIDER]               = blank;
    theme[NK_COLOR_SLIDER_CURSOR]        = chosen;
    theme[NK_COLOR_SLIDER_CURSOR_HOVER]  = hover;
    theme[NK_COLOR_SLIDER_CURSOR_ACTIVE] = active;

    theme[NK_COLOR_SCROLLBAR]               = blank;
    theme[NK_COLOR_SCROLLBAR_CURSOR]        = hover;
    theme[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = active;
    theme[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = hover;
  } break;

  case THEME_WINDOW: {
    theme[NK_COLOR_WINDOW]      = chosen;
    theme[NK_COLOR_COMBO]       = blank;
    theme[NK_COLOR_EDIT]        = blank;
    theme[NK_COLOR_EDIT_CURSOR] = chosen;
  } break;

  }
}


// UI functions //

bool UI_Init() {
  // memset(&this, 0, sizeof(UI_t));

  snk_setup(&(snk_desc_t){
    .dpi_scale = sapp_dpi_scale()
  });

  char hex[8];
  struct nk_color color;

  if (UserSettings_ReadString("textColor", hex, 8)) {
    color = nk_rgb_hex(hex);
    updateColor(THEME_TEXT, color);
  }

  if (UserSettings_ReadString("headerColor", hex, 8)) {
    color = nk_rgb_hex(hex);
    updateColor(THEME_HEADER, color);
  }

  if (UserSettings_ReadString("borderColor", hex, 8)) {
    color = nk_rgb_hex(hex);
    updateColor(THEME_BORDER, color);
  }

  if (UserSettings_ReadString("windowColor", hex, 8)) {
    color = nk_rgb_hex(hex);
    updateColor(THEME_WINDOW, color);
  }

  if (UserSettings_ReadString("buttonColor", hex, 8)) {
    color = nk_rgb_hex(hex);
    updateColor(THEME_BUTTON, color);
  }


  return true;
}


void UI_Cleanup() {
  snk_shutdown();
}


void UI_HandleEvent(sapp_event* event) {
  snk_handle_event(event);
}


void UI_StartFrame() {
  ctx = snk_new_frame();
}


void UI_RunFrame() {
  if (!showUI)
    return;

  if (!themeUpdated) {
    nk_style_from_table(ctx, theme);
    themeUpdated = true;
  }

  const float width  = sapp_widthf();
  const float height = sapp_heightf();

  char* romPath = NULL;

  // Toolbar //
  if (nk_begin(ctx, "Toolbar", nk_rect(0, 0, width, 32), NK_WINDOW_NO_SCROLLBAR)) {
    nk_menubar_begin(ctx);
    nk_layout_row_begin(ctx, NK_STATIC, 24, 3);

    // Menu
    nk_layout_row_push(ctx, 48);
    if (nk_menu_begin_label(ctx, " Menu", NK_TEXT_LEFT, nk_vec2(175, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "      Open ROM...",       NK_TEXT_LEFT)) romPath = (char*)Backend_OpenFileDialog("Please Select a ROM...");
      if (nk_menu_item_label(ctx, "[Y]   Toggle Fullscreen", NK_TEXT_LEFT)) sapp_toggle_fullscreen();
      if (nk_menu_item_label(ctx, "[U]   Toggle UI",         NK_TEXT_LEFT)) showUI = false;
      if (nk_menu_item_label(ctx, "      About",             NK_TEXT_LEFT)) showAbout = true;
      if (nk_menu_item_label(ctx, "[ESC] Quit",              NK_TEXT_LEFT)) sapp_request_quit();
      nk_menu_end(ctx);
    }

    // System
    nk_layout_row_push(ctx, 48);
    if (nk_menu_begin_label(ctx, "System", NK_TEXT_LEFT, nk_vec2(120, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "[P] Pause",       NK_TEXT_LEFT)) VSmile_SetPause(!VSmile_GetPaused());
      if (nk_menu_item_label(ctx, "[O] Step",        NK_TEXT_LEFT)) VSmile_Step();
      if (nk_menu_item_label(ctx, "[R] Reset",       NK_TEXT_LEFT)) VSmile_Reset();
      if (nk_menu_item_label(ctx, "[J] Save State",  NK_TEXT_LEFT)) VSmile_SaveState();
      if (nk_menu_item_label(ctx, "[K] Load State",  NK_TEXT_LEFT)) VSmile_LoadState();
      if (nk_menu_item_label(ctx, "    Settings",    NK_TEXT_LEFT)) showEmuSettings = true;
      nk_menu_end(ctx);
    }

    // Debug //
    nk_layout_row_push(ctx, 48);
    if (nk_menu_begin_label(ctx, " Debug", NK_TEXT_LEFT, nk_vec2(120, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "    Registers", NK_TEXT_LEFT)) showRegisters = true;
      if (nk_menu_item_label(ctx, "    Memory",    NK_TEXT_LEFT)) showMemory    = true;
      if (nk_menu_item_label(ctx, "    CPU",       NK_TEXT_LEFT)) showCPU       = true;
      nk_menu_end(ctx);
    }


    nk_menubar_end(ctx);

    nk_end(ctx);

    if (romPath != NULL) {
      VSmile_LoadROM(romPath);
      VSmile_Reset();
      VSmile_SetPause(false);
    }

  }


  // Emulation Settings //
  if (showEmuSettings) {
    if (nk_begin(ctx, "Emulation Settings", nk_rect(width*0.5f - 600*0.5f, height*0.5f - 400*0.5f, 600, 400), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {

      if (nk_tree_push(ctx, NK_TREE_NODE, "System", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 3);

        // Emulation speed
        float emulationSpeed = Backend_GetSpeed();
        fPercentSlider("Emulation Speed", 0.1f, 2.0f, 0.1f, &emulationSpeed);
        if (nk_button_label(ctx, "Set to 100%")) emulationSpeed = 1.0f;
        Backend_SetSpeed(emulationSpeed);

        nk_layout_row_dynamic(ctx, 25, 2);

        // Set Region
        nk_label(ctx, "Region: ", NK_TEXT_LEFT);
        currRegion = nk_combo(ctx, regionText, NK_LEN(regionText), currRegion, 25, nk_vec2(200,200));
        VSmile_SetRegion(regionValues[currRegion]);

        nk_layout_row_dynamic(ctx, 25, 1);

        // Show Boot Screen
        nk_checkbox_label(ctx, "Show Boot Screen", &showIntro);
        VSmile_SetIntroEnable(showIntro);

        // Show LEDs
        nk_checkbox_label(ctx, "Show LEDs", &showLeds);
        Backend_ShowLeds(showLeds);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Graphics", NK_MINIMIZED)) {
        // Screen Filter
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Screen Filter", NK_TEXT_LEFT);
        filterMode = nk_combo(ctx, filterText, NK_LEN(filterText), filterMode, 25, nk_vec2(200,200));
        Backend_SetScreenFilter(filterMode);

        nk_checkbox_label(ctx, "Maintain Aspect Ratio", &keepAR);
        Backend_SetKeepAspectRatio(keepAR);

        // Layer Visibility
        nk_label(ctx, "Layer Visibility", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 3);
        nk_checkbox_label(ctx, "Show Layer 0", &showLayer[0]);
        PPU_SetLayerVisibility(0, showLayer[0]);
        nk_checkbox_label(ctx, "Show Layer 1", &showLayer[1]);
        PPU_SetLayerVisibility(1, showLayer[1]);
        nk_checkbox_label(ctx, "Show Sprites", &showLayer[2]);
        PPU_SetLayerVisibility(2, showLayer[2]);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Sprite Visualization", NK_LEFT);
        nk_layout_row_dynamic(ctx, 25, 2);

        // Sprite Outlines
        nk_checkbox_label(ctx, "Sprite Outlines", &showSpriteOutlines);
        PPU_SetSpriteOutlines(showSpriteOutlines);
        // Sprite Flip
        nk_checkbox_label(ctx, "Sprite Flip", &showSpriteFlip);
        PPU_SetFlipVisual(showSpriteFlip);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Input Bindings", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Coming soon!", NK_LEFT);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "UI Theme", NK_MINIMIZED)) {
        chooseColor(ctx, "Text",    THEME_TEXT);
        chooseColor(ctx, "Header",  THEME_HEADER);
        chooseColor(ctx, "Border",  THEME_BORDER);
        chooseColor(ctx, "Window",  THEME_WINDOW);
        chooseColor(ctx, "Buttons", THEME_BUTTON);
        if (nk_button_label(ctx, "Save colors to profile")) {
          char hexColor[7];

          nk_color_hex_rgb(hexColor, theme[NK_COLOR_TEXT]);
          UserSettings_WriteString("textColor", hexColor, 7);
          nk_color_hex_rgb(hexColor, theme[NK_COLOR_HEADER]);
          UserSettings_WriteString("headerColor", hexColor, 7);
          nk_color_hex_rgb(hexColor, theme[NK_COLOR_BORDER]);
          UserSettings_WriteString("borderColor", hexColor, 7);
          nk_color_hex_rgb(hexColor, theme[NK_COLOR_WINDOW]);
          UserSettings_WriteString("windowColor", hexColor, 7);
          nk_color_hex_rgb(hexColor, theme[NK_COLOR_BUTTON]);
          UserSettings_WriteString("buttonColor", hexColor, 7);
        }

        nk_tree_pop(ctx);
      }


    } else showEmuSettings = false;
    nk_end(ctx);
  }


  // About //
  if (showAbout) {
    if (nk_begin(ctx, "About", nk_rect(width/2 - 480/2, height/2 - 360/2, 480, 360), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "V.Frown - The Experimental V.Smile Emulator", NK_TEXT_LEFT);
        nk_label(ctx, "Created by Schnert0 and contributors", NK_TEXT_LEFT);
        nk_label(ctx, "V.Frown emulator core is licensed under the MIT license.",  NK_TEXT_LEFT);
        nk_label(ctx, "nuklear UI library is licensed under the public domain license.",  NK_TEXT_LEFT);
        nk_label(ctx, "Sokol libraries are licensed under the zlib/libpng license.",  NK_TEXT_LEFT);
        nk_label(ctx, "ini.h is dual licensed under the MIT and public domain licenses.",  NK_TEXT_LEFT);
        nk_label(ctx, "for source code and a full list of contributors, go to:", NK_TEXT_LEFT);
        nk_label(ctx, "https://github.com/Schnert0/VFrown", NK_TEXT_LEFT);
    } else showAbout = false;
    nk_end(ctx);
  }

  // Color Picker //
  if (openColorPicker) {
    const float width  = sapp_widthf();
    const float height = sapp_heightf();
    if (nk_begin(
      ctx, "Choose a color...",
      nk_rect(width*0.5f - 600*0.5f, height*0.5f - 400*0.5f, 600, 400),
      NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
    )) {

      static char hexText[8];
      static int hexTextLen = 0;
      static bool updatingColor = false;

      nk_layout_row_static(ctx, 256, 256, 1);
      nk_color_pick(ctx, &pickedColor, NK_RGB);
      struct nk_color preview = {
        .r = pickedColor.r*255.0f,
        .g = pickedColor.g*255.0f,
        .b = pickedColor.b*255.0f,
        .a = pickedColor.a*255.0f,
      };

      nk_layout_row_dynamic(ctx, 25, 3);
      if (nk_button_label(ctx, "Confirm")) {
        if (updatingColor) {
          updatingColor = false;
          preview = nk_rgb_hex(hexText);
          pickedColor.r = preview.r/255.0f;
          pickedColor.g = preview.g/255.0f;
          pickedColor.b = preview.b/255.0f;
          pickedColor.a = preview.a/255.0f;
        }

        updateColor(colorToPick, preview);

        themeUpdated = false;
        openColorPicker = false;
      }

      nk_flags flags = nk_edit_string(ctx, NK_EDIT_SIMPLE, hexText, &hexTextLen, 8, nk_filter_hex);
      if (flags & NK_EDIT_ACTIVATED) {
        updatingColor = true;
      }
      else if (flags & (NK_EDIT_DEACTIVATED | NK_EDIT_COMMITED)) {
        updatingColor = false;
        preview = nk_rgb_hex(hexText);
        pickedColor.r = preview.r/255.0f;
        pickedColor.g = preview.g/255.0f;
        pickedColor.b = preview.b/255.0f;
        pickedColor.a = preview.a/255.0f;
      }

      if (!updatingColor) {
        nk_color_hex_rgb(hexText, preview);
        hexTextLen = strlen(hexText);
      }
      nk_button_color(ctx, preview);

    } else openColorPicker = false;
    nk_end(ctx);
  }

  // Registers //
  if (showRegisters) {
    if (nk_begin(ctx, "Registers", nk_rect(width/2 - 480/2, height/2 - 270/2, 480, 270), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
      if (nk_tree_push(ctx, NK_TREE_NODE, "GPIO", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Config", 0x3d00);

        // Port A
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Port A", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Data", 0x3d01);
        IOReg("Buffer", 0x3d02);
        IOReg("Direction", 0x3d03);
        IOReg("Attributes", 0x3d04);
        IOReg("Mask", 0x3d05);


        // Port B
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Port B", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Data", 0x3d06);
        IOReg("Buffer", 0x3d07);
        IOReg("Direction", 0x3d08);
        IOReg("Attributes", 0x3d09);
        IOReg("Mask", 0x3d0a);

        // Port C
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Port C", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Data", 0x3d0b);
        IOReg("Buffer", 0x3d0c);
        IOReg("Direction", 0x3d0d);
        IOReg("Attributes", 0x3d0e);
        IOReg("Mask", 0x3d0f);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Timers", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("TmBase set", 0x3d10);
        // IOReg("Timebase Clear", 0x3d11);
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Timer A", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Data", 0x3d12);
        IOReg("Ctrl", 0x3d13);
        IOReg("Enable", 0x3d14);
        // IOReg("IRQ Clear", 0x3d15);
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Timer B", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Data", 0x3d16);
        IOReg("Ctrl", 0x3d17);
        IOReg("Enable", 0x3d18);
        // IOReg("IRQ Clear", 0x3d19);
        IOReg("Scanline", 0x3d1c);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "Misc.", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 6);
        IOReg("Sys Ctrl", 0x3d20);
        IOReg("Int Ctrl", 0x3d21);
        IOReg("Int Stat", 0x3d22);
        IOReg("ExMem Ctrl", 0x3d23);
        // IOReg("Wtchdg Clr", 0x3d24);
        IOReg("ADC Ctrl", 0x3d25);
        IOReg("ADC Data", 0x3d27);
        // IOReg("Sleep Mode", 0x3d28);
        IOReg("Wkup Src", 0x3d29);
        IOReg("Wkup Time", 0x3d2a);
        IOReg("TV System", 0x3d2b);
        IOReg("PRNG 1", 0x3d2c);
        IOReg("PRNG 2", 0x3d2d);
        IOReg("FIQ Sel", 0x3d2e);
        IOReg("DS Reg", 0x3d2f);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "UART", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 6);

        IOReg("Ctrl", 0x3d30);
        IOReg("Stat", 0x3d31);
        IOReg("Baud Lo", 0x3d33);
        IOReg("Baud Hi", 0x3d34);
        IOReg("Tx", 0x3d35);
        IOReg("Rx", 0x3d36);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "SPI", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 1);

        nk_label(ctx, "SPI is currently not implemented", NK_TEXT_LEFT);

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_NODE, "DMA", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 25, 6);

        IOReg("Src Lo", 0x3e00);
        IOReg("Src Hi", 0x3e01);
        IOReg("Length", 0x3e02);
        IOReg("Dst", 0x3e03);

        nk_tree_pop(ctx);
      }

    } else {
      showRegisters = false;
      Backend_SetControlsEnable(true);
    }
    nk_end(ctx);
  }

  if (showMemory) {
    if (nk_begin(ctx, "Memory", nk_rect(width/2 - 480/2, height/2 - 270/2, 480, 270), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {

      // Jump to Address
      nk_menubar_begin(ctx);
      nk_layout_row_begin(ctx, NK_DYNAMIC, 27, 3);
      nk_layout_row_push(ctx, 0.25f);
      bool jumpToAddress = false;
      if (nk_button_label(ctx, "Jump")) {
        jumpToAddress = true;
        Backend_SetControlsEnable(true);
      }
      nk_layout_row_push(ctx, 0.75f);
      nk_flags flags = nk_edit_string(ctx, NK_EDIT_SIMPLE, jumpAddressText, &jumpAddressTextLen, 7, nk_filter_hex);
      if (flags & NK_EDIT_ACTIVATED) {
        Backend_SetControlsEnable(false);
      }
      nk_layout_row_end(ctx);
      nk_menubar_end(ctx);

      // Generate Memory text boxes
      nk_layout_row_dynamic(ctx, 25, 9);
      if (jumpToAddress) {
        uint32_t jumpAddr = strtol(jumpAddressText, NULL, 16);
        if (jumpAddr >= 0x2800)
          jumpAddr = 0x2800;
        jumpAddr >>= 3;
        jumpAddr *= 29;
        nk_window_set_scroll(ctx, 0, jumpAddr);
        jumpAddressTextLen = 0;
        memset(&jumpAddressText, 0, 7);
      }
      char text[5];
      for (int32_t i = 0; i < 0x2800; i++) {
        if ((i & 7) == 0) {
          snprintf((char*)&text, 5, "%04x", i);
          nk_label(ctx, (const char*)&text, NK_TEXT_LEFT);
        }
        IOReg(NULL, i);
      }
    } else {
      showMemory = false;
      Backend_SetControlsEnable(true);
    }
    nk_end(ctx);
  }


  if (showMemWatch) {
    if (nk_begin(ctx, "Memory Watch", nk_rect(width/2 - 480/2, height/2 - 270/2, 480, 270), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
      nk_label(ctx, "Addresses", NK_LEFT);
    } else {
      showMemWatch = false;
      Backend_SetControlsEnable(true);
    }
  }


  if (showCPU) {
    if (nk_begin(ctx, "CPU", nk_rect(width/2.0f - 480.0f/2.0f, height/2.0f - 270.0f/2.0f, 480, 270), NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label(ctx, "Coming soon!", NK_LEFT);
    } else {
      showCPU = false;
      Backend_SetControlsEnable(true);
    }
    nk_end(ctx);
  }

}


void UI_Render() {
  snk_render(sapp_width(), sapp_height());
}


void UI_Toggle() {
  showUI ^= true;
}
