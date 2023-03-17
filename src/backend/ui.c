#include "ui.h"

static UI_t this;

bool UI_Init() {
  memset(&this, 0, sizeof(UI_t));

  return true;
}


void UI_Cleanup() {

}


void UI_RunFrame(struct nk_context* ctx) {
  // System settings
  static int32_t emulationSpeed = 100;
  static int32_t clockSpeed = 100;
  static nk_bool emulationPaused = false;
  static nk_bool introShown = true;

  // Popup window settings
  static bool showHelp = false;
  static bool showAbout = false;
  static bool showChannels = false;
  static nk_bool chanEnable[16] = {
    true, true, true, true,
    true, true, true, true,
    true, true, true, true,
    true, true, true, true
  };

  // UI START //
  if (nk_begin(ctx, "V.Frown", nk_rect(0, 0, sapp_width(), 30), 0)) {

    // MENU BAR //
    nk_layout_row_static(ctx, 18, 48, 8);

    // File
    if (nk_menu_begin_label(ctx, "File", NK_TEXT_CENTERED, nk_vec2(60, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);

      // Drag and drop for now
      // if (nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT)) {
      //
      // }
      if (nk_menu_item_label(ctx, "Help", NK_TEXT_LEFT)) {
        showHelp = true;
      }
      if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
        showAbout = true;
      }
      if (nk_menu_item_label(ctx, "Quit", NK_TEXT_LEFT)) {
        sapp_request_quit();
      }

      nk_menu_end(ctx);
    }

    // Emulation
    if (nk_menu_begin_label(ctx, "System", NK_TEXT_CENTERED, nk_vec2(175, 240))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "Toggle FullScreen", NK_TEXT_LEFT)) {
        sapp_toggle_fullscreen();
      }

      emulationPaused = VSmile_GetPaused();
      nk_checkbox_label(ctx, "Paused",  &emulationPaused);
      VSmile_SetPause(emulationPaused);

      introShown = VSmile_GetIntroEnable();
      nk_checkbox_label(ctx, "Intro Enabled", &introShown);
      VSmile_SetIntroEnable(introShown);

      if (nk_menu_item_label(ctx, "Step", NK_TEXT_LEFT)) {
        VSmile_Step();
      }
      if (nk_menu_item_label(ctx, "Reset", NK_TEXT_LEFT)) {
        VSmile_Reset();
      }
      emulationSpeed = (int32_t)(Backend_GetSpeed() * 100.0f);
      char speedText[256];
      snprintf((char*)&speedText, 256, "Emulation Speed (%d)", emulationSpeed);
      nk_label(ctx, speedText, NK_TEXT_LEFT);
      nk_slider_int(ctx, 10, &emulationSpeed, 200, 1);
      if (nk_menu_item_label(ctx, "Set speed to 100%", NK_TEXT_LEFT)) {
        emulationSpeed = 100;
      }
      Backend_SetSpeed(emulationSpeed / 100.0f);

      float clockScale = VSmile_GetClockScale();
      snprintf((char*)&speedText, 256, "Clock Speed (%.2f MHz)", (27000000.0f * clockScale) / 1000000.0f);
      nk_label(ctx, speedText, NK_TEXT_LEFT);
      clockSpeed = (int32_t)(clockScale * 100.0f);
      nk_slider_int(ctx, 10, &clockSpeed, 200, 1);
      if (nk_menu_item_label(ctx, "Set clock speed to 100%", NK_TEXT_LEFT)) {
        clockSpeed = 100;
      }
      VSmile_SetClockScale(clockSpeed / 100.0f);

      nk_menu_end(ctx);
    }

    // Graphics
    if (nk_menu_begin_label(ctx, "Video", NK_TEXT_CENTERED, nk_vec2(180, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "Toggle Layer 0", NK_TEXT_LEFT)) {
        PPU_ToggleLayer(0);
      }
      if (nk_menu_item_label(ctx, "Toggle Layer 1", NK_TEXT_LEFT)) {
        PPU_ToggleLayer(1);
      }
      if (nk_menu_item_label(ctx, "Toggle Sprites", NK_TEXT_LEFT)) {
        PPU_ToggleLayer(2);
      }
      if (nk_menu_item_label(ctx, "Toggle Sprite Outlines", NK_TEXT_LEFT)) {
        PPU_ToggleSpriteOutlines();
      }
      if (nk_menu_item_label(ctx, "Toggle Flip Visual", NK_TEXT_LEFT)) {
        PPU_ToggleFlipVisual();
      }

      nk_menu_end(ctx);
    }

    // Debug
    if (nk_menu_begin_label(ctx, "Audio", NK_TEXT_CENTERED, nk_vec2(120, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);
      if (nk_menu_item_label(ctx, "Toggle Channels", NK_TEXT_LEFT)) {
        showChannels = true;
      }
      // if (nk_menu_item_label(ctx, "", NK_TEXT_LEFT)) {
      //
      // }
      // if (nk_menu_item_label(ctx, "", NK_TEXT_LEFT)) {
      //
      // }
      // if (nk_menu_item_label(ctx, "", NK_TEXT_LEFT)) {
      //
      // }
      // if (nk_menu_item_label(ctx, "", NK_TEXT_LEFT)) {
      //
      // }

      nk_menu_end(ctx);
    }

    // POPUP WINDOWS //

    // About window
    if (showAbout) {
      static struct nk_rect aboutBox = { 20, 40, 600, 400 };
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_CLOSABLE, aboutBox)) {
          nk_layout_row_dynamic(ctx, 25, 1);
          nk_label(ctx, "V.Frown - The Experimental V.Smile Emulator", NK_TEXT_LEFT);
          nk_label(ctx, "By Ian S. (Schnert0) and co.", NK_TEXT_LEFT);
          nk_label(ctx, "V.Frown emulator core is licensed under the MIT license.",  NK_TEXT_LEFT);
          nk_label(ctx, "nuklear UI library is licensed under the public domain license.",  NK_TEXT_LEFT);
          nk_label(ctx, "Sokol libraries are licensed under the zlib/libpng license.",  NK_TEXT_LEFT);
          nk_label(ctx, "Sokol_GP is licensed under the MIT-0 license.",  NK_TEXT_LEFT);
          nk_label(ctx, "ini.h is dual licensed under the MIT and public domain licenses.",  NK_TEXT_LEFT);
          nk_label(ctx, "for source code and a full list of contributors, go to:", NK_TEXT_LEFT);
          nk_label(ctx, "https://github.com/Schnert0/VFrown", NK_TEXT_LEFT);
          nk_popup_end(ctx);
      } else showAbout = false;
    }

    // Help window
    if (showHelp) {
      static struct nk_rect helpBox = { 20, 40, 600, 400 };
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Help", NK_WINDOW_CLOSABLE, helpBox)) {
          nk_layout_row_dynamic(ctx, 20, 1);
          nk_label(ctx, "Drag and drop a ROM on the window to start playing!", NK_TEXT_LEFT);
          nk_label(ctx, "~ - Toggle FullScreen", NK_TEXT_LEFT);
          nk_label(ctx, "1 - Toggle Layer 0", NK_TEXT_LEFT);
          nk_label(ctx, "2 - Toggle Layer 1", NK_TEXT_LEFT);
          nk_label(ctx, "3 - Toggle Sprites", NK_TEXT_LEFT);
          nk_label(ctx, "P - Pause", NK_TEXT_LEFT);
          nk_label(ctx, "O - Step", NK_TEXT_LEFT);
          // nk_label(ctx, "6 - Sprite Outlines", NK_TEXT_LEFT);
          // nk_label(ctx, "7 - Sprite Flip Visualization", NK_TEXT_LEFT);
          // nk_label(ctx, "8 - Sound Oscilloscope Visualization", NK_TEXT_LEFT);
          nk_label(ctx, "0 - Reset", NK_TEXT_LEFT);
          // nk_label(ctx, "F1 - Toggle LED Visualization", NK_TEXT_LEFT);
          // nk_label(ctx, "F10 - Close Emulator", NK_TEXT_LEFT);
          nk_label(ctx, "z - RED", NK_TEXT_LEFT);
          nk_label(ctx, "x - YELLOW", NK_TEXT_LEFT);
          nk_label(ctx, "c - GREEN", NK_TEXT_LEFT);
          nk_label(ctx, "v - BLUE", NK_TEXT_LEFT);
          nk_label(ctx, "a - HELP", NK_TEXT_LEFT);
          nk_label(ctx, "s - EXIT", NK_TEXT_LEFT);
          nk_label(ctx, "d - ABC", NK_TEXT_LEFT);
          nk_label(ctx, "[space] - ENTER", NK_TEXT_LEFT);
          nk_popup_end(ctx);
      } else showHelp = false;
    }

    // Audio Channels window
    if (showChannels) {
      static struct nk_rect channelsBox = { 20, 40, 150, 400 };
      if (nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Audio Channels", NK_WINDOW_CLOSABLE, channelsBox)) {
          nk_layout_row_dynamic(ctx, 20, 1);
          nk_checkbox_label(ctx, "Channel 0",  &chanEnable[0]);
          nk_checkbox_label(ctx, "Channel 1",  &chanEnable[1]);
          nk_checkbox_label(ctx, "Channel 2",  &chanEnable[2]);
          nk_checkbox_label(ctx, "Channel 3",  &chanEnable[3]);
          nk_checkbox_label(ctx, "Channel 4",  &chanEnable[4]);
          nk_checkbox_label(ctx, "Channel 5",  &chanEnable[5]);
          nk_checkbox_label(ctx, "Channel 6",  &chanEnable[6]);
          nk_checkbox_label(ctx, "Channel 7",  &chanEnable[7]);
          nk_checkbox_label(ctx, "Channel 8",  &chanEnable[8]);
          nk_checkbox_label(ctx, "Channel 9",  &chanEnable[9]);
          nk_checkbox_label(ctx, "Channel 10", &chanEnable[10]);
          nk_checkbox_label(ctx, "Channel 11", &chanEnable[11]);
          nk_checkbox_label(ctx, "Channel 12", &chanEnable[12]);
          nk_checkbox_label(ctx, "Channel 13", &chanEnable[13]);
          nk_checkbox_label(ctx, "Channel 14", &chanEnable[14]);
          nk_checkbox_label(ctx, "Channel 15", &chanEnable[15]);
          SPU_SetEnabledChannels(
            (chanEnable[0]       ) | (chanEnable[1]  <<  1) | (chanEnable[2]  <<  2) | (chanEnable[3]  <<  3) |
            (chanEnable[4]  <<  4) | (chanEnable[5]  <<  5) | (chanEnable[6]  <<  6) | (chanEnable[7]  <<  7) |
            (chanEnable[8]  <<  8) | (chanEnable[9]  <<  9) | (chanEnable[10] << 10) | (chanEnable[11] << 11) |
            (chanEnable[12] << 12) | (chanEnable[13] << 13) | (chanEnable[14] << 14) | (chanEnable[15] << 15)
          );
          nk_popup_end(ctx);
      } else showChannels = false;
    }

  }

  nk_end(ctx);
}
