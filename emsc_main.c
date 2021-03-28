#include "newlib.h"

#include <emscripten.h>

#include <stdio.h>
#include <stdbool.h>

#define FPS 60

bool running = true;

void
newlib_main ( void )
{
  SDL_Event event;
  if ( running )
    {
      double start = SDL_GetTicks();

      while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          lua_getglobal( L, "key_pressed" );
          lua_pushnumber( L, event.key.keysym.sym );
          lua_pushnumber( L, event.key.keysym.scancode );

          if ( lua_pcall( L, 2, 0, 0 ) ) {
            newlib_lua_error();
          }
          break;
        case SDL_TEXTINPUT:
          lua_getglobal( L, "text_input" );
          lua_pushstring( L, event.text.text );
          
          if ( lua_pcall( L, 1, 0, 0 ) ) {
            //newlib_lua_error();
          }
          break;
        }

      }

      newlib_process_events();
      newlib_update_timers();

      lua_pushnumber( L, (double)( 1000 / FPS ) / 1000 );
      lua_setglobal( L, "dt" );

      lua_getglobal( L, "update" );
      lua_pushnumber( L, mouse_state.x );
      lua_pushnumber( L, mouse_state.y );
      if ( lua_pcall( L, 2, 0, 0 ) ) {
        newlib_lua_error();
      }

      glClearColor( 0.f, 0.f, 0.f, 1.f );
      
      lua_getglobal( L, "draw" );
      if ( lua_pcall( L, 0, 0, 0 ) ) {
        newlib_lua_error();
      }

      SDL_GL_SwapWindow( window );

      double delay = start + ( 1 / 60 ) - SDL_GetTicks();
      if ( delay > 0 ) {
        SDL_Delay( delay );
        printf( "delayed\n" );
      }
    }
  else {
    lua_close( L );
  }
}

int
main ( int argc, char* argv[] )
{
  newlib_init( argc, argv );

  lua_pushstring( L, "WEB" );
  lua_setglobal( L, "platform" );

  emscripten_set_main_loop( newlib_main, 0, 1 );
  emscripten_set_main_loop_timing( EM_TIMING_RAF, 1 );
}
