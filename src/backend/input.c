#include "input.h"
#include "lib/gamepad/Gamepad.h"
#include "userSettings.h"

#define KEYDEF(keycode) \
  (Mapping_t){\
    .deviceType=INPUT_DEVICETYPE_KEYBOARD,\
    .deviceID=0,\
    .inputType=INPUT_INPUTTYPE_BUTTON,\
    .inputID=keycode,\
    .value=0x7fff,\
  }

static Input_t this;

static const char* mappingStrings[2][NUM_INPUTS] = {
  {
    "c0up",    "c0down",   "c0left", "c0right",
    "c0red",   "c0yellow", "c0blue", "c0green",
    "c0enter", "c0help",   "c0exit", "c0abc",
  },
  {
    "c1up",    "c1down",   "c1left", "c1right",
    "c1red",   "c1yellow", "c1blue", "c1green",
    "c1enter", "c1help",   "c1exit", "c1abc",
  },
};

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

static bool _Input_GetMappingString(Mapping_t mapping, char string[11]);
static bool _Input_GetMappingFromString(Mapping_t* outMapping, char string[11]);
static char _Input_GetHexDigitChar(uint8_t digit);
static uint8_t _Input_GetValueFromHexDigit(char c);

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

  this.inputEnabled = true;

  char string[11];
  Mapping_t mapping;

  for (uint32_t i = 0; i < NUM_INPUTS; i++) {
    if (!UserSettings_ReadString(mappingStrings[0][i], string, 11))
      continue;
    _Input_GetMappingFromString(&mapping, string);
    this.controllerMappings[0][i] = mapping;
  }

  // this.controllerMappings[0][INPUT_UP]    = KEYDEF(SAPP_KEYCODE_UP);
  // this.controllerMappings[0][INPUT_DOWN]  = KEYDEF(SAPP_KEYCODE_DOWN);
  // this.controllerMappings[0][INPUT_LEFT]  = KEYDEF(SAPP_KEYCODE_LEFT);
  // this.controllerMappings[0][INPUT_RIGHT] = KEYDEF(SAPP_KEYCODE_RIGHT);

  // this.controllerMappings[0][INPUT_RED]    = KEYDEF(SAPP_KEYCODE_Z);
  // this.controllerMappings[0][INPUT_YELLOW] = KEYDEF(SAPP_KEYCODE_X);
  // this.controllerMappings[0][INPUT_BLUE]   = KEYDEF(SAPP_KEYCODE_C);
  // this.controllerMappings[0][INPUT_GREEN]  = KEYDEF(SAPP_KEYCODE_V);

  // this.controllerMappings[0][INPUT_ENTER] = KEYDEF(SAPP_KEYCODE_SPACE);
  // this.controllerMappings[0][INPUT_HELP]  = KEYDEF(SAPP_KEYCODE_A);
  // this.controllerMappings[0][INPUT_EXIT]  = KEYDEF(SAPP_KEYCODE_S);
  // this.controllerMappings[0][INPUT_ABC]   = KEYDEF(SAPP_KEYCODE_D);

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

  if (this.inputEnabled) {
    if (Input_GetChangedButtons(0)) {
      Controller_UpdateButtons(0, this.curr[0]);
    }
    if (Input_GetChangedButtons(1)) {
      Controller_UpdateButtons(1, this.curr[1]);
    }
  }

  this.prev[0] = this.curr[0];
  this.prev[1] = this.curr[1];
}


uint32_t Input_GetChangedButtons(uint8_t ctrlNum) {
  return this.curr[ctrlNum] ^ this.prev[ctrlNum];
}


void Input_SetControlsEnable(bool isEnabled) {
  this.inputEnabled = isEnabled;
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


void Input_SaveMappings() {
  char string[11];

  for (uint32_t p = 0; p < 2; p++) {
    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
      _Input_GetMappingString(this.controllerMappings[p][i], string);
      UserSettings_WriteString(mappingStrings[p][i], string, 11);
    }
  }

}

