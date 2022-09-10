#include "controller.h"

static Controller_t controllers[2];

// Initialize the Controller Component
bool Controller_Init() {
  for (int32_t i = 0; i < 2; i++) {
    Controller_t* this = &controllers[i];

    memset(this, 0, sizeof(Controller_t));
    this->txEmpty = true;
    this->txTimer   = Timer_Init(SYSCLOCK / 9600, (TimerFunc_t)Controller_TxExpired,   i);
    this->rxTimer   = Timer_Init(SYSCLOCK / 9600, (TimerFunc_t)Controller_RxExpired,   i);
    this->rtsTimer  = Timer_Init(SYSCLOCK,        (TimerFunc_t)Controller_RTSExpired,  i);
    this->idleTimer = Timer_Init(SYSCLOCK,        (TimerFunc_t)Controller_IdleExpired, i);
    // Timer_Reset(this->idleTimer);
  }

  Timer_Reset(controllers[0].idleTimer);

  return true;
}


void Controller_Cleanup() {
  for (int32_t i = 0; i < 2; i++) {
    Controller_t* this = &controllers[i];

    Timer_Cleanup(this->rxTimer);
    Timer_Cleanup(this->txTimer);
    Timer_Cleanup(this->rtsTimer);
    Timer_Cleanup(this->idleTimer);
  }
}


void Controller_Tick(int32_t cycles) {
  for (int32_t i = 0; i < 2; i++) {
    Controller_t* this = &controllers[i];

    Timer_Tick(this->txTimer, cycles);
    Timer_Tick(this->rxTimer, cycles);
    Timer_Tick(this->rtsTimer, cycles);
    Timer_Tick(this->idleTimer, cycles);
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
    Timer_Reset(controllers[0].rxTimer);
  }

  if (controllers[1].select) {
    controllers[1].rxBuffer = data;
    Timer_Reset(controllers[1].rxTimer);
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
    Timer_Reset(this->rtsTimer);
    this->txActive = true;
    Timer_Adjust(this->txTimer, SYSCLOCK / 9600);
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
      Timer_Adjust(this->txTimer, SYSCLOCK / 9600);
    } else {
      Timer_Adjust(this->rtsTimer, SYSCLOCK / 2);
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
  Timer_Reset(this->idleTimer);
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
      Timer_Reset(this->idleTimer);
      Timer_Reset(this->txTimer);
    }

    if ((data & 0xf0) == 0x60) {
      this->ledStatus = data & 0xf;
      Backend_SetLedStates(data & 0xf);
    }
  // }
}

void Controller_TxTimeout(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

  if (this->active) {
    Timer_Reset(this->idleTimer);
    this->active = false;
    this->pastBytes[0] = 0x00;
    this->pastBytes[1] = 0x00;
  }
  Controller_QueueTx(ctrlNum, 0x55);
  Timer_Reset(this->txTimer);
}


void Controller_TxExpired(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

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
    Timer_Adjust(this->txTimer, SYSCLOCK / 960);
  } else {
    this->txActive = false;
  }
}


void Controller_RxExpired(uint8_t ctrlNum) {
  Controller_RxComplete(ctrlNum);
}


void Controller_RTSExpired(uint8_t ctrlNum) {
  Controller_t* this = &controllers[ctrlNum];

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
  Controller_QueueTx(ctrlNum, 0x55);
  Timer_Reset(controllers[ctrlNum].idleTimer);
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

  Timer_Reset(this->idleTimer);
  Timer_Reset(this->txTimer);
}


uint8_t Controller_GetTxValue(uint8_t index) {
  return controllers[0].txFifo[index];
}
