#include "mapa_holograma.h"
#include "settings.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

// ---------------------------------------------------------------
// Geometria (unidades do mundo 3D) - varia entre modo 2D (antigo) e modo 3D
// ---------------------------------------------------------------
#define ESCADA_ALT     0.9f     // altura do ultimo degrau
#define PLACA_ESP      0.08f
#define PAREDE_ALT     0.32f
#define ESCADA_DEGRAUS 6
#define MAX_SALAS      48

static int   g_numAndares = 3;
static bool  g_modo3D = false;
static float g_andarAlt = 3.2f;
static float g_plantaW  = 7.6f;
static float g_plantaD  = 4.0f;
static float g_camDist  = 15.5f;
static float g_camAlt   = 8.6f;

// Brilho de cada elemento (0..1). O shader converte em tons da rampa com dithering.
#define LUM_PLACA          0.14f
#define LUM_PLACA_ATUAL    0.22f
#define LUM_PLACA_BORDA    0.50f
#define LUM_PLACA_BORDA_AT 0.62f
#define LUM_CORREDOR       0.30f
#define LUM_CORREDOR_BORDA 0.46f
#define LUM_SALA_OFF       0.16f   // fora de alcance (outro andar)
#define LUM_SALA           0.34f
#define LUM_SALA_ATUAL     0.50f
#define LUM_SALA_HOVER     0.56f
#define LUM_PAREDE_OFF     0.52f
#define LUM_PAREDE         0.80f
#define LUM_PAREDE_DEST    1.00f
#define LUM_POCO           0.48f

static const Color COR_PONTO    = { 196,  72,  72, 255 };   // vermelho: posicao do jogador
static const Color COR_SUSPEITO = { 255, 224,  48, 255 };   // amarelo: suspeito na sala

static bool     g_apenasAndarAtual   = false;
static int      g_andarFoco          = 0;
static uint64_t g_salasComSuspeitos  = 0;
static bool     g_zoomApenasUmAnt    = false;
static int      g_zoomAndarFocoAnt   = -99;

static const char *HOLO_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float tempo;\n"
    "uniform float altura;\n"
    "out vec4 finalColor;\n"
    "const vec3 TONS[5] = vec3[5](vec3(0.0), vec3(32.0, 29.0, 31.0) / 255.0,\n"
    "    vec3(125.0, 112.0, 113.0) / 255.0, vec3(189.0, 181.0, 168.0) / 255.0, vec3(255.0, 242.0, 230.0) / 255.0);\n"
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    if (c.a < 0.01) discard;\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    float y = gl_FragCoord.y;\n"
    "    l *= (mod(y, 2.0) < 1.0) ? 1.0 : 0.975;\n"                          // scanlines
    "    float faixa = fract(tempo * 0.15) * (altura + 24.0) - 12.0;\n"
    "    l += 0.05 * exp(-pow((y - faixa) / 4.0, 2.0));\n"                   // faixa de varredura
    "    l *= 0.995 + 0.005 * sin(tempo * 31.0);\n"                            // tremulacao
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar), 0.0, 4.0));\n"
    "    if (idx == 0) discard;\n"                                            // preto = vazado (holograma)
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static RenderTexture2D     g_cena;     // 3D cru
static RenderTexture2D     g_final;    // depois do dithering
static bool                g_texPronta = false;
static Shader              g_shader;
static int                 g_locTempo = -1;
static int                 g_locAltura = -1;
static Camera3D            g_camera;
static float               g_fovyAjustado = 30.0f;
static int                 g_largura, g_altura;
static const HoloSala     *g_salas;
static int                 g_numSalas;
static const HoloCorredor *g_corredores;
static int                 g_numCorredores;
static bool                g_pronto = false;

static Color Lum(float l) {
    unsigned char v = (unsigned char)(Clamp(l, 0.0f, 1.0f) * 255.0f);
    return (Color){ v, v, v, 255 };
}

static float AlturaAndar(int andar) {
    return andar * g_andarAlt;
}

