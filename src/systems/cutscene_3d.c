#include "cutscene_3d.h"
#include "terreo_3d.h"
#include "player_character.h"
#include "settings.h"
#include "ui_common.h"
#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const char *CUTSCENE_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "const vec3 TONS[5] = vec3[5](\n"
    "    vec3(0.0),\n"
    "    vec3(32.0, 29.0, 31.0) / 255.0,\n"
    "    vec3(125.0, 112.0, 113.0) / 255.0,\n"
    "    vec3(189.0, 181.0, 168.0) / 255.0,\n"
    "    vec3(255.0, 242.0, 230.0) / 255.0\n"
    ");\n"
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar * 0.92), 0.0, 4.0));\n"
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static const char *STATIC_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform float uTime;\n"
    "out vec4 finalColor;\n"
    "float hash(vec2 p) {\n"
    "    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);\n"
    "}\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float n = hash(floor(uv * vec2(96.0, 54.0)) + vec2(floor(uTime * 22.0), floor(uTime * 15.0)));\n"
    "    float band = smoothstep(0.08, 0.0, abs(fract(uv.y - uTime * 0.45) - 0.5));\n"
    "    float scan = 0.85 + 0.15 * sin(uv.y * 160.0);\n"
    "    float l = clamp((n * 0.72 + band * 0.35 + 0.12) * scan, 0.08, 0.96);\n"
    "    finalColor = vec4(vec3(l), 1.0);\n"
    "}\n";

static const char *CUTSCENE_TEXTO = "Você está pronto para o teste de turing...?";

#define NUM_FEED_SALAS 4

static bool            g_csAtiva = false;
static float           g_csTempo = 0.0f;
static float           g_csTextoProg = 0.0f;
static int             g_csUltimoChar = 0;
static int             g_csBlipCount = 0;

static RenderTexture2D g_csCena = { 0 };
static RenderTexture2D g_csFinal = { 0 };
static RenderTexture2D g_csFeeds[NUM_FEED_SALAS] = { 0 };
static RenderTexture2D g_csStaticTex = { 0 };
static Shader          g_csShader = { 0 };
static Shader          g_csStaticShader = { 0 };
static int             g_csStaticTimeLoc = -1;
static bool            g_csRecursosOk = false;
static int             g_csW = 0;
static int             g_csH = 0;
static int             g_csFeedFrame = 0;
static Sound           g_csVozGrave = { 0 };
static bool            g_csVozPronta = false;

// Sintetiza uma voz sub-grave bem mais grossa que a VOZ_GRAVE do player (~56 Hz fundamental)
static Sound CriarVozSubGrave(void) {
    const int sr = 44100;
    const float dur = 0.074f;
    int nFrames = (int)(sr * dur);
    short *pcm = (short *)MemAlloc((unsigned int)nFrames * sizeof(short));
    if (!pcm) return (Sound){ 0 };

    float fase = 0.0f;
    for (int i = 0; i < nFrames; i++) {
        float t = (float)i / (float)nFrames;
        float freq = 56.0f - 8.0f * t;
        fase += (2.0f * PI * freq) / (float)sr;
        float val = 0.58f * sinf(fase)
                  + 0.26f * sinf(fase * 2.0f)
                  + 0.12f * sinf(fase * 3.0f)
                  + 0.04f * sinf(fase * 4.0f);
        float atk = fminf(1.0f, t / 0.14f);
        float dec = powf(1.0f - t, 1.15f);
        float env = atk * dec * 0.62f;
        int sample = (int)(val * env * 32767.0f);
        if (sample >  32767) sample =  32767;
        if (sample < -32768) sample = -32768;
        pcm[i] = (short)sample;
    }

    Wave w = { 0 };
    w.frameCount = (unsigned int)nFrames;
    w.sampleRate = (unsigned int)sr;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = pcm;
    Sound snd = LoadSoundFromWave(w);
    MemFree(pcm);
    return snd;
}

