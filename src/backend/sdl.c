#include "sdl.h"
#include "../font.xpm"

static SDLBackend_t this;

bool SDLBackend_Init() {
  memset(&this, 0, sizeof(SDLBackend_t));

  SDL_Init(SDL_INIT_EVERYTHING);

  this.window = SDL_CreateWindow("V.Frown", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
  if (!this.window)
    return false;

  this.renderer = SDL_CreateRenderer(this.window, -1, SDL_RENDERER_ACCELERATED);// | SDL_RENDERER_PRESENTVSYNC);
  if (!this.renderer)
    return false;

  this.texture = SDL_CreateTexture(this.renderer, SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STREAMING, 320, 240);
  if (!this.texture)
    return false;

  this.pixels = malloc(320 * 240 * sizeof(uint16_t));
  if (!this.pixels)
    return false;
  memset(this.pixels, 0, 320 * 240 * sizeof(uint16_t));

  this.oscilloscopeTexture = SDL_CreateTexture(this.renderer, SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STREAMING, 256, 128);
  if (!this.oscilloscopeTexture)
    return false;

  for (int32_t i = 0; i < 16; i++) {
    this.oscilloscopePixels[i] = malloc(256 * 128 * sizeof(uint16_t));
    if (!this.oscilloscopePixels[i])
      return false;
    memset(this.oscilloscopePixels[i], 0, 256 * 128 * sizeof(uint16_t));
  }

  this.initial = clock();
  this.showLed = false;
  this.showHelp = false;
  this.showRegisters = false;
  return true;
}


void SDLBackend_Cleanup() {
  if (this.pixels)
    free(this.pixels);

  if (this.texture)
    SDL_DestroyTexture(this.texture);

  if (this.renderer)
    SDL_DestroyRenderer(this.renderer);

  if (this.window)
    SDL_DestroyWindow(this.window);

  if (this.oscilloscopeTexture)
    SDL_DestroyTexture(this.oscilloscopeTexture);

  for (int32_t i = 0; i < 16; i++) {
    if (this.oscilloscopePixels[i])
      free(this.oscilloscopePixels[i]);
  }
}

void SDLBackend_UpdateWindow() {
  if (this.isOscilloscopeView) {
    int32_t w, h;
    SDL_GetWindowSize(this.window, &w, &h);

    for (int32_t i = 0; i < 16; i++) {
      SDL_LockTextureToSurface(this.oscilloscopeTexture, NULL, &this.surface);
      memcpy(this.surface->pixels, this.oscilloscopePixels[i], 256 * 128 * sizeof(uint16_t));
      SDL_UnlockTexture(this.oscilloscopeTexture);

      SDL_Rect rect;
      rect.w = w / 4;
      rect.h = h / 4;
      rect.x = rect.w * (i  & 3);
      rect.y = rect.h * (i >> 2);

      SDL_RenderCopy(this.renderer, this.oscilloscopeTexture, NULL, &rect);

      // memset(this.oscilloscopePixels[i], 0, 256 * 128 * sizeof(uint16_t));
    }
  } else {
    SDL_LockTextureToSurface(this.texture, NULL, &this.surface);
    memcpy(this.surface->pixels, this.pixels, 320 * 240 * sizeof(uint16_t));
    SDL_UnlockTexture(this.texture);
    SDL_RenderCopy(this.renderer, this.texture, NULL, NULL);

    if (this.showLed) SDLBackend_RenderLeds(this.currLed);
  }
  if (this.showHelp) SDLBackend_PrintHelp();
  if (this.showRegisters) SDLBackend_PrintCpu();

  SDL_RenderPresent(this.renderer);
  this.currOscilloscopeSample = 0;

  // Delay until the end of the frame
  if (!this.isVsyncEnabled) {
    this.final = clock();
    float elapsed = (this.final - this.initial);
    if (elapsed < 16666.67f) {
      usleep(16666.67f - elapsed);
    }
    this.initial = clock();
  }

}
void SDLBackend_RenderLeds(uint8_t ledState) {
  int alpha = 128;
  if (ledState & 1<<LED_RED) SDL_SetRenderDrawColor(this.renderer, 255, 0, 0, alpha);
  else SDL_SetRenderDrawColor(this.renderer, 0, 0, 0, alpha);
  SDL_RenderDrawCircle(this.renderer, 50, 30, 20);

  if (ledState & 1<<LED_YELLOW) SDL_SetRenderDrawColor(this.renderer, 255, 255, 0, alpha);
  else SDL_SetRenderDrawColor(this.renderer, 0, 0, 0, alpha);
  SDL_RenderDrawCircle(this.renderer, 110, 30, 20);

  if (ledState & 1<<LED_BLUE) SDL_SetRenderDrawColor(this.renderer, 0, 0, 255, alpha);
  else SDL_SetRenderDrawColor(this.renderer, 0, 0, 0, alpha);
  SDL_RenderDrawCircle(this.renderer, 170, 30, 20);

  if (ledState & 1<<LED_GREEN) SDL_SetRenderDrawColor(this.renderer, 0, 255, 0, alpha);
  else SDL_SetRenderDrawColor(this.renderer, 0, 0, 0, alpha);
  SDL_RenderDrawCircle(this.renderer, 230, 30, 20);

}

void SDLBackend_PrintHelp() {
  SDLBackend_PrintString("\\ - Toggle FullScreen",  10, 30);
  SDLBackend_PrintString("1 - Toggle layer 0",      10, 46);
  SDLBackend_PrintString("2 - Toggle layer 1",      10, 62);
  SDLBackend_PrintString("3 - Toggle layer 2",      10, 78);
  SDLBackend_PrintString("4 - Pause",               10, 94);
  SDLBackend_PrintString("5 - Step",                10, 110);
  SDLBackend_PrintString("6 - Sprite Outline",      10, 126);
  SDLBackend_PrintString("7 - Flip Visual",         10, 142);
  SDLBackend_PrintString("8 - Oscilloscope",        10, 158);
  SDLBackend_PrintString("0 - Reset",               10, 174);
  SDLBackend_PrintString("F1 - Toggle Leds",        10, 190);
  SDLBackend_PrintString("F10 - Quit Emu",          10, 206);

  SDLBackend_PrintString("z - RED",                 280, 30);
  SDLBackend_PrintString("x - YELLOW",              280, 46);
  SDLBackend_PrintString("c - GREEN",               280, 62);
  SDLBackend_PrintString("v - BLUE",                280, 78);
  SDLBackend_PrintString("a - HELP",                280, 94);
  SDLBackend_PrintString("s - EXIT",                280, 110);
  SDLBackend_PrintString("d - ABC",                 280, 126);
  SDLBackend_PrintString("space - BUTTON",          250, 142);
}

void SDLBackend_PrintCpu() {
  CPU_t cpu = CPU_GetCpu();
  char raw[9], sp[9], r1[9], r2[9], r3[9], r4[9], bp[9], sr[9], pc[11], srd[11];

  sprintf(raw, "RAW %04x", cpu.ins.raw);  SDLBackend_PrintString(raw, 10, 10);

  sprintf(sp, "SP %04x", cpu.sp);     SDLBackend_PrintString(sp, 10, 24);
  sprintf(r1, "R1 %04x", cpu.r1);     SDLBackend_PrintString(r1, 10, 38);
  sprintf(r2, "R2 %04x", cpu.r2);     SDLBackend_PrintString(r2, 10, 52);
  sprintf(r3, "R3 %04x", cpu.r3);     SDLBackend_PrintString(r3, 10, 66);
  sprintf(r4, "R4 %04x", cpu.r4);     SDLBackend_PrintString(r4, 10, 80);
  sprintf(bp, "BP %04x", cpu.bp);     SDLBackend_PrintString(bp, 10, 94);
  sprintf(sr, "SR %04x", cpu.sr.raw); SDLBackend_PrintString(sr, 10, 108);
  sprintf(pc, "PC %02x%04x", cpu.sr.cs, cpu.pc);     SDLBackend_PrintString(pc, 10, 122);
  SDLBackend_PrintString("c s n z ds", 115, 108);
  sprintf(srd, "%x %x %x %x %2x", cpu.sr.c, cpu.sr.s, cpu.sr.n, cpu.sr.z, cpu.sr.ds);
  SDLBackend_PrintString(srd, 115, 122);
}

uint16_t* SDLBackend_GetScanlinePointer(uint16_t scanlineNum) {
  return this.pixels + (320 * scanlineNum);
}

void SDLBackend_PrintString(char *str, int x, int y) {
  size_t len = strlen(str);
  int i;
  for(i = 0 ; i < len; i++) {
    SDLBackend_PrintChar(str[i], y, x+=11);
  }
}
void SDLBackend_PrintChar(char c, int x, int y) {
  int i,j;
  char p;
  SDL_SetRenderDrawColor(this.renderer, 0,0,0,0);
  for(i=0; i<17; i++)
    for(j=0;j<11; j++) {
      p = font[i+3][((c-32)*11)+j];
      if (p == ' ') SDL_RenderDrawPoint(this.renderer, y+j, x+i);
    }
}

bool SDLBackend_RenderScanline() {
  return !this.isOscilloscopeView;
}


void SDLBackend_ToggleFullscreen() {
  this.isFullscreen = !this.isFullscreen;

  if (this.isFullscreen) {
    SDL_SetWindowFullscreen(this.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  } else {
    SDL_SetWindowFullscreen(this.window, 0);
  }
}


void SDLBackend_SetVSync(bool vsyncEnabled) {
  this.isVsyncEnabled = vsyncEnabled;
  SDL_RenderSetVSync(this.renderer, vsyncEnabled);
  VSmile_Log("VSync is %sabled", vsyncEnabled ? "en" : "dis");
}


bool SDLBackend_GetInput() {
  bool running = true;

  this.prev = this.curr;

  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      running = false;
      break;

    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_BACKQUOTE: SDLBackend_ToggleFullscreen(); break;
      case SDLK_1: PPU_ToggleLayer(0); break; // Layer 0
      case SDLK_2: PPU_ToggleLayer(1); break; // Layer 1
      case SDLK_3: PPU_ToggleLayer(2); break; // Sprites
      case SDLK_4: VSmile_TogglePause(); break;
      case SDLK_5: VSmile_Step(); break;
      case SDLK_6: PPU_ToggleSpriteOutlines(); break;
      case SDLK_7: PPU_ToggleFlipVisual(); break;
      case SDLK_8: this.isOscilloscopeView = !this.isOscilloscopeView; break;
      case SDLK_0: VSmile_Reset(); break;
      case SDLK_F10: running = false; break; // exit
      case SDLK_F1: this.showLed ^=1; break;
      case SDLK_h: this.showHelp ^=1; break;
      case SDLK_g: this.showRegisters ^=1; break;

      case SDLK_p: SDLBackend_SetVSync(!this.isVsyncEnabled); break;

      case SDLK_UP:    this.curr |= (1 << INPUT_UP);     break;
      case SDLK_DOWN:  this.curr |= (1 << INPUT_DOWN);   break;
      case SDLK_LEFT:  this.curr |= (1 << INPUT_LEFT);   break;
      case SDLK_RIGHT: this.curr |= (1 << INPUT_RIGHT);  break;
      case SDLK_SPACE: this.curr |= (1 << INPUT_ENTER);  break;
      case SDLK_z:     this.curr |= (1 << INPUT_RED);    break;
      case SDLK_x:     this.curr |= (1 << INPUT_YELLOW); break;
      case SDLK_v:     this.curr |= (1 << INPUT_BLUE);   break;
      case SDLK_c:     this.curr |= (1 << INPUT_GREEN);  break;
      case SDLK_a:     this.curr |= (1 << INPUT_HELP);   break;
      case SDLK_s:     this.curr |= (1 << INPUT_EXIT);   break;
      case SDLK_d:     this.curr |= (1 << INPUT_ABC);    break;
      }
      break;

    case SDL_KEYUP:
      switch (event.key.keysym.sym) {
        case SDLK_UP:    this.curr &= ~(1 << INPUT_UP);     break;
        case SDLK_DOWN:  this.curr &= ~(1 << INPUT_DOWN);   break;
        case SDLK_LEFT:  this.curr &= ~(1 << INPUT_LEFT);   break;
        case SDLK_RIGHT: this.curr &= ~(1 << INPUT_RIGHT);  break;
        case SDLK_SPACE: this.curr &= ~(1 << INPUT_ENTER);  break;
        case SDLK_z:     this.curr &= ~(1 << INPUT_RED);    break;
        case SDLK_x:     this.curr &= ~(1 << INPUT_YELLOW); break;
        case SDLK_v:     this.curr &= ~(1 << INPUT_BLUE);   break;
        case SDLK_c:     this.curr &= ~(1 << INPUT_GREEN);  break;
        case SDLK_a:     this.curr &= ~(1 << INPUT_HELP);   break;
        case SDLK_s:     this.curr &= ~(1 << INPUT_EXIT);   break;
        case SDLK_d:     this.curr &= ~(1 << INPUT_ABC);    break;
      }
      break;
    }
  }

  if (SDLBackend_GetChangedButtons()) {
    Controller_UpdateButtons(0, this.curr);
  }

  return running;
}


