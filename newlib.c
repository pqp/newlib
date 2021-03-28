#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

// Mutable strings for Lua
#include "libs/lbuffer/lbuffer.h"

#include "newlib.h"

/* TODO:
   - Sprite batching
*/

static int lua_load_font ( lua_State *L );
static int lua_load_image ( lua_State *L );
static int lua_draw_image ( lua_State *L );
static int lua_draw_canvas ( lua_State *L );
static int lua_draw_pixel ( lua_State *L );
static int lua_draw_rect ( lua_State *L );
static int lua_fill_rect ( lua_State *L );
static int lua_draw_line ( lua_State *L );
static int lua_draw_text ( lua_State *L );
static int lua_draw_color ( lua_State *L );
static int lua_key_down ( lua_State *L );
static int lua_mouse_down ( lua_State *L );
static int lua_controller_get_axis ( lua_State *L );
static int lua_controller_get_button ( lua_State *L );
static int lua_rect_intersect ( lua_State *L );
static int lua_new_canvas ( lua_State *L );
static int lua_get_render_target ( lua_State *L );
static int lua_set_render_target ( lua_State *L );
static int lua_render_clear ( lua_State *L );
static int lua_new_timer ( lua_State *L );
static int lua_delete_timer ( lua_State *L );

lua_State* L = NULL;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_GLContext* gl_context = NULL;
SDL_GameController* game_controller = NULL;

typedef struct shader_t {
  GLuint vert, frag;
  GLuint program;

  // TODO: Implement more flexible uniform structure
  GLint proj_uniform;
  GLint trans_uniform;
  GLint color_uniform;
} shader_t;

GLuint rvert_shader;
GLuint rfrag_shader;

shader_t* rect_program;
shader_t* img_program;
shader_t* fb_program;
shader_t* font_program;

GLuint rvbo;
GLuint rtexbo;

GLuint r_fillebo;
GLuint r_lineebo;

GLuint imgfrag_shader;
GLuint fbfrag_shader;
GLuint fontfrag_shader;

vec3_t draw_color;
mat4_t projection;

typedef struct canvas_t {
  GLuint framebuffer;
  GLuint texture;
  int fb_width, fb_height;
  int render_width, render_height;
} canvas_t;

canvas_t* bound_canvas;

static const GLfloat rverts[] = {
     0.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
};

static const GLfloat rtexverts[] = {
     0.0f, 0.0f,
     1.0f, 0.0f,
     0.0f, 1.0f,
     1.0f, 1.0f
};

static const GLushort r_fillelements[] = { 0, 1, 2, 3 };
static const GLushort r_lineelements[] = { 0, 2, 3, 1, 0 };

GLchar rvert_src[] =
  "attribute vec4 position; \n"
  "attribute vec2 texcoord; \n"
  "uniform mat4 projection; \n"
  "uniform mat4 transform; \n"
  "varying vec2 tex_coord; \n"
  "void main() \n"
  "{ \n"
  "  gl_Position = projection * transform * vec4( position.xy, 0.0, 1.0 ); \n"
  "  tex_coord = texcoord; \n"
  "} \n";

GLchar rfrag_src[] =
  "precision mediump float; \n"
  "uniform vec3 color; \n"
  "void main() \n"
  "{ \n"
  "  gl_FragColor = vec4( color.xyz, 1.0 ); \n"
  "} \n";

GLchar imgfrag_src[] =
  "precision mediump float; \n"
  "uniform sampler2D tex; \n"
  "varying vec2 tex_coord; \n"
  "void main() \n"
  "{ \n"
  "  vec4 color = texture2D( tex, vec2( tex_coord.x, tex_coord.y ) ); \n"
  "  gl_FragColor = color; \n"
  "} \n";

// Flip framebuffer on y axis
GLchar fbfrag_src[] =
  "precision mediump float; \n"
  "uniform sampler2D tex; \n"
  "varying vec2 tex_coord; \n"
  "void main() \n"
  "{ \n"
  "  vec4 color = texture2D( tex, vec2( tex_coord.x, -tex_coord.y ) ); \n"
  "  gl_FragColor = color; \n"
  "} \n";

GLchar fontfrag_src[] =
  "precision mediump float; \n"
  "uniform sampler2D tex; \n"
  "varying vec2 tex_coord; \n"
  "void main() \n"
  "{ \n"
  "  float alpha = texture2D( tex, tex_coord ).a; \n"
  "  vec4 color = vec4( 1.0, 1.0, 1.0, alpha ); \n"
  "  gl_FragColor = color; \n"
  "} \n";

nl_font* builtin_font = NULL;
FT_Library library;

int conv_arg = 0;
char* conv_func = NULL;

char* workdir = "./";
char* CONFIG_LUA = "./config.lua";
char* MAIN_LUA = "./main.lua";

bool crashed = false;

char* WINDOW_TITLE = "newlib";
int WINDOW_WIDTH = 640;
int WINDOW_HEIGHT = 480;

Uint8* keyboard_state;
mouse_in_t mouse_state;

typedef struct nl_timer
{
  Uint32 start;
  Uint32 duration;
  Uint32 loops;
  int lref;
  bool active;
} nl_timer;

typedef struct nl_timer_node
{
  nl_timer* timer;
  nl_timer* next;
} nl_timer_node;

typedef struct nl_timer_list
{
  nl_timer_node* begin;
} nl_timer_list;

nl_timer_list timer_list;

