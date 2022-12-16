#include "main.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("usage: %s <path/to/rom> [-sysrom <path/to/BIOS>]\n", argv[0]);
    return 0;
  }

  if(!VSmile_Init())
    return 0;

  VSmile_LoadROM(argv[1]);
  if (argc > 3 && strcmp(argv[2], "-sysrom") == 0) {
    VSmile_LoadSysRom(argv[3]);
  } else {
    VSmile_LoadSysRom("sysRom/sysrom.bin");
  }
  VSmile_Reset();
  VSmile_Run();

  VSmile_Cleanup();

  return 0;
}
