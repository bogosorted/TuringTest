#include "raylib.h"
#include <stdio.h>
#include <string.h>
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

// Fichas dos funcionarios (lidas de assets/data/fichas_funcionarios.txt)
typedef struct {
    char nome[64];
    char setor[64];
    char funcao[64];
    char personalidade[64];
    char comportamentos[3][160];
    bool ehIA;                  // true = sintetico (NAO aparece na ficha)
} Ficha;

#define MAX_FICHAS 10
static Ficha g_fichas[MAX_FICHAS];
static int   g_numFichas = 0;

// Ficha na tela
static Texture2D g_ficha;                     // sprite da ficha (240x320)
static int       g_funcionario     = 0;       // funcionario sendo entrevistado
static int       g_fichaAtual      = -1;      // -1 = ficha fechada
static bool      g_arrastandoFicha = false;
static Vector2   g_fichaPos        = { 200, 20 };
static Vector2   g_offsetFicha     = { 0, 0 };

static const Rectangle FICHA_FECHAR = { 210, 5, 24, 22 };  // botao X, dentro da ficha

// Cores da paleta do jogo
static const Color COR_PRETO = {   0,   0,   0, 255 };
static const Color COR_MEDIO = {  77,  83,  60, 255 };
static const Color COR_CLARO = { 139, 149, 109, 255 };

// Declarada aqui porque o InitGameplayScene usa ela antes de ela aparecer no arquivo
static void CarregarFichas(const char *caminho);



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
static void OnClickPasta(void) {
    if (g_funcionario >= g_numFichas) return;   // arquivo nao carregou
    // Abre a ficha do funcionario entrevistado (ou fecha, se ja estiver aberta)
    g_fichaAtual = (g_fichaAtual < 0) ? g_funcionario : -1;
}
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
// ---------------------------------------------------------------
// DICA (minigame de logica)
// ---------------------------------------------------------------
typedef struct {
    const char *frase;      // proposicao em portugues
    const char *p;          // o que "p" significa
    const char *q;          // o que "q" significa
    const char *opcoes[3];  // alternativas em linguagem simbolica
    int correta;            // indice da alternativa certa (0, 1 ou 2)
} Proposicao;

// Simbolos em ASCII (a fonte padrao da raylib nao tem os simbolos logicos):
//   ~ nao    ^ e    v ou    -> se...entao    <-> se e somente se
#define NUM_PROPOSICOES 6
static const Proposicao PROPOSICOES[NUM_PROPOSICOES] = {
    { "Patricia confere os relatorios e Caio nao revisa o codigo.",
      "Patricia confere os relatorios", "Caio revisa o codigo",
      { "p ^ ~q", "~p ^ q", "p v ~q" }, 0 },
    { "Se Lucas apresenta a campanha, entao Caio nao sai mais cedo.",
      "Lucas apresenta a campanha", "Caio sai mais cedo",
      { "~q -> p", "p ^ ~q", "p -> ~q" }, 2 },
    { "Patricia nao almoca na empresa ou Lucas chega atrasado.",
      "Patricia almoca na empresa", "Lucas chega atrasado",
      { "~p ^ q", "~p v q", "~(p v q)" }, 1 },
    { "Nao e verdade que o sistema caiu e o cofre abriu.",
      "o sistema caiu", "o cofre abriu",
      { "~(p ^ q)", "~p ^ ~q", "~p ^ q" }, 0 },
    { "Caio fala ingles se, e somente se, Patricia fala espanhol.",
      "Caio fala ingles", "Patricia fala espanhol",
      { "p -> q", "p ^ q", "p <-> q" }, 2 },
    { "Se Patricia nao assina o relatorio, entao o setor nao e auditado.",
      "Patricia assina o relatorio", "o setor e auditado",
      { "~q -> ~p", "~p -> ~q", "p -> q" }, 1 },
};

static Texture2D g_dicaAceso;     // botao disponivel
static Texture2D g_dicaApagado;   // botao bloqueado

// Posicao do botao: no cantinho com o menu fechado, ao lado do menu quando aberto (sprite 32x32)
static Rectangle DicaBotao(void) {
    if (g_menuAberto) return (Rectangle){ 528, 4, 32, 32 };   // colado na borda do menu (borda visivel em x = 560)
    return (Rectangle){ 604, 4, 32, 32 };                     // cantinho superior direito
}
static const Rectangle DICA_PAINEL = { 285, 40, 200, 222 };
static const Rectangle DICA_OK     = { 345, 238, 80, 18 };

