#include "raylib.h"
#include "test_scene.h"

void InitTestScene(void) {
}

void UpdateDrawTestScene(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Hello World", 300, 200, 30, BLACK);
    EndDrawing();
}
