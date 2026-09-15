#include <raylib.h>
#include "scenes/menu_scene.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 360, "Turing Test");
    InitMenuScene();

    while (!WindowShouldClose() && !MenuSceneShouldClose()) {
        UpdateDrawMenuScene();
    }

    UnloadMenuScene();
    CloseWindow();
    return 0;
}
