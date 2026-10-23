#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include "gameplay_scene.h"
#include "selection_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include "../utils/game_state.h"
#include <stddef.h>

#define NUM_TOOLS 4

static DialogTree g_trees[3];
static bool g_treesLoaded = false;
static DialogTree *g_tree = NULL;
static bool g_dialogLoaded = false;

// Texturas dos personagens (Patricia, Andre, Lucas)
static Texture2D g_charTextures[3];
static int       g_suspeitoAtual = 0;       // 0 = Patricia, 1 = Andre, 2 = Lucas
static const int SUSPECT_FICHA_INDEX[3] = { 0, 5, 2 }; // Patricia=0, Andre=5, Lucas=2 em fichas_funcionarios.txt


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



// Guia da investigacao (checklist da prancheta)
#define NUM_CAIXAS 7
static Texture2D g_caixaVazia;                          // quadradinho sem X
static Texture2D g_caixaComX;                           // quadradinho com X
static bool      g_marcado[MAX_FICHAS][NUM_CAIXAS];     // um checklist por funcionario

static const char *GUIA[NUM_CAIXAS] = {
    "Inverte ordem dos acontecimentos.",
    "Nao lembra de acontecimentos cotidianos.",
    "Falas padronizadas demais.",
    "Inventou fatos.",
    "Mudou contexto dito anteriormente.",
    "Mudou de personalidade.",
    "Segue muito a risca a personalidade descrita.",
};

// Posicao dos quadradinhos DENTRO da prancheta (sprite 20x20)
#define CAIXA_X      58     // x de todos os quadradinhos
#define CAIXA_Y0     84     // y do primeiro
#define CAIXA_PASSO  30     // distancia entre um e outro
#define TEXTO_X      84     // x dos textos (ao lado do quadradinho)

static bool g_menuAberto = false;
static bool g_shouldClose = false;
static bool g_irParaSelecao = false;  // true quando clica no mapa

// Passos do tutorial interativo forcado no Dia 1
typedef enum {
    TUT_PASSO_SUSPEITO = 0,     // 1/6: Escolher um suspeito para conversar
    TUT_PASSO_ABRIR_MENU,       // 2/6: Abrir a aba lateral de ferramentas [<]
    TUT_PASSO_ABRIR_FICHA,      // 3/6: Clicar na Pasta de Arquivos para ver a ficha
    TUT_PASSO_FECHAR_FICHA,     // 4/6: Ler a ficha e clicar no [X] para fechar
    TUT_PASSO_PERGUNTA,         // 5/6: Fazer uma pergunta e entender os Tokens
    TUT_PASSO_ABRIR_PRANCHETA,  // 6/6: Abrir a Prancheta de anomalias
    TUT_PASSO_CONCLUSAO         // Resumo da Dica Logica + Mapa e botao de iniciar
} TutorialPasso;

static TutorialPasso g_tutorialPasso = TUT_PASSO_SUSPEITO;

