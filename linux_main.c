#include "newlib.h"

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

const int FPS = 60;

void
newlib_main ( void )
{
  lua_pushnumber( L, 1/FPS );
  lua_setglobal( L, "dt" );

  lua_pushstring( L, "NATIVE" );
  lua_setglobal( L, "platform" );

  bool running = true;
  SDL_Event event;
  while ( running )
    {
      float begin = SDL_GetTicks();

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

      if ( crashed ) {
        newlib_lua_error();

        SDL_GL_SwapWindow( window );
        continue;
      }

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

      assert( glGetError() == GL_NO_ERROR );

      SDL_GL_SwapWindow( window );

      float dt = ( SDL_GetTicks() - begin ) / 1000;

      lua_pushnumber( L, dt );
      lua_setglobal( L, "dt" );
    }

    lua_close( L );
}

int
main ( int argc, char* argv[] )
{
  int err = newlib_init( argc, argv );

  if ( err < 0 ) {
    printf( "Bailing out of main loop\n" );
    return 0;
  }
  newlib_main();
}
