cmake_minimum_required(VERSION 3.28)
project(cppgamearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 CONFIG REQUIRED)
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

add_executable(sdl3window src/apps/sdl3window/main.cpp)
target_link_libraries(sdl3window PRIVATE SDL3::SDL3)
target_compile_options(sdl3window PRIVATE -Wall -Wextra -Wpedantic)

add_executable(sdl3metalwindow src/apps/sdl3metalwindow/main.cpp)
target_include_directories(sdl3metalwindow SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/metal-cpp
)
target_link_libraries(sdl3metalwindow PRIVATE
    SDL3::SDL3
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
)
target_compile_options(sdl3metalwindow PRIVATE -Wall -Wextra -Wpedantic)

add_executable(darkbluewindow src/apps/darkbluewindow/main.cpp)
target_include_directories(darkbluewindow SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/metal-cpp
)
target_link_libraries(darkbluewindow PRIVATE
    SDL3::SDL3
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
)
target_compile_options(darkbluewindow PRIVATE -Wall -Wextra -Wpedantic)