static const
luaL_Reg lib[] = {
  { "load_font", lua_load_font },
  { "load_image", lua_load_image },
  { "draw_image", lua_draw_image },
  { "draw_canvas", lua_draw_canvas },
  { "draw_color", lua_draw_color },
  { "draw_pixel", lua_draw_pixel },
  { "draw_rect", lua_draw_rect },
  { "fill_rect", lua_fill_rect },
  { "draw_line", lua_draw_line },
  { "draw_text", lua_draw_text },
  { "key_down", lua_key_down },
  { "mouse_down", lua_mouse_down },
  { "controller_get_axis", lua_controller_get_axis },
  { "controller_get_button", lua_controller_get_button },
  { "rect_intersect", lua_rect_intersect },
  { "new_canvas", lua_new_canvas },
  { "get_render_target", lua_get_render_target },
  { "set_render_target", lua_set_render_target },
  { "render_clear", lua_render_clear },
  { "new_timer", lua_new_timer },
  { "delete_timer", lua_delete_timer },
  { NULL, NULL }
};

typedef struct keyvalue_pair {
  const char* name;
  const unsigned int value;
} keyvalue_pair;

// Key scancodes exposed to Lua interpreter
static const
keyvalue_pair scancodes[] = {
  { "KEY_LEFT", SDL_SCANCODE_LEFT },
  { "KEY_RIGHT", SDL_SCANCODE_RIGHT },
  { "KEY_DOWN", SDL_SCANCODE_DOWN },
  { "KEY_UP", SDL_SCANCODE_UP },
  { "KEY_SPACE", SDL_SCANCODE_SPACE },
  { "KEY_RETURN", SDL_SCANCODE_RETURN },
  { "KEY_BACKSPACE", SDL_SCANCODE_BACKSPACE },
  { NULL, 0 }
};

static const
keyvalue_pair controller_axes[] = {
  { "CONTROLLER_AXIS_LEFTX", SDL_CONTROLLER_AXIS_LEFTX },
  { "CONTROLLER_AXIS_LEFTY", SDL_CONTROLLER_AXIS_LEFTY },
  { "CONTROLLER_AXIS_RIGHTX", SDL_CONTROLLER_AXIS_RIGHTX },
  { "CONTROLLER_AXIS_RIGHTY", SDL_CONTROLLER_AXIS_RIGHTY },
  { NULL, 0 }
};

static const
keyvalue_pair controller_buttons[] = {
  { "CONTROLLER_BUTTON_A", SDL_CONTROLLER_BUTTON_A },
  { "CONTROLLER_BUTTON_B", SDL_CONTROLLER_BUTTON_B },
  { "CONTROLLER_BUTTON_X", SDL_CONTROLLER_BUTTON_X },
  { "CONTROLLER_BUTTON_Y", SDL_CONTROLLER_BUTTON_Y },
  { NULL, 0 }
};

void
conv_reset ( char* func_name )
{
  conv_arg = 1;
  conv_func = func_name;
}

const char*
conv_string ( lua_State* L, int idx )
{
  if ( !lua_isstring( L, idx ) ) {
    printf( "%s: Argument %d isn't a string\n", conv_func, conv_arg );
    exit( 1 );
  }

  conv_arg++;

  return lua_tostring( L, idx );
}

int
conv_int ( lua_State* L, int idx )
{
  if ( !lua_isnumber( L, idx ) ) {
    printf( "%s: Argument %d isn't an integer\n", conv_func, conv_arg );
    exit( 1 );
  }

  conv_arg++;

  return lua_tointeger( L, idx );
}

float
conv_num ( lua_State* L, int idx )
{
  if ( !lua_isnumber( L, idx ) ) {
    printf( "%s: Argument %d isn't a number\n", conv_func, conv_arg );
    exit( 1 );
  }

  conv_arg++;

  return lua_tonumber( L, idx );
}

void*
conv_userdata ( lua_State* L, int idx )
{
  if ( !lua_isuserdata( L, idx ) ) {
    printf( "%s: Argument %d isn't userdata\n", conv_func, conv_arg );
    exit( 1 );
  }

  conv_arg++;

  return lua_touserdata( L, idx );
}

static GLuint
intern_load_image ( const char* filename )
{
  SDL_Surface* surface = NULL;
  GLuint texture = 0;

  surface = IMG_Load( filename );
  if ( !surface ) {
    printf( "Failed to load %s: %s\n", filename, SDL_GetError() );
    exit ( 1 );
  }

  glGenTextures( 1, &texture );
  glBindTexture( GL_TEXTURE_2D, texture );

  int mode = GL_RGB;

  if ( surface->format->BytesPerPixel == 4 )
    mode = GL_RGBA;

  glTexImage2D( GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

  glBindTexture( GL_TEXTURE_2D, 0 );

  return texture;
}

static void
intern_draw_image ( GLuint texture, shader_t* shader, const int x, const int y, const int w, const int h, const double angle )
{
  mat4_t transform;

  transform = m4_identity();
  transform = m4_translation( vec3( x, y, 0 ) );

  // Move the origin to the middle of the image and rotate it.
  // Then pull the origin back to the top-left of the image.
  transform = m4_mul( transform, m4_translation( vec3( 0.5f * w, 0.5f * h, 0 ) ) );
  transform = m4_mul( transform, m4_rotation_z( angle ) );
  transform = m4_mul( transform, m4_translation( vec3( -0.5f * w, -0.5f * h, 0 ) ) );

  transform = m4_mul( transform, m4_scaling( vec3( w, h, 0 ) ) );

  //printf( "Binding tex %d to draw\n", texture );

  glUseProgram( shader->program );

  glBindTexture( GL_TEXTURE_2D, texture );

  // Update uniforms
  glUniformMatrix4fv( shader->proj_uniform, 1, GL_FALSE, (GLfloat* )&projection );
  glUniformMatrix4fv( shader->trans_uniform, 1, GL_FALSE, (GLfloat* )&transform );
  glUniform3fv( shader->color_uniform, 1, (GLfloat* )&draw_color );

  // Rect vertex coordinates
  glBindBuffer( GL_ARRAY_BUFFER, rvbo );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( GLfloat ) * 3, 0 );

  // Texture coordinates (same as vertex)
  glBindBuffer( GL_ARRAY_BUFFER, rtexbo );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( GLfloat ) * 2, 0 );

  glEnableVertexAttribArray( 0 );
  glEnableVertexAttribArray( 1 );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, r_fillebo );
  glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0 );
  glDisableVertexAttribArray( 0 );
  glDisableVertexAttribArray( 1 );

  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  glBindTexture( GL_TEXTURE_2D, 0 );
}