static bool g_dicaUsadaHoje      = false;  // true depois de responder (certo ou errado)
static bool g_entrevistaLiberada = false;  // acertou: a entrevista atual tem tokens infinitos
static bool g_infinitosAntes     = false;  // como estava o global_infinite_tokens antes da dica

static bool g_dicaAberta = false;
static int  g_dicaEstado = 0;    // 0 = pergunta, 1 = acertou, 2 = errou
static int  g_dicaAtual  = 0;    // qual proposicao foi sorteada

// Retangulo da alternativa i dentro do painel
static Rectangle OpcaoDica(int i) {
    return (Rectangle){ DICA_PAINEL.x + 6, DICA_PAINEL.y + 136 + i * 20, DICA_PAINEL.width - 12, 18 };
}

// Uma vez por dia, e so durante uma conversa
static bool DicaDisponivel(void) {
    return g_dialogLoaded && !g_dicaUsadaHoje;
}

// Acertou: tokens infinitos so nesta entrevista
static void LiberarEntrevista(void) {
    g_infinitosAntes = global_infinite_tokens;
    global_infinite_tokens = true;
    g_entrevistaLiberada = true;
    // TODO: desbloquear o dialogo novo que revela se o funcionario e IA
}

// A entrevista acabou: tokens voltam ao normal
static void EncerrarEntrevistaLiberada(void) {
    if (g_entrevistaLiberada) global_infinite_tokens = g_infinitosAntes;
    g_entrevistaLiberada = false;
}

// Cuida do botao e do painel da dica.
// Devolve true se a dica "pegou" o mouse neste frame.
static bool UpdateDica(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Painel aberto: so ele recebe o mouse
    if (g_dicaAberta) {
        bool hover = false;
        if (g_dicaEstado == 0) {
            const Proposicao *p = &PROPOSICOES[g_dicaAtual];
            for (int i = 0; i < 3; i++) {
                if (!CheckCollisionPointRec(mouse, ParaTela(OpcaoDica(i)))) continue;
                hover = true;
                if (clicou) {
                    g_dicaUsadaHoje = true;   // certo ou errado, a dica acabou por hoje
                    if (i == p->correta) {
                        g_dicaEstado = 1;
                        LiberarEntrevista();
                    } else {
                        g_dicaEstado = 2;
                    }
                }
            }
        } else if (CheckCollisionPointRec(mouse, ParaTela(DICA_OK))) {
            hover = true;
            if (clicou) g_dicaAberta = false;
        }
        SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
        return true;
    }

    // Botao
    if (!DicaDisponivel()) return false;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !clicou) return false;  // esta arrastando algo
    if (!CheckCollisionPointRec(mouse, ParaTela(DicaBotao()))) return false;

    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    if (clicou) {
        g_dicaAtual  = GetRandomValue(0, NUM_PROPOSICOES - 1);
        g_dicaEstado = 0;
        g_dicaAberta = true;
    }
    return true;
}

