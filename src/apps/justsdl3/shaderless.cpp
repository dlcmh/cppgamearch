// main.cpp
#include <SDL3/SDL.h>
#include <cmath>
#include <vector>

// Base Shape class - abstract interface
class Shape {
  public:
    virtual ~Shape() = default;
    virtual void draw(SDL_Renderer* renderer) const = 0;
};

// Rectangle shape
class Rect : public Shape {
  public:
    Rect(float x, float y, float w, float h, SDL_Color color)
        : rect_{x, y, w, h}, color_(color) {}

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);
        SDL_RenderFillRect(renderer, &rect_);
    }

  private:
    SDL_FRect rect_;
    SDL_Color color_;
};

// Circle shape (approximated with points)
class Circle : public Shape {
  public:
    Circle(float cx, float cy, float radius, SDL_Color color)
        : m_cx(cx), m_cy(cy), radius_(radius), color_(color) {}

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);
        // Draw circle outline using points
        const int segments = 60;
        for (int i = 0; i < segments; ++i) {
            float angle1 = (2.0f * M_PI * i) / segments;
            float angle2 = (2.0f * M_PI * (i + 1)) / segments;
            float x1 = m_cx + radius_ * cosf(angle1);
            float y1 = m_cy + radius_ * sinf(angle1);
            float x2 = m_cx + radius_ * cosf(angle2);
            float y2 = m_cy + radius_ * sinf(angle2);
            SDL_RenderLine(renderer, x1, y1, x2, y2);
        }
    }

  private:
    float m_cx, m_cy, radius_;
    SDL_Color color_;
};

// Line shape
class Line : public Shape {
  public:
    Line(float x1, float y1, float x2, float y2, SDL_Color color)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), color_(color) {}

    void draw(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b,
                               color_.a);
        SDL_RenderLine(renderer, m_x1, m_y1, m_x2, m_y2);
    }

  private:
    float m_x1, m_y1, m_x2, m_y2;
    SDL_Color color_;
};

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Shapes Demo", 800, 600, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Build the scene
    std::vector<Shape*> shapes;
    shapes.push_back(
        new Rect(100, 100, 150, 100, {255, 0, 0, 255})); // Red rectangle
    shapes.push_back(
        new Circle(500, 200, 60, {0, 255, 0, 255})); // Green circle
    shapes.push_back(
        new Line(200, 400, 600, 500, {0, 0, 255, 255})); // Blue line

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Draw all shapes
        for (const auto* shape : shapes) {
            shape->draw(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    for (auto* shape : shapes) {
        delete shape;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
