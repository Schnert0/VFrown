#ifndef DMA_H
#define DMA_H

#include "../../common.h"
#include "../vsmile.h"

#define DMA_START 0x3e00
#define DMA_SIZE  0x0010

typedef struct {
  union {
    uint32_t src;
    struct {
      uint16_t srcLo;
      uint16_t srcHi;
    };
  };
  uint16_t dst;
  uint16_t size;
} DMA_t;

bool DMA_Init();
void DMA_Cleanup();

void DMA_Reset();

uint16_t DMA_Read(uint16_t addr);
void DMA_Write(uint16_t addr, uint16_t data);

#endif // DMA_H
