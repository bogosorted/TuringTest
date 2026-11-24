#include "raylib.h"
#include "menu_scene.h"
#include "settings.h"
#include "options_ui.h"
#include "raycanvas.h"
#include "dialog_system.h"
#include "game_state.h"
#include "ranking.h"
#include "mural_ui.h"
#include "ui_common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define NUM_BUTTONS 4
#define TELA_W UI_VIRTUAL_W
#define TELA_H UI_VIRTUAL_H

// Animacao do escritorio (GIF 640x360, fica embaixo de tudo)
static Image     g_animImagem;          // todos os quadros do GIF na memoria
static Texture2D g_animTextura;         // quadro que esta sendo mostrado
static int       g_animQuadros = 0;
static int       g_animAtual   = 0;
static float     g_animTempo   = 0.0f;
#define ANIM_DURACAO_QUADRO 0.07f       // 70 ms por quadro (mesma velocidade do GIF)

static Texture2D g_painel;              // painel escuro da direita (562x360) com o padrao Perlin original
static Texture2D g_painelNoise1;        // textura de Perlin noise 1 original (GenImagePerlinNoise scale 4.5)
static Texture2D g_painelNoise2;        // textura de Perlin noise 2 original (GenImagePerlinNoise scale 14.0)
static Texture2D g_botao;               // fundo dos botoes (305x58)
static Texture2D g_botaoSilhueta;       // silhueta branca do botao para desenhar outline brilhante
static Texture2D g_logo;                // logo TURING TEST (title.png invertido para branco)
static Shader    g_waveShader;          // shader de wave bem sutil para a logo TURING TEST
static int       g_waveTimeLoc = -1;
static Shader    g_painelShader;        // shader que anima levemente o padrao Perlin noise original
static int       g_painelTimeLoc = -1;
static int       g_painelTex1Loc = -1;
static int       g_painelTex2Loc = -1;
static Font      g_fonte;               // fonte pixel do menu (VT323)
static Sound     g_sndHover;            // som sutil ao passar o mouse nos botoes

static const char *g_textosBotoes[NUM_BUTTONS] = { "Jogar", "Ranking", "Opções", "Sair" };
static bool  g_hover[NUM_BUTTONS];
static float g_botaoPosX[NUM_BUTTONS];  // posicao X atual da mola
static float g_botaoVelX[NUM_BUTTONS];  // velocidade X da mola (damped spring)
static float g_jiggleTempo[NUM_BUTTONS];// timer de jiggle quando passa o mouse

// Posicoes em pixels da tela 640x360
#define PAINEL_X        78      // painel encostado na direita (640 - 562)
#define BOTAO_X_FECHADO 175.0f  // botao retraido (deixa o texto legivel sem esconder atras do video)
#define BOTAO_X         203.0f  // botao totalmente aberto na borda esquerda do painel
#define BOTAO_Y0        124     // y do primeiro botao
#define BOTAO_PASSO     56      // distancia entre os botoes
#define BOTAO_ESCALA    0.80f   // escala dos 4 botoes no menu principal
#define FONTE_BOTAO     30.0f   // tamanho da fonte dos botoes
#define SPRING_STIFF    210.0f  // rigidez da mola (overshoot elastico)
#define SPRING_DAMP     16.5f   // amortecimento da mola (deixa um balancinho gostoso)
#define BOTAO_X_ESCONDIDO (-320.0f) // botao todo atras do video (painel de opcoes aberto)

// Painel de Opcoes, Ranking e Nome (abrem no lugar dos botoes)
static bool  g_opcoesAbertas     = false;
static float g_opcoesAnim        = 0.0f;   // 0 = invisivel, 1 = totalmente aberto
static bool  g_rankingAberto     = false;
static float g_rankingAnim       = 0.0f;
static bool  g_modoAberto        = false;
static float g_modoAnim          = 0.0f;
static int   g_hoverModoAnt      = -1;
static bool  g_dificuldadeAberto = false;
static float g_dificuldadeAnim   = 0.0f;
static int   g_hoverDifAnt       = -1;
static bool  g_ctrlAberto        = false;
static float g_ctrlAnim          = 0.0f;
static int   g_hoverCtrlAnt      = -1;
static bool  g_nomeAberto        = false;
static float g_nomeAnim          = 0.0f;
static char  g_nomeInput[9]      = "";
static int   g_playerAvatarSel   = 0;
static TomVozJogador g_playerVozSel = VOZ_SUAVE;
static bool  g_vozManual         = false;
static Texture2D g_playerMiniTex[GAME_NUM_PLAYERS] = { 0 };
static int   g_hoverAvatarAnt    = -1;
static int   g_hoverVozAnt       = -1;
static Sound g_vozPreviewSnd[3]  = { {0}, {0}, {0} };
static float g_vozPreviewTimer   = 0.0f;
static float g_vozPreviewTick    = 0.0f;
static bool  g_modoPulado        = false;
static bool  g_voltarHoverAnt    = false;  // mouse estava no botao Voltar no frame anterior
static bool  g_iniciarHoverAnt   = false;
static bool  g_esperarSoltarMouse = false; // ignora os botoes do menu ate soltar o clique que abriu/fechou as opcoes

static bool g_shouldStartGame = false;
static void AbrirTelaDificuldade(void);
static void AbrirTelaControle(void);
static void AbrirTelaNome(void);

