#pragma once

#include <SDL3/SDL.h>
#include <utility>

namespace sdl {
template <typename... Args> void log(const char* fmt, Args&&... args) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    SDL_Log(fmt, std::forward<Args>(args)...);
}
} // namespace sdl
