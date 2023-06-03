#include "main.h"
#include "backend/ui.h"

static float speed;
static char romPath[1024];
static char sysromPath[1024];

// Called when the application is initializing.
static void init() {
  speed = 0.0f;

  if (!Backend_Init()) {
    VSmile_Error("Failed to initialize backend");
  }

  // Sokol Nuklear
  snk_setup(&(snk_desc_t){
    .dpi_scale = sapp_dpi_scale()
  });
  // TODO: idk what to call to validate snk_setup worked

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
static void frame() {
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
static void cleanup() {
  UI_Cleanup();
  VSmile_Cleanup();
  Backend_Cleanup();

  snk_shutdown();
  sg_shutdown();
}


// Called when an event (keypress, mouse movement, etc.) occurs
static void event(const sapp_event* event) {
  snk_handle_event(event);
  Backend_HandleInput(event->key_code, event->type);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    const int32_t numFiles = sapp_get_num_dropped_files();

    // for (int32_t i = 0; i < numFiles; i++) {
    //   printf("%s\n", sapp_get_dropped_file_path(i));
    // }

    VSmile_LoadROM(sapp_get_dropped_file_path(0));
    if (numFiles == 2)
      VSmile_LoadSysRom(sapp_get_dropped_file_path(1));
    VSmile_Reset();
    VSmile_SetPause(false);
  }
}


// Called in case a critical error occurs
static void failure(const char* message) {

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
    .init_cb = init,
    .frame_cb = frame,
    .cleanup_cb = cleanup,
    .event_cb = event,
    .fail_cb = failure,
    .width = 640,
    .height = 480,
    .window_title = "V.Frown",
    .enable_dragndrop = true,
    .max_dropped_files = 2,
    .sample_count = 1,
    .high_dpi=true
  };

  return desc;
}
