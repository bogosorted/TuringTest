#include "settings.h"
#include "dialog_system.h"
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
static bool  g_tutorial   = true;
static bool  g_balaoInteracao = true;
static bool  g_devMode    = false;
static bool  g_crt        = false;
static bool  g_cam3dFixa  = true;
static bool  g_autoReset  = false;
static bool  g_techNight  = false;
static bool  g_devExpanded3D = false;
static bool  g_devDisable2D = false;
static bool  g_devDisable3D = false;
static bool  g_setaGuiando  = true;
static bool  g_devExpPalettes = false;
static bool  g_restartApp = false;
static unsigned int g_paletasSecretasMask = 0u;

static int SecretBitForKey(const char *chave) {
    if (!chave) return -1;
    if (strcmp(chave, "noite-azul") == 0)    return 0;
    if (strcmp(chave, "carmesim") == 0)      return 1;
    if (strcmp(chave, "cores-reais") == 0)   return 2;
    if (strcmp(chave, "espaco-mental") == 0) return 3;
    if (strcmp(chave, "whos-liliac") == 0)   return 4;
    return -1;
}

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

// Mapeia nomes especiais de paletas e formata os demais ("espaco-mental" -> "Espaço Mental")
static void NomeBonito(const char *arquivo, char out[NOME_LEN]) {
    if (strcmp(arquivo, "whos-liliac") == 0)   { snprintf(out, NOME_LEN, "Who's liliac?"); return; }
    if (strcmp(arquivo, "gameboy") == 0)       { snprintf(out, NOME_LEN, "Gameboy"); return; }
    if (strcmp(arquivo, "espaco-mental") == 0) { snprintf(out, NOME_LEN, "Espaço Mental"); return; }
    if (strcmp(arquivo, "crepusculo") == 0)     { snprintf(out, NOME_LEN, "Crepúsculo"); return; }
    if (strcmp(arquivo, "ambar-terminal") == 0) { snprintf(out, NOME_LEN, "Âmbar Terminal"); return; }

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
        else if (sscanf(linha, "tutorial=%d", &n) == 1) g_tutorial = (n != 0);
        else if (sscanf(linha, "balao_interacao=%d", &n) == 1) g_balaoInteracao = (n != 0);
        else if (sscanf(linha, "dev=%d", &n) == 1) g_devMode = (n != 0);
        else if (sscanf(linha, "crt=%d", &n) == 1) g_crt = (n != 0);
        else if (sscanf(linha, "cam3d_fixa=%d", &n) == 1) g_cam3dFixa = (n != 0);
        else if (sscanf(linha, "auto_reset=%d", &n) == 1) g_autoReset = (n != 0);
        else if (sscanf(linha, "tech_night=%d", &n) == 1) g_techNight = (n != 0);
        else if (sscanf(linha, "tela3d_expandida=%d", &n) == 1) g_devExpanded3D = (n != 0);
        else if (sscanf(linha, "dev_sem_2d=%d", &n) == 1) g_devDisable2D = (n != 0);
        else if (sscanf(linha, "dev_sem_3d=%d", &n) == 1) g_devDisable3D = (n != 0);
        else if (sscanf(linha, "seta_guiando=%d", &n) == 1) g_setaGuiando = (n != 0);
        else if (sscanf(linha, "paletas_exp=%d", &n) == 1) g_devExpPalettes = (n != 0);
        // A paleta e as paletas secretas sempre comecam zeradas ao abrir o jogo.
    }
    fclose(f);
    if (g_autoReset) {
        g_volMusica  = 0.8f;
        g_volEfeitos = 1.0f;
        g_telaCheia  = true;
        g_tutorial   = true;
        g_balaoInteracao = true;
        g_cam3dFixa  = true;
        g_paleta     = 0;
        g_crt        = false;
    }
    global_infinite_tokens = g_devMode;
}