// Desenha o botao e, se estiver aberto, o painel da dica
static void DesenharDica(void) {
    Color texto = {  30,  32,  34, 255 };
    Color fundo = { 120, 130, 110, 255 };
    Color claro = { 150, 160, 130, 255 };
    Vector2 mouse = RayCanvasGetMousePosition();

    // Botao: aceso (escurece no hover) ou apagado
    if (DicaDisponivel() && !g_dicaAberta) {
        DrawWithHover(g_dicaAceso, DicaBotao());
    } else {
        Rectangle origem = { 0, 0, (float)g_dicaApagado.width, (float)g_dicaApagado.height };
        DrawTexturePro(g_dicaApagado, origem, ParaTela(DicaBotao()), (Vector2){ 0, 0 }, 0.0f, WHITE);
    }

    if (!g_dicaAberta) return;

    const Proposicao *p = &PROPOSICOES[g_dicaAtual];
    float x = DICA_PAINEL.x + 6;
    float y = DICA_PAINEL.y;
    float largura = DICA_PAINEL.width - 12;

    // Escurece o resto da tela
    DrawRectangleRec(GetTelaRect(), Fade(BLACK, 0.4f));

    // Painel e barra de titulo
    Rectangle rPainel = ParaTela(DICA_PAINEL);
    DrawRectangleRec(rPainel, fundo);
    DrawRectangleLinesEx(rPainel, 2, texto);
    Rectangle titulo = { DICA_PAINEL.x, DICA_PAINEL.y, DICA_PAINEL.width, 16 };
    DrawRectangleRec(ParaTela(titulo), texto);
    DrawTextInArea("DICA - LOGICA", titulo, 10, fundo);

    // Proposicao e legenda
    char linha[160];
    DrawTextWrappedLeftAligned(p->frase, (Vector2){ x, y + 22 }, largura, 10, texto);
    snprintf(linha, sizeof linha, "p: %s", p->p);
    DrawTextWrappedLeftAligned(linha, (Vector2){ x, y + 66 }, largura, 10, texto);
    snprintf(linha, sizeof linha, "q: %s", p->q);
    DrawTextWrappedLeftAligned(linha, (Vector2){ x, y + 94 }, largura, 10, texto);

    if (g_dicaEstado == 0) {
        // Pergunta e as 3 alternativas
        DrawTextLeftAligned("Qual a forma simbolica?", (Vector2){ x, y + 120 }, 10, texto);
        for (int i = 0; i < 3; i++) {
            Rectangle op = OpcaoDica(i);
            Rectangle rOp = ParaTela(op);
            bool hover = CheckCollisionPointRec(mouse, rOp);
            DrawRectangleRec(rOp, hover ? claro : fundo);
            DrawRectangleLinesEx(rOp, 2, texto);
            DrawTextInArea(p->opcoes[i], op, 10, texto);
        }
    } else {
        // Resultado
        char msg[200];
        const char *nome = (g_funcionario < g_numFichas) ? g_fichas[g_funcionario].nome : "este funcionario";
        if (g_dicaEstado == 1) {
            snprintf(msg, sizeof msg, "Correto! A entrevista com %s nao gasta tokens.", nome);
        } else {
            snprintf(msg, sizeof msg, "Errado. A resposta era: %s. A dica so volta amanha.",
                     p->opcoes[p->correta]);
        }
        DrawTextWrappedLeftAligned(msg, (Vector2){ x, y + 136 }, largura, 10, texto);

        Rectangle rOk = ParaTela(DICA_OK);
        bool hoverOk = CheckCollisionPointRec(mouse, rOk);
        DrawRectangleRec(rOk, hoverOk ? claro : fundo);
        DrawRectangleLinesEx(rOk, 2, texto);
        DrawTextInArea("OK", DICA_OK, 10, texto);
    }
}

