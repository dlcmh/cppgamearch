cmake_minimum_required(VERSION 3.28)
project(cppgamearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Apply warning flags globally to ALL targets defined in this file
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
)

# 2. Automatically include your global 'include' directory for ALL targets
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 3. Find dependencies once
find_package(SDL3 CONFIG REQUIRED)
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

# Group Apple Metal frameworks into a reusable variable
set(APPLE_METAL_LIBS
    SDL3::SDL3
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
)

# --- Define Your Apps cleanly ---
add_executable(tetris apps/tetris/main.cpp)
target_link_libraries(tetris PRIVATE ${APPLE_METAL_LIBS})

add_executable(gameloopclass apps/gameloopclass/main.cpp)

add_executable(chrspaceinvaders apps/chrspaceinvaders/main.cpp)

add_executable(sdl3window apps/sdl3window/main.cpp)
target_link_libraries(sdl3window PRIVATE SDL3::SDL3)

add_executable(justsdl3 apps/justsdl3/main.cpp)
target_link_libraries(justsdl3 PRIVATE ${APPLE_METAL_LIBS})
add_executable(justsdl3-refactored apps/justsdl3/main-refactored.cpp)
target_link_libraries(justsdl3-refactored PRIVATE ${APPLE_METAL_LIBS})
add_executable(shaderless apps/justsdl3/shaderless.cpp)
target_link_libraries(shaderless PRIVATE ${APPLE_METAL_LIBS})
add_executable(shaderless-r1 apps/justsdl3/shaderless-r1.cpp)
target_link_libraries(shaderless-r1 PRIVATE ${APPLE_METAL_LIBS})

add_executable(sdl3metalwindow apps/sdl3metalwindow/main.cpp)
target_include_directories(sdl3metalwindow SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/metal-cpp)
target_link_libraries(sdl3metalwindow PRIVATE ${APPLE_METAL_LIBS})

add_executable(darkbluewindow apps/darkbluewindow/main.cpp)
target_include_directories(darkbluewindow SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/metal-cpp)
target_link_libraries(darkbluewindow PRIVATE ${APPLE_METAL_LIBS})
