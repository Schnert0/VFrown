#include "controller.h"

static Controller_t controllers[2];

// Initialize the Controller Component
bool Controller_Init() {
  for (int32_t i = 0; i < 2; i++) {
    Controller_t* this = &controllers[i];

    memset(this, 0, sizeof(Controller_t));
    this->txEmpty = true;
    this->rxTimerReset   = SYSCLOCK / 9600;
    this->txTimerReset   = SYSCLOCK / 9600;
    this->rtsTimerReset  = SYSCLOCK;
    this->idleTimerReset = SYSCLOCK;
  }

  Controller_IdleTimerReset(0);
  // Controller_IdleTimerReset(1);

  return true;
}


void Controller_Cleanup() {
  // for (int32_t i = 0; i < 2; i++) {
  //   Controller_t* this = &controllers[i];
  // }
}


void Controller_Tick(int32_t cycles) {
  for (int32_t i = 0; i < 2; i++) {
    Controller_t* this = &controllers[i];

    if (this->rxTimer > 0) {
      this->rxTimer -= cycles;
      if (this->rxTimer <= 0) {
        Controller_RxExpired(i);
      }
    }

    if (this->txTimer > 0) {
      this->txTimer -= cycles;
      if (this->txTimer <= 0) {
        Controller_TxExpired(i);
      }
    }

    if (this->rtsTimer > 0) {
      this->rtsTimer -= cycles;
      if (this->rtsTimer <= 0) {
        Controller_RTSExpired(i);
      }
    }

    if (this->idleTimer > 0) {
      this->idleTimer -= cycles;
      if (this->idleTimer <= 0) {
        Controller_IdleExpired(i);
      }
    }

  }
}


uint8_t Controller_SendByte() {
  // Send Controller 0 data
  if (controllers[0].select && !controllers[1].select) {
    return controllers[0].txBuffer;
  }

  // Send Controller 1 data
  if (!controllers[0].select && controllers[1].select) {
    return controllers[1].txBuffer;
  }

  // If both selects just happen to be enabled, I assume it would send
  // garbage that is the bitwise OR of both Tx buffers, but this is just a guess
  if (controllers[0].select && controllers[1].select) {
    return controllers[0].txBuffer | controllers[1].txBuffer;
  }


  // Send neither controller's data (probably 0x00)
  return 0x00;
}


void Controller_RecieveByte(uint8_t data) {
  if (controllers[0].select) {
    controllers[0].rxBuffer = data;
     Controller_RxTimerReset(0);
  }

  if (controllers[1].select) {
    controllers[1].rxBuffer = data;
    Controller_RxTimerReset(1);
  }
}


bool Controller_PushTx(uint8_t ctrlNum, uint8_t data) {
  Controller_t* this = &controllers[ctrlNum];

  if (!this->txEmpty && (this->txHead == this->txTail)) {
    VSmile_Warning("Tx byte %02x discarded because FIFO is full", data);
    return false;
  }

  this->txFifo[this->txHead] = data;
  this->txHead = (this->txHead + 1) & 0xf;
  this->txEmpty = false;

  return true;
}


uint8_t Controller_PopTx(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->txEmpty) {
    VSmile_Warning("returning 0x00 because Tx FIFO is empty");
    return 0x00;
  }

  uint8_t data = this->txFifo[this->txTail];
  this->txTail = (this->txTail + 1) & 0xf;
  this->txEmpty = (this->txHead == this->txTail);

  return data;
  return 0;
}


uint16_t Controller_GetRequests() {
  bool rts[2];

  rts[0] = !Controller_GetRequest(0);
  rts[1] = !Controller_GetRequest(1);

  return (rts[0] << 10) | (rts[1] << 12) | ((rts[0] && rts[1]) << 13);
}


void Controller_SetSelects(uint16_t data) {
  Controller_SetSelect(0, (data & 0x100) != 0);
  Controller_SetSelect(1, (data & 0x200) != 0);
}


void Controller_SetRequest(uint8_t ctrlNum, bool request) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->request == request)
    return;

  Bus_SetIRQFlags(0x3d22, ctrlNum == 0 ? 0x0200 : 0x1000);
  this->request = request;
}


bool Controller_GetRequest(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  return this->request;
}


void Controller_SetSelect(uint8_t ctrlNum, bool select) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->select == select)
    return;

  if (select && !this->txEmpty &&  !this->txActive) {
    // printf("select asserted, starting transmission...\n");
    Controller_RTSTimerReset(ctrlNum);
    this->txActive = true;
    Controller_TxTimerAdjust(ctrlNum, SYSCLOCK / 9600);
  }

  this->select = select;
}


bool Controller_GetSelect(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  return this->select;
}


