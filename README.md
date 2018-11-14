Install SDL2, SDL_ttf (may have to build from source, on Arch Linux this is a 
pain).

Compile with

```
gcc engine.c $(pkg-config --cflags --libs sdl2) -lSDL2_ttf -lm -o engine
```
