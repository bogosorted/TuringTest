#include "raylib.h"
#include "menu_scene.h"
#include "../utils/raycanvas.h"
#include "../utils/dialog_system.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

#define NUM_BUTTONS 3
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

static const char *g_textosBotoes[NUM_BUTTONS] = { "Jogar", "Opções", "Sair" };
static bool  g_hover[NUM_BUTTONS];
static float g_botaoPosX[NUM_BUTTONS];  // posicao X atual da mola
static float g_botaoVelX[NUM_BUTTONS];  // velocidade X da mola (damped spring)
static float g_jiggleTempo[NUM_BUTTONS];// timer de jiggle quando passa o mouse

// Posicoes em pixels da tela 640x360
#define PAINEL_X        78      // painel encostado na direita (640 - 562)
#define BOTAO_X_FECHADO 125.0f  // botao retraido (parte do botao/texto fica escondida atras do video em x < 203)
#define BOTAO_X         203.0f  // botao totalmente aberto na borda esquerda do painel
#define BOTAO_Y0        150     // y do primeiro botao
#define BOTAO_PASSO     70      // distancia entre os botoes
#define FONTE_BOTAO     38.0f   // tamanho da fonte dos botoes (maior e em destaque)
#define SPRING_STIFF    210.0f  // rigidez da mola (overshoot elastico)
#define SPRING_DAMP     16.5f   // amortecimento da mola (deixa um balancinho gostoso)

static bool g_shouldStartGame = false;
// Funcoes de clique de cada botao
static void OnClickPlay(void) {
    TraceLog(LOG_INFO, "Botao JOGAR clicado!");
    g_shouldStartGame = true;
}

static void OnClickOptions(void) {
    TraceLog(LOG_INFO, "Botao OPCOES clicado!");
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
    return (Rectangle){ g_botaoPosX[i], (float)(BOTAO_Y0 + i * BOTAO_PASSO), (float)g_botao.width, (float)g_botao.height };
}

// Area clicavel/visivel do botao i (a partir de x = BOTAO_X, pois a esquerda fica atras do video)
static Rectangle BotaoHitRect(int i) {
    float x = g_botaoPosX[i];
    float fimX = x + (float)g_botao.width;
    float inicioX = (x < BOTAO_X) ? BOTAO_X : x;
    return (Rectangle){ inicioX, (float)(BOTAO_Y0 + i * BOTAO_PASSO), fimX - inicioX, (float)g_botao.height };
}