static void GarantirRecursos(int w, int h) {
    if (w < 320) w = 320;
    if (h < 180) h = 180;
    if (!g_csRecursosOk) {
        g_csShader = LoadShaderFromMemory(NULL, CUTSCENE_FS);
        g_csStaticShader = LoadShaderFromMemory(NULL, STATIC_FS);
        g_csStaticTimeLoc = GetShaderLocation(g_csStaticShader, "uTime");
        for (int i = 0; i < NUM_FEED_SALAS; i++) {
            g_csFeeds[i] = LoadRenderTexture(256, 144);
            SetTextureFilter(g_csFeeds[i].texture, TEXTURE_FILTER_POINT);
        }
        g_csStaticTex = LoadRenderTexture(160, 90);
        SetTextureFilter(g_csStaticTex.texture, TEXTURE_FILTER_POINT);
        g_csRecursosOk = true;
    }
    if (!g_csVozPronta && IsAudioDeviceReady()) {
        g_csVozGrave = CriarVozSubGrave();
        g_csVozPronta = true;
    }
    if (g_csW != w || g_csH != h || g_csCena.id == 0 || g_csFinal.id == 0) {
        if (g_csCena.id > 0) UnloadRenderTexture(g_csCena);
        if (g_csFinal.id > 0) UnloadRenderTexture(g_csFinal);
        g_csCena  = LoadRenderTexture(w, h);
        g_csFinal = LoadRenderTexture(w, h);
        SetTextureFilter(g_csCena.texture, TEXTURE_FILTER_POINT);
        SetTextureFilter(g_csFinal.texture, TEXTURE_FILTER_POINT);
        g_csW = w;
        g_csH = h;
    }
}

// Renderiza uma preview 3D real de um local da empresa para o monitor CFTV
static void RenderizarFeedSala(int slot, float tempo) {
    const TerreoSalaDef *def = NULL;
    switch (slot) {
        case 0: def = SalaJardimDef(); break;
        case 1: def = SalaRecepcaoDef(); break;
        case 2: def = SalaContabilidade1Def(); break;
        case 3: def = SalaServer1Def(); break;
        default: break;
    }
    if (!def || g_csFeeds[slot].id == 0) return;

    // Leve pan de camera de seguranca (CCTV) na sala real
    Camera3D camFeed = def->camera;
    camFeed.position.x += 0.35f * sinf(tempo * 0.35f + (float)slot * 1.7f);

    BeginTextureMode(g_csFeeds[slot]);
    ClearBackground(def->bgColor);
    BeginMode3D(camFeed);
    if (def->drawFunc) {
        def->drawFunc(tempo, false);
    }
    EndMode3D();

    // Sobreposicao leve de scanlines CFTV + indicador REC
    for (int y = 0; y < 144; y += 4) {
        DrawRectangle(0, y, 256, 1, Fade(BLACK, 0.24f));
    }
    DrawRectangleLines(2, 2, 252, 140, Fade(WHITE, 0.28f));
    if (fmodf(tempo + (float)slot * 0.3f, 1.2f) < 0.65f) {
        DrawRectangle(226, 8, 20, 8, (Color){ 245, 240, 235, 230 });
    }
    EndTextureMode();
}

static void DesenharQuadTextura(Texture2D tex, float sw, float sh, float z) {
    float hw = sw * 0.5f;
    float hh = sh * 0.5f;
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlNormal3f(0.0f, 0.0f, 1.0f);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-hw, -hh, z);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f( hw, -hh, z);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f( hw,  hh, z);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-hw,  hh, z);
    rlEnd();
    rlSetTexture(0);
}

