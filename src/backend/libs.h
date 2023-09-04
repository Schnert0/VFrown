#ifndef LIBS_H
#define LIBS_H

#include "../common.h"
#include "../core/vsmile.h"

#ifndef DNDEBUG
  #ifdef SOKOL_LOG
    #undef SOKOL_LOG
  #endif // SOKOL_LOG
  #define SOKOL_LOG(msg) VSmile_Error(msg);
#endif // DNDEBUG

// #include "lib/ini.h"
#include "lib/nuklear.h"

#ifndef SOKOL_APP_INCLUDED
#include "lib/sokol_app.h"
#endif // SOKOL_APP_INCLUDED

#ifndef SOKOL_AUDIO_INCLUDED
#include "lib/sokol_audio.h"
#endif // SOKOL_AUDIO_INCLUDED

#ifndef SOKOL_GFX_INCLUDED
#include "lib/sokol_gfx.h"
#endif // SOKOL_GFX_INCLUDED

#ifndef SOKOL_GL_INCLUDED
#include "lib/sokol_gl.h"
#endif // SOKOL_GL_INCLUDED

#ifndef SOKOL_NUKLEAR_INCLUDED
#include "lib/sokol_nuklear.h"
#endif // SOKOL_NUKLEAR_INCLUDED

#ifndef SOKOL_GLUE_INCLUDED
#include "lib/sokol_glue.h"
#endif // SOKOL_GLUE_INCLUDED

#endif // LIBS_H