void SettingsSave(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) {
        TraceLog(LOG_WARNING, "Nao foi possivel salvar %s", SETTINGS_FILE);
        return;
    }
    // A paleta nao e salva: vale so enquanto o jogo esta aberto
    fprintf(f, "musica=%.2f\nefeitos=%.2f\ntelacheia=%d\ntutorial=%d\nbalao_interacao=%d\ndev=%d\ncrt=%d\ncam3d_fixa=%d\nauto_reset=%d\ntech_night=%d\ntela3d_expandida=%d\ndev_sem_2d=%d\ndev_sem_3d=%d\nseta_guiando=%d\npaletas_exp=%d\npaletas_desbloqueadas=%u\n",
            g_volMusica, g_volEfeitos, g_telaCheia ? 1 : 0, g_tutorial ? 1 : 0, g_balaoInteracao ? 1 : 0, g_devMode ? 1 : 0,
            g_crt ? 1 : 0, g_cam3dFixa ? 1 : 0, g_autoReset ? 1 : 0, g_techNight ? 1 : 0, g_devExpanded3D ? 1 : 0,
            g_devDisable2D ? 1 : 0, g_devDisable3D ? 1 : 0, g_setaGuiando ? 1 : 0, g_devExpPalettes ? 1 : 0, g_paletasSecretasMask);
    fclose(f);
}

float SettingsGetMusicVolume(void) { return g_volMusica; }
void  SettingsSetMusicVolume(float v) { g_volMusica = Limitar01(v); }
float SettingsGetSfxVolume(void) { return g_volEfeitos; }
void  SettingsSetSfxVolume(float v) { g_volEfeitos = Limitar01(v); }

bool SettingsGetFullscreen(void) { return g_telaCheia; }
void SettingsSetFullscreen(bool ligado) { g_telaCheia = ligado; }

bool SettingsGetTutorial(void) { return g_tutorial; }
void SettingsSetTutorial(bool ligado) { g_tutorial = ligado; }

bool SettingsGetBalaoInteracao(void) { return g_balaoInteracao; }
void SettingsSetBalaoInteracao(bool ligado) { g_balaoInteracao = ligado; }

bool SettingsGetDevMode(void) { return g_devMode; }
void SettingsSetDevMode(bool ligado) {
    g_devMode = ligado;
    global_infinite_tokens = ligado;
}

bool SettingsGetCRT(void) { return g_crt; }
void SettingsSetCRT(bool ligado) { g_crt = ligado; }

bool SettingsGetCamera3DFixa(void) { return g_cam3dFixa; }
void SettingsSetCamera3DFixa(bool fixa) { g_cam3dFixa = fixa; }

bool SettingsGetAutoReset(void) { return g_autoReset; }
void SettingsResetToDefaultWithCRT(void) {
    bool prevFull = g_telaCheia;
    g_volMusica  = 0.8f;
    g_volEfeitos = 1.0f;
    g_telaCheia  = true;
    g_tutorial   = true;
    g_balaoInteracao = true;
    g_cam3dFixa  = true;
    g_paleta     = 0;
    g_crt        = false;
    SettingsSave();
    if (prevFull != g_telaCheia) {
        SettingsApplyWindowMode();
    }
}

void SettingsSetAutoReset(bool ligado) {
    g_autoReset = ligado;
    if (ligado) {
        SettingsResetToDefaultWithCRT();
    }
}

static bool g_devUnlockSubsolo = false;
static bool g_devUnlockUpperFloors = false;
static bool g_devFirstPerson = false;

bool SettingsGetTechNight(void) { return g_techNight; }
void SettingsSetTechNight(bool ligado) { g_techNight = ligado; }
bool SettingsGetDevUnlockSubsolo(void) { return g_devUnlockSubsolo; }
void SettingsSetDevUnlockSubsolo(bool ligado) { g_devUnlockSubsolo = ligado; }
bool SettingsGetDevUnlockUpperFloors(void) { return g_devUnlockUpperFloors; }
void SettingsSetDevUnlockUpperFloors(bool ligado) { g_devUnlockUpperFloors = ligado; }
bool SettingsGetDevFirstPerson(void) { return g_devFirstPerson; }
void SettingsSetDevFirstPerson(bool ligado) { g_devFirstPerson = ligado; }
bool SettingsGetDevExpanded3D(void) { return g_devExpanded3D; }
void SettingsSetDevExpanded3D(bool ligado) { g_devExpanded3D = ligado; }
bool SettingsGetDevDisable2D(void) { return g_devDisable2D; }
void SettingsSetDevDisable2D(bool ligado) { g_devDisable2D = ligado; }
bool SettingsGetDevDisable3D(void) { return g_devDisable3D; }
void SettingsSetDevDisable3D(bool ligado) { g_devDisable3D = ligado; }
bool SettingsGetSetaGuiando(void) { return g_setaGuiando; }
void SettingsSetSetaGuiando(bool ligado) { g_setaGuiando = ligado; }
bool SettingsGetDevExperimentalPalettes(void) { return g_devExpPalettes; }
void SettingsSetDevExperimentalPalettes(bool ligado) { g_devExpPalettes = ligado; }

