#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) { // starts SDL's video subsystem
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "sdl3metalwindow",
        1280, 720,
        SDL_WINDOW_METAL // tells SDL we intend to render this window
                               // with Metal later
    );

    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        SDL_Log("Failed to create Metal device");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    SDL_Log("Metal device: %s", device->name()->utf8String()); // prints:
    // 2026-10-24 16:02:30.829 sdl3metalwindow[46121:16047798] Metal device: Apple M5

    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { // drains all pending events
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // We will render here in the next stage.
    }

    device->release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    pool->drain();
    return 0;
}