// Centro de um retangulo da planta, em cima da placa
static Vector3 CentroRet(int andar, HoloRet r, float y) {
    return (Vector3){ r.x + r.w / 2.0f, AlturaAndar(andar) + PLACA_ESP / 2.0f + y, r.z + r.d / 2.0f };
}

static Vector2 Projetar(Vector3 p) {
    return GetWorldToScreenEx(p, g_camera, g_largura, g_altura);
}

void MapaHoloInit(const HoloSala *salas, int numSalas, const HoloCorredor *corredores, int numCorredores, bool modo3D) {
    MapaHoloUnload();   // seguro chamar de novo (ex.: reiniciar a cena)
    g_salas = salas;
    g_numSalas = (numSalas > MAX_SALAS) ? MAX_SALAS : numSalas;
    g_corredores = corredores;
    g_numCorredores = numCorredores;
    g_modo3D = modo3D;

    int maxAndar = 0;
    for (int i = 0; i < g_numSalas; i++) {
        if (g_salas[i].numPartes > 0 && g_salas[i].andar > maxAndar) maxAndar = g_salas[i].andar;
    }
    g_numAndares = maxAndar + 1;
    if (g_numAndares < 1) g_numAndares = 1;
    if (g_numAndares > HOLO_NUM_ANDARES) g_numAndares = HOLO_NUM_ANDARES;

    if (modo3D) {
        g_andarAlt = 3.2f;
        g_plantaW  = 7.6f;
        g_plantaD  = 4.0f;
        g_camDist  = 15.5f;
        g_camAlt   = 8.6f;
    } else {
        g_andarAlt = 2.6f;
        g_plantaW  = 6.2f;
        g_plantaD  = 3.2f;
        g_camDist  = 13.0f;
        g_camAlt   = 7.0f;
    }

    g_shader = LoadShaderFromMemory(NULL, HOLO_FS);
    g_locTempo  = GetShaderLocation(g_shader, "tempo");
    g_locAltura = GetShaderLocation(g_shader, "altura");

    g_camera = (Camera3D){ 0 };
    g_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    g_camera.projection = CAMERA_PERSPECTIVE;
    g_largura = g_altura = 0;
    g_apenasAndarAtual = false;
    g_andarFoco = 0;
    g_salasComSuspeitos = 0;
    g_zoomApenasUmAnt = false;
    g_zoomAndarFocoAnt = -99;
    g_pronto = true;
}

void MapaHoloSetFiltro(bool apenasAndarAtual, int andarFoco, uint64_t salasComSuspeitos) {
    g_apenasAndarAtual  = apenasAndarAtual;
    g_andarFoco         = andarFoco;
    g_salasComSuspeitos = salasComSuspeitos;
}

void MapaHoloUnload(void) {
    if (!g_pronto) return;
    if (g_texPronta) {
        UnloadRenderTexture(g_cena);
        UnloadRenderTexture(g_final);
        g_texPronta = false;
    }
    UnloadShader(g_shader);
    g_pronto = false;
}

// Camera de frente e de cima, balancando devagar (o holograma "flutua")
static void PosicionarCamera(float ang, float flutua) {
    if (g_apenasAndarAtual) {
        float yAndar = AlturaAndar(g_andarFoco) + 0.16f;
        Vector3 alvo = { g_plantaW / 2.0f, yAndar, g_plantaD / 2.0f };
        float dist = 8.4f;
        float alt  = 7.2f;
        g_camera.target = alvo;
        g_camera.position = (Vector3){ alvo.x + sinf(ang) * dist, alvo.y + alt + flutua, alvo.z + cosf(ang) * dist };
        return;
    }
    bool showSubsolo = SettingsGetDevMode() && SettingsGetDevUnlockSubsolo();
    float centroY = showSubsolo
        ? g_andarAlt * (g_numAndares - 2) / 2.0f + 0.15f
        : g_andarAlt * (g_numAndares - 1) / 2.0f + 0.15f;
    Vector3 alvo = { g_plantaW / 2.0f, centroY, g_plantaD / 2.0f };
    g_camera.target = alvo;
    g_camera.position = (Vector3){ alvo.x + sinf(ang) * g_camDist, alvo.y + g_camAlt + flutua, alvo.z + cosf(ang) * g_camDist };
}

