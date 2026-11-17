cmake_minimum_required(VERSION 3.14)

find_program(CCACHE_PROGRAM ccache HINTS "$ENV{HOME}/.local/bin")
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

project(TuringTest C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

if(UNIX AND NOT APPLE)
    find_program(MOLD_LINKER mold HINTS "$ENV{HOME}/.local/bin")
    if(MOLD_LINKER)
        add_link_options("-fuse-ld=mold")
    endif()
endif()

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip network checks when dependencies are already populated")
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(turing_test main.c scenes/menu_scene.c scenes/gameplay_scene.c scenes/selection_scene.c utils/raycanvas.c utils/dialog_system.c utils/dialog_data.c utils/transition.c utils/game_state.c utils/settings.c utils/options_ui.c utils/ranking.c utils/mapa_holograma.c utils/setas_3d.c scenes/verdict_scene.c scenes/intro_scene.c)
target_link_libraries(turing_test PRIVATE raylib)

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
