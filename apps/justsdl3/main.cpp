#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 GPU Clear", 800, 600, 0);
    if (window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_ShowWindow(window);

    // 0 shader formats is valid for a clear-only program.
    // On macOS, SDL selects the Metal backend automatically.
    SDL_GPUDevice* device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
    if (device == nullptr) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_GPUCommandBuffer* command = SDL_AcquireGPUCommandBuffer(device);
        if (command == nullptr) {
            SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
            break;
        }

        SDL_GPUTexture* swapchain = nullptr;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command, window, &swapchain,
                                                   nullptr, nullptr)) {
            SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s",
                    SDL_GetError());
            SDL_CancelGPUCommandBuffer(command);
            break;
        }

        if (swapchain != nullptr) {
            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = SDL_FColor{0.0F, 0.5F, 0.5F, 1.0F};
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass =
                SDL_BeginGPURenderPass(command, &target, 1, nullptr);

            SDL_EndGPURenderPass(pass);
        }

        SDL_SubmitGPUCommandBuffer(command);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
