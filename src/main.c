#include "main.h"

int main(int argc, char* argv[]) {

  if (argc != 2) {
    printf("usage: %s <path/to/rom>\n", argv[0]);
    return 0;
  }

  if(!VSmile_Init())
    return 0;

  VSmile_LoadROM(argv[1]);
  VSmile_Reset();
  VSmile_Run();

  VSmile_Cleanup();

  return 0;
}
