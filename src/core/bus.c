#include "bus.h"
#include "vsmile.h"

static Bus_t this;

#define UART_CTRL  0x30
#define UART_STAT  0x31
#define UART_RESET 0x32
#define UART_TX    0x35
#define UART_RX    0x36

bool Bus_Init() {
  memset(&this, 0, sizeof(Bus_t));

  if (!GPIO_Init())   return false;
  if (!Timers_Init()) return false;
  if (!Misc_Init())   return false;
  if (!UART_Init())   return false;

  // this.rxEmpty = true;

  // this.io[0x00] = 0x001f; // 3d00 - GPIO Control
  // this.io[0x01] = 0xffff; // 3d01-3d05 - IOA
  // this.io[0x02] = 0xffff;
  // this.io[0x03] = 0xffff;
  // this.io[0x04] = 0xffff;
  // this.io[0x05] = 0xffff;
  //
  // this.io[0x06] = 0x00ff; // 3d06-3d0a - IOB
  // this.io[0x07] = 0x00ff;
  // this.io[0x08] = 0x00ff;
  // this.io[0x09] = 0x00ff;
  // this.io[0x0a] = 0x00ff;
  //
  // this.io[0x0b] = 0xffff; // 3d0b-3d0f - IOC
  // this.io[0x0c] = 0xffff;
  // this.io[0x0d] = 0xffff;
  // this.io[0x0e] = 0xffff;
  // this.io[0x0f] = 0xffff;

  // this.io[0x10] = 0x000f; // 3d10 - Timebase freq
  // this.io[0x11] = 0x0;    // 3d11-3d1f
  // this.io[0x12] = 0x0;
  // this.io[0x13] = 0x0;
  // this.io[0x14] = 0x0;
  // this.io[0x15] = 0x0;
  // this.io[0x16] = 0x0;
  // this.io[0x17] = 0x0;
  // this.io[0x18] = 0x0;
  // this.io[0x19] = 0x0;
  // this.io[0x1a] = 0x0;
  // this.io[0x1b] = 0x0;
  // this.io[0x1c] = 0x0;
  // this.io[0x1d] = 0x0;
  // this.io[0x1e] = 0x0;
  // this.io[0x1f] = 0x0;

  // this.io[0x20] = 0x4006; // 3d20 - System control
  // this.io[0x21] = 0x3ffb; // 3d21 - IRQ control
  // this.io[0x22] = 0x7fff; // 3d22 - IRQ Status
  // this.io[0x23] = 0x003e; // 3d23 - Memory control
  // this.io[0x24] = 0xffff; // 3d24 - Watchdog
  // this.io[0x25] = 0x2002; // 3d25 - ADC Ctrl
  // this.io[0x26] = 0x0;
  // this.io[0x27] = 0x0;
  // this.io[0x28] = 0xffff; // 3d28 - Sleep
  // this.io[0x29] = 0x0080; // 3d29 - Wakeup source
  // this.io[0x2a] = 0x00ff; // 3d2a - Wakeup delay
  // this.io[0x2b] = 0x0001; // 3d2b - PAL/NTSC
  // this.io[0x2c] = 0x1418; // 3d2c - PRNG1
  // this.io[0x2d] = 0x1658; // 3d2d - PRNG2
  // this.io[0x2e] = 0x0007; // 3d2e - FIQ source
  // this.io[0x2f] = 0x003f; // 3d2f - DS
  // this.io[0x30] = 0x00ef; // 3d30 - UART Control ?maybe wrong since init captured using uart
  // this.io[0x31] = 0x0003; // 3d31 - UART Status ?same
  // this.io[0x32] = 0x0;
  // this.io[0x33] = 0x00ff; // 3d33 - UART Baud rate ?same
  // this.io[0x34] = 0x00ff; // 3d34 - UART Baud rate ?same
  // this.io[0x35] = 0x00ff; // 3d35 - UART TX

  // this.io[0x23] = 0x0028; // 3d23 - External Memory Ctrl
  // this.io[0x25] = 0x2000; // 3d25 - ADC Ctrl

  // this.watchdogTimer = Timer_Init(0, Bus_WatchdogWakeup, 0);
  //
  // for (int32_t i = 0; i < 4; i++) {
  //   this.adcTimers[i] = Timer_Init(0, Bus_DoADCConversion, i);
  // }
  //
  // this.adcValues[0] = 0x03ff;
  // this.adcValues[1] = 0x03ff;
  // this.adcValues[2] = 0x03ff;
  // this.adcValues[3] = 0x03ff;

  return true;
}


