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

add_executable(turing_test
    src/main.c
    src/core/game_state.c
    src/core/settings.c
    src/core/ranking.c
    src/core/transition.c
    src/core/raycanvas.c
    src/core/ui_common.c
    src/scenes/menu_scene.c
    src/scenes/intro_scene.c
    src/scenes/selection_scene.c
    src/scenes/gameplay_scene.c
    src/gameplay/gameplay_data.c
    src/gameplay/gameplay_logic_hint.c
    src/scenes/verdict_scene.c
    src/systems/dialog_system.c
    src/systems/dialog_data.c
    src/systems/webcam.c
    src/systems/mural_ui.c
    src/systems/options_ui.c
    src/systems/evidencia_holograma.c
    src/systems/mapa_holograma.c
    src/systems/setas_3d.c
    src/systems/player_character.c
    src/world/terreo_3d.c
    src/world/sala_recepcao.c
    src/world/sala_lanchonete.c
    src/world/sala_jardim.c
    src/world/sala_escritorio.c
    src/world/sala_escritorio_2.c
    src/world/sala_convivio.c
    src/world/sala_hall_1.c
    src/world/sala_contabilidade_1.c
    src/world/sala_contabilidade_2.c
    src/world/sala_arquivo_1.c
    src/world/sala_copa_1.c
    src/world/sala_hall_2.c
    src/world/sala_ciber_1.c
    src/world/sala_ciber_2.c
    src/world/sala_server_1.c
    src/world/sala_server_2.c
    src/world/sala_hall_3.c
    src/world/sala_treino_escritorio.c
    src/world/sala_treino_ia.c
    src/world/sala_garagem_prototipos.c
    src/world/sala_hall_4.c
    src/world/sala_auditorio.c
    src/world/sala_reuniao_4.c
    src/world/sala_subsolo.c
)
target_include_directories(turing_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/scenes
    ${CMAKE_SOURCE_DIR}/src/gameplay
    ${CMAKE_SOURCE_DIR}/src/systems
    ${CMAKE_SOURCE_DIR}/src/world
)
target_link_libraries(turing_test PRIVATE raylib)
if(WIN32)
    target_link_libraries(turing_test PRIVATE mf mfplat mfreadwrite mfuuid ole32)
endif()

if(WIN32)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR})
else()
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src SYMBOLIC)
endif()
