CFLAGS=-Wall -std=c99
DEFS=
# CC=gcc
INCS=-I. -Isrc/backend/lib

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
	LIBS+=-lkernel32 -luser32 -lshell32 -lgdi32 -ld3d11 -ldxgi -lole32 -loleaut32 -lomdlg32
	OUTEXT=.exe
else ifeq ($(platform), linux)
	DEFS+=-D_GNU_SOURCE
	CFLAGS+=-pthread
	LIBS+=-lX11 -lXi -lXcursor -lGL -ldl -lm -lasound
else ifeq ($(platform), macos)
  LIBS+=-framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox -framework IOKit
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

CFLAGS += $(DEFS)
CFLAGS += $(INCS)

obj=$(src:.c=.o)

.PHONY: VFrown clean

VFrown: $(obj)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(obj)
