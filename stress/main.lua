function init()
end

function update()
end

function draw()
   for y=0, 100, 1 do
      for x=0, 100, 1 do
         local v = math.random( 0, 255 )
         
         draw_color( v/3, v/6, v/8 )
         draw_rect( x*16, y*16, 16, 16 )
      end
   end
end