// Funcoes de clique de cada botao
static void OnClickPlay(void) {
    TraceLog(LOG_INFO, "Botao JOGAR clicado!");
    bool dev = SettingsGetDevMode();
    bool sem2D = dev && SettingsGetDevDisable2D();
    bool sem3D = dev && SettingsGetDevDisable3D();
    if (sem2D && !sem3D) {
        g_modoPulado = true;
        GameStateSetMode(MODO_3D);
        AbrirTelaDificuldade();
        return;
    }
    if (sem3D && !sem2D) {
        g_modoPulado = true;
        GameStateSetMode(MODO_2D);
        AbrirTelaDificuldade();
        return;
    }
    g_modoPulado = false;
    g_modoAberto = true;
    g_hoverModoAnt = -1;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void AbrirTelaDificuldade(void) {
    g_modoAberto = false;
    g_dificuldadeAberto = true;
    g_hoverDifAnt = -1;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void AbrirTelaControle(void) {
    g_modoAberto = false;
    g_dificuldadeAberto = false;
    g_ctrlAberto = true;
    g_hoverCtrlAnt = -1;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void AbrirTelaNome(void) {
    static const TomVozJogador VOZ_SUG[GAME_NUM_PLAYERS] = {
        VOZ_SUAVE, VOZ_GRAVE, VOZ_SUAVE, VOZ_SUAVE, VOZ_GRAVE, VOZ_GRAVE
    };
    g_modoAberto = false;
    g_dificuldadeAberto = false;
    g_ctrlAberto = false;
    g_nomeAberto = true;
    g_nomeInput[0] = '\0';
    g_playerAvatarSel = GetRandomValue(0, GAME_NUM_PLAYERS - 1);
    GameStateSetPlayerAvatar(g_playerAvatarSel);
    g_playerVozSel = VOZ_SUG[g_playerAvatarSel];
    GameStateSetPlayerVoiceTone(g_playerVozSel);
    g_vozManual = false;
    g_vozPreviewTimer = 0.0f;
    g_vozPreviewTick = 0.0f;
    g_hoverAvatarAnt = -1;
    g_hoverVozAnt = -1;
    g_voltarHoverAnt = false;
    g_iniciarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void OnClickRanking(void) {
    TraceLog(LOG_INFO, "Botao RANKING clicado!");
    MuralReload(-1);
    g_rankingAberto = true;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void FecharRanking(void) {
    g_rankingAberto = false;
    g_esperarSoltarMouse = true;
}

static void FecharModo(void) {
    g_modoAberto = false;
    g_esperarSoltarMouse = true;
}

static void FecharDificuldade(void) {
    g_dificuldadeAberto = false;
    g_modoAberto = !g_modoPulado;
    g_hoverModoAnt = -1;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void FecharControle(void) {
    g_ctrlAberto = false;
    g_dificuldadeAberto = true;
    g_hoverDifAnt = -1;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void FecharNome(void) {
    g_nomeAberto = false;
    g_vozPreviewTimer = 0.0f;
    if (GameStateIs3DMode()) {
        g_ctrlAberto = true;
        g_hoverCtrlAnt = -1;
    } else {
        g_dificuldadeAberto = true;
        g_hoverDifAnt = -1;
    }
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void OnClickOptions(void) {
    TraceLog(LOG_INFO, "Botao OPCOES clicado!");
    g_opcoesAbertas = true;
    OptionsUiReset();
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void FecharOpcoes(void) {
    g_opcoesAbertas = false;
    OptionsUiReset();
    g_esperarSoltarMouse = true;
    SettingsSave();
}

static bool g_shouldClose = false;

static void OnClickExit(void) {
    TraceLog(LOG_INFO, "Botao SAIDA clicado! Encerrando o jogo.");
    g_shouldClose = true;
}

bool MenuSceneShouldClose(void) {
    return g_shouldClose;
}

bool MenuSceneShouldStartGame(void) {
    return g_shouldStartGame;
}

// Retangulo onde a tela 640x360 e desenhada (centralizada na janela)
#define TelaRect UIGetTelaRect
#define ParaTela UIParaTela

// Retangulo completo do botao i (para desenho), em pixels 640x360
static Rectangle BotaoRect(int i) {
    return (Rectangle){ g_botaoPosX[i], (float)(BOTAO_Y0 + i * BOTAO_PASSO),
                        (float)g_botao.width * BOTAO_ESCALA, (float)g_botao.height * BOTAO_ESCALA };
}

// Area clicavel/visivel do botao i (a partir de x = BOTAO_X, pois a esquerda fica atras do video)
static Rectangle BotaoHitRect(int i) {
    float w = (float)g_botao.width * BOTAO_ESCALA;
    float h = (float)g_botao.height * BOTAO_ESCALA;
    float x = g_botaoPosX[i];
    float fimX = x + w;
    float inicioX = (x < BOTAO_X) ? BOTAO_X : x;
    return (Rectangle){ inicioX, (float)(BOTAO_Y0 + i * BOTAO_PASSO), fimX - inicioX, h };
}

// Desenha um sprite no tamanho original, na posicao (x, y) da tela 640x360
static void DesenharTextura(Texture2D tex, float x, float y, Color cor) {
    Rectangle origem  = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle destino = ParaTela((Rectangle){ x, y, (float)tex.width, (float)tex.height });
    DrawTexturePro(tex, origem, destino, (Vector2){ 0, 0 }, 0.0f, cor);
}

static void DesenharTexturaRect(Texture2D tex, Rectangle dest, Color cor) {
    Rectangle origem  = { 0, 0, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, origem, ParaTela(dest), (Vector2){ 0, 0 }, 0.0f, cor);
}

// Copia 'txt' trocando as vogais com til pelas vogais sem til (ã -> a, õ -> o).
// Usado nas passadas extras do negrito: deslocar o til na horizontal junta as
// duas "bolinhas" dele numa barra reta. Como a VT323 e monoespacada, as letras
// continuam no mesmo lugar e so a passada principal desenha o til.
static void TextoSemTil(const char *txt, char *out, size_t tam) {
    size_t o = 0;
    for (const unsigned char *p = (const unsigned char *)txt; *p && o + 1 < tam; p++) {
        if (p[0] == 0xC3 && p[1] != 0) {
            unsigned char c = p[1];
            char base = 0;
            if (c == 0xA3) base = 'a';        // ã
            else if (c == 0xB5) base = 'o';   // õ
            else if (c == 0x83) base = 'A';   // Ã
            else if (c == 0x95) base = 'O';   // Õ
            if (base) { out[o++] = base; p++; continue; }
        }
        out[o++] = (char)*p;
    }
    out[o] = '\0';
}

// Escreve o texto do botao centralizado, em NEGRITO (bold) e com outline claro quando em hover
static void DesenharTextoBotao(const char *txt, Rectangle botao, bool hover, float jiggleT) {
    // O botao tem um chanfro diagonal de 58px na ponta direita, entao o centro visual util
    // fica nos primeiros (width - 28) pixels
    float larguraUtil = botao.width - 28.0f;
    Rectangle rTam = ParaTela((Rectangle){ 0, 0, 0, FONTE_BOTAO });
    Rectangle rTelaBotao = ParaTela((Rectangle){ botao.x, botao.y, larguraUtil, botao.height });
    float escalaX = rTelaBotao.width / larguraUtil;
    float escalaY = rTelaBotao.height / botao.height;

    float fontSize = rTam.height;
    (void)jiggleT;
    float spacing = 2.0f * escalaX;
    Vector2 med = MeasureTextEx(g_fonte, txt, fontSize, spacing);

    float tx = rTelaBotao.x + (rTelaBotao.width  - med.x) / 2.0f;
    float ty = rTelaBotao.y + (rTelaBotao.height - med.y) / 2.0f;

    Color corTexto   = {  32,  29,  31, 255 };
    Color corOutline = { 255, 242, 230, 245 }; // branco quente da paleta

    float boldOffset = 1.1f * escalaX;
    if (boldOffset < 1.0f) boldOffset = 1.0f;

    char semTil[64];
    TextoSemTil(txt, semTil, sizeof semTil);

    // 1) Se estiver com o mouse em cima, desenha outline branco/creme ao redor do texto bold
    if (hover) {
        float out = 1.8f * escalaX;
        float outY = 1.8f * escalaY;
        const Vector2 dirs[8] = {
            { -out, 0 }, { out + boldOffset, 0 }, { 0, -outY }, { 0, outY },
            { -out, -outY }, { out + boldOffset, -outY }, { -out, outY }, { out + boldOffset, outY }
        };
        for (int d = 0; d < 8; d++) {
            DrawTextEx(g_fonte, txt, (Vector2){ tx + dirs[d].x, ty + dirs[d].y }, fontSize, spacing, corOutline);
        }
    } else {
        // Sombra suave de relevo quando fechado
        DrawTextEx(g_fonte, semTil, (Vector2){ tx + boldOffset, ty + 1.2f * escalaY }, fontSize, spacing, Fade((Color){ 189, 181, 168, 255 }, 0.35f));
    }

    // 2) Texto em NEGRITO (multi-pass horizontal + vertical sutil para encorpar a fonte pixel)
    DrawTextEx(g_fonte, semTil, (Vector2){ tx + boldOffset,      ty },                   fontSize, spacing, corTexto);
    DrawTextEx(g_fonte, semTil, (Vector2){ tx + boldOffset*0.5f, ty + 0.5f * escalaY }, fontSize, spacing, corTexto);
    DrawTextEx(g_fonte, txt,    (Vector2){ tx,                   ty },                   fontSize, spacing, corTexto);
}

bool InitMenuScene(void) {
    g_shouldClose = false;
    g_shouldStartGame = false;
    g_opcoesAbertas = false;
    g_opcoesAnim = 0.0f;
    g_rankingAberto = false;
    g_rankingAnim = 0.0f;
    g_nomeAberto = false;
    g_nomeAnim = 0.0f;
    g_nomeInput[0] = '\0';
    OptionsUiReset();
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = false;
    RayCanvasInit(TELA_W, TELA_H);

    // GIF: a raylib carrega todos os quadros de uma vez
    g_animImagem  = LoadImageAnim("assets/sprites/menu/spr_mainmenu_gif.gif", &g_animQuadros);
    g_animTextura = LoadTextureFromImage(g_animImagem);
    SetTextureFilter(g_animTextura, TEXTURE_FILTER_POINT);
    SetTextureWrap(g_animTextura, TEXTURE_WRAP_CLAMP);
    g_animAtual = 0;
    g_animTempo = 0.0f;

    // Carrega o painel da direita e as 2 texturas de Perlin noise originais (mesmo padrao antigo, agora com movimento sutil!)
    g_painel = LoadTexture("assets/sprites/menu/spr_mainmenu_fill.png");
    SetTextureFilter(g_painel, TEXTURE_FILTER_POINT);
    SetTextureWrap(g_painel, TEXTURE_WRAP_CLAMP);

    Image noise1 = GenImagePerlinNoise(g_painel.width, g_painel.height, 0, 0, 4.5f);
    Image noise2 = GenImagePerlinNoise(g_painel.width, g_painel.height, 137, 293, 14.0f);
    g_painelNoise1 = LoadTextureFromImage(noise1);
    g_painelNoise2 = LoadTextureFromImage(noise2);
    UnloadImage(noise1);
    UnloadImage(noise2);
    SetTextureFilter(g_painelNoise1, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(g_painelNoise2, TEXTURE_FILTER_BILINEAR);

    const char *painelFs =
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D noiseTex1;\n"
        "uniform sampler2D noiseTex2;\n"
        "uniform float time;\n"
        "out vec4 finalColor;\n"
        "float bayer4x4(vec2 p) {\n"
        "    int x = int(mod(p.x, 4.0));\n"
        "    int y = int(mod(p.y, 4.0));\n"
        "    int idx = y * 4 + x;\n"
        "    const int m[16] = int[16](\n"
        "         0,  8,  2, 10,\n"
        "        12,  4, 14,  6,\n"
        "         3, 11,  1,  9,\n"
        "        15,  7, 13,  5\n"
        "    );\n"
        "    return (float(m[idx]) / 15.0) - 0.5;\n"
        "}\n"
        "void main() {\n"
        "    vec4 mask = texture(texture0, fragTexCoord);\n"
        "    if (mask.a < 0.01) { finalColor = vec4(0.0); return; }\n"
        "    vec2 px = floor(fragTexCoord * vec2(562.0, 360.0));\n"
        "    vec2 uv = px / vec2(562.0, 360.0);\n"
        "    vec2 drift1 = vec2(sin(time * 0.14) * 0.012, cos(time * 0.11) * 0.012);\n"
        "    vec2 drift2 = vec2(cos(time * 0.18) * 0.018, sin(time * 0.15) * 0.018);\n"
        "    float v1 = texture(noiseTex1, clamp(uv + drift1, 0.002, 0.998)).r;\n"
        "    float v2 = texture(noiseTex2, clamp(uv + drift2, 0.002, 0.998)).r;\n"
        "    float val = v1 * 0.72 + v2 * 0.28;\n"
        "    float onda = sin(val * 18.0 + time * 0.32);\n"
        "    float b = bayer4x4(px);\n"
        "    float dOnda = b * 0.28;\n"
        "    float dVal  = b * 0.18;\n"
        "    vec3 cPreto   = vec3( 0.0,  0.0,  0.0) / 255.0;\n"
        "    vec3 cEscuro0 = vec3(18.0, 16.0, 18.0) / 255.0;\n"
        "    vec3 cBase    = vec3(32.0, 29.0, 31.0) / 255.0;\n"
        "    vec3 cMeio    = vec3(44.0, 39.0, 41.0) / 255.0;\n"
        "    vec3 cClaro   = vec3(62.0, 56.0, 57.0) / 255.0;\n"
        "    vec3 col = cBase;\n"
        "    if (val + dVal < 0.38 || onda + dOnda < -0.52) col = cPreto;\n"
        "    else if (val + dVal < 0.48 || onda + dOnda < -0.18) col = cEscuro0;\n"
        "    else if (onda + dOnda > 0.74) col = cClaro;\n"
        "    else if (onda + dOnda > 0.36) col = cMeio;\n"
        "    finalColor = vec4(col, mask.a) * fragColor;\n"
        "}\n";
    g_painelShader = LoadShaderFromMemory(NULL, painelFs);
    g_painelTimeLoc = GetShaderLocation(g_painelShader, "time");
    g_painelTex1Loc = GetShaderLocation(g_painelShader, "noiseTex1");
    g_painelTex2Loc = GetShaderLocation(g_painelShader, "noiseTex2");

    g_botao = LoadTexture("assets/sprites/menu/spr_mainmenu_botao.png");
    SetTextureFilter(g_botao, TEXTURE_FILTER_POINT);
    // CLAMP: sem isso, ao desenhar o botao em posicao fracionaria (mola/outline) a ultima
    // linha le a primeira linha do sprite (wrap "repeat") e aparece um traco que passa da diagonal
    SetTextureWrap(g_botao, TEXTURE_WRAP_CLAMP);

    // Cria uma silhueta branca do botao para desenhar o outline externo no hover
    Image imgBotao = LoadImage("assets/sprites/menu/spr_mainmenu_botao.png");
    ImageFormat(&imgBotao, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = (Color *)imgBotao.data;
    int totalPx = imgBotao.width * imgBotao.height;
    for (int p = 0; p < totalPx; p++) {
        if (px[p].a > 0) px[p] = (Color){ 255, 242, 230, px[p].a };
    }
    g_botaoSilhueta = LoadTextureFromImage(imgBotao);
    UnloadImage(imgBotao);
    SetTextureFilter(g_botaoSilhueta, TEXTURE_FILTER_POINT);
    SetTextureWrap(g_botaoSilhueta, TEXTURE_WRAP_CLAMP);

    // Logo original (title.png vem em preto; invertemos para branco para poder colorir com a paleta)
    // Adicionamos uma margem transparente em cima e embaixo para a onda nao cortar na borda da textura
    Image imgLogo = LoadImage("assets/sprites/menu/title.png");
    ImageColorInvert(&imgLogo);
    ImageResizeCanvas(&imgLogo, imgLogo.width, imgLogo.height + 24, 0, 12, BLANK);
    g_logo = LoadTextureFromImage(imgLogo);
    UnloadImage(imgLogo);
    SetTextureFilter(g_logo, TEXTURE_FILTER_POINT);
    SetTextureWrap(g_logo, TEXTURE_WRAP_CLAMP);

    // Shader de wave bem leve e lento para dar movimento sutil na logo TURING TEST
    const char *waveFs =
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform float time;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec2 uv = fragTexCoord;\n"
        "    uv.y += sin(uv.x * 6.5 + time * 0.85) * 0.014;\n"
        "    uv.x += cos(uv.y * 4.5 + time * 0.65) * 0.0012;\n"
        "    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {\n"
        "        finalColor = vec4(0.0);\n"
        "    } else {\n"
        "        finalColor = texture(texture0, uv) * fragColor;\n"
        "    }\n"
        "}\n";
    g_waveShader = LoadShaderFromMemory(NULL, waveFs);
    g_waveTimeLoc = GetShaderLocation(g_waveShader, "time");

    // Fonte pixel. Por padrao a raylib so carrega as letras sem acento,
    // entao passamos a lista de caracteres 32 a 255 (inclui ç, ã, é...)
    int caracteres[224];
    for (int i = 0; i < 224; i++) caracteres[i] = 32 + i;
    g_fonte = LoadFontEx("assets/fonts/VT323-Regular.ttf", 128, caracteres, 224);
    SetTextureFilter(g_fonte.texture, TEXTURE_FILTER_POINT);

    if (FileExists("assets/audio/sfx_hover.wav")) {
        g_sndHover = LoadSound("assets/audio/sfx_hover.wav");
    }
    if (FileExists("assets/audio/blip_andre.wav"))    g_vozPreviewSnd[VOZ_GRAVE] = LoadSound("assets/audio/blip_andre.wav");
    if (FileExists("assets/audio/blip_lucas.wav"))    g_vozPreviewSnd[VOZ_MEDIA] = LoadSound("assets/audio/blip_lucas.wav");
    if (FileExists("assets/audio/blip_patricia.wav")) g_vozPreviewSnd[VOZ_SUAVE] = LoadSound("assets/audio/blip_patricia.wav");

    for (int i = 0; i < NUM_BUTTONS; i++) {
        g_hover[i] = false;
        g_botaoPosX[i] = BOTAO_X_FECHADO;
        g_botaoVelX[i] = 0.0f;
        g_jiggleTempo[i] = 0.0f;
    }

    for (int i = 0; i < GAME_NUM_PLAYERS; i++) {
        char pPath[96];
        snprintf(pPath, sizeof pPath, "assets/sprites/players/player_%d.png", i + 1);
        if (FileExists(pPath)) {
            g_playerMiniTex[i] = LoadTexture(pPath);
            SetTextureFilter(g_playerMiniTex[i], TEXTURE_FILTER_POINT);
        }
    }

    MuralInit();
    if (g_animTextura.id == 0 || g_painel.id == 0 || g_botao.id == 0 || g_logo.id == 0 || g_fonte.texture.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar recursos essenciais do menu");
        UnloadMenuScene();
        return false;
    }
    return true;
}

// Troca o quadro do GIF a cada 70 ms
static void AtualizarAnimacao(void) {
    if (g_animQuadros <= 1) return;

    bool mudou = false;
    g_animTempo += GetFrameTime();
    while (g_animTempo >= ANIM_DURACAO_QUADRO) {
        g_animTempo -= ANIM_DURACAO_QUADRO;
        g_animAtual = (g_animAtual + 1) % g_animQuadros;
        mudou = true;
    }

    if (mudou) {
        // Os quadros ficam um depois do outro na memoria: pula ate o quadro atual
        int bytesPorQuadro = g_animImagem.width * g_animImagem.height * 4;
        UpdateTexture(g_animTextura, ((unsigned char *)g_animImagem.data) + bytesPorQuadro * g_animAtual);
    }
}

static void TocarSomHover(void) {
    if (g_sndHover.frameCount > 0) {
        SetSoundVolume(g_sndHover, 0.35f * SettingsGetSfxVolume());
        SetSoundPitch(g_sndHover, 0.98f + ((float)GetRandomValue(0, 6) / 100.0f));
        PlaySound(g_sndHover);
    }
}

static void UpdateMenuButtons(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    bool algumHover = false;
    bool menuAtivo = !g_opcoesAbertas && !g_rankingAberto && !g_modoAberto && !g_dificuldadeAberto && !g_ctrlAberto && !g_nomeAberto;   // com painel aberto os botoes se escondem
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f; // evita pulo brusco se a janela travar num frame

    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool estavaHover = g_hover[i];
        g_hover[i] = menuAtivo && CheckCollisionPointRec(mouse, ParaTela(BotaoHitRect(i)));
        if (g_hover[i]) algumHover = true;

        // Dispara o efeito de jiggle e impulso extra ao entrar com o mouse no botao
        if (g_hover[i] && !estavaHover) {
            g_jiggleTempo[i] = 1.0f;
            g_botaoVelX[i] += 180.0f; // chute inicial para dar punch no slide
            TocarSomHover();
        } else if (!g_hover[i] && estavaHover) {
            g_jiggleTempo[i] = 0.55f;
        }

        if (g_jiggleTempo[i] > 0.0f) {
            g_jiggleTempo[i] -= dt * 3.2f;
            if (g_jiggleTempo[i] < 0.0f) g_jiggleTempo[i] = 0.0f;
        }

        // Fisica de mola (damped spring): cresce apenas para a direita, sem nunca sair para fora da foto na esquerda
        float alvoX = !menuAtivo ? BOTAO_X_ESCONDIDO : (g_hover[i] ? BOTAO_X : BOTAO_X_FECHADO);
        float forca = (alvoX - g_botaoPosX[i]) * SPRING_STIFF;
        g_botaoVelX[i] += forca * dt;
        g_botaoVelX[i] *= expf(-SPRING_DAMP * dt);
        g_botaoPosX[i] += g_botaoVelX[i] * dt;
        if (menuAtivo && g_botaoPosX[i] > BOTAO_X) {
            g_botaoPosX[i] = BOTAO_X;
            if (g_botaoVelX[i] > 0.0f) g_botaoVelX[i] = 0.0f;
        }

        if (!RayCanvasInputBlocked() && !g_esperarSoltarMouse && g_hover[i] &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
            if (i == 0)      OnClickPlay();
            else if (i == 1) OnClickRanking();
            else if (i == 2) OnClickOptions();
            else if (i == 3) OnClickExit();
        }
    }

    SetMouseCursor(algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

// Ordem de desenho: o que vem primeiro fica embaixo
static void DrawMenu(void) {
    Color bege   = { 189, 181, 168, 255 };   // tom mais claro da paleta para destacar a logo
    Color claro  = { 160, 147, 142, 255 };   // cor da borda do botao
    Color medio  = { 125, 112, 113, 255 };   // cor de preenchimento do botao

    // 1) GIF embaixo de tudo, ocupando a tela inteira
    DesenharTextura(g_animTextura, 0, 0, WHITE);

    // 2) Painel por cima do GIF, usando o padrao Perlin noise original com movimento sutil
    float tAgora = (float)GetTime();
    if (g_painelTimeLoc >= 0) {
        SetShaderValue(g_painelShader, g_painelTimeLoc, &tAgora, SHADER_UNIFORM_FLOAT);
    }
    BeginShaderMode(g_painelShader);
    if (g_painelTex1Loc >= 0) SetShaderValueTexture(g_painelShader, g_painelTex1Loc, g_painelNoise1);
    if (g_painelTex2Loc >= 0) SetShaderValueTexture(g_painelShader, g_painelTex2Loc, g_painelNoise2);
    DesenharTextura(g_painel, PAINEL_X, 0, WHITE);
    EndShaderMode();

    // 3) Logo TURING TEST maior e alinhada no painel direito, com shader de wave bem leve
    if (g_waveTimeLoc >= 0) {
        SetShaderValue(g_waveShader, g_waveTimeLoc, &tAgora, SHADER_UNIFORM_FLOAT);
    }
    Rectangle logoOrig   = { 0, 0, (float)g_logo.width, (float)g_logo.height };
    Rectangle logoSombra = ParaTela((Rectangle){ BOTAO_X - 8 + 3, 30 + 3, 430, 62 });
    Rectangle logoDest   = ParaTela((Rectangle){ BOTAO_X - 8,     30,     430, 62 });
    BeginShaderMode(g_waveShader);
    DrawTexturePro(g_logo, logoOrig, logoSombra, (Vector2){ 0, 0 }, 0.0f, Fade(BLACK, 0.55f));
    DrawTexturePro(g_logo, logoOrig, logoDest,   (Vector2){ 0, 0 }, 0.0f, bege);
    EndShaderMode();

    // Linha decorativa sutil abaixo da logo, acompanhando a largura dos botoes
    DrawRectangleRec(ParaTela((Rectangle){ BOTAO_X, 108, (float)g_botao.width, 2 }), medio);
    DrawRectangleRec(ParaTela((Rectangle){ BOTAO_X, 107, 48, 4 }), claro);

    // 4) Botoes que deslizam da esquerda (escondidos atras do video em x < BOTAO_X)
    Rectangle clipPainel = ParaTela((Rectangle){ BOTAO_X, 0, TELA_W - BOTAO_X, TELA_H });
    BeginScissorMode((int)clipPainel.x, (int)clipPainel.y, (int)clipPainel.width, (int)clipPainel.height);
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Rectangle b = BotaoRect(i);

        // Sombra projetada embaixo do botao
        DesenharTexturaRect(g_botao, (Rectangle){ b.x + 3.0f, b.y + 4.0f, b.width, b.height }, Fade(BLACK, 0.45f));

        // Outline branco quente ao redor do botao quando em hover
        if (g_hover[i]) {
            Color corOutBtn = { 255, 242, 230, 235 };
            DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x - 1.5f, b.y,        b.width, b.height }, corOutBtn);
            DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x + 1.5f, b.y,        b.width, b.height }, corOutBtn);
            DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x,        b.y - 1.5f, b.width, b.height }, corOutBtn);
            DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x,        b.y + 1.5f, b.width, b.height }, corOutBtn);
        }

        Color tint = g_hover[i] ? (Color){ 215, 208, 200, 255 } : WHITE;
        DesenharTexturaRect(g_botao, b, tint);
        DesenharTextoBotao(g_textosBotoes[i], b, g_hover[i], g_jiggleTempo[i]);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------
// PAINEL DE OPCOES
// Os controles (musica, efeitos, paleta) ficam em utils/options_ui.c,
// os mesmos do menu de pausa da gameplay. Aqui ficam o titulo, o botao
// Voltar e o layout/cores proprios do menu principal.
// ---------------------------------------------------------------
// Grade do painel (pixels 640x360):
//   coluna 1: nomes, 8px depois da borda do video (x = BOTAO_X) para nao encostar nele
//   coluna 2: sliders / seletor de paleta, 10px depois do nome mais largo
//   coluna 3: valores ("80%", "3/6"), alinhados a direita em x = 600
#define OPC_LABEL_X    (BOTAO_X + 8.0f)
#define OPC_Y_TITULO   132.0f
#define OPC_Y_VOLTAR   296.0f

static const OptionsUiLayout LAYOUT_OPCOES = {
    .x = OPC_LABEL_X, .direita = 600.0f, .yPrimeira = 148.0f, .passoY = 22.0f,
    .larguraValor = 52.0f, .gapNome = 10.0f
};

// Posicao do mouse em pixels 640x360 (inverso do ParaTela)
static Vector2 MouseVirtual(void) {
    Rectangle t = TelaRect();
    Vector2 m = RayCanvasGetMousePosition();
    return (Vector2){ (m.x - t.x) * (float)TELA_W / t.width, (m.y - t.y) * (float)TELA_H / t.height };
}

// Texto na fonte do menu. 'yCentro' e o centro vertical
static void TextoOpcoes(const char *txt, float x, float yCentro, float tamanho, Color cor) {
    Rectangle r = ParaTela((Rectangle){ x, yCentro - tamanho / 2.0f, 0, tamanho });
    float esc = r.height / tamanho;
    float spacing = 1.0f * esc;
    DrawTextEx(g_fonte, txt, (Vector2){ r.x + esc, r.y + esc }, r.height, spacing, Fade(BLACK, 0.5f * (cor.a / 255.0f)));
    DrawTextEx(g_fonte, txt, (Vector2){ r.x, r.y }, r.height, spacing, cor);
}

static void UpdateDrawOpcoes(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_opcoesAbertas ? 1.0f : 0.0f;
    g_opcoesAnim += (alvo - g_opcoesAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_opcoesAbertas && g_opcoesAnim < 0.01f) { g_opcoesAnim = 0.0f; return; }

    float a = g_opcoesAnim;
    bool interativo = g_opcoesAbertas && a > 0.5f && !RayCanvasInputBlocked();

    if (g_opcoesAbertas && !OptionsUiModalAberto() && IsKeyPressed(KEY_ESCAPE)) {
        FecharOpcoes();
        return;
    }

    // Titulo
    TextoOpcoes("OPÇÕES", OPC_LABEL_X, OPC_Y_TITULO, 30.0f, Fade((Color){ 189, 181, 168, 255 }, a));

    // Musica, efeitos e paleta (texto claro sobre o painel escuro)
    OptionsUiEstilo est = {
        .fonte = g_fonte, .tamFonte = 22.0f, .espacamento = 1.0f, .sombraTexto = true,
        .nome            = { 189, 181, 168, 255 },
        .valor           = { 189, 181, 168, 255 },
        .valorAtivo      = { 255, 242, 230, 255 },
        .trilhoBorda     = { 125, 112, 113, 255 },
        .trilhoFundo     = {  18,  16,  18, 255 },
        .preenchido      = { 189, 181, 168, 255 },
        .preenchidoAtivo = { 255, 242, 230, 255 },
        .puxadorBorda    = {  32,  29,  31, 255 },
        .setaBorda       = { 125, 112, 113, 255 },
        .setaBordaHover  = { 255, 242, 230, 255 },
        .setaFundo       = {  32,  29,  31, 255 },
        .setaFundoHover  = { 125, 112, 113, 255 },
        .setaTexto       = { 255, 242, 230, 255 },
        .nomePaleta      = { 255, 242, 230, 255 },
        .amostraBorda    = { 125, 112, 113, 255 },
    };
    bool algumHover = OptionsUiUpdateDraw(&LAYOUT_OPCOES, &est, a, interativo, TocarSomHover);

    // Botao Voltar (mesmo sprite dos botoes do menu)
    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    Rectangle bVoltar = { BOTAO_X, OPC_Y_VOLTAR, bw, bh };
    Rectangle hitVoltar = { BOTAO_X, OPC_Y_VOLTAR, bw - 22.0f, bh };
    bool hoverVoltar = interativo && !OptionsUiArrastando() && !OptionsUiModalAberto() && CheckCollisionPointRec(MouseVirtual(), hitVoltar);
    if (hoverVoltar) {
        algumHover = true;
        if (!g_voltarHoverAnt) TocarSomHover();
    }
    g_voltarHoverAnt = hoverVoltar;

    DesenharTexturaRect(g_botao, (Rectangle){ bVoltar.x + 3.0f, bVoltar.y + 4.0f, bw, bh }, Fade(BLACK, 0.45f * a));
    if (hoverVoltar) {
        Color corOut = Fade((Color){ 255, 242, 230, 235 }, a);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ bVoltar.x - 1.5f, bVoltar.y,        bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ bVoltar.x + 1.5f, bVoltar.y,        bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ bVoltar.x,        bVoltar.y - 1.5f, bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ bVoltar.x,        bVoltar.y + 1.5f, bw, bh }, corOut);
    }
    DesenharTexturaRect(g_botao, bVoltar, Fade(hoverVoltar ? (Color){ 215, 208, 200, 255 } : WHITE, a));
    if (a > 0.6f) DesenharTextoBotao("Voltar", bVoltar, hoverVoltar, 0.0f);

    if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharOpcoes();
    }

    if (g_opcoesAbertas) {
        SetMouseCursor(algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

static void DesenharBotaoPainel(const char *texto, float x, float y, float a, bool hover) {
    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    Rectangle b = { x, y, bw, bh };
    DesenharTexturaRect(g_botao, (Rectangle){ b.x + 3.0f, b.y + 4.0f, bw, bh }, Fade(BLACK, 0.45f * a));
    if (hover) {
        Color corOut = Fade((Color){ 255, 242, 230, 235 }, a);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x - 1.5f, b.y,        bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x + 1.5f, b.y,        bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x,        b.y - 1.5f, bw, bh }, corOut);
        DesenharTexturaRect(g_botaoSilhueta, (Rectangle){ b.x,        b.y + 1.5f, bw, bh }, corOut);
    }
    DesenharTexturaRect(g_botao, b, Fade(hover ? (Color){ 215, 208, 200, 255 } : WHITE, a));
    if (a > 0.6f) DesenharTextoBotao(texto, b, hover, 0.0f);
}

static void UpdateDrawRanking(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_rankingAberto ? 1.0f : 0.0f;
    g_rankingAnim += (alvo - g_rankingAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_rankingAberto && g_rankingAnim < 0.01f) { g_rankingAnim = 0.0f; return; }

    float a = g_rankingAnim;
    bool pediuVoltar = MuralUpdateDraw(a, g_sndHover);
    if (pediuVoltar && g_rankingAberto && !g_esperarSoltarMouse) {
        FecharRanking();
    }
}

static void UpdateDrawModo(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_modoAberto ? 1.0f : 0.0f;
    g_modoAnim += (alvo - g_modoAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_modoAberto && g_modoAnim < 0.01f) { g_modoAnim = 0.0f; return; }

    float a = g_modoAnim;
    bool interativo = g_modoAberto && a > 0.5f && !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();

    if (g_modoAberto && IsKeyPressed(KEY_ESCAPE)) {
        FecharModo();
        return;
    }

    Color bege   = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro  = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio  = Fade((Color){ 125, 112, 113, 255 }, a);
    Color fundo  = Fade((Color){ 24, 21, 23, 255 }, a);
    Color fundoH = Fade((Color){ 58, 52, 54, 255 }, a);

    TextoOpcoes("ESCOLHA O MODO", OPC_LABEL_X, 122.0f, 26.0f, bege);
    TextoOpcoes("Como deseja explorar a RASEC?", OPC_LABEL_X, 146.0f, 16.0f, medio);

    static const char *TITULOS[2] = { "MODO 2D", "MODO 3D" };
    static const char *LINHA1[2]  = { "Point & Click clássico", "Exploração 3D livre" };
    static const char *LINHA2[2]  = { "com setas pelos andares", "pelo Térreo da RASEC" };

    int hovModo = -1;
    const float cardW = 186.0f;
    const float cardH = 108.0f;
    const float cardGap = 16.0f;
    const float cardY = 166.0f;

    for (int m = 0; m < 2; m++) {
        bool devBloq = SettingsGetDevMode() && ((m == 0 && SettingsGetDevDisable2D()) || (m == 1 && SettingsGetDevDisable3D()));
        Rectangle rCard = { OPC_LABEL_X + m * (cardW + cardGap), cardY, cardW, cardH };
        bool hov = !devBloq && interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, rCard);
        if (hov) hovModo = m;

        Rectangle rTela = ParaTela(rCard);
        float esc = rTela.height / cardH;
        DrawRectangleRec(ParaTela((Rectangle){ rCard.x + 2.5f, rCard.y + 3.0f, cardW, cardH }), Fade(BLACK, 0.5f * a));
        DrawRectangleRec(rTela, devBloq ? Fade(fundo, 0.55f) : (hov ? fundoH : fundo));
        DrawRectangleLinesEx(rTela, (hov ? 2.2f : 1.4f) * esc, devBloq ? medio : (hov ? claro : medio));

        Vector2 tamTit = MeasureTextEx(g_fonte, TITULOS[m], 26.0f, 1.0f);
        TextoOpcoes(TITULOS[m], rCard.x + (cardW - tamTit.x) * 0.5f, rCard.y + 30.0f, 26.0f, devBloq ? medio : (hov ? claro : bege));

        const char *l1 = devBloq ? "DESATIVADO" : LINHA1[m];
        const char *l2 = devBloq ? "nas opções Dev" : LINHA2[m];
        Vector2 tamL1 = MeasureTextEx(g_fonte, l1, 15.0f, 0.5f);
        TextoOpcoes(l1, rCard.x + (cardW - tamL1.x) * 0.5f, rCard.y + 62.0f, 15.0f, devBloq ? (Color){ 210, 76, 76, 255 } : claro);

        Vector2 tamL2 = MeasureTextEx(g_fonte, l2, 15.0f, 0.5f);
        TextoOpcoes(l2, rCard.x + (cardW - tamL2.x) * 0.5f, rCard.y + 80.0f, 15.0f, medio);

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            GameStateSetMode((m == 0) ? MODO_2D : MODO_3D);
            TocarSomHover();
            AbrirTelaDificuldade();
            return;
        }
    }
    if (hovModo >= 0 && hovModo != g_hoverModoAnt) TocarSomHover();
    g_hoverModoAnt = hovModo;

    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    const float yVoltar = 292.0f;
    Rectangle hitVoltar = { BOTAO_X, yVoltar, bw - 22.0f, bh };
    bool hoverVoltar = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, hitVoltar);
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_voltarHoverAnt = hoverVoltar;

    DesenharBotaoPainel("Voltar", BOTAO_X, yVoltar, a, hoverVoltar);
    if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharModo();
    }

    if (g_modoAberto) {
        bool algumHov = hoverVoltar || (hovModo >= 0);
        SetMouseCursor(algumHov ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

static void UpdateDrawDificuldade(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_dificuldadeAberto ? 1.0f : 0.0f;
    g_dificuldadeAnim += (alvo - g_dificuldadeAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_dificuldadeAberto && g_dificuldadeAnim < 0.01f) { g_dificuldadeAnim = 0.0f; return; }

    float a = g_dificuldadeAnim;
    bool interativo = g_dificuldadeAberto && a > 0.5f && !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();

    if (g_dificuldadeAberto && IsKeyPressed(KEY_ESCAPE)) {
        FecharDificuldade();
        return;
    }

    Color bege   = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro  = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio  = Fade((Color){ 125, 112, 113, 255 }, a);
    Color fundo  = Fade((Color){ 24, 21, 23, 255 }, a);
    Color fundoH = Fade((Color){ 58, 52, 54, 255 }, a);
    Color alerta = Fade((Color){ 210, 76, 76, 255 }, a);

    TextoOpcoes("DIFICULDADE", OPC_LABEL_X, 122.0f, 26.0f, bege);
    TextoOpcoes("Escolha o ritmo da investigação:", OPC_LABEL_X, 146.0f, 16.0f, medio);

    static const char *TITULOS[2] = { "MODO NORMAL", "MODO HARD" };
    static const char *LINHA1[2]  = { "Sem limite de tempo", "2:00 min por dia" };
    static const char *LINHA2[2]  = { "Investigue sem pressa", "Relógio nunca pausa!" };

    int hovDif = -1;
    const float cardW = 186.0f;
    const float cardH = 108.0f;
    const float cardGap = 16.0f;
    const float cardY = 166.0f;

    for (int d = 0; d < 2; d++) {
        Rectangle rCard = { OPC_LABEL_X + d * (cardW + cardGap), cardY, cardW, cardH };
        bool hov = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, rCard);
        if (hov) hovDif = d;

        Rectangle rTela = ParaTela(rCard);
        float esc = rTela.height / cardH;
        DrawRectangleRec(ParaTela((Rectangle){ rCard.x + 2.5f, rCard.y + 3.0f, cardW, cardH }), Fade(BLACK, 0.5f * a));
        DrawRectangleRec(rTela, hov ? fundoH : fundo);
        DrawRectangleLinesEx(rTela, (hov ? 2.2f : 1.4f) * esc, (d == 1 && hov) ? alerta : (hov ? claro : medio));

        Vector2 tamTit = MeasureTextEx(g_fonte, TITULOS[d], 25.0f, 1.0f);
        TextoOpcoes(TITULOS[d], rCard.x + (cardW - tamTit.x) * 0.5f, rCard.y + 28.0f, 25.0f, (d == 1) ? alerta : (hov ? claro : bege));

        Vector2 tamL1 = MeasureTextEx(g_fonte, LINHA1[d], 15.0f, 0.5f);
        TextoOpcoes(LINHA1[d], rCard.x + (cardW - tamL1.x) * 0.5f, rCard.y + 60.0f, 15.0f, claro);

        Vector2 tamL2 = MeasureTextEx(g_fonte, LINHA2[d], 14.5f, 0.5f);
        TextoOpcoes(LINHA2[d], rCard.x + (cardW - tamL2.x) * 0.5f, rCard.y + 79.0f, 14.5f, (d == 1) ? alerta : medio);

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            GameStateSetHardMode(d == 1);
            TocarSomHover();
            if (GameStateIs3DMode()) AbrirTelaControle();
            else                     AbrirTelaNome();
            return;
        }
    }
    if (hovDif >= 0 && hovDif != g_hoverDifAnt) TocarSomHover();
    g_hoverDifAnt = hovDif;

    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    const float yVoltar = 292.0f;
    Rectangle hitVoltar = { BOTAO_X, yVoltar, bw - 22.0f, bh };
    bool hoverVoltar = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, hitVoltar);
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_voltarHoverAnt = hoverVoltar;

    DesenharBotaoPainel("Voltar", BOTAO_X, yVoltar, a, hoverVoltar);
    if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharDificuldade();
    }

    if (g_dificuldadeAberto) {
        bool algumHov = hoverVoltar || (hovDif >= 0);
        SetMouseCursor(algumHov ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

static void UpdateDrawControle(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_ctrlAberto ? 1.0f : 0.0f;
    g_ctrlAnim += (alvo - g_ctrlAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_ctrlAberto && g_ctrlAnim < 0.01f) { g_ctrlAnim = 0.0f; return; }

    float a = g_ctrlAnim;
    bool interativo = g_ctrlAberto && a > 0.5f && !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();

    if (g_ctrlAberto && IsKeyPressed(KEY_ESCAPE)) {
        FecharControle();
        return;
    }

    Color bege   = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro  = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio  = Fade((Color){ 125, 112, 113, 255 }, a);
    Color fundo  = Fade((Color){ 24, 21, 23, 255 }, a);
    Color fundoH = Fade((Color){ 58, 52, 54, 255 }, a);

    TextoOpcoes("CONTROLE NO MODO 3D", OPC_LABEL_X, 122.0f, 24.0f, bege);
    TextoOpcoes("Selecione como deseja se movimentar e interagir:", OPC_LABEL_X, 146.0f, 15.0f, medio);

    static const char *TITULOS[3] = { "TECLADO", "MOUSE", "AMBOS" };
    static const char *LINHA1[3]  = { "WASD / Setas p/ andar", "Clique no chao p/ andar", "Teclado e Mouse" };
    static const char *LINHA2[3]  = { "Shift: correr | E/Enter", "Clique no objeto p/ agir", "ativos juntos" };

    int hovCtrl = -1;
    const float cardW = 122.0f;
    const float cardH = 108.0f;
    const float cardGap = 11.0f;
    const float cardY = 166.0f;

    for (int c = 0; c < 3; c++) {
        Rectangle rCard = { OPC_LABEL_X + c * (cardW + cardGap), cardY, cardW, cardH };
        bool hov = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, rCard);
        if (hov) hovCtrl = c;

        Rectangle rTela = ParaTela(rCard);
        float esc = rTela.height / cardH;
        DrawRectangleRec(ParaTela((Rectangle){ rCard.x + 2.5f, rCard.y + 3.0f, cardW, cardH }), Fade(BLACK, 0.5f * a));
        DrawRectangleRec(rTela, hov ? fundoH : fundo);
        DrawRectangleLinesEx(rTela, (hov ? 2.2f : 1.4f) * esc, hov ? claro : medio);

        Vector2 tamTit = MeasureTextEx(g_fonte, TITULOS[c], 22.0f, 1.0f);
        TextoOpcoes(TITULOS[c], rCard.x + (cardW - tamTit.x) * 0.5f, rCard.y + 24.0f, 22.0f, hov ? claro : bege);

        float tamLinha = 12.0f;
        Vector2 tamL1;
        Vector2 tamL2;
        do {
            tamL1 = MeasureTextEx(g_fonte, LINHA1[c], tamLinha, 1.0f);
            tamL2 = MeasureTextEx(g_fonte, LINHA2[c], tamLinha, 1.0f);
            if ((tamL1.x <= cardW - 16.0f && tamL2.x <= cardW - 16.0f) || tamLinha <= 6.0f) break;
            tamLinha -= 0.5f;
        } while (true);

        TextoOpcoes(LINHA1[c], rCard.x + (cardW - tamL1.x) * 0.5f, rCard.y + 58.0f, tamLinha, claro);
        TextoOpcoes(LINHA2[c], rCard.x + (cardW - tamL2.x) * 0.5f, rCard.y + 76.0f, tamLinha, medio);

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            GameStateSetControle3D((ControleModo3D)c);
            TocarSomHover();
            AbrirTelaNome();
            return;
        }
    }
    if (hovCtrl >= 0 && hovCtrl != g_hoverCtrlAnt) TocarSomHover();
    g_hoverCtrlAnt = hovCtrl;

    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    const float yVoltar = 292.0f;
    Rectangle hitVoltar = { BOTAO_X, yVoltar, bw - 22.0f, bh };
    bool hoverVoltar = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, hitVoltar);
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_voltarHoverAnt = hoverVoltar;

    DesenharBotaoPainel("Voltar", BOTAO_X, yVoltar, a, hoverVoltar);
    if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharControle();
    }

    if (g_ctrlAberto) {
        bool algumHov = hoverVoltar || (hovCtrl >= 0);
        SetMouseCursor(algumHov ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

static void UpdateDrawNome(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_nomeAberto ? 1.0f : 0.0f;
    g_nomeAnim += (alvo - g_nomeAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_nomeAberto && g_nomeAnim < 0.01f) { g_nomeAnim = 0.0f; return; }

    float a = g_nomeAnim;
    bool devMode = SettingsGetDevMode();
    bool interativo = g_nomeAberto && a > 0.5f && !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();

    if (g_nomeAberto && IsKeyPressed(KEY_ESCAPE)) {
        FecharNome();
        return;
    }

    if (interativo) {
        int ch = GetCharPressed();
        while (ch > 0) {
            int len = (int)strlen(g_nomeInput);
            if (len < 8 && ch >= 32 && ch < 128 && isalnum((unsigned char)ch)) {
                g_nomeInput[len] = (char)toupper((unsigned char)ch);
                g_nomeInput[len + 1] = '\0';
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(g_nomeInput);
            if (len > 0) g_nomeInput[len - 1] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && (g_nomeInput[0] != '\0' || devMode)) {
            GameStateSetPlayerName(g_nomeInput);
            GameStateSetPlayerAvatar(g_playerAvatarSel);
            GameStateSetPlayerVoiceTone(g_playerVozSel);
            g_shouldStartGame = true;
            return;
        }
    }

    Color bege  = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio = Fade((Color){ 125, 112, 113, 255 }, a);
    Color escuro = Fade((Color){ 18, 16, 18, 255 }, a);

    TextoOpcoes("IDENTIFICAÇÃO", OPC_LABEL_X, 120.0f, 26.0f, bege);
    TextoOpcoes("Nome (até 8 letras):", OPC_LABEL_X, 144.0f, 16.0f, bege);

    Rectangle caixa = { OPC_LABEL_X, 155.0f, 200.0f, 30.0f };
    Rectangle rCaixa = ParaTela(caixa);
    float esc = rCaixa.height / caixa.height;
    DrawRectangleRec(rCaixa, escuro);
    DrawRectangleLinesEx(rCaixa, 2.0f * esc, medio);

    bool cursorVis = ((int)(GetTime() * 2.2) % 2) == 0;
    char exib[16];
    if (g_nomeInput[0] == '\0') {
        snprintf(exib, sizeof exib, "%s", cursorVis ? "_" : "");
    } else {
        snprintf(exib, sizeof exib, "%s%s", g_nomeInput, (cursorVis && strlen(g_nomeInput) < 8) ? "_" : "");
    }
    TextoOpcoes(exib, OPC_LABEL_X + 8.0f, 170.0f, 22.0f, claro);
    if (devMode && g_nomeInput[0] == '\0') {
        TextoOpcoes("(opcional)", OPC_LABEL_X + 126.0f, 170.0f, 14.0f, medio);
    }

    // Selecao de Tom de Voz (sem pronomes: Grave, Medio ou Suave)
    TextoOpcoes("Tom de Voz:", OPC_LABEL_X, 198.0f, 16.0f, bege);
    const char *lblVoz[3] = { "Grave", "Médio", "Suave" };
    int hovVozAtual = -1;
    for (int v = 0; v < 3; v++) {
        Rectangle rVz = { OPC_LABEL_X + v * 68.0f, 209.0f, 64.0f, 22.0f };
        bool selVz = ((int)g_playerVozSel == v);
        bool hovVz = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, rVz);
        if (hovVz) hovVozAtual = v;
        Rectangle rVzTela = ParaTela(rVz);
        Color fundoVz = selVz ? Fade((Color){ 189, 181, 168, 255 }, a)
                      : hovVz ? Fade((Color){ 90, 83, 83, 255 }, a)
                              : Fade((Color){ 28, 25, 27, 255 }, a);
        Color bordaVz = (selVz || hovVz) ? claro : medio;
        Color txtVz   = selVz ? Fade((Color){ 24, 21, 23, 255 }, a) : claro;
        DrawRectangleRec(rVzTela, fundoVz);
        DrawRectangleLinesEx(rVzTela, (selVz ? 2.0f : 1.4f) * esc, bordaVz);
        Vector2 tamTxtVz = MeasureTextEx(g_fonte, lblVoz[v], 15.0f, 0.5f);
        TextoOpcoes(lblVoz[v], rVz.x + (rVz.width - tamTxtVz.x) * 0.5f, rVz.y + rVz.height * 0.5f, 15.0f, txtVz);

        if (hovVz && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_playerVozSel = (TomVozJogador)v;
            g_vozManual = true;
            g_vozPreviewTimer = 3.0f;
            g_vozPreviewTick = 0.0f;
            TocarSomHover();
        }
    }
    // Amostra de 3 segundos do tom de voz selecionado
    if (g_nomeAberto && g_vozPreviewTimer > 0.0f) {
        g_vozPreviewTimer -= dt;
        g_vozPreviewTick -= dt;
        if (g_vozPreviewTick <= 0.0f) {
            g_vozPreviewTick = 0.085f;
            int vi = (int)g_playerVozSel;
            if (vi >= 0 && vi < 3 && g_vozPreviewSnd[vi].frameCount > 0) {
                float basePitch = (vi == VOZ_SUAVE) ? 1.04f : (vi == VOZ_MEDIA ? 0.96f : 0.84f);
                SetSoundPitch(g_vozPreviewSnd[vi], basePitch + ((float)GetRandomValue(-3, 4) / 100.0f));
                SetSoundVolume(g_vozPreviewSnd[vi], 0.45f * SettingsGetSfxVolume());
                PlaySound(g_vozPreviewSnd[vi]);
            }
        }
    }
    if (hovVozAtual >= 0 && hovVozAtual != g_hoverVozAnt) TocarSomHover();
    g_hoverVozAnt = hovVozAtual;

    // Grade 3x2 a direita com os 6 retratos do Player
    TextoOpcoes("Retrato do Inspetor:", 456.0f, 144.0f, 16.0f, bege);
    const float cardS = 52.0f;
    const float gapS  = 6.0f;
    int hovAvatarAtual = -1;
    // Sugestao padrao de tom de voz por retrato caso nao tenha escolhido manualmente: Players 1,3,4 -> Suave; 2,5,6 -> Grave
    static const TomVozJogador VOZ_SUGERIDA[GAME_NUM_PLAYERS] = {
        VOZ_SUAVE, VOZ_GRAVE, VOZ_SUAVE, VOZ_SUAVE, VOZ_GRAVE, VOZ_GRAVE
    };
    for (int k = 0; k < GAME_NUM_PLAYERS; k++) {
        int col = k % 3;
        int row = k / 3;
        Rectangle rAv = { 456.0f + col * (cardS + gapS), 156.0f + row * (cardS + gapS), cardS, cardS };
        bool selAv = (g_playerAvatarSel == k);
        bool hovAv = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, rAv);
        if (hovAv) hovAvatarAtual = k;

        Rectangle rAvTela = ParaTela(rAv);
        DrawRectangleRec(ParaTela((Rectangle){ rAv.x + 2.0f, rAv.y + 2.0f, rAv.width, rAv.height }), Fade(BLACK, 0.5f * a));
        DrawRectangleRec(rAvTela, selAv ? Fade((Color){ 64, 58, 60, 255 }, a) : Fade((Color){ 24, 21, 23, 255 }, a));
        if (g_playerMiniTex[k].id > 0) {
            Rectangle srcP = { 0, 0, (float)g_playerMiniTex[k].width, (float)g_playerMiniTex[k].height };
            Rectangle dstP = ParaTela((Rectangle){ rAv.x + 2.0f, rAv.y + 2.0f, rAv.width - 4.0f, rAv.height - 4.0f });
            DrawTexturePro(g_playerMiniTex[k], srcP, dstP, (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, selAv ? a : a * 0.78f));
        }
        DrawRectangleLinesEx(rAvTela, (selAv ? 2.4f : (hovAv ? 1.8f : 1.2f)) * esc,
                             selAv ? claro : (hovAv ? bege : medio));

        if (hovAv && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_playerAvatarSel = k;
            if (!g_vozManual) {
                g_playerVozSel = VOZ_SUGERIDA[k];
            }
            TocarSomHover();
        }
    }
    if (hovAvatarAtual >= 0 && hovAvatarAtual != g_hoverAvatarAnt) TocarSomHover();
    g_hoverAvatarAnt = hovAvatarAtual;

    const char *descVoz = (g_playerVozSel == VOZ_SUAVE) ? "Voz: Suave"
                        : (g_playerVozSel == VOZ_MEDIA) ? "Voz: Média"
                                                        : "Voz: Grave";
    TextoOpcoes(descVoz, 456.0f, 282.0f, 15.0f, bege);

    bool podeIniciar = (g_nomeInput[0] != '\0') || devMode;
    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    const float yIniciar = 240.0f;
    const float yVoltar  = 292.0f;
    Rectangle hitIniciar = { BOTAO_X, yIniciar, bw - 22.0f, bh };
    Rectangle hitVoltar  = { BOTAO_X, yVoltar,  bw - 22.0f, bh };
    bool hoverIniciar = interativo && podeIniciar && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, hitIniciar);
    bool hoverVoltar  = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(mv, hitVoltar);

    if (hoverIniciar && !g_iniciarHoverAnt) TocarSomHover();
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_iniciarHoverAnt = hoverIniciar;
    g_voltarHoverAnt  = hoverVoltar;

    DesenharBotaoPainel("Iniciar", BOTAO_X, yIniciar, podeIniciar ? a : a * 0.45f, hoverIniciar);
    DesenharBotaoPainel("Voltar", BOTAO_X, yVoltar, a, hoverVoltar);

    if (hoverIniciar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        GameStateSetPlayerName(g_nomeInput);
        GameStateSetPlayerAvatar(g_playerAvatarSel);
        GameStateSetPlayerVoiceTone(g_playerVozSel);
        g_shouldStartGame = true;
    } else if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharNome();
    }

    if (g_nomeAberto) {
        bool algumHov = hoverIniciar || hoverVoltar || (hovVozAtual >= 0) || (hovAvatarAtual >= 0);
        SetMouseCursor(algumHov ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

void UpdateDrawMenuScene(void) {
    RayCanvasBegin();

    // Libera os botoes do menu so depois de soltar o clique que abriu/fechou as opcoes
    if (g_esperarSoltarMouse && !IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        g_esperarSoltarMouse = false;
    }

    AtualizarAnimacao();
    UpdateMenuButtons();
    DrawMenu();
    UpdateDrawOpcoes();
    UpdateDrawRanking();
    UpdateDrawModo();
    UpdateDrawDificuldade();
    UpdateDrawControle();
    UpdateDrawNome();

    RayCanvasEnd();
}

void UnloadMenuScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UISafeUnloadTexture(&g_animTextura);
    if (g_animImagem.data) UnloadImage(g_animImagem);
    g_animImagem = (Image){ 0 };
    UISafeUnloadTexture(&g_painel);
    UISafeUnloadTexture(&g_painelNoise1);
    UISafeUnloadTexture(&g_painelNoise2);
    UISafeUnloadTexture(&g_botao);
    UISafeUnloadTexture(&g_botaoSilhueta);
    UISafeUnloadTexture(&g_logo);
    UISafeUnloadShader(&g_waveShader);
    UISafeUnloadShader(&g_painelShader);
    if (g_fonte.texture.id > 0) UnloadFont(g_fonte);
    g_fonte = (Font){ 0 };
    UISafeUnloadSound(&g_sndHover);
    g_animQuadros = 0;
    g_animAtual = 0;
    g_animTempo = 0.0f;
    for (int v = 0; v < 3; v++) {
        if (g_vozPreviewSnd[v].frameCount > 0) {
            UnloadSound(g_vozPreviewSnd[v]);
            g_vozPreviewSnd[v] = (Sound){ 0 };
        }
    }
    for (int i = 0; i < GAME_NUM_PLAYERS; i++) {
        if (g_playerMiniTex[i].id > 0) {
            UnloadTexture(g_playerMiniTex[i]);
            g_playerMiniTex[i] = (Texture2D){ 0 };
        }
    }
    MuralUnload();
    RayCanvasClose();
}