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

### Setup:

Install SDL3 with:

```sh
brew install sdl3
```

Verify with `pkg-config --modversion sdl3` and `ls "$(brew --prefix sdl3)/include/SDL3/SDL.h"`.


Bring in Metal-cpp "pinned to the macOS 26.4 release to match your platform baseline":

```sh
# third_party subfolder gets created:
git clone --depth 1 \
  --branch release/metal-cpp_macOS26.4_iOS26.4 \
  https://github.com/apple/metal-cpp.git \
  third_party/metal-cpp
```

Verify with `ls third_party/metal-cpp/Metal/Metal.hpp`.


After setting up a game's folder with `CMakeLists.txt`, do the following one-time vscode-related setup.

```sh
# run this if build folder deleted or compile_commands.json needs to regenerated
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

Build and run a specific target (eg sdl3window):

```sh
cmake --build build --target sdl3window
build/sdl3window
```