static bool _Input_GetMappingString(Mapping_t mapping, char string[11]) {
  memset(string, 0, sizeof(*string));

  // Mapping String is broken up as follows:
  // 'ABCDDDEEEE'
  // A: device type (K=keyboard, M=mouse, C=controller)
  // B: device id (range 0-3 inclusive)
  // C: input type (B=button, A=axis, M=motion)
  // D: input id (range 0x000-0x3ff inclusive)
  // E: (threshold) value (range 0x8000 - 0x7fff inclusive)

  switch (mapping.deviceType) {
  case INPUT_DEVICETYPE_KEYBOARD:
    string[0] = 'K'; // Keyboard type
    string[1] = '0'; // DeviceID 0
    break;

  case INPUT_DEVICETYPE_MOUSE:
    string[0] = 'M';
    string[1] = '0';
    break;

  case INPUT_DEVICETYPE_CONTROLLER:
    string[0] = 'C';
    string[1] = '0' + mapping.deviceID;
    break;

  default:
    VSmile_Warning("Could not generate mapping string: Invalid device type %d", mapping.deviceType);
    return false;
  }

  switch (mapping.inputType) {
  case INPUT_INPUTTYPE_BUTTON: string[2] = 'B'; break;
  case INPUT_INPUTTYPE_AXIS:   string[2] = 'A'; break;
  case INPUT_INPUTTYPE_MOTION: string[2] = 'M'; break;
  default:
    VSmile_Warning("Could not generate mapping string: Invalid input type %d", mapping.inputType);
    return false;
  }

  if (mapping.inputID > 0x3ff) {
    VSmile_Warning(
      "Could not generate mapping string: inputID %d greater than allowed range", mapping.inputID
    );
    return false;
  }

  // Get hex code for input ID
  uint16_t inputID = mapping.inputID;
  for (uint32_t i = 0; i < 3; i++) {
    string[3+i] = _Input_GetHexDigitChar(inputID & 0xf);
    inputID >>= 4;
  }

  // Get hex code for value
  uint16_t value = mapping.value;
  for (uint32_t i = 0; i < 4; i++) {
    string[6+i] = _Input_GetHexDigitChar(value & 0xf);
    value >>= 4;
  }

  string[10] = '\0';

  return true;
}


static bool _Input_GetMappingFromString(Mapping_t* outMapping, char string[11]) {
  Mapping_t mapping;
  mapping.raw = 0;

  switch (string[0]) {
  case 'K': case 'k':
    mapping.deviceType = INPUT_DEVICETYPE_KEYBOARD;
    break;
  case 'M': case 'm':
    mapping.deviceType = INPUT_DEVICETYPE_MOUSE;
    break;
  case 'C': case 'c': {
    mapping.deviceType = INPUT_DEVICETYPE_CONTROLLER;
    uint8_t deviceID = _Input_GetValueFromHexDigit(string[1]);
    if (deviceID > 3) {
      VSmile_Warning("Unable to load mapping from string: device id %d out of valid range", deviceID);
      return false;
    }
    mapping.deviceID = deviceID;
  } break;

  default:
    VSmile_Warning("Unable to load mapping from string: unknown device type '%c'", string[0]);
    return false;
  }

  switch (string[2]) {
  case 'B': case 'b': mapping.inputType = INPUT_INPUTTYPE_BUTTON; break;
  case 'A': case 'a': mapping.inputType = INPUT_INPUTTYPE_AXIS;   break;
  case 'M': case 'm': mapping.inputType = INPUT_INPUTTYPE_MOTION; break;
  default:
    VSmile_Warning("Unable to load mapping from string: unknown input type '%c'", string[2]);
    return false;
  }

  uint16_t inputID = 0;
  for (uint32_t i = 0; i < 3; i++) {
    uint8_t digitValue = _Input_GetValueFromHexDigit(string[3+i]);
    if (digitValue > 0xf) {
      VSmile_Warning("Unable to load mapping from string: invalid character '%c' in inputID", string[3+i]);
      return false;
    }
    inputID = (inputID << 4) | digitValue;
  }
  if (inputID > 0x3ff) {
    VSmile_Warning("Unable to load mapping from string: inputID %d out of valid range", inputID);
    return false;
  }
  mapping.inputID = inputID;

  int16_t value = 0;
  for (uint32_t i = 0; i < 3; i++) {
    uint8_t digitValue = _Input_GetValueFromHexDigit(string[3+i]);
    if (digitValue > 0xf) {
      VSmile_Warning("Unable to load mapping from string: invalid character '%c' in value", string[3+i]);
      return false;
    }
    value = (value << 4) | digitValue;
  }
  mapping.value = value;

  (*outMapping) = mapping;

  return true;
}

static char _Input_GetHexDigitChar(uint8_t digit) {
  if (digit >= 10)
    return (digit-10) + 'A';
  return digit + '0';
}


static uint8_t _Input_GetValueFromHexDigit(char c) {
  if (c >= '0' && c <= '9') {
    return c-'0';
  }

  if (c >= 'A' && c <= 'F') {
    return (c-'A')+10;
  }

  if (c >= 'a' && c <= 'f') {
    return (c-'a') + 10;
  }

  return 0xff;
}
