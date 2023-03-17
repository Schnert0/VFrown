#define SOKOL_IMPL
#include "backend.h"

static Backend_t this;

bool Backend_Init() {
  memset(&this, 0, sizeof(Backend_t));

  saudio_setup(&(saudio_desc){
    .sample_rate  = 48000,
    .num_channels = 2
  });
  saudio_sample_rate();
  saudio_channels();

  this.sampleBuffer = NULL;
  this.sampleCount = NULL;
  this.saveFile = NULL;

  this.emulationSpeed = 1.0f;
  this.currButtons = 0;
  this.prevButtons = 0;

  return true;
}


void Backend_Cleanup() {
  saudio_shutdown();
}


void Backend_Update() {
  if (Backend_GetChangedButtons())
    Controller_UpdateButtons(0, this.currButtons);
  this.prevButtons = this.currButtons;
}


// Find and store the title of the current ROM
void Backend_GetFileName(const char* path) {
  if (!path)
    return;

  // Get the end of the file name before the '.' of the file extension
  char* end = (char*)(path + strlen(path));
  while (end != path && (*end) != PATH_CHAR && (*end) != '.') {
    end--;
  }
  // If the found character is not '.', then there's probably no file extension
  if (end == path || (*end) != '.')
    end = (char*)(path + strlen(path));

  // Start searching for the start of the title without the directory
  char* start = end;
  while (start != path && (*start) != PATH_CHAR) {
    start--;
  }
  start++; // Moved one too far

  int32_t size = (int32_t)(end - start);
  if (size > 256)
    size = 256;

  for (int i = 0; i < size; i++)
    this.title[i] = start[i];
  this.title[255] = '\0';
}


void Backend_WriteSave(void* data, uint32_t size) {
  uint8_t* bytes = (uint8_t*)data;
  for (int32_t i = 0; i < size; i++)
    fputc(bytes[i], this.saveFile);

  uint32_t alignmentSize = (size + 0xf) & ~0xf;
  for (int32_t i = 0; i < (alignmentSize - size); i++)
    fputc(0x00, this.saveFile);

}


void Backend_ReadSave(void* data, uint32_t size) {
  uint8_t* bytes = (uint8_t*)data;
  for (int32_t i = 0; i < size; i++)
    bytes[i] = fgetc(this.saveFile);

  uint32_t alignmentSize = (size + 0xf) & ~0xf;
  for (int32_t i = 0; i < (alignmentSize - size); i++)
    fgetc(this.saveFile);
}


void Backend_SaveState() {
  char path[256];
  snprintf((char*)&path, 256, "savestates/%s.vfss", (const char*)&this.title);

  VSmile_Log("Saving state from '%s'...", (const char*)&path);
  this.saveFile = fopen(path, "wb+");
  if (!this.saveFile) {
    VSmile_Warning("Save state failed!");
    return;
  }

  Bus_SaveState();
  CPU_SaveState();
  PPU_SaveState();
  SPU_SaveState();
  VSmile_SaveState();
  Controller_SaveState();
  DMA_SaveState();
  GPIO_SaveState();
  Misc_SaveState();
  Timers_SaveState();
  UART_SaveState();

  Backend_WriteSave(&this.currButtons, sizeof(uint32_t));
  Backend_WriteSave(&this.prevButtons, sizeof(uint32_t));

  fclose(this.saveFile);

  VSmile_Log("State saved!");
}

void Backend_LoadState() {
  char path[256];
  snprintf((char*)&path, 256, "savestates/%s.vfss", (const char*)&this.title);

  VSmile_Log("Loading state from '%s'...", (const char*)&path);
  this.saveFile = fopen(path, "rb");
  if (!this.saveFile) {
    VSmile_Warning("Load state failed!");
    return;
  }

  Bus_LoadState();
  CPU_LoadState();
  PPU_LoadState();
  SPU_LoadState();
  VSmile_LoadState();
  Controller_LoadState();
  DMA_LoadState();
  GPIO_LoadState();
  Misc_LoadState();
  Timers_LoadState();
  UART_LoadState();

  Backend_ReadSave(&this.currButtons, sizeof(uint32_t));
  Backend_ReadSave(&this.prevButtons, sizeof(uint32_t));

  fclose(this.saveFile);

  VSmile_Log("State loaded!");
}


