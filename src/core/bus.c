#include "bus.h"
#include "cpu.h"
#include "hw/misc.h"
#include "vsmile.h"

static Bus_t this;

bool Bus_Init() {
  memset(&this, 0, sizeof(Bus_t));

  if (!GPIO_Init())   return false;
  if (!Timers_Init()) return false;
  if (!Misc_Init())   return false;
  if (!UART_Init())   return false;
  if (!DMA_Init())    return false;

  return true;
}


void Bus_Cleanup() {
  if (this.romBuffer)
    free(this.romBuffer);

  if (this.sysRomBuffer)
    free(this.sysRomBuffer);

  DMA_Cleanup();
  UART_Cleanup();
  Misc_Cleanup();
  Timers_Cleanup();
  GPIO_Cleanup();
}


void Bus_SaveState() {
  Backend_WriteSave(&this, sizeof(Bus_t));
}


void Bus_LoadState() {
  uint16_t* romBuffer = this.romBuffer;
  uint16_t* sysRomBuffer = this.sysRomBuffer;
  Backend_ReadSave(&this, sizeof(Bus_t));
  this.romBuffer = romBuffer;
  this.sysRomBuffer = sysRomBuffer;
}


void Bus_Reset() {
  GPIO_Reset();
  Timers_Reset();
  Misc_Reset();
  UART_Reset();
  DMA_Reset();
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
  fread(this.romBuffer, this.romSize, sizeof(uint8_t), file);

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
  fread(this.sysRomBuffer, this.sysRomSize, sizeof(uint16_t), file);

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
    return PPU_Read(addr);
  }
  else if (addr < SPU_START+SPU_SIZE) {
    return SPU_Read(addr);
  }
  else if (addr < 0x3d00) {
    VSmile_Warning("read from internal memory location %04x", addr);
    return 0x0000;
  }
  else if (addr < IO_START+IO_SIZE+DMA_SIZE) {
    // printf("read IO %04x (%04x)\n", addr, GPIO_Read(addr));
    switch (addr) {
    case GPIO_START ... (GPIO_START+GPIO_SIZE-1):
      return GPIO_Read(addr);

    case TIMERS_START ... (TIMERS_START+TIMERS_SIZE-1):
      return Timers_Read(addr);

    case MISC_START ... (MISC_START+MISC_SIZE-1):
      return Misc_Read(addr);

    case UART_START ... (UART_START+UART_SIZE-1):
      return UART_Read(addr);

    case DMA_START ... (DMA_START+DMA_SIZE-1):
      return DMA_Read(addr);
    }

    VSmile_Warning("unknown read from IO port %04x at %06x\n", addr, CPU_GetCSPC());
    return 0x0000;
  } else if (addr < 0x4000) {
    VSmile_Warning("read from internal memory location %04x", addr);
    return 0x0000;
  }

  if ((this.romDecodeMode & 2) && this.sysRomBuffer && (addr >= SYSROM_START) && (addr < BUS_SIZE)) {
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
    this.ram[addr - RAM_START] = data;
    return;
  }
  else if (addr < PPU_START+PPU_SIZE) {
    PPU_Write(addr, data);
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
    // printf("write IO %04x (%04x)\n", addr, data);
    switch (addr) {
    case GPIO_START ... (GPIO_START+GPIO_SIZE-1):
      GPIO_Write(addr, data);
      return;

    case TIMERS_START ... (TIMERS_START+TIMERS_SIZE-1):
      Timers_Write(addr, data);
      return;

    case MISC_START ... (MISC_START+MISC_SIZE-1):
      Misc_Write(addr, data);
      return;

    case UART_START ... (UART_START+UART_SIZE-1):
      UART_Write(addr, data);
      return;

    case DMA_START ... (DMA_START+DMA_SIZE-1):
      DMA_Write(addr, data);
      return;
    }

    VSmile_Warning("write to unknown IO port %04x with %04x at %06x", addr, data, CPU_GetCSPC());
    return;
  }
  else if (addr < 0x4000) {
    VSmile_Warning("write to internal memory location %04x with %04x", addr, data);
    return;
  }

  // VSmile_Warning("attempt to write to out of bounds location %06x with %04x", addr, data);
  // printf("%06x\n", CPU_GetCSPC());
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
