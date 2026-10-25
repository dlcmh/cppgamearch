#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Bring own namespace into global scope for this file only
using namespace dakerr;

class Canvas {
  private:
    int width_, height_;
    std::vector<char> cells_;

  public:
    // Constructor - Member Initializer List (preferred style)
    Canvas(int width, int height)
        : width_(width), height_(height),
          cells_(static_cast<std::size_t>(width * height), ' ') {}

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    // Member function
    void clear() { std::ranges::fill(cells_, ' '); }

    void drawBorder() {
        for (int x = 0; x < width_; ++x) {
            set(x, 0, '#');
            set(x, height_ - 1, '#');
        }
        for (int y = 0; y < height_; ++y) {
            set(0, y, '#');
            set(width_ - 1, y, '#');
        }
    }

    void set(int x, int y, char glyph) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) {
            return;
        }
        cells_[sz(x + (y * width_))] = glyph;
    }

    [[nodiscard]] std::string str() const {
        std::string out;
        out.reserve(sz((width_ + 1) * height_));
        for (int y = 0; y < height_; ++y) {
            out.append(&cells_[sz(y * width_)], sz(width_));
            out.push_back('\n');
        }
        return out;
    }
};

class Fleet {
  public:
    Fleet(int rows, int columns, int startX, int startY, int fieldWidth)
        : rows_(rows), columns_(columns), startX_(startX), startY_(startY),
          fieldWidth_(fieldWidth) {
        reset();
    }

    void reset() {
        aliens_.clear();
        direction_ = 1;
        for (int row = 0; row < rows_; ++row) {
            for (int column = 0; column < columns_; ++column) {
                aliens_.push_back(Alien{.x = startX_ + (column * 3),
                                        .y = startY_ + (row * 2)});
            }
        }
    }

    void update() {
        const auto [minX, maxX] = horizontalBounds();
        const bool atRightEdge = direction_ > 0 && maxX >= fieldWidth_ - 2;
        const bool atLeftEdge = direction_ < 0 && minX <= 1;

        if (atRightEdge || atLeftEdge) {
            direction_ = -direction_;
            for (auto& alien : aliens_) {
                ++alien.y;
            }
        } else {
            for (auto& alien : aliens_) {
                alien.x += direction_;
            }
        }
    }

    void draw(Canvas& canvas) const {
        for (const auto& alien : aliens_) {
            canvas.set(alien.x, alien.y, 'A');
        }
    }

    [[nodiscard]] int bottom() const {
        int maxY = 0;
        for (const auto& alien : aliens_) {
            maxY = std::max(maxY, alien.y);
        }
        return maxY;
    }

  private:
    struct Alien {
        int x, y;
    };
    int rows_;
    int columns_;
    int startX_;
    int startY_;
    int fieldWidth_;
    int direction_ = 1;
    std::vector<Alien> aliens_;

    [[nodiscard]] std::pair<int, int> horizontalBounds() const {
        int minX = aliens_.front().x;
        int maxX = aliens_.front().x;
        for (const auto& alien : aliens_) {
            minX = std::min(minX, alien.x);
            maxX = std::max(maxX, alien.x);
        }
        return {minX, maxX};
    }
};

class Terminal {
  public:
    Terminal() {
        // "\x1b[02J" erase the entire display
        std::cout << "\x1b[02J";
    }

    // Prevent copying/moving
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(Terminal&&) = delete;

    ~Terminal() { std::cout << "\x1b[02J"; }

    static void present(const Canvas& canvas) {
        // "\x1b[H" moves cursor to row 1, column 1
        std::cout << "\x1b[H" << canvas.str() << std::flush;
    }
};

class GameLoop {
  public:
    GameLoop(Fleet& fleet, Canvas& canvas) : fleet_(fleet), canvas_(canvas) {}

    // GameLoop is a top-level controller and should never be copied.
    // The following deletes are to prevent copying/moving:
    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
    GameLoop(GameLoop&&) = delete;
    GameLoop& operator=(GameLoop&&) = delete;
    ~GameLoop() = default;

    void run() {
        using Clock = std::chrono::steady_clock;
        constexpr double kUpdateInterval = 0.12;
        constexpr auto kFrameInterval = std::chrono::milliseconds(16);
        constexpr double kMaxFrameTime = 0.25;

        auto previous = Clock::now();
        double accumulator = 0.0;

        for (;;) {
            const auto frameStart = Clock::now();
            double frameTime =
                std::chrono::duration<double>(frameStart - previous).count();
            previous = frameStart;

            frameTime = std::min(frameTime, kMaxFrameTime);
            // if (frameTime > kMaxFrameTime) {
            //     frameTime = kMaxFrameTime;
            // }
            accumulator += frameTime;

            while (accumulator >= kUpdateInterval) {
                update();
                accumulator -= kUpdateInterval;
            }

            render();

            const auto frameEnd = Clock::now();
            const auto frameDuration = frameEnd - frameStart;
            if (frameDuration < kFrameInterval) {
                std::this_thread::sleep_for(kFrameInterval - frameDuration);
            }
        }
    }

  private:
    void update() {
        fleet_.update();
        if (fleet_.bottom() >= canvas_.height() - 1) {
            fleet_.reset();
        }
    }

    void render() {
        canvas_.clear();
        canvas_.drawBorder();
        fleet_.draw(canvas_);
        Terminal::present(canvas_);
    }

    Fleet& fleet_;
    Canvas& canvas_;
};

int main() {
    constexpr int kFleetRows = 3;
    constexpr int kFleetColumns = 6;
    constexpr int kWidth = 40;
    constexpr int kHeight = 10;

    Canvas canvas(kWidth, kHeight);
    Fleet fleet(kFleetRows, kFleetColumns, 3, 2, kWidth);
    Terminal terminal;
    GameLoop loop(fleet, canvas);

    loop.run();
}