static void
intern_draw_char ( nl_font* font, const unsigned char c, const int x, const int y )
{
  nl_glyph glyph;

  if ( !font ) {
    printf( "Tried to draw a character from a NULL font! What the hell are you doing???\n" );
    exit( 1 );
  }

  glyph = font->glyphs[c];

  intern_draw_image( glyph.texture, font_program, x, y, glyph.width, glyph.height, 0 );
}

static void
intern_draw_text ( nl_font* font, const char* str, const int x, const int y, const int wrap_len )
{
  int len;
  int pen_x, pen_y;
  nl_glyph glyph;

  pen_x = x;
  pen_y = y;

  len = strlen( str );

  // Get the character with the largest y bearing.
  // The string's layout will be based on it.
  int baseline_y = font->baseline_y;
  for ( unsigned i = 0; i < len; i++ ) {
    int bearing = font->glyphs[str[i]].bearing.y;
    
    if ( bearing > baseline_y )
      baseline_y = bearing;
  }

  // Iterate through the string and draw
  // the corresponding glyphs.
  for ( unsigned i = 0; i < len; i++ ) {
    int nx, ny;
    unsigned char c;
    
    c = str[i];
    glyph = font->glyphs[c];

    switch ( c ) {
    case ' ':
      pen_x += font->size / 2;
      continue;
    case '\n':
      pen_x = x;
      pen_y += font->size;
      continue;
    };

    // If this character is to be drawn past wrap_len,
    // wrap to the next line.
    if ( wrap_len != 0 && ( pen_x + glyph.bearing.x ) >= wrap_len ) {
      pen_x = x;
      pen_y += font->size;
    }
    // Proper padding to left of character
    nx = pen_x + ( glyph.bearing.x );
    // Proper padding to top of character
    ny = pen_y + ( baseline_y - glyph.bearing.y );
                               
    intern_draw_char( font, str[i], nx, ny );

    // Advance to the next position
    pen_x += glyph.advance.x >> 6;
    pen_y += glyph.advance.y >> 6;
  }
}

void
newlib_lua_error ( void )
{
  const char* error = lua_tostring( L, -1 );

  crashed = true;

  intern_draw_text( builtin_font, error, 0, 0, WINDOW_WIDTH );
  printf( "Error: %s\n", error );
}

static bool
power_of_two( int x )
{
  return ( x & ( x - 1 ) ) == 0;
}

static canvas_t*
intern_new_canvas ( const int width, const int height )
{
  canvas_t* canvas;

  canvas = malloc( sizeof( canvas_t ) );

  canvas->render_width = width;
  canvas->render_height = height;

  // WebGL 1.0 doesn't support non-power-of-two textures.
  // If either the width or the height specified doesn't
  // fit this requirement, pad the framebuffer so that it does.

  // Then when we draw the framebuffer we just scale each axis
  // by (fb_width/render_width) (fb_height/render_height) etc

  // If the specified dimensions are powers of two, then leave
  // it as is.
  if ( !power_of_two( width ) )  {
    int w = width;
    while ( !power_of_two( w ) ) {
      w++;
    }

    canvas->fb_width = w;

    printf( "Resizing framebuffer width to %d (power of two)\n", w );
  } else {
    canvas->fb_width = width;
  }

  if ( !power_of_two( height ) ) {
    int h = height;
    while ( !power_of_two( h ) ) {
      h++;
    }

    canvas->fb_height = h;
    printf( "Resizing framebuffer height to %d (power of two)\n", h );
  } else {
    canvas->fb_height = height;
  }

  glGenFramebuffers( 1, &canvas->framebuffer );
  glBindFramebuffer( GL_FRAMEBUFFER, canvas->framebuffer );

  glGenTextures( 1, &canvas->texture );
  glBindTexture( GL_TEXTURE_2D, canvas->texture );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, canvas->fb_width, canvas->fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas->texture, 0 );

  if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
    printf( "problems generating framebuffer\n" );
    exit( 1 );
  }

  glBindTexture( GL_TEXTURE_2D, 0 );
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );

  printf( "generated: fb %d tex %d\n", canvas->framebuffer, canvas->texture );

  return canvas;
}

static int
lua_new_canvas ( lua_State* L )
{
  canvas_t* canvas;
  int width, height;

  conv_reset( "new_canvas()" );
  width = conv_int( L, -2 );
  height = conv_int( L, -1 );

  canvas = intern_new_canvas( width, height );
  if ( !canvas ) {
    printf( "Failed to create canvas\n" );
    lua_pushlightuserdata( L, NULL );
  } else {
    lua_pushlightuserdata( L, canvas );
  }

  return 1;
}

