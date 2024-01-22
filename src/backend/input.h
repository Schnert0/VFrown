#ifndef INPUT_H
#define INPUT_H

#include "../common.h"
#include "../core/vsmile.h"
#include "backend.h"
#include "libs.h"

enum {
  INPUT_DEVICETYPE_KEYBOARD,
  INPUT_DEVICETYPE_MOUSE,
  INPUT_DEVICETYPE_CONTROLLER,
  INPUT_DEVICETYPE_INVALID,
};

enum {
  INPUT_INPUTTYPE_BUTTON,
  INPUT_INPUTTYPE_AXIS,
  INPUT_INPUTTYPE_MOTION,
  INPUT_INPUTTYPE_INVALID,
};

#define INPUT_CHECKPAD_TIMER 60

typedef union {
  uint32_t raw;
  struct {
    uint32_t deviceType :  2;
    uint32_t deviceID   :  2;
    uint32_t inputType  :  2;
    uint32_t inputID    : 10;
    uint32_t value      : 16; // Also the threshold, deadzone, etc.
  };
} Mapping_t;

typedef struct {
  unsigned int gamepadID[2];
  Mapping_t controllerMappings[2][NUM_INPUTS];
  Mapping_t lastEvent;
  int8_t  buttons[2][NUM_INPUTS];
  uint32_t curr[2], prev[2];
  int8_t checkPadTimer;
} Input_t;


bool Input_Init();
void Input_Cleanup();

void Input_Update();
uint32_t Input_GetChangedButtons(uint8_t ctrlNum);

void Input_KeyboardMouseEvent(sapp_event* event);

#endif // INPUT_H
