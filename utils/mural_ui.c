#include "mural_ui.h"
#include "webcam.h"
#include "ranking.h"
#include "raycanvas.h"
#include "settings.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#define CRIAR_PASTA(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define CRIAR_PASTA(p) mkdir(p, 0755)
#endif

#define VIRTUAL_W 640
#define VIRTUAL_H 360
#define PASTA_FOTOS "ranking_fotos"

static Texture2D g_logoTuring       = { 0 };
static Texture2D g_fotos[RANKING_MAX_ENTRIES] = { 0 };
static float     g_cardAnim[RANKING_MAX_ENTRIES] = { 0 };
static int       g_cardHoverAnt     = -1;
static bool      g_btnVoltarHovAnt  = false;
static int       g_highlightIdx     = -1;
static bool      g_iniciado         = false;

// Estado dos controles Dev no Mural (caixa de selecao + botao Apagar ranking)
static int       g_devAlvoApagar     = -1; // -1 = Todos, 0..9 = indice especifico
static bool      g_devDropdownAberto = false;
static bool      g_devComboHovAnt    = false;
static bool      g_devApagarHovAnt   = false;
static int       g_devDropItemHovAnt = -2;

// Estado da cabine de foto
static bool          g_camTentouAbrir = false;
static bool          g_camDisponivel  = false;
static bool          g_fotoCongelada  = false;
static bool          g_temQuadroCam   = false;
static float         g_flashTimer     = 0.0f;
static int           g_btnFotoHovAnt  = -1;
static unsigned char g_rgbBruto[MURAL_FOTO_W * MURAL_FOTO_H * 3];
static Color         g_pixelsDither[MURAL_FOTO_W * MURAL_FOTO_H];
static Texture2D     g_texPreview     = { 0 };

static Rectangle TelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)VIRTUAL_W, (float)VIRTUAL_H }
    );
}

static Rectangle ParaTela(Rectangle r) {
    Rectangle t = TelaRect();
    float ex = t.width  / (float)VIRTUAL_W;
    float ey = t.height / (float)VIRTUAL_H;
    return (Rectangle){ t.x + r.x * ex, t.y + r.y * ey, r.width * ex, r.height * ey };
}

static Vector2 MouseVirtual(void) {
    Rectangle t = TelaRect();
    Vector2 m = RayCanvasGetMousePosition();
    return (Vector2){
        (m.x - t.x) * (float)VIRTUAL_W / t.width,
        (m.y - t.y) * (float)VIRTUAL_H / t.height
    };
}

static void TocarHover(Sound snd) {
    if (snd.frameCount > 0) {
        SetSoundVolume(snd, 0.30f * SettingsGetSfxVolume());
        SetSoundPitch(snd, 0.98f + ((float)GetRandomValue(0, 5) / 100.0f));
        PlaySound(snd);
    }
}

static void DesenharTextoCentrado(const char *txt, Rectangle areaVirt, float tamVirt, Color cor) {
    Rectangle r = ParaTela(areaVirt);
    float esc = r.height / (areaVirt.height > 0.0f ? areaVirt.height : 1.0f);
    int fz = (int)roundf(tamVirt * esc);
    if (fz < 1) fz = 1;
    int tw = MeasureText(txt, fz);
    DrawText(txt, (int)(r.x + (r.width - tw) * 0.5f), (int)(r.y + (r.height - fz) * 0.5f), fz, cor);
}

// -----------------------------------------------------------------------------
// Dither LEVE de alta nitidez (12 degraus suaves alinhados a rampa do jogo + Bayer 4x4 sutil)
// Mantem rostos, oculos, sorrisos e olhos 100% reconheciveis mesmo em luz interna de notebook!
// -----------------------------------------------------------------------------
#define NUM_TONS_RETRATO 12
static const Color RAMPA_RETRATO[NUM_TONS_RETRATO] = {
    {   8,   7,   8, 255 },
    {  26,  23,  25, 255 },
    {  46,  42,  44, 255 },
    {  68,  62,  63, 255 },
    {  90,  82,  83, 255 },
    { 112, 101, 102, 255 },
    { 135, 123, 122, 255 },
    { 158, 146, 142, 255 },
    { 180, 170, 160, 255 },
    { 203, 194, 182, 255 },
    { 228, 218, 206, 255 },
    { 252, 240, 228, 255 }
};

static const int BAYER4X4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

static void AplicarDitherLeveRetrato(const unsigned char *rgbIn, Color *outPixels, int w, int h) {
    int total = w * h;
    float lum[MURAL_FOTO_W * MURAL_FOTO_H];
    int hist[256] = { 0 };

    for (int i = 0; i < total; i++) {
        const unsigned char *p = rgbIn + i * 3;
        float l = 0.299f * (float)p[0] + 0.587f * (float)p[1] + 0.114f * (float)p[2];
        lum[i] = l;
        int bin = (int)(l + 0.5f);
        if (bin < 0) bin = 0;
        if (bin > 255) bin = 255;
        hist[bin]++;
    }

    // Auto-exposicao suave (percentis 4% e 97%) para compensar salas escuras ou luz forte
    int alvoLo = (total * 4) / 100;
    int alvoHi = (total * 97) / 100;
    int acc = 0, pLo = 16, pHi = 235;
    for (int b = 0; b < 256; b++) {
        acc += hist[b];
        if (acc >= alvoLo) { pLo = b; break; }
    }
    acc = 0;
    for (int b = 0; b < 256; b++) {
        acc += hist[b];
        if (acc >= alvoHi) { pHi = b; break; }
    }
    if (pLo > 65) pLo = 65;
    if (pHi < pLo + 55) pHi = pLo + 55;
    if (pHi > 252) pHi = 252;

    float invSpan = 1.0f / (float)(pHi - pLo);

    for (int y = 0; y < h; y++) {
        int ym = (y > 0) ? (y - 1) : 0;
        int yp = (y + 1 < h) ? (y + 1) : (h - 1);
        for (int x = 0; x < w; x++) {
            int xm = (x > 0) ? (x - 1) : 0;
            int xp = (x + 1 < w) ? (x + 1) : (w - 1);

            float c  = lum[y * w + x];
            float viz = 0.25f * (lum[ym * w + x] + lum[yp * w + x] + lum[y * w + xm] + lum[y * w + xp]);
            // Nitidez suave (unsharp mask leve) para destacar olhos, oculos e tracos do rosto
            float nitido = c + (c - viz) * 0.42f;

            float v = (nitido - (float)pLo) * invSpan;
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            // Leve curva gamma (0.86) para abrir meios-tons do rosto sem estourar luzes
            v = powf(v, 0.86f);

            float bayer = ((float)BAYER4X4[y & 3][x & 3] + 0.5f) / 16.0f - 0.5f;
            // Dither leve (forca 0.52 entre vizinhos de uma rampa de 12 tons = textura retro suave e rosto nitido)
            float pos = v * (float)(NUM_TONS_RETRATO - 1) + bayer * 0.52f;
            int idx = (int)floorf(pos + 0.5f);
            if (idx < 0) idx = 0;
            if (idx >= NUM_TONS_RETRATO) idx = NUM_TONS_RETRATO - 1;

            outPixels[y * w + x] = RAMPA_RETRATO[idx];
        }
    }
}

