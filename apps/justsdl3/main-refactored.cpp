#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <memory>
#include <string_view>
#include <utility>

namespace {

//
// Constants
//

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr SDL_WindowFlags kWindowFlags = 0; // no special flags

// Teal-ish clear color: (R, G, B, A)
constexpr SDL_FColor kClearColor{0.0F, 0.5F, 0.5F, 1.0F};

//
// Logging shim
//
// SDL_Log is a C vararg function; clang-tidy's pro-type-vararg rule (and
// C++ Core Guidelines) correctly flag it because varargs bypass type
// checking. SDL3 offers no non-vararg replacement, so we wrap it here —
// this is the ONE place in the codebase allowed to touch it.
//

void LogError(std::string_view context, const char* detail = nullptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    SDL_Log("%.*s: %s", static_cast<int>(context.size()), context.data(),
            detail != nullptr ? detail : "");
}

//
// RAII wrappers, one for each SDL resource with a distinct desctructor.
//

struct SDLQuitter {
    ~SDLQuitter() { SDL_Quit(); }

    SDLQuitter(const SDLQuitter&) = delete;
    SDLQuitter& operator=(const SDLQuitter&) = delete;
    SDLQuitter(SDLQuitter&&) = delete;
    SDLQuitter& operator=(SDLQuitter&&) = delete;
};

struct WindowDeleter {
    void operator()(SDL_Window* w) const noexcept { SDL_DestroyWindow(w); }
};
using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;

struct GPUDeviceDeleter {
    void operator()(SDL_GPUDevice* d) const noexcept {
        SDL_DestroyGPUDevice(d);
    }
};
using GPUDevicePtr = std::unique_ptr<SDL_Window, GPUDeviceDeleter>;

// Command buffer is *not* an owning handle in the usual sense — you either
// submit or cancel it. Model that explicitly with a small RAII type so every
// exit path does exactly one of those.
class CommandBuffer {
  public:
    explicit CommandBuffer(SDL_GPUDevice* device) noexcept
        : device_(device), buf_(SDL_AcquireGPUCommandBuffer(device)) {}

    ~CommandBuffer() {
        if (buf_ != nullptr) {
            // Only reached if the caller neither submitted nor cancelled.
            SDL_CancelGPUCommandBuffer(buf_);
        }
    }

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(CommandBuffer&& other) noexcept
        : device_(other.device_), buf_(std::exchange(other.buf_, nullptr)) {}

    CommandBuffer& operator=(CommandBuffer&& other) noexcept {
        if (this != &other) {
            Reset();
            device_ = other.device_;
            buf_ = std::exchange(other.buf_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] SDL_GPUCommandBuffer* Get() const noexcept { return buf_; }
    explicit operator bool() const noexcept { return buf_ != nullptr; }

    // Consume the buffer, handing ownership to SDL.
    bool Submit() {
        if (buf_ == nullptr) {
            return false;
        }
        const bool ok = SDL_SubmitGPUCommandBuffer(buf_);
        buf_ = nullptr;
        return ok;
    }

    void Cancel() {
        if (buf_ == nullptr) {
            return;
        }
        SDL_CancelGPUCommandBuffer(buf_);
        buf_ = nullptr;
    }

  private:
    void Reset() {}

    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUCommandBuffer* buf_ = nullptr;
};

//
// Boot / shutdown
//

struct Engine {
    SDLQuitter quit_guard;
    WindowPtr window;
    GPUDevicePtr device;
};

[[nodiscard]] bool InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LogError("SDL_Init", SDL_GetError());
        return false;
    }
    return true;
}

[[nodiscard]] WindowPtr CreateWindowOrNull() {
    WindowPtr w(SDL_CreateWindow("SDL3 GPU Clear", kWindowWidth, kWindowHeight,
                                 kWindowFlags));
    if (!w) {
        LogError("SDL_CreateWindow", SDL_GetError());
        return nullptr;
    }
    SDL_ShowWindow(w.get());
    return w;
}

[[nodiscard]] GPUDevicePtr CreateDeviceOrNull() {
    // MSL shader format is correct on macOS; SDL picks Metal automatically.
    // A clear-only program needs, no shader, but this must match the backend.
}

[[nodiscard]] bool ClaimWindow(SDL_GPUDevice* device, SDL_Window* window) {
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        LogError("SDL_ClaimWindowForGPUDevice", SDL_GetError());
        return false;
    }
    return true;
}

} // namespace

int main() {}