static bool TutorialAtivo(void) {
    return GameStateIsTutorialActive();
}

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
    if (!g_dialogLoaded) return;                // ninguem sendo entrevistado
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
// O tamanho da fonte acompanha o zoom da tela e encolhe se o texto nao couber.
static void DrawTextInArea(const char *texto, Rectangle area, int tamanhoFonte, Color cor) {
    Rectangle destino = ParaTela(area);
    float escala = destino.height / area.height;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;

    float margem = 6.0f * escala;
    float maxLargura = destino.width - margem * 2.0f;
    if (maxLargura < 10.0f) maxLargura = destino.width;

    int minTam = (int)(5.0f * escala);
    if (minTam < 1) minTam = 1;

    while (tam > minTam && MeasureText(texto, tam) > maxLargura) {
        tam--;
    }

    int largura = MeasureText(texto, tam);
    float x = destino.x + (destino.width - largura) / 2.0f;
    if (x < destino.x + margem) x = destino.x + margem;
    float y = destino.y + (destino.height - tam) / 2.0f;

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
    
    char buffer[1024];
    strncpy(buffer, texto, 1023);
    buffer[1023] = '\0';
    
    char *word = strtok(buffer, " ");
    char line[1024] = "";
    int yOffset = 0;
    
    while (word != NULL) {
        char testLine[1024];
        strncpy(testLine, line, 1023);
        testLine[1023] = '\0';
        if (strlen(testLine) > 0) {
            strncat(testLine, " ", 1023 - strlen(testLine));
        }
        strncat(testLine, word, 1023 - strlen(testLine));
        
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
    { "Patrícia confere os relatórios e Caio não revisa o código.",
      "Patrícia confere os relatórios", "Caio revisa o código",
      { "p ^ ~q", "~p ^ q", "p v ~q" }, 0 },
    { "Se Lucas apresenta a campanha, então Caio não sai mais cedo.",
      "Lucas apresenta a campanha", "Caio sai mais cedo",
      { "~q -> p", "p ^ ~q", "p -> ~q" }, 2 },
    { "Patrícia não almoça na empresa ou Lucas chega atrasado.",
      "Patrícia almoça na empresa", "Lucas chega atrasado",
      { "~p ^ q", "~p v q", "~(p v q)" }, 1 },
    { "Não é verdade que o sistema caiu e o cofre abriu.",
      "o sistema caiu", "o cofre abriu",
      { "~(p ^ q)", "~p ^ ~q", "~p ^ q" }, 0 },
    { "Caio fala inglês se, e somente se, Patrícia fala espanhol.",
      "Caio fala inglês", "Patrícia fala espanhol",
      { "p -> q", "p ^ q", "p <-> q" }, 2 },
    { "Se Patrícia não assina o relatório, então o setor não é auditado.",
      "Patrícia assina o relatório", "o setor é auditado",
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
    if (TutorialAtivo()) return false;
    Vector2 mouse = RayCanvasGetMousePosition();
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Painel aberto: so ele recebe o mouse
    if (g_dicaAberta) {
        bool hover = false;

        // Botao [X] para fechar/desistir momentaneamente (padronizado com a ficha)
        Rectangle btnFechar = { DICA_PAINEL.x + DICA_PAINEL.width - 24, DICA_PAINEL.y, 24, 22 };
        if (CheckCollisionPointRec(mouse, ParaTela(btnFechar))) {
            hover = true;
            if (clicou) {
                g_dicaAberta = false;
                return true;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            g_dicaAberta = false;
            return true;
        }

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
    Rectangle titulo = { DICA_PAINEL.x, DICA_PAINEL.y, DICA_PAINEL.width - 24, 22 };
    DrawRectangleRec(ParaTela(titulo), texto);
    DrawTextInArea("DICA - LÓGICA", titulo, 10, fundo);

    // Botao [X] de fechar (padronizado com o sprite e efeito da ficha de funcionario)
    Rectangle btnFechar = { DICA_PAINEL.x + DICA_PAINEL.width - 24, DICA_PAINEL.y, 24, 22 };
    Rectangle rFechar = ParaTela(btnFechar);
    DrawTexturePro(g_ficha, FICHA_FECHAR, rFechar, (Vector2){ 0, 0 }, 0.0f, WHITE);
    if (CheckCollisionPointRec(mouse, rFechar)) {
        DrawRectangleRec(rFechar, Fade(BLACK, 0.3f));
    }

    // Proposicao e legenda
    char linha[160];
    DrawTextWrappedLeftAligned(p->frase, (Vector2){ x, y + 22 }, largura, 10, texto);
    snprintf(linha, sizeof linha, "p: %s", p->p);
    DrawTextWrappedLeftAligned(linha, (Vector2){ x, y + 66 }, largura, 10, texto);
    snprintf(linha, sizeof linha, "q: %s", p->q);
    DrawTextWrappedLeftAligned(linha, (Vector2){ x, y + 94 }, largura, 10, texto);

    if (g_dicaEstado == 0) {
        // Pergunta e as 3 alternativas
        DrawTextLeftAligned("Qual a forma simbólica?", (Vector2){ x, y + 120 }, 10, texto);
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
        const char *nome = (g_funcionario < g_numFichas) ? g_fichas[g_funcionario].nome : "este funcionário";
        if (g_dicaEstado == 1) {
            snprintf(msg, sizeof msg, "Correto! A entrevista com %s não gasta tokens.", nome);
        } else {
            snprintf(msg, sizeof msg, "Errado. A resposta era: %s. A dica só volta amanhã.",
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

#define DIALOG_BOX_X      280.0f
#define DIALOG_BOX_W      226.0f
#define DIALOG_BTN_H      22.0f
#define DIALOG_BTN_STEP   25.0f
#define DIALOG_TEXT_H     84.0f

#define MAX_CHAT_MESSAGES 64

typedef struct {
    char speaker[32];
    char text[512];
    bool is_player;
} ChatEntry;

typedef struct {
    ChatEntry entries[MAX_CHAT_MESSAGES];
    int count;
    float scrollOffset;
} ChatHistory;

static ChatHistory g_chatHistories[3]; // 0 = Patrícia, 1 = André, 2 = Lucas

static void AdicionarMensagemChat(int suspeito, const char *speaker, const char *text, bool is_player) {
    if (suspeito < 0 || suspeito >= 3) return;
    if (!text || text[0] == '\0') return;
    ChatHistory *hist = &g_chatHistories[suspeito];
    if (hist->count >= MAX_CHAT_MESSAGES) return;

    // Evita duplicatas consecutivas idênticas
    if (hist->count > 0 &&
        strcmp(hist->entries[hist->count - 1].speaker, speaker) == 0 &&
        strcmp(hist->entries[hist->count - 1].text, text) == 0) {
        return;
    }

    snprintf(hist->entries[hist->count].speaker, sizeof(hist->entries[0].speaker), "%s", speaker);
    snprintf(hist->entries[hist->count].text, sizeof(hist->entries[0].text), "%s", text);
    hist->entries[hist->count].is_player = is_player;
    hist->count++;
    hist->scrollOffset = 99999.0f; // auto-scroll para a mais recente
}

static float MedirAlturaTextoQuebrado(const char *texto, float maxLargura, int tamanhoFonte) {
    Rectangle fakeArea = {0, 0, maxLargura, 10};
    float maxScaledW = ParaTela(fakeArea).width;
    float escala = ParaTela(fakeArea).height / 10.0f;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;

    char buffer[1024];
    strncpy(buffer, texto, 1023);
    buffer[1023] = '\0';

    char *word = strtok(buffer, " ");
    char line[1024] = "";
    int numLines = 0;

    while (word != NULL) {
        char testLine[1024];
        strncpy(testLine, line, 1023);
        testLine[1023] = '\0';
        if (strlen(testLine) > 0) strncat(testLine, " ", 1023 - strlen(testLine));
        strncat(testLine, word, 1023 - strlen(testLine));

        if (MeasureText(testLine, tam) > maxScaledW && strlen(line) > 0) {
            numLines++;
            strcpy(line, word);
        } else {
            strcpy(line, testLine);
        }
        word = strtok(NULL, " ");
    }
    if (strlen(line) > 0) numLines++;
    if (numLines < 1) numLines = 1;

    return (float)(numLines * (tamanhoFonte + 4));
}

static void DesenharHistoricoChat(Rectangle area, int suspectIndex) {
    if (suspectIndex < 0 || suspectIndex >= 3) return;
    ChatHistory *hist = &g_chatHistories[suspectIndex];

    Color textColor  = (Color){ 30, 32, 34, 255 };
    Color bgColor    = (Color){ 120, 130, 110, 255 };
    Color hoverColor = (Color){ 150, 160, 130, 255 };

    // Barra de título da conversa com o nome do contato (verde-escuro padrão)
    Rectangle rTitulo = { area.x, area.y, area.width, 16 };
    DrawRectangleRec(ParaTela(rTitulo), textColor);
    const char *nomesSuspeitos[] = { "PATRÍCIA", "ANDRÉ", "LUCAS" };
    DrawTextInArea(nomesSuspeitos[suspectIndex], rTitulo, 10, bgColor);

    // Área do corpo do chat (fundo verde-oliva padrão da paleta)
    Rectangle rCorpo = { area.x, area.y + 16, area.width, area.height - 16 };
    Rectangle rCorpoTela = ParaTela(rCorpo);
    DrawRectangleRec(rCorpoTela, bgColor);
    DrawRectangleLinesEx(rCorpoTela, 2, textColor);

    if (hist->count == 0) {
        DrawTextInArea("(Nenhuma mensagem anterior)", rCorpo, 9, textColor);
        return;
    }

    // Calcula a altura de cada bolha no espaço 640x360
    float maxBubbleW = area.width - 16.0f; // ~210px
    float bubbleHeights[MAX_CHAT_MESSAGES];
    float totalH = 4.0f;

    for (int i = 0; i < hist->count; i++) {
        float textH = MedirAlturaTextoQuebrado(hist->entries[i].text, maxBubbleW - 10.0f, 9);
        bubbleHeights[i] = 14.0f + textH + 4.0f;
        totalH += bubbleHeights[i] + 4.0f;
    }

    float visivelH = rCorpo.height - 6.0f;
    float maxScroll = (totalH > visivelH) ? (totalH - visivelH) : 0.0f;

    // Rolagem com a roda do mouse quando o ponteiro estiver em cima da tela do chat
    Vector2 mouse = RayCanvasGetMousePosition();
    if (CheckCollisionPointRec(mouse, rCorpoTela)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            hist->scrollOffset -= wheel * 18.0f;
        }
    }
    if (hist->scrollOffset < 0) hist->scrollOffset = 0;
    if (hist->scrollOffset > maxScroll) hist->scrollOffset = maxScroll;

    // Recorte para não vazar fora do painel
    if (rCorpoTela.width > 4 && rCorpoTela.height > 4) {
        BeginScissorMode((int)rCorpoTela.x + 2, (int)rCorpoTela.y + 2,
                         (int)rCorpoTela.width - 4, (int)rCorpoTela.height - 4);

        float currY = rCorpo.y + 4.0f - hist->scrollOffset;
        for (int i = 0; i < hist->count; i++) {
            float bH = bubbleHeights[i];
            float bW = maxBubbleW;
            bool isPl = hist->entries[i].is_player;

            // Bolha do jogador à direita; do NPC à esquerda
            float bX = isPl ? (area.x + area.width - bW - 4.0f) : (area.x + 4.0f);
            Rectangle rBubble = { bX, currY, bW, bH };
            Rectangle rBubbleTela = ParaTela(rBubble);

            if (rBubbleTela.y + rBubbleTela.height >= rCorpoTela.y && rBubbleTela.y <= rCorpoTela.y + rCorpoTela.height) {
                // Cores harmonizadas com a paleta oficial (hoverColor e tom claro)
                Color bCor = isPl ? hoverColor : (Color){ 135, 145, 125, 255 };
                DrawRectangleRec(rBubbleTela, bCor);
                DrawRectangleLinesEx(rBubbleTela, 1, textColor);

                // Texto em textColor padrão (tamanho aumentado para maior legibilidade)
                DrawTextLeftAligned(hist->entries[i].speaker, (Vector2){ bX + 5, currY + 3 }, 8, textColor);
                DrawTextWrappedLeftAligned(hist->entries[i].text, (Vector2){ bX + 5, currY + 14 }, bW - 10.0f, 9, textColor);
            }
            currY += bH + 4.0f;
        }

        EndScissorMode();
    }

    // Barra de rolagem visual
    if (maxScroll > 0) {
        float barH = (visivelH / totalH) * visivelH;
        if (barH < 8.0f) barH = 8.0f;
        float barY = rCorpo.y + 3.0f + (hist->scrollOffset / maxScroll) * (visivelH - barH);
        Rectangle rBar = { area.x + area.width - 4, barY, 2, barH };
        DrawRectangleRec(ParaTela(rBar), textColor);
    }
}

static void DrawInterrogationTexts(void) {
    Color textColor = (Color){ 30, 32, 34, 255 };
    Color bgColor = (Color){ 120, 130, 110, 255 }; 
    Color hoverColor = (Color){ 150, 160, 130, 255 };

    char bufTokens[32];
    if (global_infinite_tokens) sprintf(bufTokens, "Tokens: infinitos");
    else                        sprintf(bufTokens, "Tokens: %d", global_tokens);
    DrawTextLeftAligned(bufTokens, (Vector2){ DIALOG_BOX_X, 6 }, 10, textColor);

    char ficha[256];
    sprintf(ficha, "LOCAL:\nSala de Interrogatório");
    DrawTextWrappedLeftAligned(ficha, (Vector2){ 10, 322 }, 250.0f, 10, textColor);

    if (!g_dialogLoaded) {
        const char *labels[] = { "Ir até Patrícia", "Ir até André", "Ir até Lucas" };
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

    // 1) Janela unificada da conversa estilo chat (ocupa o espaço até os botões)
    Rectangle chatArea = { DIALOG_BOX_X, 18, DIALOG_BOX_W, 230 };
    DesenharHistoricoChat(chatArea, g_suspeitoAtual);

    // 3) Botões de respostas do jogador
    DialogChoice active_choices[MAX_CHOICES];
    int active_count = HasTokens() ? GetAvailableChoices(g_tree, active_choices) : 0;

    int startY = 252;
    int exitY = startY;

    if (HasTokens()) {
        for (int i = 0; i < active_count; i++) {
            Rectangle choiceArea = { DIALOG_BOX_X, startY + (i * (int)DIALOG_BTN_STEP), DIALOG_BOX_W, DIALOG_BTN_H };
            Rectangle rTela = ParaTela(choiceArea);
            
            bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rTela);
            Color boxColor = hover ? hoverColor : bgColor;
            
            DrawRectangleRec(rTela, boxColor);
            DrawRectangleLinesEx(rTela, 2, textColor);
            DrawTextInArea(active_choices[i].text, choiceArea, 10, textColor);
        }
        exitY = startY + (active_count * (int)DIALOG_BTN_STEP);
    }

    Rectangle exitArea = { DIALOG_BOX_X, exitY, DIALOG_BOX_W, DIALOG_BTN_H };
    Rectangle rTela = ParaTela(exitArea);
    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rTela);
    Color boxColor = hover ? hoverColor : bgColor;
    DrawRectangleRec(rTela, boxColor);
    DrawRectangleLinesEx(rTela, 2, textColor);
    if (HasTokens()) {
        DrawTextInArea("Pode voltar ao trabalho.", exitArea, 10, textColor);
    } else {
        DrawTextInArea("Tô cansado, depois falo contigo.", exitArea, 10, textColor);
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
    memset(g_chatHistories, 0, sizeof(g_chatHistories));

    // Jogo novo (dia 1): limpa os checklists de todos os funcionarios
    if (GameStateGetDay() == 1) {
        memset(g_marcado, 0, sizeof g_marcado);
        g_tutorialPasso = TUT_PASSO_SUSPEITO;
    }
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
        int dia = GameStateGetDay();
        char path[128];
        snprintf(path, sizeof path, "assets/dialogues/patricia_dia%d.txt", dia);
        LoadDialogTree(path, &g_trees[0]);

        snprintf(path, sizeof path, "assets/dialogues/andre_dia%d.txt", dia);
        LoadDialogTree(path, &g_trees[1]);

        snprintf(path, sizeof path, "assets/dialogues/lucas_dia%d.txt", dia);
        LoadDialogTree(path, &g_trees[2]);
        g_treesLoaded = true;
    }

    g_charTextures[0] = LoadTexture("assets/gameplay_scene/char_patricia.png");
    GenTextureMipmaps(&g_charTextures[0]);
    SetTextureFilter(g_charTextures[0], TEXTURE_FILTER_BILINEAR);

    g_charTextures[1] = LoadTexture("assets/gameplay_scene/char_andre.png");
    GenTextureMipmaps(&g_charTextures[1]);
    SetTextureFilter(g_charTextures[1], TEXTURE_FILTER_BILINEAR);

    g_charTextures[2] = LoadTexture("assets/gameplay_scene/char_lucas.png");
    GenTextureMipmaps(&g_charTextures[2]);
    SetTextureFilter(g_charTextures[2], TEXTURE_FILTER_BILINEAR);

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

    // As marcacoes NAO sao zeradas aqui: sao as anotacoes da investigacao
    RayCanvasInit(g_telaFechada.width, g_telaFechada.height);
}
// Retangulo do quadradinho i, em pixels da imagem 640x360
static Rectangle CaixaNaImagem(int i) {
    return (Rectangle){
        g_pranchetaPos.x + CAIXA_X,
        g_pranchetaPos.y + CAIXA_Y0 + i * CAIXA_PASSO,
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
            snprintf(linha, sizeof linha, "%.*s", (int)(sizeof(linha) - 1), teste);
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
            if (TutorialAtivo() && g_tutorialPasso == TUT_PASSO_FECHAR_FICHA) {
                g_menuAberto = false;
                g_tutorialPasso = TUT_PASSO_PERGUNTA;
            }
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
    if (g_dialogLoaded && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < NUM_CAIXAS; i++) {
            if (CheckCollisionPointRec(mouse, CaixaNaImagem(i))) {
                g_marcado[g_funcionario][i] = !g_marcado[g_funcionario][i];
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

// ---------- Tutorial Interativo (Dia 1) ----------

static const Rectangle TUT_BTN_COMECAR = { 18, 224, 180, 20 };

static float PulsoTutorial(void) {
    float t = (float)GetTime() * 1.5f;
    t = t - (int)t;
    float tri = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
    return tri * tri * (3.0f - 2.0f * tri);
}

// Desenha uma moldura pulsante e uma seta animada apontando para o alvo
static void DesenharDestaqueTutorial(Rectangle alvoVirtual, const char *setaTxt, int ladoSeta) {
    float pulso = PulsoTutorial();
    Rectangle rTela = ParaTela(alvoVirtual);
    float escala = rTela.height / alvoVirtual.height;
    Color corBrilho = (Color){ 225, 240, 155, 255 };
    Color corEscura = (Color){ 20, 22, 18, 235 };

    DrawRectangleRec(rTela, Fade(corBrilho, 0.08f + 0.12f * pulso));
    DrawRectangleLinesEx(rTela, 3.5f * escala, corEscura);
    DrawRectangleLinesEx(rTela, (1.5f + 1.5f * pulso) * escala, corBrilho);

    if (!setaTxt || !setaTxt[0]) return;

    float offset = 4.0f * pulso;
    Rectangle badge = { 0, 0, 24, 16 };
    if (ladoSeta == 0) { // esquerda do alvo (aponta ">>")
        badge.x = alvoVirtual.x - badge.width - 4.0f - offset;
        badge.y = alvoVirtual.y + (alvoVirtual.height - badge.height) / 2.0f;
    } else if (ladoSeta == 1) { // direita do alvo (aponta "<<")
        badge.x = alvoVirtual.x + alvoVirtual.width + 4.0f + offset;
        badge.y = alvoVirtual.y + (alvoVirtual.height - badge.height) / 2.0f;
    } else if (ladoSeta == 2) { // abaixo do alvo (aponta "^")
        badge.x = alvoVirtual.x + (alvoVirtual.width - badge.width) / 2.0f;
        badge.y = alvoVirtual.y + alvoVirtual.height + 3.0f + offset;
    }

    Rectangle rBadge = ParaTela(badge);
    DrawRectangleRec(rBadge, corEscura);
    DrawRectangleLinesEx(rBadge, 1.5f * escala, corBrilho);
    DrawTextInArea(setaTxt, badge, 10, corBrilho);
}

// Desenha o card enxuto de instrucao do passo atual
static void DesenharCardTutorial(Rectangle card, const char *titulo, const char *corpo, const char *acao) {
    Color corEscura  = (Color){ 25, 28, 24, 255 };
    Color corFundo   = (Color){ 125, 136, 114, 248 };
    Color corRodape  = (Color){ 155, 168, 136, 255 };
    Color corDestaque = (Color){ 225, 240, 155, 255 };

    Rectangle sombra = { card.x + 3, card.y + 3, card.width, card.height };
    DrawRectangleRec(ParaTela(sombra), Fade(BLACK, 0.45f));

    Rectangle rCard = ParaTela(card);
    float escala = rCard.height / card.height;
    DrawRectangleRec(rCard, corFundo);
    DrawRectangleLinesEx(rCard, 2.0f * escala, corEscura);

    Rectangle rCabecalho = { card.x, card.y, card.width, 18 };
    DrawRectangleRec(ParaTela(rCabecalho), corEscura);
    DrawTextInArea(titulo, rCabecalho, 9, corDestaque);

    char buf[512];
    snprintf(buf, sizeof buf, "%s", corpo);
    float currY = card.y + 23.0f;
    char *p = buf;
    while (p && *p) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        if (*p) {
            DrawTextWrappedLeftAligned(p, (Vector2){ card.x + 8, currY }, card.width - 16, 9, corEscura);
            currY += MedirAlturaTextoQuebrado(p, card.width - 16, 9);
        } else {
            currY += 5.0f;
        }
        if (!nl) break;
        p = nl + 1;
    }

    if (acao && acao[0]) {
        Rectangle rAcao = { card.x + 6, card.y + card.height - 22, card.width - 12, 16 };
        Rectangle rAcaoTela = ParaTela(rAcao);
        DrawRectangleRec(rAcaoTela, corRodape);
        DrawRectangleLinesEx(rAcaoTela, 1.0f * escala, corEscura);
        DrawTextInArea(acao, rAcao, 9, corEscura);
    }
}

static bool UpdateTutorialFinal(void) {
    if (!TutorialAtivo() || g_tutorialPasso != TUT_PASSO_CONCLUSAO) return false;
    Vector2 mouse = RayCanvasGetMousePosition();
    Rectangle rBtn = ParaTela(TUT_BTN_COMECAR);
    if (CheckCollisionPointRec(mouse, rBtn)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_pranchetaAberta = false;
            g_menuAberto = false;
            ResetTokens(); // devolve o token gasto na pergunta de teste
            GameStateCompleteTutorial();
            return true;
        }
    }
    return false;
}

static void DesenharTutorial(void) {
    if (!TutorialAtivo()) return;

    switch (g_tutorialPasso) {
        case TUT_PASSO_SUSPEITO: {
            Rectangle alvo = { 51, 76, 168, 144 };
            DesenharDestaqueTutorial(alvo, "<<", 1);
            Rectangle card = { 282, 74, 222, 122 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 1/6 - A MISSÃO",
                "Há 1 IA infiltrada entre estes 3 funcionários da RASEC.\n\nVocê tem 3 dias para descobrir quem é o impostor.",
                "> Clique em um suspeito à esquerda"
            );
            break;
        }
        case TUT_PASSO_ABRIR_MENU: {
            Rectangle alvo = { 596, 66, 42, 51 };
            DesenharDestaqueTutorial(alvo, ">>", 0);
            Rectangle card = { 10, 12, 248, 96 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 2/6 - FERRAMENTAS",
                "Antes de interrogar, veja quem está na sua frente para notar mentiras ou desvios.",
                "> Clique na seta [<] à direita"
            );
            break;
        }
        case TUT_PASSO_ABRIR_FICHA: {
            Rectangle alvo = { 570, 108, 62, 56 };
            DesenharDestaqueTutorial(alvo, ">>", 0);
            Rectangle card = { 10, 12, 248, 96 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 3/6 - FICHA PESSOAL",
                "A Pasta guarda o perfil confidencial, o cargo e os hábitos reais deste funcionário.",
                "> Clique na Pasta no menu à direita"
            );
            break;
        }
        case TUT_PASSO_FECHAR_FICHA: {
            if (g_fichaAtual >= 0) {
                Rectangle alvoX = NaFicha(FICHA_FECHAR);
                DesenharDestaqueTutorial(alvoX, ">>", 0);
            }
            Rectangle card = { 8, 12, 244, 114 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 4/6 - PERSONALIDADE",
                "Compare a ficha com as respostas! IAs costumam contradizer a ficha ou segui-la de forma robótica.",
                "> Leia a ficha e clique no [X] dela"
            );
            break;
        }
        case TUT_PASSO_PERGUNTA: {
            Rectangle alvoTokens = { DIALOG_BOX_X - 3, 3, 86, 14 };
            Rectangle alvoPerguntas = { DIALOG_BOX_X - 2, 250, DIALOG_BOX_W + 4, 49 };
            DesenharDestaqueTutorial(alvoTokens, "", 0);
            DesenharDestaqueTutorial(alvoPerguntas, ">>", 0);
            Rectangle card = { 10, 12, 248, 102 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 5/6 - TOKENS E PERGUNTAS",
                "Você tem 12 Tokens por dia (veja acima do chat). Cada pergunta gasta 1 Token!",
                "> Clique em uma pergunta abaixo do chat"
            );
            break;
        }
        case TUT_PASSO_ABRIR_PRANCHETA: {
            Rectangle alvo = { 569, 8, 64, 76 };
            DesenharDestaqueTutorial(alvo, ">>", 0);
            Rectangle card = { 10, 12, 248, 102 };
            DesenharCardTutorial(
                card,
                "TUTORIAL 6/6 - ANOTAÇÕES",
                "Veja a resposta no chat! Quando notar alguma falha suspeita, anote no seu Guia da Investigação.",
                "> Clique na Prancheta (topo do menu)"
            );
            break;
        }
        case TUT_PASSO_CONCLUSAO: {
            Rectangle alvoDica = { 526, 2, 36, 36 };
            Rectangle alvoMapa = { 566, 282, 71, 65 };
            DesenharDestaqueTutorial(alvoDica, "^", 2);
            DesenharDestaqueTutorial(alvoMapa, ">>", 0);

            Rectangle card = { 8, 12, 200, 238 };
            DesenharCardTutorial(
                card,
                "PRONTO PARA INVESTIGAR!",
                "Marque na prancheta as falhas de cada suspeito.\n\n- LÂMPADA (topo): 1 puzzle lógico por dia para ter perguntas grátis na conversa!\n\n- MAPA (menu): Use ao fim do dia para avançar até o Veredito.",
                NULL
            );

            Rectangle rBtn = ParaTela(TUT_BTN_COMECAR);
            float escala = rBtn.height / TUT_BTN_COMECAR.height;
            bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), rBtn);
            Color corBtn = hover ? (Color){ 225, 240, 155, 255 } : (Color){ 165, 180, 140, 255 };
            DrawRectangleRec(rBtn, corBtn);
            DrawRectangleLinesEx(rBtn, 2.0f * escala, (Color){ 25, 28, 24, 255 });
            DrawTextInArea("Começar Investigação! (12 Tokens)", TUT_BTN_COMECAR, 9, (Color){ 25, 28, 24, 255 });
            break;
        }
    }
}

static void UpdateGameplay(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    Rectangle seta = ParaTela(g_menuAberto ? SETA_ABERTA : SETA_FECHADA);
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool tut = TutorialAtivo();
    bool permiteSeta = !tut || (g_tutorialPasso == TUT_PASSO_ABRIR_MENU);
    bool hover = permiteSeta && CheckCollisionPointRec(mouse, seta);

    if (g_dialogLoaded) {
        DialogNode *node = GetCurrentNode(g_tree);
        if (node) {
            // Mesma lista que o desenho usa (ver DrawInterrogationTexts)
            DialogChoice active_choices[MAX_CHOICES];
            int active_count = HasTokens() ? GetAvailableChoices(g_tree, active_choices) : 0;

            int startY = 252;
            int exitY = startY;

            bool permitePergunta = !tut || (g_tutorialPasso == TUT_PASSO_PERGUNTA);
            if (HasTokens() && permitePergunta) {
                for (int i = 0; i < active_count; i++) {
                    Rectangle choiceArea = { DIALOG_BOX_X, startY + (i * (int)DIALOG_BTN_STEP), DIALOG_BOX_W, DIALOG_BTN_H };
                    if (CheckCollisionPointRec(mouse, ParaTela(choiceArea))) {
                        hover = true;
                        if (clicou) {
                            // Registra no histórico se o nó anterior era falado pelo NPC e ainda não constava
                            if (node && strcmp(node->speaker, "Você") != 0) {
                                AdicionarMensagemChat(g_suspeitoAtual, node->speaker, node->text, false);
                            }

                            // Registra a fala/pergunta do jogador
                            AdicionarMensagemChat(g_suspeitoAtual, "Você", active_choices[i].text, true);

                            // Transiciona para o nó escolhido
                            MakeChoiceById(g_tree, active_choices[i].next_node);

                            // Registra a resposta do NPC (se houver e não for o jogador)
                            DialogNode *newNode = GetCurrentNode(g_tree);
                            if (newNode && strcmp(newNode->speaker, "Você") != 0) {
                                AdicionarMensagemChat(g_suspeitoAtual, newNode->speaker, newNode->text, false);
                            }

                            if (tut && g_tutorialPasso == TUT_PASSO_PERGUNTA) {
                                g_menuAberto = true;
                                g_tutorialPasso = TUT_PASSO_ABRIR_PRANCHETA;
                            }
                            return;
                        }
                    }
                }
            }
            if (HasTokens()) {
                exitY = startY + (active_count * (int)DIALOG_BTN_STEP);
            }
            
            Rectangle exitArea = { DIALOG_BOX_X, exitY, DIALOG_BOX_W, DIALOG_BTN_H };
            if (!tut && CheckCollisionPointRec(mouse, ParaTela(exitArea))) {
                hover = true;
                if (clicou) {
                    g_dialogLoaded = false;
                    g_tree = NULL;
                    g_fichaAtual = -1;
                    EncerrarEntrevistaLiberada();
                    return;
                }
            }
        }
    } else {
        int startY = 80;
        bool permiteSuspeito = !tut || (g_tutorialPasso == TUT_PASSO_SUSPEITO);
        for (int i = 0; i < 3 && permiteSuspeito; i++) {
            Rectangle choiceArea = { 55, startY + (i * 50), 160, 36 };
            if (CheckCollisionPointRec(mouse, ParaTela(choiceArea))) {
                hover = true;
                if (clicou) {
                    g_tree = &g_trees[i];
                    g_dialogLoaded = true;
                    g_suspeitoAtual = i;
                    g_funcionario = SUSPECT_FICHA_INDEX[i]; // o botao clicado define qual ficha a pasta abre
                    g_fichaAtual = -1;     // fecha a ficha do funcionario anterior, se estiver aberta
                    // Se for a primeira vez e a fala inicial for de um NPC (ex: Patrícia, Lucas), registra no histórico
                    if (g_chatHistories[i].count == 0) {
                        DialogNode *firstNode = GetCurrentNode(g_tree);
                        if (firstNode && strcmp(firstNode->speaker, "Você") != 0) {
                            AdicionarMensagemChat(i, firstNode->speaker, firstNode->text, false);
                        }
                    }
                    if (tut && g_tutorialPasso == TUT_PASSO_SUSPEITO) {
                        g_tutorialPasso = TUT_PASSO_ABRIR_MENU;
                    }
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

    // 1) Seta: abre/fecha o menu
    if (permiteSeta && clicou && CheckCollisionPointRec(mouse, seta)) {
        g_menuAberto = !g_menuAberto;
        if (tut && g_tutorialPasso == TUT_PASSO_ABRIR_MENU && g_menuAberto) {
            g_tutorialPasso = TUT_PASSO_ABRIR_FICHA;
        }
        return;
    }

    // 2) Ferramentas (so funcionam com o menu aberto)
    if (g_menuAberto) {
        for (int i = 0; i < NUM_TOOLS; i++) {
            if (!CheckCollisionPointRec(mouse, ParaTela(FERRAMENTAS[i]))) continue;
            if (tut) {
                bool permitido = (i == 1 && (g_tutorialPasso == TUT_PASSO_ABRIR_FICHA || g_tutorialPasso == TUT_PASSO_FECHAR_FICHA)) ||
                                 (i == 0 && g_tutorialPasso == TUT_PASSO_ABRIR_PRANCHETA);
                if (!permitido) continue;
            }
            hover = true;
            if (clicou) {
                if (i == 0) {
                    OnClickPrancheta();
                    if (tut && g_tutorialPasso == TUT_PASSO_ABRIR_PRANCHETA && g_pranchetaAberta) {
                        g_pranchetaPos = (Vector2){ 216, 8 };
                        g_tutorialPasso = TUT_PASSO_CONCLUSAO;
                    }
                } else if (i == 1) {
                    OnClickPasta();
                    if (tut && g_tutorialPasso == TUT_PASSO_ABRIR_FICHA && g_fichaAtual >= 0) {
                        g_fichaPos = (Vector2){ 262, 20 };
                        g_tutorialPasso = TUT_PASSO_FECHAR_FICHA;
                    } else if (tut && g_tutorialPasso == TUT_PASSO_FECHAR_FICHA && g_fichaAtual < 0) {
                        g_menuAberto = false;
                        g_tutorialPasso = TUT_PASSO_PERGUNTA;
                    }
                } else if (i == 2) OnClickMaleta();
                else if (i == 3) OnClickMapa();
            }
        }
    }

    SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

void UpdateDrawGameplayScene(void) {
    RayCanvasBegin();

    // A ficha e a prancheta ficam por cima de tudo, entao elas tem prioridade no clique
    if (!UpdateTutorialFinal() && !UpdateDica() && !UpdateFicha() && !UpdatePrancheta()) {
        UpdateGameplay();
    }

    // Background atras da tela (a moldura da tela cobre as bordas)
    Rectangle bgOrigem = { 0, 0, (float)g_background.width, (float)g_background.height };
    DrawTexturePro(g_background, bgOrigem, ParaTela(BG_AREA), (Vector2){ 0, 0 }, 0.0f, WHITE);

    // Personagem desenhado na parte inferior centralizada do escritorio durante a conversa
    if (g_dialogLoaded && g_suspeitoAtual >= 0 && g_suspeitoAtual < 3) {
        Texture2D tex = g_charTextures[g_suspeitoAtual];
        if (tex.id > 0) {
            float charW = 180.0f;
            float charH = 180.0f;
            Rectangle charDest = { (BG_AREA.width - charW) / 2.0f, 112.0f, charW, charH };
            Rectangle charOrigem = { 0, 0, (float)tex.width, (float)tex.height };
            DrawTexturePro(tex, charOrigem, ParaTela(charDest), (Vector2){ 0, 0 }, 0.0f, WHITE);
            DrawRectangleLinesEx(ParaTela(charDest), 1.0f, (Color){ 30, 32, 34, 255 });
        }
    }
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
        float px = g_pranchetaPos.x;
        float py = g_pranchetaPos.y;

        TextoNaImagem("GUIA DA INVESTIGACAO", px + CAIXA_X, py + 60, 8, COR_MEDIO);

        if (!g_dialogLoaded) {
            // Ninguem sendo entrevistado: so um aviso
            TextoQuebrado("Nenhum funcionario em entrevista. Escolha alguem para comecar a investigacao.",
                          px + CAIXA_X, py + 74, 190, 10, 12, COR_PRETO);
        } else {
            // Nome do funcionario dono deste checklist
            const char *nome = (g_funcionario < g_numFichas) ? g_fichas[g_funcionario].nome : "";
            TextoQueCabe(nome, px + CAIXA_X, py + 70, 190, 10, COR_PRETO);

            // Quadradinhos (escurecem no hover) e textos ao lado
            for (int i = 0; i < NUM_CAIXAS; i++) {
                Texture2D tex = g_marcado[g_funcionario][i] ? g_caixaComX : g_caixaVazia;
                DrawWithHover(tex, CaixaNaImagem(i));
                TextoQuebrado(GUIA[i], px + TEXTO_X, py + CAIXA_Y0 + i * CAIXA_PASSO + 1,
                              165, 10, 11, COR_PRETO);
            }
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

        // Foto do funcionario no slot da ficha (ajustado exatamente dentro do contorno sem sobrepor)
        int charIdx = -1;
        for (int s = 0; s < 3; s++) {
            if (SUSPECT_FICHA_INDEX[s] == g_fichaAtual) {
                charIdx = s;
                break;
            }
        }
        if (charIdx >= 0 && g_charTextures[charIdx].id > 0) {
            Texture2D tex = g_charTextures[charIdx];
            Rectangle fotoSlot = { 16.0f, 42.0f, 58.0f, 70.0f };
            Rectangle rFotoTela = ParaTela(NaFicha(fotoSlot));
            float aspect = fotoSlot.width / fotoSlot.height;
            float srcW = (float)tex.width;
            float srcH = (float)tex.height;
            float srcX = 0.0f;
            float srcY = 0.0f;
            if (srcW / srcH > aspect) {
                float newW = srcH * aspect;
                srcX = (srcW - newW) / 2.0f;
                srcW = newW;
            } else {
                float newH = srcW / aspect;
                srcY = (srcH - newH) / 2.0f;
                srcH = newH;
            }
            DrawTexturePro(tex, (Rectangle){ srcX, srcY, srcW, srcH }, rFotoTela, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }

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

    // 7) Overlay do tutorial guiado no Dia 1
    DesenharTutorial();

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
    for (int i = 0; i < 3; i++) {
        UnloadTexture(g_charTextures[i]);
    }
    EncerrarEntrevistaLiberada();   // sair pelo mapa tambem encerra a entrevista
    RayCanvasClose();
}