void Bus_Cleanup() {
  if (this.romBuffer)
    free(this.romBuffer);

  if (this.sysRomBuffer)
    free(this.sysRomBuffer);

  UART_Cleanup();
  Misc_Cleanup();
  Timers_Cleanup();
  GPIO_Cleanup();

//   for (int32_t i = 0; i < 4; i++) {
//     if (this.adcTimers[i])
//       Timer_Cleanup(this.adcTimers[i]);
//   }
//
//   if (this.watchdogTimer)
//     Timer_Cleanup(this.watchdogTimer);
}


void Bus_Reset() {
  // GPIO_Reset();
  Timers_Reset();
  Misc_Reset();
  UART_Reset();

  // this.rxHead = 0;
  // this.rxTail = 0;
  // this.rxEmpty = true;
}


void Bus_LoadROM(const char* filePath) {
  FILE* file = fopen(filePath, "rb");
  if (!file) {
    VSmile_Error("unable to load ROM - can't open \"%s\"", filePath);
    return;
  }

  // Get size of ROM
  fseek(file, 0, SEEK_END);
  this.romSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (this.romSize > BUS_SIZE*sizeof(uint16_t)) {
    VSmile_Error("file too large!");
    this.romSize = BUS_SIZE*sizeof(uint16_t);
  }
  if (this.romBuffer)
    free(this.romBuffer);

  this.romBuffer = malloc(BUS_SIZE*sizeof(uint16_t));
  int r = fread(this.romBuffer, this.romSize, sizeof(uint8_t), file);
  if (!r) VSmile_Error("error reading romfile");

  fclose(file);
}


void Bus_LoadSysRom(const char* filePath) {
  FILE* file = fopen(filePath, "rb");
  if (!file) {
    VSmile_Warning("unable to load system rom - can't open \"%s\"", filePath);
    return;
  }

  // Get size of ROM
  fseek(file, 0, SEEK_END);
  this.sysRomSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (this.sysRomBuffer)
    free(this.sysRomBuffer);

  this.sysRomBuffer = malloc(this.sysRomSize);
  int32_t success = fread(this.sysRomBuffer, this.sysRomSize, sizeof(uint16_t), file);

  if (!success)
    VSmile_Error("error reading system rom");

  fclose(file);
}


void Bus_Update(int32_t cycles) {
  Timers_Update(cycles);
  Misc_Update(cycles);
  UART_Update(cycles);
}

uint16_t Bus_Load(uint32_t addr) {
  if (addr < RAM_START+RAM_SIZE) {
    return this.ram[addr - RAM_START];
  }
  else if (addr < PPU_START+PPU_SIZE) {
    // VSmile_Log("Read from PPU address %04x at %06x", addr, CPU_GetCSPC());

    if (addr == 0x2838)
      return PPU_GetCurrLine();

    return this.ppu[addr - PPU_START];
  }
  else if (addr < SPU_START+SPU_SIZE) {
    return SPU_Read(addr);
  }
  else if (addr < 0x3d00) {
    // VSmile_Warning("read from internal memory location %04x", addr);
    return 0x0000;
  }
  else if (addr < IO_START+IO_SIZE+DMA_SIZE) {
    // if (addr != 0x3d21 && addr != 0x3d22)
    //   VSmile_Log("Read from IO address %04x at %06x", addr, CPU_GetCSPC());

    switch (addr) {
    case GPIO_START   ... (GPIO_START+GPIO_SIZE-1):
      return GPIO_Read(addr);

    case TIMERS_START ... (TIMERS_START+TIMERS_SIZE-1):
      return Timers_Read(addr);

    case MISC_START ... (MISC_START+MISC_SIZE-1):
      return Misc_Read(addr);

    case UART_START ... (UART_START+UART_SIZE-1):
      return UART_Read(addr);
    }
    
    printf("unknown read from IO port %04x at %06x\n", addr, CPU_GetCSPC());
    return this.io[addr - IO_START];
  }

  if ((this.romDecodeMode & 2) && this.sysRomBuffer && (addr >= SYSROM_START)) {
    return this.sysRomBuffer[addr - SYSROM_START];
  }

  if (this.romBuffer && (addr < BUS_SIZE)) {
    return this.romBuffer[addr];
  }

  return 0x0000;
}


