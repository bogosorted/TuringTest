#include "raylib.h"
#include "menu_scene.h"
#include "../utils/settings.h"
#include "../utils/options_ui.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include "../utils/game_state.h"
#include "../utils/ranking.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define NUM_BUTTONS 4
#define TELA_W 640
#define TELA_H 360

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
static float g_rankingScroll     = 0.0f;
static bool  g_nomeAberto        = false;
static float g_nomeAnim          = 0.0f;
static char  g_nomeInput[9]      = "";
static bool  g_voltarHoverAnt    = false;  // mouse estava no botao Voltar no frame anterior
static bool  g_iniciarHoverAnt   = false;
static bool  g_esperarSoltarMouse = false; // ignora os botoes do menu ate soltar o clique que abriu/fechou as opcoes

static bool g_shouldStartGame = false;
// Funcoes de clique de cada botao
static void OnClickPlay(void) {
    TraceLog(LOG_INFO, "Botao JOGAR clicado!");
    g_nomeAberto = true;
    g_nomeInput[0] = '\0';
    g_voltarHoverAnt = false;
    g_iniciarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void OnClickRanking(void) {
    TraceLog(LOG_INFO, "Botao RANKING clicado!");
    RankingLoad();
    g_rankingAberto = true;
    g_rankingScroll = 0.0f;
    g_voltarHoverAnt = false;
    g_esperarSoltarMouse = true;
}

static void FecharRanking(void) {
    g_rankingAberto = false;
    g_esperarSoltarMouse = true;
}

static void FecharNome(void) {
    g_nomeAberto = false;
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
static Rectangle TelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)TELA_W, (float)TELA_H }
    );
}

// Converte um retangulo em pixels 640x360 para a posicao real na janela
static Rectangle ParaTela(Rectangle r) {
    Rectangle t = TelaRect();
    float escalaX = t.width  / (float)TELA_W;
    float escalaY = t.height / (float)TELA_H;
    return (Rectangle){ t.x + r.x * escalaX, t.y + r.y * escalaY, r.width * escalaX, r.height * escalaY };
}

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

// ---------------------------------------------------------------
// MODO DESENVOLVEDOR (testes)
// Digitar "treloso" no menu revela o botao "Tokens Infinitos",
// que liga/desliga global_infinite_tokens. Antes disso o botao nem
// e desenhado nem recebe clique.
// ---------------------------------------------------------------
#define DEV_CODE      "treloso"
#define DEV_CODE_LEN  7

static bool g_devUnlocked = false;                 // fica liberado ate fechar o jogo
static char g_devTyped[DEV_CODE_LEN + 1] = { 0 };  // ultimas letras digitadas

static void UpdateDevCode(void) {
    int c;
    while ((c = GetCharPressed()) != 0) {
        // Empurra o buffer uma casa e coloca a nova letra no final
        memmove(g_devTyped, g_devTyped + 1, DEV_CODE_LEN - 1);
        g_devTyped[DEV_CODE_LEN - 1] = (c < 128) ? (char)tolower(c) : '?';

        if (strncmp(g_devTyped, DEV_CODE, DEV_CODE_LEN) == 0) {
            g_devUnlocked = true;
            TraceLog(LOG_INFO, "Modo desenvolvedor liberado");
        }
    }
}

static void UpdateDrawDevButton(void) {
    if (!g_devUnlocked) return;

    float scale = RayCanvasGetUIScale();

    // Meio da direita da tela
    Rectangle area = RayCanvasGetRect(
        (Vector2){ 1.0f, 0.5f },
        (Vector2){ 1.0f, 0.5f },
        (Vector2){ -10.0f, 0.0f },
        (Vector2){ 120.0f, 22.0f }
    );

    bool hover = CheckCollisionPointRec(RayCanvasGetMousePosition(), area);

    // So "Pressed": com Pressed || Released ele ligaria e desligaria no mesmo clique
    if (hover && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        global_infinite_tokens = !global_infinite_tokens;
        TraceLog(LOG_INFO, "Tokens infinitos: %s", global_infinite_tokens ? "LIGADO" : "DESLIGADO");
    }

    DrawRectangleRec(area, hover ? (Color){ 220, 220, 220, 255 } : WHITE);

    int fonte = (int)(10 * scale);
    if (fonte < 1) fonte = 1;
    const char *label = "Tokens Infinitos";
    int larguraLabel = MeasureText(label, fonte);
    DrawText(label,
             (int)(area.x + (area.width - larguraLabel) / 2.0f),
             (int)(area.y + (area.height - fonte) / 2.0f),
             fonte, BLACK);

    // Estado atual, pequeno, logo abaixo do botao
    const char *estado = global_infinite_tokens ? "ligado" : "desligado";
    int fonteEstado = (int)(8 * scale);
    if (fonteEstado < 1) fonteEstado = 1;
    int larguraEstado = MeasureText(estado, fonteEstado);
    DrawText(estado,
             (int)(area.x + (area.width - larguraEstado) / 2.0f),
             (int)(area.y + area.height + 4.0f * scale),
             fonteEstado, WHITE);
}