// Monitor CFTV / TV industrial detalhada com suporte, moldura profunda, botoes e feed real ou estatica
static void DesenharMonitorTV(Vector3 pos, float rotYDeg, float w, float h, int feedIdx, bool ehParede, float tempo) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Suporte articulado (parede) ou pedestal robusto (mesa)
    if (ehParede) {
        TerreoCaixa((Vector3){ 0.0f, 0.0f, -0.24f }, (Vector3){ w * 0.34f, h * 0.34f, 0.14f }, 0.14f, 0.10f, 0.10f, 0.20f);
        DrawCylinderEx((Vector3){ 0.0f, 0.0f, -0.32f }, (Vector3){ 0.0f, 0.0f, -0.18f }, 0.035f, 0.035f, 8, TerreoLum(0.22f));
    } else {
        TerreoCaixa((Vector3){ 0.0f, -h * 0.5f - 0.06f, -0.06f }, (Vector3){ w * 0.42f, 0.03f, 0.22f }, 0.18f, 0.12f, 0.12f, 0.24f);
        TerreoCaixa((Vector3){ 0.0f, -h * 0.5f - 0.02f, -0.10f }, (Vector3){ 0.09f, 0.08f, 0.08f }, 0.16f, 0.12f, 0.12f, 0.22f);
    }

    // Carcaca traseira afunilada estilo monitor broadcast / CRT
    TerreoCaixa((Vector3){ 0.0f, 0.0f, -0.15f }, (Vector3){ w * 0.80f, h * 0.78f, 0.12f }, 0.13f, 0.09f, 0.10f, 0.20f);
    TerreoCaixa((Vector3){ 0.0f, 0.0f, -0.06f }, (Vector3){ w, h, 0.12f }, 0.16f, 0.11f, 0.12f, 0.24f);

    // Moldura frontal saliente em 4 bordas (recesso real ao redor da tela)
    float sw = w * 0.84f;
    float sh = h * 0.74f;
    float bordaX = (w - sw) * 0.5f;
    float bordaTop = (h - sh) * 0.40f;
    float bordaBot = (h - sh) * 0.60f;
    float telaY = (bordaBot - bordaTop) * 0.5f;

    TerreoCaixa((Vector3){ 0.0f,  h * 0.5f - bordaTop * 0.5f, 0.015f }, (Vector3){ w, bordaTop, 0.035f }, 0.10f, 0.08f, 0.09f, 0.22f);
    TerreoCaixa((Vector3){ 0.0f, -h * 0.5f + bordaBot * 0.5f, 0.015f }, (Vector3){ w, bordaBot, 0.035f }, 0.12f, 0.09f, 0.10f, 0.24f);
    TerreoCaixa((Vector3){ -w * 0.5f + bordaX * 0.5f, telaY, 0.015f }, (Vector3){ bordaX, sh, 0.035f }, 0.10f, 0.08f, 0.09f, 0.22f);
    TerreoCaixa((Vector3){  w * 0.5f - bordaX * 0.5f, telaY, 0.015f }, (Vector3){ bordaX, sh, 0.035f }, 0.10f, 0.08f, 0.09f, 0.22f);

    // Detalhes na borda inferior: grade de som, botoes giratorios e LED de energia
    float botY = -h * 0.5f + bordaBot * 0.48f;
    for (int g = 0; g < 4; g++) {
        TerreoCaixa((Vector3){ -w * 0.34f + (float)g * 0.035f, botY, 0.034f }, (Vector3){ 0.018f, bordaBot * 0.45f, 0.006f }, 0.04f, 0.04f, 0.04f, -1.0f);
    }
    TerreoCaixa((Vector3){ w * 0.24f, botY, 0.036f }, (Vector3){ 0.025f, 0.025f, 0.012f }, 0.36f, 0.30f, 0.30f, -1.0f);
    TerreoCaixa((Vector3){ w * 0.30f, botY, 0.036f }, (Vector3){ 0.025f, 0.025f, 0.012f }, 0.36f, 0.30f, 0.30f, -1.0f);
    float ledPow = (feedIdx < 0 && fmodf(tempo, 0.7f) < 0.35f) ? 0.35f : 0.94f;
    TerreoCaixa((Vector3){ w * 0.38f, botY, 0.035f }, (Vector3){ 0.024f, 0.016f, 0.008f }, ledPow, ledPow, ledPow, -1.0f);

    // Tela: preview real de local da empresa ou estatica animada
    Texture2D tex = (feedIdx >= 0 && feedIdx < NUM_FEED_SALAS && g_csFeeds[feedIdx].id > 0)
                  ? g_csFeeds[feedIdx].texture
                  : g_csStaticTex.texture;
    if (tex.id > 0) {
        rlPushMatrix();
        rlTranslatef(0.0f, telaY, 0.008f);
        DesenharQuadTextura(tex, sw, sh, 0.0f);
        rlPopMatrix();
    } else {
        TerreoCaixa((Vector3){ 0.0f, telaY, 0.008f }, (Vector3){ sw, sh, 0.01f }, 0.65f, 0.65f, 0.65f, -1.0f);
    }

    rlPopMatrix();
}