void Bus_Store(uint32_t addr, uint16_t data) {
  if ((addr & 0xffff) < 0x4000) {
    addr &= 0x3fff;
  }

  if (addr < RAM_START+RAM_SIZE) {
    // if (addr == 0x2298)
    //   printf("write to [2298] with %04x at %06x\n", data, CPU_GetCSPC());
    this.ram[addr - RAM_START] = data;
    return;
  }
  else if (addr < PPU_START+PPU_SIZE) {
    // VSmile_Log("Write to PPU address %04x with %04x at %06x", addr, data, CPU_GetCSPC());

    switch (addr) {
    case 0x2810: // Page 1 X scroll
    case 0x2816: // Page 2 X scroll
      this.ppu[addr - PPU_START] = (data & 0x01ff);
      break;

    case 0x2811: // Page 1 Y scroll
    case 0x2817: // Page 2 Y scroll
      this.ppu[addr - PPU_START] = (data & 0x00ff);
      break;

    case 0x2814:
    case 0x281a:
      // printf("layer %d tilemap address set to %04x at %06x\n", addr == 0x281a, data, CPU_GetCSPC());
      this.ppu[addr - PPU_START] = data;
      break;

    case 0x2820:
    case 0x2821:
      // printf("layer %d tile data segment set to %04x at %06x\n", addr == 0x2821, data, CPU_GetCSPC());
      this.ppu[addr - PPU_START] = data;
      break;

    case 0x2863: // PPU IRQ acknowledge
      this.ppu[addr - PPU_START] &= ~data;
      break;

    case 0x2870: this.ppu[addr - PPU_START] = data & 0x3fff; break;
    case 0x2871: this.ppu[addr - PPU_START] = data & 0x03ff; break;
    case 0x2872: { // Do PPU DMA
      data &= 0x03ff;
      if (data == 0)
        data = 0x0400;

      uint32_t src = this.ppu[0x2870 - PPU_START];
      uint32_t dst = this.ppu[0x2871 - PPU_START] + 0x2c00;
      for (uint32_t i = 0; i < data; i++)
        Bus_Store(dst+i, Bus_Load(src+i));
      this.ppu[0x2872 - PPU_START] = 0;
      Misc_SetIRQFlags(0x2863, 0x0004);
    } break;

    default:
      this.ppu[addr - PPU_START] = data;
    }
    // printf("write to PPU address %04x with %04x\n", addr, data);
    return;
  }
  else if (addr < SPU_START+SPU_SIZE) {
    SPU_Write(addr, data);
    return;
  }
  else if (addr < 0x3d00) {
    VSmile_Warning("write to internal memory location %04x with %04x", addr, data);
    return;
  }
  else if (addr < IO_START+IO_SIZE+DMA_SIZE) {
    // if (addr != 0x3d21 && addr != 0x3d22 && addr != 0x3d24)
    //   VSmile_Log("Write to IO address %04x with %04x at %06x", addr, data, CPU_GetCSPC());

    switch (addr) {
    case GPIO_START   ... (GPIO_START+GPIO_SIZE-1):
      GPIO_Write(addr, data);
      break;

    case TIMERS_START ... (TIMERS_START+TIMERS_SIZE-1):
      Timers_Write(addr, data);
      break;

    case MISC_START ... (MISC_START+MISC_SIZE-1):
      Misc_Write(addr, data);
      break;

    case UART_START ... (UART_START+UART_SIZE-1):
      UART_Write(addr, data);
      break;

    case 0x3e00:
    case 0x3e01:
    case 0x3e03: this.io[addr - IO_START] = data; break;

    case 0x3e02: { // Do DMA
      uint32_t src = ((this.io[0x3e01 - IO_START] & 0x3f) << 16) | this.io[0x3e00 - IO_START];
      uint32_t dst = this.io[0x3e03 - IO_START] & 0x3fff;
      for(uint32_t i = 0; i < data; i++) {
          uint16_t transfer = Bus_Load(src+i);
          Bus_Store(dst+i, transfer);
      }
      this.io[0x3e02 - IO_START] = 0;
      } break;

    default:
      // printf("write to unknown IO port %04x with %04x at %06x\n", addr, data, CPU_GetCSPC());
      this.io[addr - IO_START] = data;
    }
    return;
  }
  else if (IN_RANGE(addr, 0x3e04, 0x4000 - 0x3e04)) {
    VSmile_Warning("write to internal memory location %04x with %04x", addr, data);
    return;
  }

  VSmile_Error("attempt to write to out of bounds location %06x with %04x", addr, data);
}


void Bus_SetIRQFlags(uint32_t address, uint16_t data) {
  switch (address) {
  case 0x2863:
    this.ppu[0x63] |= data;
    break;
  case 0x3d22:
    this.io[0x22] |= data;
    break;
  default:
    VSmile_Warning("unknown IRQ Acknowledge for %04x with data %04x", address, data);
  }

  CPU_ActivatePendingIRQs(); // Notify CPU that there might be IRQ's to handle
}


uint16_t Bus_GetChipSelectMode() {
  return this.chipSelectMode;
}


void Bus_SetChipSelectMode(uint16_t mode) {
  this.chipSelectMode = mode;
}


uint16_t Bus_GetRamDecode() {
  return this.ramDecodeMode;
}


void Bus_SetRamDecode(uint16_t data) {
  this.ramDecodeMode = data;
}


uint16_t Bus_GetRomDecode() {
  return this.romDecodeMode;
}


void Bus_SetRomDecode(uint16_t data) {
  this.romDecodeMode = data;
}
