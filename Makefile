###
# Linux
###
ifeq ($(target),linux)
	CC=gcc
	CFLAGS=-c -Wall -g `sdl2-config --cflags` `freetype-config --cflags` -Ilibs/lua_native
	LDFLAGS=`sdl2-config --libs` `freetype-config --libs` -llua -Llibs/lua_native -lm -lGL -Wl,-E -ldl -lSDL2_image
	SOURCES+=linux_main.c
	EXECUTABLE=newlib
endif

###
# Emscripten
###
ifeq ($(target),emsc)
	CC=emcc
	ESFLAGS=-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_FREETYPE=1
	CFLAGS+=$(ESFLAGS) -Ilibs/lua_emsc -O3 
	LDFLAGS+=-O3 -llua -Llibs/lua_emsc $(ESFLAGS) --preload-file aw --pre-js arguments.js
	SOURCES+=emsc_main.c
	EXECUTABLE=main.html
endif

SOURCES+=newlib.c veramono.c libs/lbuffer/lbuffer.c libs/lbuffer/lbufflib.c
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

clean:
	rm -f $(OBJECTS) \
	rm -f $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
