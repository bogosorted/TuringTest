#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SETTINGS_FILE  "settings.cfg"
#define PALETTES_DIR   "assets/pallets"
#define MAX_PALETAS    64
#define NOME_LEN       64

typedef struct {
    char  nome[NOME_LEN];    // mostrado no menu ("Soft Gameboy")
    char  chave[NOME_LEN];   // nome do arquivo sem extensao
    Color tons[PALETTE_NUM_STOPS];   // do mais escuro ao mais claro
} Paleta;

// A 0 e a rampa que o jogo inteiro usa hoje (fica fixa no codigo).
// As outras vem das imagens em assets/pallets: o shader de raycanvas.c troca
// cada tom dessa rampa pelo tom equivalente da paleta escolhida.
static Paleta g_paletas[MAX_PALETAS] = {
    { "Original", "original", { {   0,   0,   0, 255 }, {  32,  29,  31, 255 }, { 125, 112, 113, 255 }, { 189, 181, 168, 255 }, { 255, 242, 230, 255 } } },
};
static int g_numPaletas = 1;

static float g_volMusica  = 0.8f;
static float g_volEfeitos = 1.0f;
static int   g_paleta     = 0;
static bool  g_telaCheia  = true;

#define JANELA_W 1280
#define JANELA_H 720

static float Limitar01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float Luminancia(Color c) {
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

static int CompararLuminancia(const void *a, const void *b) {
    float la = Luminancia(*(const Color *)a);
    float lb = Luminancia(*(const Color *)b);
    return (la > lb) - (la < lb);
}

static int CompararTexto(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

// "soft-gameboy-1x" -> "Soft Gameboy"
static void NomeBonito(const char *arquivo, char out[NOME_LEN]) {
    snprintf(out, NOME_LEN, "%s", arquivo);
    size_t n = strlen(out);
    if (n > 3 && strcmp(out + n - 3, "-1x") == 0) out[n - 3] = '\0';   // sufixo do export do Lospec

    bool inicioPalavra = true;
    for (char *p = out; *p; p++) {
        if (*p == '-' || *p == '_') { *p = ' '; inicioPalavra = true; continue; }
        if (inicioPalavra && *p >= 'a' && *p <= 'z') *p = (char)toupper((unsigned char)*p);
        inicioPalavra = false;
    }
}

// Le uma imagem de paleta (1 pixel por cor) e preenche os 5 tons, do escuro ao claro.
// Com mais de 5 cores pega 5 espalhadas; com menos, interpola entre elas.
static bool CarregarPaleta(const char *caminho, Paleta *pal) {
    Image img = LoadImage(caminho);
    if (img.data == NULL) return false;

    int total = img.width * img.height;
    Color *pixels = LoadImageColors(img);
    UnloadImage(img);
    if (pixels == NULL || total <= 0) return false;

    // Ignora pixels transparentes
    int n = 0;
    for (int i = 0; i < total; i++) {
        if (pixels[i].a > 0) pixels[n++] = (Color){ pixels[i].r, pixels[i].g, pixels[i].b, 255 };
    }
    if (n == 0) { UnloadImageColors(pixels); return false; }

    qsort(pixels, (size_t)n, sizeof(Color), CompararLuminancia);

    for (int k = 0; k < PALETTE_NUM_STOPS; k++) {
        float pos = (float)k * (float)(n - 1) / (float)(PALETTE_NUM_STOPS - 1);
        if (n >= PALETTE_NUM_STOPS) {
            pal->tons[k] = pixels[(int)(pos + 0.5f)];   // cor exata da paleta
        } else {
            int i = (int)pos;
            int j = (i + 1 < n) ? i + 1 : i;
            float t = pos - (float)i;
            pal->tons[k] = (Color){
                (unsigned char)(pixels[i].r + (pixels[j].r - pixels[i].r) * t),
                (unsigned char)(pixels[i].g + (pixels[j].g - pixels[i].g) * t),
                (unsigned char)(pixels[i].b + (pixels[j].b - pixels[i].b) * t),
                255
            };
        }
    }
    UnloadImageColors(pixels);
    return true;
}

static void CarregarPaletas(void) {
    g_numPaletas = 1;   // mantem so a Original
    if (!DirectoryExists(PALETTES_DIR)) return;

    FilePathList arquivos = LoadDirectoryFiles(PALETTES_DIR);

    // Ordem alfabetica (o sistema nao garante nenhuma ordem)
    qsort(arquivos.paths, arquivos.count, sizeof(char *), CompararTexto);

    for (unsigned int i = 0; i < arquivos.count && g_numPaletas < MAX_PALETAS; i++) {
        const char *caminho = arquivos.paths[i];
        if (!IsFileExtension(caminho, ".png")) continue;

        Paleta *pal = &g_paletas[g_numPaletas];
        if (!CarregarPaleta(caminho, pal)) {
            TraceLog(LOG_WARNING, "Paleta invalida: %s", caminho);
            continue;
        }
        snprintf(pal->chave, NOME_LEN, "%s", GetFileNameWithoutExt(caminho));
        NomeBonito(pal->chave, pal->nome);
        g_numPaletas++;
    }
    UnloadDirectoryFiles(arquivos);
    TraceLog(LOG_INFO, "Paletas carregadas: %d (+ Original)", g_numPaletas - 1);
}

void SettingsLoad(void) {
    CarregarPaletas();

    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;   // primeira vez: fica com os padroes

    char linha[128];
    while (fgets(linha, sizeof linha, f)) {
        float v;
        int   n;
        if (sscanf(linha, "musica=%f", &v) == 1)       g_volMusica  = Limitar01(v);
        else if (sscanf(linha, "telacheia=%d", &n) == 1) g_telaCheia = (n != 0);
        else if (sscanf(linha, "efeitos=%f", &v) == 1) g_volEfeitos = Limitar01(v);
        // A paleta NAO e lida: o jogo sempre abre na Original (g_paleta = 0).
    }
    fclose(f);
}

void SettingsSave(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) {
        TraceLog(LOG_WARNING, "Nao foi possivel salvar %s", SETTINGS_FILE);
        return;
    }
    // A paleta nao e salva: vale so enquanto o jogo esta aberto
    fprintf(f, "musica=%.2f\nefeitos=%.2f\ntelacheia=%d\n",
            g_volMusica, g_volEfeitos, g_telaCheia ? 1 : 0);
    fclose(f);
}

float SettingsGetMusicVolume(void) { return g_volMusica; }
void  SettingsSetMusicVolume(float v) { g_volMusica = Limitar01(v); }
float SettingsGetSfxVolume(void) { return g_volEfeitos; }
void  SettingsSetSfxVolume(float v) { g_volEfeitos = Limitar01(v); }

bool SettingsGetFullscreen(void) { return g_telaCheia; }
void SettingsSetFullscreen(bool ligado) { g_telaCheia = ligado; }

// Tela cheia = janela sem borda do tamanho do monitor ("borderless windowed").
// Nao usamos ToggleFullscreen/ToggleBorderlessWindowed: no Linux a raylib usa
// fullscreen exclusivo, que trava o Alt+Tab (ver comentario no main.c).
void SettingsApplyWindowMode(void) {
    int mon = GetCurrentMonitor();
    int mw = GetMonitorWidth(mon);
    int mh = GetMonitorHeight(mon);
    Vector2 origem = GetMonitorPosition(mon);

    if (g_telaCheia) {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowPosition((int)origem.x, (int)origem.y);
        SetWindowSize(mw, mh);
    } else {
        // Janela 1280x720 centralizada (menor se o monitor for pequeno)
        int w = JANELA_W, h = JANELA_H;
        if (w > mw * 9 / 10) { w = mw * 9 / 10; h = w * 9 / 16; }
        if (h > mh * 9 / 10) { h = mh * 9 / 10; w = h * 16 / 9; }
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(w, h);
        SetWindowPosition((int)origem.x + (mw - w) / 2, (int)origem.y + (mh - h) / 2);
    }
}

int SettingsPaletteCount(void) { return g_numPaletas; }
int SettingsGetPalette(void) { return g_paleta; }

void SettingsSetPalette(int indice) {
    g_paleta = ((indice % g_numPaletas) + g_numPaletas) % g_numPaletas;
}

const char *SettingsPaletteName(int indice) {
    if (indice < 0 || indice >= g_numPaletas) return "";
    return g_paletas[indice].nome;
}

const Color *SettingsPaletteStops(int indice) {
    if (indice < 0 || indice >= g_numPaletas) indice = 0;
    return g_paletas[indice].tons;
}

const Color *SettingsOriginalStops(void) {
    return g_paletas[0].tons;
}
