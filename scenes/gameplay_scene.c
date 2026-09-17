#include "raylib.h"
#include "gameplay_scene.h"
#include "../utils/raycanvas.h"

#define NUM_TOOLS 4

// Texturas das duas versoes da tela
static Texture2D g_telaFechada;   // menu de ferramentas fechado
static Texture2D g_telaAberta;  // menu de ferramentas aberto
static Texture2D g_seta;  // botao da seta (aponta para a direita)
static Texture2D g_menuFerramentas; // barra do menu de ferramentas
static Texture2D g_iconePrancheta;  // icone da prancheta
static Texture2D g_iconeArquivos;   // icone da pasta de arquivos
static Texture2D g_iconeMapa;       // icone do mapa

// Prancheta arrastavel
static Texture2D g_prancheta;                       // sprite da prancheta aberta (299x343)
static bool      g_pranchetaAberta = false;
static bool      g_arrastando      = false;
static Vector2   g_pranchetaPos    = { 170, 8 };    // posicao inicial (centralizada)
static Vector2   g_offsetArraste   = { 0, 0 };      // onde o mouse "pegou" a prancheta

static bool g_menuAberto = false;
static bool g_shouldClose = false;

// Areas clicaveis, em pixels DENTRO da imagem (640x360).
// Elas sao somadas a posicao da imagem na tela (ver ParaTela).
static const Rectangle SETA_FECHADA = { 598, 66, 60, 51 };  // seta "<" (parte fica fora da tela)
static const Rectangle SETA_ABERTA  = { 513, 66, 60, 51 };  // seta ">"
static const Rectangle MENU_AREA    = { 510, 0, 130, 360 }; // menu encostado na direita

static const Rectangle FERRAMENTAS[NUM_TOOLS] = {
    { 571,  10, 60, 72 },  // 0 - prancheta (tamanho do sprite: 60x72)
    { 572, 110, 58, 52 },  // 1 - pasta (tamanho do sprite: 58x52)
    { 564, 184, 74, 82 },  // 2 - maleta
    { 568, 284, 67, 61 },  // 3 - mapa (tamanho do sprite: 67x61)
};

// Funcoes de clique de cada ferramenta (mesmo estilo do menu)
static void OnClickPrancheta(void) {
    g_pranchetaAberta = !g_pranchetaAberta;
    TraceLog(LOG_INFO, "Prancheta %s", g_pranchetaAberta ? "aberta" : "fechada");
}
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

// Desenha um sprite numa area da imagem 640x360.
// Se o mouse estiver em cima, desenha mais escuro (mesmo efeito dos botoes do menu).
static void DesenharComHover(Texture2D tex, Rectangle area) {
    Rectangle destino = ParaTela(area);
    Rectangle origem  = { 0, 0, (float)tex.width, (float)tex.height };

    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), destino);
    Color cor = hover ? (Color){ 180, 180, 180, 255 } : WHITE;

    DrawTexturePro(tex, origem, destino, (Vector2){ 0, 0 }, 0.0f, cor);
}

// Posicao do mouse convertida para pixels da imagem 640x360
static Vector2 MouseNaImagem(void) {
    Rectangle tela = GetTelaRect();
    Vector2 m = RayCanvasGetMousePosition();

    return (Vector2){
        (m.x - tela.x) * g_telaFechada.width  / tela.width,
        (m.y - tela.y) * g_telaFechada.height / tela.height
    };
}

        
bool GameplaySceneShouldClose(void) {
    return g_shouldClose;
}