// Menor fovy em que o predio inteiro (ou o andar focado) cabe na textura (com folga).
static void AjustarZoom(void) {
    PosicionarCamera(0.0f, 0.0f);
    bool showSubsolo = SettingsGetDevMode() && SettingsGetDevUnlockSubsolo();
    float topo = g_apenasAndarAtual ? (AlturaAndar(g_andarFoco) + PAREDE_ALT * 1.4f) : (AlturaAndar(g_numAndares - 1) + ESCADA_ALT);
    float base = g_apenasAndarAtual ? (AlturaAndar(g_andarFoco) - PLACA_ESP) : (showSubsolo ? AlturaAndar(-1) - PLACA_ESP : -PLACA_ESP);
    Vector3 cantos[8];
    for (int i = 0; i < 8; i++) {
        cantos[i] = (Vector3){ (i & 1) ? g_plantaW : 0.0f, (i & 2) ? topo : base, (i & 4) ? g_plantaD : 0.0f };
    }
    float margemX = g_largura * (g_apenasAndarAtual ? 0.05f : 0.08f);
    float margemY = g_altura  * (g_apenasAndarAtual ? 0.07f : 0.08f);
    for (float fovy = 8.0f; fovy <= 80.0f; fovy += 0.5f) {
        g_camera.fovy = fovy;
        bool cabe = true;
        for (int i = 0; i < 8 && cabe; i++) {
            Vector2 p = Projetar(cantos[i]);
            cabe = p.x >= margemX && p.x <= g_largura - margemX && p.y >= margemY && p.y <= g_altura - margemY;
        }
        if (cabe) { g_fovyAjustado = fovy; g_zoomApenasUmAnt = g_apenasAndarAtual; g_zoomAndarFocoAnt = g_andarFoco; return; }
    }
    g_fovyAjustado = 80.0f;
    g_zoomApenasUmAnt = g_apenasAndarAtual;
    g_zoomAndarFocoAnt = g_andarFoco;
}

