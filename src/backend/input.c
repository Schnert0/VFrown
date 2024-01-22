#include "input.h"
#include "lib/gamepad/Gamepad.h"

#define KEYDEF(keycode) \
  (Mapping_t){\
    .deviceType=INPUT_DEVICETYPE_KEYBOARD,\
    .deviceID=0,\
    .inputType=INPUT_INPUTTYPE_BUTTON,\
    .inputID=keycode,\
    .value=0x7fff,\
  }

static Input_t this;

static void _Input_CheckInput(
  uint8_t deviceType, uint8_t deviceID, uint8_t inputType, uint16_t inputID, int16_t value
);
static int8_t _Input_GetDeviceID(struct Gamepad_device* device);
static void _Input_GamepadButtonDown(
  struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context
);
static void _Input_GamepadButtonUp(
  struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context
);
static void _Input_GamepadAxisChange(
  struct Gamepad_device * device,
  unsigned int axisID, float value, float lastValue, double timestamp, void * context
);
static void _Input_GamepadAttached(struct Gamepad_device * device, void * context);
static void _Input_GamepadDetached(struct Gamepad_device * device, void * context);



bool Input_Init() {
  memset(&this, 0, sizeof(Input_t));

  Gamepad_init();
  Gamepad_buttonDownFunc(_Input_GamepadButtonDown, NULL);
  Gamepad_buttonUpFunc(_Input_GamepadButtonUp, NULL);
  Gamepad_axisMoveFunc(_Input_GamepadAxisChange, NULL);
  Gamepad_deviceAttachFunc(_Input_GamepadAttached, NULL);
  Gamepad_deviceRemoveFunc(_Input_GamepadDetached, NULL);

  this.gamepadID[0] = -1;
  this.gamepadID[1] = -1;

  this.controllerMappings[0][INPUT_UP]    = KEYDEF(SAPP_KEYCODE_UP);
  this.controllerMappings[0][INPUT_DOWN]  = KEYDEF(SAPP_KEYCODE_DOWN);
  this.controllerMappings[0][INPUT_LEFT]  = KEYDEF(SAPP_KEYCODE_LEFT);
  this.controllerMappings[0][INPUT_RIGHT] = KEYDEF(SAPP_KEYCODE_RIGHT);

  this.controllerMappings[0][INPUT_RED]    = KEYDEF(SAPP_KEYCODE_Z);
  this.controllerMappings[0][INPUT_YELLOW] = KEYDEF(SAPP_KEYCODE_X);
  this.controllerMappings[0][INPUT_BLUE]   = KEYDEF(SAPP_KEYCODE_C);
  this.controllerMappings[0][INPUT_GREEN]  = KEYDEF(SAPP_KEYCODE_V);

  this.controllerMappings[0][INPUT_ENTER] = KEYDEF(SAPP_KEYCODE_SPACE);
  this.controllerMappings[0][INPUT_HELP]  = KEYDEF(SAPP_KEYCODE_A);
  this.controllerMappings[0][INPUT_EXIT]  = KEYDEF(SAPP_KEYCODE_S);
  this.controllerMappings[0][INPUT_ABC]   = KEYDEF(SAPP_KEYCODE_D);

  return true;
}


void Input_Cleanup() {
  Gamepad_shutdown();
}


void Input_Update() {
  Gamepad_processEvents();

  this.checkPadTimer--;
  if (this.checkPadTimer < 0) {
    this.checkPadTimer = INPUT_CHECKPAD_TIMER;
    Gamepad_detectDevices();
  }

  if (Input_GetChangedButtons(0)) {
    Controller_UpdateButtons(0, this.curr[0]);
  }
  if (Input_GetChangedButtons(1)) {
    Controller_UpdateButtons(1, this.curr[1]);
  }

  this.prev[0] = this.curr[0];
  this.prev[1] = this.curr[1];
}


uint32_t Input_GetChangedButtons(uint8_t ctrlNum) {
  return this.curr[ctrlNum] ^ this.prev[ctrlNum];
}


