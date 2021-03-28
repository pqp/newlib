require( "common" )

local overworld_map = {
   [0]={ tile = 0 }, { tile = 0 }, { tile = 0 }, { tile = 0 }, { tile = 0 }, { tile = 0 },
   { tile = 0 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
   { tile = 0 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
   { tile = 0 }, { tile = 1 }, { tile = 2 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
   { tile = 0 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
   { tile = 0 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
   { tile = 0 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 }, { tile = 1 },
}

-- This is basically an offset from the origin of the tilemap.
local overworld_camera = {
   x = 0,
   y = 0
}

local OVERWORLD_MAP_WIDTH = 6
local OVERWORLD_MAP_HEIGHT = 7

local OVERWORLD_TILE_WIDTH = 48
local OVERWORLD_TILE_HEIGHT = 48

local player = {
   x = 0,
   y = 0,
   x_goal = 0,
   y_goal = 0
}

function ease( start, dest )
   value = start + ( dest - start ) * 0.1
   return value
end

function generate_map()
end

function init()
   default_framebuffer = get_render_target()
   game_framebuffer = new_canvas( GAME_WIDTH, GAME_HEIGHT )

   font = load_font( "assets/Hippauf.ttf", 12 )
   overlay_font = load_font( "assets/Hippauf.ttf", 22 )

   for y=0, OVERWORLD_MAP_HEIGHT-1, 1 do
      for x=0, OVERWORLD_MAP_WIDTH-1, 1 do
         local idx = ( y * OVERWORLD_MAP_WIDTH ) + x
         local elem = overworld_map[idx]

         if type( elem ) == "table" then
            if elem.tile == 2 then
               player.x = x
               player.y = y
            end
         end
      end
   end
end

function key_pressed( key, scancode )
   if scancode == KEY_LEFT then
      player.x = player.x - 1
   elseif scancode == KEY_RIGHT then
      player.x = player.x + 1
   elseif scancode == KEY_UP then
      player.y = player.y - 1
   elseif scancode == KEY_DOWN then
      player.y = player.y + 1
   elseif scancode == KEY_RETURN then
      dofile( "common.lua" )

      common.update = update
      common.key_pressed = key_pressed
      common.draw = draw

      dofile( "event.lua" )
   end
end

function update()
   local offset_x = OVERWORLD_TILE_WIDTH * 2
   local offset_y = OVERWORLD_TILE_HEIGHT * 2

   local player_world_x = player.x * OVERWORLD_TILE_WIDTH
   local player_world_y = player.y * OVERWORLD_TILE_HEIGHT

   -- Player should at least be able to see ONE tile ahead
   local left = ( player.x - 2 ) * OVERWORLD_TILE_WIDTH
   local right = ( ( player.x + 2 ) * OVERWORLD_TILE_WIDTH ) + OVERWORLD_TILE_WIDTH
   local top = ( player.y - 2 ) * OVERWORLD_TILE_HEIGHT
   local bottom = ( player.y + 2 ) * OVERWORLD_TILE_HEIGHT

   local camera_left = overworld_camera.x
   local camera_right = overworld_camera.x + GAME_WIDTH
   local camera_top = overworld_camera.y
   local camera_bottom = overworld_camera.y + GAME_HEIGHT

   if left < camera_left then
      overworld_camera.x = overworld_camera.x - offset_x
      player.x_goal = player.x_goal + offset_x
   elseif right > camera_right then
      overworld_camera.x = overworld_camera.x + offset_x
      player.x_goal = player.x_goal - offset_x
   end

   if top < camera_top then
      overworld_camera.y = overworld_camera.y - offset_y
      player.y_goal = player.y_goal + offset_x
   elseif bottom > camera_bottom then
      overworld_camera.y = overworld_camera.y + offset_y
      player.y_goal = player.y_goal - offset_y
   end

   player.x_goal = ease( player.x_goal, ( player.x * OVERWORLD_TILE_WIDTH ) - overworld_camera.x )
   player.y_goal = ease( player.y_goal, ( player.y * OVERWORLD_TILE_HEIGHT ) - overworld_camera.y )
end

function draw()
   set_render_target( game_framebuffer )

   render_clear()

   for y=0, OVERWORLD_MAP_HEIGHT-1, 1 do
      for x=0, OVERWORLD_MAP_WIDTH-1, 1 do
         local idx = ( y * OVERWORLD_MAP_WIDTH ) + x
         local elem = overworld_map[idx]

         if elem.tile == 2 then
            local v = math.random( 0, 255 )
            draw_color( 0, v/6, v/3 )
         else
            local v = math.random( 0, 255 )
            draw_color( v, v/2, v/3 )
         end
         
         if overworld_map[idx].tile ~= 0 then
            fill_rect( ( x*OVERWORLD_TILE_WIDTH ) - overworld_camera.x, ( y*OVERWORLD_TILE_HEIGHT ) - overworld_camera.y, OVERWORLD_TILE_WIDTH, OVERWORLD_TILE_HEIGHT )
         end
      end
   end

   draw_color( 255, 255, 255 )
   draw_rect( player.x_goal, player.y_goal, OVERWORLD_TILE_WIDTH, OVERWORLD_TILE_HEIGHT )

   draw_text( font, "X: " .. player.x .. ", Y: " .. player.y, 0, 0 )
   draw_text( font, "DX: " .. player.x_goal .. ", DY: " .. player.y_goal, 0, 12 )
   draw_text( font, "CX: " .. overworld_camera.x .. ", CY: " .. overworld_camera.y, 0, 24 )
   
   draw_text( font, "Hello, world.", 16, GAME_HEIGHT - 40 )

   set_render_target( default_framebuffer )
   draw_canvas( game_framebuffer, 0, 0, 960, 720, 0 )

   
end
