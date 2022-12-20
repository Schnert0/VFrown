#include "main.h"

// Command line Argument Parsing
static uint32_t argparse = 0;
static uint32_t sysromIndex = 0;

static const char* usageString = "usage: %s <path/to/rom> [-help][-nosysrom][-sysrom <path/to/BIOS>]\n";

static void parseArguments(int argc, char* argv[]) {
  bool sysromOptionUsed = false;

  // Parse command line arguments
  for (int32_t i = 2; i < argc; i++) {
    if (strequ(argv[i], "-nosysrom")) {
      if (sysromOptionUsed)
        argparse |= ARGPARSE_USAGE;

      argparse |= ARGPARSE_NOSYSROM;

      sysromOptionUsed = true;
    }
    else if (strequ(argv[i], "-sysrom")) {
      if (sysromOptionUsed)
        argparse |= ARGPARSE_USAGE;

      // Should be argument for path
      if (i == argc-1) {
        argparse |= ARGPARSE_USAGE;
      } else {
        argparse |= ARGPARSE_SYSROM;
        sysromIndex = ++i;
      }
      sysromOptionUsed = true;
    }
    else if (strequ(argv[i], "-help")) {
      argparse |= ARGPARSE_HELP;
    }
    else {
      argparse |= ARGPARSE_USAGE;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    printf(usageString, argv[0]);
    return 0;
  }

  parseArguments(argc, argv);

  VSmile_Init();
  VSmile_LoadROM(argv[1]);

  // If an invalid combination of parameters was used, print out usage and quit.
  if (argparse & ARGPARSE_USAGE) {
    printf(usageString, argv[0]);
    return 0;
  }

  // In case we want to do a more detailed help list...
  if (argparse & ARGPARSE_HELP) { // -help
    printf("usage: %s <path/to/rom> [-help][-nosysrom][-sysrom <path/to/BIOS>]\n", argv[0]);
    return 0;
  }

  // sysrom flags
  if (argparse & ARGPARSE_NOSYSROM) { // -nosysrom
    VSmile_Log("Starting emulation without system rom...");
  }
  else if (argparse & ARGPARSE_SYSROM) { // -sysrom <...>
    VSmile_LoadSysRom(argv[sysromIndex]);
  }
  else { // no sysrom flags
    VSmile_LoadSysRom("sysrom/sysrom.bin");
  }

  // Run the system
  VSmile_Reset();
  VSmile_Run();
  VSmile_Cleanup();

  return 0;
}