static int
lua_get_render_target ( lua_State* L )
{
  printf( "returning fb %d tex %d\n", bound_canvas->framebuffer, bound_canvas->texture );
  lua_pushlightuserdata( L, bound_canvas );

  return 1;
}

static int
lua_set_render_target ( lua_State* L )
{
  canvas_t* canvas;

  conv_reset( "set_render_target()" );

  canvas = conv_userdata( L, -1 );
  bound_canvas = canvas;

  /*
  if ( bound_canvas->framebuffer == 0 )
    printf( "Binding default framebuffer\n" );
  else
    printf( "Binding framebuffer %d\n", bound_canvas->framebuffer );
  */

  glBindFramebuffer( GL_FRAMEBUFFER, bound_canvas->framebuffer );
  glViewport( 0, 0, bound_canvas->fb_width, bound_canvas->fb_height );

  projection = m4_identity();
  projection = m4_ortho( 0.f, bound_canvas->fb_width, bound_canvas->fb_height, 0.f, 0.f, 1.f );

  return 0;
}

static int
lua_render_clear ( lua_State* L )
{
  glClear( GL_COLOR_BUFFER_BIT );

  return 0;
}

static int
lua_load_image ( lua_State* L )
{
  GLuint texture;
  const char* filename = NULL;

  conv_reset( "load_image()" );
  filename = conv_string( L, -1 );
  if ( !filename ) {
    exit( 1 );
  }

  texture = intern_load_image( filename );
  if ( !texture ) {
    exit( 1 );
  }

  lua_pushinteger( L, texture );

  return 1;
}

nl_font*
intern_load_glyphs ( nl_font* font, const int size )
{
  FT_Bitmap bitmap;
  FT_GlyphSlot glyph;
  GLuint texture;

  font->size = size;

  FT_Set_Pixel_Sizes( font->face, 0, size );

  // One byte alignment (because of GL_ALPHA fonts)
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

  for ( unsigned i = 0; i < 128; i++ ) {
    int error = FT_Load_Char( font->face, i, FT_LOAD_RENDER );
    if ( error ) {
      printf( "FT_Load_Char() failed\n" );
      continue;
    }

    glyph = font->face->glyph;
    bitmap = glyph->bitmap;

    // Our new dimensions should we need
    // to resize to power-of-two dimensions
    int new_width = bitmap.width;
    int new_height = bitmap.rows;

    if ( bitmap.width <= 0 || bitmap.rows <= 0 ) {
      printf( "Glyph %d doesn't exist, skipping\n", i );
      continue;
    }

    // TODO: don't do this on native builds
    if ( !power_of_two( bitmap.width ) ) {
      int w = bitmap.width;
      while ( !power_of_two( w ) ) {
        w++;
      }

      new_width = w;
    }

    if ( !power_of_two( bitmap.rows ) ) {
      int h = bitmap.rows;
      while ( !power_of_two( h ) ) {
        h++;
      }

      new_height = h;
    }

    /*
    int bearing = font->glyphs[i].bearing.y;
    
    if ( bearing > font->baseline_y )
      font->baseline_y = bearing;
    */

    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );

    // Create a power-of-two blank texture, but write the glyph data
    // into a smaller sub-region (this way it displays correctly)

    // For WebGL.
    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, new_width, new_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0 );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, bitmap.width, bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.buffer );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glBindTexture( GL_TEXTURE_2D, 0 );

    // TODO: check texture creation status
    
    font->glyphs[i].width = new_width;
    font->glyphs[i].height = new_height;
    font->glyphs[i].advance.x = glyph->advance.x;
    font->glyphs[i].advance.y = glyph->advance.y;
    font->glyphs[i].bearing.x = glyph->bitmap_left;
    font->glyphs[i].bearing.y = glyph->bitmap_top;
    font->glyphs[i].texture = texture;
  }

  return font;
}

nl_font*
intern_load_font ( const char* filename, const int size )
{
  nl_font* font;

  if ( !filename || strlen( filename ) <= 0 ) {
    printf( "Invalid filename!\n" );
    exit( 1 );
  }

  font = calloc( 1, sizeof( nl_font ) );
  if ( !font ) {
    printf( "Couldn't allocate font!\n" );
    exit( 1 );
  }

  int error = FT_New_Face( library,
                           filename,
                           0,
                           &font->face );
  
  if ( error == FT_Err_Unknown_File_Format ) {
    printf( "Unknown font file format\n" );
    exit( 1 );
  }

  return intern_load_glyphs( font, size );
}

nl_font*
intern_load_memory_font ( const unsigned char* buffer, const int buffer_size, const int size )
{
  nl_font* font;

  if ( !buffer ) {
    printf( "Invalid TTF buffer!\n" );
    return NULL;
  }

  font = calloc( 1, sizeof( nl_font ) );
  if ( !font ) {
    printf( "Couldn't allocate font!\n" );
    return NULL;
  }

  int error = FT_New_Memory_Face( library,
                           buffer,
                           buffer_size,
                           0,
                           &font->face );
  
  if ( error == FT_Err_Unknown_File_Format ) {
    printf( "Unknown font file format\n" );
    return NULL;
  }

  return intern_load_glyphs( font, size );
}

