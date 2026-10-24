cmake_minimum_required(VERSION 3.28)
project(TheArchitecture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 CONFIG REQUIRED)

add_executable(TheArchitecture src/main.cpp)
target_link_libraries(TheArchitecture PRIVATE SDL3::SDL3)
target_compile_options(TheArchitecture PRIVATE -Wall -Wextra -Wpedantic)
