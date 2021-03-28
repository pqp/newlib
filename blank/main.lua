local fill = false
local angle = 0

local GAME_WIDTH = 320
local GAME_HEIGHT = 240

function init ()
   p = load_image( "p.png" )
   font = load_font( "Hippauf.ttf", 12 )

   default_framebuffer = get_render_target()
   game_framebuffer = new_canvas( GAME_WIDTH, GAME_HEIGHT )

   timer = new_timer( 100 )
   timer.on_complete =
      function()
         print( "Timer complete!" )
      end

   timer:start()
end

function update ()
   angle = angle + 0.1
end

function key_pressed ( key, scancode )
   if scancode == KEY_RETURN then
      fill = not fill
   end
   if scancode == KEY_SPACE then
   end
end

function draw()
   set_render_target( game_framebuffer )

   render_clear()

   draw_color( 0, 0, 255 )

   if fill then
      fill_rect( 0, 0, 64, 64 )
   else
      draw_rect( 0, 0, 64, 64 )
   end

   draw_text( font, "Hello, world!", GAME_WIDTH / 3, GAME_HEIGHT / 3 )
   draw_image( p, 128, 128, 64, 64, angle )

   set_render_target( default_framebuffer )

   --draw_canvas( game_framebuffer, 0, 0, 960, 720, 0 )
   draw_canvas( game_framebuffer, 0, 0, 960, 720, 0 )
end