bool Controller_QueueTx(uint8_t ctrlNum, uint8_t data) {
  Controller_t* this = &controllers[ctrlNum];

  bool wasEmpty = this->txEmpty;
  Controller_PushTx(ctrlNum, data);

  if (wasEmpty) {
    Controller_SetRequest(ctrlNum, true);
    if (this->select) {
      this->txActive = true;
      Controller_TxTimerAdjust(ctrlNum, SYSCLOCK / 9600);
    } else {
      Controller_RTSTimerAdjust(ctrlNum, SYSCLOCK / 2);
    }
  }

  return true;
}


void Controller_TxComplete(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->stale & INPUT_DIRECTIONS) {
    bool up = (this->stale & (1 << INPUT_UP)) != 0;
    bool down = (this->stale & (1 << INPUT_DOWN)) != 0;
    if (up && !down) {
      Controller_QueueTx(ctrlNum, 0x87);
    } else if (!up && down) {
      Controller_QueueTx(ctrlNum, 0x8f);
    } else {
      Controller_QueueTx(ctrlNum, 0x80);
    }

    bool left = (this->stale & (1 << INPUT_LEFT)) != 0;
    bool right = (this->stale & (1 << INPUT_RIGHT)) != 0;
    if (left && !right) {
      Controller_QueueTx(ctrlNum, 0xcf);
    } else if (!left && right) {
      Controller_QueueTx(ctrlNum, 0xc7);
    } else {
      Controller_QueueTx(ctrlNum, 0xc0);
    }
  }

  if (this->stale & INPUT_COLORS) {
    uint8_t colors = 0x90;
    if (this->stale & (1 << INPUT_RED)) colors |= 0x08;
    if (this->stale & (1 << INPUT_YELLOW)) colors |= 0x04;
    if (this->stale & (1 << INPUT_GREEN)) colors |= 0x02;
    if (this->stale & (1 << INPUT_BLUE)) colors |= 0x01;
    Controller_QueueTx(ctrlNum, colors);
  }


  if (this->stale & INPUT_BUTTONS) {
    if (this->stale & (1 << INPUT_ENTER))   Controller_QueueTx(ctrlNum, 0xa1);
    if (this->stale & (1 << INPUT_HELP))    Controller_QueueTx(ctrlNum, 0xa2);
    if (this->stale & (1 << INPUT_EXIT))    Controller_QueueTx(ctrlNum, 0xa3);
    if (this->stale & (1 << INPUT_ABC))     Controller_QueueTx(ctrlNum, 0xa4);
    if ((this->stale & INPUT_BUTTONS) == 0) Controller_QueueTx(ctrlNum, 0xa0);
  }

  this->active = true;
  this->stale = 0;
  Controller_IdleTimerReset(ctrlNum);
}


void Controller_RxComplete(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // if (this->select) {
    uint8_t data = this->rxBuffer;
    if (((data & 0xf0) == 0x70) || ((data & 0xf0) == 0xb0)) {
      this->pastBytes[0] = ((data & 0xf0) == 0x70) ? 0x00 : this->pastBytes[1];
      this->pastBytes[1] = data;
      uint8_t response = ((this->pastBytes[0] + this->pastBytes[1] + 0x0f) & 0x0f) ^ 0xb5;
      Controller_QueueTx(ctrlNum, response);
      Controller_IdleTimerReset(ctrlNum);
      Controller_TxTimerReset(ctrlNum);
    }
  // }
}

void Controller_TxTimeout(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->active) {
    Controller_IdleTimerReset(ctrlNum);
    this->active = false;
    this->pastBytes[0] = 0x00;
    this->pastBytes[1] = 0x00;
  }
  Controller_QueueTx(ctrlNum, 0x55);
  Controller_TxTimerReset(ctrlNum);
}


// TODO: create a generic timer struct with its own adjust, reset,
// and tick functions with callbacks to the expired functions

void Controller_TxTimerReset(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("tx timer reset\n");
  this->txTimer = this->txTimerReset;
}

void Controller_TxTimerAdjust(uint8_t ctrlNum, int32_t newReset) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("tx timer adjusted\n");
  this->txTimerReset = newReset;
  this->txTimer = this->txTimerReset;
}

void Controller_RxTimerReset(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("rx timer reset\n");
  this->rxTimer = this->rxTimerReset;
}

void Controller_RxTimerAdjust(uint8_t ctrlNum, int32_t newReset) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("rx timer adjusted\n");
  this->rxTimerReset = newReset;
  this->rxTimer = this->rxTimerReset;
}

void Controller_RTSTimerReset(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("rts timer reset\n");
  this->rtsTimer = this->rtsTimerReset;
}

