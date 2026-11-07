# Chapter 1: A Window Into the Maze

!!! abstract "Chapter goal"
    Open an 800×600 window, paint it a deep navy, keep it responsive, and close it cleanly when the player presses Esc or clicks the close button.

    This is the skeleton every later chapter plugs into. By the end, you will understand every line that runs before the first frame.

Every game, no matter how large, starts with the same three questions:

1. Where do the pixels go?
2. When do I update them?
3. How does the player get out?

SDL3 answers the first question, a game loop answers the second, and an event answers the third. We will build all three in this chapter. No maze yet, no Pac-Man yet — just a window that behaves itself.

This chapter takes about 30 minutes, not counting the tool installation.

## 1. Install the tools

You need four Homebrew packages:

| Package | Why you need it |
| ------- | --------------- |
| `llvm` | Provides a modern `clang++` that understands C++23 |
| `cmake` | Generates the build and drives the compiler |
| `sdl3` | The window, input, and drawing library |
| `pkg-config` | Lets us verify that SDL3 is installed and findable |

Install them together:

~~~bash
brew install llvm cmake sdl3 pkg-config
~~~

If you already have some of them, Homebrew will say so and move on.

!!! note "Why Homebrew LLVM instead of Apple clang?"
    Apple's command-line compiler is usually a little behind the newest C++ standard. This project explicitly selects Homebrew's LLVM before CMake configures the build, so you get the same compiler on every machine. You do not need to change your shell or create a `clang++` alias. The root `CMakeLists.txt` does it for you.

    On Apple Silicon, Homebrew lives under `/opt/homebrew`; on Intel Macs, under `/usr/local`. The project checks both locations. In the commands below we ask Homebrew for the path so the same command works on either machine.

### Verify the installation

Run these from any directory:

~~~bash
"$(brew --prefix llvm)/bin/clang++" --version
cmake --version
pkg-config --modversion sdl3
~~~

You should see something like this:

~~~text
Homebrew clang version 23.1.4
Target: arm64-apple-darwin25.6.0
cmake version 4.4.4
3.4.18
~~~

The exact numbers do not matter. What matters is that `clang++` identifies itself as a Homebrew build and that `pkg-config` prints a 3.x version of SDL3. If your Mac is Intel, the target line will say `x86_64` instead of `arm64`.