// Camera de vigilancia montada no alto apontando para a sala
static void DesenharCameraTeto(Vector3 pos, float yawDeg, float pitchDeg, float tempo, int idx) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    rlRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);

    TerreoCaixa((Vector3){ 0.0f, 0.08f, -0.10f }, (Vector3){ 0.06f, 0.16f, 0.06f }, 0.22f, 0.18f, 0.18f, 0.10f);
    TerreoCaixa((Vector3){ 0.0f, 0.00f,  0.02f }, (Vector3){ 0.13f, 0.11f, 0.26f }, 0.26f, 0.20f, 0.22f, 0.12f);
    DrawCylinderEx((Vector3){ 0.0f, 0.0f, 0.15f }, (Vector3){ 0.0f, 0.0f, 0.21f }, 0.045f, 0.045f, 10, TerreoLum(0.08f));
    float led = (fmodf(tempo + (float)idx * 0.4f, 1.2f) < 0.65f) ? 0.96f : 0.20f;
    TerreoCaixa((Vector3){ 0.045f, 0.035f, 0.155f }, (Vector3){ 0.02f, 0.02f, 0.01f }, led, led, led, -1.0f);

    rlPopMatrix();
}

// Cadeira executiva luxuosa de encosto alto (de costas para a camera)
static void DesenharCadeiraExecutiva(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    DrawCylinder((Vector3){ 0.0f, 0.04f, 0.0f }, 0.30f, 0.30f, 0.04f, 10, TerreoLum(0.16f));
    DrawCylinder((Vector3){ 0.0f, 0.08f, 0.0f }, 0.045f, 0.045f, 0.34f, 10, TerreoLum(0.24f));
    TerreoCaixa((Vector3){ 0.0f, 0.44f, 0.02f }, (Vector3){ 0.60f, 0.11f, 0.56f }, 0.18f, 0.12f, 0.14f, 0.26f);
    TerreoCaixa((Vector3){ -0.32f, 0.64f, 0.02f }, (Vector3){ 0.07f, 0.06f, 0.44f }, 0.20f, 0.14f, 0.15f, 0.28f);
    TerreoCaixa((Vector3){  0.32f, 0.64f, 0.02f }, (Vector3){ 0.07f, 0.06f, 0.44f }, 0.20f, 0.14f, 0.15f, 0.28f);
    TerreoCaixa((Vector3){ 0.0f, 1.02f, -0.24f }, (Vector3){ 0.58f, 1.06f, 0.10f }, 0.16f, 0.11f, 0.13f, 0.28f);
    TerreoCaixa((Vector3){ 0.0f, 1.50f, -0.22f }, (Vector3){ 0.42f, 0.24f, 0.11f }, 0.20f, 0.14f, 0.16f, 0.30f);

    rlPopMatrix();
}

