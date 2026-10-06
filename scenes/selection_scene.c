#include "raylib.h"
#include "selection_scene.h"
#include "../utils/raycanvas.h"

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

// As duas telas desta cena
typedef enum {
    STEP_SCENARIO,   // "Escolher cenario" -> botao "Escritorio"
    STEP_EMPLOYEES   // "Funcionarios"     -> botoes Alpha / Beta / Gamma
} SelectionStep;

static SelectionStep g_step = STEP_SCENARIO;
static bool          g_shouldStartInterrogation = false;
static Employee      g_selectedEmployee = EMPLOYEE_ALPHA;

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
             tamanho, WHITE);
}

// Desenha uma coluna de botoes centralizados e devolve o indice do botao
// clicado neste frame (ou -1 se nenhum foi clicado).
static int UpdateDrawButtons(const char *labels[], int count) {
    Vector2 mouse = RayCanvasGetMousePosition();
    float escala = Escala();
    int clicado = -1;
    bool algumHover = false;

    for (int i = 0; i < count; i++) {
        Rectangle area = ParaTela((Rectangle){
            (VIRTUAL_W - BTN_W) / 2.0f,
            (float)(BUTTONS_TOP + i * (BTN_H + BTN_SPACING)),
            (float)BTN_W,
            (float)BTN_H
        });

        bool hover = CheckCollisionPointRec(mouse, area);
        if (hover) {
            algumHover = true;
            // So "Pressed": evita que o release do clique do menu
            // dispare um botao desta cena logo ao entrar nela.
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) clicado = i;
        }

        // Mesmo efeito de hover dos botoes do menu (escurece)
        Color fundo = hover ? (Color){ 180, 180, 180, 255 } : WHITE;
        DrawRectangleRec(area, fundo);
        DrawRectangleLinesEx(area, 2.0f * escala, BLACK);

        int tamanho = (int)(BTN_FONT * escala);
        int largura = MeasureText(labels[i], tamanho);
        DrawText(labels[i],
                 (int)(area.x + (area.width - largura) / 2.0f),
                 (int)(area.y + (area.height - tamanho) / 2.0f),
                 tamanho, BLACK);
    }

    SetMouseCursor(algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    return clicado;
}

bool SelectionSceneShouldStartInterrogation(void) {
    return g_shouldStartInterrogation;
}

Employee SelectionSceneGetEmployee(void) {
    return g_selectedEmployee;
}

void InitSelectionScene(void) {
    g_step = STEP_SCENARIO;
    g_shouldStartInterrogation = false;
    g_selectedEmployee = EMPLOYEE_ALPHA;
    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

void UpdateDrawSelectionScene(void) {
    RayCanvasBegin();

    // Fundo placeholder (cor lisa)
    DrawRectangleRec(GetTelaRect(), (Color){ 30, 34, 40, 255 });

    if (g_step == STEP_SCENARIO) {
        DrawTitle("Escolher cenário");

        const char *labels[] = { "Escritório" };
        int clicado = UpdateDrawButtons(labels, 1);

        if (clicado == 0) {
            TraceLog(LOG_INFO, "Cenario ESCRITORIO escolhido!");
            g_step = STEP_EMPLOYEES;
        }
    } else {
        DrawTitle("Funcionários");

        const char *labels[] = { "Alpha", "Beta", "Gamma" };
        int clicado = UpdateDrawButtons(labels, 3);

        if (clicado >= 0) {
            g_selectedEmployee = (Employee)clicado;
            TraceLog(LOG_INFO, "Funcionario escolhido: %s", labels[clicado]);
            g_shouldStartInterrogation = true;
        }
    }

    RayCanvasEnd();
}

void UnloadSelectionScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    RayCanvasClose();
}