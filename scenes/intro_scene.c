#include "raylib.h"
#include "intro_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/game_state.h"
#include <stdio.h>

// Mesmo espaco virtual das outras cenas (640x360)
#define VIRTUAL_W 640
#define VIRTUAL_H 360

#define TEXT_FONT      16     // tamanho da fonte do texto (espaco virtual)
#define TEXT_W         520    // largura maxima de uma linha (espaco virtual)
#define LINE_SPACING   1.5f   // altura da linha = fonte * isso
#define FADE_IN_TIME   1.2f   // segundos para o texto (e o botao) aparecerem

#define BTN_W          130
#define BTN_H          28
#define BTN_FONT       14
#define BTN_MARGIN     12

#define MAX_LINHAS     24
#define LINHA_LEN      160
#define NUM_PARAGRAFOS 3

// Texto de contexto, um paragrafo por item
static char g_ultimoParagrafo[64];
static const char *g_paragrafos[NUM_PARAGRAFOS] = {
    "Você é um investigador secreto contratado pela empresa de tecnologia RASEC, "
    "e deve investigar 3 funcionários da empresa para descobrir qual deles é uma "
    "IA mal-intencionada.",

    "Você assumiu a identidade de um estagiário novato para se misturar no "
    "ambiente. Vá e impeça que a IA se prolifere e substitua outros funcionários.",

    NULL   // "Você tem N dias." (montado no Init, usa GAME_NUM_DAYS)
};

static char  g_linhas[MAX_LINHAS][LINHA_LEN];   // texto ja quebrado em linhas
static float g_tempo = 0.0f;                    // segundos desde que a cena abriu
static bool  g_prosseguir = false;

static Rectangle GetTelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)VIRTUAL_W, (float)VIRTUAL_H }
    );
}

static Rectangle ParaTela(Rectangle r) {
    Rectangle tela = GetTelaRect();
    float escalaX = tela.width  / (float)VIRTUAL_W;
    float escalaY = tela.height / (float)VIRTUAL_H;

    return (Rectangle){
        tela.x + r.x * escalaX,
        tela.y + r.y * escalaY,
        r.width  * escalaX,
        r.height * escalaY
    };
}

static float Escala(void) {
    return GetTelaRect().width / (float)VIRTUAL_W;
}

// Quebra os paragrafos em linhas que caibam em 'larguraMax' (pixels reais),
// deixando uma linha em branco entre paragrafos. Devolve quantas linhas.
static int BuildLines(int fonte, int larguraMax) {
    int n = 0;

    for (int p = 0; p < NUM_PARAGRAFOS && n < MAX_LINHAS; p++) {
        if (p > 0 && n < MAX_LINHAS) g_linhas[n++][0] = '\0';   // linha em branco

        const char *s = g_paragrafos[p];
        char atual[LINHA_LEN] = "";

        while (s && *s && n < MAX_LINHAS) {
            // Pega a proxima palavra
            char palavra[64];
            int len = 0;
            while (*s == ' ') s++;
            while (*s && *s != ' ' && len < 63) palavra[len++] = *s++;
            palavra[len] = '\0';
            if (len == 0) break;

            char tentativa[LINHA_LEN];
            snprintf(tentativa, sizeof tentativa, "%s%s%s", atual, atual[0] ? " " : "", palavra);

            if (atual[0] && MeasureText(tentativa, fonte) > larguraMax) {
                // Nao coube: fecha a linha atual e comeca outra com a palavra
                snprintf(g_linhas[n++], LINHA_LEN, "%s", atual);
                snprintf(atual, sizeof atual, "%s", palavra);
            } else {
                snprintf(atual, sizeof atual, "%s", tentativa);
            }
        }
        if (atual[0] && n < MAX_LINHAS) snprintf(g_linhas[n++], LINHA_LEN, "%s", atual);
    }
    return n;
}

bool IntroSceneShouldContinue(void) {
    return g_prosseguir;
}

void InitIntroScene(void) {
    g_tempo = 0.0f;
    g_prosseguir = false;

    snprintf(g_ultimoParagrafo, sizeof g_ultimoParagrafo, "Você tem %d dias.", GAME_NUM_DAYS);
    g_paragrafos[NUM_PARAGRAFOS - 1] = g_ultimoParagrafo;

    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

void UpdateDrawIntroScene(void) {
    RayCanvasBegin();

    g_tempo += GetFrameTime();
    float alpha = g_tempo / FADE_IN_TIME;
    if (alpha > 1.0f) alpha = 1.0f;

    // Tela preta de verdade (o fundo padrao do canvas e cinza escuro)
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    float escala = Escala();
    Rectangle tela = GetTelaRect();
    Color branco = Fade(WHITE, alpha);

    // Texto centralizado na tela (horizontal e vertical)
    int fonte = (int)(TEXT_FONT * escala);
    int alturaLinha = (int)(fonte * LINE_SPACING);
    int n = BuildLines(fonte, (int)(TEXT_W * escala));
    int y = (int)(tela.y + (tela.height - n * alturaLinha) / 2.0f);

    for (int i = 0; i < n; i++) {
        if (g_linhas[i][0]) {
            int largura = MeasureText(g_linhas[i], fonte);
            DrawText(g_linhas[i], (int)(tela.x + (tela.width - largura) / 2.0f), y, fonte, branco);
        }
        y += alturaLinha;
    }

    // Botao "prosseguir ->" no canto inferior direito.
    // So reage depois do fade-in terminar (evita clique sem querer logo ao entrar).
    Rectangle area = ParaTela((Rectangle){
        VIRTUAL_W - BTN_W - BTN_MARGIN,
        VIRTUAL_H - BTN_H - BTN_MARGIN,
        (float)BTN_W,
        (float)BTN_H
    });

    bool pronto = (alpha >= 1.0f);
    bool hover = pronto && CheckCollisionPointRec(RayCanvasGetMousePosition(), area);

    if (hover) DrawRectangleRec(area, branco);
    DrawRectangleLinesEx(area, 2.0f * escala, branco);

    const char *label = "prosseguir ->";
    int tamanho = (int)(BTN_FONT * escala);
    int largura = MeasureText(label, tamanho);
    DrawText(label,
             (int)(area.x + (area.width - largura) / 2.0f),
             (int)(area.y + (area.height - tamanho) / 2.0f),
             tamanho, hover ? BLACK : branco);

    if (hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_prosseguir = true;
    }

    SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    RayCanvasEnd();
}

void UnloadIntroScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    RayCanvasClose();
}
