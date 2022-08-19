#include "sdl.h"

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
  }
  SDL_RenderPresent(this.renderer);
  this.currOscilloscopeSample = 0;

  this.final = clock();
  float elapsed = ((float)(this.final - this.initial)) / (float)CLOCKS_PER_SEC;
  if (elapsed < 0.01667f) {
    sleep(0.01667 - elapsed);
    this.initial = clock();
  }
}


uint16_t* SDLBackend_GetScanlinePointer(uint16_t scanlineNum) {
  return this.pixels + (320 * scanlineNum);
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
