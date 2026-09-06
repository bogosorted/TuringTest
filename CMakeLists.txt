cmake_minimum_required(VERSION 3.14)
project(TuringTest C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(raylib)

add_executable(turing_test main.c scenes/test_scene.c)
target_link_libraries(turing_test PRIVATE raylib)