void InitMenuScene(void) {
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

    for (int i = 0; i < NUM_BUTTONS; i++) {
        g_hover[i] = false;
        g_botaoPosX[i] = BOTAO_X_FECHADO;
        g_botaoVelX[i] = 0.0f;
        g_jiggleTempo[i] = 0.0f;
    }
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
    bool menuAtivo = !g_opcoesAbertas && !g_rankingAberto && !g_nomeAberto;   // com painel aberto os botoes se escondem
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

        // Fisica de mola (damped spring): desliza rapido e da um leve overshoot elastico no fim
        float alvoX = !menuAtivo ? BOTAO_X_ESCONDIDO : (g_hover[i] ? BOTAO_X : BOTAO_X_FECHADO);
        float forca = (alvoX - g_botaoPosX[i]) * SPRING_STIFF;
        g_botaoVelX[i] += forca * dt;
        g_botaoVelX[i] *= expf(-SPRING_DAMP * dt);
        g_botaoPosX[i] += g_botaoVelX[i] * dt;

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
#define OPC_Y_VOLTAR   286.0f

static const OptionsUiLayout LAYOUT_OPCOES = {
    .x = OPC_LABEL_X, .direita = 600.0f, .yPrimeira = 163.0f, .passoY = 28.0f,
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

    if (g_opcoesAbertas && IsKeyPressed(KEY_ESCAPE)) {
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
    bool hoverVoltar = interativo && !OptionsUiArrastando() && CheckCollisionPointRec(MouseVirtual(), hitVoltar);
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
    bool interativo = g_rankingAberto && a > 0.5f && !RayCanvasInputBlocked();

    if (g_rankingAberto && IsKeyPressed(KEY_ESCAPE)) {
        FecharRanking();
        return;
    }

    Color bege = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio = Fade((Color){ 160, 147, 142, 255 }, a);

    TextoOpcoes("RANKING", OPC_LABEL_X, 128.0f, 30.0f, bege);

    int total = RankingGetCount();
    Rectangle listaArea = { OPC_LABEL_X, 146.0f, 380.0f, 128.0f };
    const float passoY = 22.0f;
    float conteudoH = total * passoY;
    float maxScroll = (conteudoH > listaArea.height) ? (conteudoH - listaArea.height) : 0.0f;

    if (interativo && maxScroll > 0.0f) {
        float roda = GetMouseWheelMove();
        if (roda != 0.0f) {
            g_rankingScroll -= roda * passoY;
        }
    }
    if (g_rankingScroll < 0.0f) g_rankingScroll = 0.0f;
    if (g_rankingScroll > maxScroll) g_rankingScroll = maxScroll;

    if (total == 0) {
        TextoOpcoes("Nenhuma partida registrada ainda.", OPC_LABEL_X, 184.0f, 20.0f, medio);
    } else {
        Rectangle rClip = ParaTela(listaArea);
        BeginScissorMode((int)rClip.x, (int)rClip.y, (int)rClip.width, (int)rClip.height);
        for (int i = 0; i < total; i++) {
            const RankingEntry *e = RankingGetEntry(i);
            if (!e) continue;
            float y = listaArea.y + 12.0f + i * passoY - g_rankingScroll;
            char linha[64];
            snprintf(linha, sizeof linha, "#%d  %-8s", i + 1, e->nome);
            TextoOpcoes(linha, OPC_LABEL_X, y, 20.0f, (i == 0) ? claro : bege);

            char pts[32];
            snprintf(pts, sizeof pts, "%d pts  [%s]", e->pontos, e->acertou ? "ACERTO" : "ERRO");
            TextoOpcoes(pts, OPC_LABEL_X + 165.0f, y, 18.0f, e->acertou ? claro : medio);
        }
        EndScissorMode();

        if (maxScroll > 0.0f) {
            Rectangle trilho = { listaArea.x + listaArea.width - 8.0f, listaArea.y + 2.0f, 4.0f, listaArea.height - 4.0f };
            DrawRectangleRec(ParaTela(trilho), Fade((Color){ 18, 16, 18, 255 }, a));
            float fracVis = listaArea.height / conteudoH;
            float thumbH = trilho.height * fracVis;
            if (thumbH < 18.0f) thumbH = 18.0f;
            float thumbY = trilho.y + (trilho.height - thumbH) * (g_rankingScroll / maxScroll);
            DrawRectangleRec(ParaTela((Rectangle){ trilho.x, thumbY, trilho.width, thumbH }), bege);
        }
    }

    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    Rectangle hitVoltar = { BOTAO_X, OPC_Y_VOLTAR, bw - 22.0f, bh };
    bool hoverVoltar = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(MouseVirtual(), hitVoltar);
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_voltarHoverAnt = hoverVoltar;

    DesenharBotaoPainel("Voltar", BOTAO_X, OPC_Y_VOLTAR, a, hoverVoltar);
    if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharRanking();
    }
    if (g_rankingAberto) {
        SetMouseCursor(hoverVoltar ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
}

static void UpdateDrawNome(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    float alvo = g_nomeAberto ? 1.0f : 0.0f;
    g_nomeAnim += (alvo - g_nomeAnim) * fminf(dt * 9.0f, 1.0f);
    if (!g_nomeAberto && g_nomeAnim < 0.01f) { g_nomeAnim = 0.0f; return; }

    float a = g_nomeAnim;
    bool interativo = g_nomeAberto && a > 0.5f && !RayCanvasInputBlocked();

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
        if (IsKeyPressed(KEY_ENTER) && g_nomeInput[0] != '\0') {
            GameStateSetPlayerName(g_nomeInput);
            g_shouldStartGame = true;
            return;
        }
    }

    Color bege  = Fade((Color){ 189, 181, 168, 255 }, a);
    Color claro = Fade((Color){ 255, 242, 230, 255 }, a);
    Color medio = Fade((Color){ 125, 112, 113, 255 }, a);

    TextoOpcoes("IDENTIFICAÇÃO", OPC_LABEL_X, 128.0f, 28.0f, bege);
    TextoOpcoes("Digite seu nome (até 8 letras):", OPC_LABEL_X, 156.0f, 18.0f, bege);

    Rectangle caixa = { OPC_LABEL_X, 174.0f, 250.0f, 34.0f };
    Rectangle rCaixa = ParaTela(caixa);
    DrawRectangleRec(rCaixa, Fade((Color){ 18, 16, 18, 255 }, a));
    DrawRectangleLinesEx(rCaixa, 2.0f * (rCaixa.height / caixa.height), medio);

    bool cursorVis = ((int)(GetTime() * 2.2) % 2) == 0;
    char exib[16];
    if (g_nomeInput[0] == '\0') {
        snprintf(exib, sizeof exib, "%s", cursorVis ? "_" : "");
    } else {
        snprintf(exib, sizeof exib, "%s%s", g_nomeInput, (cursorVis && strlen(g_nomeInput) < 8) ? "_" : "");
    }
    TextoOpcoes(exib, OPC_LABEL_X + 10.0f, 191.0f, 24.0f, claro);

    bool podeIniciar = (g_nomeInput[0] != '\0');
    float bw = (float)g_botao.width * BOTAO_ESCALA;
    float bh = (float)g_botao.height * BOTAO_ESCALA;
    Rectangle hitIniciar = { BOTAO_X, 224.0f, bw - 22.0f, bh };
    Rectangle hitVoltar  = { BOTAO_X, OPC_Y_VOLTAR, bw - 22.0f, bh };
    bool hoverIniciar = interativo && podeIniciar && !g_esperarSoltarMouse && CheckCollisionPointRec(MouseVirtual(), hitIniciar);
    bool hoverVoltar  = interativo && !g_esperarSoltarMouse && CheckCollisionPointRec(MouseVirtual(), hitVoltar);

    if (hoverIniciar && !g_iniciarHoverAnt) TocarSomHover();
    if (hoverVoltar && !g_voltarHoverAnt) TocarSomHover();
    g_iniciarHoverAnt = hoverIniciar;
    g_voltarHoverAnt  = hoverVoltar;

    DesenharBotaoPainel("Iniciar", BOTAO_X, 224.0f, podeIniciar ? a : a * 0.45f, hoverIniciar);
    DesenharBotaoPainel("Voltar", BOTAO_X, OPC_Y_VOLTAR, a, hoverVoltar);

    if (hoverIniciar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        GameStateSetPlayerName(g_nomeInput);
        g_shouldStartGame = true;
    } else if (hoverVoltar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        FecharNome();
    }

    if (g_nomeAberto) {
        SetMouseCursor((hoverIniciar || hoverVoltar) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
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
    UpdateDrawNome();

    if (!g_opcoesAbertas && !g_rankingAberto && !g_nomeAberto) {
        UpdateDevCode();
        UpdateDrawDevButton();
    }

    RayCanvasEnd();
}

void UnloadMenuScene(void) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    UnloadTexture(g_animTextura);
    UnloadImage(g_animImagem);      // libera os quadros do GIF
    UnloadTexture(g_painel);
    UnloadTexture(g_painelNoise1);
    UnloadTexture(g_painelNoise2);
    UnloadTexture(g_botao);
    UnloadTexture(g_botaoSilhueta);
    UnloadTexture(g_logo);
    UnloadShader(g_waveShader);
    UnloadShader(g_painelShader);
    UnloadFont(g_fonte);
    if (g_sndHover.frameCount > 0) {
        UnloadSound(g_sndHover);
        g_sndHover = (Sound){ 0 };
    }
    RayCanvasClose();
}