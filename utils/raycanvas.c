#include "raycanvas.h"
#include "settings.h"
#include <math.h>
#include <stddef.h>

static int g_refWidth = 640;
static int g_refHeight = 360;
static float g_uiScale = 1.0f;
static float g_fadeAlpha = 0.0f;

void RayCanvasInit(int referenceWidth, int referenceHeight) {
    g_refWidth = referenceWidth;
    g_refHeight = referenceHeight;
}

void RayCanvasClose(void) {
}

// ---------------------------------------------------------------
// PALETAS (pos-processamento)
// Com uma paleta diferente da original, o frame inteiro e desenhado numa
// textura e um shader troca cada tom da rampa original pelo tom equivalente
// da paleta escolhida. A diferenca entre o pixel e a rampa (ex.: o vermelho
// dos alertas) e mantida, entao cores de destaque continuam se destacando.
// Com a paleta original nada disso roda: desenha direto na tela.
// ---------------------------------------------------------------
static const char *PALETA_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec3 orig[5];\n"
    "uniform vec3 pal[5];\n"
    "out vec4 finalColor;\n"
    "float lum(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }\n"
    "void main() {\n"
    "    vec4 src = texture(texture0, fragTexCoord);\n"
    "    float l = lum(src.rgb);\n"
    "    vec3 o = orig[4];\n"
    "    vec3 p = pal[4];\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "        float l0 = lum(orig[i]);\n"
    "        float l1 = lum(orig[i + 1]);\n"
    "        if (l <= l1) {\n"
    "            float t = clamp((l - l0) / max(l1 - l0, 0.0001), 0.0, 1.0);\n"
    "            o = mix(orig[i], orig[i + 1], t);\n"
    "            p = mix(pal[i], pal[i + 1], t);\n"
    "            break;\n"
    "        }\n"
    "    }\n"
    "    finalColor = vec4(clamp(p + (src.rgb - o), 0.0, 1.0), 1.0) * fragColor;\n"
    "}\n";

static Shader          g_paletaShader;
static bool            g_paletaShaderPronto = false;
static int             g_locOrig = -1;
static int             g_locPal  = -1;
static RenderTexture2D g_alvo;                // frame desenhado antes de aplicar a paleta
static bool            g_alvoPronto = false;
static bool            g_usandoAlvo = false;  // este frame esta indo para g_alvo?

static void CoresParaVec3(const Color *cores, float out[PALETTE_NUM_STOPS * 3]) {
    for (int i = 0; i < PALETTE_NUM_STOPS; i++) {
        out[i * 3 + 0] = cores[i].r / 255.0f;
        out[i * 3 + 1] = cores[i].g / 255.0f;
        out[i * 3 + 2] = cores[i].b / 255.0f;
    }
}

void RayCanvasBeginFrame(void) {
    BeginDrawing();

    g_usandoAlvo = (SettingsGetPalette() != 0);
    if (!g_usandoAlvo) return;

    if (!g_paletaShaderPronto) {
        g_paletaShader = LoadShaderFromMemory(NULL, PALETA_FS);
        g_locOrig = GetShaderLocation(g_paletaShader, "orig");
        g_locPal  = GetShaderLocation(g_paletaShader, "pal");
        g_paletaShaderPronto = true;
    }

    // Recria a textura se a janela mudou de tamanho (F11, redimensionar)
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    if (!g_alvoPronto || g_alvo.texture.width != w || g_alvo.texture.height != h) {
        if (g_alvoPronto) UnloadRenderTexture(g_alvo);
        g_alvo = LoadRenderTexture(w, h);
        SetTextureFilter(g_alvo.texture, TEXTURE_FILTER_POINT);
        g_alvoPronto = true;
    }

    BeginTextureMode(g_alvo);
}