// Desenha um sprite no tamanho original, na posicao (x, y) da tela 640x360
static void DesenharTextura(Texture2D tex, float x, float y, Color cor) {
    Rectangle origem  = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle destino = ParaTela((Rectangle){ x, y, (float)tex.width, (float)tex.height });
    DrawTexturePro(tex, origem, destino, (Vector2){ 0, 0 }, 0.0f, cor);
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
        DrawTextEx(g_fonte, txt, (Vector2){ tx + boldOffset, ty + 1.2f * escalaY }, fontSize, spacing, Fade((Color){ 189, 181, 168, 255 }, 0.35f));
    }

    // 2) Texto em NEGRITO (multi-pass horizontal + vertical sutil para encorpar a fonte pixel)
    DrawTextEx(g_fonte, txt, (Vector2){ tx,                   ty },                   fontSize, spacing, corTexto);
    DrawTextEx(g_fonte, txt, (Vector2){ tx + boldOffset,      ty },                   fontSize, spacing, corTexto);
    DrawTextEx(g_fonte, txt, (Vector2){ tx + boldOffset*0.5f, ty + 0.5f * escalaY }, fontSize, spacing, corTexto);
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
    RayCanvasInit(TELA_W, TELA_H);

    // GIF: a raylib carrega todos os quadros de uma vez
    g_animImagem  = LoadImageAnim("assets/menu_scene/menu_oficial/spr_mainmenu_gif.gif", &g_animQuadros);
    g_animTextura = LoadTextureFromImage(g_animImagem);
    SetTextureFilter(g_animTextura, TEXTURE_FILTER_POINT);
    g_animAtual = 0;
    g_animTempo = 0.0f;

    // Carrega o painel da direita e as 2 texturas de Perlin noise originais (mesmo padrao antigo, agora com movimento sutil!)
    g_painel = LoadTexture("assets/menu_scene/menu_oficial/spr_mainmenu_fill.png");
    SetTextureFilter(g_painel, TEXTURE_FILTER_POINT);

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

    g_botao = LoadTexture("assets/menu_scene/menu_oficial/spr_mainmenu_botao.png");
    SetTextureFilter(g_botao, TEXTURE_FILTER_POINT);

    // Cria uma silhueta branca do botao para desenhar o outline externo no hover
    Image imgBotao = LoadImage("assets/menu_scene/menu_oficial/spr_mainmenu_botao.png");
    ImageFormat(&imgBotao, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = (Color *)imgBotao.data;
    int totalPx = imgBotao.width * imgBotao.height;
    for (int p = 0; p < totalPx; p++) {
        if (px[p].a > 0) px[p] = (Color){ 255, 242, 230, px[p].a };
    }
    g_botaoSilhueta = LoadTextureFromImage(imgBotao);
    UnloadImage(imgBotao);
    SetTextureFilter(g_botaoSilhueta, TEXTURE_FILTER_POINT);

    // Logo original (title.png vem em preto; invertemos para branco para poder colorir com a paleta)
    // Adicionamos uma margem transparente em cima e embaixo para a onda nao cortar na borda da textura
    Image imgLogo = LoadImage("assets/menu_scene/title.png");
    ImageColorInvert(&imgLogo);
    ImageResizeCanvas(&imgLogo, imgLogo.width, imgLogo.height + 24, 0, 12, BLANK);
    g_logo = LoadTextureFromImage(imgLogo);
    UnloadImage(imgLogo);
    SetTextureFilter(g_logo, TEXTURE_FILTER_POINT);

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

static void UpdateMenuButtons(void) {
    Vector2 mouse = RayCanvasGetMousePosition();
    bool algumHover = false;
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f; // evita pulo brusco se a janela travar num frame

    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool estavaHover = g_hover[i];
        g_hover[i] = CheckCollisionPointRec(mouse, ParaTela(BotaoHitRect(i)));
        if (g_hover[i]) algumHover = true;

        // Dispara o efeito de jiggle e impulso extra ao entrar com o mouse no botao
        if (g_hover[i] && !estavaHover) {
            g_jiggleTempo[i] = 1.0f;
            g_botaoVelX[i] += 180.0f; // chute inicial para dar punch no slide
            if (g_sndHover.frameCount > 0) {
                SetSoundVolume(g_sndHover, 0.35f);
                SetSoundPitch(g_sndHover, 0.98f + ((float)GetRandomValue(0, 6) / 100.0f));
                PlaySound(g_sndHover);
            }
        } else if (!g_hover[i] && estavaHover) {
            g_jiggleTempo[i] = 0.55f;
        }

        if (g_jiggleTempo[i] > 0.0f) {
            g_jiggleTempo[i] -= dt * 3.2f;
            if (g_jiggleTempo[i] < 0.0f) g_jiggleTempo[i] = 0.0f;
        }

        // Fisica de mola (damped spring): desliza rapido e da um leve overshoot elastico no fim
        float alvoX = g_hover[i] ? BOTAO_X : BOTAO_X_FECHADO;
        float forca = (alvoX - g_botaoPosX[i]) * SPRING_STIFF;
        g_botaoVelX[i] += forca * dt;
        g_botaoVelX[i] *= expf(-SPRING_DAMP * dt);
        g_botaoPosX[i] += g_botaoVelX[i] * dt;

        if (!RayCanvasInputBlocked() && g_hover[i] &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
            if (i == 0)      OnClickPlay();
            else if (i == 1) OnClickOptions();
            else if (i == 2) OnClickExit();
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
        DesenharTextura(g_botao, b.x + 3.0f, b.y + 4.0f, Fade(BLACK, 0.45f));

        // Outline branco quente ao redor do botao quando em hover
        if (g_hover[i]) {
            Color corOutBtn = { 255, 242, 230, 235 };
            DesenharTextura(g_botaoSilhueta, b.x - 1.5f, b.y,        corOutBtn);
            DesenharTextura(g_botaoSilhueta, b.x + 1.5f, b.y,        corOutBtn);
            DesenharTextura(g_botaoSilhueta, b.x,        b.y - 1.5f, corOutBtn);
            DesenharTextura(g_botaoSilhueta, b.x,        b.y + 1.5f, corOutBtn);
        }

        Color tint = g_hover[i] ? (Color){ 215, 208, 200, 255 } : WHITE;
        DesenharTextura(g_botao, b.x, b.y, tint);
        DesenharTextoBotao(g_textosBotoes[i], b, g_hover[i], g_jiggleTempo[i]);
    }
    EndScissorMode();
}

void UpdateDrawMenuScene(void) {
    RayCanvasBegin();

    AtualizarAnimacao();
    UpdateMenuButtons();
    DrawMenu();

    UpdateDevCode();
    UpdateDrawDevButton();

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