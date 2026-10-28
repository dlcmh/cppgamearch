#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) { // starts SDL's video subsystem
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "darkbluewindow",
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

    // pool must come before SDL_Log
    // Why: device->name() returns an autoreleased Metal-cpp string.
    // The pool must exist before it is created.
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    SDL_Log("Metal device: %s", device->name()->utf8String());

    // Attach the layer
    // SDL_Metal_CreateView does the Cocoa-specific work:
    // it creates a CAMetalLayer-backed NSView and installs it
    // in the window.
    // SDL_Metal_GetLayer then hands us the actual layer.
    // SDL deliberately leaves associating the MTLDevice to us,
    // hence setDevice.
    SDL_MetalView view = SDL_Metal_CreateView(window);
    if (!view) {
        SDL_Log("SDL_Metal_CreateView failed: %s", SDL_GetError());
        device->release();
        SDL_DestroyWindow(window);
        SDL_Quit();
        pool->drain();
        return 1;
    }

    CA::MetalLayer* layer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(view));
    if (!layer) {
        SDL_Log("SDL_Metal_GetLayer failed: %s", SDL_GetError());
        SDL_Metal_DestroyView(view);
        device->release();
        SDL_DestroyWindow(window);
        SDL_Quit();
        pool->drain();
        return 1;
    }

    layer->setDevice(device);
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    // End: Attach the layer

    // Create the command queue and render pass
    // The command queue is created once, not per frame.
    // The render pass descriptor is also reused:
    // each frame we'll only swap in that frame's drawable texture.
    // The clear color is a dark blue: (0.05, 0.05, 0.12, 1.0).
    MTL::CommandQueue* commandQueue = device->newCommandQueue();
    if (!commandQueue) {
        SDL_Log("Failed to create command queue");
        SDL_Metal_DestroyView(view);
        device->release();
        SDL_DestroyWindow(window);
        SDL_Quit();
        pool->drain();
        return 1;
    }

    MTL::RenderPassDescriptor* pass = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* color = pass->colorAttachments()->object(0);

    color->setLoadAction(MTL::LoadActionClear);
    color->setStoreAction(MTL::StoreActionStore);
    color->setClearColor(MTL::ClearColor::Make(0.05, 0.05, 0.12, 1.0));
    // End: Create the command queue and render pass

    // Encode a frame in the loop
    bool running = true;

    while (running) {
        NS::AutoreleasePool* framePool = NS::AutoreleasePool::alloc()->init();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        if (!running) {
            framePool->drain();
            break;
        }

        CA::MetalDrawable* drawable = layer->nextDrawable();
        if (drawable) {
            color->setTexture(drawable->texture());
            MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
            MTL::RenderCommandEncoder* encoder = commandBuffer->renderCommandEncoder(pass);
            encoder->endEncoding();
            commandBuffer->presentDrawable(drawable);
            commandBuffer->commit();
        }

        framePool->drain();
    }
    // End: Encode a frame in the loop

    // Cleanup
    pass->release();
    commandQueue->release();
    SDL_Metal_DestroyView(view);
    device->release();
    SDL_DestroyWindow(window);
    SDL_Quit();
    pool->drain();
    return 0;
    // End: Cleanup
}