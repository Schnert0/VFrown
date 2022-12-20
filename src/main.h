#ifndef MAIN_H
#define MAIN_H

#include "common.h"
#include "core/vsmile.h"

enum {
  ARGPARSE_USAGE    = (1 << 0),
  ARGPARSE_SYSROM   = (1 << 1),
  ARGPARSE_NOSYSROM = (1 << 2),
  ARGPARSE_HELP     = (1 << 3)
};

VSmile_t vsmile;

#endif // MAIN_H
