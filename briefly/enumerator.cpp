// main.cpp
// Compile with: g++ -std=c++23 main.cpp -o main
//          or: clang++ -std=c++23 main.cpp -o main
//          or: MSVC 19.36+ (VS 2022 17.6+)

#include <iostream>
#include <ranges>
#include <vector>

struct Particle {
    double x, y;
    double vx, vy;
    bool alive = true;
};

int main() {
    std::vector<Particle> particles = {
        {0.0, 0.0, 1.0, 0.5, true},
        {1.0, 1.0, 0.0, 0.0, false}, // dead, will be skipped
        {2.0, 2.0, -1.0, 2.0, true},
        {3.0, 3.0, 0.5, -0.5, true},
    };

    const double dt = 1.0;

    // Iterate with both index and reference to element.
    for (auto [i, p] : particles | std::views::enumerate) {
        if (!p.alive) {
            std::cout << "skip  [" << i << "] (dead)\n";
            continue;
        }

        p.x += p.vx * dt;
        p.y += p.vy * dt;

        std::cout << "update[" << i << "] -> (" << p.x << ", " << p.y << ")\n";
    }

    std::cout << "\nFinal state:\n";
    for (auto [i, p] : particles | std::views::enumerate) {
        std::cout << "  [" << i << "] pos=(" << p.x << ", " << p.y
                  << ") alive=" << std::boolalpha << p.alive << "\n";
    }
}
