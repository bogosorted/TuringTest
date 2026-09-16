#include <raylib.h>
#include "scenes/menu_scene.h"
#include "scenes/gameplay_scene.h"

// Lista das telas que o jogo tem
typedef enum {
    SCENE_MENU,
    SCENE_GAMEPLAY
} Scene;

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 360, "Turing Test");

    Scene currentScene = SCENE_MENU;  // o jogo comeca no menu
    InitMenuScene();

    bool running = true;

    while (running && !WindowShouldClose()) {
        switch (currentScene) {
            case SCENE_MENU:
                UpdateDrawMenuScene();

                if (MenuSceneShouldClose()) {
                    running = false;              // botao Sair
                } else if (MenuSceneShouldStartGame()) {
                    UnloadMenuScene();            // 1) descarrega o menu
                    InitGameplayScene();          // 2) carrega a gameplay
                    currentScene = SCENE_GAMEPLAY; // 3) troca de tela
                }
                break;

            case SCENE_GAMEPLAY:
                UpdateDrawGameplayScene();

                if (GameplaySceneShouldClose()) {
                    running = false;
                }
                break;
        }
    }

    // Descarrega a tela que estiver aberta no momento
    if (currentScene == SCENE_MENU) {
        UnloadMenuScene();
    } else {
        UnloadGameplayScene();
    }

    CloseWindow();
    return 0;
}