static void GarantirTexturas(int largura, int altura) {
    if (g_texPronta && g_largura == largura && g_altura == altura) {
        if (g_zoomApenasUmAnt != g_apenasAndarAtual || g_zoomAndarFocoAnt != g_andarFoco) {
            AjustarZoom();
        }
        return;
    }
    if (g_texPronta) {
        UnloadRenderTexture(g_cena);
        UnloadRenderTexture(g_final);
    }
    g_largura = largura;
    g_altura = altura;
    g_cena  = LoadRenderTexture(largura, altura);
    g_final = LoadRenderTexture(largura, altura);
    SetTextureFilter(g_cena.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(g_final.texture, TEXTURE_FILTER_POINT);
    g_texPronta = true;
    AjustarZoom();
}

static void DesenharPlaneRet(int andar, HoloRet r, float y, Color cor) {
    DrawPlane(CentroRet(andar, r, y), (Vector2){ r.w, r.d }, cor);
}

static void DesenharAndar(int andar, bool andarAtual) {
    Color corPlaca = Lum(andarAtual ? LUM_PLACA_ATUAL : LUM_PLACA);
    Color corBorda = Lum(andarAtual ? LUM_PLACA_BORDA_AT : LUM_PLACA_BORDA);
    if (g_modo3D && andar >= 0) {
        if (andar == 0) {
            // Terreo completo (TerreoRASEC3D.png): 0..7.6 x 0..4.0
            Vector3 c = { g_plantaW / 2.0f, AlturaAndar(andar), g_plantaD / 2.0f };
            DrawCube(c, g_plantaW, PLACA_ESP, g_plantaD, corPlaca);
            DrawCubeWires(c, g_plantaW, PLACA_ESP, g_plantaD, corBorda);
        } else {
            // 1o Andar+ (PrimeiroAndarRASEC3D.png): com recorte aberto sobre o Jardim (x=1.70..3.90, z=0..1.95)
            // Bloco esquerdo (Copa + Banheiros)
            Vector3 cEsq = { 0.85f, AlturaAndar(andar), 2.00f };
            DrawCube(cEsq, 1.70f, PLACA_ESP, 4.00f, corPlaca);
            DrawCubeWires(cEsq, 1.70f, PLACA_ESP, 4.00f, corBorda);
            // Bloco central-frente (Hall 1 + Escada, abaixo do jardim)
            Vector3 cMid = { 2.80f, AlturaAndar(andar), 2.975f };
            DrawCube(cMid, 2.20f, PLACA_ESP, 2.05f, corPlaca);
            DrawCubeWires(cMid, 2.20f, PLACA_ESP, 2.05f, corBorda);
            // Bloco direito (Contabilidade 1, 2, Arquivo, Sala de manutencao)
            Vector3 cDir = { 5.75f, AlturaAndar(andar), 2.00f };
            DrawCube(cDir, 3.70f, PLACA_ESP, 4.00f, corPlaca);
            DrawCubeWires(cDir, 3.70f, PLACA_ESP, 4.00f, corBorda);
        }
        return;
    }
    Vector3 c = { g_plantaW / 2.0f, AlturaAndar(andar), g_plantaD / 2.0f };
    DrawCube(c, g_plantaW, PLACA_ESP, g_plantaD, corPlaca);
    DrawCubeWires(c, g_plantaW, PLACA_ESP, g_plantaD, corBorda);
}

static void DesenharCorredor(const HoloCorredor *k, bool andarAtual) {
    float f = andarAtual ? 1.0f : 0.7f;
    DesenharPlaneRet(k->andar, k->r, 0.006f, Lum(LUM_CORREDOR * f));
    // Bordas do corredor (sem paredes: e so piso)
    float y = AlturaAndar(k->andar) + PLACA_ESP / 2.0f + 0.01f;
    HoloRet r = k->r;
    Color borda = Lum(LUM_CORREDOR_BORDA * f);
    DrawLine3D((Vector3){ r.x, y, r.z },       (Vector3){ r.x + r.w, y, r.z },       borda);
    DrawLine3D((Vector3){ r.x, y, r.z + r.d }, (Vector3){ r.x + r.w, y, r.z + r.d }, borda);
}

// Existe escadaria no andar de cima? (no ultimo andar so tem a porta para descer)
static bool TemEscadaAcima(int andar) {
    for (int i = 0; i < g_numSalas; i++)
        if (g_salas[i].escada && g_salas[i].numPartes > 0 && g_salas[i].andar == andar + 1) return true;
    return false;
}

// Patamar com a porta da escadaria em pe no fundo (ultimo andar: nao ha para onde subir)
static void DesenharPortaEscada(int andar, HoloRet r, Color corPorta, Color corPiso) {
    DesenharPlaneRet(andar, r, 0.01f, corPiso);
    float base = AlturaAndar(andar) + PLACA_ESP / 2.0f;
    float z = r.z + 0.35f;
    float portaW = r.w * 0.6f, portaH = ESCADA_ALT * 0.85f;
    float x0 = r.x + (r.w - portaW) / 2.0f, x1 = x0 + portaW;
    DrawCubeWires((Vector3){ (x0 + x1) / 2.0f, base + portaH / 2.0f, z }, portaW, portaH, 0.06f, corPorta);  // batente
    DrawLine3D((Vector3){ x1 - 0.12f, base + portaH * 0.45f, z + 0.04f },
               (Vector3){ x1 - 0.12f, base + portaH * 0.55f, z + 0.04f }, corPorta);                     // macaneta
    // Seta no chao apontando para a porta (por onde se desce)
    float zc = r.z + r.d * 0.65f, xc = r.x + r.w / 2.0f, y = base + 0.02f;
    DrawLine3D((Vector3){ xc, y, zc + 0.5f }, (Vector3){ xc, y, zc - 0.5f }, corPorta);
    DrawLine3D((Vector3){ xc, y, zc - 0.5f }, (Vector3){ xc - 0.2f, y, zc - 0.25f }, corPorta);
    DrawLine3D((Vector3){ xc, y, zc - 0.5f }, (Vector3){ xc + 0.2f, y, zc - 0.25f }, corPorta);
}

static void DesenharEscada(int andar, HoloRet r, Color corDegrau, Color corPiso) {
    DesenharPlaneRet(andar, r, 0.01f, corPiso);
    // Andares pares sobem de frente (z maior) para tras; impares (2o andar) invertem (caracol).
    // Degraus flutuantes (espessura fina) + longarinas laterais para manter leitura limpa de qualquer angulo.
    float base = AlturaAndar(andar) + PLACA_ESP / 2.0f;
    float passo = r.d / ESCADA_DEGRAUS;
    float altDegrau = ESCADA_ALT / ESCADA_DEGRAUS;
    float espDegrau = altDegrau * 0.5f;
    float degrauW = r.w * 0.78f;
    bool invertida = (andar % 2 == 1);

    for (int i = 0; i < ESCADA_DEGRAUS; i++) {
        float yTopo = base + altDegrau * (i + 1);
        float cz = invertida ? (r.z + passo * (i + 0.5f))
                             : (r.z + r.d - passo * (i + 0.5f));
        Vector3 c = { r.x + r.w / 2.0f, yTopo - espDegrau * 0.5f, cz };
        DrawCubeWires(c, degrauW, espDegrau, passo * 0.92f, corDegrau);
    }

    // Longarinas/corrimãos diagonais nas laterais indicando claramente o sentido da subida
    float zIni = invertida ? (r.z + passo * 0.5f) : (r.z + r.d - passo * 0.5f);
    float zFim = invertida ? (r.z + r.d - passo * 0.5f) : (r.z + passo * 0.5f);
    float yIni = base + altDegrau;
    float yFim = base + ESCADA_ALT;
    float corrimaoH = 0.22f;
    float xL = r.x + r.w / 2.0f - degrauW * 0.5f;
    float xR = r.x + r.w / 2.0f + degrauW * 0.5f;
    for (int s = 0; s < 2; s++) {
        float xs = (s == 0) ? xL : xR;
        DrawLine3D((Vector3){ xs, yIni, zIni }, (Vector3){ xs, yFim, zFim }, corDegrau);
        DrawLine3D((Vector3){ xs, yIni + corrimaoH, zIni }, (Vector3){ xs, yFim + corrimaoH, zFim }, corDegrau);
        DrawLine3D((Vector3){ xs, yIni, zIni }, (Vector3){ xs, yIni + corrimaoH, zIni }, corDegrau);
        DrawLine3D((Vector3){ xs, yFim, zFim }, (Vector3){ xs, yFim + corrimaoH, zFim }, corDegrau);
    }
}

static void DesenharSala(int i, bool atual, bool hover, bool acessivel) {
    const HoloSala *s = &g_salas[i];
    float lumPiso   = hover ? LUM_SALA_HOVER : atual ? LUM_SALA_ATUAL : acessivel ? LUM_SALA : LUM_SALA_OFF;
    float lumParede = (atual || hover) ? LUM_PAREDE_DEST : acessivel ? LUM_PAREDE : LUM_PAREDE_OFF;
    if (s->escada) {
        if (TemEscadaAcima(s->andar)) DesenharEscada(s->andar, s->partes[0], Lum(lumParede), Lum(lumPiso));
        else                          DesenharPortaEscada(s->andar, s->partes[0], Lum(lumParede), Lum(lumPiso));
        return;
    }
    float alt = (atual || hover) ? PAREDE_ALT * 1.3f : PAREDE_ALT;
    for (int p = 0; p < s->numPartes; p++) {
        HoloRet r = s->partes[p];
        DesenharPlaneRet(s->andar, r, 0.012f, Lum(lumPiso));
        DrawCubeWires(CentroRet(s->andar, r, alt / 2.0f), r.w, alt, r.d, Lum(lumParede));
    }
}

// Poco da escadaria: linhas verticais ligando as escadas de andares vizinhos
static void DesenharPocoEscada(void) {
    for (int i = 0; i < g_numSalas; i++) {
        if (!g_salas[i].escada || g_salas[i].numPartes == 0) continue;
        for (int j = 0; j < g_numSalas; j++) {
            if (!g_salas[j].escada || g_salas[j].numPartes == 0 || g_salas[j].andar != g_salas[i].andar + 1) continue;
            HoloRet r = g_salas[i].partes[0];
            HoloRet r2 = g_salas[j].partes[0];
            if (fabsf(r.x - r2.x) > 0.25f) continue;
            float y0 = AlturaAndar(g_salas[i].andar), y1 = AlturaAndar(g_salas[j].andar);
            float xs[2] = { r.x, r.x + r.w }, zs[2] = { r.z, r.z + r.d };
            for (int a = 0; a < 2; a++)
                for (int b = 0; b < 2; b++)
                    DrawLine3D((Vector3){ xs[a], y0, zs[b] }, (Vector3){ xs[a], y1, zs[b] }, Lum(LUM_POCO));
        }
    }
}

void MapaHoloRender(int largura, int altura, int salaAtual, int salaHover, uint64_t acessiveis, float tempo) {
    if (!g_pronto || largura <= 0 || altura <= 0) return;
    GarantirTexturas(largura, altura);
    PosicionarCamera(sinf(tempo * 0.15f) * 0.10f, sinf(tempo * 0.4f) * 0.08f);
    g_camera.fovy = g_fovyAjustado;

    int andarAtual = (salaAtual >= 0 && salaAtual < g_numSalas) ? g_salas[salaAtual].andar : -1;

    // 1) Cena 3D crua
    BeginTextureMode(g_cena);
    ClearBackground(BLANK);
    BeginMode3D(g_camera);
    if (!g_apenasAndarAtual) {
        DesenharPocoEscada();
        if (g_modo3D) {
            // Poco vertical do elevador conectando Recepcao <-> Hall 1+ (x=3.35, z=2.35)
            float cxE = 3.35f, czE = 2.35f, ew = 0.42f, ed = 0.36f;
            for (int a = 0; a + 1 < g_numAndares; a++) {
                float y0 = AlturaAndar(a) + PLACA_ESP * 0.5f;
                float y1 = AlturaAndar(a + 1) + PLACA_ESP * 0.5f;
                DrawCube((Vector3){ cxE, (y0 + y1) * 0.5f, czE }, ew, y1 - y0, ed, Lum(LUM_SALA_OFF));
                DrawCubeWires((Vector3){ cxE, (y0 + y1) * 0.5f, czE }, ew, y1 - y0, ed, Lum(LUM_PAREDE_DEST));
            }
        }
    }
    bool showSubsolo = SettingsGetDevMode() && SettingsGetDevUnlockSubsolo();
    int minAndar = g_apenasAndarAtual ? g_andarFoco : (showSubsolo ? -1 : 0);
    int maxAndarLoop = g_apenasAndarAtual ? (g_andarFoco + 1) : g_numAndares;
    for (int a = minAndar; a < maxAndarLoop; a++) DesenharAndar(a, a == andarAtual);
    for (int k = 0; k < g_numCorredores; k++) {
        if (!showSubsolo && g_corredores[k].andar < 0) continue;
        if (g_apenasAndarAtual && g_corredores[k].andar != g_andarFoco) continue;
        DesenharCorredor(&g_corredores[k], g_corredores[k].andar == andarAtual);
    }
    for (int i = 0; i < g_numSalas; i++) {
        if (g_salas[i].numPartes == 0) continue;
        if (!showSubsolo && g_salas[i].andar < 0) continue;
        if (g_apenasAndarAtual && g_salas[i].andar != g_andarFoco) continue;
        bool acessivel = ((acessiveis >> i) & 1ULL) != 0;
        DesenharSala(i, i == salaAtual, i == salaHover && acessivel, acessivel || i == salaAtual);
    }
    EndMode3D();
    EndTextureMode();

    // 2) Dithering + scanlines na rampa da paleta
    BeginTextureMode(g_final);
    ClearBackground(BLANK);
    float alturaF = (float)g_altura;
    SetShaderValue(g_shader, g_locTempo, &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_shader, g_locAltura, &alturaF, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(g_shader);
    DrawTextureRec(g_cena.texture, (Rectangle){ 0, 0, (float)g_largura, -(float)g_altura }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();

    // 3) Marcadores nitidos por cima do dithering: suspeitos (amarelo) + jogador (vermelho)
    for (int i = 0; i < g_numSalas; i++) {
        if (((g_salasComSuspeitos >> i) & 1ULL) == 0) continue;
        if (g_apenasAndarAtual && g_salas[i].andar != g_andarFoco) continue;
        Vector2 ps = MapaHoloPosSala(i);
        ps.x = floorf(ps.x) + 0.5f + (i == salaAtual ? 7.0f : 0.0f);
        ps.y = floorf(ps.y) - 4.0f;
        float pulsoS = fmodf(tempo * 1.4f, 1.0f);
        DrawCircleLinesV(ps, 3.5f + pulsoS * 4.5f, Fade(COR_SUSPEITO, 1.0f - pulsoS));
        DrawCircleV(ps, 3.2f, COR_SUSPEITO);
        DrawCircleLinesV(ps, 3.2f, BLACK);
    }
    if (salaAtual >= 0 && salaAtual < g_numSalas && (!g_apenasAndarAtual || g_salas[salaAtual].andar == g_andarFoco)) {
        Vector2 p = MapaHoloPosSala(salaAtual);
        p.y -= 4.0f;
        p.x = floorf(p.x) + 0.5f;
        p.y = floorf(p.y) + 0.5f;
        float pulso = fmodf(tempo * 1.3f, 1.0f);
        DrawCircleLinesV(p, 4.0f + pulso * 6.5f, Fade(COR_PONTO, 1.0f - pulso));
        DrawCircleV(p, 3.6f, COR_PONTO);
        DrawCircleLinesV(p, 3.6f, WHITE);
    }
    EndTextureMode();
}

Texture2D MapaHoloTextura(void) {
    return g_final.texture;
}

int MapaHoloNumAndares(void) {
    return g_numAndares;
}

int MapaHoloSalaEm(Vector2 ponto) {
    if (!g_pronto || !g_texPronta) return -1;
    // De cima para baixo: o andar de cima e desenhado por ultimo e fica na frente
    int minAndar = (SettingsGetDevMode() && SettingsGetDevUnlockSubsolo()) ? -1 : 0;
    for (int andar = g_numAndares - 1; andar >= minAndar; andar--) {
        for (int i = 0; i < g_numSalas; i++) {
            const HoloSala *s = &g_salas[i];
            if (s->numPartes == 0 || s->andar != andar) continue;
            float alt = s->escada ? ESCADA_ALT : PAREDE_ALT;
            for (int p = 0; p < s->numPartes; p++) {
                HoloRet r = s->partes[p];
                float base = AlturaAndar(andar) + PLACA_ESP / 2.0f;
                float ys[2] = { base, base + alt };
                for (int k = 0; k < 2; k++) {
                    Vector2 quad[4] = {
                        Projetar((Vector3){ r.x,       ys[k], r.z }),
                        Projetar((Vector3){ r.x + r.w, ys[k], r.z }),
                        Projetar((Vector3){ r.x + r.w, ys[k], r.z + r.d }),
                        Projetar((Vector3){ r.x,       ys[k], r.z + r.d }),
                    };
                    if (CheckCollisionPointPoly(ponto, quad, 4)) return i;
                }
            }
        }
    }
    return -1;
}

Vector2 MapaHoloPosSala(int sala) {
    if (sala < 0 || sala >= g_numSalas) return (Vector2){ 0, 0 };
    const HoloSala *s = &g_salas[sala];
    HoloRet r = s->partes[0];   // a primeira parte e a principal (rotulo e bolinha ficam nela)
    return Projetar(CentroRet(s->andar, r, s->escada ? ESCADA_ALT * 0.5f : 0.0f));
}

Vector2 MapaHoloPosAndar(int andar) {
    return Projetar((Vector3){ 0.0f, AlturaAndar(andar) - PLACA_ESP / 2.0f, g_plantaD });
}
