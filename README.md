# newlib

Newlib is a tiny, tiny game development framework that's built with SDL2 and OpenGL 3. 

Games built with newlib use Lua as an implementation language. It supports some graphics and input operations. It has a timer implementation, too.

## Peering in

Peek into these directories to see some small example games/demos:

- **cic/**
- **plat/**
- **aw/**

A basic project template is in **blank/**.

## Launch a demo

newlib games are launched by passing project directories in as a command-line argument. For example, to play the demo in plat/, type the following into your terminal:

`$ newlib plat`

and newlib will load the game from *main.lua*.