// -----------------------------------------------------------------------------
// Desenho da Moldura Quebrada (quando o jogador escolhe NAO tirar foto ou nao ha camera)
// -----------------------------------------------------------------------------
static void DesenharRetratoQuebrado(Rectangle rFotoVirt, float alpha) {
    Rectangle r = ParaTela(rFotoVirt);
    float esc = r.height / rFotoVirt.height;

    Color cFundo   = Fade((Color){  22,  19,  21, 255 }, alpha);
    Color cSil     = Fade((Color){  46,  41,  43, 255 }, alpha);
    Color cTrinca1 = Fade((Color){ 189, 181, 168, 215 }, alpha);
    Color cTrinca2 = Fade((Color){  90,  83,  83, 190 }, alpha);
    Color cEscuro  = Fade((Color){   8,   7,   8, 240 }, alpha);

    DrawRectangleRec(r, cFundo);

    // Silhueta anonima apagada ao fundo
    float cx = r.x + r.width * 0.5f;
    float cy = r.y + r.height * 0.44f;
    DrawCircle((int)cx, (int)(cy - 8.0f * esc), 13.0f * esc, cSil);
    DrawRectangleRec((Rectangle){ cx - 22.0f * esc, cy + 7.0f * esc, 44.0f * esc, 26.0f * esc }, cSil);

    // Canto superior direito "lascado" / quebrado no vidro
    Vector2 lasca1 = { r.x + r.width,              r.y };
    Vector2 lasca2 = { r.x + r.width - 20.0f * esc, r.y };
    Vector2 lasca3 = { r.x + r.width,              r.y + 22.0f * esc };
    DrawTriangle(lasca1, lasca2, lasca3, cEscuro);

    // Rachaduras diagonais de vidro quebrado irradiando de um ponto de impacto
    Vector2 imp = { r.x + r.width * 0.62f, r.y + r.height * 0.36f };
    Vector2 pontas[6] = {
        { r.x + r.width * 0.10f, r.y + r.height * 0.08f },
        { r.x + r.width * 0.88f, r.y + r.height * 0.06f },
        { r.x + r.width * 0.96f, r.y + r.height * 0.58f },
        { r.x + r.width * 0.74f, r.y + r.height * 0.94f },
        { r.x + r.width * 0.16f, r.y + r.height * 0.86f },
        { r.x + r.width * 0.05f, r.y + r.height * 0.44f }
    };
    for (int i = 0; i < 6; i++) {
        Vector2 meio = {
            imp.x + (pontas[i].x - imp.x) * 0.52f + ((i % 2 == 0) ? -4.0f : 4.5f) * esc,
            imp.y + (pontas[i].y - imp.y) * 0.52f + ((i % 2 == 0) ?  3.5f : -3.0f) * esc
        };
        DrawLineEx(imp, meio, 1.5f * esc, cTrinca1);
        DrawLineEx(meio, pontas[i], 1.1f * esc, cTrinca2);
    }
    // Linhas de teia secundarias perto do impacto
    DrawLineEx((Vector2){ imp.x - 9.0f * esc, imp.y - 4.0f * esc },
               (Vector2){ imp.x + 5.0f * esc, imp.y - 11.0f * esc }, 1.0f * esc, cTrinca2);
    DrawLineEx((Vector2){ imp.x + 8.0f * esc, imp.y + 6.0f * esc },
               (Vector2){ imp.x - 6.0f * esc, imp.y + 12.0f * esc }, 1.0f * esc, cTrinca2);

    // Carimbo discreto "SEM FOTO" na base do vidro trincado
    Rectangle tarja = { rFotoVirt.x + 6.0f, rFotoVirt.y + rFotoVirt.height - 16.0f, rFotoVirt.width - 12.0f, 12.0f };
    Rectangle rTarja = ParaTela(tarja);
    DrawRectangleRec(rTarja, Fade((Color){ 18, 16, 18, 232 }, alpha));
    DrawRectangleLinesEx(rTarja, 1.0f * esc, Fade((Color){ 125, 112, 113, 220 }, alpha));
    DesenharTextoCentrado("SEM RETRATO", tarja, 7.0f, Fade((Color){ 189, 181, 168, 235 }, alpha));
}

// -----------------------------------------------------------------------------
// Gerenciamento do Mural de 10 Retratos
// -----------------------------------------------------------------------------
static void DescarregarFotosMural(void) {
    for (int i = 0; i < RANKING_MAX_ENTRIES; i++) {
        if (g_fotos[i].id > 0) {
            UnloadTexture(g_fotos[i]);
            g_fotos[i] = (Texture2D){ 0 };
        }
    }
}