static void DrawInterrogationTexts(void) {
    Color textColor = (Color){ 30, 32, 34, 255 };
    Color bgColor = (Color){ 120, 130, 110, 255 }; 
    Color hoverColor = (Color){ 150, 160, 130, 255 };

    char bufTokens[32];
    if (global_infinite_tokens) sprintf(bufTokens, "Tokens: infinitos");
    else                        sprintf(bufTokens, "Tokens: %d", global_tokens);
    DrawTextLeftAligned(bufTokens, (Vector2){ 280, 10 }, 10, textColor);

    char ficha[256];
    sprintf(ficha, "LOCAL:\nSala de Interrogatorio");
    DrawTextWrappedLeftAligned(ficha, (Vector2){ 10, 322 }, 250.0f, 10, textColor);

    if (!g_dialogLoaded) {
        const char *labels[] = { "Ir até Patricia", "Ir até Caio", "Ir até Lucas" };
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

    // Mesma lista que o clique usa (ver UpdateGameplay)
    DialogChoice active_choices[MAX_CHOICES];
    int active_count = HasTokens() ? GetAvailableChoices(g_tree, active_choices) : 0;

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

    if (HasTokens()) {
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
    if (HasTokens()) {
        DrawTextInArea("Pode voltar ao trabalho.", exitArea, 10, textColor);
    } else {
        DrawTextInArea("To cansado, depois falo contigo.", exitArea, 10, textColor);
    }
}

bool GameplaySceneShouldClose(void) {
    return g_shouldClose;
}

void GameplaySceneSetFuncionario(int indice) {
    if (indice < 0) indice = 0;
    g_funcionario = indice;
}

bool GameplaySceneGoToSelection(void) {
    return g_irParaSelecao;
}

// Faz o proximo InitGameplayScene recarregar as arvores (zera os "visited").
// O LoadDialogTree aloca memoria, entao libera as arvores antigas antes.
void GameplaySceneResetDialogues(void) {
    if (g_treesLoaded) {
        for (int i = 0; i < 3; i++) FreeDialogTree(&g_trees[i]);
    }
    g_treesLoaded = false;
    g_dialogLoaded = false;
        g_tree = NULL;

    // Dia novo: a dica volta a ficar disponivel
    g_dicaUsadaHoje = false;
    g_dicaAberta = false;
    EncerrarEntrevistaLiberada();
}


// true quando o dia pode terminar (o botao "proximo dia" / "veredito" aparece):
//   1) os tokens acabaram, OU
//   2) os 3 funcionarios ja nao tem nenhuma escolha restante.
// Obs.: nos arquivos de dialogo atuais os ramos se excluem (ex.: depois de
// "funcao", so da para ver UM entre "descanso" e "eficiente"), entao "nao ha
// mais escolhas" e o que significa "ver todos os dialogos" na pratica.
bool GameplaySceneDayFinished(void) {
    if (!HasTokens()) return true;

    if (!g_treesLoaded) return false;
    for (int t = 0; t < 3; t++) {
        if (g_trees[t].node_count == 0) return false;   // arquivo nao carregou

        DialogChoice restantes[MAX_CHOICES];
        if (GetAvailableChoices(&g_trees[t], restantes) > 0) return false;
    }
    return true;
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

    g_dicaAceso = LoadTexture("assets/gameplay_scene/spr_botao_dica_aceso.png");
    SetTextureFilter(g_dicaAceso, TEXTURE_FILTER_POINT);
    g_dicaApagado = LoadTexture("assets/gameplay_scene/spr_botao_dica_apagado.png");
    SetTextureFilter(g_dicaApagado, TEXTURE_FILTER_POINT);
    g_dicaAberta = false;

    CarregarFichas("assets/data/fichas_funcionarios.txt");

    g_ficha = LoadTexture("assets/gameplay_scene/spr_placeholder_ficha.png");
    SetTextureFilter(g_ficha, TEXTURE_FILTER_POINT);

    g_fichaAtual = -1;
    g_arrastandoFicha = false;
    g_fichaPos = (Vector2){ 200, 20 };

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
// ---------- Leitura do arquivo de fichas ----------

// Troca letras acentuadas (UTF-8) pela versao sem acento: "Funcao"
static void RemoverAcentos(char *dest, int tam, const char *src) {
    static const char *TABELA = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    int j = 0;
    for (int i = 0; src[i] && j < tam - 1; i++) {
        unsigned char c  = (unsigned char)src[i];
        unsigned char c2 = (unsigned char)src[i + 1];
        if (c < 0x80) {
            dest[j++] = (char)c;                      // letra normal
        } else if (c == 0xC3 && c2 >= 0x80 && c2 <= 0xBF) {
            dest[j++] = TABELA[c2 - 0x80];            // letra acentuada
            i++;
        }
        // outros caracteres especiais sao ignorados
    }
    dest[j] = '\0';
}

// Copia tirando espacos do comeco e a quebra de linha do fim
static void CopiarTexto(char *dest, int tam, const char *src) {
    while (*src == ' ') src++;
    snprintf(dest, tam, "%s", src);
    int n = (int)strlen(dest);
    while (n > 0 && (dest[n - 1] == '\n' || dest[n - 1] == '\r' || dest[n - 1] == ' ')) {
        dest[--n] = '\0';
    }
}

// Devolve o que vem depois do ':' ("Nome: Caio" -> " Caio")
static const char *DepoisDosDoisPontos(const char *linha) {
    const char *p = strchr(linha, ':');
    return p ? p + 1 : linha;
}

// Le o arquivo de fichas e preenche g_fichas
static void CarregarFichas(const char *caminho) {
    g_numFichas = 0;

    FILE *arq = fopen(caminho, "r");
    if (!arq) {
        TraceLog(LOG_WARNING, "Nao foi possivel abrir %s", caminho);
        return;
    }

    char bruta[256], linha[256];
    Ficha *f = NULL;
    int nComp = 0;

    while (fgets(bruta, sizeof bruta, arq)) {
        RemoverAcentos(linha, sizeof linha, bruta);

        if (strncmp(linha, "Funcionario", 11) == 0) {
            // Comeca uma ficha nova
            if (g_numFichas >= MAX_FICHAS) break;
            f = &g_fichas[g_numFichas++];
            memset(f, 0, sizeof *f);
            nComp = 0;
        }
        else if (f == NULL) {
            continue;   // ignora o que vier antes do primeiro funcionario
        }
        else if (strncmp(linha, "Nome:", 5) == 0) {
            CopiarTexto(f->nome, sizeof f->nome, DepoisDosDoisPontos(linha));
        }
        else if (strncmp(linha, "Setor:", 6) == 0) {
            CopiarTexto(f->setor, sizeof f->setor, DepoisDosDoisPontos(linha));
        }
        else if (strncmp(linha, "Funcao:", 7) == 0) {
            CopiarTexto(f->funcao, sizeof f->funcao, DepoisDosDoisPontos(linha));
        }
        else if (strncmp(linha, "Personalidade", 13) == 0) {
            CopiarTexto(f->personalidade, sizeof f->personalidade, DepoisDosDoisPontos(linha));
        }
        else if (strncmp(linha, "IA:", 3) == 0) {
            const char *v = DepoisDosDoisPontos(linha);
            while (*v == ' ') v++;
            f->ehIA = (*v == 'S' || *v == 's');
        }
        else if (linha[0] == '-' && nComp < 3) {
            CopiarTexto(f->comportamentos[nComp], sizeof f->comportamentos[0], linha + 1);
            nComp++;
        }
    }

    fclose(arq);
    TraceLog(LOG_INFO, "Fichas carregadas: %d", g_numFichas);
}

// ---------- Texto ----------

// Escreve texto usando coordenadas da imagem 640x360
static void TextoNaImagem(const char *txt, float x, float y, float tam, Color cor) {
    Rectangle r = ParaTela((Rectangle){ x, y, 0, tam });
    DrawTextEx(GetFontDefault(), txt, (Vector2){ r.x, r.y }, r.height, r.height / 10.0f, cor);
}

// Escreve numa linha so; se nao couber em 'largura', diminui a letra
static void TextoQueCabe(const char *txt, float x, float y, float largura, float tam, Color cor) {
    while (tam > 6 && MeasureTextEx(GetFontDefault(), txt, tam, tam / 10.0f).x > largura) tam -= 1;
    TextoNaImagem(txt, x, y, tam, cor);
}

// Escreve quebrando em varias linhas de no maximo 'largura' pixels.
// Devolve quantas linhas usou.
static int TextoQuebrado(const char *txt, float x, float y, float largura,
                         float tam, float entreLinhas, Color cor) {
    char linha[192] = "";
    char palavra[64];
    char teste[256];
    int linhas = 0;
    const char *p = txt;

    while (*p) {
        // Pega a proxima palavra
        int n = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && n < 63) palavra[n++] = *p++;
        palavra[n] = '\0';
        if (n == 0) break;

        if (linha[0]) snprintf(teste, sizeof teste, "%s %s", linha, palavra);
        else          snprintf(teste, sizeof teste, "%s", palavra);

        // Nao coube: escreve a linha atual e comeca outra com essa palavra
        if (linha[0] && MeasureTextEx(GetFontDefault(), teste, tam, tam / 10.0f).x > largura) {
            TextoNaImagem(linha, x, y + linhas * entreLinhas, tam, cor);
            linhas++;
            snprintf(linha, sizeof linha, "%s", palavra);
        } else {
            snprintf(linha, sizeof linha, "%s", teste);
        }
    }

    if (linha[0]) {
        TextoNaImagem(linha, x, y + linhas * entreLinhas, tam, cor);
        linhas++;
    }
    return linhas;
}

// ---------- Arraste da ficha ----------

// Converte algo que esta DENTRO da ficha para a imagem (acompanha o arraste)
static Rectangle NaFicha(Rectangle r) {
    return (Rectangle){ g_fichaPos.x + r.x, g_fichaPos.y + r.y, r.width, r.height };
}

// Cuida do arraste e do botao X da ficha.
// Devolve true se a ficha "pegou" o mouse neste frame.
static bool UpdateFicha(void) {
    if (g_fichaAtual < 0) return false;

    Vector2 mouse = MouseNaImagem();
    Rectangle area = { g_fichaPos.x, g_fichaPos.y, (float)g_ficha.width, (float)g_ficha.height };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Botao X: fecha a ficha
        if (CheckCollisionPointRec(mouse, NaFicha(FICHA_FECHAR))) {
            g_fichaAtual = -1;
            g_arrastandoFicha = false;
            return true;
        }
        // Clicou na ficha: comeca a arrastar
        if (CheckCollisionPointRec(mouse, area)) {
            g_arrastandoFicha = true;
            g_offsetFicha = (Vector2){ mouse.x - g_fichaPos.x, mouse.y - g_fichaPos.y };
        }
    }

    if (!g_arrastandoFicha) return false;

    // Soltou o botao: para de arrastar
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        g_arrastandoFicha = false;
        return false;
    }

    // Arrastando: a ficha segue o mouse
    g_fichaPos.x = mouse.x - g_offsetFicha.x;
    g_fichaPos.y = mouse.y - g_offsetFicha.y;

    // Nao deixa sair da tela
    float maxX = (float)(g_telaFechada.width  - g_ficha.width);
    float maxY = (float)(g_telaFechada.height - g_ficha.height);
    if (g_fichaPos.x < 0)    g_fichaPos.x = 0;
    if (g_fichaPos.y < 0)    g_fichaPos.y = 0;
    if (g_fichaPos.x > maxX) g_fichaPos.x = maxX;
    if (g_fichaPos.y > maxY) g_fichaPos.y = maxY;

    SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    return true;
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
            // Mesma lista que o desenho usa (ver DrawInterrogationTexts)
            DialogChoice active_choices[MAX_CHOICES];
            int active_count = HasTokens() ? GetAvailableChoices(g_tree, active_choices) : 0;

            int total_buttons = active_count + 1;
            int startY = 355 - (total_buttons * 20);
            int exitY = startY;

            if (HasTokens()) {
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
                EncerrarEntrevistaLiberada();
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
                    g_funcionario = i;     // o botao clicado define qual ficha a pasta abre
                    g_fichaAtual = -1;     // fecha a ficha do funcionario anterior, se estiver aberta
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

    // A ficha e a prancheta ficam por cima de tudo, entao elas tem prioridade no clique
        if (!UpdateDica() && !UpdateFicha() && !UpdatePrancheta()) {
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
    // 5) Ficha do funcionario (por cima de tudo)
    if (g_fichaAtual >= 0) {
        const Ficha *f = &g_fichas[g_fichaAtual];
        float x = g_fichaPos.x;
        float y = g_fichaPos.y;

        Rectangle area   = { x, y, (float)g_ficha.width, (float)g_ficha.height };
        Rectangle origem = { 0, 0, (float)g_ficha.width, (float)g_ficha.height };
        DrawTexturePro(g_ficha, origem, ParaTela(area), (Vector2){ 0, 0 }, 0.0f, WHITE);

        // Botao X escurece no hover
        Rectangle fechar = ParaTela(NaFicha(FICHA_FECHAR));
        if (CheckCollisionPointRec(RayCanvasGetMousePosition(), fechar)) {
            DrawRectangleRec(fechar, Fade(BLACK, 0.3f));
        }

        TextoNaImagem("FICHA DO FUNCIONARIO", x + 12, y + 11, 10, COR_CLARO);

        // Ao lado da foto
        TextoNaImagem("NOME",          x + 86, y + 40, 8, COR_MEDIO);
        TextoQueCabe(f->nome,          x + 86, y + 52, 140, 10, COR_PRETO);
        TextoNaImagem("PERSONALIDADE", x + 86, y + 70, 8, COR_MEDIO);
        TextoQueCabe(f->personalidade, x + 86, y + 82, 140, 10, COR_PRETO);

        // Largura inteira
        TextoNaImagem("SETOR",  x + 14, y + 122, 8, COR_MEDIO);
        TextoQueCabe(f->setor,  x + 14, y + 134, 212, 10, COR_PRETO);
        TextoNaImagem("FUNCAO", x + 14, y + 152, 8, COR_MEDIO);
        TextoQueCabe(f->funcao, x + 14, y + 164, 212, 10, COR_PRETO);

        // Comportamentos: um tracinho por item, quebrando nas linhas da caixa
        TextoNaImagem("COMPORTAMENTOS", x + 14, y + 182, 8, COR_MEDIO);
        float linhaY = y + 198;
        for (int i = 0; i < 3; i++) {
            if (f->comportamentos[i][0] == '\0') continue;   // item vazio
            TextoNaImagem("-", x + 18, linhaY, 10, COR_PRETO);
            int usadas = TextoQuebrado(f->comportamentos[i], x + 26, linhaY, 196, 10, 12, COR_PRETO);
            linhaY += usadas * 12;
        }

        TextoNaImagem("RASEC - CONFIDENCIAL", x + 14, y + 302, 8, COR_MEDIO);
    }
    // 6) Botao e painel da dica (por cima de tudo)
    DesenharDica();

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
    UnloadTexture(g_ficha);
    UnloadTexture(g_caixaVazia);
    UnloadTexture(g_caixaComX);
    UnloadTexture(g_dicaAceso);
    UnloadTexture(g_dicaApagado);
    EncerrarEntrevistaLiberada();   // sair pelo mapa tambem encerra a entrevista
    RayCanvasClose();
}