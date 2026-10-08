#include "raylib.h"
#include "gameplay_scene.h"
#include "selection_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define NUM_TOOLS 4

static DialogTree g_trees[3];
static bool g_treesLoaded = false;
static DialogTree *g_tree = NULL;
static bool g_dialogLoaded = false;


// Texturas das duas versoes da tela
static Texture2D g_telaFechada;     // menu de ferramentas fechado
static Texture2D g_telaAberta;      // menu de ferramentas aberto
static Texture2D g_background;  // foto do escritorio atras da tela
static Texture2D g_seta;            // botao da seta (aponta para a direita)
static Texture2D g_menuFerramentas; // barra do menu de ferramentas
static Texture2D g_iconePrancheta;  // icone da prancheta
static Texture2D g_iconeArquivos;   // icone da pasta de arquivos
static Texture2D g_iconeMaleta;     // icone da maleta
static Texture2D g_iconeMapa;       // icone do mapa

// Prancheta arrastavel
static Texture2D g_prancheta;                       // sprite da prancheta aberta (299x343)
static bool      g_pranchetaAberta = false;
static bool      g_arrastando      = false;
static Vector2   g_pranchetaPos    = { 170, 8 };    // posicao inicial (centralizada)
static Vector2   g_offsetArraste   = { 0, 0 };      // onde o mouse "pegou" a prancheta

// Quadradinhos da prancheta
#define NUM_CAIXAS 5
static Texture2D g_caixaVazia;               // quadradinho sem X
static Texture2D g_caixaComX;                // quadradinho com X
static bool      g_marcado[NUM_CAIXAS];      // true = quadradinho com X

// Posicao de cada quadradinho DENTRO da prancheta (sprite 34x33)
static const Vector2 CAIXAS[NUM_CAIXAS] = {
    { 59,  60 },
    { 59, 110 },
    { 59, 160 },
    { 59, 210 },
    { 59, 260 },
};

static bool g_menuAberto = false;
static bool g_shouldClose = false;
static bool g_irParaSelecao = false;  // true quando clica no mapa

// ---------------------------------------------------------------
// TEXTOS DO INTERROGATORIO
// Troque aqui o que aparece na tela.
// ---------------------------------------------------------------
static const char *g_nomeAtual  = "Nome da Pessoa";
static const char *g_fraseAtual = "Qualquer frase aqui";

// Para calibrar a posicao, mude para true: aparecem retangulos
// de teste (vermelho = nome, azul = frase) com texto branco.
#define MOSTRAR_AREAS_DEBUG false

// Areas clicaveis, em pixels DENTRO da imagem (640x360).
// Elas sao somadas a posicao da imagem na tela (ver ParaTela).
static const Rectangle SETA_FECHADA = { 598, 66, 60, 51 };  // seta "<" (parte fica fora da tela)
static const Rectangle SETA_ABERTA  = { 513, 66, 60, 51 };  // seta ">"
static const Rectangle MENU_AREA    = { 510, 0, 130, 360 }; // menu encostado na direita
static const Rectangle BG_AREA      = { 0, 0, 270, 360 };   // quadrado da esquerda (sprite 270x360)

// Areas dos textos (tambem em pixels da imagem 640x360)
static const Rectangle NOME_AREA  = { 2, 300, 96, 18 };     // quadrado vermelho
static const Rectangle FRASE_AREA = { 120, 318, 126, 26 };  // circulo vermelho

static const Rectangle FERRAMENTAS[NUM_TOOLS] = {
    { 571,  10, 60, 72 },  // 0 - prancheta (tamanho do sprite: 60x72)
    { 572, 110, 58, 52 },  // 1 - pasta (tamanho do sprite: 58x52)
    { 573, 200, 57, 57 },  // 2 - maleta (tamanho do sprite: 57x57)
    { 568, 284, 67, 61 },  // 3 - mapa (tamanho do sprite: 67x61)
};

