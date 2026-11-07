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

    Uint64 lastFpsTick = SDL_GetTicks();
    int framesThisSecond = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        ++framesThisSecond;
        const Uint64 now = SDL_GetTicks();
        if (now - lastFpsTick >= 1000ULL) {
            SDL_Log("FPS: %d", framesThisSecond);
            framesThisSecond = 0;
            lastFpsTick = now;
        }
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
