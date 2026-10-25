#include <chrono>
#include <iostream>
#include <thread>

class GameLoop {
    // Define a named constant for the target update rate
    static constexpr double TARGET_UPDATE_HZ = 60.0;
    static constexpr double FIXED_DT = 1.0 / TARGET_UPDATE_HZ;

    // Maximum elapsed time accepted for one frame: 250 ms.
    // Prevents a stall from triggering unbounded catch-up updates.
    static constexpr double MAX_FRAME_TIME_SEC = 0.25;

  public:
    void run() {
        using Clock = std::chrono::high_resolution_clock;
        auto previousTime = Clock::now();
        double accumulator = 0.0;

        while (m_running) {
            auto currentTime = Clock::now();
            std::chrono::duration<double> elapsed = currentTime - previousTime;
            previousTime = currentTime;

            // Safety clamp to avoid the "spiral of death" after a stall
            // (e.g., when dragging the window or a debug break)
            if (elapsed.count() > MAX_FRAME_TIME_SEC) {
                elapsed = std::chrono::duration<double>(MAX_FRAME_TIME_SEC);
            }

            accumulator += elapsed.count();

            // Fixed-step update loop
            while (accumulator >= m_fixedDt) {
                processInput(m_fixedDt);
                update(m_fixedDt);
                accumulator -= m_fixedDt;
            }

            // Render with interpolation factor (alpha)
            // alpha is between 0.0 and 1.0, representing how far we are
            // between the previous and current simulation state.
            double alpha = accumulator / m_fixedDt;
            render(alpha);

            // Optional: Simple frame limiter if you want to cap CPU usage
            // This is not strictly necessary if VSync is enabled.
            limitFrameRate(currentTime);
        }
    }

    void stop() { m_running = false; }

  private:
    std::string m_betaLabel = "Beta"; // dummy instance member to suppress
                                      // 'readability-convert-member-functions-to-static' warning on
                                      // instance functions, remove when no longer needed
    double m_fixedDt = FIXED_DT;
    bool m_running = true;

    void processInput(double dt) {
        // Handle input once per fixed update, not per render frame.
        // This ensures input consistency.
        std::cout << m_betaLabel << ": Processing input with dt: " << dt << " seconds" << '\n';
    }

    void update(double dt) {
        // Game logic, physics, AI, etc.
        // Always use the fixed dt provided here.
        // Example: m_player.position += m_player.velocity * dt;
        std::cout << m_betaLabel << ": Updating game state with dt: " << dt << " seconds" << '\n';
    }

    void render(double alpha) {
        // Render the game.
        // Use the 'alpha' value to interpolate positions between the
        // previous state and the current state for smooth visuals.
        // Example:
        // auto renderPos = lerp(m_prevPos, m_currPos, alpha);
        std::cout << m_betaLabel << ": Rendering with interpolation factor: " << alpha << '\n';
    }

    void limitFrameRate(std::chrono::high_resolution_clock::time_point frameStart) {
        // Target render rate (e.g., 120 Hz or 60 Hz).
        // If VSync is enabled in your graphics API, you can remove this.
        const double targetFrameTime = 1.0 / 120.0; // 120 Hz target
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameDuration = frameEnd - frameStart;

        if (frameDuration.count() < targetFrameTime) {
            std::this_thread::sleep_for(
                std::chrono::duration<double>(targetFrameTime - frameDuration.count()));
        }
    }
};

int main() {
    GameLoop loop;
    loop.run();
    return 0;
}