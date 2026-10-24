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
- vscode (Visual Studio Code) with extensions:
  - clangd
  - Microsoft C++ Intellisense installed but disabled in favor of clangd
  - CMake Tools
  - CodeLLDB
  - LLDB DAP
  - Slang

Setup:

`brew install sdl3`, verify with `pkg-config --modversion sdl3` and `ls "$(brew --prefix sdl3)/include/SDL3/SDL.h"`.

After setting up a game's folder with `CMakeLists.txt`, do the following one-time vscode-related setup.

```
cd gamefolder

cmake -B build
```

In `.vscode/settings.json`:

```json
{
    "clangd.arguments": [
        "--compile-commands-dir=build"
    ]
}
```