void SettingsRequestAppRestart(void) { g_restartApp = true; }
bool SettingsIsAppRestartRequested(void) { return g_restartApp; }
bool SettingsConsumeAppRestart(void) {
    bool r = g_restartApp;
    g_restartApp = false;
    return r;
}

// Aplica modo tela cheia sem bordas (borderless windowed / full screen falso, para Alt+Tab fluido) ou janela
void SettingsApplyWindowMode(void) {
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    int mon = GetCurrentMonitor();
    if (mon < 0) mon = 0;
    int mw = GetMonitorWidth(mon);
    int mh = GetMonitorHeight(mon);
    Vector2 origem = GetMonitorPosition(mon);
    if (mw <= 0 || mh <= 0) {
        mw = JANELA_W;
        mh = JANELA_H;
        origem = (Vector2){ 0.0f, 0.0f };
    }

    if (g_telaCheia) {
        if (!IsWindowState(FLAG_WINDOW_UNDECORATED)) {
            SetWindowState(FLAG_WINDOW_UNDECORATED);
        }
        SetWindowSize(mw, mh);
        SetWindowPosition((int)origem.x, (int)origem.y);
    } else {
        // Janela 1280x720 centralizada (menor se o monitor for pequeno)
        int w = JANELA_W, h = JANELA_H;
        if (w > mw * 9 / 10) { w = mw * 9 / 10; h = w * 9 / 16; }
        if (h > mh * 9 / 10) { h = mh * 9 / 10; w = h * 16 / 9; }
        if (w < 640) w = 640;
        if (h < 360) h = 360;
        if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
        }
        SetWindowSize(w, h);
        SetWindowPosition((int)origem.x + (mw - w) / 2, (int)origem.y + (mh - h) / 2);
    }
}

bool SettingsIsPaletteSecret(int indice) {
    if (indice < 0 || indice >= g_numPaletas) return false;
    return SecretBitForKey(g_paletas[indice].chave) >= 0;
}

bool SettingsIsPaletteUnlocked(int indice) {
    if (indice < 0 || indice >= g_numPaletas) return false;
    int bit = SecretBitForKey(g_paletas[indice].chave);
    if (bit < 0) return true;
    return (g_paletasSecretasMask & (1u << bit)) != 0;
}

bool SettingsUnlockPaletteByKey(const char *chave) {
    int bit = SecretBitForKey(chave);
    if (bit < 0) return false;
    bool eraNova = ((g_paletasSecretasMask & (1u << bit)) == 0);
    g_paletasSecretasMask |= (1u << bit);
    return eraNova;
}

void SettingsResetSecretPalettes(void) {
    g_paletasSecretasMask = 0u;
    if (SettingsIsPaletteSecret(g_paleta)) {
        g_paleta = 0;
    }
}

int SettingsUnlockedPaletteCount(void) {
    int cnt = 0;
    for (int i = 0; i < g_numPaletas; i++) {
        if (SettingsIsPaletteUnlocked(i)) cnt++;
    }
    return cnt;
}

int SettingsUnlockedPaletteOrder(int indice) {
    int ord = 0;
    for (int i = 0; i < g_numPaletas; i++) {
        if (!SettingsIsPaletteUnlocked(i)) continue;
        if (i == indice) return ord;
        ord++;
    }
    return 0;
}

int SettingsPaletteCount(void) { return g_numPaletas; }
int SettingsGetPalette(void) { return g_paleta; }

void SettingsSetPalette(int indice) {
    if (g_numPaletas <= 0) { g_paleta = 0; return; }
    int dir = (indice < g_paleta) ? -1 : 1;
    int cand = ((indice % g_numPaletas) + g_numPaletas) % g_numPaletas;
    for (int step = 0; step < g_numPaletas; step++) {
        if (SettingsIsPaletteUnlocked(cand)) {
            g_paleta = cand;
            return;
        }
        cand = ((cand + dir) % g_numPaletas + g_numPaletas) % g_numPaletas;
    }
    g_paleta = 0;
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
