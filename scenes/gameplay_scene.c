#include "raylib.h"
#include "gameplay_scene.h"
#include "../utils/raycanvas.h"

#define NUM_TOOLS 4

// Texturas das duas versoes da tela
static Texture2D g_telaFechada;   // menu de ferramentas fechado
static Texture2D g_telaAberta;  // menu de ferramentas aberto
static Texture2D g_seta;  // botao da seta (aponta para a direita)

static bool g_menuAberto = false;
static bool g_shouldClose = false;

// Areas clicaveis, em pixels DENTRO da imagem (640x360).
// Elas sao somadas a posicao da imagem na tela (ver ParaTela).
static const Rectangle SETA_FECHADA = { 598, 66, 60, 51 };  // seta "<" (parte fica fora da tela)
static const Rectangle SETA_ABERTA  = { 513, 66, 60, 51 };  // seta ">"

static const Rectangle FERRAMENTAS[NUM_TOOLS] = {
    { 564,   2, 74, 88 },  // 0 - prancheta
    { 564,  92, 74, 88 },  // 1 - pasta
    { 564, 184, 74, 82 },  // 2 - maleta
    { 564, 270, 74, 88 },  // 3 - mapa
};

// Funcoes de clique de cada ferramenta (mesmo estilo do menu)
static void OnClickPrancheta(void) { TraceLog(LOG_INFO, "Prancheta clicada!"); }
static void OnClickPasta(void)     { TraceLog(LOG_INFO, "Pasta clicada!"); }
static void OnClickMaleta(void)    { TraceLog(LOG_INFO, "Maleta clicada!"); }
static void OnClickMapa(void)      { TraceLog(LOG_INFO, "Mapa clicado!"); }

// Retangulo onde a imagem inteira e desenhada (centralizada no canvas)
static Rectangle GetTelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)g_telaFechada.width, (float)g_telaFechada.height }
    );
}

// Converte um retangulo "da imagem" para a posicao real no canvas
static Rectangle ParaTela(Rectangle r) {
    Rectangle tela = GetTelaRect();

    // Quanto a imagem foi ampliada (1 = tamanho original)
    float escalaX = tela.width  / (float)g_telaFechada.width;
    float escalaY = tela.height / (float)g_telaFechada.height;

    return (Rectangle){
        tela.x + r.x * escalaX,
        tela.y + r.y * escalaY,
        r.width  * escalaX,
        r.height * escalaY
    };
}

bool GameplaySceneShouldClose(void) {
    return g_shouldClose;
}

void InitGameplayScene(void) {
    g_shouldClose = false;
    g_menuAberto = false;

    g_telaFechada = LoadTexture("assets/gameplay_scene/spr_tela_fechada.png");
    SetTextureFilter(g_telaFechada, TEXTURE_FILTER_POINT);

    g_telaAberta = LoadTexture("assets/gameplay_scene/spr_tela_aberta.png");
    SetTextureFilter(g_telaAberta, TEXTURE_FILTER_POINT);

    g_seta = LoadTexture("assets/gameplay_scene/spr_botao_seta.png");
       SetTextureFilter(g_seta, TEXTURE_FILTER_POINT);

    RayCanvasInit(g_telaFechada.width, g_telaFechada.height);
}

static void UpdateGameplay(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    Rectangle seta = ParaTela(g_menuAberto ? SETA_ABERTA : SETA_FECHADA);
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Ajuda para calibrar: mostra no console onde voce clicou (coordenadas da imagem)
    if (clicou) {
        Rectangle tela = GetTelaRect();
        TraceLog(LOG_INFO, "Clique na imagem: %d, %d",
                 (int)(mouse.x - tela.x), (int)(mouse.y - tela.y));
    }

    // Cursor de maozinha sobre itens clicaveis
    bool hover = CheckCollisionPointRec(mouse, seta);

    // 1) Seta: abre/fecha o menu
    if (clicou && CheckCollisionPointRec(mouse, seta)) {
        g_menuAberto = !g_menuAberto;
        return;
    }

    // 2) Ferramentas (so funcionam com o menu aberto)
    if (g_menuAberto) {
        for (int i = 0; i < NUM_TOOLS; i++) {
            if (!CheckCollisionPointRec(mouse, ParaTela(FERRAMENTAS[i]))) continue;
            hover = true;
            if (clicou) {
                if (i == 0) OnClickPrancheta();
                else if (i == 1) OnClickPasta();
                else if (i == 2) OnClickMaleta();
                else if (i == 3) OnClickMapa();
            }
        }
    }

    SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

   void UpdateDrawGameplayScene(void) {
       RayCanvasBegin();

       UpdateGameplay();

       // 1) Fundo
       RayCanvasDrawTexture(g_menuAberto ? g_telaAberta : g_telaFechada, GetTelaRect(), WHITE);

       // 2) Seta por cima do fundo
       Rectangle setaDestino = ParaTela(g_menuAberto ? SETA_ABERTA : SETA_FECHADA);
       Rectangle setaOrigem  = { 0, 0, (float)g_seta.width, (float)g_seta.height };

       // O sprite aponta para a direita (">").
       // Com o menu fechado, espelhamos para apontar para a esquerda ("<").
       if (!g_menuAberto) {
           setaOrigem.width = -setaOrigem.width;
       }

           // Destaque quando o mouse esta em cima (mesmo efeito dos botoes do menu)
    bool setaHover = CheckCollisionPointRec(RayCanvasGetMousePosition(), setaDestino);
    Color corSeta = setaHover ? (Color){ 180, 180, 180, 255 } : WHITE;

    DrawTexturePro(g_seta, setaOrigem, setaDestino, (Vector2){ 0, 0 }, 0.0f, corSeta);
       RayCanvasEnd();
   }

void UnloadGameplayScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_telaFechada);
    UnloadTexture(g_telaAberta);
    UnloadTexture(g_seta);
    RayCanvasClose();
}