uint32_t SDLBackend_GetButtonStates() {
  return this.curr;
}


uint32_t SDLBackend_GetChangedButtons() {
  return this.prev ^ this.curr;
}

uint32_t SDLBackend_SetLedStates(uint8_t state) {
  this.currLed = state;
  return this.currLed;
}

void SDLBackend_InitAudioDevice() {
  SDL_AudioSpec want;

  memset(&want, 0, sizeof(want));
  want.freq     = 44100;
  want.format   = AUDIO_S16SYS;
  want.channels = 1;
  want.samples  = 4096;

  this.audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, NULL, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  SDL_PauseAudioDevice(this.audioDevice, false);
}


void SDLBackend_PushAudioSample(int16_t leftSample, int16_t rightSample) {
  this.audioBuffer[0] = leftSample;
  this.audioBuffer[1] = rightSample;

  SDL_QueueAudio(this.audioDevice, this.audioBuffer, 2);

  if (this.currOscilloscopeSample < 735)
    this.currOscilloscopeSample++;
}

static const uint16_t channelColors[] = {
  0x7c00, 0x83e0, 0x001f, 0xffe0,
  0x7c1f, 0x83ff, 0xfe24, 0xffff,
  0x7c00, 0x83e0, 0x001f, 0xffe0,
  0x7c1f, 0x83ff, 0xfe24, 0xffff
};