// Funcoes de clique de cada ferramenta (mesmo estilo do menu)
static void OnClickPrancheta(void) {
    g_pranchetaAberta = !g_pranchetaAberta;
    TraceLog(LOG_INFO, "Prancheta %s", g_pranchetaAberta ? "aberta" : "fechada");
}
static void OnClickPasta(void)     { TraceLog(LOG_INFO, "Pasta clicada!"); }
static void OnClickMaleta(void)    { TraceLog(LOG_INFO, "Maleta clicada!"); }
static void OnClickMapa(void) {
    TraceLog(LOG_INFO, "Mapa clicado! Indo para a selecao de cenario");
    g_irParaSelecao = true;
}

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
static void DrawWithHover(Texture2D tex, Rectangle area) {
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

// Desenha um texto centralizado dentro de uma area da imagem 640x360.
// O tamanho da fonte acompanha o zoom da tela.
static void DrawTextInArea(const char *texto, Rectangle area, int tamanhoFonte, Color cor) {
    Rectangle destino = ParaTela(area);
    float escala = destino.height / area.height;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;

    int largura = MeasureText(texto, tam);
    float x = destino.x + (destino.width  - largura) / 2;
    float y = destino.y + (destino.height - tam) / 2;

    DrawText(texto, (int)x, (int)y, tam, cor);
}

static void DrawTextLeftAligned(const char *texto, Vector2 pos, int tamanhoFonte, Color cor) {
    Rectangle fakeArea = {pos.x, pos.y, 10, 10};
    Rectangle destino = ParaTela(fakeArea);
    float escala = destino.height / 10.0f;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;
    DrawText(texto, (int)destino.x, (int)destino.y, tam, cor);
}

static void DrawTextWrappedLeftAligned(const char *texto, Vector2 pos, float maxLargura, int tamanhoFonte, Color cor) {
    Rectangle fakeArea = {0, 0, maxLargura, 10};
    float maxScaledW = ParaTela(fakeArea).width;
    float escala = ParaTela(fakeArea).height / 10.0f;
    int tam = (int)(tamanhoFonte * escala);
    
    char buffer[512];
    strncpy(buffer, texto, 511);
    buffer[511] = '\0';
    
    char *word = strtok(buffer, " ");
    char line[512] = "";
    int yOffset = 0;
    
    while (word != NULL) {
        char testLine[512];
        strcpy(testLine, line);
        if (strlen(testLine) > 0) strcat(testLine, " ");
        strcat(testLine, word);
        
        if (MeasureText(testLine, tam) > maxScaledW && strlen(line) > 0) {
            DrawTextLeftAligned(line, (Vector2){ pos.x, pos.y + yOffset }, tamanhoFonte, cor);
            strcpy(line, word);
            yOffset += (tamanhoFonte + 4);
        } else {
            strcpy(line, testLine);
        }
        word = strtok(NULL, " ");
    }
    if (strlen(line) > 0) {
        DrawTextLeftAligned(line, (Vector2){ pos.x, pos.y + yOffset }, tamanhoFonte, cor);
    }
}

static void DrawInterrogationTexts(void) {
    Color textColor = (Color){ 30, 32, 34, 255 };
    Color bgColor = (Color){ 120, 130, 110, 255 }; 
    Color hoverColor = (Color){ 150, 160, 130, 255 };

    char bufTokens[32];
    sprintf(bufTokens, "Tokens: %d", global_tokens);
    DrawTextLeftAligned(bufTokens, (Vector2){ 280, 10 }, 10, textColor);

    char ficha[256];
    sprintf(ficha, "LOCAL:\nSala de Interrogatorio");
    DrawTextWrappedLeftAligned(ficha, (Vector2){ 10, 322 }, 250.0f, 10, textColor);

    if (!g_dialogLoaded) {
        const char *labels[] = { "Ir até Alpha", "Ir até Beta", "Ir até Gamma" };
        int startY = 80; // (360 - 136) / 2
        for (int i = 0; i < 3; i++) {
            Rectangle choiceArea = { 55, startY + (i * 50), 160, 36 }; // 50 = 36 height + 14 spacing
            Rectangle rTela = ParaTela(choiceArea);
            bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rTela);
            Color boxColor = hover ? hoverColor : bgColor;
            DrawRectangleRec(rTela, boxColor);
            DrawRectangleLinesEx(rTela, 2, textColor);
            DrawTextInArea(labels[i], choiceArea, 16, textColor);
        }
        return;
    }

    DialogNode *node = GetCurrentNode(g_tree);
    if (!node) return;

    DrawTextLeftAligned(node->speaker, (Vector2){ 35, 303 }, 12, textColor);

    DialogChoice active_choices[MAX_CHOICES];
    int active_count = 0;

    if (global_tokens > 0) {
        if (node->choice_count > 0) {
            for (int i = 0; i < node->choice_count; i++) {
                const char *next_id = node->choices[i].next_node;
                bool visited = false;
                for (int j = 0; j < g_tree->node_count; j++) {
                    if (strcmp(g_tree->nodes[j].id, next_id) == 0 && g_tree->nodes[j].visited) {
                        visited = true;
                        break;
                    }
                }
                if (!visited && active_count < MAX_CHOICES) {
                    active_choices[active_count++] = node->choices[i];
                }
            }
        } else {
            DialogNode *startNode = NULL;
            for (int i = 0; i < g_tree->node_count; i++) {
                if (strcmp(g_tree->nodes[i].id, "start") == 0) {
                    startNode = &g_tree->nodes[i];
                    break;
                }
            }
            if (startNode) {
                for (int i = 0; i < startNode->choice_count; i++) {
                    const char *next_id = startNode->choices[i].next_node;
                    bool visited = false;
                    for (int j = 0; j < g_tree->node_count; j++) {
                        if (strcmp(g_tree->nodes[j].id, next_id) == 0 && g_tree->nodes[j].visited) {
                            visited = true;
                            break;
                        }
                    }
                    if (!visited && active_count < MAX_CHOICES) {
                        sprintf(active_choices[active_count].text, "[Voltar] %s", startNode->choices[i].text);
                        strcpy(active_choices[active_count].next_node, startNode->choices[i].next_node);
                        active_count++;
                    }
                }
            }
        }
    }

    int total_buttons = active_count + 1;
    int startY = 355 - (total_buttons * 20);
    int exitY = startY;

    int textHeight = 90;
    int boxY = startY - textHeight - 10;

    Rectangle nameBox = { 280, boxY - 14, 70, 14 };
    Rectangle rNameBox = ParaTela(nameBox);
    DrawRectangleRec(rNameBox, bgColor);
    DrawRectangleLinesEx(rNameBox, 2, textColor);
    DrawTextInArea(node->speaker, nameBox, 10, textColor);

    Rectangle textBox = { 280, boxY, 200, textHeight };
    Rectangle rTextBox = ParaTela(textBox);
    DrawRectangleRec(rTextBox, bgColor);
    DrawRectangleLinesEx(rTextBox, 2, textColor);
    DrawTextWrappedLeftAligned(node->text, (Vector2){ 285, boxY + 5 }, 190.0f, 10, textColor);

    if (global_tokens > 0) {
        for (int i = 0; i < active_count; i++) {
            Rectangle choiceArea = { 280, startY + (i * 20), 200, 18 };
            Rectangle rTela = ParaTela(choiceArea);
            
            bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rTela);
            Color boxColor = hover ? hoverColor : bgColor;
            
            DrawRectangleRec(rTela, boxColor);
            DrawRectangleLinesEx(rTela, 2, textColor);
            DrawTextInArea(active_choices[i].text, choiceArea, 10, textColor);
        }
        exitY = startY + (active_count * 20);
    }

    Rectangle exitArea = { 280, exitY, 200, 18 };
    Rectangle rTela = ParaTela(exitArea);
    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rTela);
    Color boxColor = hover ? hoverColor : bgColor;
    DrawRectangleRec(rTela, boxColor);
    DrawRectangleLinesEx(rTela, 2, textColor);
    if (global_tokens > 0) {
        DrawTextInArea("Pode voltar ao trabalho.", exitArea, 10, textColor);
    } else {
        DrawTextInArea("To cansado, depois falo contigo.", exitArea, 10, textColor);
    }
}

