#include "dma.h"

static DMA_t this;

bool DMA_Init() {
  memset(&this, 0, sizeof(DMA_t));

  return true;
}


void DMA_Cleanup() {

}


void DMA_Reset() {
  this.src  = 0x000000;
  this.dst  =   0x0000;
  this.size =   0x0000;
}


uint16_t DMA_Read(uint16_t addr) {
  switch (addr) {
  case 0x3e00: return this.srcLo;
  case 0x3e01: return this.srcHi;
  case 0x3e02: return this.size;
  case 0x3e03: return this.dst;
  default:
    VSmile_Warning("unknown read from DMA address %04x at %06x", addr, CPU_GetCSPC());
  }

  return 0x0000;
}


void DMA_Write(uint16_t addr, uint16_t data) {
  switch (addr) {
  case 0x3e00: this.srcLo = data;          return;
  case 0x3e01: this.srcHi = (data & 0x3f); return;
  case 0x3e03: this.dst   = data;          return;

  case 0x3e02: {
    uint16_t size = (data & 0x3fff);
    for (uint32_t i = 0; i < size; i++) {
      uint16_t transfer = Bus_Load(this.src+i);
      Bus_Store(this.dst+i, transfer);
    }
    this.size = 0x0000;
  } return;

  default:
    VSmile_Warning("unknown read from DMA address %04x at %06x", addr, CPU_GetCSPC());
  }
}
