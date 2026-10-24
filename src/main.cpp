#include <SDL3/SDL.h>

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) { // starts SDL's video subsystem
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "cppgamearch",
        1280, 720,
        SDL_WINDOW_METAL // tells SDL we intend to render this window
                               // with Metal later
    );

    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

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

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}