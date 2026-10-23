#include "raylib.h"
#include "menu_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include <string.h>
#include <ctype.h>

#define NUM_BUTTONS 3
#define TELA_W 640
#define TELA_H 360

// Animacao do escritorio (GIF 640x360, fica embaixo de tudo)
static Image     g_animImagem;          // todos os quadros do GIF na memoria
static Texture2D g_animTextura;         // quadro que esta sendo mostrado
static int       g_animQuadros = 0;
static int       g_animAtual   = 0;
static float     g_animTempo   = 0.0f;
#define ANIM_DURACAO_QUADRO 0.07f       // 70 ms por quadro (mesma velocidade do GIF)

static Texture2D g_painel;              // painel escuro da direita (562x360)
static Texture2D g_botao;               // fundo dos botoes (305x58)
static Font      g_fonte;               // fonte pixel do menu (VT323)

static const char *g_textosBotoes[NUM_BUTTONS] = { "Jogar", "Opções", "Sair" };
static bool g_hover[NUM_BUTTONS];

// Posicoes em pixels da tela 640x360
#define PAINEL_X     78      // painel encostado na direita (640 - 562)
#define BOTAO_X      203     // botoes saem da borda esquerda do painel
#define BOTAO_Y0     150     // y do primeiro botao
#define BOTAO_PASSO  70      // distancia entre os botoes
#define TEXTO_X      235     // x do texto dentro do botao

static bool g_shouldStartGame = false;
// Funcoes de clique de cada botao
static void OnClickPlay(void) {
    TraceLog(LOG_INFO, "Botao JOGAR clicado!");
    g_shouldStartGame = true;
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

// Retangulo onde a tela 640x360 e desenhada (centralizada na janela)
static Rectangle TelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)TELA_W, (float)TELA_H }
    );
}

// Converte um retangulo em pixels 640x360 para a posicao real na janela
static Rectangle ParaTela(Rectangle r) {
    Rectangle t = TelaRect();
    float escala = t.width / (float)TELA_W;
    return (Rectangle){ t.x + r.x * escala, t.y + r.y * escala, r.width * escala, r.height * escala };
}

// Retangulo do botao i, em pixels 640x360
static Rectangle BotaoRect(int i) {
    return (Rectangle){ BOTAO_X, BOTAO_Y0 + i * BOTAO_PASSO, (float)g_botao.width, (float)g_botao.height };
}

// Desenha um sprite no tamanho original, na posicao (x, y) da tela 640x360
static void DesenharTextura(Texture2D tex, float x, float y, Color cor) {
    Rectangle origem  = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle destino = ParaTela((Rectangle){ x, y, (float)tex.width, (float)tex.height });
    DrawTexturePro(tex, origem, destino, (Vector2){ 0, 0 }, 0.0f, cor);
}

// Escreve um texto com a fonte do menu na posicao (x, y) da tela 640x360
static void Texto(const char *txt, float x, float y, float tam, Color cor) {
    Rectangle r = ParaTela((Rectangle){ x, y, 0, tam });
    DrawTextEx(g_fonte, txt, (Vector2){ r.x, r.y }, r.height, 1.0f, cor);
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
    RayCanvasInit(TELA_W, TELA_H);

    // GIF: a raylib carrega todos os quadros de uma vez
    g_animImagem  = LoadImageAnim("assets/menu_scene/menu_oficial/spr_mainmenu_gif.gif", &g_animQuadros);
    g_animTextura = LoadTextureFromImage(g_animImagem);
    SetTextureFilter(g_animTextura, TEXTURE_FILTER_POINT);
    g_animAtual = 0;
    g_animTempo = 0.0f;

    g_painel = LoadTexture("assets/menu_scene/menu_oficial/spr_mainmenu_fill.png");
    SetTextureFilter(g_painel, TEXTURE_FILTER_POINT);

    g_botao = LoadTexture("assets/menu_scene/menu_oficial/spr_mainmenu_botao.png");
    SetTextureFilter(g_botao, TEXTURE_FILTER_POINT);

    // Fonte pixel. Por padrao a raylib so carrega as letras sem acento,
    // entao passamos a lista de caracteres 32 a 255 (inclui ç, ã, é...)
    int caracteres[224];
    for (int i = 0; i < 224; i++) caracteres[i] = 32 + i;
    g_fonte = LoadFontEx("assets/fonts/VT323-Regular.ttf", 96, caracteres, 224);
    SetTextureFilter(g_fonte.texture, TEXTURE_FILTER_BILINEAR);

    for (int i = 0; i < NUM_BUTTONS; i++) g_hover[i] = false;
}

// Troca o quadro do GIF a cada 70 ms
static void AtualizarAnimacao(void) {
    if (g_animQuadros <= 1) return;

    bool mudou = false;
    g_animTempo += GetFrameTime();
    while (g_animTempo >= ANIM_DURACAO_QUADRO) {
        g_animTempo -= ANIM_DURACAO_QUADRO;
        g_animAtual = (g_animAtual + 1) % g_animQuadros;
        mudou = true;
    }

    if (mudou) {
        // Os quadros ficam um depois do outro na memoria: pula ate o quadro atual
        int bytesPorQuadro = g_animImagem.width * g_animImagem.height * 4;
        UpdateTexture(g_animTextura, ((unsigned char *)g_animImagem.data) + bytesPorQuadro * g_animAtual);
    }
}

static void UpdateMenuButtons(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    bool algumHover = false;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        g_hover[i] = CheckCollisionPointRec(mouse, ParaTela(BotaoRect(i)));
        if (g_hover[i]) algumHover = true;

        if (!RayCanvasInputBlocked() && g_hover[i] &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
            if (i == 0)      OnClickPlay();
            else if (i == 1) OnClickOptions();
            else if (i == 2) OnClickExit();
        }
    }

    SetMouseCursor(algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

// Ordem de desenho: o que vem primeiro fica embaixo
static void DrawMenu(void) {
    Color claro  = { 160, 147, 142, 255 };   // cor da borda do botao
    Color escuro = {  32,  29,  31, 255 };   // cor do painel

    // 1) GIF embaixo de tudo, ocupando a tela inteira
    DesenharTextura(g_animTextura, 0, 0, WHITE);

    // 2) Painel por cima do GIF (o corte transparente deixa a animacao aparecer)
    DesenharTextura(g_painel, PAINEL_X, 0, WHITE);

    // 3) Titulo
    Texto("TESTE DE TURING", 232, 48, 40, claro);

    // 4) Botoes (escurecem no hover) com o texto por cima
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Rectangle b = BotaoRect(i);
        Color tint = g_hover[i] ? (Color){ 180, 180, 180, 255 } : WHITE;
        DesenharTextura(g_botao, b.x, b.y, tint);
        Texto(g_textosBotoes[i], TEXTO_X, b.y + 15, 26, escuro);
    }
}

void UpdateDrawMenuScene(void) {
    RayCanvasBegin();

    AtualizarAnimacao();
    UpdateMenuButtons();
    DrawMenu();

    UpdateDevCode();
    UpdateDrawDevButton();

    RayCanvasEnd();
}

void UnloadMenuScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_animTextura);
    UnloadImage(g_animImagem);      // libera os quadros do GIF
    UnloadTexture(g_painel);
    UnloadTexture(g_botao);
    UnloadFont(g_fonte);
    RayCanvasClose();
}