#ifndef MAIN_H
#define MAIN_H

#include "common.h"
#include "core/vsmile.h"

enum {
  ARGPARSE_USAGE    = (1 << 0),
  ARGPARSE_HELP     = (1 << 1),
  ARGPARSE_SYSROM   = (1 << 2),
  ARGPARSE_NOSYSROM = (1 << 3),
  ARGPARSE_REGION   = (1 << 4),
  ARGPARSE_NOINTRO  = (1 << 5)
};

enum {
  REGION_ITALIAN    = 0x2,
  REGION_CHINESE    = 0x7,
  REGION_PORTUGUESE = 0x8,
  REGION_DUTCH      = 0x9,
  REGION_GERMAN     = 0xa,
  REGION_SPANISH    = 0xb,
  REGION_FRENCH     = 0xc,
  REGION_UK         = 0xe,
  REGION_US         = 0xf
};

VSmile_t vsmile;

#endif // MAIN_H
