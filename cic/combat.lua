local LS_X = nil
local LS_Y = nil

local RS_X = nil
local RS_Y = nil

local Hand = {}

function Hand.new ( x, y )
   local o = {
      anchor = {
         x = x,
         y = y
      },
      x = 0,
      y = 0,
      accel = {
         x = 0,
         y = 0
      },
      vel = {
         x = 0,
         y = 0
      },
      thrust = false
   }

   return o
end

local Opponent = {}

function Opponent.new( x, y )
   local o = {
      left_hand = Hand.new( x, y ),
      head = { x = x + 53, y = y },
      right_hand = Hand.new( x + 130, y )
   }

   return o
end

local player = Opponent.new( GAME_WIDTH / 4, 175 )
local enemy = Opponent.new( GAME_WIDTH / 4, 20 )

local MAX_VELOCITY = 400
local AXIS_MAX = 32767
local DEAD_ZONE = 0.20

function init()
end

function key_pressed( key, scancode )
   if scancode == KEY_RETURN then
      update = common.update
      key_pressed = common.key_pressed
      draw = common.draw
   end
end

function update()
   LS_X = controller_get_axis( CONTROLLER_AXIS_LEFTX ) / AXIS_MAX;
   LS_Y = controller_get_axis( CONTROLLER_AXIS_LEFTY ) / AXIS_MAX;

   RS_X = controller_get_axis( CONTROLLER_AXIS_RIGHTX ) / AXIS_MAX;
   RS_Y = controller_get_axis( CONTROLLER_AXIS_RIGHTY ) / AXIS_MAX;

   local a_pushed = controller_get_button( CONTROLLER_BUTTON_A );
   if a_pushed then
      player.left_hand.accel.x = LS_X * ( 200 * dt )
      player.left_hand.accel.y = LS_Y * ( 200 * dt )

      player.left_hand.thrust = true
   end

   if player.left_hand.thrust ~= true then
      if LS_X < -DEAD_ZONE or LS_X > DEAD_ZONE then
         player.left_hand.x = player.left_hand.anchor.x + ( LS_X * ( 2400 * dt ) )
      else
         player.left_hand.x = player.left_hand.anchor.x
      end

      if LS_Y < -DEAD_ZONE or LS_Y > DEAD_ZONE then
         player.left_hand.y = player.left_hand.anchor.y + ( LS_Y * ( 2400 * dt ) )
      else
         player.left_hand.y = player.left_hand.anchor.y
      end
   end

   if RS_X < -DEAD_ZONE or RS_X > DEAD_ZONE then
      player.right_hand.x = player.right_hand.anchor.x + ( RS_X * ( 2400 * dt ) )
   else
      player.right_hand.x = player.right_hand.anchor.x
   end

   if RS_Y < -DEAD_ZONE or RS_Y > DEAD_ZONE then
      player.right_hand.y = player.right_hand.anchor.y + ( RS_Y * ( 2400 * dt ) )
   else
      player.right_hand.y = player.right_hand.anchor.y
   end

   player.left_hand.vel.x = player.left_hand.vel.x + player.left_hand.accel.x
   player.left_hand.vel.y = player.left_hand.vel.y + player.left_hand.accel.y

   if player.left_hand.thrust then
      if player.left_hand.vel.x > MAX_VELOCITY then
         player.left_hand.vel.x = MAX_VELOCITY
      end

      if player.left_hand.vel.y > MAX_VELOCITY then
         player.left_hand.vel.y = MAX_VELOCITY
      end
   end

   player.left_hand.x = player.left_hand.x + player.left_hand.vel.x
   player.left_hand.y = player.left_hand.y + player.left_hand.vel.y

   -- Is hand out of bounds
   if player.left_hand.x <= 0 or player.left_hand.x >= GAME_WIDTH or
      player.left_hand.y <= 0 or player.left_hand.y >= GAME_HEIGHT then
      player.left_hand.thrust = false

      player.left_hand.accel.x = 0
      player.left_hand.accel.y = 0

      player.left_hand.vel.x = 0
      player.left_hand.vel.y = 0
   end

   enemy.left_hand.x = enemy.left_hand.anchor.x
   enemy.left_hand.y = enemy.left_hand.anchor.y

   enemy.right_hand.x = enemy.right_hand.anchor.x
   enemy.right_hand.y = enemy.right_hand.anchor.y
end

function draw()
   set_render_target( game_framebuffer )

   render_clear()
   
   draw_color( 255, 0, 255 )

   fill_rect( player.left_hand.x, player.left_hand.y, 24, 24 )
   fill_rect( player.right_hand.x, player.right_hand.y, 24, 24 )
   fill_rect( player.head.x, player.head.y, 48, 48 )

   fill_rect( enemy.left_hand.x, enemy.left_hand.y, 24, 24 )
   fill_rect( enemy.right_hand.x, enemy.right_hand.y, 24, 24 )
   fill_rect( enemy.head.x, enemy.head.y, 48, 48 )

   set_render_target( default_framebuffer )

   draw_image( game_framebuffer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0 )
   draw_text( font, "L Thumb X: " .. LS_X, 0, 0 )
   draw_text( font, "L Thumb Y: " .. LS_Y, 0, 12 )

   draw_text( font, "R Thumb X: " .. RS_X, 0, 36 )
   draw_text( font, "R Thumb Y: " .. RS_Y, 0, 48 )

   draw_text( font, "Left X: " .. player.left_hand.x, 0, 64 )
   draw_text( font, "Left Y: " .. player.left_hand.y, 0, 76 )

   draw_text( font, "Left Vel_X: " .. player.left_hand.vel.x, 0, 100 )
   draw_text( font, "Left Vel_Y: " .. player.left_hand.vel.y, 0, 112 )
end
