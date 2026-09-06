#include "raylib.h"
#include "scenes/test_scene.h"

int main(void) {
    InitWindow(800, 450, "Turing Test");
    InitTestScene();

    while (!WindowShouldClose()) {
        UpdateDrawTestScene();
    }

    CloseWindow();
    return 0;
}