static void DesenharCenaCutscene(float tempo) {
    // Sala luxuosa escura (painelamento escuro, piso polido, tapete, mesa executiva e video wall)
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 9.0f, 9.0f }, TerreoLum(0.10f));
    Color corPiso = TerreoLum(0.15f);
    for (float x = -4.0f; x <= 4.01f; x += 0.80f) {
        DrawLine3D((Vector3){ x, 0.003f, -4.0f }, (Vector3){ x, 0.003f, 4.0f }, corPiso);
    }
    for (float z = -4.0f; z <= 4.01f; z += 0.80f) {
        DrawLine3D((Vector3){ -4.0f, 0.003f, z }, (Vector3){ 4.0f, 0.003f, z }, corPiso);
    }

    // Tapete escuro luxuoso no centro da sala + reflexo frio dos monitores
    DrawPlane((Vector3){ 0.0f, 0.005f, -0.35f }, (Vector2){ 4.20f, 3.20f }, TerreoLum(0.06f));
    DrawPlane((Vector3){ 0.0f, 0.006f, -1.10f }, (Vector2){ 3.40f, 1.10f }, TerreoLum(0.19f));

    // Paredes altas escuras com ripas de madeira/marmore e vigas
    TerreoCaixa((Vector3){  0.0f, 1.65f, -3.55f }, (Vector3){ 8.40f, 3.30f, 0.20f }, 0.09f, 0.11f, 0.08f, 0.05f);
    TerreoCaixa((Vector3){ -4.15f, 1.65f, 0.0f },  (Vector3){ 0.20f, 3.30f, 7.60f }, 0.09f, 0.10f, 0.11f, 0.05f);
    TerreoCaixa((Vector3){  4.15f, 1.65f, 0.0f },  (Vector3){ 0.20f, 3.30f, 7.60f }, 0.09f, 0.10f, 0.11f, 0.05f);
    for (int i = -3; i <= 3; i++) {
        if (i == 0) continue;
        TerreoCaixa((Vector3){ (float)i * 1.15f, 1.65f, -3.42f }, (Vector3){ 0.14f, 3.25f, 0.08f }, 0.16f, 0.14f, 0.12f, 0.08f);
    }
    TerreoCaixa((Vector3){ 0.0f, 3.05f, -2.00f }, (Vector3){ 8.40f, 0.28f, 0.45f }, 0.12f, 0.08f, 0.08f, 0.06f);
    TerreoCaixa((Vector3){ 0.0f, 3.05f,  0.20f }, (Vector3){ 8.40f, 0.28f, 0.45f }, 0.12f, 0.08f, 0.08f, 0.06f);

    // Estantes executivas e racks escuros nas laterais
    TerreoArmario((Vector3){ -3.55f, 0.0f, -2.20f },  45.0f, 1.25f, 2.35f, 0.58f);
    TerreoArmario((Vector3){  3.55f, 0.0f, -2.20f }, -45.0f, 1.25f, 2.35f, 0.58f);
    TerreoRackServidor((Vector3){ -3.65f, 0.0f, -0.40f },  90.0f);
    TerreoRackServidor((Vector3){  3.65f, 0.0f, -0.40f }, -90.0f);

    // Mesa executiva imponente escura
    TerreoCaixa((Vector3){ 0.0f, 0.76f, -1.55f }, (Vector3){ 3.50f, 0.08f, 1.05f }, 0.22f, 0.14f, 0.12f, 0.24f);
    TerreoCaixa((Vector3){ 0.0f, 0.37f, -1.92f }, (Vector3){ 3.30f, 0.72f, 0.18f }, 0.12f, 0.09f, 0.08f, 0.14f);
    TerreoCaixa((Vector3){ -1.55f, 0.37f, -1.55f }, (Vector3){ 0.18f, 0.72f, 0.90f }, 0.14f, 0.10f, 0.10f, 0.16f);
    TerreoCaixa((Vector3){  1.55f, 0.37f, -1.55f }, (Vector3){ 0.18f, 0.72f, 0.90f }, 0.14f, 0.10f, 0.10f, 0.16f);

    // Monitores na mesa (Jardim no centro, Recepcao e estatica nas laterais)
    DesenharMonitorTV((Vector3){ -1.12f, 1.07f, -1.68f },  16.0f, 0.68f, 0.46f,  1, false, tempo);
    DesenharMonitorTV((Vector3){  0.00f, 1.08f, -1.78f },   0.0f, 0.76f, 0.50f,  0, false, tempo);
    DesenharMonitorTV((Vector3){  1.12f, 1.07f, -1.68f }, -16.0f, 0.68f, 0.46f, -1, false, tempo);

    // Grande Video Wall no fundo + asas laterais (0=Jardim, 1=Recepcao, 2=Contabilidade, 3=Servidores, -1=Estatica)
    TerreoCaixa((Vector3){ 0.0f, 2.02f, -3.32f }, (Vector3){ 5.60f, 1.65f, 0.10f }, 0.08f, 0.06f, 0.06f, 0.16f);
    DesenharMonitorTV((Vector3){  0.00f, 2.04f, -3.10f },   0.0f, 1.56f, 0.98f,  0, true, tempo);
    DesenharMonitorTV((Vector3){ -1.42f, 2.34f, -3.06f },   8.0f, 1.02f, 0.58f,  1, true, tempo);
    DesenharMonitorTV((Vector3){ -1.42f, 1.68f, -3.06f },   8.0f, 1.02f, 0.58f, -1, true, tempo);
    DesenharMonitorTV((Vector3){  1.42f, 2.34f, -3.06f },  -8.0f, 1.02f, 0.58f,  2, true, tempo);
    DesenharMonitorTV((Vector3){  1.42f, 1.68f, -3.06f },  -8.0f, 1.02f, 0.58f,  3, true, tempo);
    DesenharMonitorTV((Vector3){ -2.52f, 2.30f, -2.80f },  24.0f, 0.92f, 0.56f, -1, true, tempo);
    DesenharMonitorTV((Vector3){ -2.52f, 1.64f, -2.80f },  24.0f, 0.92f, 0.56f,  3, true, tempo);
    DesenharMonitorTV((Vector3){  2.52f, 2.30f, -2.80f }, -24.0f, 0.92f, 0.56f,  1, true, tempo);
    DesenharMonitorTV((Vector3){  2.52f, 1.64f, -2.80f }, -24.0f, 0.92f, 0.56f, -1, true, tempo);

    // Cameras de vigilancia montadas no alto vigiando a sala
    DesenharCameraTeto((Vector3){ -2.95f, 2.82f, -1.90f },  35.0f, 26.0f, tempo, 0);
    DesenharCameraTeto((Vector3){  2.95f, 2.82f, -1.90f }, -35.0f, 26.0f, tempo, 1);
    DesenharCameraTeto((Vector3){ -3.45f, 2.75f,  0.60f },  65.0f, 22.0f, tempo, 2);
    DesenharCameraTeto((Vector3){  3.45f, 2.75f,  0.60f }, -65.0f, 22.0f, tempo, 3);

    // Pessoa sentada na cadeira executiva de costas para o jogador (olhando para os monitores)
    Vector3 posPessoa = { 0.0f, 0.0f, -0.58f };
    Vector3 camRefPos = { 0.0f, 1.62f, 2.10f };
    PlayerCharacterDrawNpc3D(posPessoa, 180.0f, true, tempo, camRefPos);
    DesenharCadeiraExecutiva(posPessoa, 180.0f);
}