bool GameplaySceneShouldClose(void) {
    return g_shouldClose;
}

bool GameplaySceneGoToSelection(void) {
    return g_irParaSelecao;
}

void InitGameplayScene(void) {
    g_shouldClose = false;
    g_irParaSelecao = false;
    g_menuAberto = false;
    g_pranchetaAberta = false;
    g_arrastando = false;
    g_pranchetaPos = (Vector2){ 170, 8 };

    InitDialogSystem();
    g_dialogLoaded = false;
    g_tree = NULL;
    
    if (!g_treesLoaded) {
        LoadDialogTree("assets/dialogues/npc_alpha.txt", &g_trees[0]);
        LoadDialogTree("assets/dialogues/npc_beta.txt", &g_trees[1]);
        LoadDialogTree("assets/dialogues/npc_gamma.txt", &g_trees[2]);
        g_treesLoaded = true;
    }

    g_telaFechada = LoadTexture("assets/gameplay_scene/spr_tela_fechada.png");
    SetTextureFilter(g_telaFechada, TEXTURE_FILTER_POINT);

    g_telaAberta = LoadTexture("assets/gameplay_scene/spr_tela_aberta.png");
    SetTextureFilter(g_telaAberta, TEXTURE_FILTER_POINT);

    g_background = LoadTexture("assets/gameplay_scene/spr_background_teste.png");
    SetTextureFilter(g_background, TEXTURE_FILTER_POINT);

    g_seta = LoadTexture("assets/gameplay_scene/spr_botao_seta.png");
    SetTextureFilter(g_seta, TEXTURE_FILTER_POINT);

    g_menuFerramentas = LoadTexture("assets/gameplay_scene/spr_menu_ferramentas.png");
    SetTextureFilter(g_menuFerramentas, TEXTURE_FILTER_POINT);

    g_iconePrancheta = LoadTexture("assets/gameplay_scene/spr_icone_prancheta.png");
    SetTextureFilter(g_iconePrancheta, TEXTURE_FILTER_POINT);

    g_iconeArquivos = LoadTexture("assets/gameplay_scene/spr_icone_arquivos.png");
    SetTextureFilter(g_iconeArquivos, TEXTURE_FILTER_POINT);

    g_iconeMaleta = LoadTexture("assets/gameplay_scene/spr_icone_ferramentas.png");
    SetTextureFilter(g_iconeMaleta, TEXTURE_FILTER_POINT);

    g_iconeMapa = LoadTexture("assets/gameplay_scene/spr_icone_mapa.png");
    SetTextureFilter(g_iconeMapa, TEXTURE_FILTER_POINT);

    g_prancheta = LoadTexture("assets/gameplay_scene/spr_prancheta.png");
    SetTextureFilter(g_prancheta, TEXTURE_FILTER_POINT);

    g_caixaVazia = LoadTexture("assets/gameplay_scene/spr_icone_quadrado_vazio.png");
    SetTextureFilter(g_caixaVazia, TEXTURE_FILTER_POINT);

    g_caixaComX = LoadTexture("assets/gameplay_scene/spr_icone_quadrado_prancheta.png");
    SetTextureFilter(g_caixaComX, TEXTURE_FILTER_POINT);

    // Comeca com todos os quadradinhos vazios
    for (int i = 0; i < NUM_CAIXAS; i++) g_marcado[i] = false;

    RayCanvasInit(g_telaFechada.width, g_telaFechada.height);
}
// Retangulo do quadradinho i, em pixels da imagem 640x360
static Rectangle CaixaNaImagem(int i) {
    return (Rectangle){
        g_pranchetaPos.x + CAIXAS[i].x,
        g_pranchetaPos.y + CAIXAS[i].y,
        (float)g_caixaVazia.width,
        (float)g_caixaVazia.height
    };
}
// Cuida do arraste da prancheta.
// Devolve true se a prancheta "pegou" o mouse neste frame
// (assim o clique nao atravessa para o que esta embaixo dela).
static bool UpdatePrancheta(void) {
    if (!g_pranchetaAberta) return false;

    Vector2 mouse = MouseNaImagem();
    Rectangle area = { g_pranchetaPos.x, g_pranchetaPos.y,
                       (float)g_prancheta.width, (float)g_prancheta.height };

// 0) Clicou num quadradinho: marca/desmarca o X (e nao arrasta)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < NUM_CAIXAS; i++) {
            if (CheckCollisionPointRec(mouse, CaixaNaImagem(i))) {
                g_marcado[i] = !g_marcado[i];
                return true;
            }
        }
    }
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

    if (g_dialogLoaded) {
        DialogNode *node = GetCurrentNode(g_tree);
        if (node) {
            DialogChoice active_choices[MAX_CHOICES];
            int active_count = 0;

            if (global_tokens > 0) {
        if (node->choice_count > 0) {
            for (int i = 0; i < node->choice_count; i++) {
                const char *next_id = node->choices[i].next_node;
                bool visited = false;
                for (int j = 0; j < g_tree->node_count; j++) {
                    if (strcmp(g_tree->nodes[j].id, next_id) == 0 && g_tree->nodes[j].visited) {
                        visited = true;
                        break;
                    }
                }
                if (!visited && active_count < MAX_CHOICES) {
                    active_choices[active_count++] = node->choices[i];
                }
            }
        } else {
                    DialogNode *startNode = NULL;
                    for (int i = 0; i < g_tree->node_count; i++) {
                        if (strcmp(g_tree->nodes[i].id, "start") == 0) {
                            startNode = &g_tree->nodes[i];
                            break;
                        }
                    }
                    if (startNode) {
                        for (int i = 0; i < startNode->choice_count; i++) {
                            const char *next_id = startNode->choices[i].next_node;
                            bool visited = false;
                            for (int j = 0; j < g_tree->node_count; j++) {
                                if (strcmp(g_tree->nodes[j].id, next_id) == 0 && g_tree->nodes[j].visited) {
                                    visited = true;
                                    break;
                                }
                            }
                            if (!visited && active_count < MAX_CHOICES) {
                                sprintf(active_choices[active_count].text, "[Voltar] %s", startNode->choices[i].text);
                                strcpy(active_choices[active_count].next_node, startNode->choices[i].next_node);
                                active_count++;
                            }
                        }
                    }
                }
            }

            int total_buttons = active_count + 1;
            int startY = 355 - (total_buttons * 20);
            int exitY = startY;

            if (global_tokens > 0) {
                for (int i = 0; i < active_count; i++) {
                    Rectangle choiceArea = { 280, startY + (i * 20), 200, 18 };
                    if (clicou && CheckCollisionPointRec(mouse, ParaTela(choiceArea))) {
                        MakeChoiceById(g_tree, active_choices[i].next_node);
                        return;
                    }
                }
                exitY = startY + (active_count * 20);
            }
            
            Rectangle exitArea = { 280, exitY, 200, 18 };
            if (clicou && CheckCollisionPointRec(mouse, ParaTela(exitArea))) {
                g_dialogLoaded = false;
                g_tree = NULL;
                return;
            }
        }
    } else {
        int startY = 80;
        for (int i = 0; i < 3; i++) {
            Rectangle choiceArea = { 55, startY + (i * 50), 160, 36 };
            if (CheckCollisionPointRec(mouse, ParaTela(choiceArea))) {
                if (clicou) {
                    g_tree = &g_trees[i];
                    g_dialogLoaded = true;
                    return;
                }
            }
        }
    }
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

    // Background atras da tela (a moldura da tela cobre as bordas)
    Rectangle bgOrigem = { 0, 0, (float)g_background.width, (float)g_background.height };
    DrawTexturePro(g_background, bgOrigem, ParaTela(BG_AREA), (Vector2){ 0, 0 }, 0.0f, WHITE);
    // 1) Fundo
    RayCanvasDrawTexture(g_telaFechada, GetTelaRect(), WHITE);

    // 2) Menu de ferramentas por cima do fundo (so quando aberto)
    if (g_menuAberto) {
        Rectangle menuOrigem = { 0, 0, (float)g_menuFerramentas.width, (float)g_menuFerramentas.height };
        DrawTexturePro(g_menuFerramentas, menuOrigem, ParaTela(MENU_AREA), (Vector2){ 0, 0 }, 0.0f, WHITE);
        DrawWithHover(g_iconePrancheta, FERRAMENTAS[0]);
        DrawWithHover(g_iconeArquivos, FERRAMENTAS[1]);
        DrawWithHover(g_iconeMaleta, FERRAMENTAS[2]);
        DrawWithHover(g_iconeMapa, FERRAMENTAS[3]);
    }

    // 3) Seta por cima do fundo
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

    // 4) Nome e frase do interrogado (desenhados por ultimo, funcionou assim)
    DrawInterrogationTexts();

     // 5) Prancheta por cima de tudo
    if (g_pranchetaAberta) {
        Rectangle area   = { g_pranchetaPos.x, g_pranchetaPos.y,
                             (float)g_prancheta.width, (float)g_prancheta.height };
        Rectangle origem = { 0, 0, (float)g_prancheta.width, (float)g_prancheta.height };
        DrawTexturePro(g_prancheta, origem, ParaTela(area), (Vector2){ 0, 0 }, 0.0f, WHITE);
        // Quadradinhos por cima da prancheta (escurecem no hover)
        for (int i = 0; i < NUM_CAIXAS; i++) {
            Texture2D tex = g_marcado[i] ? g_caixaComX : g_caixaVazia;
            DrawWithHover(tex, CaixaNaImagem(i));
        }
    }

    RayCanvasEnd();
}

void UnloadGameplayScene(void) {
    
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_telaFechada);
    UnloadTexture(g_telaAberta);
    UnloadTexture(g_background);
    UnloadTexture(g_seta);
    UnloadTexture(g_menuFerramentas);
    UnloadTexture(g_iconePrancheta);
    UnloadTexture(g_iconeArquivos);
    UnloadTexture(g_iconeMaleta);
    UnloadTexture(g_iconeMapa);
    UnloadTexture(g_prancheta);
    UnloadTexture(g_caixaVazia);
    UnloadTexture(g_caixaComX);
    RayCanvasClose();
}