#ifndef NEWLIB_H
#define NEWLIB_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengles2.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdbool.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define PLATFORM_WEB    0x0000
#define PLATFORM_LINUX  0x1000
#define PLATFORM_WIN32  0x0100
#define PLATFORM_MACOS  0x0010

typedef struct nl_glyph {
  unsigned width;
  unsigned height;
  struct {
    int x, y;
  } advance;
  struct {
    int x, y;
  } bearing;
  GLuint texture;
} nl_glyph;

typedef struct nl_font {
  FT_Face face;
  int size;
  nl_glyph glyphs[128];
  int baseline_y;
} nl_font;

typedef struct mouse_in_t
{
  int x, y;
  Uint32 mask;
} mouse_in_t;

extern lua_State* L;
extern SDL_Window* window;
extern SDL_Event event;
extern nl_font* builtin_font;

extern Uint8* keyboard_state;
extern mouse_in_t mouse_state;

extern bool crashed;

extern char* WINDOW_TITLE;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

extern unsigned char VeraMono_ttf[];
extern unsigned int VeraMono_ttf_len;

int newlib_init ( int argc, char* argv[] );
void newlib_process_events ( void );
void newlib_update_timers ( void );
void newlib_lua_error ( void );

#endif
