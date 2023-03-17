#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../../common.h"
#include "../vsmile.h"
#include "../timer.h"

typedef void (*ControllerFunc_t)();
struct Timer_t;

/*
 * Emulates the VSmile Controller Interface
 * TODO: this needs to be separated out to allow for emulation of
 * the keyboard and mat controllers
 */
typedef struct Controller_t {
  // Timers
  struct Timer_t* txTimer;
  struct Timer_t* rxTimer;
  struct Timer_t* rtsTimer;
  struct Timer_t* idleTimer;

  bool select, request, active;

  uint8_t txFifo[16];
  uint8_t txHead, txTail;
  uint8_t rxBuffer, txBuffer;
  bool    txEmpty, txActive;

  uint8_t  pastBytes[2];
  uint16_t stale;
  uint8_t  ledStatus;
} Controller_t;

bool Controller_Init();
void Controller_Cleanup();

void Controller_SaveState();
void Controller_LoadState();

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
bool Controller_PushTx(uint8_t ctrlNum, uint8_t data);
uint8_t Controller_PopTx(uint8_t ctrlNum);
bool Controller_QueueTx(uint8_t ctrlNum, uint8_t data);
void Controller_TxComplete(uint8_t ctrlNum);
void Controller_TxTimeout(uint8_t ctrlNum);

// UART Timers
void Controller_TxExpired(uint8_t ctrlNum);
void Controller_RxExpired(uint8_t ctrlNum);
void Controller_RTSExpired(uint8_t ctrlNum);
void Controller_IdleExpired(uint8_t ctrlNum);

void Controller_UpdateButtons(uint8_t ctrlNum, uint32_t buttons);

uint8_t Controller_GetTxValue(uint8_t index);


#endif // CONTROLLER_H
