#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

namespace config {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr SDL_Color kBackground{12, 12, 30, 255};

constexpr int kMazeCols = 28;
constexpr int kMazeRows = 21;
constexpr float kTileSize = 24.0F;
constexpr float kMazeOriginX = 64.0F;
constexpr float kMazeOriginY = 56.0F;

constexpr SDL_Color kWallFillColor{10, 12, 60, 255};
constexpr SDL_Color kWallEdgeColor{38, 60, 220, 255};
constexpr SDL_Color kPelletColor{255, 213, 74, 255};
constexpr SDL_Color kDoorColor{255, 184, 222, 255};

constexpr float kWallThickness = 3.0F;
constexpr float kPelletRadius = 3.0F;
constexpr float kPowerPelletRadius = 7.0F;
constexpr float kDoorThickness = 4.0F;

// Legend: '#' wall, '.' pellet, 'o' power pellet, '=' ghost-house door, ' '
// empty walkable floor.
// This table is a picture; keep one maze row per line.
// clang-format off
constexpr std::array<std::string_view, kMazeRows> kMaze = {{
    "############################",
    "#o........................o#",
    "#.###.###.########.###.###.#",
    "#.###.###.########.###.###.#",
    "#.###.###.########.###.###.#",
    "#..........................#",
    "#.###.###.########.###.###.#",
    "#..........................#",
    "#.###.###.###==###.###.###.#",
    "#.###.###.#      #.###.###.#",
    "#.###.###.#      #.###.###.#",
    "    ......#      #......    ", // tunnel row: open at both edges
    "#.###.###.########.###.###.#",
    "#............  ............#", // Pac-Man will spawn here later
    "#.###.###.########.###.###.#",
    "#..........................#",
    "#.###.###.########.###.###.#",
    "#.###.###.########.###.###.#",
    "#.###.###.########.###.###.#",
    "#o........................o#",
    "############################",
}};
// clang-format on

} // namespace config

namespace {

// -----------------------------------------------------------------------------
// SDL lifetime
// -----------------------------------------------------------------------------

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
    SdlGuard(const SdlGuard&&) = delete;
    SdlGuard& operator=(const SdlGuard&&) = delete;

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

// -----------------------------------------------------------------------------
// Tile coordinates
// -----------------------------------------------------------------------------

constexpr float tileLeft(int col) {
    return config::kMazeOriginX + (static_cast<float>(col) * config::kTileSize);
}

constexpr float tileTop(int row) {
    return config::kMazeOriginY + (static_cast<float>(row) * config::kTileSize);
}

constexpr float tileCenterX(int col) {
    return tileLeft(col) + (config::kTileSize / 2.0F);
}

constexpr float tileCenterY(int row) {
    return tileLeft(row) + (config::kTileSize / 2.0F);
}

char tileAt(int col, int row) {
    if (col < 0 || col >= config::kMazeCols) {
        return '#';
    }
    if (row < 0 || row >= config::kMazeRows) {
        return '#';
    }
    return config::kMaze.at(static_cast<std::size_t>(row))
        .at(static_cast<std::size_t>(col));
}

bool isWall(int col, int row) { return tileAt(col, row) == '#'; }

// -----------------------------------------------------------------------------
// Drawing helpers
// -----------------------------------------------------------------------------

void fillRect(SDL_Renderer* renderer, float x, float y, float width,
              float height) {
    const SDL_FRect rect{x, y, width, height};
    SDL_RenderFillRect(renderer, &rect);
}

void drawWallEdges(SDL_Renderer* renderer, int col, int row) {
    const float left = tileLeft(col);
    const float top = tileTop(row);

    if (!isWall(col, row - 1)) {
        fillRect(renderer, left, top, config::kTileSize,
                 config::kWallThickness);
    }
    if (!isWall(col, row + 1)) {
        fillRect(renderer, left,
                 top + config::kTileSize - config::kWallThickness,
                 config::kTileSize, config::kWallThickness);
    }
}

} // namespace

int main() {
    SdlGuard sdl;
    if (!sdl.initialized()) {
        return 1;
    }

    WindowPtr window(SDL_CreateWindow("Pac-Man", config::kWindowWidth,
                                      config::kWindowHeight, 0));
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    RendererPtr renderer(SDL_CreateRenderer(window.get(), nullptr));
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_SetRenderVSync(renderer.get(), 1)) {
        SDL_Log("VSync unavailable: %s", SDL_GetError());
    }

    bool running = true;
    SDL_Event event{};

    // FPS - commented out
    // Uint64 lastFpsTick = SDL_GetTicks();
    // int framesThisSecond = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        // FPS - commented out
        // ++framesThisSecond;
        // const Uint64 now = SDL_GetTicks();
        // if (now - lastFpsTick >= 1000ULL) {
        //     SDL_Log("FPS: %d", framesThisSecond);
        //     framesThisSecond = 0;
        //     lastFpsTick = now;
        // }
        // 2026-11-07 22:36:24.607 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:25.607 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:26.607 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:27.607 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:28.607 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:29.615 pacman[2571:22283265] FPS: 121
        // 2026-11-07 22:36:30.623 pacman[2571:22283265] FPS: 121
        // 2026-11-07 22:36:31.623 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:32.623 pacman[2571:22283265] FPS: 120
        // 2026-11-07 22:36:33.623 pacman[2571:22283265] FPS: 120

        SDL_SetRenderDrawColor(renderer.get(), config::kBackground.r,
                               config::kBackground.g, config::kBackground.b,
                               config::kBackground.a);
        SDL_RenderClear(renderer.get());
        SDL_RenderPresent(renderer.get());
    }
}
