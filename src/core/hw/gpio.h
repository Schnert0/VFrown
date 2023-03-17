#ifndef GPIO_H
#define GPIO_H

#include "../../common.h"
#include "../vsmile.h"

#define GPIO_START 0x3d00
#define GPIO_SIZE  0x0010

enum {
  GPIO_PORTA,
  GPIO_PORTB,
  GPIO_PORTC,

  NUM_GPIOPORTS
};


typedef struct {
  uint16_t data;
  uint16_t buffer;
  uint16_t direction;
  uint16_t attributes;
  uint16_t mask;
} GPIOPort_t;


typedef struct {
  uint8_t region;

  union {
    uint16_t regs[16];
    struct {
      uint16_t   config;
      GPIOPort_t ports[NUM_GPIOPORTS];
    };
  };
} GPIO_t;


bool GPIO_Init();
void GPIO_Cleanup();

void GPIO_SaveState();
void GPIO_LoadState();

void GPIO_Reset();

uint16_t GPIO_Read(uint16_t addr);
void     GPIO_Write(uint16_t addr, uint16_t data);

uint16_t GPIO_GetIOA(uint16_t mask);
void     GPIO_SetIOA(uint16_t data, uint16_t mask);
uint16_t GPIO_GetIOB(uint16_t mask);
void     GPIO_SetIOB(uint16_t data, uint16_t mask);
uint16_t GPIO_GetIOC(uint16_t mask);
void     GPIO_SetIOC(uint16_t data, uint16_t mask);

void GPIO_SetRegion(uint8_t region);
void GPIO_SetIntroEnable(bool shouldShowIntro);

#endif // GPIO_H
