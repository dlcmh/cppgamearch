# cppgamearch

Tech stack:

- macOS Tahoe 26 or newer
- Metal-cpp
- C++23 or newer
- Slang shading language
- SDL3 (Simple DirectMedia Layer) - handles macOS-specific details of creating a window and pumping events, and gives us a proper event loop to build on.
- OpenGL Mathematics (GLM)
- CMake
  - CMakeLists.txt

Setup:

- `brew install sdl3`, verify with `pkg-config --modversion sdl3` and `ls "$(brew --prefix sdl3)/include/SDL3/SDL.h"`.