void SDLBackend_PushOscilloscopeSample(uint8_t ch, int16_t sample) {
  if (!this.isOscilloscopeView)
    return;

  if (this.currOscilloscopeSample % 3)
    return;

  sample = (sample >> 9) + 64;

  int32_t start, end;
  if (sample > this.prevSample[ch]) {
    start = this.prevSample[ch];
    end = sample;
  } else if (sample < this.prevSample[ch]) {
    start = sample;
    end = this.prevSample[ch];
  } else {
    start = sample;
    end = sample + 1;
  }

  for (int32_t i = 0; i < 128; i++) {
    this.oscilloscopePixels[ch][(i * 256) + (this.currOscilloscopeSample / 3)    ] = 0x0000;
    this.oscilloscopePixels[ch][(i * 256) + (this.currOscilloscopeSample / 3) + 1] = 0x0000;
  }

  for (int32_t i = start; i < end; i++) {
    this.oscilloscopePixels[ch][(i * 256) + (this.currOscilloscopeSample / 3)] = channelColors[ch];
  }

  this.prevSample[ch] = sample;
}

uint32_t SDL_RenderDrawCircle(SDL_Renderer *renderer, int32_t x, int32_t y, uint32_t radius) {
  int offsetx, offsety, d;
  int status;

  offsetx = 0;
  offsety = radius;
  d = radius - 1;
  status = 0;

  while (offsety >= offsetx) {
    status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);

    status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

    if (status < 0) {
      status = -1;
      break;
    }
    if (d >= 2*offsetx) {
      d -= 2*offsetx + 1;
      offsetx += 1;
    } else if (d < 2 * (radius - offsety)) {
      d += 2 * offsety - 1;
      offsety -= 1;
    } else {
      d += 2 * (offsety - offsetx -1);
      offsety -= 1;
      offsetx += 1;
    }
  }
  return status;
}
