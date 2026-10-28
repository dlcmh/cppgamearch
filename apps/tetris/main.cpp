#include "sdl_log.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <vector>

namespace config {
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 600;
constexpr int kCircleSegments = 60;

constexpr SDL_Color kPink{255, 0, 200, 255};
constexpr SDL_Color kGold{255, 200, 60, 255};
constexpr SDL_Color kCoral{255, 120, 100, 255};
constexpr SDL_Color kDarkTeal{0, 90, 90, 255};
constexpr SDL_Color kBackground{kDarkTeal};

// Scene layout (kept together so the demo is easy to tweak).
constexpr SDL_FRect kRectDemo{100.0F, 100.0F, 150.0F, 100.0F};
constexpr SDL_FRect kCircleDemo{500.0F, 200.0F, 60.0F,
                                60.0F}; // x,y,radius,unused
constexpr SDL_FRect kLineDemo{200.0F, 400.0F, 600.0F, 500.0F}; // x1,y1,x2,y2
} // namespace config

// -----------------------------------------------------------------------------
// RAII guard for SDL
// -----------------------------------------------------------------------------

class SdlGuard {
  public:
    SdlGuard() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            sdl::log("SDL_Init failed: %s", SDL_GetError());
            ok_ = false;
        }
    }
    ~SdlGuard() {
        if (ok_) { // Why if (ok_)? If SDL_Init failed, SDL was never
                   // initialized. Calling SDL_Quit() on a library that was
                   // never initialized is at best a no-op and at worst
                   // undefined. The flag we set in the constructor tells us:
                   // "only clean up what we actually acquired." This is the
                   // standard RAII pattern — the destructor mirrors the
                   // constructor, but only releases what was successfully
                   // acquired.
            SDL_Quit();
        }
    }

    // Copying/moving would mean two objects think they own SDL.
    // Forbid it (same reasoning as your Shape base class).
    SdlGuard(const SdlGuard&) = delete;
    SdlGuard& operator=(const SdlGuard&) = delete;
    SdlGuard(const SdlGuard&&) = delete;
    SdlGuard& operator=(SdlGuard&&) = delete;

    [[nodiscard]] bool ok() const { return ok_; }

  private:
    bool ok_ = true;
};

// ---------------------------------------------------------------------------
// Shape hierarchy
// ---------------------------------------------------------------------------

class Shape {
  public:
    Shape() = default;
    virtual ~Shape() = default;

    // A polymorphic base must not be copied/moved by value (slicing).
    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    Shape(Shape&&) = delete;
    Shape& operator=(Shape&&) = delete;

    virtual void draw(SDL_Renderer* renderer) const = 0;
    virtual void update(float /*deltaSeconds*/) {}
};

class Rect final : public Shape {
  public:
    Rect(float x, float y, float width, float height, SDL_Color color)
        : rect_{x, y, width, height}, color_(color) {}

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);
        SDL_RenderFillRect(renderer, &rect_);
    }

  private:
    SDL_FRect rect_;
    SDL_Color color_;
};

class Circle final : public Shape {
  public:
    Circle(float centerX, float centerY, float radius, SDL_Color color)
        : centerX_(centerX), centerY_(centerY), radius_(radius), color_(color) {
    }

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);

        constexpr float kTwoPi = 2.0F * std::numbers::pi_v<float>;

        // Cache the previous point so we only evaluate trig once per vertex.
        float previousX = centerX_ + radius_; // angle 0
        float previousY = centerY_;

        for (int segment = 1; segment <= config::kCircleSegments; ++segment) {
            const float angle = (kTwoPi * static_cast<float>(segment)) /
                                static_cast<float>(config::kCircleSegments);
            const float nextX = centerX_ + (radius_ * std::cos(angle));
            const float nextY = centerY_ + (radius_ * std::sin(angle));
            SDL_RenderLine(renderer, previousX, previousY, nextX, nextY);
            previousX = nextX;
            previousY = nextY;
        }
    }

    void update(float deltaSeconds) override {
        constexpr float kSpeed = 80.0F; // px per second
        const float radius = radius_;
        const float minY = radius;
        const float maxY = static_cast<float>(config::kWindowHeight) - radius;

        centerY_ += direction_ * kSpeed * deltaSeconds;
        if (centerY_ <= minY) {
            centerY_ = minY;
            direction_ = 1.0F;
        } else if (centerY_ >= maxY) {
            centerY_ = maxY;
            direction_ = -1.0F;
        }
    }

  private:
    float centerX_;
    float centerY_;
    float radius_;
    float direction_ = -1.0F;
    SDL_Color color_;
};

class Line final : public Shape {
  public:
    Line(float startX, float startY, float endX, float endY, SDL_Color color)
        : startX_(startX), startY_(startY), endX_(endX), endY_(endY),
          color_(color) {}

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);
        SDL_RenderLine(renderer, startX_, startY_, endX_, endY_);
    }

  private:
    float startX_;
    float startY_;
    float endX_;
    float endY_;
    SDL_Color color_;
};

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------

namespace {

// RAII wrappers so we don't have to remember cleanup order on every early
// return.
struct WindowDeleter {
    void operator()(SDL_Window* w) const { SDL_DestroyWindow(w); }
};
struct RendererDeleter {
    void operator()(SDL_Renderer* r) const { SDL_DestroyRenderer(r); }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

std::vector<std::unique_ptr<Shape>> buildScene() {
    std::vector<std::unique_ptr<Shape>> shapes;

    const auto& shape1 = config::kRectDemo;
    shapes.push_back(std::make_unique<Rect>(shape1.x, shape1.y, shape1.w,
                                            shape1.h, config::kPink));

    const auto& shape2 = config::kCircleDemo;
    shapes.push_back(
        std::make_unique<Circle>(shape2.x, shape2.y, shape2.w, config::kGold));

    const auto& shape3 = config::kLineDemo;
    shapes.push_back(std::make_unique<Line>(shape3.x, shape3.y, shape3.w,
                                            shape3.h, config::kCoral));

    return shapes;
}

} // namespace

int main() {
    SdlGuard sdl;
    if (!sdl.ok()) {
        return 1;
    }

    WindowPtr window{SDL_CreateWindow("Shapes Demo", config::kWindowWidth,
                                      config::kWindowHeight, 0)};
    if (!window) {
        sdl::log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    RendererPtr renderer{SDL_CreateRenderer(window.get(), nullptr)};
    if (!renderer) {
        sdl::log("Renderer creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    const auto shapes = buildScene();

    bool running = true;
    SDL_Event event;

    auto lastTicks = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        const auto now = SDL_GetTicks();
        const auto deltaSeconds = static_cast<float>(now - lastTicks) / 1000.0F;
        lastTicks = now;

        for (const auto& shape : shapes) {
            shape->update(deltaSeconds);
        }

        SDL_SetRenderDrawColor(renderer.get(), config::kBackground.r,
                               config::kBackground.g, config::kBackground.b,
                               config::kBackground.a);
        SDL_RenderClear(renderer.get());

        for (const auto& shape : shapes) {
            shape->draw(renderer.get());
        }

        SDL_RenderPresent(renderer.get());
    }

    // No manual cleanup loop needed — unique_ptr handles it.

    return 0;
}
