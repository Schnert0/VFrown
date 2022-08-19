#ifndef BUS_H
#define BUS_H

#include "../common.h"
#include "vsmile.h"

#define BUS_START 0x000000
#define BUS_SIZE  0x400000

#define RAM_START 0x000000
#define RAM_SIZE  0x002800

#define PPU_START 0x002800
#define PPU_SIZE  0x000800

#define SPU_START 0x003000
#define SPU_SIZE  0x000800

#define IO_START  0x003d00
#define IO_SIZE   0x000100

#define DMA_START 0x003e00
#define DMA_SIZE  0x000004

#define BIOS_SIZE  0x200000
#define BIOS_START 0x200000


/*
 * Bus is the main memory bus for the vsmile and handles memory reads and writes
 * as well as I/O access. Bus always outputs 16-bit words
 */
typedef struct Bus_t {
  uint16_t mem[BUS_SIZE];

  uint16_t ram[RAM_SIZE];
  uint16_t ppu[PPU_SIZE];
  // uint16_t spu[SPU_SIZE];
  uint16_t io[IO_SIZE+DMA_SIZE];
  uint16_t nvram[0x200000];

  // UART
  uint32_t baudRate;
  int32_t  rxTimer, txTimer;
  uint8_t  rxBuffer, txBuffer;
  bool     rxAvailable;

  // UART Rx Fifo
  uint8_t rxFifo[16];
  uint8_t rxHead, rxTail;
  bool    rxEmpty;

  uint8_t romDecodeMode;
  uint8_t ramDecodeMode;
  uint8_t chipSelectMode;

  struct Timer_t* sysTimers;
  uint16_t timer2khz, timer1khz, timer4hz;

  struct Timer_t* adcTimers[4];
  uint16_t adcValue[4];

  uint32_t romSize;
  uint16_t* romBuffer;

  uint32_t biosSize;
  uint16_t* biosBuffer;

} Bus_t;

bool Bus_Init();
void Bus_Cleanup();

void Bus_Reset();
void Bus_LoadROM(const char* filePath);
void Bus_LoadBIOS(const char* filePath);

void Bus_Tick(int32_t cycles);
void Bus_TickTimers(int32_t index);

uint16_t Bus_Load(uint32_t addr);
void Bus_Store(uint32_t addr, uint16_t data);

void Bus_SetIRQFlags(uint32_t addr, uint16_t data);
void Bus_DoGPIO(uint16_t addr, bool write);
uint16_t Bus_GetIOB(uint16_t mask);
void Bus_SetIOB(uint16_t data, uint16_t mask);
uint16_t Bus_GetIOC(uint16_t mask);
void Bus_SetIOC(uint16_t data, uint16_t mask);

// ADC
void Bus_WriteADCCtrl(uint16_t data);
void Bus_DoADCConversion(int32_t index);

// Controller handlers
void Bus_SetUARTCtrl(uint16_t data);
void Bus_SetUARTStat(uint16_t data);
void Bus_ResetUART(uint16_t data);
void Bus_SetUARTBAUD(uint16_t data);
uint16_t Bus_GetRxBuffer();
void Bus_SetTxBuffer(uint16_t data);

void Bus_RxTimerReset();
void Bus_TransmitTick();
void Bus_RecieveTick();

bool Bus_PushRx(uint8_t data);
uint8_t Bus_PopRx();

#endif // BUS_H