static int
lua_load_font ( lua_State* L )
{
  nl_font* font;
  const char* filename;
  int size;

  conv_reset("load_font()");

  filename = conv_string( L, -2 );
  size = conv_int( L, -1 );

  font = intern_load_font( filename, size );

  lua_pushlightuserdata( L, (void*)font );

  return 1;
}

static int
lua_draw_image ( lua_State* L )
{
  GLuint texture;
  float x, y;
  float w, h;
  double angle;

  x = y = 0.f;
  w = h = 0.f;
  angle = 0.f;

  unsigned args = lua_gettop( L );

  conv_reset( "draw_image()" );

  // This is a weird way to go about doing this
  if ( args == 3 ) {
    texture = conv_int( L, -3 );
    x = conv_num( L, -2 );
    y = conv_num( L, -1 );
  } else if ( args == 6 ) {
    texture = conv_int( L, -6 );
    x = conv_num( L, -5 );
    y = conv_num( L, -4 );
    w = conv_num( L, -3 );
    h = conv_num( L, -2 );
    angle = conv_num( L, -1 );
  } else {
    printf( "botched image draw\n" );
    return 0;
  }

  intern_draw_image( texture, img_program, x, y, w, h, angle );

  return 0;
}

static int
lua_draw_canvas ( lua_State* L )
{
  canvas_t* canvas;
  float x, y;
  float w, h;
  double angle;

  x = y = 0;
  w = h = 0;
  angle = 0.f;

  unsigned args = lua_gettop( L );

  // This is a weird way to go about doing this
  if ( args == 3 ) {
    canvas = lua_touserdata( L, -3 );
    x = lua_tonumber( L, -2 );
    y = lua_tonumber( L, -1 );
  } else if ( args == 6 ) {
    canvas = lua_touserdata( L, -6 );
    x = lua_tonumber( L, -5 );
    y = lua_tonumber( L, -4 );
    w = lua_tonumber( L, -3 );
    h = lua_tonumber( L, -2 );
    angle = lua_tonumber( L, -1 );
  } else {
    printf( "botched image draw\n" );
    return 0;
  }

  // Scale the canvas to fit requested dimensions (instead of
  // the stretched default values if NPOT)
  float scale_x = (float)canvas->fb_width / (float)canvas->render_width;
  float scale_y = (float)canvas->fb_height / (float)canvas->render_height;

  intern_draw_image( canvas->texture, fb_program, x, y, w*scale_x, h*scale_y, angle );

  return 0;
}

static int
lua_draw_text ( lua_State* L )
{
  const char* str;
  nl_font* font;
  int x, y;
  int arg_max = 5;
  int wrap_len;

  unsigned args = lua_gettop( L );

  if ( args == 4 ) {
    font = lua_touserdata( L, -4 );
    str = lua_tostring( L, -3 );
    x = lua_tonumber( L, -2 );
    y = lua_tonumber( L, -1 );
    wrap_len = 0;
  } else if ( args == 5 ) {
    font = lua_touserdata( L, -5 );
    str = lua_tostring( L, -4 );
    x = lua_tonumber( L, -3 );
    y = lua_tonumber( L, -2 );
    wrap_len = lua_tonumber( L, -1 );
  }

  intern_draw_text( font, str, x, y, wrap_len );

  return 0;
}

static int
lua_draw_color ( lua_State* L )
{
  int r, g, b;

  conv_reset( "draw_color()" );

  r = conv_num( L, -3 );
  g = conv_num( L, -2 );
  b = conv_num( L, -1 );

  // Map [0, 255] (byte) to [0, 1] (float)

  draw_color.x = (float)r / 255.f;
  draw_color.y = (float)g / 255.f;
  draw_color.z = (float)b / 255.f;

  return 0;
}

static int
lua_draw_pixel ( lua_State* L )
{
  int x, y;

  conv_reset( "draw_pixel()" );

  x = conv_num( L, -2 );
  y = conv_num( L, -1 );

  SDL_RenderDrawPoint( renderer, x, y );

  return 0;
}

void
draw_rect ( const int x1, const int y1, const int x2, const int y2, const bool fill )
{
  mat4_t transform;
  float x_offset, y_offset;

  transform = m4_identity();
  transform = m4_translation( vec3( x1, y1, 0 ) );

  if ( !fill ) {
    // Shift vertices by 0.5 pixels to render rect outline corners properly
    // (search for "diamond exit rule")
    x_offset = 0.5f;
    y_offset = 0.5f;
  } else {
    x_offset = y_offset = 0.f;
  }

  transform = m4_mul( transform, m4_translation( vec3( x_offset, y_offset, 0 ) ) );
  transform = m4_mul( transform, m4_scaling( vec3( x2+x_offset, y2, 0 ) ) );

  glUseProgram( rect_program->program );

  glUniformMatrix4fv( rect_program->proj_uniform, 1, GL_FALSE, (GLfloat* )&projection );
  glUniformMatrix4fv( rect_program->trans_uniform, 1, GL_FALSE, (GLfloat* )&transform );
  glUniform3fv( rect_program->color_uniform, 1, (GLfloat* )&draw_color );

  glBindBuffer( GL_ARRAY_BUFFER, rvbo );

  glVertexAttribPointer( 0, 3, GL_FLOAT, 0, 0, 0 );
  glEnableVertexAttribArray( 0 );

  if ( fill ) {
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, r_fillebo );
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0 );
  } else {
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, r_lineebo );
    glDrawElements( GL_LINE_STRIP, 5, GL_UNSIGNED_SHORT, 0 );
  }

  glDisableVertexAttribArray( 0 );

  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

