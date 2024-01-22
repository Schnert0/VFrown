#include "main.h"
#include "backend/input.h"
#include "backend/ui.h"

static float speed;
static char romPath[1024];
static char sysromPath[1024];

// Called when the application is initializing.
static void initFunc() {
  speed = 0.0f;

  if (!Backend_Init()) {
    VSmile_Error("Failed to initialize backend");
  }

  if (!Input_Init()) {
    VSmile_Error("Failed to initialize input handler");
  }

  // Emulator core
  if (!VSmile_Init()) {
    VSmile_Error("Failed to initialize emulation core");
  }

  if (sysromPath[0] != '\0')
    VSmile_LoadSysRom((const char*)&sysromPath);
  else
    VSmile_LoadSysRom("sysrom/sysrom.bin");


  if (romPath[0] != '\0') {
    VSmile_LoadROM((const char*)&romPath);
    VSmile_SetPause(false);
  } else {
    VSmile_SetPause(true);
  }

  VSmile_SetRegion(0xf);
  VSmile_SetIntroEnable(true);

  VSmile_Reset();

  if (!UI_Init()) {
    VSmile_Error("Failed to create UI handler");
  }

}


// Called on every frame of the application.
static void frameFunc() {
  Input_Update();

  float systemSpeed = Backend_GetSpeed();
  speed += systemSpeed;
  if (speed >= 1.0f) {
    systemSpeed = 1.0f / systemSpeed;
    while (speed > 0.0f) {
      VSmile_RunFrame();
      speed -= systemSpeed;
    }
  }

  Backend_Update();
}


// Called when the application is shutting down
static void cleanupFunc() {
  UI_Cleanup();
  VSmile_Cleanup();
  Input_Cleanup();
  Backend_Cleanup();

  sg_shutdown();
}


// Called when an event (keypress, mouse movement, etc.) occurs
static void eventFunc(sapp_event* event) {
  UI_HandleEvent(event);
  Backend_HandleInput(event->key_code, event->type);
  Input_KeyboardMouseEvent(event);

  // TODO: move this to the backend
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    const int32_t numFiles = sapp_get_num_dropped_files();

    VSmile_LoadROM(sapp_get_dropped_file_path(0));
    if (numFiles == 2)
      VSmile_LoadSysRom(sapp_get_dropped_file_path(1));
    VSmile_Reset();
    VSmile_SetPause(false);
  }
}

// Platform-agnostic main from sokol_app.h
sapp_desc sokol_main(int argc, char* argv[]) {
  romPath[0] = '\0';
  sysromPath[0] = '\0';

  if (argc == 2) {
    strncpy((char*)&romPath, argv[1], 1024);
    romPath[1023] = '\0';
  }
  else if (argc == 3) {
    strncpy((char*)&sysromPath, argv[1], 1024);
    strncpy((char*)&romPath, argv[2], 1024);
    sysromPath[1023] = '\0';
    romPath[1023] = '\0';
  }

  sapp_desc desc = {
    .init_cb = initFunc,
    .frame_cb = frameFunc,
    .cleanup_cb = cleanupFunc,
    .event_cb = (void(*)(const sapp_event*))eventFunc,
    .width = 640,
    .height = 480,
    .window_title = "V.Frown",
    .enable_dragndrop = true,
    .max_dropped_files = 2,
    .sample_count = 1,
    .high_dpi=false
  };

  return desc;
}