void Controller_RTSTimerAdjust(uint8_t ctrlNum, int32_t newReset) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("rts timer adjusted\n");
  this->rtsTimerReset = newReset;
  this->rtsTimer = this->rtsTimerReset;
}

void Controller_IdleTimerReset(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("idle timer reset\n");
  this->idleTimer = this->idleTimerReset;
}

void Controller_IdleTimerAdjust(uint8_t ctrlNum, int32_t newReset) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("idle timer adjusted\n");
  this->idleTimerReset = newReset;
  this->rtsTimer = this->rtsTimerReset;
}

void Controller_TxExpired(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("tx expired\n");

  // if (!this->txActive)
  //   VSmile_Warning("controller tx is not active!");

  // if (this->txEmpty)
  //   VSmile_Warning("controller tx FIFO is empty");

  // Transmit one byte to the Console's UART
  uint8_t data = Controller_PopTx(ctrlNum);
  Bus_PushRx(data);
  Bus_RxTimerReset();

  if (this->txEmpty)
    Controller_TxComplete(ctrlNum);

  if (this->txEmpty) {
    this->txActive = false;
    Controller_SetRequest(ctrlNum, false);
  } else if (this->select) {
    Controller_TxTimerAdjust(ctrlNum, SYSCLOCK / 960);
  } else {
    this->txActive = false;
  }


}

void Controller_RxExpired(uint8_t ctrlNum) {
  // printf("controller %d rx expired\n", ctrlNum);
  Controller_RxComplete(ctrlNum);
}

void Controller_RTSExpired(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  // printf("rts expired\n");

  if (!this->txEmpty) {
    this->txHead = 0;
    this->txTail = 0;
    this->txEmpty = true;
    Controller_TxTimeout(ctrlNum);
    if (this->txEmpty) {
      Controller_SetRequest(ctrlNum, false);
    }
  }
}


void Controller_IdleExpired(uint8_t ctrlNum) {
  // printf("idle expired\n");
  Controller_QueueTx(ctrlNum, 0x55);
  Controller_IdleTimerReset(ctrlNum);
  // Controller_RTSTimerReset(ctrlNum);
}


void Controller_UpdateButtons(uint8_t ctrlNum, uint32_t buttons) {
  Controller_t* this = &controllers[ctrlNum];

  if (!this->active) {
    Controller_SetRequest(ctrlNum, true);
    return;
  }

  if (!this->txEmpty) {
    this->stale |= buttons;
    return;
  }

  uint32_t changed = Backend_GetChangedButtons();

  if (changed & INPUT_DIRECTIONS) {
    bool up = (buttons & (1 << INPUT_UP)) != 0;
    bool down = (buttons & (1 << INPUT_DOWN)) != 0;
    if (up && !down) {
      Controller_QueueTx(ctrlNum, 0x87);
    } else if (!up && down) {
      Controller_QueueTx(ctrlNum, 0x8f);
    } else {
      Controller_QueueTx(ctrlNum, 0x80);
    }

    bool left = (buttons & (1 << INPUT_LEFT)) != 0;
    bool right = (buttons & (1 << INPUT_RIGHT)) != 0;
    if (left && !right) {
      Controller_QueueTx(ctrlNum, 0xcf);
    } else if (!left && right) {
      Controller_QueueTx(ctrlNum, 0xc7);
    } else {
      Controller_QueueTx(ctrlNum, 0xc0);
    }
  }

  if (changed & INPUT_COLORS) {
    uint8_t colors = 0x90;
    if (buttons & (1 << INPUT_RED)) colors |= 0x08;
    if (buttons & (1 << INPUT_YELLOW)) colors |= 0x04;
    if (buttons & (1 << INPUT_GREEN)) colors |= 0x02;
    if (buttons & (1 << INPUT_BLUE)) colors |= 0x01;
    Controller_QueueTx(ctrlNum, colors);
  }


  if (changed & INPUT_BUTTONS) {
    if (buttons & (1 << INPUT_ENTER))   Controller_QueueTx(ctrlNum, 0xa1);
    if (buttons & (1 << INPUT_HELP))    Controller_QueueTx(ctrlNum, 0xa2);
    if (buttons & (1 << INPUT_EXIT))    Controller_QueueTx(ctrlNum, 0xa3);
    if (buttons & (1 << INPUT_ABC))     Controller_QueueTx(ctrlNum, 0xa4);
    if ((buttons & INPUT_BUTTONS) == 0) Controller_QueueTx(ctrlNum, 0xa0);
  }

  Controller_IdleTimerReset(ctrlNum);
  Controller_TxTimerReset(ctrlNum);
}


uint8_t Controller_GetTxValue(uint8_t index) {
  return controllers[0].txFifo[index];
}
