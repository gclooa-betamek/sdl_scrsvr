# A simple SDL3 screensaver.
Requires SDL3. Clone SDL3 and SDL3 TTF extension into project root, file structure should look like this:
```
vendored/SDL
vendored/SDL_ttf
```
To build, run:
```
cmake -S . -B build -DSDLIMAGE_VENDORED=OFF
cmake --build build
```