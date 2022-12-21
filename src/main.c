#include "main.h"

// Command line Argument Parsing
static uint32_t argparse = 0;
static uint32_t sysromIndex = 0;

static const char* usageString = "usage: %s <path/to/rom> [-help][-nosysrom][-sysrom <path/to/BIOS>]\n";

static bool parseArguments(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    printf(usageString, argv[0]);
    return 0;
  }

  VSmile_Init();

  if (!parseArguments(argc, argv))
    return 0;

  VSmile_LoadROM(argv[1]);

  // Run the system
  VSmile_Reset();
  VSmile_Run();
  VSmile_Cleanup();

  return 0;
}


static bool parseArguments(int argc, char* argv[]) {
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

  // ***  Handling of arguments *** //

  // If an invalid combination of parameters was used, print out usage and quit.
  if (argparse & ARGPARSE_USAGE) {
    printf(usageString, argv[0]);
    return false;
  }

  if (argparse & ARGPARSE_HELP) { // -help
    printf(
      "\n*** V.Frown - The Experimental V.Smile Emulator ***\n\n"

      "V.Frown requires a path to a V.Smile ROM file to run. Some games also\n"
      "require a system rom/bios file to boot games. If one is not provided\n"
      "with the '-sysrom' flag, V.Frown will try to find one under the path\n"
      "'sysrom/sysrom.bin'.\n"

      "\n"

      "flags:\n"

      "\t-help\n"
      "\t\tThis help screen.\n"

      "\n"

      "\t-nosysrom\n"
      "\t\tBoot V.Smile without loading a system rom/bios.\n"

      "\n"

      "\t-sysrom <path>\n"
      "\t\tBoot V.Smile with system rom/bios at specified path.\n"

      "\n"

      // "\t-region <region>\n"
      // "\t\tSet V.Smile to a specified region. If no region\n"
      // "\t\tis selected, the emulator defaults to US.\n"
      // "\t\tvalid regions:\n"
      // "\t\t\t'US'\n"
      // "\t\t\t'UK'\n"
      // "\t\t\t'Italian'\n"
      // "\t\t\t'German'\n"
      // "\t\t\t'Spanish'\n"
      // "\t\t\t'Chinese'\n"
      //
      // "\n"
      //
      // "\t-noROM\n"
      // "\t\tBoot V.Smile as if it had no cartridge inserted.\n"
      //
      // "\n"
      //
      // "\t-nointro\n"
      // "\t\tDisable the V.Tech/V.Smile boot screen.\n"
      //
      // "\n"
    );
    return false;
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

  return true;
}