void Cutscene3DStart(void) {
    g_csAtiva = true;
    g_csTempo = 0.0f;
    g_csTextoProg = 0.0f;
    g_csUltimoChar = 0;
    g_csBlipCount = 0;
    g_csFeedFrame = 0;
}

void Cutscene3DStop(void) {
    g_csAtiva = false;
}

bool Cutscene3DIsActive(void) {
    return g_csAtiva;
}

void Cutscene3DUpdateAndRender(float dt) {
    if (!g_csAtiva) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;
    g_csTempo += dt;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    GarantirRecursos(sw, sh);

    // Avanco do texto estilo typewriter + voz sub-grave
    int totalLen = (int)strlen(CUTSCENE_TEXTO);
    if (g_csTempo > 0.75f && g_csTextoProg < (float)totalLen) {
        g_csTextoProg += dt * 16.0f;
        if (g_csTextoProg > (float)totalLen) g_csTextoProg = (float)totalLen;
        int curChars = (int)g_csTextoProg;
        if (curChars > g_csUltimoChar) {
            g_csUltimoChar = curChars;
            unsigned char ch = (unsigned char)CUTSCENE_TEXTO[curChars - 1];
            if (ch != ' ' && ch != '.' && ch != '?') {
                g_csBlipCount++;
                if ((g_csBlipCount % 2) == 1 && g_csVozPronta && g_csVozGrave.frameCount > 0) {
                    float pitch = 0.87f + ((float)GetRandomValue(-2, 3) / 100.0f);
                    SetSoundVolume(g_csVozGrave, 0.55f * SettingsGetSfxVolume());
                    SetSoundPitch(g_csVozGrave, pitch);
                    PlaySound(g_csVozGrave);
                }
            }
        }
    }

    // Entrada para pular/fechar a cutscene
    bool terminouTexto = (g_csTextoProg >= (float)totalLen);
    if (IsKeyPressed(KEY_ESCAPE)) {
        g_csAtiva = false;
        return;
    }
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!terminouTexto) {
            g_csTextoProg = (float)totalLen;
            g_csUltimoChar = totalLen;
        } else {
            g_csAtiva = false;
            return;
        }
    }

    // Atualiza os feeds reais das salas (todos no 1o frame, depois 1 por frame para manter fluidez)
    if (g_csFeedFrame == 0) {
        for (int i = 0; i < NUM_FEED_SALAS; i++) RenderizarFeedSala(i, g_csTempo);
    } else {
        RenderizarFeedSala(g_csFeedFrame % NUM_FEED_SALAS, g_csTempo);
    }
    g_csFeedFrame++;

    // Atualiza textura de estatica animada
    if (g_csStaticTex.id > 0 && g_csStaticShader.id > 0) {
        if (g_csStaticTimeLoc >= 0) SetShaderValue(g_csStaticShader, g_csStaticTimeLoc, &g_csTempo, SHADER_UNIFORM_FLOAT);
        BeginTextureMode(g_csStaticTex);
        ClearBackground(BLACK);
        BeginShaderMode(g_csStaticShader);
        DrawRectangle(0, 0, g_csStaticTex.texture.width, g_csStaticTex.texture.height, WHITE);
        EndShaderMode();
        EndTextureMode();
    }

    // Camera em primeira pessoa aproximando-se lentamente com leve respiracao
    float avanco = fminf(1.0f, g_csTempo / 7.0f);
    float smooth = avanco * avanco * (3.0f - 2.0f * avanco);
    float camZ = 2.90f - 0.95f * smooth;
    float camY = 1.62f + 0.015f * sinf(g_csTempo * 1.4f);
    float camX = 0.03f * sinf(g_csTempo * 0.65f);

    Camera3D cam = {
        .position   = { camX, camY, camZ },
        .target     = { 0.0f, 1.32f + 0.01f * cosf(g_csTempo * 1.1f), -1.75f },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = 48.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    BeginTextureMode(g_csCena);
    ClearBackground((Color){ 6, 5, 6, 255 });
    BeginMode3D(cam);
    DesenharCenaCutscene(g_csTempo);
    EndMode3D();
    EndTextureMode();

    BeginTextureMode(g_csFinal);
    ClearBackground(BLACK);
    if (g_csRecursosOk && g_csShader.id > 0) {
        BeginShaderMode(g_csShader);
        DrawTextureRec(g_csCena.texture,
                       (Rectangle){ 0, 0, (float)g_csCena.texture.width, -(float)g_csCena.texture.height },
                       (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
    } else {
        DrawTextureRec(g_csCena.texture,
                       (Rectangle){ 0, 0, (float)g_csCena.texture.width, -(float)g_csCena.texture.height },
                       (Vector2){ 0, 0 }, WHITE);
    }
    EndTextureMode();
}

void Cutscene3DDrawOverlay(void) {
    if (!g_csAtiva || g_csFinal.id == 0) return;

    Rectangle tela = UIGetTelaRect();
    DrawTexturePro(g_csFinal.texture,
                   (Rectangle){ 0, 0, (float)g_csFinal.texture.width, -(float)g_csFinal.texture.height },
                   tela, (Vector2){ 0, 0 }, 0.0f, WHITE);

    // Vinheta escura sutil nas bordas para clima claustrofobico
    DrawRectangleRec(UIParaTela((Rectangle){ 0, 0, UI_VIRTUAL_W, 24 }), Fade(BLACK, 0.55f));
    DrawRectangleRec(UIParaTela((Rectangle){ 0, UI_VIRTUAL_H - 108, UI_VIRTUAL_W, 108 }), Fade(BLACK, 0.45f));

    // Caixa de dialogo destacada em toda a cutscene (sem opcoes de fala)
    Rectangle rBox = { 38.0f, 246.0f, 564.0f, 92.0f };
    DrawRectangleRec(UIParaTela((Rectangle){ rBox.x + 3, rBox.y + 3, rBox.width, rBox.height }), Fade(BLACK, 0.72f));
    DrawRectangleRec(UIParaTela(rBox), (Color){ 18, 16, 18, 246 });
    DrawRectangleLinesEx(UIParaTela(rBox), 2.2f, (Color){ 255, 242, 230, 240 });

    // Etiqueta discreta no topo da caixa
    Rectangle rTag = { 54.0f, 236.0f, 118.0f, 18.0f };
    DrawRectangleRec(UIParaTela(rTag), (Color){ 32, 29, 31, 255 });
    DrawRectangleLinesEx(UIParaTela(rTag), 1.4f, (Color){ 189, 181, 168, 255 });
    UIDrawTextInArea("VOZ DESCONHECIDA", rTag, 7.5f, (Color){ 255, 242, 230, 255 });

    // Texto principal revelado progressivamente (UTF-8 safe: copia bytes completos)
    int len = (int)strlen(CUTSCENE_TEXTO);
    int visBytes = (int)g_csTextoProg;
    if (visBytes > len) visBytes = len;
    while (visBytes > 0 && visBytes < len && ((unsigned char)CUTSCENE_TEXTO[visBytes] & 0xC0) == 0x80) {
        visBytes++;
    }
    char buf[128];
    if (visBytes > (int)sizeof(buf) - 1) visBytes = (int)sizeof(buf) - 1;
    memcpy(buf, CUTSCENE_TEXTO, (size_t)visBytes);
    buf[visBytes] = '\0';

    UIDrawTextWrappedLeftAligned(buf, (Vector2){ rBox.x + 20.0f, rBox.y + 22.0f }, rBox.width - 40.0f, 15, (Color){ 255, 242, 230, 255 });

    // Dica discreta para continuar/sair
    const char *hint = (g_csTextoProg >= (float)len)
                     ? "[ESPAÇO / ENTER / CLIQUE] Continuar"
                     : "[ESC] Sair";
    UIDrawTextInArea(hint, (Rectangle){ rBox.x + rBox.width - 210.0f, rBox.y + rBox.height - 20.0f, 196.0f, 16.0f },
                     7.5f, (Color){ 189, 181, 168, 220 });
}

void Cutscene3DUnload(void) {
    g_csAtiva = false;
    if (g_csCena.id > 0) { UnloadRenderTexture(g_csCena); g_csCena = (RenderTexture2D){ 0 }; }
    if (g_csFinal.id > 0) { UnloadRenderTexture(g_csFinal); g_csFinal = (RenderTexture2D){ 0 }; }
    for (int i = 0; i < NUM_FEED_SALAS; i++) {
        if (g_csFeeds[i].id > 0) { UnloadRenderTexture(g_csFeeds[i]); g_csFeeds[i] = (RenderTexture2D){ 0 }; }
    }
    if (g_csStaticTex.id > 0) { UnloadRenderTexture(g_csStaticTex); g_csStaticTex = (RenderTexture2D){ 0 }; }
    UISafeUnloadShader(&g_csShader);
    UISafeUnloadShader(&g_csStaticShader);
    g_csStaticTimeLoc = -1;
    g_csRecursosOk = false;
    if (g_csVozPronta) {
        UISafeUnloadSound(&g_csVozGrave);
        g_csVozPronta = false;
    }
    g_csW = 0;
    g_csH = 0;
}
