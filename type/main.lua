if platform == "NATIVE" then
  require('mobdebug').start()
end

local buffer = B""
local cursor = 1
local default_fb = nil
local canvas = nil
local angle = 0

local WINDOW_WIDTH = 960
local WINDOW_HEIGHT = 720

local GAME_WIDTH = 320
local GAME_HEIGHT = 240

local SCALE_X = WINDOW_WIDTH / GAME_WIDTH
local SCALE_Y = WINDOW_HEIGHT / GAME_HEIGHT

function init ()
   font = load_font( "assets/Inconsolata-Regular.ttf", 18 )

   default_fb = get_render_target()
   canvas = new_canvas( GAME_WIDTH, GAME_HEIGHT )
end

function update ()
end

function key_pressed ( key, scancode )
   if scancode == KEY_RETURN then
      if buffer:len() > 0 then
         --[[local script = load( tostring( buffer ) )
            script()--]]
         buffer:set( cursor, "\n" )
         cursor = cursor + 1

         print( cursor )
      end
   end
   if scancode == KEY_BACKSPACE then
      if buffer:len() > 0 then
         if cursor - 1 >= 1 then
            cursor = cursor - 1
            print( "deleting " .. buffer[cursor] )
            buffer:set( cursor, " " )

            print( cursor )
         end
      end
   end
end

function text_input ( text )
   buffer:set( cursor, text )
   cursor = cursor + string.len( text )

   print( cursor )
end

function draw ()
   set_render_target( canvas )
   render_clear()

   local size = 8
   for y=0, GAME_HEIGHT, size do
      for x=0, GAME_WIDTH, size do
         v = math.random( 0, 255 )

         draw_color( v, v, v*255 )
         draw_rect( x, y, size, size )
      end
   end

   angle = angle + 0.1
   if buffer:len() ~= 0 then
      draw_text( font, string.format( "%s", buffer ) , 0, 0 )
   end

   set_render_target( default_fb )

   draw_canvas( canvas, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0 )
   draw_text( font, "testing", 0, WINDOW_HEIGHT - 18 )
end