!!! warning "If the commands fail"
    - **`brew: command not found`** — install Homebrew from [brew.sh](https://brew.sh/) first.
    - **`xcrun: error: invalid active developer path`** — install the Xcode command-line tools with `xcode-select --install`. Homebrew needs them to build and link macOS programs.
    - **`pkg-config: command not found`** — run `brew install pkg-config`.
    - **No SDL3 version** — run `brew install sdl3` and try again.

## 2. One project, many targets

This tutorial lives in the `cppgamearch` repository. The important parts look like this:

~~~text
cppgamearch/
├── CMakeLists.txt          ← the build rules for everything
├── include/                ← small shared headers
└── apps/
    ├── tetris/
    │   └── main.cpp
    └── pacman/
        ├── main.cpp        ← the game you are writing
        ├── AGENTS.md
        └── tutorial/       ← the site you are reading
~~~

The repository builds several small programs. Each one is a **CMake target**: a named executable with its own source files and libraries. Pac-Man will be the `pacman` target.

CMake is not a compiler. It reads `CMakeLists.txt`, works out the compiler commands, and writes them into a build directory. Then `cmake --build` runs those commands. This two-step design is why installing a new library sometimes means re-running the configure step.

### The two lines that matter

The root `CMakeLists.txt` already has the compiler selection, the C++23 standard, warning flags, and the SDL3 discovery. To add Pac-Man, the only new rules are:

~~~cmake
add_executable(pacman apps/pacman/main.cpp)
target_link_libraries(pacman PRIVATE SDL3::SDL3)
~~~

`add_executable` says: "Build an executable called `pacman` from this source file."

`target_link_libraries` says: "When linking `pacman`, use SDL3." `SDL3::SDL3` is an **imported CMake target**. It carries the include path, the library path, and any macOS frameworks SDL needs. You do not have to write `-I/opt/homebrew/include` or `-L/opt/homebrew/lib` by hand.

!!! note "Why doesn't Pac-Man add the Metal frameworks?"
    The `tetris` target adds Metal because that app experiments with shaders. Pac-Man uses SDL's 2D renderer, which already talks to the GPU for us. We link only `SDL3::SDL3`; SDL's own library pulls in whatever macOS frameworks it needs internally, so you do not have to add them to the `pacman` target.

The repository does not ship a Pac-Man program, so it does not ship this target either. You add both the source file and the two lines above. Put the target after the `tetris` target in the root `CMakeLists.txt`. Create `apps/pacman/main.cpp` before you configure, or CMake will complain that the source file is missing.

Let's create that file and fill it in.

## 3. The life of an SDL program

Before typing code, it helps to see the shape of every SDL game:

~~~text
SDL_Init
  └── create a window          ← the rectangle macOS puts on screen
       └── create a renderer   ← the 2D drawing surface inside it
            └── repeat once per frame:
                 ├── poll events        keyboard, mouse, close button
                 ├── clear the screen
                 ├── draw the game      (empty for now)
                 └── present the frame
  └── destroy the renderer
  └── destroy the window
SDL_Quit
~~~

A few terms worth knowing now:

**Window.** The operating system's rectangle, title bar, and close button. You can have a window without a renderer, but you cannot draw in it without one.

**Renderer.** SDL's 2D drawing context. It knows how to fill rectangles, draw lines, and present a finished frame. On macOS it uses a GPU API internally, but that is an implementation detail we never touch.

**Event.** A message from the operating system. "A key went down", "the mouse moved", "the user clicked the red close button". SDL places events in a queue; we take them off one at a time.

**Frame.** One trip around the game loop. A typical game shows 60 or 120 of them per second.

**VSync.** "Wait for the display to be ready before showing the next frame." It prevents the game from running thousands of frames per second, wasting battery and spinning the fan.

SDL is a C library. It creates resources as raw pointers and expects you to call the matching destroy function. In modern C++ we can do better: tie each resource to an object whose destructor cleans it up. That pattern is called **RAII** — Resource Acquisition Is Initialization. The rest of this chapter uses it so that every `return`, including the error paths, cleans up correctly.

## 4. Writing `main.cpp`

Create a new file at `apps/pacman/main.cpp` and open it in your editor. We will build the file in five pieces.

### 4.1 Includes and constants

Start with the includes and the values that describe the window:

~~~cpp
// Pac-Man tutorial - Chapter 1: a window into the maze.

#include <SDL3/SDL.h>

#include <memory>

namespace config {
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr SDL_Color kBackground{12, 12, 30, 255};
} // namespace config
~~~

`<SDL3/SDL.h>` brings in every SDL function we need. The angle brackets mean "look in the compiler's system include paths"; CMake adds SDL3's include directory to that search path through the `SDL3::SDL3` target.

`<memory>` gives us `std::unique_ptr`, which appears in a moment.

The `config` namespace holds values that describe the game. Keeping them together beats scattering magic numbers through the file. `constexpr` means the compiler evaluates them at compile time and refuses to let anything modify them.

- `800 × 600` is a comfortable window size for the 28 × 21 tile maze we will draw in Chapter 2.
- `SDL_Color` stores red, green, blue, and alpha, each from 0 to 255. `{12, 12, 30, 255}` is a very dark navy, not quite black. Alpha 255 means fully opaque.
- The `k` prefix is a common C++ convention for constants. It makes them easy to spot in a function.

### 4.2 Own the SDL library

Initializing SDL is a resource acquisition: something must be released when we are done. Wrap it:

~~~cpp
namespace {

// Shut down SDL when this object goes out of scope.
class SdlGuard {
  public:
    SdlGuard() : initialized_(SDL_Init(SDL_INIT_VIDEO)) {
        if (!initialized_) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
        }
    }

    ~SdlGuard() {
        if (initialized_) {
            SDL_Quit();
        }
    }

    SdlGuard(const SdlGuard&) = delete;
    SdlGuard& operator=(const SdlGuard&) = delete;
    SdlGuard(SdlGuard&&) = delete;
    SdlGuard& operator=(SdlGuard&&) = delete;

    [[nodiscard]] bool initialized() const { return initialized_; }

  private:
    bool initialized_ = false;
};
~~~

Let's read it carefully.

The constructor's initializer list calls `SDL_Init(SDL_INIT_VIDEO)` and stores the result in `initialized_`. We ask for the **video subsystem** only; we are not using audio or game controllers yet. If initialization fails, we log SDL's explanation.

The destructor is the mirror image: if initialization succeeded, call `SDL_Quit`. The `if` matters. Calling `SDL_Quit` after a failed initialization would be trying to clean up something that was never set up. The flag records exactly what we acquired.

Copying and moving are deleted. Two `SdlGuard` objects would both think they own SDL and would both call `SDL_Quit`. Deleting the special member functions turns that bug into a compile-time error. This is the Rule of Five in action: if you write one of the destructor, copy constructor, copy assignment, move constructor, or move assignment, think about all five.

`[[nodiscard]]` asks the compiler to warn if we call `initialized()` and throw the answer away. We are about to use it to decide whether to continue, so ignoring it would be a bug.

!!! tip "What is RAII?"
    Acquire a resource in a constructor. Release it in the destructor. Then let scope do the cleanup — even when a function returns early. SDL itself is the first resource we manage this way; later we will reuse the idea for other SDL resources.

### 4.3 Teach `unique_ptr` how to destroy SDL objects

The SDL library is initialized, but we still need a window and a renderer. SDL hands those back as raw pointers. A plain `std::unique_ptr` would try to `delete` them, which is wrong: SDL has its own destroy functions.

A **deleter** is a tiny function object that tells `unique_ptr` what to call instead:

~~~cpp
struct WindowDeleter {
    void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
};

struct RendererDeleter {
    void operator()(SDL_Renderer* renderer) const {
        SDL_DestroyRenderer(renderer);
    }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

} // namespace
~~~

Now `WindowPtr` behaves like an owning pointer: when it goes out of scope, it calls `SDL_DestroyWindow` on whatever it holds. `RendererPtr` calls `SDL_DestroyRenderer`. No manual cleanup on error paths, no leaks when we add more `return` statements later.

The `operator()` syntax may look strange if you have not met function objects before. It makes an object callable like a function. `unique_ptr` stores the deleter and calls it with the raw pointer when the pointer is reset or destroyed.

When we create the window and renderer in `main`, we will declare them in this order:

~~~cpp
WindowPtr window{...};     // destroys last
RendererPtr renderer{...}; // destroys first
~~~

Local variables are destroyed in reverse order of construction. The renderer must be destroyed before the window it draws into, and the `SdlGuard` was constructed before both, so `SDL_Quit` runs last. That ordering is not an accident; it is why we declare the objects in this order.

### 4.4 Create the window and renderer

Now the program's entry point:

~~~cpp
int main() {
    SdlGuard sdl;
    if (!sdl.initialized()) {
        return 1;
    }

    WindowPtr window{SDL_CreateWindow("Pac-Man", config::kWindowWidth,
                                      config::kWindowHeight, 0)};
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    RendererPtr renderer{SDL_CreateRenderer(window.get(), nullptr)};
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return 1;
    }
~~~

The `SdlGuard` owns the SDL library for the whole of `main`. If `SDL_Init` failed, there is nothing else to do, so return a non-zero exit code.

`SDL_CreateWindow(title, width, height, flags)` creates the operating system window. The title appears in the title bar, and the `0` means "no special flags for now". Later, if we want a resizable or high-DPI window, this is where the flag goes.

`SDL_CreateRenderer(window.get(), nullptr)` creates the drawing context. Passing `nullptr` lets SDL pick a suitable renderer for the machine; we do not care whether it uses Metal or another backend. We pass `window.get()` because the renderer needs a window, but ownership stays with the `WindowPtr`. Calling `.get()` borrows the raw pointer without giving up ownership.

Both creation functions can fail. A window can fail to appear; a renderer can fail to initialize. We check the pointer, print SDL's error message, and return. Because the resources are owned by RAII objects, returning early is safe.

`SDL_Log` is SDL's logging function. It writes to the terminal, just like `std::cerr`, but works consistently on macOS, Windows, and Linux, including inside application bundles where standard output can be awkward. `SDL_GetError()` returns a human-readable description of the last SDL failure.

!!! note "Why not use `assert`?"
    An assertion aborts the program. A player cannot do anything with that. Logging the error and returning a non-zero exit code lets the shell know the program failed, and it is much friendlier when you are debugging. Use assertions for programmer mistakes, not for "the OS would not give me a window".

### 4.5 Keep the window alive

The window now exists, but a program that ends immediately would flash on screen and vanish. We need the game loop:

~~~cpp
    if (!SDL_SetRenderVSync(renderer.get(), 1)) {
        SDL_Log("VSync unavailable: %s", SDL_GetError());
    }

    bool running = true;
    SDL_Event event{};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer.get(), config::kBackground.r,
                               config::kBackground.g, config::kBackground.b,
                               config::kBackground.a);
        SDL_RenderClear(renderer.get());
        SDL_RenderPresent(renderer.get());
    }

    return 0;
}
~~~

