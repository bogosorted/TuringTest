#include <raylib.h>
#include "scenes/menu_scene.h"
#include "scenes/selection_scene.h"
#include "scenes/gameplay_scene.h"
#include "scenes/verdict_scene.h"
#include "utils/game_state.h"
#include "utils/dialog_system.h"
#include "utils/transition.h"

// Lista das telas que o jogo tem
typedef enum {
    SCENE_MENU,
    SCENE_SELECTION,   // escolher cenario
    SCENE_GAMEPLAY,    // tela de interrogatorio
    SCENE_VERDICT      // "qual deles e a IA?" (depois do ultimo dia)
} Scene;

static void InitScene(Scene scene) {
    switch (scene) {
        case SCENE_MENU:      InitMenuScene();      break;
        case SCENE_SELECTION: InitSelectionScene(); break;
        case SCENE_GAMEPLAY:  InitGameplayScene();  break;
        case SCENE_VERDICT:   InitVerdictScene();   break;
    }
}

static void UnloadScene(Scene scene) {
    switch (scene) {
        case SCENE_MENU:      UnloadMenuScene();      break;
        case SCENE_SELECTION: UnloadSelectionScene(); break;
        case SCENE_GAMEPLAY:  UnloadGameplayScene();  break;
        case SCENE_VERDICT:   UnloadVerdictScene();   break;
    }
}

// Troca de tela na hora (sem fade): descarrega a atual e carrega a proxima
static void SwitchScene(Scene *current, Scene next) {
    UnloadScene(*current);
    InitScene(next);
    *current = next;
}

// Para onde vamos quando o fade-out da transicao terminar
static Scene nextScene = SCENE_MENU;

// Fade-out -> tela preta com texto (3s) -> nextScene
static void GoToWithTransition(Scene destino, const char *texto) {
    nextScene = destino;
    TransitionStart(texto);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 360, "Turing Test");
    ToggleFullscreen();

    Scene currentScene = SCENE_MENU; // o jogo comeca no menu
    InitScene(currentScene);

    bool running = true;

    while (running && !WindowShouldClose()) {

        // 1) Transicao: quando o fade-out termina, a troca de cena acontece
        //    "por tras" da tela preta
        if (TransitionUpdate() == TRANSITION_FADE_OUT_DONE) {
            SwitchScene(&currentScene, nextScene);
        }

        // Durante o texto ("Dia 1" etc.) so a tela preta e desenhada
        if (TransitionShowingText()) {
            TransitionDrawTextScreen();
            continue;
        }

        // 2) Cena atual
        switch (currentScene) {
            case SCENE_MENU:
                UpdateDrawMenuScene();

                if (MenuSceneShouldClose()) {
                    running = false;             // botao Sair
                } else if (MenuSceneShouldStartGame() && !TransitionActive()) {
                    GameStateNewRun();               // volta para o dia 1
                    ResetTokens();                   // tokens do dia 1
                    GameplaySceneResetDialogues();   // dialogos "nao vistos" de novo
                    GoToWithTransition(SCENE_SELECTION,
                                       TextFormat("Dia %d", GameStateGetDay()));
                }
                break;

            case SCENE_SELECTION:
                UpdateDrawSelectionScene();

                if (SelectionSceneShouldStartInterrogation()) {
                    SwitchScene(&currentScene, SCENE_GAMEPLAY);
                } else if (SelectionSceneShouldAdvanceDay() && !TransitionActive()) {
                    GameStateNextDay();
                    ResetTokens();                   // tokens do novo dia
                    GameplaySceneResetDialogues();
                    GoToWithTransition(SCENE_SELECTION,
                                       TextFormat("Dia %d", GameStateGetDay()));
                } else if (SelectionSceneShouldGoToVerdict() && !TransitionActive()) {
                    GoToWithTransition(SCENE_VERDICT,
                                       TextFormat("Dia %d - Veredito", GAME_NUM_DAYS + 1));
                }
                break;

            case SCENE_GAMEPLAY:
                UpdateDrawGameplayScene();

                if (GameplaySceneShouldClose()) {
                    running = false;
                } else if (GameplaySceneGoToSelection()) {
                    SwitchScene(&currentScene, SCENE_SELECTION);
                }
                break;

            case SCENE_VERDICT:
                UpdateDrawVerdictScene();

                if (VerdictSceneShouldRestart()) {
                    SwitchScene(&currentScene, SCENE_MENU);   // "Tentar de novo"
                }
                break;
        }
    }

    // Descarrega a tela que estiver aberta no momento
    UnloadScene(currentScene);

    CloseWindow();
    return 0;
}
