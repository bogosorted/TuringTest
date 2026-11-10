#include <raylib.h>
#include <stddef.h>
#include "scenes/menu_scene.h"
#include "scenes/selection_scene.h"
#include "scenes/gameplay_scene.h"
#include "scenes/verdict_scene.h"
#include "scenes/intro_scene.h"
#include "utils/game_state.h"
#include "utils/dialog_system.h"
#include "utils/transition.h"
#include "utils/raycanvas.h"
#include "utils/settings.h"
#include <math.h>

// Lista das telas que o jogo tem
typedef enum {
    SCENE_MENU,
    SCENE_INTRO,       // texto de contexto (antes do Dia 1)
    SCENE_SELECTION,   // escolher cenario
    SCENE_GAMEPLAY,    // tela de interrogatorio
    SCENE_VERDICT      // "qual deles e a IA?" (depois do ultimo dia)
} Scene;

static void InitScene(Scene scene) {
    switch (scene) {
        case SCENE_MENU:      InitMenuScene();      break;
        case SCENE_INTRO:     InitIntroScene();     break;
        case SCENE_SELECTION: InitSelectionScene(); break;
        case SCENE_GAMEPLAY:  InitGameplayScene();  break;
        case SCENE_VERDICT:   InitVerdictScene();   break;
    }
}

static void UnloadScene(Scene scene) {
    switch (scene) {
        case SCENE_MENU:      UnloadMenuScene();      break;
        case SCENE_INTRO:     UnloadIntroScene();     break;
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
// (texto NULL = so o fade-out, a cena nova faz o proprio fade-in)
static void GoToWithTransition(Scene destino, const char *texto) {
    nextScene = destino;
    TransitionStart(texto);
}

int main(void) {
    // Garante que o jogo encontre os arquivos em assets/ mesmo se rodar de build/
    ChangeDirectory(GetApplicationDirectory());
    if (!DirectoryExists("assets") && DirectoryExists("../assets")) {
        ChangeDirectory("..");
    }
    SettingsLoad();   // volumes e paleta salvos no settings.cfg

    // Modo janela maximizada sem bordas (borderless windowed real no Linux/X11):
    // Nao usamos ToggleBorderlessWindowed() porque no Linux a raylib 5.5 chama
    // glfwSetWindowMonitor(..., monitor, ...) que trava o Alt+Tab em fullscreen exclusivo!
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_UNDECORATED | FLAG_VSYNC_HINT);
    InitWindow(640, 360, "Turing Test");
    InitAudioDevice(); // Inicializa dispositivo de som da raylib
    SetExitKey(0); // desabilita ESC fechar a janela direto; tratamos saida de segurança no loop
    SettingsApplyWindowMode();   // tela cheia ou janela, conforme o settings.cfg
    SetTargetFPS(60);

    // Música principal do jogo (toca continuamente com volume ajustado por cena e transição)
    Music bgm = { 0 };
    bool bgmCarregada = false;
    if (FileExists("assets/audio/bgm_main.mp3")) {
        bgm = LoadMusicStream("assets/audio/bgm_main.mp3");
        bgm.looping = true;
        PlayMusicStream(bgm);
        SetMusicVolume(bgm, 0.0f);
        bgmCarregada = true;
    }
    float volumeBgmAtual = 0.0f;

    Scene currentScene = SCENE_MENU; // o jogo comeca no menu
    InitScene(currentScene);
    TransitionStartFadeIn();         // fade-in suave ao abrir o jogo

    bool running = true;

    while (running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // Atualização e controle dinâmico da música principal
        if (bgmCarregada) {
            UpdateMusicStream(bgm);

            // Volume base: mais alto no menu/intro/veredito (0.60), mais baixo no gameplay (0.28)
            float baseVol = (currentScene == SCENE_GAMEPLAY) ? 0.28f : 0.60f;

            // Durante transições (fade e tela de texto entre dias), atenua o volume
            float fade = RayCanvasGetFade();
            float fatorTransicao = 1.0f - (fade * 0.75f);
            if (TransitionShowingText()) fatorTransicao = 0.20f;
            if (fatorTransicao < 0.15f)  fatorTransicao = 0.15f;

            float targetVol = baseVol * fatorTransicao * SettingsGetMusicVolume();
            volumeBgmAtual += (targetVol - volumeBgmAtual) * fminf(dt * 5.0f, 1.0f);
            SetMusicVolume(bgm, volumeBgmAtual);
        }
        // Atalho de emergencia: Ctrl+Q ou F10 fecha o jogo imediatamente de qualquer lugar,
        // e F11 alterna entre tela cheia e modo janela (o mesmo da opcao "Tela cheia" do menu)
        if (IsKeyPressed(KEY_F10) || ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Q))) {
            break;
        }
        if (IsKeyPressed(KEY_F11)) {
            SettingsSetFullscreen(!SettingsGetFullscreen());
            SettingsApplyWindowMode();
            SettingsSave();
        }

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
                    GoToWithTransition(SCENE_INTRO, NULL);   // fade-out -> texto de contexto
                }
                break;

            case SCENE_INTRO:
                UpdateDrawIntroScene();

                if (IntroSceneShouldContinue() && !TransitionActive()) {
                    GoToWithTransition(SCENE_GAMEPLAY,
                                       TextFormat("Dia %d", GameStateGetDay()));
                }
                break;

            case SCENE_SELECTION:
                UpdateDrawSelectionScene();

                if (SelectionSceneShouldStartInterrogation() && !TransitionActive()) {
                    GoToWithTransition(SCENE_GAMEPLAY, NULL);
                } else if (SelectionSceneShouldAdvanceDay() && !TransitionActive()) {
                    GameStateNextDay();
                    ResetTokens();                   // tokens do novo dia
                    GameplaySceneResetDialogues();
                    GoToWithTransition(SCENE_GAMEPLAY,
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
                } else if (GameplaySceneShouldReturnToMenu() && !TransitionActive()) {
                    GoToWithTransition(SCENE_MENU, NULL);
                } else if (GameplaySceneShouldAdvanceDay() && !TransitionActive()) {
                    GameStateNextDay();
                    ResetTokens();
                    GameplaySceneResetDialogues();
                    GoToWithTransition(SCENE_GAMEPLAY,
                                       TextFormat("Dia %d", GameStateGetDay()));
                } else if (GameplaySceneShouldGoToVerdict() && !TransitionActive()) {
                    GoToWithTransition(SCENE_VERDICT,
                                       TextFormat("Dia %d - Veredito", GAME_NUM_DAYS + 1));
                } else if (GameplaySceneGoToSelection() && !TransitionActive()) {
                    GoToWithTransition(SCENE_SELECTION, NULL);
                }
                break;

            case SCENE_VERDICT:
                UpdateDrawVerdictScene();

                if (VerdictSceneShouldRestart() && !TransitionActive()) {
                    GoToWithTransition(SCENE_MENU, NULL);     // "Tentar de novo" com fade
                }
                break;
        }
    }

    // Descarrega a tela que estiver aberta no momento
    UnloadScene(currentScene);

    if (bgmCarregada) {
        UnloadMusicStream(bgm);
    }
    CloseAudioDevice(); // Encerra dispositivo de som
    RayCanvasShutdown();
    CloseWindow();
    return 0;
}
