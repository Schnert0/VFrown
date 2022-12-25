#include "gpio.h"

static GPIO_t this;

bool GPIO_Init() {
  memset(&this, 0, sizeof(GPIO_t));

  return true;
}


void GPIO_Cleanup() {

}


void GPIO_Reset() {
  this.config = 0x001f; // 0x3d00

  this.ports[GPIO_PORTA].data       = 0xffff; // 0x3d01
  this.ports[GPIO_PORTA].buffer     = 0xffff; // 0x3d02
  this.ports[GPIO_PORTA].direction  = 0xffff; // 0x3d03
  this.ports[GPIO_PORTA].attributes = 0xffff; // 0x3d04
  this.ports[GPIO_PORTA].mask       = 0xffff; // 0x3d05

  this.ports[GPIO_PORTB].data       = 0x00ff; // 0x3d06
  this.ports[GPIO_PORTB].buffer     = 0x00ff; // 0x3d07
  this.ports[GPIO_PORTB].direction  = 0x00ff; // 0x3d08
  this.ports[GPIO_PORTB].attributes = 0x00ff; // 0x3d09
  this.ports[GPIO_PORTB].mask       = 0x00ff; // 0x3d0a

  this.ports[GPIO_PORTC].data       = 0xffff; // 0x3d0b
  this.ports[GPIO_PORTC].buffer     = 0xffff; // 0x3d0c
  this.ports[GPIO_PORTC].direction  = 0xffff; // 0x3d0d
  this.ports[GPIO_PORTC].attributes = 0xffff; // 0x3d0e
  this.ports[GPIO_PORTC].mask       = 0xffff; // 0x3d0f
}


uint16_t GPIO_Read(uint16_t addr) {
  if (addr == 0x3d00) {
    return this.config;
  }

  if (addr == 0x3d01 || addr == 0x3d06 || addr == 0x3d0b) {
    uint8_t port = (addr - 0x3d01) / 5;

    uint16_t buffer  = this.regs[2+port*5];
    uint16_t dir     = this.regs[3+port*5];
    uint16_t attr    = this.regs[4+port*5];
    uint16_t special = this.regs[5+port*5];

    uint16_t push = dir;
    uint16_t pull = (~dir) & (~attr);
    uint16_t what = (buffer & (push | pull));
    what ^= (dir & ~attr);
    what &= ~special;

    what = (what & ~pull);

    switch (port) {
    case GPIO_PORTA: what |= GPIO_GetIOA(push & ~special) & pull; break;
    case GPIO_PORTB: what |= GPIO_GetIOB(push & ~special) & pull; break;
    case GPIO_PORTC: what |= GPIO_GetIOC(push & ~special) & pull; break;
    }

    this.regs[1+port*5] = what;
  }

  return this.regs[addr - GPIO_START];
}


void GPIO_Write(uint16_t addr, uint16_t data) {
  if (addr == 0x3d00) {
    this.config = data;
    return;
  }

  if (addr == 0x3d01 || addr == 0x3d06 || addr == 0x3d0b) {
    addr++;
  }

  this.regs[addr - GPIO_START] = data;

  uint8_t port = (addr - 0x3d01) / 5;

  uint16_t buffer  = this.regs[2+port*5];
  uint16_t dir     = this.regs[3+port*5];
  uint16_t attr    = this.regs[4+port*5];
  uint16_t special = this.regs[5+port*5];

  uint16_t push = dir;
  uint16_t pull = (~dir) & (~attr);
  uint16_t what = (buffer & (push | pull));
  what ^= (dir & ~attr);
  what &= ~special;

  switch (port) {
  case GPIO_PORTA: GPIO_SetIOA(what, push & ~special); break;
  case GPIO_PORTB: GPIO_SetIOB(what, push & ~special); break;
  case GPIO_PORTC: GPIO_SetIOC(what, push & ~special); break;
  }

  what = (what & ~pull);

  this.regs[1+port*5] = what;
}


// Get data from IO port A (stubbed)
uint16_t GPIO_GetIOA(uint16_t mask) {
  return 0x0000;
}


// Send data to IO port A (stubbed)
void GPIO_SetIOA(uint16_t data, uint16_t mask) {

}


// Get data from IO port B
uint16_t GPIO_GetIOB(uint16_t mask) {
  return 0x00c8 | Bus_GetChipSelectMode();
}


// Send data to IO port B
void GPIO_SetIOB(uint16_t data, uint16_t mask) {
  // printf("write to IOB (data: %04x, mask: %04x)\n", data, mask);
  if (mask & 7) {
    Bus_SetChipSelectMode((data & 7));
    // printf("chip select set to %d\n", (data & 7));
  }
}


// Get data from IO port C
uint16_t GPIO_GetIOC(uint16_t mask) {
  uint16_t data = this.region;
  data |= Controller_GetRequests();

  // printf("read from IOC (%04x) at %06x\n", data, CPU_GetCSPC());

  return data;
}


// Send data to IO port C
void GPIO_SetIOC(uint16_t data, uint16_t mask) {
  // printf("write to IOC with %04x at %06x\n", data, CPU_GetCSPC());

  if ((mask >> 8) & 1) {
    Controller_SetSelect(0, (data >> 8) & 1);
  }

  if ((mask >> 9) & 1) {
    Controller_SetSelect(1, (data >> 9) & 1);
  }
}


void GPIO_SetRegion(uint8_t region) {
  this.region &= ~0xf;
  this.region = region & 0xf;
}


void GPIO_SetIntroEnable(bool shouldShowIntro) {
  this.region &= 0xffcf;
  this.region |= shouldShowIntro ? 0x30 : 0x20;
}
