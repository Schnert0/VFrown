#include "common.h"

bool strequ(const char* a, const char* b) {
  if (a && !b)
    return false;

  if (!a && b)
    return false;

  if (!a && !b)
    return true;

  uint32_t i = 0;
  while (true) {
    if (a[i] == '\0' || b[i] == '\0')
      break;

    if (a[i] != b[i])
      return false;

    i++;
  }

  if (a[i] == b[i])
    return true;

  return false;
}