static int
lua_draw_rect ( lua_State* L )
{
  int x1, y1, x2, y2;

  conv_reset( "draw_rect()" );

  x1 = conv_num( L, -4 );
  y1 = conv_num( L, -3 );
  x2 = conv_num( L, -2 );
  y2 = conv_num( L, -1 );

  draw_rect( x1, y1, x2, y2, false );

  return 0;
}

static int
lua_fill_rect ( lua_State* L )
{
  int x1, y1, x2, y2;

  conv_reset( "fill_rect()" );

  x1 = conv_num( L, -4 );
  y1 = conv_num( L, -3 );
  x2 = conv_num( L, -2 );
  y2 = conv_num( L, -1 );

  draw_rect( x1, y1, x2, y2, true );

  return 0;
}

static int
lua_draw_line ( lua_State* L )
{
  int x1, x2, y1, y2;

  conv_reset( "draw_line()" );

  x1 = conv_num( L, -4 );
  y1 = conv_num( L, -3 );
  x2 = conv_num( L, -2 );
  y2 = conv_num( L, -1 );

  SDL_RenderDrawLine( renderer, x1, y1, x2, y2 );

  return 0;
}

// Retrieve field from Lua table
static int
getfield_int ( const char* key, int index )
{
  int result;

  lua_pushstring( L, key );
  lua_gettable( L, index );
  if ( !lua_isnumber( L, -1 ) ) {
    //printf( "ERROR: field retrieved isn't a number!\n" );
  } else {
    result = (int)lua_tonumber( L, -1 );
    //printf( "Retrieved %d\n", result );
  }
  lua_pop( L, 1 );

  return result;
}

static char*
getfield_str ( const char* key, int index )
{
  int result;

  lua_pushstring( L, key );
  lua_gettable( L, index );
  if ( !lua_isstring( L, -1 ) ) {
    //printf( "ERROR: field retrieved isn't a number!\n" );
  } else {
    result = (int)lua_tostring( L, -1 );
    //printf( "Retrieved %d\n", result );
  }
  lua_pop( L, 1 );

  return result;
}

// Build SDL_Rects with tables
// Do they intersect?
static int
lua_rect_intersect ( lua_State* L )
{
  SDL_Rect a, b;

  // -3: Rect A
  a.x = getfield_int( "x", -3 );
  a.y = getfield_int( "y", -3 );
  a.w = getfield_int( "w", -3 );
  a.h = getfield_int( "h", -3 );

  // -2: Rect B
  b.x = getfield_int( "x", -2 );
  b.y = getfield_int( "y", -2 );
  b.w = getfield_int( "w", -2 );
  b.h = getfield_int( "h", -2 );

  bool collide = SDL_HasIntersection( &a, &b );
  //printf( "A( %d, %d, %d, %d ) : B ( %d, %d, %d, %d ): %d\n", a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h, collide );
  lua_pushboolean( L, collide );

  return 1;
}

static int
lua_key_down ( lua_State* L )
{
  unsigned int key;

  conv_reset( "key_down()" );
  key = conv_num( L, -1 );

  lua_pushboolean( L, keyboard_state[key] );

  return 1;
}

static int
lua_mouse_down( lua_State* L )
{
  unsigned int button;

  conv_reset( "mouse_down()" );
  button = conv_int( L, -1 );

  lua_pushboolean( L, mouse_state.mask & SDL_BUTTON( button ) );
  
  return 1;
}

static int
lua_controller_get_axis ( lua_State* L )
{
  int axis;

  conv_reset( "controller_get_axis()" );
  axis = conv_int( L, -1 );

  Sint16 idx = SDL_GameControllerGetAxis( game_controller, axis );
  lua_pushnumber( L, idx );

  return 1;
}

static int
lua_controller_get_button ( lua_State* L )
{
  int button;

  conv_reset( "controller_get_button()" );
  button = conv_int( L, -1 );

  bool down = SDL_GameControllerGetButton( game_controller, button );
  lua_pushboolean( L, down );

  return 1;
}

GLuint
load_shader ( GLenum type, const GLchar* src )
{
  GLuint shader;
  GLint compiled;

  shader = glCreateShader( type );

  glShaderSource( shader, 1, &src, NULL );
  glCompileShader( shader );
  glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

  if ( !compiled ) {
    // TODO: get length of log and allocate memory on the heap,
    // not the stack
    char log[512];

    glGetShaderInfoLog( shader, 512, NULL, log );
    printf( "%s: \n", log );
  }

  return shader;
}

shader_t*
create_program( GLuint vert, GLuint frag )
{
  shader_t* shader = NULL;

  shader = malloc( sizeof( shader_t ) );
  if ( !shader ) {
    printf( "Failed to allocate memory for shader\n" );
  }

  shader->program = glCreateProgram();

  glAttachShader( shader->program, vert );
  glAttachShader( shader->program, frag );

  glBindAttribLocation( shader->program, 0, "position" );
  glLinkProgram( shader->program );

  shader->proj_uniform = glGetUniformLocation( shader->program, "projection" );
  shader->trans_uniform = glGetUniformLocation( shader->program, "transform" );
  shader->color_uniform = glGetUniformLocation( shader->program, "color" );

  return shader;
}

void
generate_buffer ( GLenum type, GLuint* buffer_obj, GLfloat* verts, int num )
{
  glGenBuffers( 1, buffer_obj );

  glBindBuffer( type, *buffer_obj );
  glBufferData( type, num * sizeof( GLfloat ), verts, GL_STATIC_DRAW );
}

