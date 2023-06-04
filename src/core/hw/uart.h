#ifndef UART_H
#define UART_H

#include "../../common.h"
#include "../vsmile.h"

#define UART_START 0x3d30
#define UART_SIZE  0x0010

typedef struct {
  int32_t  rxTimer, txTimer;
  uint8_t  rxBuffer, txBuffer;
  bool     rxAvailable;

  uint8_t rxFifo[16];
  uint8_t rxHead, rxTail;
  bool    rxEmpty;

  uint32_t baudRate;
  union {
    uint16_t baud;
    struct {
      uint8_t baudLo;
      uint8_t baudHi;
    };
  };

  uint16_t ctrl;
  uint16_t stat;
  uint16_t rx, tx;
} UART_t;

bool UART_Init();
void UART_Cleanup();


void UART_SaveState();
void UART_LoadState();

void UART_Reset();
void UART_Update(int32_t cycles);

uint16_t UART_Read(uint16_t addr);
void UART_Write(uint16_t addr, uint16_t data);

void UART_SetCtrl(uint16_t data);
void UART_SetStat(uint16_t data);
// void Bus_ResetUART(uint16_t data);
void UART_SetBaud();
uint16_t UART_GetRxBuffer();
void UART_SetTxBuffer(uint16_t data);

void UART_RxTimerReset();
void UART_TransmitTick();
void UART_RecieveTick();

bool UART_PushRx(uint8_t data);
uint8_t UART_PopRx();

#endif // UART_H