This is the smallest useful game loop. Let's break it down.

**VSync.** `SDL_SetRenderVSync(renderer.get(), 1)` asks SDL to synchronize presentation with the display. The `1` means "on". If the renderer or driver does not support it, we print a warning but keep going. A game without VSync still runs; it may simply use more CPU or tear. We do not want a missing optimization to stop the program.

**The run flag.** `running` is true until something asks us to stop. The loop condition is the whole definition of "the game is alive". The event object is declared once, before the loop, as `SDL_Event event{};`. The braces value-initialize it, and we reuse the same object for every event instead of creating a new one each frame.

**Draining the event queue.** `SDL_PollEvent(&event)` returns true while there is an event waiting. The inner `while` handles *every* pending event, not just one per frame. If you only read one event per frame, a burst of mouse movement or key repeats can make the window feel laggy or unresponsive. Draining the queue each frame is the standard pattern.

**Two ways out.** `SDL_EVENT_QUIT` is posted when the user clicks the close button or otherwise asks the application to terminate. We also treat the Esc key as a quit request, which is convenient when you are launching from a terminal. `SDL_EVENT_KEY_DOWN` is only sent for the press, not the release, and `event.key.key` holds the key code.

**Clear.** `SDL_SetRenderDrawColor` sets the color that subsequent drawing operations use. `SDL_RenderClear` fills the entire render target with that color. Our background is dark navy, which makes it easy to tell a successfully rendered frame from a black, uninitialized window.