static int
timer_start ( lua_State* L )
{
  if ( !lua_istable( L, -1 ) ) {
    printf( "timer_start: no table!\n" );
  }

  // get timer pointer
  // push this table's lref to the timer
  // start the timer.

  // Get reference to table (this also pops it from the stack)
  int ref = luaL_ref( L, LUA_REGISTRYINDEX );
  // Put the table back on the stack
  lua_rawgeti( L, LUA_REGISTRYINDEX, ref );

  // Get the raw timer pointer
  lua_getfield( L, -1, "_timer" );
  nl_timer* timer = lua_touserdata( L, -1 );

  // Assign the table reference to the timer
  timer->lref = ref;
  timer->active = true;
  timer->start = SDL_GetTicks();

  return 0;
}

static void
timer_complete ( nl_timer* timer )
{
  if ( !timer ) {
    printf( "Called timer_complete on null timer!\n" );
    exit( 1 );
  }

  lua_rawgeti( L, LUA_REGISTRYINDEX, timer->lref );

  if ( !lua_istable( L, -1 ) ) {
    printf( "timer_complete: no table!\n" );
  }

  lua_getfield( L, -1, "on_complete" );
  if ( lua_pcall( L, 0, 0, 0 ) ) {
    printf( "LUA: %s\n", lua_tostring( L, -1 ) );
  }

  if ( timer->loops != -1 ) {
    timer->active = false;
  } else {
    timer->active = true;
    timer->start = SDL_GetTicks();
  }
}

static int
lua_new_timer ( lua_State* L )
{
  Uint32 duration, loops;

  duration = lua_tointeger( L, -2 );
  loops = lua_tointeger( L, -1 );

  nl_timer* timer = malloc( sizeof( nl_timer ) );

  if ( !timer ) {
    printf( "Couldn't allocate timer\n" );
  }

  // List hasn't begun yet; allocate first node
  // and assign this timer to it.
  if ( !timer_list.begin ) {
    nl_timer_node* node = malloc( sizeof( nl_timer_node ) );
      
    if ( !node ) {
      printf( "Couldn't allocate timer node\n" );
      exit( 1 );
    }

    timer_list.begin = node;
    printf( "Allocated first node of timer list\n" );

    node->timer = timer;
    node->next = NULL;
  } else {
    // List has already been started;
    // attach this timer to the end of the list.

    for ( nl_timer_node* node = timer_list.begin; ; node = node->next ) {
      if ( node->next == NULL ) {
        nl_timer_node* n = malloc( sizeof( nl_timer_node ) );

        if ( !n ) {
          printf( "Couldn't allocate new node\n" );
          exit( 1 );
        }

        n->timer = timer;
        n->next = NULL;

        node->next = n;
        printf( "Attached new node to timer list\n" );

        break;
      }
    }
  }

  timer->duration = duration;
  timer->loops = loops;

  lua_newtable( L );
  lua_pushlightuserdata( L, timer );
  lua_setfield( L, -2, "_timer" );
  lua_pushcfunction( L, timer_start );
  lua_setfield( L, -2, "start" );
  lua_pushinteger( L, 2 );
  lua_setfield( L, -2, "id" );

  return 1;
}

static int
lua_delete_timer ( lua_State* L )
{
  nl_timer* timer = NULL;

  if ( !lua_istable( L, -1 ) ) {
    printf( "timer_start: no table!\n" );
  }

  // Get the raw timer pointer
  lua_getfield( L, -1, "_timer" );
  timer = lua_touserdata( L, -1 );

  if ( timer ) {
    free( timer );
    printf( "Freed %0x%x\n", timer );
  } 

  return 0;
}

