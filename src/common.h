#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define PATH_CHAR '\\'
#else
#define PATH_CHAR '/'
#endif

#include "backend/libs.h"
#include "backend/backend.h"

#define RGB5A1_TO_RGBA8(color) (((color & 0x1f) << 19) | ((color & 0x3e0) << 6) | ((color & 0x7c00) >> 7) | 0xff000000)

#define CYCLES_PER_LINE 1716
#define LINES_PER_FIELD 262

// Is this correct?
#define SYSCLOCK 27000000
// #define SYSCLOCK (CYCLES_PER_LINE * LINES_PER_FIELD * 60)

// Sound output frequency
#define OUTPUT_FREQUENCY 48000

#define IN_RANGE(addr, start, size) \
  (addr >= start && addr < start+size)


enum {
  LED_GREEN,
  LED_BLUE,
  LED_YELLOW,
  LED_RED
};


enum {
  INPUT_UP,
  INPUT_DOWN,
  INPUT_LEFT,
  INPUT_RIGHT,

  INPUT_RED,
  INPUT_YELLOW,
  INPUT_BLUE,
  INPUT_GREEN,

  INPUT_ENTER,
  INPUT_HELP,
  INPUT_EXIT,
  INPUT_ABC,

  NUM_INPUTS,

  INPUT_DIRECTIONS = (1 << INPUT_UP) | (1 << INPUT_DOWN) | (1 << INPUT_LEFT) | (1 << INPUT_RIGHT),
  INPUT_COLORS     = (1 << INPUT_RED) | (1 << INPUT_YELLOW) | (1 << INPUT_BLUE) | (1 << INPUT_GREEN),
  INPUT_BUTTONS    = (1 << INPUT_ENTER) | (1 << INPUT_HELP) | (1 << INPUT_EXIT) | (1 << INPUT_ABC)
};


// Retrieved from vtech.pulkomandy.tk/doku.php?id=io
enum {
  REGION_US         = 0b1111,
  REGION_UK         = 0b1110,
  REGION_FRENCH     = 0b1101,
  REGION_SPANISH    = 0b1100,
  REGION_GERMAN     = 0b1011,
  REGION_ITALIAN    = 0b1010,
  REGION_DUTCH      = 0b1001,
  REGION_POLISH     = 0b1000, // \ unsure what 0b1000 is
  REGION_PORTUGUESE = 0b1000, // /
  REGION_CHINESE    = 0b0111
};


bool strequ(const char* a, const char* b);

#endif // COMMON_H