void Input_KeyboardMouseEvent(sapp_event* event) {
  switch (event->type) {

  case SAPP_EVENTTYPE_KEY_DOWN:
    _Input_CheckInput(INPUT_DEVICETYPE_KEYBOARD, 0, INPUT_INPUTTYPE_BUTTON, event->key_code, 0x7fff);
    break;

  case SAPP_EVENTTYPE_KEY_UP:
    _Input_CheckInput(INPUT_DEVICETYPE_KEYBOARD, 0, INPUT_INPUTTYPE_BUTTON, event->key_code, 0x0000);
    break;

  case SAPP_EVENTTYPE_MOUSE_MOVE:
    if (event->mouse_dx != 0)
      _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_MOTION, 0, event->mouse_dx);
    if (event->mouse_dy != 0)
      _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_MOTION, 1, event->mouse_dy);
    break;

  case SAPP_EVENTTYPE_MOUSE_SCROLL:
    if (event->scroll_x != 0)
      _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_MOTION, 2, event->scroll_x);
    if (event->scroll_y != 0)
      _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_MOTION, 3, event->scroll_y);
    break;

  case SAPP_EVENTTYPE_MOUSE_DOWN:
    _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_BUTTON, event->mouse_button, 0x7fff);
    break;

  case SAPP_EVENTTYPE_MOUSE_UP:
    _Input_CheckInput(INPUT_DEVICETYPE_MOUSE, 0, INPUT_INPUTTYPE_BUTTON, event->mouse_button, 0);
    break;

  default: break;
  }
}


// If event is mapped to input, then set virtual controller button
static void _Input_CheckInput(
  uint8_t deviceType, uint8_t deviceID, uint8_t inputType, uint16_t inputID, int16_t value
) {

  // printf(
  //   "deviceType=%d, deviceID=%d, inputType=%d, inputID=%d, value=%d\n",
  //   deviceType, deviceID, inputType, inputID, value
  // );

  Mapping_t event = {
    .deviceType = deviceType,
    .deviceID   = deviceID,
    .inputType  = inputType,
    .inputID    = inputID,
    .value      = value,
  };
  this.lastEvent = event;

  for (uint32_t p = 0; p < 2; p++) {
    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
      Mapping_t mapping = this.controllerMappings[p][i];
      // printf("%08x, %08x\n", mapping.raw, event.raw);
      if ((mapping.raw & 0xffff) == (event.raw & 0xffff)) {
        int16_t threshold = mapping.value;
        if (((threshold < 0) && (value <= threshold)) || ((threshold > 0) && value >= threshold)) {
          this.curr[p] |=  (1 << i);
        } else {
          this.curr[p] &= ~(1 << i);
        }
      }
    }
  }

}


// Get registered gamepad id from device
static int8_t _Input_GetDeviceID(struct Gamepad_device* device) {
  if (device->deviceID == this.gamepadID[0])
    return 0;
  else if (device->deviceID == this.gamepadID[1])
    return 1;

  return -1;
}


// Button down callback
static void _Input_GamepadButtonDown(struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context) {
  int8_t deviceID = _Input_GetDeviceID(device);
  if (deviceID >= 0)
    _Input_CheckInput(INPUT_DEVICETYPE_CONTROLLER, deviceID, INPUT_INPUTTYPE_BUTTON, buttonID, 0x7fff);
}


// Button up callback
static void _Input_GamepadButtonUp(
  struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context
) {
  int8_t deviceID = _Input_GetDeviceID(device);
  if (deviceID >= 0)
    _Input_CheckInput(INPUT_DEVICETYPE_CONTROLLER, deviceID, INPUT_INPUTTYPE_BUTTON, buttonID, 0);
}


// Axis callback
static void _Input_GamepadAxisChange(
  struct Gamepad_device * device,
  unsigned int axisID, float value, float lastValue, double timestamp, void * context
) {
  int8_t deviceID = _Input_GetDeviceID(device);
  if (deviceID >= 0) {
    int16_t axisValue = (int16_t)(value * 32767.0f);
    _Input_CheckInput(INPUT_DEVICETYPE_CONTROLLER, deviceID, INPUT_INPUTTYPE_AXIS, axisID, axisValue);
  }
}


// Controller connected callback
static void _Input_GamepadAttached(struct Gamepad_device * device, void * context) {
  VSmile_Log("Gamepad attached");
  if (this.gamepadID[0] == -1)
    this.gamepadID[0] = device->deviceID;
  else if (this.gamepadID[1] == -1)
    this.gamepadID[1] = device->deviceID;
}


// Controller disconnected callback
static void _Input_GamepadDetached(struct Gamepad_device * device, void * context) {
  VSmile_Log("Gamepad detached");
  if (device->deviceID == this.gamepadID[0])
    this.gamepadID[0] = -1;
  else if (device->deviceID == this.gamepadID[1])
    this.gamepadID[1] = -1;
}
