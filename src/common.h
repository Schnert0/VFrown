#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "backend/backend.h"

#define CYCLES_PER_LINE 1716
#define LINES_PER_FIELD 262
// Is this correct? or is it 27000000?
#define SYSCLOCK 27000000
//(CYCLES_PER_LINE * LINES_PER_FIELD * 60)

#define IN_RANGE(addr, start, size) \
  (addr >= start && addr < start+size)


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

#endif // COMMON_H
