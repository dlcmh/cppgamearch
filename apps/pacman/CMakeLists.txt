cmake_minimum_required(VERSION 3.28)

# Force Homebrew LLVM before project() so it takes effect
if(APPLE)
    # ARM
    if(EXISTS "/opt/homebrew/opt/llvm/bin/clang++")
        set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
        set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
    # Intel
    elseif(EXISTS "/usr/local/opt/llvm/bin/clang++")
        set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
        set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
    endif()
endif()

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

add_executable(pacman main.cpp)
target_link_libraries(pacman PRIVATE SDL3::SDL3)
