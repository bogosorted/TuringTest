#include "raylib.h"
#include "intro_scene.h"
#include "settings.h"
#include "raycanvas.h"
#include "game_state.h"
#include "ui_common.h"
#include <stdio.h>

// Mesmo espaco virtual das outras cenas (640x360)
#define VIRTUAL_W UI_VIRTUAL_W
#define VIRTUAL_H UI_VIRTUAL_H

#define TEXT_FONT      16     // tamanho da fonte do texto (espaco virtual)
#define TEXT_W         520    // largura maxima de uma linha (espaco virtual)
#define LINE_SPACING   1.5f   // altura da linha = fonte * isso
#define FADE_IN_TIME   1.2f   // segundos para o texto (e o botao) aparecerem

#define BTN_W          130
#define BTN_H          28
#define BTN_FONT       14
#define BTN_MARGIN     12

#define MAX_LINHAS     24
#define LINHA_LEN      256
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

static UIBgShader g_bgShader = { 0 };
static Sound      g_sndHover = { 0 };
static bool       g_btnHoverAnt = false;

#define GetTelaRect UIGetTelaRect
#define ParaTela    UIParaTela
#define Escala      UIGetEscala

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
            if (atual[0]) snprintf(tentativa, sizeof(tentativa), "%.190s %s", atual, palavra);
            else snprintf(tentativa, sizeof(tentativa), "%s", palavra);

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

bool InitIntroScene(void) {
    g_tempo = 0.0f;
    g_prosseguir = false;

    snprintf(g_ultimoParagrafo, sizeof g_ultimoParagrafo, "Você tem %d dias.", GAME_NUM_DAYS);
    g_paragrafos[NUM_PARAGRAFOS - 1] = g_ultimoParagrafo;

    if (FileExists("assets/audio/sfx_hover.wav")) {
        g_sndHover = LoadSound("assets/audio/sfx_hover.wav");
    }
    g_btnHoverAnt = false;

    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
    if (!UIBgShaderInit(&g_bgShader)) {
        TraceLog(LOG_ERROR, "Falha ao carregar recursos essenciais da introducao");
        UnloadIntroScene();
        return false;
    }
    return true;
}

void UpdateDrawIntroScene(void) {
    RayCanvasBegin();

    g_tempo += GetFrameTime();
    float alpha = g_tempo / FADE_IN_TIME;
    if (alpha > 1.0f) alpha = 1.0f;

    float escala = Escala();
    Rectangle tela = GetTelaRect();

    // Cores oficiais da paleta do menu
    Color corFundo    = (Color){  32,  29,  31, 255 };
    Color corPainel   = (Color){  90,  83,  83, 255 };
    Color corBorda    = (Color){ 160, 147, 142, 255 };
    Color corTexto    = Fade((Color){ 189, 181, 168, 255 }, alpha);
    Color corDestaque = Fade((Color){ 255, 242, 230, 255 }, alpha);
    Color corBtnFundo = Fade((Color){ 125, 112, 113, 255 }, alpha);
    Color corBtnHover = Fade((Color){ 160, 147, 142, 255 }, alpha);

    // Fundo animado com o mesmo shader e ruido procedural do menu
    UIBgShaderDraw(&g_bgShader, (float)GetTime(), WHITE);

    // Sombra suave sob a moldura do dossier para destacar do fundo vivo
    Rectangle sombraMoldura = ParaTela((Rectangle){ 38, 30, VIRTUAL_W - 72, VIRTUAL_H - 56 });
    DrawRectangleRec(sombraMoldura, Fade(BLACK, 0.55f * alpha));

    // Moldura elegante e coesa no estilo dossier da RASEC com translucidez para exibir o shader animado
    Rectangle moldura = ParaTela((Rectangle){ 36, 28, VIRTUAL_W - 72, VIRTUAL_H - 56 });
    DrawRectangleRec(moldura, Fade((Color){ 20, 18, 20, 205 }, alpha));
    DrawRectangleLinesEx(moldura, 2.0f * escala, Fade(corBorda, alpha));

    // Tarja de cabecalho estilizada do dossie
    Rectangle tarjaCab = ParaTela((Rectangle){ 36, 28, VIRTUAL_W - 72, 26 });
    DrawRectangleRec(tarjaCab, Fade((Color){ 18, 16, 18, 255 }, alpha));
    DrawRectangleLinesEx(tarjaCab, 1.5f * escala, Fade(corPainel, alpha));

    const char *cabecalho = "RASEC - DOSSIÊ CONFIDENCIAL // DIRETORIA DE SEGURANÇA";
    int tamCab = (int)(9 * escala);
    DrawText(cabecalho,
             (int)(tela.x + (tela.width - MeasureText(cabecalho, tamCab)) / 2.0f),
             (int)(moldura.y + 7 * escala),
             tamCab, Fade(corDestaque, alpha));

    // Texto centralizado na tela (horizontal e vertical)
    int fonte = (int)(TEXT_FONT * escala);
    int alturaLinha = (int)(fonte * LINE_SPACING);
    int n = BuildLines(fonte, (int)(TEXT_W * escala));
    int y = (int)(tela.y + (tela.height - n * alturaLinha) / 2.0f);

    for (int i = 0; i < n; i++) {
        if (g_linhas[i][0]) {
            int largura = MeasureText(g_linhas[i], fonte);
            Color cLinha = (i == n - 1) ? corDestaque : corTexto;
            DrawText(g_linhas[i], (int)(tela.x + (tela.width - largura) / 2.0f), y, fonte, cLinha);
        }
        y += alturaLinha;
    }

    // Botao "prosseguir ->" no canto inferior direito na paleta do jogo
    Rectangle area = ParaTela((Rectangle){
        VIRTUAL_W - BTN_W - 48,
        VIRTUAL_H - BTN_H - 40,
        (float)BTN_W,
        (float)BTN_H
    });

    bool pronto = (alpha >= 1.0f);
    bool hover = pronto && CheckCollisionPointRec(RayCanvasGetMousePosition(), area);
    if (hover && !g_btnHoverAnt && g_sndHover.frameCount > 0) {
        SetSoundVolume(g_sndHover, 0.32f * SettingsGetSfxVolume());
        PlaySound(g_sndHover);
    }
    g_btnHoverAnt = hover;

    DrawRectangleRec(area, hover ? corBtnHover : corBtnFundo);
    DrawRectangleLinesEx(area, 2.0f * escala, Fade(corFundo, alpha));

    const char *label = "Prosseguir ->";
    int tamanho = (int)(BTN_FONT * escala);
    int largura = MeasureText(label, tamanho);
    DrawText(label,
             (int)(area.x + (area.width - largura) / 2.0f),
             (int)(area.y + (area.height - tamanho) / 2.0f),
             tamanho, Fade(corFundo, alpha));

    if (hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_prosseguir = true;
    }

    SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    RayCanvasEnd();
}

void UnloadIntroScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UIBgShaderUnload(&g_bgShader);
    UISafeUnloadSound(&g_sndHover);
    RayCanvasClose();
}
