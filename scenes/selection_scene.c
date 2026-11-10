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

static Color textColor = (Color){ 32, 29, 31, 255 };
static Color bgColor = (Color){ 125, 112, 113, 255 };
static Color hoverColor = (Color){ 160, 147, 142, 255 };

static bool g_shouldStartInterrogation = false;
static bool g_shouldAdvanceDay = false;
static bool g_shouldGoToVerdict = false;

static int  g_dia = 1;               // dia mostrado (lido uma vez no Init)
static bool g_podeAvancar = false;   // true se o dia acabou (tokens ou dialogos esgotados)
static bool g_algumHover = false;    // para escolher o cursor no fim do frame
static Texture2D g_mapaTex;          // sprite completo do mapa (265x255)
static bool g_avisoCantina = false;  // mostra aviso ao clicar na cantina

#define MAPA_X 188.0f
#define MAPA_Y  34.0f

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
    g_avisoCantina = false;

    g_dia = GameStateGetDay();
    g_podeAvancar = GameplaySceneDayFinished();

    g_mapaTex = LoadTexture("assets/gameplay_scene/spr_mapa_completo.png");
    SetTextureFilter(g_mapaTex, TEXTURE_FILTER_POINT);

    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

void UpdateDrawSelectionScene(void) {
    RayCanvasBegin();
    g_algumHover = false;

    // Fundo escuro da paleta
    DrawRectangleRec(GetTelaRect(), textColor);

    DrawDayLabel();

    // Desenha a moldura oficial do Mapa (265x255) centralizada na tela
    Rectangle mapaOrig = { 0, 0, (float)g_mapaTex.width, (float)g_mapaTex.height };
    Rectangle mapaDest = ParaTela((Rectangle){ MAPA_X, MAPA_Y, (float)g_mapaTex.width, (float)g_mapaTex.height });
    DrawTexturePro(g_mapaTex, mapaOrig, mapaDest, (Vector2){ 0, 0 }, 0.0f, WHITE);

    float escala = Escala();
    Vector2 mouse = RayCanvasGetMousePosition();
    bool clicou = !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // 1) Barra de titulo dentro do mapa (59, 23, 163, 36)
    Rectangle rTitulo = ParaTela((Rectangle){ MAPA_X + 59, MAPA_Y + 23, 163, 36 });
    int tamTit = (int)(12 * escala);
    const char *txtTit = "MAPA — RASEC";
    DrawText(txtTit,
             (int)(rTitulo.x + (rTitulo.width - MeasureText(txtTit, tamTit)) / 2.0f),
             (int)(rTitulo.y + (rTitulo.height - tamTit) / 2.0f),
             tamTit, (Color){ 189, 181, 168, 255 });

    // 2) Area do Escritorio (30, 89, 103, 88)
    Rectangle rEscrVirt = { MAPA_X + 30, MAPA_Y + 89, 103, 88 };
    Rectangle rEscr = ParaTela(rEscrVirt);
    bool hovEscr = CheckCollisionPointRec(mouse, rEscr);
    if (hovEscr) {
        g_algumHover = true;
        DrawRectangleRec(rEscr, Fade(hoverColor, 0.35f));
        if (clicou) {
            TraceLog(LOG_INFO, "Cenario ESCRITORIO escolhido!");
            g_shouldStartInterrogation = true;
        }
    }
    int tamEscr = (int)(12 * escala);
    const char *lblEscr = "Escritório";
    DrawText(lblEscr,
             (int)(rEscr.x + (rEscr.width - MeasureText(lblEscr, tamEscr)) / 2.0f),
             (int)(rEscr.y + (rEscr.height - tamEscr) / 2.0f - 6 * escala),
             tamEscr, (Color){ 189, 181, 168, 255 });
    int tamSub = (int)(8 * escala);
    const char *subEscr = "[3 Suspeitos]";
    DrawText(subEscr,
             (int)(rEscr.x + (rEscr.width - MeasureText(subEscr, tamSub)) / 2.0f),
             (int)(rEscr.y + (rEscr.height - tamSub) / 2.0f + 8 * escala),
             tamSub, bgColor);

    // 3) Area da Cantina (151, 115, 62, 57)
    Rectangle rCantVirt = { MAPA_X + 151, MAPA_Y + 115, 62, 57 };
    Rectangle rCant = ParaTela(rCantVirt);
    bool hovCant = CheckCollisionPointRec(mouse, rCant);
    if (hovCant) {
        g_algumHover = true;
        DrawRectangleRec(rCant, Fade(bgColor, 0.25f));
        if (clicou) g_avisoCantina = true;
    }
    int tamCant = (int)(9 * escala);
    const char *lblCant = "Cantina";
    DrawText(lblCant,
             (int)(rCant.x + (rCant.width - MeasureText(lblCant, tamCant)) / 2.0f),
             (int)(rCant.y + (rCant.height - tamCant) / 2.0f),
             tamCant, bgColor);

    // 4) Indicador de Andar / Dia à direita (219, 82, 29, 95)
    for (int d = 0; d < 3; d++) {
        Rectangle rSlot = ParaTela((Rectangle){ MAPA_X + 225, MAPA_Y + 90 + d * 27, 17, 20 });
        bool ativo = ((d + 1) == g_dia);
        if (ativo) DrawRectangleRec(rSlot, (Color){ 189, 181, 168, 255 });
        int tamD = (int)(8 * escala);
        const char *txtD = TextFormat("D%d", d + 1);
        DrawText(txtD,
                 (int)(rSlot.x + (rSlot.width - MeasureText(txtD, tamD)) / 2.0f),
                 (int)(rSlot.y + (rSlot.height - tamD) / 2.0f),
                 tamD, ativo ? textColor : bgColor);
    }

    if (g_avisoCantina) {
        const char *msgCant = "A cantina está vazia no momento. Interrogue os funcionários no Escritório.";
        int tamMsg = (int)(10 * escala);
        Rectangle tela = GetTelaRect();
        DrawText(msgCant,
                 (int)(tela.x + (tela.width - MeasureText(msgCant, tamMsg)) / 2.0f),
                 (int)(tela.y + 300 * escala),
                 tamMsg, hoverColor);
    }

    // Pulso suave (0..1) para destacar acoes importantes no Dia 1
    float t = (float)GetTime() * 1.4f;
    t = t - (int)t;
    float tri = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
    float pulso = tri * tri * (3.0f - 2.0f * tri);
    Color corDestaque = (Color){ 189, 181, 168, (unsigned char)(160 + 95 * pulso) };

    if (GameStateIsTutorialActive()) {
        DrawRectangleLinesEx(rEscr, (2.0f + 1.5f * pulso) * Escala(), corDestaque);

        const char *dicaInicio = "Clique em 'Escritório' no mapa para começar a investigação";
        Rectangle tela = GetTelaRect();
        int tamDica = (int)(12 * escala);
        int largDica = MeasureText(dicaInicio, tamDica);
        DrawText(dicaInicio,
                 (int)(tela.x + (tela.width - largDica) / 2.0f),
                 (int)(tela.y + 300 * escala),
                 tamDica, corDestaque);
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

        if (g_dia == 1) {
            Rectangle rDestaque = ParaTela((Rectangle){
                area.x - 3.0f, area.y - 3.0f, area.width + 6.0f, area.height + 6.0f
            });
            DrawRectangleLinesEx(rDestaque, (2.0f + 1.5f * pulso) * Escala(), corDestaque);
        }
    }

    SetMouseCursor(g_algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

    RayCanvasEnd();
}

void UnloadSelectionScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_mapaTex);
    RayCanvasClose();
}
