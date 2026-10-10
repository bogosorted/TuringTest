#include "raylib.h"
#include "menu_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include <string.h>
#include <ctype.h>

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

// ---------------------------------------------------------------
// MODO DESENVOLVEDOR (testes)
// Digitar "treloso" no menu revela o botao "Tokens Infinitos",
// que liga/desliga global_infinite_tokens. Antes disso o botao nem
// e desenhado nem recebe clique.
// ---------------------------------------------------------------
#define DEV_CODE      "treloso"
#define DEV_CODE_LEN  7

static bool g_devUnlocked = false;                 // fica liberado ate fechar o jogo
static char g_devTyped[DEV_CODE_LEN + 1] = { 0 };  // ultimas letras digitadas

static void UpdateDevCode(void) {
    int c;
    while ((c = GetCharPressed()) != 0) {
        // Empurra o buffer uma casa e coloca a nova letra no final
        memmove(g_devTyped, g_devTyped + 1, DEV_CODE_LEN - 1);
        g_devTyped[DEV_CODE_LEN - 1] = (c < 128) ? (char)tolower(c) : '?';

        if (strncmp(g_devTyped, DEV_CODE, DEV_CODE_LEN) == 0) {
            g_devUnlocked = true;
            TraceLog(LOG_INFO, "Modo desenvolvedor liberado");
        }
    }
}

static void UpdateDrawDevButton(void) {
    if (!g_devUnlocked) return;

    float scale = RayCanvasGetUIScale();

    // Meio da direita da tela
    Rectangle area = RayCanvasGetRect(
        (Vector2){ 1.0f, 0.5f },
        (Vector2){ 1.0f, 0.5f },
        (Vector2){ -10.0f, 0.0f },
        (Vector2){ 120.0f, 22.0f }
    );

    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), area);

    // So "Pressed": com Pressed || Released ele ligaria e desligaria no mesmo clique
    if (hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        global_infinite_tokens = !global_infinite_tokens;
        TraceLog(LOG_INFO, "Tokens infinitos: %s", global_infinite_tokens ? "LIGADO" : "DESLIGADO");
    }

    DrawRectangleRec(area, hover ? (Color){ 220, 220, 220, 255 } : WHITE);

    int fonte = (int)(10 * scale);
    if (fonte < 1) fonte = 1;
    const char *label = "Tokens Infinitos";
    int larguraLabel = MeasureText(label, fonte);
    DrawText(label,
             (int)(area.x + (area.width - larguraLabel) / 2.0f),
             (int)(area.y + (area.height - fonte) / 2.0f),
             fonte, BLACK);

    // Estado atual, pequeno, logo abaixo do botao
    const char *estado = global_infinite_tokens ? "ligado" : "desligado";
    int fonteEstado = (int)(8 * scale);
    if (fonteEstado < 1) fonteEstado = 1;
    int larguraEstado = MeasureText(estado, fonteEstado);
    DrawText(estado,
             (int)(area.x + (area.width - larguraEstado) / 2.0f),
             (int)(area.y + area.height + 4.0f * scale),
             fonteEstado, WHITE);
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
        if (!RayCanvasInputBlocked() && g_buttons[i].isHovered && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
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

    UpdateDevCode();
    UpdateDrawDevButton();

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
