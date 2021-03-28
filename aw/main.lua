--require('mobdebug').start()

local default_fb = nil
local game_fb = nil

local GAME_WIDTH = 320
local GAME_HEIGHT = 240

local TILE_WIDTH = 16
local TILE_HEIGHT = 16

local BOARD_X = 64
local BOARD_Y = 16

local BOARD_WIDTH = 12
local BOARD_HEIGHT = 12

time = 0

Bomb = {}
bombs = {}

Explosion = {}
explosions = {}
explosion_map = {}

Tile = {
   value = 0,
   toggled = false
}

local rseed = os.time()

math.randomseed( rseed )
print( "seed: " .. rseed ) 

board = {}
for i=1,BOARD_WIDTH*BOARD_HEIGHT do
   local t = {
      value = math.random( 0, 1 ),
      toggled = false
   }
   table.insert( board, t )
end

board[44].value = -1

EXPLODE_TIME = 75

function Bomb:new( x, y )
   local o = table.copy( self )
   o.x = x
   o.y = y
   return o
end

function Bomb:init()
   self.timer = new_timer( EXPLODE_TIME, 1 )
   self.cur_exp = 0
   -- 13 = 1 + 12 (the bomb location + 12 others)
   self.max_exp = 13 

   self.timer.on_complete =
      function()
         self:spread( self.x, self.y )
      end
   self.timer:start()
end

function Explosion.new ( x, y )
   local o = table.copy( Explosion )
   o.x = x
   o.y = y
   return o
end

function Bomb:spread ( x, y )
   local tile_x = ( x - BOARD_X ) / TILE_WIDTH
   local tile_y = ( y - BOARD_Y ) / TILE_HEIGHT
   local idx = ( ( tile_y * BOARD_WIDTH ) + tile_x ) + 1
   local elem = board[idx]
   local val = elem.value
   if self.cur_exp >= self.max_exp then return end

   if tile_x < 0 then return end
   if tile_x >= BOARD_WIDTH then return end

   if tile_y < 0 then return end
   if tile_y >= BOARD_HEIGHT then return end

   if explosion_map[idx] == 1 then
      return
   end

   if board[idx].toggled then
      return
   end

   local e = Explosion.new( x, y )
   table.insert( explosions, e )
   print ( tile_x .. ", " .. tile_y .. " new" )
   explosion_map[idx] = 1
   self.cur_exp = self.cur_exp + 1

   print( self.max_exp ..  " -> " .. ( self.max_exp + val ) )
   self.max_exp = self.max_exp + val
   
   e.timer = new_timer( EXPLODE_TIME, 1 )
   e.timer.on_complete =
      function()
         self:spread( e.x, e.y + TILE_HEIGHT )
         self:spread( e.x, e.y - TILE_HEIGHT )
         self:spread( e.x + TILE_WIDTH, e.y )
         self:spread( e.x - TILE_WIDTH, e.y )

         delete_timer( e.timer )
      end
   e.timer:start()
   e:start()
end

function Explosion:start ()

end

function Explosion:remove ()
   delete_timer( self.timer )
   self.timer = nil
   self = nil
end

function init()
   default_fb = get_render_target()
   game_fb = new_canvas( GAME_WIDTH, GAME_HEIGHT )

   font = load_font( "Inconsolata-Regular.ttf", 12 )

   for y=0,BOARD_HEIGHT-1 do
      for x=0,BOARD_WIDTH-1 do
         local idx = ( ( y * BOARD_WIDTH ) + x ) + 1
         local elem = board[idx]
      
         if elem.value == -1 then
            local bomb = Bomb:new( ( BOARD_X + ( x*TILE_WIDTH ) ), ( BOARD_Y + ( y*TILE_HEIGHT ) ) )
            table.insert( bombs, bomb )
         end
      end  
   end
end

function key_pressed( key, scancode )
   if scancode == KEY_RETURN then
      local timer = new_timer( 1000, -1 )
      timer.on_complete =
         function()
            time = time + 1
         end
      timer:start()
   end
   if scancode == KEY_SPACE then
      explosions = {}
      explosion_map = {}
      bombs[1]:init()
   end
end

function update( mouse_x, mouse_y )
   mx = ( mouse_x / window.width ) * GAME_WIDTH
   my = ( mouse_y / window.height ) * GAME_HEIGHT

   editor_x = ( math.floor( mx / TILE_WIDTH ) ) * TILE_WIDTH
   editor_y = ( math.floor( my / TILE_HEIGHT ) ) * TILE_HEIGHT

   local tile_x = ( editor_x - BOARD_X ) / TILE_WIDTH
   local tile_y = ( editor_y - BOARD_Y ) / TILE_HEIGHT
   local eidx = ( ( tile_y * BOARD_WIDTH ) + tile_x ) + 1

   local MAX_TILES = ( BOARD_WIDTH * BOARD_HEIGHT )

   local elem = board[eidx]

   if mouse_down( 1 ) then
      if tile_x < 0 or tile_x >= BOARD_WIDTH then return end
      if tile_y < 0 or tile_y >= BOARD_HEIGHT then return end
   
      print ( "" .. eidx .. " toggled" )
      elem.toggled = true
   end
   if mouse_down( 3 ) then
      if eidx < 1 or eidx > MAX_TILES then return end

      elem.toggled = false
   end

end

function draw_board()
   for y=0, BOARD_HEIGHT - 1 do
      for x=0, BOARD_WIDTH - 1 do
         local idx = ( ( y * BOARD_WIDTH ) + x ) + 1
         local elem = board[idx]

         local tile_x = BOARD_X + ( x*TILE_WIDTH )
         local tile_y = BOARD_Y + ( y*TILE_HEIGHT )

         draw_text( font, "" .. elem.value, tile_x + ( TILE_WIDTH / 4 ), tile_y + ( TILE_HEIGHT / 4 ) )

         draw_color( 255, 255, 255 )
         if elem.toggled then
            draw_rect( tile_x, tile_y, TILE_WIDTH, TILE_HEIGHT )
         end
      end
   end
end

function draw()
   set_render_target( game_fb )
   
   render_clear()

   for i=1, #bombs do
      fill_rect( bombs[i].x, bombs[i].y, 16, 16 )
   end

   draw_color( 0, 0, 255 )
   for i=1, #explosions do
      local e = explosions[i]

      if e ~= nil then
         fill_rect( explosions[i].x, explosions[i].y, 16, 16 )
      end
   end

   draw_board()

   draw_text( font, "Time: " .. time, 8, GAME_HEIGHT - 24 )
   draw_text( font, "Spread: " .. #explosions, 8, GAME_HEIGHT - 12 )

   draw_color( 255, 255, 255 )
   draw_rect( editor_x, editor_y, TILE_WIDTH, TILE_HEIGHT )

   set_render_target( default_fb )

   draw_canvas( game_fb, 0, 0, window.width, window.height, 0 )
end

function table.copy( obj )
   if type(obj) ~= 'table' then
      return obj
   end
   
   local res = {}
   for k, v in pairs(obj) do
      res[table.copy(k)] = table.copy(v)
   end
   
   return res
end
