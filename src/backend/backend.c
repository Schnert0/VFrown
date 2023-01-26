#define SOKOL_IMPL
#include "backend.h"

static uint32_t pixelBuffer[240][320];
static uint32_t currButtons = 0, prevButtons = 0;

static float* sampleBuffer = NULL;
static int32_t* sampleCount = NULL;


bool Backend_Init() {
  saudio_setup(&(saudio_desc){
    .sample_rate  = 48000,
    .num_channels = 2
  });
  saudio_sample_rate();
  saudio_channels();

  return true;
}


void Backend_Cleanup() {
  saudio_shutdown();
}


void Backend_Update() {
  if (Backend_GetChangedButtons())
    Controller_UpdateButtons(0, currButtons);
  prevButtons = currButtons;
}


uint32_t* Backend_GetScanlinePointer(uint16_t scanlineNum) {
  return (uint32_t*)&pixelBuffer[scanlineNum][0];
}


bool Backend_RenderScanline() {
  return true;
}


bool Backend_GetInput() {
  return true;
}

uint32_t Backend_SetLedStates(uint8_t state) {
  return 0;
}

uint32_t Backend_GetcurrButtonstates() {
  return 0;
}


uint32_t Backend_GetChangedButtons() {
  return currButtons ^ prevButtons;
}


void Backend_InitAudioDevice(float* buffer, int32_t* count) {
  sampleBuffer = buffer;
  sampleCount = count;
}

void Backend_PushBuffer() {
  saudio_push(sampleBuffer, (*sampleCount)/2);
  memset(sampleBuffer, 0, (*sampleCount)*sizeof(float));
  (*sampleCount) = 0;
}


void Backend_PushOscilloscopeSample(uint8_t ch, int16_t sample) {
}

// Old SDL keycodes
// case SDLK_BACKQUOTE: SDLBackend_ToggleFullscreen(); break;
// case SDLK_1: PPU_ToggleLayer(0); break; // Layer 0
// case SDLK_2: PPU_ToggleLayer(1); break; // Layer 1
// case SDLK_3: PPU_ToggleLayer(2); break; // Sprites
// case SDLK_4: VSmile_TogglePause(); break;
// case SDLK_5: VSmile_Step(); break;
// case SDLK_6: PPU_ToggleSpriteOutlines(); break;
// case SDLK_7: PPU_ToggleFlipVisual(); break;
// case SDLK_8: this.isOscilloscopeView = !this.isOscilloscopeView; break;
// case SDLK_0: VSmile_Reset(); break;
// case SDLK_F10: running = false; break; // Exit
// case SDLK_F1: this.showLed ^=1; break;
// case SDLK_h: this.showHelp ^=1; break;
// case SDLK_g: this.showRegisters ^=1; break;
//
// case SDLK_p: SDLBackend_SetVSync(!this.isVsyncEnabled); break;
//
// case SDLK_UP:    this.currButtons |= (1 << INPUT_UP);     break;
// case SDLK_DOWN:  this.currButtons |= (1 << INPUT_DOWN);   break;
// case SDLK_LEFT:  this.currButtons |= (1 << INPUT_LEFT);   break;
// case SDLK_RIGHT: this.currButtons |= (1 << INPUT_RIGHT);  break;
// case SDLK_SPACE: this.currButtons |= (1 << INPUT_ENTER);  break;
// case SDLK_z:     this.currButtons |= (1 << INPUT_RED);    break;
// case SDLK_x:     this.currButtons |= (1 << INPUT_YELLOW); break;
// case SDLK_v:     this.currButtons |= (1 << INPUT_BLUE);   break;
// case SDLK_c:     this.currButtons |= (1 << INPUT_GREEN);  break;
// case SDLK_a:     this.currButtons |= (1 << INPUT_HELP);   break;
// case SDLK_s:     this.currButtons |= (1 << INPUT_EXIT);   break;
// case SDLK_d:     this.currButtons |= (1 << INPUT_ABC);    break;


void Backend_HandleInput(int32_t keycode, int32_t eventType) {
  if (eventType == SAPP_EVENTTYPE_KEY_DOWN) {
    switch (keycode) {
    case SAPP_KEYCODE_UP:    currButtons |= (1 << INPUT_UP);     break;
    case SAPP_KEYCODE_DOWN:  currButtons |= (1 << INPUT_DOWN);   break;
    case SAPP_KEYCODE_LEFT:  currButtons |= (1 << INPUT_LEFT);   break;
    case SAPP_KEYCODE_RIGHT: currButtons |= (1 << INPUT_RIGHT);  break;
    case SAPP_KEYCODE_SPACE: currButtons |= (1 << INPUT_ENTER);  break;
    case SAPP_KEYCODE_Z:     currButtons |= (1 << INPUT_RED);    break;
    case SAPP_KEYCODE_X:     currButtons |= (1 << INPUT_YELLOW); break;
    case SAPP_KEYCODE_V:     currButtons |= (1 << INPUT_BLUE);   break;
    case SAPP_KEYCODE_C:     currButtons |= (1 << INPUT_GREEN);  break;
    case SAPP_KEYCODE_A:     currButtons |= (1 << INPUT_HELP);   break;
    case SAPP_KEYCODE_S:     currButtons |= (1 << INPUT_EXIT);   break;
    case SAPP_KEYCODE_D:     currButtons |= (1 << INPUT_ABC);    break;
    }
  } else if (eventType == SAPP_EVENTTYPE_KEY_UP){
    switch (keycode) {
    case SAPP_KEYCODE_UP:    currButtons &= ~(1 << INPUT_UP);     break;
    case SAPP_KEYCODE_DOWN:  currButtons &= ~(1 << INPUT_DOWN);   break;
    case SAPP_KEYCODE_LEFT:  currButtons &= ~(1 << INPUT_LEFT);   break;
    case SAPP_KEYCODE_RIGHT: currButtons &= ~(1 << INPUT_RIGHT);  break;
    case SAPP_KEYCODE_SPACE: currButtons &= ~(1 << INPUT_ENTER);  break;
    case SAPP_KEYCODE_Z:     currButtons &= ~(1 << INPUT_RED);    break;
    case SAPP_KEYCODE_X:     currButtons &= ~(1 << INPUT_YELLOW); break;
    case SAPP_KEYCODE_V:     currButtons &= ~(1 << INPUT_BLUE);   break;
    case SAPP_KEYCODE_C:     currButtons &= ~(1 << INPUT_GREEN);  break;
    case SAPP_KEYCODE_A:     currButtons &= ~(1 << INPUT_HELP);   break;
    case SAPP_KEYCODE_S:     currButtons &= ~(1 << INPUT_EXIT);   break;
    case SAPP_KEYCODE_D:     currButtons &= ~(1 << INPUT_ABC);    break;
    }
  }
}
