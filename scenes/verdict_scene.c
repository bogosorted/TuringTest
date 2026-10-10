#include "raylib.h"
#include "verdict_scene.h"
#include "selection_scene.h"      // enum Employee (EMPLOYEE_BETA = a IA)
#include "../utils/raycanvas.h"

// Mesmo espaco virtual das outras cenas (640x360)
#define VIRTUAL_W 640
#define VIRTUAL_H 360

#define TITLE_Y        30
#define TITLE_FONT     24

#define FRAME_W        150
#define FRAME_H        140
#define FRAME_SPACING  25
#define FRAME_TOP      90
#define FRAME_FONT     22

#define RESULT_Y       252
#define RESULT_FONT    28

#define BTN_H          34
#define BTN_FONT       16
#define CONFIRM_W      200
#define RETRY_W        170
#define MARGIN         12

// Quem e a IA infiltrada (placeholder)
#define INDICE_DA_IA   EMPLOYEE_BETA

static Color textColor  = (Color){ 30, 32, 34, 255 };
static Color bgColor    = (Color){ 120, 130, 110, 255 };
static Color hoverColor = (Color){ 150, 160, 130, 255 };
static Color selColor   = (Color){ 190, 200, 150, 255 };
static Color offColor   = (Color){ 80, 85, 75, 255 };

static const char *NOMES[3] = { "Alpha", "Beta", "Gamma" };

static int  g_selecionado = -1;     // -1 = ninguem
static bool g_confirmado = false;
static bool g_acertou = false;
static bool g_tentarDeNovo = false;
static bool g_algumHover = false;

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

// Texto centralizado dentro de um retangulo (ja em pixels reais da tela)
static void DrawTextCentered(const char *texto, Rectangle area, int tamanho, Color cor) {
    int largura = MeasureText(texto, tamanho);
    DrawText(texto,
             (int)(area.x + (area.width - largura) / 2.0f),
             (int)(area.y + (area.height - tamanho) / 2.0f),
             tamanho, cor);
}

// Botao simples. Devolve true se foi clicado neste frame.
// 'ativo' = false desenha o botao apagado e ignora o clique.
static bool DrawButton(Rectangle areaVirtual, const char *label, bool ativo) {
    Rectangle area = ParaTela(areaVirtual);
    float escala = Escala();

    bool hover = ativo && CheckCollisionPointRec(RayCanvasGetMousePosition(), area);
    if (hover) g_algumHover = true;

    Color fundo = !ativo ? offColor : (hover ? hoverColor : bgColor);
    DrawRectangleRec(area, fundo);
    DrawRectangleLinesEx(area, 2.0f * escala, textColor);
    DrawTextCentered(label, area, (int)(BTN_FONT * escala), textColor);

    return hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// As 3 molduras com os nomes. Clicar seleciona (ate confirmar).
static void UpdateDrawFrames(void) {
    float escala = Escala();
    float inicioX = (VIRTUAL_W - (3 * FRAME_W + 2 * FRAME_SPACING)) / 2.0f;

    for (int i = 0; i < 3; i++) {
        Rectangle area = ParaTela((Rectangle){
            inicioX + i * (FRAME_W + FRAME_SPACING),
            (float)FRAME_TOP,
            (float)FRAME_W,
            (float)FRAME_H
        });

        bool hover = !g_confirmado && CheckCollisionPointRec(RayCanvasGetMousePosition(), area);
        if (hover) {
            g_algumHover = true;
            if (!RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g_selecionado = i;
        }

        bool selecionado = (i == g_selecionado);
        Color fundo = selecionado ? selColor : (hover ? hoverColor : bgColor);

        DrawRectangleRec(area, fundo);
        DrawRectangleLinesEx(area, (selecionado ? 5.0f : 2.0f) * escala, textColor);
        DrawTextCentered(NOMES[i], area, (int)(FRAME_FONT * escala), textColor);
    }
}

bool VerdictSceneShouldRestart(void) {
    return g_tentarDeNovo;
}

void InitVerdictScene(void) {
    g_selecionado = -1;
    g_confirmado = false;
    g_acertou = false;
    g_tentarDeNovo = false;
    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

void UpdateDrawVerdictScene(void) {
    RayCanvasBegin();
    g_algumHover = false;
    float escala = Escala();

    // Fundo placeholder (cor lisa)
    DrawRectangleRec(GetTelaRect(), textColor);

    // Pergunta no topo, centralizada
    DrawTextCentered("Qual deles é a IA infiltrada?",
                     ParaTela((Rectangle){ 0, TITLE_Y, VIRTUAL_W, TITLE_FONT }),
                     (int)(TITLE_FONT * escala), bgColor);

    UpdateDrawFrames();

    if (!g_confirmado) {
        // Botao so funciona depois de escolher alguem
        Rectangle confirmar = {
            (VIRTUAL_W - CONFIRM_W) / 2.0f,
            (float)(VIRTUAL_H - BTN_H - 20),
            (float)CONFIRM_W,
            (float)BTN_H
        };
        if (DrawButton(confirmar, "Confirmar Seleção", g_selecionado >= 0)) {
            g_confirmado = true;
            g_acertou = (g_selecionado == INDICE_DA_IA);
            TraceLog(LOG_INFO, "Veredito: %s", g_acertou ? "correto" : "incorreto");
        }
    } else {
        DrawTextCentered(g_acertou ? "Correto" : "Incorreto",
                         ParaTela((Rectangle){ 0, RESULT_Y, VIRTUAL_W, RESULT_FONT }),
                         (int)(RESULT_FONT * escala), WHITE);

        Rectangle tentar = {
            (float)(VIRTUAL_W - RETRY_W - MARGIN),
            (float)(VIRTUAL_H - BTN_H - MARGIN),
            (float)RETRY_W,
            (float)BTN_H
        };
        if (DrawButton(tentar, "Tentar de novo", true)) g_tentarDeNovo = true;
    }

    SetMouseCursor(g_algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    RayCanvasEnd();
}

void UnloadVerdictScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    RayCanvasClose();
}
