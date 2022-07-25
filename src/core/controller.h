#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../common.h"
#include "vsmile.h"

typedef struct Controller_t {
  // Timers
  int32_t txTimer,    txTimerReset;
  int32_t rxTimer,    rxTimerReset;
  int32_t rtsTimer,   rtsTimerReset;
  int32_t idleTimer,  idleTimerReset;

  bool select, request, active;

  uint8_t txFifo[16];
  uint8_t txHead, txTail;
  uint8_t rxBuffer, txBuffer;
  bool    txEmpty, txActive;

  uint8_t  pastBytes[2];
  uint16_t stale;
} Controller_t;

bool Controller_Init();
void Controller_Cleanup();

void Controller_Tick(int32_t cycles);
uint16_t Controller_GetRequests();
void Controller_SetSelects(uint16_t data);
uint8_t Controller_SendByte();
void Controller_RecieveByte(uint8_t data);

void Controller_SetRequest(uint8_t ctrlNum, bool request);
bool Controller_GetRequest(uint8_t ctrlNum);
void Controller_SetSelect(uint8_t ctrlNum, bool select);
bool Controller_GetSelect(uint8_t ctrlNum);

// Tx Fifo information
void Controller_PrintTxFIFO(uint8_t ctrlNum);
bool Controller_PushTx(uint8_t ctrlNum, uint8_t data);
uint8_t Controller_PopTx(uint8_t ctrlNum);
bool Controller_QueueTx(uint8_t ctrlNum, uint8_t data);
void Controller_TxComplete(uint8_t ctrlNum);
void Controller_TxTimeout(uint8_t ctrlNum);

// UART Timers
void Controller_RxTimerReset(uint8_t ctrlNum);
void Controller_RxTimerAdjust(uint8_t ctrlNum, int32_t newReset);
void Controller_TxTimerReset(uint8_t ctrlNum);
void Controller_TxTimerAdjust(uint8_t ctrlNum, int32_t newReset);
void Controller_RTSTimerReset(uint8_t ctrlNum);
void Controller_RTSTimerAdjust(uint8_t ctrlNum, int32_t newReset);
void Controller_IdleTimerReset(uint8_t ctrlNum);
void Controller_IdleTimerAdjust(uint8_t ctrlNum, int32_t newReset);
void Controller_RxExpired(uint8_t ctrlNum);
void Controller_TxExpired(uint8_t ctrlNum);
void Controller_RTSExpired(uint8_t ctrlNum);
void Controller_IdleExpired(uint8_t ctrlNum);

void Controller_UpdateButtons(uint8_t ctrlNum, uint32_t buttons);

uint8_t Controller_GetTxValue(uint8_t index);


#endif // CONTROLLER_H
