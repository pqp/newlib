if platform == "NATIVE" then
  require('mobdebug').start()
end

local map1 = require "assets/map1"

SCREEN_WIDTH = 960
SCREEN_HEIGHT = 720

GAME_WIDTH = 320
GAME_HEIGHT = 240

SCALE_X = SCREEN_WIDTH / GAME_WIDTH
SCALE_Y = SCREEN_HEIGHT / GAME_HEIGHT

TILE_WIDTH = 16
TILE_HEIGHT = 16

MAX_VELOCITY_X = 200
MAX_VELOCITY_Y = 200

local player = {
   x = 24,
   y = 16,
   w = 16,
   h = 24,
   
   vel_x = 0,
   vel_y = 0,
   accel_x = 2400,
   accel_y = 800
}

local default_fb = nil
local canvas = nil

jumping = false

function ease( start, dest )
   value = start + ( dest - start ) * 0.1
   return value
end

function init()
   canvas = new_canvas( GAME_WIDTH, GAME_HEIGHT )
   default_fb = get_render_target()
   
   local map_width = map1.layers[1].width
   local map_height = map1.layers[1].height

   font = load_font( "assets/slkscr.ttf", 8 )
   
   rects = {}
   local i = 0
   for y=1, map_height, 1 do
      for x=1, map_width, 1 do
         if map1.layers[1].data[y*map_width + x] == 1 then
            rects[i] = {}
            rects[i].x = ( x*TILE_WIDTH )
            rects[i].y = ( y*TILE_HEIGHT )
            rects[i].w = TILE_WIDTH
            rects[i].h = TILE_HEIGHT
            print( "rect " .. i .. " at " .. rects[i].x .. ", " .. rects[i].y )
            i = i + 1
         end
      end
   end
end

function key_pressed( key, scancode )
  if jumping ~= true then
     if scancode == KEY_SPACE then
        jumping = true
        player.vel_y = -200
     end
  end
end

function update()
  if key_down( KEY_LEFT ) == true then
     player.vel_x = player.vel_x - ( ( player.accel_x * dt ) * 1 )
     if player.vel_x < -MAX_VELOCITY_X then
        player.vel_x = -MAX_VELOCITY_X
    end
  end
  if key_down( KEY_RIGHT ) == true then
     player.vel_x = player.vel_x + ( ( player.accel_x * dt ) * 1 )
     if player.vel_x > MAX_VELOCITY_X then
        player.vel_x = MAX_VELOCITY_X
    end
  end
  
  if key_down( KEY_LEFT ) ~= true and key_down( KEY_RIGHT ) ~= true then
     
     if player.vel_x < 0 then
        player.vel_x = player.vel_x + ( ( player.accel_x * dt ) * 1.5 )
        
        if player.vel_x > 0 then
           player.vel_x = 0
        end
     end
     if player.vel_x > 0 then
        player.vel_x = player.vel_x - ( ( player.accel_x * dt ) * 1.5 )
        
        if player.vel_x < 0 then
           player.vel_x = 0
        end
     end
     
  end
  -- Apply velocity to position
  player.x = math.floor( player.x + ( player.vel_x * dt ) )
  
  for i=0, #rects, 1 do
     if rect_intersect( player, rects[i] ) == true then
        
        local leftA = player.x
        local rightA = player.x + player.w
        local leftB = rects[i].x
        local rightB = rects[i].x + rects[i].w
        
        if player.vel_x > 0 then
           local delta = rightA - leftB
           player.x = math.floor( player.x - delta )
        end
        if player.vel_x < 0 then
           local delta = rightB - leftA
           player.x = math.floor( player.x + delta )
        end
        
        player.vel_x = 0
     end
  end
  
  player.vel_y = player.vel_y + ( player.accel_y * dt )
  player.y = math.floor( player.y + ( player.vel_y * dt ) )
  
  for i=0, #rects, 1 do
     if rect_intersect( player, rects[i] ) == true then
        
        local topA = player.y
        local bottomA = player.y + player.h
        local topB = rects[i].y
        local bottomB = rects[i].y + rects[i].h

        if player.vel_y > 0 then
           local diff = bottomA - topB
           player.y = player.y - diff
           jumping = false
        elseif player.vel_y < 0 then
           local diff = bottomB - topA
           player.y = player.y + diff
        end
        
        player.vel_y = 0
     end
  end

end

function draw_tilemap()
   local map_width = map1.layers[1].width
   local map_height = map1.layers[1].height
   
   for y=1, map_height, 1 do
      for x=1, map_width, 1 do
         if map1.layers[1].data[y*map_width + x] == 1 then
            draw_color( 148, 64, 64 )
            fill_rect( x*TILE_WIDTH, y*TILE_HEIGHT, 16, 16 )
         end
      end
   end
end

function draw()
   set_render_target( canvas )
   
   draw_color( 128, 128, 255 )
   fill_rect( 0, 0, GAME_WIDTH, GAME_HEIGHT )

   draw_color( 255, 255, 255 )
   fill_rect( player.x, player.y, player.w, player.h )
   
   draw_tilemap()
   
   set_render_target( default_fb )

   draw_canvas( canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0 )
  --[[draw_text( font, "X: " .. player.x .. " Y: " .. player.y, 0, 0 )
     draw_text( font, "vel_X: " .. player.vel_x .. " vel_Y: " .. player.vel_y, 0, 16 )
     draw_text( font, "accel_X: " .. player.accel_x .. " accel_Y: " .. player.accel_y, 0, 32 )
     draw_image( image, player.x, player.y, player.w, 0 )--]]
end
