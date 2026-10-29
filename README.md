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

In `.zshrc`:

```sh
alias clang++='clang++ -std=c++23'
```

Install **SDL3** with:

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
# run this if:
# build folder deleted
# compile_commands.json needs to regenerated
# a new executable is added (which needs compile_commands.json to be regenerated)
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

`clang-tidy` enforces modern C++ and Core Guidelines. Create `.clang-tidy` file in project root:

```yaml
Checks: >
  -*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -cppcoreguidelines-avoid-magic-numbers,
  -readability-magic-numbers,
  -readability-identifier-length,
  -modernize-use-designated-initializers

WarningsAsErrors: >
  cppcoreguidelines-owning-memory,
  modernize-make-unique,
  modernize-make-shared,
  modernize-use-using
```

`clang-format` formats code automatically with a consistent style. First, `brew install clang-format`. Next, create `.clang-format` file in project root:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 80
DerivePointerAlignment: false
PointerAlignment: Left
```

And in `.vscode/settings.json`, add:

```json
"clangd.arguments": [
    "--compile-commands-dir=build"
],
"[cpp]": {
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",
    "editor.formatOnSave": true
},
"[c]": {
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",
    "editor.formatOnSave": true
},
```

- `editor.defaultFormatter`: This tells VSCode to use the clangd extension as the formatter for C and C++ files. Clangd uses the `clang-format` engine, which respects your project's `.clang-format` file.
- `editor.formatOnSave`: This triggers the formatter automatically whenever you press `Cmd+S`.


**Build and run a specific target (eg sdl3window):**

```sh
# if new executable added
cmake -B build && cmake --build build --target sdl3window

# if re-compiling an existing executable
cmake --build build --target sdl3window

build/sdl3window
```
