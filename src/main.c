#include "main.h"
#include "backend/ui.h"

static sg_image emuFrame;

// Called when the application is initializing.
static void init() {
  // Sokol GFX
  const sg_desc sgdesc = {
    .context = sapp_sgcontext()
  };
  sg_setup(&sgdesc);
  if (!sg_isvalid()) {
    VSmile_Error("Failed to create Sokol GFX context");
  }

  // Sokol GP
  sgp_setup(&(sgp_desc){0});
  if(!sgp_is_valid()) {
    VSmile_Error("Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
  }

  // Create Emulation framebuffer image
  emuFrame = sg_make_image(&(sg_image_desc){
    .width = 320,
    .height = 240,
    .usage = SG_USAGE_STREAM,
    .pixel_format = SG_PIXELFORMAT_RGBA8
  });


  // Sokol Nuklear
  snk_setup(&(snk_desc_t){
    .dpi_scale = sapp_dpi_scale()
  });
  // TODO: idk what to call to validate snk_setup worked

  if (!Backend_Init()) {
    VSmile_Error("Failed to initialize backend");
  }

  // Emulator core
  if (!VSmile_Init()) {
    VSmile_Error("Failed to initialize emulation core");
  }

  VSmile_LoadSysRom("sysrom/sysrom.bin");
  VSmile_SetRegion(0xf);
  VSmile_SetIntroEnable(true);
  VSmile_Reset();
  VSmile_SetPause(true);

  if (!UI_Init()) {
    VSmile_Error("Failed to create UI handler");
  }

}


// Called on every frame of the application.
static void frame() {
  // Cache window dimensions
  const int32_t width  = sapp_width();
  const int32_t height = sapp_height();

  // Get nuklear UI rendering context
  struct nk_context* ctx = snk_new_frame();

  // Run one frame of emulation
  Backend_Update();
  VSmile_RunFrame();

  sg_update_image(emuFrame, &(sg_image_data){
    .subimage[0][0] = {
      .ptr = Backend_GetScanlinePointer(0),
      .size = 320*240*sizeof(uint32_t)
    }
  });

  // Draw the emulated frame to the screen
  sgp_begin(width, height);
  sgp_viewport(0, 0, width, height);
  sgp_set_color(0.0f, 0.0f, 0.0f, 1.0f);
  sgp_clear();
  sgp_reset_color();
  sgp_set_image(0, emuFrame);
  sgp_draw_textured_rect(0, 0, width, height);
  sgp_unset_image(0);

  UI_RunFrame(ctx);

  // Draw UI content and render to the window
  const sg_pass_action pass_action = { 0 };
  sg_begin_default_pass(&pass_action, width, height);
  sgp_flush();
  snk_render(width, height);
  sgp_end();
  sg_end_pass();
  sg_commit();
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
  // (void)argc;
  // (void)argv;

  sapp_desc desc = {
    .init_cb = init,
    .frame_cb = frame,
    .cleanup_cb = cleanup,
    .event_cb = event,
    .fail_cb = failure,
    .width = 640,
    .height = 480,
    .window_title = "V.Frown - The V.Smile Emulator",
    .enable_dragndrop = true,
    .max_dropped_files = 2,
    .sample_count = 1,
    .high_dpi=true
  };

  return desc;
}