float Backend_GetSpeed() {
  return this.emulationSpeed;
}


void Backend_SetSpeed(float newSpeed) {
  this.emulationSpeed = newSpeed;
}


uint32_t* Backend_GetScanlinePointer(uint16_t scanlineNum) {
  return (uint32_t*)&this.pixelBuffer[scanlineNum][0];
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
  return this.currButtons ^ this.prevButtons;
}


void Backend_InitAudioDevice(float* buffer, int32_t* count) {
  this.sampleBuffer = buffer;
  this.sampleCount = count;
}

void Backend_PushBuffer() {
  saudio_push(this.sampleBuffer, (*this.sampleCount)/2);
  memset(this.sampleBuffer, 0, (*this.sampleCount)*sizeof(float));
  (*this.sampleCount) = 0;
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
    case SAPP_KEYCODE_0: VSmile_Reset(); break;
    case SAPP_KEYCODE_1: PPU_ToggleLayer(0); break;
    case SAPP_KEYCODE_2: PPU_ToggleLayer(1); break;
    case SAPP_KEYCODE_3: PPU_ToggleLayer(2); break;

    case SAPP_KEYCODE_O: VSmile_Step(); break;
    case SAPP_KEYCODE_P: VSmile_SetPause(!VSmile_GetPaused()); break;

    case SAPP_KEYCODE_J:
      if (this.title[0])
        Backend_SaveState();
      break;
    case SAPP_KEYCODE_K:
      if (this.title[0])
        Backend_LoadState();
      break;

    case SAPP_KEYCODE_UP:    this.currButtons |= (1 << INPUT_UP);     break;
    case SAPP_KEYCODE_DOWN:  this.currButtons |= (1 << INPUT_DOWN);   break;
    case SAPP_KEYCODE_LEFT:  this.currButtons |= (1 << INPUT_LEFT);   break;
    case SAPP_KEYCODE_RIGHT: this.currButtons |= (1 << INPUT_RIGHT);  break;
    case SAPP_KEYCODE_SPACE: this.currButtons |= (1 << INPUT_ENTER);  break;
    case SAPP_KEYCODE_Z:     this.currButtons |= (1 << INPUT_RED);    break;
    case SAPP_KEYCODE_X:     this.currButtons |= (1 << INPUT_YELLOW); break;
    case SAPP_KEYCODE_V:     this.currButtons |= (1 << INPUT_BLUE);   break;
    case SAPP_KEYCODE_C:     this.currButtons |= (1 << INPUT_GREEN);  break;
    case SAPP_KEYCODE_A:     this.currButtons |= (1 << INPUT_HELP);   break;
    case SAPP_KEYCODE_S:     this.currButtons |= (1 << INPUT_EXIT);   break;
    case SAPP_KEYCODE_D:     this.currButtons |= (1 << INPUT_ABC);    break;
    }
  } else if (eventType == SAPP_EVENTTYPE_KEY_UP){
    switch (keycode) {
    case SAPP_KEYCODE_UP:    this.currButtons &= ~(1 << INPUT_UP);     break;
    case SAPP_KEYCODE_DOWN:  this.currButtons &= ~(1 << INPUT_DOWN);   break;
    case SAPP_KEYCODE_LEFT:  this.currButtons &= ~(1 << INPUT_LEFT);   break;
    case SAPP_KEYCODE_RIGHT: this.currButtons &= ~(1 << INPUT_RIGHT);  break;
    case SAPP_KEYCODE_SPACE: this.currButtons &= ~(1 << INPUT_ENTER);  break;
    case SAPP_KEYCODE_Z:     this.currButtons &= ~(1 << INPUT_RED);    break;
    case SAPP_KEYCODE_X:     this.currButtons &= ~(1 << INPUT_YELLOW); break;
    case SAPP_KEYCODE_V:     this.currButtons &= ~(1 << INPUT_BLUE);   break;
    case SAPP_KEYCODE_C:     this.currButtons &= ~(1 << INPUT_GREEN);  break;
    case SAPP_KEYCODE_A:     this.currButtons &= ~(1 << INPUT_HELP);   break;
    case SAPP_KEYCODE_S:     this.currButtons &= ~(1 << INPUT_EXIT);   break;
    case SAPP_KEYCODE_D:     this.currButtons &= ~(1 << INPUT_ABC);    break;
    }
  }
}
