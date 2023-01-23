CFLAGS=-std=c99
CFLAGS+=-Wall
DEFS=
# CC=gcc
INCS=-I.

src=\
$(wildcard src/*.c)\
$(wildcard src/backend/*.c)\
$(wildcard src/backend/lib/*.c)\
$(wildcard src/core/*.c)\
$(wildcard src/core/hw/*.c)\

# platform
ifndef platform
	platform=linux
endif

ifeq ($(platform), windows)
	CC=x86_64-w64-mingw32-gcc
	LIBS+=-lkernel32 -luser32 -lshell32 -lgdi32 -ld3d11 -ldxgi -lole32 -loleaut32
	OUTEXT=.exe
else ifeq ($(platform), linux)
	DEFS+=-D_GNU_SOURCE
	CFLAGS+=-pthread
	LIBS+=-lX11 -lXi -lXcursor -lGL -ldl -lm
else ifeq ($(platform), macos)
  LIBS+=-framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox
  CFLAGS+= -x objective-c
else ifeq ($(platform), web)
	LIBS+=-sUSE_WEBGL2=1
	CC=emcc
	OUTEXT=.html
	CFLAGS+=--shell-file=samples/sample-shell.html --embed-file images
endif


# build type
ifndef build
	build=debug
endif

ifeq ($(platform), web)
	ifeq ($(build), debug)
		CFLAGS+=-O1 -fno-inline -g
	else
		CFLAGS+=-Oz -g0 -flto
		CFLAGS+=-Wl,--strip-all,--gc-sections,--lto-O3
	endif
else ifeq ($(build), debug)
	CFLAGS+=-Og -g
else ifeq ($(build), release)
	CFLAGS+=-O3 -g -ffast-math -fno-plt -flto
	DEFS+=-DNDEBUG
endif


# backend
ifndef backend
	ifeq ($(platform), windows)
		backend=d3d11
	else ifeq ($(platform), macos)
		backend=metal
	else ifeq ($(platform), web)
		backend=gles3
	else
		backend=glcore33
	endif
endif

ifeq ($(backend), glcore33)
	DEFS+=-DSOKOL_GLCORE33
else ifeq ($(backend), gles2)
	DEFS+=-DSOKOL_GLES2
else ifeq ($(backend), gles3)
	DEFS+=-DSOKOL_GLES3
else ifeq ($(backend), d3d11)
	DEFS+=-DSOKOL_D3D11
else ifeq ($(backend), metal)
	DEFS+=-DSOKOL_METAL
else ifeq ($(backend), dummy)
	DEFS+=-DSOKOL_DUMMY_BACKEND
endif

CFLAGS += $(DEFS)
CFLAGS += $(INCS)

obj=$(src:.c=.o)

.PHONY: VFrown clean

VFrown: $(obj)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(obj)
