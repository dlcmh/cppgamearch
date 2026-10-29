#include "sdl_log.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numbers>
#include <optional>
#include <vector>

// -----------------------------------------------------------------------------
// Playfield: a grid of cells. Empty cells are represented by std::nullopt.
// -----------------------------------------------------------------------------

namespace config {
constexpr int kBoardWidth = 10;  // columns
constexpr int kBoardHeight = 20; // rows
// constexpr float kCellSize = 24.0F; // pixels per cell
} // namespace config

class Board {
  public:
    // Returns true if the cell is empty (in-bounds).
    [[nodiscard]] bool isEmpty(int col, int row) const {
        if (col < 0 || col >= config::kBoardWidth) {
            return false;
        }
        if (row < 0 || row >= config::kBoardHeight) {
            return false;
        }
        return !cells_.at(static_cast<size_t>(row))
                    .at(static_cast<size_t>(col))
                    .has_value();
    }

    // Fill a cell. Caller must ensure it's in-bounds.
    void set(int col, int row, SDL_Color color) {
        cells_.at(static_cast<size_t>(row)).at(static_cast<size_t>(col)) =
            color;
    }

    // Clear a cell.
    void clear(int col, int row) {
        cells_.at(static_cast<size_t>(row))
            .at(static_cast<size_t>(col))
            .reset();
    }

  private:
    // std::optional<SDL_Color> per cell: "empty" vs "filled with color".

    /*  Original C-style array flagged by clang-tidy
     *  ============================================
     *  std::optional<SDL_Color> cells_[config::kBoardHeight]
     *                                 [config::kBoardWidth];
     */

    using Row = std::array<std::optional<SDL_Color>, config::kBoardWidth>;
    std::array<Row, config::kBoardHeight>
        cells_{}; // cells_{} — the {} value-initializes every cell to
                  // std::nullopt (default-constructed optional). Without it,
                  // the optionals would still be default-constructed (they're a
                  // class type), but writing {} is clearer and matches the
                  // "always initialize" guideline.
};

// 2. The Falling Tetromino: model, spawn, collision

// 2A. Represent a Tetromino's Shape

// A cell offset within the piece, relative to the piece's anchor.
struct CellOffset {
    int col;
    int row;
};

// 4 cells per rotation state, 4 rotation states per piece.
using RotationState = std::array<CellOffset, 4>;
using ShapeTable = std::array<RotationState, 4>;

enum class PieceType : uint8_t { I, O, T, S, Z, J, L };

namespace {

// All 7 tetrominoes, each in 4 rotation states, each state = 4 cell offsets.
// Offsets are relative to the anchor (anchor = top-left of the piece's bounding
// box). Rotations follow the SRS convention: states cycle 0→1→2→3.
constexpr std::array<ShapeTable, 7> kShapes = {{
    // I
    {{
        {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}, // state 0: horizontal
        {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}}, // state 1: vertical
        {{{0, 2}, {1, 2}, {2, 2}, {3, 2}}}, // state 2: horizontal (shifted)
        {{{1, 0}, {1, 1}, {1, 2}, {1, 3}}}, // state 3: vertical (shifted)
    }},
    // O
    {{
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}, // state 0
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}, // state 1 (identical)
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}, // state 2 (identical)
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}, // state 3 (identical)
    }},
    // T
    {{
        {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}}, // state 0: pointing up
        {{{1, 0}, {1, 1}, {2, 1}, {1, 2}}}, // state 0: pointing right
        {{{0, 1}, {1, 1}, {2, 1}, {1, 2}}}, // state 0: pointing down
        {{{1, 0}, {0, 1}, {1, 1}, {1, 2}}}, // state 0: pointing left
    }},
    // S
    {{
        {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}}, // state 0: pointing up
        {{{1, 0}, {1, 1}, {2, 1}, {2, 2}}}, // state 0: pointing right
        {{{0, 1}, {1, 1}, {2, 1}, {1, 2}}}, // state 0: pointing down
        {{{1, 0}, {0, 1}, {1, 1}, {1, 2}}}, // state 0: pointing left
    }},
    // Z
    {{
        {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}, // state 0: pointing up
        {{{2, 0}, {1, 1}, {2, 1}, {1, 2}}}, // state 0: pointing right
        {{{0, 1}, {1, 1}, {1, 2}, {2, 2}}}, // state 0: pointing down
        {{{1, 0}, {0, 1}, {1, 1}, {0, 2}}}, // state 0: pointing left
    }},
    // J
    {{
        {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}}, // state 0: pointing up
        {{{1, 0}, {2, 0}, {1, 1}, {1, 2}}}, // state 0: pointing right
        {{{0, 1}, {1, 1}, {2, 1}, {2, 2}}}, // state 0: pointing down
        {{{1, 0}, {1, 1}, {0, 2}, {1, 2}}}, // state 0: pointing left
    }},
    // L
    {{
        {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}}, // state 0: pointing up
        {{{1, 0}, {1, 1}, {1, 2}, {2, 2}}}, // state 0: pointing right
        {{{0, 1}, {1, 1}, {2, 1}, {0, 2}}}, // state 0: pointing down
        {{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}, // state 0: pointing left
    }},
}};

} // namespace

