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

int main() {}
