src=\
$(wildcard src/*.c)\
$(wildcard src/core/*.c)\
$(wildcard src/core/hw/*.c)\
$(wildcard src/backend/*.c)\

obj=$(src:.c=.o)

LDFLAGS= -O2 -lSDL2 -Wall -I.

VFrown: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