// end 2A. Represent a Tetromino's Shape

// 2B. The Tetromino Type

class Tetromino {
  public:
    Tetromino(PieceType type, int anchorCol, int anchorRow, int rotation,
              SDL_Color color)
        : type_(type), anchorCol_(anchorCol), anchorRow_(anchorRow),
          rotation_(rotation), color_(color) {}

    // The four cell offsets for the current rotation state.
    [[nodiscard]] const RotationState& offsets() const {
        return kShapes.at(static_cast<size_t>(type_))
            .at(static_cast<size_t>(rotation_));
    }

    // Absolute board position of cell i (i in [0, 4]).
    [[nodiscard]] CellOffset cellAt(int i) const {
        const CellOffset off = offsets().at(static_cast<size_t>(i));
        return {anchorCol_ + off.col, anchorRow_ + off.row};
    }

    [[nodiscard]] SDL_Color color() const { return color_; }

    void moveLeft() { --anchorCol_; }
    void moveRight() { ++anchorCol_; }
    void moveDown() { ++anchorRow_; }

    void rotateCW() { rotation_ = (rotation_ + 1) % 4; }
    void rotateCCW() {
        rotation_ = (rotation_ + 3) % 4;
    } // / Add 3 (≡ −1 mod 4) instead of subtracting 1, so the operand stays
      // non-negative and % 4 never yields a negative index.

  private:
    PieceType type_;
    int anchorCol_;
    int anchorRow_;
    int rotation_ = 0; // the rotation state (0-3)
    SDL_Color color_;
};

// end 2B. The Tetromino Type

// 2C. Spawn Colors

namespace {

SDL_Color colorFor(PieceType type) {
    switch (type) {
    case PieceType::I:
        return SDL_Color{0, 240, 240, 255}; // cyan
    case PieceType::O:
        return SDL_Color{240, 240, 0, 255}; // yellow
    case PieceType::T:
        return SDL_Color{160, 0, 240, 255}; // purple
    case PieceType::S:
        return SDL_Color{0, 240, 0, 255}; // green
    case PieceType::Z:
        return SDL_Color{240, 0, 0, 255}; // red
    case PieceType::J:
        return SDL_Color{0, 0, 240, 255}; // blue
    case PieceType::L:
        return SDL_Color{240, 160, 0, 255}; // orange
    }
} // namespace

// end 2C. Spawn Colors

// 2D. Collision Against the Board

namespace {
// Test a hypothetical move (eg -1, 0 for move left) to see if a piece will fit.
// The piece will fit if the end position does not overlap any filled cell or
// the board edge.
bool fitsAt(const Board& board, const Tetromino& piece, int dCol, int dRow) {
    for (int i = 0; i < 4; ++i) {
        const CellOffset c = piece.cellAt(i);
        if (!board.isEmpty(c.col + dCol, c.row + dRow)) {
            return false;
        }
    }
    return true;
}

// Returns true if `piece` placed at its current position (0, 0) does not
// overlap any filled cell or the board edges.
bool canPlace(const Board& board, const Tetromino& piece) {
    return fitsAt(board, piece, 0, 0);
}
} // namespace

// end 2D. Collision Against the Board

// end 2. The Falling Tetromino: model, spawn, collision

// -----------------------------------------------------------------------------
// TODO: Delete once no longer needed
// -----------------------------------------------------------------------------

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