void InitGameplayScene(void) {
    g_shouldClose = false;
    g_menuAberto = false;
    g_pranchetaAberta = false;
    g_arrastando = false;
    g_pranchetaPos = (Vector2){ 170, 8 };

    g_telaFechada = LoadTexture("assets/gameplay_scene/spr_tela_fechada.png");
    SetTextureFilter(g_telaFechada, TEXTURE_FILTER_POINT);

    g_telaAberta = LoadTexture("assets/gameplay_scene/spr_tela_aberta.png");
    SetTextureFilter(g_telaAberta, TEXTURE_FILTER_POINT);

    g_seta = LoadTexture("assets/gameplay_scene/spr_botao_seta.png");
       SetTextureFilter(g_seta, TEXTURE_FILTER_POINT);

    g_menuFerramentas = LoadTexture("assets/gameplay_scene/spr_menu_ferramentas.png");
    SetTextureFilter(g_menuFerramentas, TEXTURE_FILTER_POINT);

    g_iconePrancheta = LoadTexture("assets/gameplay_scene/spr_icone_prancheta.png");
    SetTextureFilter(g_iconePrancheta, TEXTURE_FILTER_POINT);

    g_iconeArquivos = LoadTexture("assets/gameplay_scene/spr_icone_arquivos.png");
    SetTextureFilter(g_iconeArquivos, TEXTURE_FILTER_POINT);

    g_iconeMapa = LoadTexture("assets/gameplay_scene/spr_icone_mapa.png");
    SetTextureFilter(g_iconeMapa, TEXTURE_FILTER_POINT);

    g_prancheta = LoadTexture("assets/gameplay_scene/spr_prancheta.png");
    SetTextureFilter(g_prancheta, TEXTURE_FILTER_POINT);

    RayCanvasInit(g_telaFechada.width, g_telaFechada.height);
}

// Cuida do arraste da prancheta.
// Devolve true se a prancheta "pegou" o mouse neste frame
// (assim o clique nao atravessa para o que esta embaixo dela).
static bool UpdatePrancheta(void) {
    if (!g_pranchetaAberta) return false;

    Vector2 mouse = MouseNaImagem();
    Rectangle area = { g_pranchetaPos.x, g_pranchetaPos.y,
                       (float)g_prancheta.width, (float)g_prancheta.height };

    // 1) Comecou a arrastar: clicou em cima da prancheta
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, area)) {
        g_arrastando = true;
        g_offsetArraste = (Vector2){ mouse.x - g_pranchetaPos.x, mouse.y - g_pranchetaPos.y };
    }

    if (!g_arrastando) return false;

    // 2) Soltou o botao: para de arrastar
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        g_arrastando = false;
        return false;
    }

    // 3) Arrastando: a prancheta segue o mouse
    g_pranchetaPos.x = mouse.x - g_offsetArraste.x;
    g_pranchetaPos.y = mouse.y - g_offsetArraste.y;

    // Nao deixa sair da tela
    float maxX = (float)(g_telaFechada.width  - g_prancheta.width);
    float maxY = (float)(g_telaFechada.height - g_prancheta.height);
    if (g_pranchetaPos.x < 0)    g_pranchetaPos.x = 0;
    if (g_pranchetaPos.y < 0)    g_pranchetaPos.y = 0;
    if (g_pranchetaPos.x > maxX) g_pranchetaPos.x = maxX;
    if (g_pranchetaPos.y > maxY) g_pranchetaPos.y = maxY;

    SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    return true;
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

           // A prancheta fica por cima de tudo, entao ela tem prioridade no clique
    if (!UpdatePrancheta()) {
        UpdateGameplay();
    }

           RayCanvasDrawTexture(g_telaFechada, GetTelaRect(), WHITE);

    // 2) Menu de ferramentas por cima do fundo (so quando aberto)
    if (g_menuAberto) {
        Rectangle menuOrigem = { 0, 0, (float)g_menuFerramentas.width, (float)g_menuFerramentas.height };
        DrawTexturePro(g_menuFerramentas, menuOrigem, ParaTela(MENU_AREA), (Vector2){ 0, 0 }, 0.0f, WHITE);
        DesenharComHover(g_iconePrancheta, FERRAMENTAS[0]);
        DesenharComHover(g_iconeArquivos, FERRAMENTAS[1]);
        DesenharComHover(g_iconeMapa, FERRAMENTAS[3]);
    }

    

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

        // 4) Prancheta por cima de tudo
    if (g_pranchetaAberta) {
        Rectangle area   = { g_pranchetaPos.x, g_pranchetaPos.y,
                             (float)g_prancheta.width, (float)g_prancheta.height };
        Rectangle origem = { 0, 0, (float)g_prancheta.width, (float)g_prancheta.height };
        DrawTexturePro(g_prancheta, origem, ParaTela(area), (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
       RayCanvasEnd();
   }

void UnloadGameplayScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_telaFechada);
    UnloadTexture(g_telaAberta);
    UnloadTexture(g_seta);
    UnloadTexture(g_menuFerramentas);
    UnloadTexture(g_iconePrancheta);
    UnloadTexture(g_iconeArquivos);
    UnloadTexture(g_iconeMapa);
    UnloadTexture(g_prancheta);
    RayCanvasClose();
}