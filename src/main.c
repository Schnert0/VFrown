#include "main.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("usage: %s <path/to/rom> [-sysrom <path/to/BIOS>]\n", argv[0]);
    return 0;
  }

  if(!VSmile_Init())
    return 0;

  VSmile_LoadROM(argv[1]);

  if (argc == 1) {
    if (strequ(argv[2], "-nosysrom"))
      VSmile_Log("Starting emulation without system rom...");
  }
  else if (argc == 3) {
    if (strequ(argv[2], "-sysrom"))
      VSmile_LoadSysRom(argv[3]);
  } else {
    VSmile_LoadSysRom("sysRom/sysrom.bin");
  }
  VSmile_Reset();
  VSmile_Run();

  VSmile_Cleanup();

  return 0;
}
