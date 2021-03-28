local text = [[
In the clearing ahead you see a person standing over a ragged corpse. They turn towards you.

1) In your presence they lower their weapons: they are not interested in hurting you. They step closer to speak.

"This man--the wretch--tried to steal food from me. 
He left me no choice; I struck him down."

The silence gives way to the neon stream flitting and lapping over the rocks. The <person> steps aside to let you pass.
]]

function update()
end

function key_pressed( key, scancode )
   if scancode == KEY_RETURN then
      update = common.update
      key_pressed = common.key_pressed
      draw = common.draw
   end
end

function draw()
   render_clear()

   draw_color( 128, 128, 255 )

   local rect = {}
   rect.x = 45
   rect.y = 45
   rect.w = WINDOW_WIDTH / 1.1
   rect.h = WINDOW_HEIGHT / 1.5
   
   fill_rect( rect.x, rect.y, rect.w, rect.h )

   draw_text( overlay_font, text, rect.x + 20, rect.y + 20, rect.x + ( rect.w - 20 ) )
end