We set the clear color every frame even though it never changes in this chapter. Later, drawing the maze and characters will change the current draw color many times per frame. Resetting it before every clear is a reliable habit that prevents mysterious color bugs.

**Present.** `SDL_RenderPresent` takes the frame we have been assembling and shows it on screen. Drawing without presenting is like writing a letter and never putting it in the mailbox: the work exists, but nobody sees it.

That is the entire loop: input, update, draw, present. Input currently handles only quitting, update is empty, and the only drawing is the background clear. Later chapters fill in the rest.

## 5. The complete file

Here is the full `main.cpp`. Compare it with yours; the order of declarations matters.

~~~cpp
// Pac-Man tutorial - Chapter 1: a window into the maze.

#include <SDL3/SDL.h>

#include <memory>

namespace config {
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr SDL_Color kBackground{12, 12, 30, 255};
} // namespace config

namespace {

// Shut down SDL when this object goes out of scope.
class SdlGuard {
  public:
    SdlGuard() : initialized_(SDL_Init(SDL_INIT_VIDEO)) {
        if (!initialized_) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
        }
    }

    ~SdlGuard() {
        if (initialized_) {
            SDL_Quit();
        }
    }

    SdlGuard(const SdlGuard&) = delete;
    SdlGuard& operator=(const SdlGuard&) = delete;
    SdlGuard(SdlGuard&&) = delete;
    SdlGuard& operator=(SdlGuard&&) = delete;

    [[nodiscard]] bool initialized() const { return initialized_; }

  private:
    bool initialized_ = false;
};

struct WindowDeleter {
    void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
};

