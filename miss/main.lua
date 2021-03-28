local GAME_WIDTH = 320
local GAME_HEIGHT = 240

local WINDOW_WIDTH = 960
local WINDOW_HEIGHT = 720

local default_fb = nil
local game_fb = nil

local mx = nil
local my = nil

function init ()
   default_fb = get_render_target( default_fb )
   game_fb = new_canvas( GAME_WIDTH, GAME_HEIGHT )
end

function key_pressed ( key, scancode )
end

function change_color ()
   draw_color( 255, 255, 65 )
end

function update ( mouse_x, mouse_y )
   draw_color( 64, 64, 255 )
   
   mx = ( mouse_x / WINDOW_WIDTH ) * GAME_WIDTH
   my = ( mouse_y / WINDOW_HEIGHT ) * GAME_HEIGHT

   if mouse_down( 1 ) then
      --[[
         local timer = new_timer( 100 )
         timer:on_complete( change_color )
         timer:start()
      --]]
   elseif mouse_down( 2 ) then
      draw_color( 255, 0, 65 )
   elseif mouse_down( 3 ) then
      draw_color( 255, 128, 0 )
   end
end

function draw ()
   set_render_target( game_fb )

   render_clear()

   fill_rect( mx - 8, my - 8, 16, 16 )

   set_render_target( default_fb )

   draw_canvas( game_fb, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0 )
end