void MuralInit(void) {
    if (!g_iniciado) {
        if (FileExists("assets/sprites/menu/title.png")) {
            Image imgLogo = LoadImage("assets/sprites/menu/title.png");
            ImageColorInvert(&imgLogo);
            g_logoTuring = LoadTextureFromImage(imgLogo);
            UnloadImage(imgLogo);
            SetTextureFilter(g_logoTuring, TEXTURE_FILTER_POINT);
        }
        g_iniciado = true;
    }
    MuralReload(-1);
}

void MuralReload(int highlightRankIndex) {
    g_highlightIdx = highlightRankIndex;
    g_cardHoverAnt = -1;
    g_btnVoltarHovAnt = false;
    g_devDropdownAberto = false;
    g_devComboHovAnt = false;
    g_devApagarHovAnt = false;
    g_devDropItemHovAnt = -2;
    memset(g_cardAnim, 0, sizeof g_cardAnim);

    DescarregarFotosMural();
    RankingLoad();
    int total = RankingGetCount();
    if (g_devAlvoApagar >= total) g_devAlvoApagar = -1;
    for (int i = 0; i < total && i < RANKING_MAX_ENTRIES; i++) {
        const RankingEntry *e = RankingGetEntry(i);
        if (e && e->foto[0] != '\0' && FileExists(e->foto)) {
            g_fotos[i] = LoadTexture(e->foto);
            SetTextureFilter(g_fotos[i], TEXTURE_FILTER_POINT);
        }
    }
}

void MuralUnload(void) {
    MuralFotoClose();
    if (g_texPreview.id > 0) {
        UnloadTexture(g_texPreview);
        g_texPreview = (Texture2D){ 0 };
    }
    DescarregarFotosMural();
    if (g_logoTuring.id > 0) {
        UnloadTexture(g_logoTuring);
        g_logoTuring = (Texture2D){ 0 };
    }
    g_iniciado = false;
}