struct RendererDeleter {
    void operator()(SDL_Renderer* renderer) const {
        SDL_DestroyRenderer(renderer);
    }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

} // namespace

int main() {
    SdlGuard sdl;
    if (!sdl.initialized()) {
        return 1;
    }

    WindowPtr window{SDL_CreateWindow("Pac-Man", config::kWindowWidth,
                                      config::kWindowHeight, 0)};
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    RendererPtr renderer{SDL_CreateRenderer(window.get(), nullptr)};
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_SetRenderVSync(renderer.get(), 1)) {
        SDL_Log("VSync unavailable: %s", SDL_GetError());
    }

    bool running = true;
    SDL_Event event{};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer.get(), config::kBackground.r,
                               config::kBackground.g, config::kBackground.b,
                               config::kBackground.a);
        SDL_RenderClear(renderer.get());
        SDL_RenderPresent(renderer.get());
    }

    return 0;
}
~~~

There is no checked-in reference copy to compare against — the repository deliberately contains no Pac-Man source. The listing above is the reference for this checkpoint. If something is not working, compare your file with it line by line. Keeping your work in git gives you a safe point to return to.

## 6. Build and run

Run these commands from the repository root, the directory that contains the root `CMakeLists.txt`:

~~~bash
cmake -B build
cmake --build build --target pacman
./build/pacman
~~~

Here is what each command does:

1. **`cmake -B build`** configures the project. It finds Homebrew LLVM, checks for SDL3, and writes build files into a new `build/` directory. You need this step the first time, and again whenever you change `CMakeLists.txt` or add a source file to a target. If CMake says it cannot find `apps/pacman/main.cpp`, you have not created the file yet or the path in the target is wrong; fix that and rerun the command.
2. **`cmake --build build --target pacman`** compiles and links only the Pac-Man target. The `--target pacman` is useful because the repository contains many other programs; without it you might build all of them.
3. **`./build/pacman`** runs the executable. On macOS, `build/pacman` is not a `.app` bundle; it is a normal Unix executable that creates a window when it runs.

Expect a window about 800 by 600 pixels, titled "Pac-Man", filled with dark navy. Press Esc, or click the red close button. The window should disappear and your shell prompt should return.

On subsequent runs, you only need the build and run pair:

~~~bash
cmake --build build --target pacman && ./build/pacman
~~~

If the compiler prints warnings, read them. This project enables a fairly strict set of warnings on purpose. Chapter 1's code should build without warnings. A warning today is often a crash or a rendering bug tomorrow.

!!! tip "What if the window opens behind the terminal?"
    Click it once. SDL applications launched from a terminal do not always steal focus on macOS. This is normal and not a bug in your code.

## 7. When it does not work

Start with the first error, not the last one. Build systems often cascade one small failure into a page of noise.

| Symptom | Likely cause | What to try |
| ------- | ------------ | ----------- |
| `Could NOT find SDL3` during configure | SDL3 is missing, or the build directory is stale | Run `brew install sdl3`, then clean the build (below) |
| `Cannot find source file: apps/pacman/main.cpp` | The file has not been created yet, or the target path is wrong | Create `apps/pacman/main.cpp`, check the `add_executable` path, rerun CMake |
| `'SDL3/SDL.h' file not found` | The target is not linked to `SDL3::SDL3` | Check the two target lines in the root `CMakeLists.txt`, rerun CMake |
| `SDL_Init failed` in the terminal | No graphical session, or SDL could not start video | Run locally from a logged-in macOS session, not over SSH |
| The window appears and disappears instantly | A creation step failed, or the loop exits immediately | Run from the terminal and read the log line |
| The window is black or blank | The clear or present step is missing | Compare your loop with the listing above |
| The build uses `/usr/bin/clang++` | The build directory was configured before LLVM was installed | Clean rebuild, so CMake picks the compiler again |
| `VSync unavailable` warning | The renderer or display does not support VSync | Usually harmless; the game still runs |

### Clean rebuild

When in doubt, delete the generated build directory and configure again. This deletes only compiled artifacts; your source files are safe. Run it from the repository root:

~~~bash
rm -rf build
cmake -B build
cmake --build build --target pacman
~~~

