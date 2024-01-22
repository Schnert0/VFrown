#if defined(_WIN32)
  #define SOKOL_D3D11
#elif defined(__APPLE__)
  #define SOKOL_METAL
#elif defined(__linux__)
  #define SOKOL_GLCORE33
#else
  #error Unknown platform
#endif

#ifndef INI_IMPLEMENTATION
#define INI_IMPLEMENTATION
#endif // INI_IMPLEMENTATION

#ifndef NK_IMPLEMENTATION
#define NK_IMPLEMENTATION
#endif // NK_IMPLEMENTATION

#ifndef SOKOL_NUKLEAR_IMPL
#define SOKOL_NUKLEAR_IMPL
#endif // SOKOL_NUKLEAR_IMPL

#ifndef SOKOL_APP_IMPL
#define SOKOL_APP_IMPL
#endif // SOKOL_APP_IMPL

#ifndef SOKOL_AUDIO_IMPL
#define SOKOL_AUDIO_IMPL
#endif // SOKOL_AUDIO_IMPL

#ifndef SOKOL_GFX_IMPL
#define SOKOL_GFX_IMPL
#endif // SOKOL_GFX_IMPL

#ifndef SOKOL_GL_IMPL
#define SOKOL_GL_IMPL
#endif // SOKOL_GL_IMPL

#ifndef SOKOL_GLUE_IMPL
#define SOKOL_GLUE_IMPL
#endif // SOKOL_GLUE_IMPL

#include "lib/gamepad/Gamepad_private.c"
#if defined(_WIN32)
  #include "lib/gamepad/Gamepad_windows_dinput.c"
#elif defined(__APPLE__)
  #include "lib/gamepad/Gamepad_macosx.c"
#elif defined(__linux__)
  #include "lib/gampead/Gamepad_linux.c"
#else
  #error Unknown platform
#endif

#include "libs.h"
