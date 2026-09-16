#include "raylib.h"
#include "menu_scene.h"
#include "../utils/raycanvas.h"

#define NUM_BUTTONS 4

typedef struct {
    Texture2D texture;
    Rectangle bounds;
    bool isHovered;
} MenuButton;

static Texture2D g_bgTexture;
static Texture2D g_titleTexture;
static Texture2D g_polaroidLeftTex;
static Texture2D g_polaroidRightTex;

static MenuButton g_buttons[NUM_BUTTONS];
static const char *g_buttonPaths[NUM_BUTTONS] = {
    "assets/menu_scene/button_play.png",
    "assets/menu_scene/button_load.png",
    "assets/menu_scene/button_options.png",
    "assets/menu_scene/button_exit.png"
};

static bool g_shouldStartGame = false;
// Funcoes de clique de cada botao
static void OnClickPlay(void) {
    TraceLog(LOG_INFO, "Botao JOGAR clicado!");
    g_shouldStartGame = true;
}

static void OnClickLoad(void) {
    TraceLog(LOG_INFO, "Botao CARREGAR clicado!");
}

static void OnClickOptions(void) {
    TraceLog(LOG_INFO, "Botao OPCOES clicado!");
}

static bool g_shouldClose = false;

static void OnClickExit(void) {
    TraceLog(LOG_INFO, "Botao SAIDA clicado! Encerrando o jogo.");
    g_shouldClose = true;
}

bool MenuSceneShouldClose(void) {
    return g_shouldClose;
}

bool MenuSceneShouldStartGame(void) {
    return g_shouldStartGame;
}

void InitMenuScene(void) {
    g_shouldClose = false;
    g_shouldStartGame = false;
    g_bgTexture = LoadTexture("assets/menu_scene/background.png");
    SetTextureFilter(g_bgTexture, TEXTURE_FILTER_POINT);
    RayCanvasInit(g_bgTexture.width, g_bgTexture.height);

    g_titleTexture = LoadTexture("assets/menu_scene/title.png");
    SetTextureFilter(g_titleTexture, TEXTURE_FILTER_POINT);

    g_polaroidLeftTex = LoadTexture("assets/menu_scene/polaroid_left.png");
    SetTextureFilter(g_polaroidLeftTex, TEXTURE_FILTER_POINT);

    g_polaroidRightTex = LoadTexture("assets/menu_scene/polaroid_right.png");
    SetTextureFilter(g_polaroidRightTex, TEXTURE_FILTER_POINT);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        g_buttons[i].texture = LoadTexture(g_buttonPaths[i]);
        SetTextureFilter(g_buttons[i].texture, TEXTURE_FILTER_POINT);
        g_buttons[i].isHovered = false;
    }
}

static void UpdateMenuButtons(void) {
    Vector2 mousePos = RayCanvasGetMousePosition();
    int spacing = 11;
    float bottomMargin = 40.0f;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        int stepsFromBottom = (NUM_BUTTONS - 1) - i;
        float yOffset = -bottomMargin - (float)(stepsFromBottom * (39 + spacing));

        g_buttons[i].bounds = RayCanvasGetRect(
            (Vector2){ 0.5f, 1.0f },
            (Vector2){ 0.5f, 1.0f },
            (Vector2){ 0.0f, yOffset },
            (Vector2){ (float)g_buttons[i].texture.width, (float)g_buttons[i].texture.height }
        );

        g_buttons[i].isHovered = CheckCollisionPointRec(mousePos, g_buttons[i].bounds);

        // Dispara o metodo correspondente ao botao clicado de forma direta (press ou release)
        if (g_buttons[i].isHovered && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
            if (i == 0) OnClickPlay();
            else if (i == 1) OnClickLoad();
            else if (i == 2) OnClickOptions();
            else if (i == 3) OnClickExit();
        }
    }
}

static void DrawMenuDecorations(void) {
    Rectangle polaroidLeftRect = RayCanvasGetRect(
        (Vector2){ 0.0f, 1.0f },
        (Vector2){ 0.0f, 1.0f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)g_polaroidLeftTex.width, (float)g_polaroidLeftTex.height }
    );

    Rectangle polaroidRightRect = RayCanvasGetRect(
        (Vector2){ 1.0f, 1.0f },
        (Vector2){ 1.0f, 1.0f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)g_polaroidRightTex.width, (float)g_polaroidRightTex.height }
    );

    Rectangle titleRect = RayCanvasGetRect(
        (Vector2){ 0.5f, 0.0f },
        (Vector2){ 0.5f, 0.0f },
        (Vector2){ 0.0f, 24.0f },
        (Vector2){ (float)g_titleTexture.width, (float)g_titleTexture.height }
    );

    RayCanvasDrawTexture(g_polaroidLeftTex, polaroidLeftRect, WHITE);
    RayCanvasDrawTexture(g_polaroidRightTex, polaroidRightRect, WHITE);
    RayCanvasDrawTexture(g_titleTexture, titleRect, WHITE);
}

static void DrawMenuButtons(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Color tint = g_buttons[i].isHovered ? (Color){ 180, 180, 180, 255 } : WHITE;
        RayCanvasDrawTexture(g_buttons[i].texture, g_buttons[i].bounds, tint);
    }
}

void UpdateDrawMenuScene(void) {
    RayCanvasBegin();

    RayCanvasDrawTiledBackground(g_bgTexture);
    DrawMenuDecorations();
    UpdateMenuButtons();
    DrawMenuButtons();

    RayCanvasEnd();
}

void UnloadMenuScene(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        UnloadTexture(g_buttons[i].texture);
    }
    UnloadTexture(g_polaroidLeftTex);
    UnloadTexture(g_polaroidRightTex);
    UnloadTexture(g_titleTexture);
    UnloadTexture(g_bgTexture);
    RayCanvasClose();
}
