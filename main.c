#include <raylib.h>
#include "scenes/menu_scene.h"
#include "scenes/selection_scene.h"
#include "scenes/gameplay_scene.h"

// Lista das telas que o jogo tem
typedef enum {
    SCENE_MENU,
    SCENE_SELECTION,   // escolher cenario -> escolher funcionario
    SCENE_GAMEPLAY     // tela de interrogatorio
} Scene;

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 360, "Turing Test");

    Scene currentScene = SCENE_MENU; // o jogo comeca no menu
    InitMenuScene();

    bool running = true;

    while (running && !WindowShouldClose()) {
        switch (currentScene) {
            case SCENE_MENU:
                UpdateDrawMenuScene();

                if (MenuSceneShouldClose()) {
                    running = false;             // botao Sair
                } else if (MenuSceneShouldStartGame()) {
                    UnloadMenuScene();           // 1) descarrega o menu
                    InitSelectionScene();        // 2) carrega a selecao
                    currentScene = SCENE_SELECTION; // 3) troca de tela
                }
                break;

            case SCENE_SELECTION:
                UpdateDrawSelectionScene();

                if (SelectionSceneShouldStartInterrogation()) {
                    // Funcionario escolhido (por enquanto so fica disponivel;
                    // a gameplay ainda nao usa essa informacao)
                    // Employee escolhido = SelectionSceneGetEmployee();
                    UnloadSelectionScene();      // 1) descarrega a selecao
                    InitGameplayScene();         // 2) carrega o interrogatorio
                    currentScene = SCENE_GAMEPLAY;  // 3) troca de tela
                }
                break;

                        case SCENE_GAMEPLAY:
                UpdateDrawGameplayScene();

                if (GameplaySceneShouldClose()) {
                    running = false;
                } else if (GameplaySceneGoToSelection()) {
                    UnloadGameplayScene();          // 1) descarrega o interrogatorio
                    InitSelectionScene();           // 2) carrega a selecao
                    currentScene = SCENE_SELECTION; // 3) troca de tela
                }
                break;
        }
    }

    // Descarrega a tela que estiver aberta no momento
    switch (currentScene) {
        case SCENE_MENU:      UnloadMenuScene();      break;
        case SCENE_SELECTION: UnloadSelectionScene(); break;
        case SCENE_GAMEPLAY:  UnloadGameplayScene();  break;
    }

    CloseWindow();
    return 0;
}