void RayCanvasEndFrame(void) {
    if (g_usandoAlvo) {
        EndTextureMode();

        float orig[PALETTE_NUM_STOPS * 3];
        float pal[PALETTE_NUM_STOPS * 3];
        CoresParaVec3(SettingsOriginalStops(), orig);
        CoresParaVec3(SettingsPaletteStops(SettingsGetPalette()), pal);
        SetShaderValueV(g_paletaShader, g_locOrig, orig, SHADER_UNIFORM_VEC3, PALETTE_NUM_STOPS);
        SetShaderValueV(g_paletaShader, g_locPal,  pal,  SHADER_UNIFORM_VEC3, PALETTE_NUM_STOPS);

        BeginShaderMode(g_paletaShader);
        // Render textures ficam de cabeca para baixo no OpenGL: altura negativa desvira
        Rectangle src = { 0, 0, (float)g_alvo.texture.width, -(float)g_alvo.texture.height };
        DrawTextureRec(g_alvo.texture, src, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
    }
    EndDrawing();
}

void RayCanvasShutdown(void) {
    if (g_alvoPronto) UnloadRenderTexture(g_alvo);
    if (g_paletaShaderPronto) UnloadShader(g_paletaShader);
    g_alvoPronto = false;
    g_paletaShaderPronto = false;
}

void RayCanvasBegin(void) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Escala uniforme suave para os elementos de interface
    g_uiScale = fminf(screenW / (float)g_refWidth, screenH / (float)g_refHeight);
    if (g_uiScale < 0.1f) g_uiScale = 0.1f;

    RayCanvasBeginFrame();
    ClearBackground((Color){ 32, 29, 31, 255 });
}

void RayCanvasEnd(void) {
    // Fade das transicoes: retangulo na cor escura da paleta por cima de tudo que a cena desenhou
    if (g_fadeAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade((Color){ 32, 29, 31, 255 }, g_fadeAlpha));
    }
    RayCanvasEndFrame();
}

void RayCanvasSetFade(float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    g_fadeAlpha = alpha;
}

float RayCanvasGetFade(void) {
    return g_fadeAlpha;
}

bool RayCanvasInputBlocked(void) {
    return g_fadeAlpha > 0.0f;
}

void RayCanvasDraw(void) {
}

void RayCanvasDrawTiledBackground(Texture2D texture) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // O background usa exclusivamente escala inteira para proteger o dithering
    float bgScale = floorf(screenH / (float)texture.height);
    if (bgScale < 1.0f) bgScale = 1.0f;

    float tileW = (float)texture.width * bgScale;
    float tileH = (float)texture.height * bgScale;
    (void)tileH;

    // Alinhado pelo centro da tela
    float centerOffset = fmodf((screenW - tileW) * 0.5f, tileW);
    float startX = centerOffset;
    while (startX > 0.0f) {
        startX -= tileW;
    }

    // Sobreposição de 1px entre blocos (tileW + 1.0f) para eliminar a linha preta de subpixel na rasterização
    for (float x = startX; x < screenW; x += tileW) {
        Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
        Rectangle dst = { floorf(x), 0.0f, tileW + 1.0f, screenH };
        DrawTexturePro(texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }
}

void RayCanvasDrawTexture(Texture2D texture, Rectangle dest, Color tint) {
    Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    DrawTexturePro(texture, src, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

Rectangle RayCanvasGetRect(Vector2 anchor, Vector2 pivot, Vector2 offset, Vector2 baseSize) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Quando a cena pede o retangulo da tela virtual inteira (640x360),
    // estica para preencher 100% da janela sem faixas sobrando
    if (baseSize.x == (float)g_refWidth && baseSize.y == (float)g_refHeight &&
        offset.x == 0.0f && offset.y == 0.0f) {
        return (Rectangle){ 0.0f, 0.0f, screenW, screenH };
    }

    float scaledWidth = baseSize.x * g_uiScale;
    float scaledHeight = baseSize.y * g_uiScale;
    float scaledOffsetX = offset.x * g_uiScale;
    float scaledOffsetY = offset.y * g_uiScale;

    float x = (screenW * anchor.x) + scaledOffsetX - (scaledWidth * pivot.x);
    float y = (screenH * anchor.y) + scaledOffsetY - (scaledHeight * pivot.y);

    return (Rectangle){ x, y, scaledWidth, scaledHeight };
}

Vector2 RayCanvasGetMousePosition(void) {
    return GetMousePosition();
}

float RayCanvasGetUIScale(void) {
    return g_uiScale;
}