Never run `rm -rf build` from a different directory unless you are certain what `build` contains there.

## 8. Exercises

These are small on purpose. The goal is to make the loop *yours*, not to add features.

1. **Repaint the world.** Change `kBackground` to a color you like. Rebuild and run. Try a few values; remember that each channel is 0–255.
2. **Rename the game.** Change the window title from `"Pac-Man"` to something else, and try a different window size. Set the size back to 800 × 600 before Chapter 2, because the maze layout assumes it.
3. **Two ways out.** Make the `Q` key quit as well as Esc. You need to change the condition that checks `event.key.key`. Remember that both keys are only meaningful on `SDL_EVENT_KEY_DOWN`.
4. **Lights on, lights off.** Toggle between the dark background and a lighter one when the player presses Space. You will need a `bool` that survives across frames, and the clear color must read it.
5. **Measure the frame rate.** Every second, log how many frames the loop completed. This is your first taste of timing, which Chapter 3 will make central.

??? example "One possible solution to exercise 4"
    Add a flag before the loop:

    ~~~cpp
    bool lightsOn = true;
    ~~~

    Handle Space in the event loop:

    ~~~cpp
    } else if (event.type == SDL_EVENT_KEY_DOWN &&
               event.key.key == SDLK_SPACE) {
        lightsOn = !lightsOn;
    }
    ~~~

    Choose the color when clearing:

    ~~~cpp
    const SDL_Color background =
        lightsOn ? config::kBackground : SDL_Color{70, 30, 90, 255};
    SDL_SetRenderDrawColor(renderer.get(), background.r, background.g,
                           background.b, background.a);
    SDL_RenderClear(renderer.get());
    ~~~

??? example "One possible solution to exercise 5"
    Capture the start time before the loop and count frames inside it:

    ~~~cpp
    Uint64 lastFpsTick = SDL_GetTicks();
    int framesThisSecond = 0;

    while (running) {
        // ... event handling ...

        ++framesThisSecond;
        const Uint64 now = SDL_GetTicks();
        if (now - lastFpsTick >= 1000ULL) {
            SDL_Log("FPS: %d", framesThisSecond);
            framesThisSecond = 0;
            lastFpsTick = now;
        }

        // ... clearing and presenting ...
    }
    ~~~

    `SDL_GetTicks()` returns milliseconds since SDL started. With VSync on, the number should sit near your display's refresh rate: about 60, 120, or whatever your Mac uses. A different number is not automatically a bug; VSync can be disabled or unavailable.

## 9. Checkpoint

Before moving on, your program should satisfy all of these:

- [ ] `cmake --build build --target pacman` finishes with no errors.
- [ ] A window titled "Pac-Man" opens at 800 × 600.
- [ ] The window is filled with the dark background color.
- [ ] Esc closes the window.
- [ ] The red close button also closes the window.
- [ ] The program returns to the shell prompt without crashing.
- [ ] You can explain what the window is, what the renderer is, and why we drain the event queue every frame.

If all the boxes are checked, you have a solid foundation. If not, fix it now — every later chapter assumes this checkpoint works.

## 10. Recap

In this chapter you:

- Installed Homebrew LLVM, CMake, and SDL3, and verified that CMake can find them.
- Added a `pacman` executable target that links only `SDL3::SDL3`.
- Initialized SDL with a small RAII guard and wrapped SDL's raw pointers in `std::unique_ptr` with custom deleters.
- Created a window and a renderer, and handled creation failures with `SDL_Log` and `SDL_GetError`.
- Built a game loop that drains events, clears the screen, presents a frame, and exits on Esc or the close button.
- Turned on VSync so the loop does not run flat out.

Those pieces — a resource-safe lifetime, an event-driven loop, and a present step — are the chassis of every game in this tutorial. They will not change much from here.

## 11. What's next

Right now the renderer clears a dark rectangle. In **Chapter 2: Drawing the Maze**, we will give it something to draw:

- Store the maze as a small grid of characters.
- Turn tile coordinates into pixels.
- Draw walls, pellets, and power pellets with SDL rectangles and circles.
- Keep the window and loop exactly as they are.

The next chapter is where the game starts to look like Pac-Man.
