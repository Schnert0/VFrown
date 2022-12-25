#ifndef BUS_H
#define BUS_H

#include "../common.h"
#include "vsmile.h"

#include "hw/gpio.h"
#include "hw/timers.h"
#include "hw/misc.h"
#include "hw/uart.h"
#include "hw/dma.h"

#define BUS_START 0x000000
#define BUS_SIZE  0x400000

#define RAM_START 0x000000
#define RAM_SIZE  0x002800

#define PPU_START 0x002800
#define PPU_SIZE  0x000800

#define SPU_START 0x003000
#define SPU_SIZE  0x000800

#define IO_START  0x003d00
#define IO_SIZE   0x000200

#define SYSROM_SIZE  0x100000
#define SYSROM_START 0x300000


/*
 * Bus is the main memory bus for the vsmile and handles memory reads and writes
 * as well as I/O access. Bus always outputs 16-bit words
 */
typedef struct Bus_t {
  uint16_t mem[BUS_SIZE];
  uint16_t ram[RAM_SIZE];
  uint16_t nvram[0x200000];

  uint16_t* romBuffer;
  uint16_t* sysRomBuffer;

  uint32_t romSize;
  uint32_t sysRomSize;

  uint8_t romDecodeMode;
  uint8_t ramDecodeMode;
  uint8_t chipSelectMode;
} Bus_t;

bool Bus_Init();
void Bus_Cleanup();

void Bus_Reset();
void Bus_LoadROM(const char* filePath);
void Bus_LoadSysRom(const char* filePath);

void Bus_Update(int32_t cycles);

uint16_t Bus_Load(uint32_t addr);
void Bus_Store(uint32_t addr, uint16_t data);

uint16_t Bus_GetChipSelectMode();
void Bus_SetChipSelectMode(uint16_t data);
uint16_t Bus_GetRamDecode();
void Bus_SetRamDecode(uint16_t data);
uint16_t Bus_GetRomDecode();
void Bus_SetRomDecode(uint16_t data);

#endif // BUS_H
