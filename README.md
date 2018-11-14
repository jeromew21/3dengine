Install SDL2 (usually already installed), SDL_ttf (you may have to build from 
source, 
on Arch Linux this is a tremendous pain but I somehow managed to do it. Using a 
different distro it should be much easier).

Compile with something like (make sure you have `pkg-config` installed, it's very 
useful):

```
gcc engine.c $(pkg-config --cflags --libs sdl2) -lSDL2_ttf -lm -o engine
```

I have no clue how to do this on Windows but it shouldn't be too hard.