int
newlib_init ( int argc, char* argv[] )
{
  printf( "Initializing...\n" );

  if ( argc >= 2 ) {
    workdir = argv[1];
  }

  printf( "Working directory: %s\n", workdir );

  // TODO: error check
  chdir( workdir );

  L = luaL_newstate();
  luaL_openlibs( L );
  luaopen_buffer( L );

  // Expose internal functions to Lua code
  for ( unsigned i = 0; lib[i].name != NULL; i++ ) {
    lua_register( L, lib[i].name, lib[i].func );
  }

  // Expose SDL_SCANCODE values to Lua code
  for ( unsigned i = 0; scancodes[i].name != NULL; i++ ) {
    lua_pushnumber( L, scancodes[i].value );
    lua_setglobal( L, scancodes[i].name );
  }

  // Expose SDL_GameControllerAxis
  for ( unsigned i = 0; controller_axes[i].name != NULL; i++ ) {
    lua_pushnumber( L, controller_axes[i].value );
    lua_setglobal( L, controller_axes[i].name );
  }

  for ( unsigned i = 0; controller_buttons[i].name != NULL; i++ ) {
    lua_pushnumber( L, controller_buttons[i].value );
    lua_setglobal( L, controller_buttons[i].name );
  }


  printf( "Loading %s...", CONFIG_LUA );
  int ret = luaL_dofile( L, CONFIG_LUA );

  switch ( ret ) {
  case LUA_OK:
    printf( "loaded.\n" );
    break;
  case LUA_ERRSYNTAX:
    printf( "syntax error!\n" );
    break;
  case LUA_ERRMEM:
    printf( "memory allocation error\n" );
    break;
  case LUA_ERRGCMM:
    printf( "LUA_ERRGCMM\n" );
    break;
  }

  lua_getglobal( L, "window" );

  if ( lua_istable( L, -2 ) ) {
    printf( "config error\n" );
  }

  WINDOW_TITLE = getfield_str( "title", -2 );
  WINDOW_WIDTH = getfield_int( "width", -2 );
  WINDOW_HEIGHT = getfield_int( "height", -2 );

  int flags = SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS;
  if ( SDL_Init( flags ) < 0 ) {
    printf( "Couldn't initialize SDL: %s\n", SDL_GetError() );
    exit( 1 );
  }

  if ( IMG_Init( IMG_INIT_PNG ) < 0 ) {
    printf( "Couldn't initialize SDL_image: %s\n", SDL_GetError() );
    exit( 1 );
  }

  unsigned num_joysticks = SDL_NumJoysticks();
  printf( "Attached controllers: %d\n", num_joysticks );

  if ( num_joysticks > 0 ) {
    game_controller = SDL_GameControllerOpen( 0 );

    if ( !game_controller ) {
      printf( "Couldn't open game controller: %s\n", SDL_GetError() );
    }
  }

  // TODO: Error messages should be more detailed
  if ( FT_Init_FreeType( &library ) ) {
    printf( "FreeType couldn't initialize\n" );
    exit( 1 );
  }

  printf( "Create (%d, %d) window\n", WINDOW_WIDTH, WINDOW_HEIGHT );

  window = SDL_CreateWindow( WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL );
  if ( !window ) {
    printf( "Can't init window\n" );
    exit( 1 );
  }

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );

  gl_context = SDL_GL_CreateContext( window );

  // Enable vsync
  SDL_GL_SetSwapInterval( 1 );

  // Enable blending
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glViewport( 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT );

  rvert_shader = load_shader( GL_VERTEX_SHADER, rvert_src );
  rfrag_shader = load_shader( GL_FRAGMENT_SHADER, rfrag_src );
  imgfrag_shader = load_shader( GL_FRAGMENT_SHADER, imgfrag_src );
  fbfrag_shader = load_shader( GL_FRAGMENT_SHADER, fbfrag_src );
  fontfrag_shader = load_shader( GL_FRAGMENT_SHADER, fontfrag_src );

  rect_program = create_program( rvert_shader, rfrag_shader );
  img_program = create_program( rvert_shader, imgfrag_shader );
  fb_program = create_program( rvert_shader, fbfrag_shader );
  font_program = create_program( rvert_shader, fontfrag_shader );

  generate_buffer( GL_ARRAY_BUFFER, &rvbo, rverts, 12 );
  generate_buffer( GL_ARRAY_BUFFER, &rtexbo, rtexverts, 8 );
  generate_buffer( GL_ELEMENT_ARRAY_BUFFER, &r_fillebo, r_fillelements, 4 );
  generate_buffer( GL_ELEMENT_ARRAY_BUFFER, &r_lineebo, r_lineelements, 5 );

  projection = m4_identity();
  projection = m4_ortho( 0.f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.f, 0.f, 1.f );

  // Allocate a canvas structure to represent
  // the default framebuffer.
  bound_canvas = malloc( sizeof( canvas_t ) );
  bound_canvas->fb_width = WINDOW_WIDTH;
  bound_canvas->fb_height = WINDOW_HEIGHT;
  bound_canvas->framebuffer = 0;
  bound_canvas->texture = -1;

  glBindFramebuffer( GL_FRAMEBUFFER, bound_canvas->framebuffer );

  timer_list.begin = NULL;

  // Load internal font (fallback for error messages)
  builtin_font = intern_load_memory_font( VeraMono_ttf, VeraMono_ttf_len, 18 );

  if ( !builtin_font ) {
    printf( "Failed to load built-in font. Something is very wrong!\n" );
    // exit( 1 );
  }

  printf( "Loading %s...", MAIN_LUA );
  ret = luaL_loadfile( L, MAIN_LUA );

  switch ( ret ) {
  case LUA_OK:
    printf( "loaded.\n" );
    break;
  case LUA_ERRSYNTAX:
    printf( "syntax error!\n" );
    break;
  case LUA_ERRMEM:
    printf( "memory allocation error\n" );
    break;
  case LUA_ERRGCMM:
    printf( "LUA_ERRGCMM\n" );
    break;
  }

  if ( lua_pcall( L, 0, 0, 0 ) ) {
    printf( "LUA: %s\n", lua_tostring( L, -1 ) );
  }

  lua_getglobal( L, "init" );
  if ( !lua_isfunction( L, -1 ) ) {
    printf( "Can't call init()! This is bad!\n" );
    printf( "Did you pass the right project directory???\n" );
    return -1;
  }

  if ( lua_pcall( L, 0, 0, 0 ) ) {
    printf( "LUA: %s\n", lua_tostring( L, -1 ) );
  }

  return 0;
}

void
newlib_process_events ( void )
{
  keyboard_state = SDL_GetKeyboardState( NULL );
  mouse_state.mask = SDL_GetMouseState( &mouse_state.x, &mouse_state.y );
}

void
newlib_update_timers ( void )
{
  if ( !timer_list.begin ) {
    return;
  }

  for ( nl_timer_node* t = timer_list.begin; ; t = t->next ) {
    nl_timer* timer = t->timer;
    if ( !timer->active ) {
      if ( !t->next ) break;
      else continue;
    }

    Uint32 ticks = SDL_GetTicks();
    Uint32 now = ticks - timer->start;

    //printf( "%d\n", now );
    if ( now >= timer->duration ) {
      timer_complete( timer );
    }

    if ( !t->next ) break;
  }
}
