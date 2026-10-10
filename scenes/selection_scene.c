#include "raylib.h"
#include "selection_scene.h"
#include "gameplay_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/game_state.h"

// Tamanho "virtual" da tela (o mesmo das outras cenas: 640x360).
// Todas as medidas abaixo sao em pixels desse espaco e depois
// convertidas pela escala atual da janela (ver ParaTela).
#define VIRTUAL_W 640
#define VIRTUAL_H 360

#define TITLE_Y        30
#define TITLE_FONT     28
#define BUTTONS_TOP    120
#define BTN_W          160
#define BTN_H          36
#define BTN_SPACING    14
#define BTN_FONT       20

// Botao "proximo dia" / "veredito" (canto inferior direito)
#define NEXT_W         220
#define NEXT_H         28
#define NEXT_FONT      12
#define NEXT_MARGIN    12

static Color textColor = (Color){ 30, 32, 34, 255 };
static Color bgColor = (Color){ 120, 130, 110, 255 };
static Color hoverColor = (Color){ 150, 160, 130, 255 };

static bool g_shouldStartInterrogation = false;
static bool g_shouldAdvanceDay = false;
static bool g_shouldGoToVerdict = false;

static int  g_dia = 1;               // dia mostrado (lido uma vez no Init)
static bool g_podeAvancar = false;   // true se o dia acabou (tokens ou dialogos esgotados)
static bool g_algumHover = false;    // para escolher o cursor no fim do frame

// Retangulo onde a "tela" 640x360 e desenhada (centralizada no canvas).
// Mesmo padrao usado na gameplay_scene.
static Rectangle GetTelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)VIRTUAL_W, (float)VIRTUAL_H }
    );
}

// Converte um retangulo em pixels 640x360 para a posicao real no canvas
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

// Escala atual (1.0 = tamanho original)
static float Escala(void) {
    return GetTelaRect().width / (float)VIRTUAL_W;
}

// Desenha o titulo centralizado no topo
static void DrawTitle(const char *texto) {
    Rectangle tela = GetTelaRect();
    float escala = Escala();
    int tamanho = (int)(TITLE_FONT * escala);
    int largura = MeasureText(texto, tamanho);

    DrawText(texto,
             (int)(tela.x + (tela.width - largura) / 2.0f),
             (int)(tela.y + TITLE_Y * escala),
             tamanho, bgColor);
}

// Desenha "Dia X" no canto superior esquerdo
static void DrawDayLabel(void) {
    Rectangle tela = GetTelaRect();
    float escala = Escala();
    DrawText(TextFormat("Dia %d", g_dia),
             (int)(tela.x + 12 * escala),
             (int)(tela.y + 10 * escala),
             (int)(16 * escala), bgColor);
}

// Desenha UM botao (retangulo no espaco virtual 640x360) e devolve true
// se ele foi clicado neste frame.
static bool UpdateDrawButton(Rectangle areaVirtual, const char *label, int fonte) {
    Rectangle area = ParaTela(areaVirtual);
    float escala = Escala();

    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), area);
    if (hover) g_algumHover = true;

    DrawRectangleRec(area, hover ? hoverColor : bgColor);
    DrawRectangleLinesEx(area, 2.0f * escala, textColor);

    int tamanho = (int)(fonte * escala);
    int largura = MeasureText(label, tamanho);
    DrawText(label,
             (int)(area.x + (area.width - largura) / 2.0f),
             (int)(area.y + (area.height - tamanho) / 2.0f),
             tamanho, textColor);

    // So "Pressed": evita que o release do clique do menu
    // dispare um botao desta cena logo ao entrar nela.
    // Durante o fade (transicao) os cliques sao ignorados.
    return hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Desenha uma coluna de botoes centralizados e devolve o indice do botao
// clicado neste frame (ou -1 se nenhum foi clicado).
static int UpdateDrawButtons(const char *labels[], int count) {
    int clicado = -1;

    for (int i = 0; i < count; i++) {
        Rectangle area = {
            (VIRTUAL_W - BTN_W) / 2.0f,
            (float)(BUTTONS_TOP + i * (BTN_H + BTN_SPACING)),
            (float)BTN_W,
            (float)BTN_H
        };
        if (UpdateDrawButton(area, labels[i], BTN_FONT)) clicado = i;
    }
    return clicado;
}

bool SelectionSceneShouldStartInterrogation(void) {
    return g_shouldStartInterrogation;
}

bool SelectionSceneShouldAdvanceDay(void) {
    return g_shouldAdvanceDay;
}

bool SelectionSceneShouldGoToVerdict(void) {
    return g_shouldGoToVerdict;
}

Employee SelectionSceneGetEmployee(void) {
    return EMPLOYEE_ALPHA; // Dummy, unused now
}

void InitSelectionScene(void) {
    g_shouldStartInterrogation = false;
    g_shouldAdvanceDay = false;
    g_shouldGoToVerdict = false;

    g_dia = GameStateGetDay();
    g_podeAvancar = GameplaySceneDayFinished();

    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

void UpdateDrawSelectionScene(void) {
    RayCanvasBegin();
    g_algumHover = false;

    // Fundo placeholder (cor lisa)
    DrawRectangleRec(GetTelaRect(), textColor);

    DrawTitle("Escolher cenário");
    DrawDayLabel();

    const char *labels[] = { "Escritório" };
    int clicado = UpdateDrawButtons(labels, 1);

    if (clicado == 0) {
        TraceLog(LOG_INFO, "Cenario ESCRITORIO escolhido!");
        g_shouldStartInterrogation = true;
    }

    // Aparece quando os tokens acabam ou os dialogos do dia se esgotam
    if (g_podeAvancar) {
        bool ultimoDia = (g_dia >= GAME_NUM_DAYS);
        Rectangle area = {
            VIRTUAL_W - NEXT_W - NEXT_MARGIN,
            VIRTUAL_H - NEXT_H - NEXT_MARGIN,
            (float)NEXT_W,
            (float)NEXT_H
        };
        const char *label = ultimoDia ? "Seguir para o Veredito" : "Seguir para o próximo dia";

        if (UpdateDrawButton(area, label, NEXT_FONT)) {
            if (ultimoDia) g_shouldGoToVerdict = true;
            else           g_shouldAdvanceDay = true;
        }
    }

    SetMouseCursor(g_algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    RayCanvasEnd();
}

void UnloadSelectionScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    RayCanvasClose();
}