bool MuralUpdateDraw(float alpha, Sound sndHover) {
    if (!g_iniciado) MuralInit();

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    bool interativo = (alpha > 0.65f) && !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();
    bool algumHover = false;

    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10.0f, 10.0f });
    float esc = ref.height / 10.0f;

    // 1) Parede de galeria corporativa RASEC ocupando a tela inteira (640x360)
    Rectangle rFundo = ParaTela((Rectangle){ 0, 0, VIRTUAL_W, VIRTUAL_H });
    DrawRectangleRec(rFundo, Fade((Color){ 24, 21, 23, 255 }, alpha));

    // Linhas sutis de lambris/paineis de parede ao fundo
    for (int x = 32; x < VIRTUAL_W; x += 64) {
        Rectangle rLin = ParaTela((Rectangle){ (float)x, 34.0f, 1.0f, VIRTUAL_H - 34.0f });
        DrawRectangleRec(rLin, Fade((Color){ 36, 32, 34, 180 }, alpha));
    }
    DrawRectangleRec(ParaTela((Rectangle){ 0, 34.0f, VIRTUAL_W, 2.0f }), Fade((Color){ 125, 112, 113, 220 }, alpha));

    // 2) Barra superior com o titulo TURING TEST + subtitulo do Mural + botao de voltar ao Menu Principal
    DrawRectangleRec(ParaTela((Rectangle){ 0, 0, VIRTUAL_W, 34.0f }), Fade((Color){ 16, 14, 16, 248 }, alpha));
    if (g_logoTuring.id > 0) {
        Rectangle srcL = { 0, 0, (float)g_logoTuring.width, (float)g_logoTuring.height };
        Rectangle dstL = ParaTela((Rectangle){ 10.0f, 6.0f, 162.0f, 21.0f });
        DrawTexturePro(g_logoTuring, srcL, dstL, (Vector2){ 0, 0 }, 0.0f, Fade((Color){ 255, 242, 230, 255 }, alpha));
    } else {
        DesenharTextoCentrado("TURING TEST", (Rectangle){ 10.0f, 5.0f, 150.0f, 24.0f }, 14.0f, Fade((Color){ 255, 242, 230, 255 }, alpha));
    }

    bool devMode = SettingsGetDevMode();
    if (!devMode) {
        g_devDropdownAberto = false;
        g_devAlvoApagar = -1;
    }

    int count = RankingGetCount();
    if (g_devAlvoApagar >= count) g_devAlvoApagar = -1;

    if (devMode) {
        DesenharTextoCentrado("MURAL DE DESTAQUE",
                              (Rectangle){ 174.0f, 4.0f, 126.0f, 15.0f }, 9.0f,
                              Fade((Color){ 255, 242, 230, 255 }, alpha));
        DesenharTextoCentrado("TOP 10 - MODO DEV",
                              (Rectangle){ 174.0f, 18.0f, 126.0f, 12.0f }, 7.5f,
                              Fade((Color){ 160, 147, 142, 255 }, alpha));
    } else {
        DesenharTextoCentrado("MURAL DE FUNCIONÁRIOS DE DESTAQUE",
                              (Rectangle){ 176.0f, 4.0f, 316.0f, 15.0f }, 10.0f,
                              Fade((Color){ 255, 242, 230, 255 }, alpha));
        DesenharTextoCentrado("RASEC - TOP 10 INSPETORES",
                              (Rectangle){ 176.0f, 18.0f, 316.0f, 12.0f }, 8.0f,
                              Fade((Color){ 160, 147, 142, 255 }, alpha));
    }

    // Controles DEV no topo: caixa de selecao ("Todos" ou #i Nome) + botao "Apagar ranking"
    Rectangle rCombo = { 304.0f, 6.0f, 98.0f, 22.0f };
    Rectangle rBtnApagar = { 406.0f, 6.0f, 90.0f, 22.0f };
    bool cliqueTratadoDev = false;
    if (devMode) {
        bool hovCombo = interativo && CheckCollisionPointRec(mv, rCombo);
        if (hovCombo) {
            algumHover = true;
            if (!g_devComboHovAnt) TocarHover(sndHover);
        }
        g_devComboHovAnt = hovCombo;

        Rectangle rComboTela = ParaTela(rCombo);
        Color cComboBg = (hovCombo || g_devDropdownAberto) ? (Color){ 56, 48, 50, 255 } : (Color){ 32, 28, 30, 255 };
        Color cComboBorda = g_devDropdownAberto ? (Color){ 255, 242, 230, 255 } : (Color){ 160, 147, 142, 255 };
        DrawRectangleRec(rComboTela, Fade(cComboBg, alpha));
        DrawRectangleLinesEx(rComboTela, 1.3f * esc, Fade(cComboBorda, alpha));

        const char *lblCombo = "Todos";
        if (g_devAlvoApagar >= 0 && g_devAlvoApagar < count) {
            const RankingEntry *sel = RankingGetEntry(g_devAlvoApagar);
            if (sel) lblCombo = TextFormat("#%d %s", g_devAlvoApagar + 1, sel->nome);
        } else if (count == 0) {
            lblCombo = "Vazio";
        }
        Rectangle rComboTxt = { rCombo.x + 2.0f, rCombo.y, rCombo.width - 14.0f, rCombo.height };
        DesenharTextoCentrado(lblCombo, rComboTxt, 8.0f,
                              Fade(count > 0 ? (Color){ 255, 242, 230, 255 } : (Color){ 125, 112, 113, 255 }, alpha));
        Rectangle rSetaBox = { rCombo.x + rCombo.width - 12.0f, rCombo.y, 10.0f, rCombo.height };
        DesenharTextoCentrado(g_devDropdownAberto ? "^" : "v", rSetaBox, 8.0f,
                              Fade((Color){ 189, 181, 168, 255 }, alpha));

        if (interativo && hovCombo && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (count > 0) g_devDropdownAberto = !g_devDropdownAberto;
            cliqueTratadoDev = true;
        }

        bool podeApagar = (count > 0);
        bool hovApagar = interativo && podeApagar && !g_devDropdownAberto && CheckCollisionPointRec(mv, rBtnApagar);
        if (hovApagar) {
            algumHover = true;
            if (!g_devApagarHovAnt) TocarHover(sndHover);
        }
        g_devApagarHovAnt = hovApagar;

        Rectangle rBtnApagarTela = ParaTela(rBtnApagar);
        DrawRectangleRec(ParaTela((Rectangle){ rBtnApagar.x + 2.0f, rBtnApagar.y + 2.0f, rBtnApagar.width, rBtnApagar.height }),
                         Fade(BLACK, 0.45f * alpha));
        Color cApagarBg = !podeApagar ? (Color){ 52, 42, 44, 220 }
                        : hovApagar   ? (Color){ 196, 72, 72, 255 }
                                      : (Color){ 138, 48, 50, 255 };
        DrawRectangleRec(rBtnApagarTela, Fade(cApagarBg, alpha));
        DrawRectangleLinesEx(rBtnApagarTela, 1.3f * esc,
                             Fade(podeApagar ? (Color){ 255, 242, 230, 255 } : (Color){ 110, 96, 98, 220 }, alpha));
        DesenharTextoCentrado("Apagar ranking", rBtnApagar, 8.0f,
                              Fade(podeApagar ? (Color){ 255, 242, 230, 255 } : (Color){ 125, 112, 113, 220 }, alpha));

        if (interativo && hovApagar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (g_devAlvoApagar < 0) {
                RankingClear();
            } else {
                RankingRemoveAt(g_devAlvoApagar);
            }
            g_devAlvoApagar = -1;
            MuralReload(-1);
            count = RankingGetCount();
            cliqueTratadoDev = true;
        }
    }

    // Botao "Menu Principal" no canto superior direito
    Rectangle btnMenu = { 500.0f, 6.0f, 130.0f, 22.0f };
    bool hovMenu = interativo && !g_devDropdownAberto && CheckCollisionPointRec(mv, btnMenu);
    if (hovMenu) {
        algumHover = true;
        if (!g_btnVoltarHovAnt) TocarHover(sndHover);
    }
    g_btnVoltarHovAnt = hovMenu;

    Rectangle rBtnMenu = ParaTela(btnMenu);
    DrawRectangleRec(ParaTela((Rectangle){ btnMenu.x + 2.0f, btnMenu.y + 2.0f, btnMenu.width, btnMenu.height }), Fade(BLACK, 0.5f * alpha));
    DrawRectangleRec(rBtnMenu, Fade(hovMenu ? (Color){ 189, 181, 168, 255 } : (Color){ 125, 112, 113, 255 }, alpha));
    DrawRectangleLinesEx(rBtnMenu, 1.5f * esc, Fade((Color){ 255, 242, 230, 255 }, alpha));
    DesenharTextoCentrado("<< Menu Principal", btnMenu, 9.0f,
                          Fade(hovMenu ? (Color){ 24, 21, 23, 255 } : (Color){ 255, 242, 230, 255 }, alpha));

    bool sair = false;
    if (interativo) {
        if (g_devDropdownAberto && IsKeyPressed(KEY_ESCAPE)) {
            g_devDropdownAberto = false;
        } else if ((hovMenu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
            sair = true;
        }
    }

    // 3) Grade 2x5 com os 10 Retratos ocupando toda a parede (x = 9..631, y = 40..352)
    const float cardW = 118.0f;
    const float cardH = 152.0f;
    const float gapX  = 8.0f;
    const float gapY  = 8.0f;
    const float startX = 9.0f;
    const float startY = 40.0f;

    int hoverCardAtual = -1;
    float pulso = 0.5f + 0.5f * sinf((float)GetTime() * 4.5f);

    for (int i = 0; i < RANKING_MAX_ENTRIES; i++) {
        int col = i % 5;
        int row = i / 5;
        Rectangle baseCard = {
            startX + col * (cardW + gapX),
            startY + row * (cardH + gapY),
            cardW,
            cardH
        };

        bool ocupado = (i < count);
        const RankingEntry *e = ocupado ? RankingGetEntry(i) : NULL;
        bool ehTop1 = (i == 0 && ocupado);
        bool ehDestaqueNovo = (i == g_highlightIdx && ocupado);

        bool hov = interativo && !g_devDropdownAberto && ocupado && CheckCollisionPointRec(mv, baseCard);
        if (hov) {
            hoverCardAtual = i;
            algumHover = true;
        }
        float alvo = hov ? 1.0f : 0.0f;
        g_cardAnim[i] += (alvo - g_cardAnim[i]) * fminf(1.0f, 14.0f * dt);

        Rectangle card = baseCard;
        card.y -= 2.5f * g_cardAnim[i];
        // Moldura quebrada fica levemente torta (1 pixel desalinhado no topo) para dar charme!
        bool quebrado = ocupado && (g_fotos[i].id == 0);

        // Sombra na parede atras do quadro
        Rectangle sombra = { baseCard.x + 2.5f, baseCard.y + 3.0f, cardW, cardH };
        DrawRectangleRec(ParaTela(sombra), Fade(BLACK, (ocupado ? 0.58f : 0.30f) * alpha));

        Rectangle rCard = ParaTela(card);
        Color corMolduraExt = !ocupado ? (Color){ 62, 56, 58, 220 }
                            : ehTop1   ? (Color){ 255, 242, 230, 255 }
                            : hov      ? (Color){ 228, 218, 206, 255 }
                                       : (Color){ 140, 127, 124, 255 };
        if (ehDestaqueNovo && pulso > 0.45f) {
            corMolduraExt = (Color){ 255, 242, 230, 255 };
        }
        Color corCorpoQuadro = !ocupado ? (Color){ 30, 27, 29, 210 }
                             : ehTop1   ? (Color){ 52, 46, 46, 255 }
                                        : (Color){ 38, 34, 36, 255 };

        DrawRectangleRec(rCard, Fade(corCorpoQuadro, alpha));
        DrawRectangleLinesEx(rCard, (ehTop1 || hov || ehDestaqueNovo) ? (2.2f * esc) : (1.4f * esc), Fade(corMolduraExt, alpha));

        // Faixa de topo do quadro (#1 FUNCIONARIO DO MES ou posicao #2..#10)
        Rectangle rFaixaTopo = { card.x + 3.0f, card.y + 3.0f, card.width - 6.0f, 13.0f };
        if (ehTop1) {
            DrawRectangleRec(ParaTela(rFaixaTopo), Fade((Color){ 189, 181, 168, 255 }, alpha));
            DesenharTextoCentrado("1o - DO MES", rFaixaTopo, 8.0f, Fade((Color){ 24, 21, 23, 255 }, alpha));
        } else {
            DrawRectangleRec(ParaTela(rFaixaTopo), Fade((Color){ 24, 21, 23, 230 }, alpha));
            const char *lblPos = ocupado
                ? (ehDestaqueNovo ? TextFormat("#%d - VOCÊ!", i + 1) : TextFormat("#%d LUGAR", i + 1))
                : TextFormat("#%d VAGA", i + 1);
            Color cPos = ehDestaqueNovo ? (Color){ 255, 242, 230, 255 }
                       : ocupado        ? (Color){ 189, 181, 168, 255 }
                                        : (Color){ 90, 83, 83, 200 };
            DesenharTextoCentrado(lblPos, rFaixaTopo, 8.0f, Fade(cPos, alpha));
        }

        // Janela interna do retrato (96 x 102 virtual -> proporcao proxima a 128x144!)
        Rectangle fotoBox = { card.x + 11.0f, card.y + 19.0f, 96.0f, 102.0f };
        Rectangle rFoto = ParaTela(fotoBox);

        if (!ocupado) {
            DrawRectangleRec(rFoto, Fade((Color){ 20, 18, 20, 220 }, alpha));
            DrawRectangleLinesEx(rFoto, 1.0f * esc, Fade((Color){ 56, 50, 52, 200 }, alpha));
            DesenharTextoCentrado("VAGA", (Rectangle){ fotoBox.x, fotoBox.y + 36.0f, fotoBox.width, 12.0f },
                                  8.0f, Fade((Color){ 78, 70, 72, 200 }, alpha));
            DesenharTextoCentrado("ABERTA", (Rectangle){ fotoBox.x, fotoBox.y + 49.0f, fotoBox.width, 12.0f },
                                  8.0f, Fade((Color){ 78, 70, 72, 200 }, alpha));
        } else if (quebrado) {
            DesenharRetratoQuebrado(fotoBox, alpha);
            DrawRectangleLinesEx(rFoto, 1.3f * esc, Fade((Color){ 125, 112, 113, 240 }, alpha));
        } else {
            Texture2D tf = g_fotos[i];
            DrawTexturePro(tf, (Rectangle){ 0, 0, (float)tf.width, (float)tf.height },
                           rFoto, (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, alpha));
            DrawRectangleLinesEx(rFoto, 1.4f * esc, Fade(ehTop1 ? (Color){ 255, 242, 230, 255 } : (Color){ 160, 147, 142, 255 }, alpha));
        }

        // Plaqueta metalica inferior com Nome e Pontuacao
        Rectangle plaqueta = { card.x + 5.0f, card.y + 124.0f, card.width - 10.0f, 24.0f };
        Rectangle rPlaq = ParaTela(plaqueta);
        DrawRectangleRec(rPlaq, Fade(ehTop1 ? (Color){ 30, 26, 27, 255 } : (Color){ 22, 19, 21, 248 }, alpha));
        DrawRectangleLinesEx(rPlaq, 1.0f * esc, Fade(ehTop1 ? (Color){ 228, 218, 206, 255 } : (Color){ 90, 83, 83, 230 }, alpha));

        if (ocupado && e) {
            DesenharTextoCentrado(e->nome,
                                  (Rectangle){ plaqueta.x, plaqueta.y + 1.5f, plaqueta.width, 11.0f },
                                  9.0f, Fade((Color){ 255, 242, 230, 255 }, alpha));
            const char *txtPts = TextFormat("%d pts %s", e->pontos, e->acertou ? "- OK" : "- X");
            Color corPts = e->acertou ? (Color){ 189, 181, 168, 255 } : (Color){ 210, 96, 94, 255 };
            DesenharTextoCentrado(txtPts,
                                  (Rectangle){ plaqueta.x, plaqueta.y + 12.5f, plaqueta.width, 10.0f },
                                  7.5f, Fade(corPts, alpha));
        } else {
            DesenharTextoCentrado("---",
                                  (Rectangle){ plaqueta.x, plaqueta.y + 2.0f, plaqueta.width, 10.0f },
                                  8.0f, Fade((Color){ 80, 72, 74, 200 }, alpha));
            DesenharTextoCentrado("0 pts",
                                  (Rectangle){ plaqueta.x, plaqueta.y + 12.0f, plaqueta.width, 10.0f },
                                  7.5f, Fade((Color){ 80, 72, 74, 200 }, alpha));
        }
    }

    if (hoverCardAtual >= 0 && hoverCardAtual != g_cardHoverAnt) {
        TocarHover(sndHover);
    }
    g_cardHoverAnt = hoverCardAtual;

    // Lista suspensa da caixa de selecao DEV (desenhada sobre os quadros)
    if (devMode && g_devDropdownAberto && count > 0) {
        float itemH = 16.0f;
        int totalItens = 1 + count; // 0: Todos, 1..count: entradas do ranking
        Rectangle rLista = { rCombo.x, rCombo.y + rCombo.height + 2.0f, rCombo.width, itemH * (float)totalItens };

        DrawRectangleRec(ParaTela((Rectangle){ rLista.x + 2.0f, rLista.y + 2.0f, rLista.width, rLista.height }),
                         Fade(BLACK, 0.65f * alpha));
        DrawRectangleRec(ParaTela(rLista), Fade((Color){ 24, 21, 23, 252 }, alpha));
        DrawRectangleLinesEx(ParaTela(rLista), 1.3f * esc, Fade((Color){ 255, 242, 230, 255 }, alpha));

        int itemHoverAtual = -2;
        for (int idx = -1; idx < count; idx++) {
            int rowIdx = idx + 1;
            Rectangle rItem = { rLista.x + 1.0f, rLista.y + (float)rowIdx * itemH, rLista.width - 2.0f, itemH };
            bool hovItem = interativo && CheckCollisionPointRec(mv, rItem);
            bool selItem = (g_devAlvoApagar == idx);
            if (hovItem) {
                itemHoverAtual = idx;
                algumHover = true;
            }

            if (hovItem || selItem) {
                Color cItemBg = hovItem ? (Color){ 138, 48, 50, 255 } : (Color){ 56, 48, 50, 255 };
                DrawRectangleRec(ParaTela(rItem), Fade(cItemBg, alpha));
            }
            const char *txtItem = "Todos";
            if (idx >= 0) {
                const RankingEntry *eItem = RankingGetEntry(idx);
                txtItem = eItem ? TextFormat("#%d %s", idx + 1, eItem->nome) : "---";
            }
            DesenharTextoCentrado(txtItem, rItem, 8.0f,
                                  Fade((hovItem || selItem) ? (Color){ 255, 242, 230, 255 } : (Color){ 189, 181, 168, 255 }, alpha));

            if (interativo && hovItem && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g_devAlvoApagar = idx;
                g_devDropdownAberto = false;
                cliqueTratadoDev = true;
            }
        }
        if (itemHoverAtual != -2 && itemHoverAtual != g_devDropItemHovAnt) {
            TocarHover(sndHover);
        }
        g_devDropItemHovAnt = itemHoverAtual;

        if (interativo && !cliqueTratadoDev && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            !CheckCollisionPointRec(mv, rLista) && !CheckCollisionPointRec(mv, rCombo)) {
            g_devDropdownAberto = false;
        }
    } else {
        g_devDropItemHovAnt = -2;
    }

    if (interativo) {
        SetMouseCursor(algumHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    }
    return sair;
}

// -----------------------------------------------------------------------------
// Cabine de Foto pos-pontuacao (captura ao vivo + dither leve + opcao sem foto)
// -----------------------------------------------------------------------------
void MuralFotoOpen(void) {
    g_fotoCongelada = false;
    g_temQuadroCam  = false;
    g_flashTimer    = 0.0f;
    g_btnFotoHovAnt = -1;
    g_camTentouAbrir = true;
    g_camDisponivel  = WebcamOpen();

    if (g_texPreview.id == 0) {
        Image blank = GenImageColor(MURAL_FOTO_W, MURAL_FOTO_H, (Color){ 24, 21, 23, 255 });
        g_texPreview = LoadTextureFromImage(blank);
        UnloadImage(blank);
        SetTextureFilter(g_texPreview, TEXTURE_FILTER_POINT);
    }
}

void MuralFotoClose(void) {
    WebcamClose();
    g_camDisponivel = false;
    g_camTentouAbrir = false;
}

static bool SalvarFotoCapturadaNoDisco(char *outPath, size_t outSize) {
    CRIAR_PASTA(PASTA_FOTOS);
    unsigned long ts = (unsigned long)time(NULL);
    int rnd = GetRandomValue(100, 999);
    snprintf(outPath, outSize, "%s/agente_%lu_%d.png", PASTA_FOTOS, ts, rnd);

    Image img = { 0 };
    img.data    = g_pixelsDither;
    img.width   = MURAL_FOTO_W;
    img.height  = MURAL_FOTO_H;
    img.mipmaps = 1;
    img.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return ExportImage(img, outPath);
}

static bool BotaoCabine(Rectangle rVirt, const char *label, bool destaque, bool negativo,
                        Vector2 mv, bool interativo, int idHov, int *hovAtualOut, float esc) {
    bool hov = interativo && CheckCollisionPointRec(mv, rVirt);
    if (hov) *hovAtualOut = idHov;

    Rectangle rTela = ParaTela(rVirt);
    DrawRectangleRec(ParaTela((Rectangle){ rVirt.x + 2.0f, rVirt.y + 2.5f, rVirt.width, rVirt.height }), Fade(BLACK, 0.55f));

    Color cFundo, cBorda, cTexto;
    if (negativo) {
        cFundo = hov ? (Color){ 196, 72, 72, 255 } : (Color){ 115, 44, 46, 255 };
        cBorda = hov ? (Color){ 255, 242, 230, 255 } : (Color){ 210, 96, 94, 255 };
        cTexto = (Color){ 255, 242, 230, 255 };
    } else if (destaque) {
        cFundo = hov ? (Color){ 255, 242, 230, 255 } : (Color){ 189, 181, 168, 255 };
        cBorda = (Color){ 255, 242, 230, 255 };
        cTexto = (Color){ 24, 21, 23, 255 };
    } else {
        cFundo = hov ? (Color){ 160, 147, 142, 255 } : (Color){ 90, 83, 83, 255 };
        cBorda = hov ? (Color){ 255, 242, 230, 255 } : (Color){ 160, 147, 142, 255 };
        cTexto = hov ? (Color){ 24, 21, 23, 255 } : (Color){ 255, 242, 230, 255 };
    }

    DrawRectangleRec(rTela, cFundo);
    DrawRectangleLinesEx(rTela, (hov ? 2.0f : 1.4f) * esc, cBorda);
    DesenharTextoCentrado(label, rVirt, 9.0f, cTexto);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool MuralFotoUpdateDraw(const char *nomeJogador, int pontuacao, bool acertouVeredito,
                         Sound sndHover, int *outRankIndex) {
    if (!g_camTentouAbrir) MuralFotoOpen();

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    if (g_flashTimer > 0.0f) {
        g_flashTimer -= dt;
        if (g_flashTimer < 0.0f) g_flashTimer = 0.0f;
    }

    // Atualiza preview ao vivo enquanto nao tirou a foto
    if (g_camDisponivel && !g_fotoCongelada) {
        if (WebcamGrabFrameRGB(g_rgbBruto, MURAL_FOTO_W, MURAL_FOTO_H)) {
            AplicarDitherLeveRetrato(g_rgbBruto, g_pixelsDither, MURAL_FOTO_W, MURAL_FOTO_H);
            UpdateTexture(g_texPreview, g_pixelsDither);
            g_temQuadroCam = true;
        }
    }

    bool interativo = !RayCanvasInputBlocked();
    Vector2 mv = MouseVirtual();
    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10.0f, 10.0f });
    float esc = ref.height / 10.0f;

    // Fundo escuro elegante
    DrawRectangleRec(ParaTela((Rectangle){ 0, 0, VIRTUAL_W, VIRTUAL_H }), (Color){ 18, 16, 18, 255 });

    // Painel central estilo Cracha/Terminal de Registro do Mural RASEC
    Rectangle painel = { 86.0f, 22.0f, 468.0f, 316.0f };
    Rectangle rPainel = ParaTela(painel);
    DrawRectangleRec(ParaTela((Rectangle){ painel.x + 4.0f, painel.y + 4.0f, painel.width, painel.height }), Fade(BLACK, 0.65f));
    DrawRectangleRec(rPainel, (Color){ 32, 29, 31, 252 });
    DrawRectangleLinesEx(rPainel, 2.0f * esc, (Color){ 189, 181, 168, 255 });

    Rectangle barraTopo = { painel.x, painel.y, painel.width, 24.0f };
    DrawRectangleRec(ParaTela(barraTopo), (Color){ 125, 112, 113, 255 });
    DesenharTextoCentrado("REGISTRO NO MURAL DE FUNCIONÁRIOS - RASEC", barraTopo, 10.0f, (Color){ 24, 21, 23, 255 });

    // Moldura grande de retrato na esquerda (160 x 180 virtual)
    Rectangle molduraExt = { painel.x + 22.0f, painel.y + 40.0f, 168.0f, 206.0f };
    Rectangle fotoArea   = { molduraExt.x + 12.0f, molduraExt.y + 12.0f, 144.0f, 162.0f };
    DrawRectangleRec(ParaTela(molduraExt), (Color){ 46, 41, 43, 255 });
    DrawRectangleLinesEx(ParaTela(molduraExt), 2.0f * esc, (Color){ 228, 218, 206, 255 });

    if (g_camDisponivel && g_temQuadroCam) {
        DrawTexturePro(g_texPreview,
                       (Rectangle){ 0, 0, (float)MURAL_FOTO_W, (float)MURAL_FOTO_H },
                       ParaTela(fotoArea), (Vector2){ 0, 0 }, 0.0f, WHITE);
        DrawRectangleLinesEx(ParaTela(fotoArea), 1.4f * esc, (Color){ 189, 181, 168, 255 });
        if (g_flashTimer > 0.0f) {
            DrawRectangleRec(ParaTela(fotoArea), Fade((Color){ 255, 242, 230, 255 }, g_flashTimer / 0.35f));
        }
    } else if (g_camDisponivel && !g_temQuadroCam) {
        DrawRectangleRec(ParaTela(fotoArea), (Color){ 20, 18, 20, 255 });
        DrawRectangleLinesEx(ParaTela(fotoArea), 1.2f * esc, (Color){ 125, 112, 113, 255 });
        DesenharTextoCentrado("Iniciando câmera...", fotoArea, 9.0f, (Color){ 189, 181, 168, 255 });
    } else {
        DesenharRetratoQuebrado(fotoArea, 1.0f);
        DrawRectangleLinesEx(ParaTela(fotoArea), 1.2f * esc, (Color){ 125, 112, 113, 255 });
    }

    DesenharTextoCentrado(TextFormat("%s - %d PTS", nomeJogador ? nomeJogador : "AGENTE", pontuacao),
                          (Rectangle){ molduraExt.x + 6.0f, molduraExt.y + 180.0f, molduraExt.width - 12.0f, 20.0f },
                          9.0f, (Color){ 255, 242, 230, 255 });

    // Coluna da direita: explicacao amigavel + botoes de acao
    float rx = painel.x + 208.0f;
    float rw = 240.0f;
    int hovAtual = -1;

    DesenharTextoCentrado("FOTO DO INSPETOR", (Rectangle){ rx, painel.y + 44.0f, rw, 16.0f }, 12.0f, (Color){ 255, 242, 230, 255 });

    if (g_camDisponivel && !g_fotoCongelada) {
        DesenharTextoCentrado("Registre seu retrato para o Mural", (Rectangle){ rx, painel.y + 68.0f, rw, 13.0f }, 9.0f, (Color){ 189, 181, 168, 255 });
        DesenharTextoCentrado("de Top 10 Funcionários da RASEC!", (Rectangle){ rx, painel.y + 82.0f, rw, 13.0f }, 9.0f, (Color){ 189, 181, 168, 255 });
        DesenharTextoCentrado("(Opcional: se preferir não tirar,", (Rectangle){ rx, painel.y + 102.0f, rw, 12.0f }, 8.0f, (Color){ 140, 127, 124, 255 });
        DesenharTextoCentrado("seu quadro ficará com vidro trincado.)", (Rectangle){ rx, painel.y + 114.0f, rw, 12.0f }, 8.0f, (Color){ 140, 127, 124, 255 });

        bool capturar = BotaoCabine((Rectangle){ rx, painel.y + 150.0f, rw, 30.0f },
                                    "[ TIRAR FOTO ]", true, false, mv, interativo && g_temQuadroCam, 1, &hovAtual, esc);
        if (interativo && g_temQuadroCam && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) capturar = true;

        bool pular = BotaoCabine((Rectangle){ rx, painel.y + 194.0f, rw, 26.0f },
                                 "Prefiro não tirar foto", false, false, mv, interativo, 2, &hovAtual, esc);

        if (capturar && g_temQuadroCam) {
            g_fotoCongelada = true;
            g_flashTimer = 0.35f;
            TocarHover(sndHover);
        } else if (pular) {
            MuralFotoClose();
            int pos = RankingAddWithPhoto(nomeJogador, pontuacao, acertouVeredito, NULL);
            if (outRankIndex) *outRankIndex = pos;
            MuralReload(pos);
            return true;
        }
    } else if (g_camDisponivel && g_fotoCongelada) {
        DesenharTextoCentrado("Retrato capturado!", (Rectangle){ rx, painel.y + 68.0f, rw, 14.0f }, 10.0f, (Color){ 255, 242, 230, 255 });
        DesenharTextoCentrado("Ficou bom ou quer tirar outra?", (Rectangle){ rx, painel.y + 86.0f, rw, 13.0f }, 9.0f, (Color){ 189, 181, 168, 255 });

        bool confirmar = BotaoCabine((Rectangle){ rx, painel.y + 132.0f, rw, 30.0f },
                                     "Salvar e Ver Mural >>", true, false, mv, interativo, 3, &hovAtual, esc);
        if (interativo && IsKeyPressed(KEY_ENTER)) confirmar = true;

        bool repetir = BotaoCabine((Rectangle){ rx, painel.y + 172.0f, rw, 26.0f },
                                   "Tirar outra foto", false, false, mv, interativo, 4, &hovAtual, esc);

        bool descartar = BotaoCabine((Rectangle){ rx, painel.y + 208.0f, rw, 24.0f },
                                     "Ir sem foto (quadro quebrado)", false, true, mv, interativo, 5, &hovAtual, esc);

        if (confirmar) {
            char caminhoSalvo[RANKING_PHOTO_MAX] = { 0 };
            bool ok = SalvarFotoCapturadaNoDisco(caminhoSalvo, sizeof caminhoSalvo);
            MuralFotoClose();
            int pos = RankingAddWithPhoto(nomeJogador, pontuacao, acertouVeredito, ok ? caminhoSalvo : NULL);
            if (outRankIndex) *outRankIndex = pos;
            MuralReload(pos);
            return true;
        } else if (repetir) {
            g_fotoCongelada = false;
            TocarHover(sndHover);
        } else if (descartar) {
            MuralFotoClose();
            int pos = RankingAddWithPhoto(nomeJogador, pontuacao, acertouVeredito, NULL);
            if (outRankIndex) *outRankIndex = pos;
            MuralReload(pos);
            return true;
        }
    } else {
        DesenharTextoCentrado("Câmera não detectada ou indisponível.", (Rectangle){ rx, painel.y + 70.0f, rw, 13.0f }, 9.0f, (Color){ 189, 181, 168, 255 });
        DesenharTextoCentrado("Sua pontuação será registrada no", (Rectangle){ rx, painel.y + 92.0f, rw, 12.0f }, 8.5f, (Color){ 160, 147, 142, 255 });
        DesenharTextoCentrado("Mural com moldura de vidro trincado!", (Rectangle){ rx, painel.y + 105.0f, rw, 12.0f }, 8.5f, (Color){ 160, 147, 142, 255 });

        bool tentarCam = BotaoCabine((Rectangle){ rx, painel.y + 148.0f, rw, 26.0f },
                                     "Tentar abrir câmera novamente", false, false, mv, interativo, 6, &hovAtual, esc);
        bool continuar = BotaoCabine((Rectangle){ rx, painel.y + 186.0f, rw, 30.0f },
                                     "Continuar para o Mural >>", true, false, mv, interativo, 7, &hovAtual, esc);
        if (interativo && IsKeyPressed(KEY_ENTER)) continuar = true;

        if (tentarCam) {
            MuralFotoClose();
            MuralFotoOpen();
        } else if (continuar) {
            MuralFotoClose();
            int pos = RankingAddWithPhoto(nomeJogador, pontuacao, acertouVeredito, NULL);
            if (outRankIndex) *outRankIndex = pos;
            MuralReload(pos);
            return true;
        }
    }

    DesenharTextoCentrado("As fotos ficam salvas apenas localmente nesta máquina.",
                          (Rectangle){ painel.x + 12.0f, painel.y + painel.height - 20.0f, painel.width - 24.0f, 12.0f },
                          7.5f, (Color){ 125, 112, 113, 230 });

    if (hovAtual >= 0 && hovAtual != g_btnFotoHovAnt) {
        TocarHover(sndHover);
    }
    g_btnFotoHovAnt = hovAtual;
    SetMouseCursor((hovAtual >= 0) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    return false;
}
