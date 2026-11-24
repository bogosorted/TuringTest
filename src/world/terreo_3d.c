#include "terreo_3d.h"
#include "game_state.h"
#include "settings.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const char *TERREO_FS =
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
    "const vec3 AMARELO[4] = vec3[4](\n"
    "    vec3(145.0, 108.0, 10.0) / 255.0,\n"
    "    vec3(220.0, 174.0, 22.0) / 255.0,\n"
    "    vec3(255.0, 224.0, 48.0) / 255.0,\n"
    "    vec3(255.0, 248.0, 150.0) / 255.0\n"
    ");\n"
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    // Preserva tinta amarela brilhante (estilo objetivo/evidencia Resident Evil)\n"
    "    if (c.r > 0.70 && c.g > 0.55 && c.b < 0.34 && (c.r - c.b) > 0.42) {\n"
    "        float yl = clamp((c.r * 0.55 + c.g * 0.45), 0.0, 1.0);\n"
    "        int yidx = int(clamp(floor(yl * 3.0 + 0.5 + limiar * 0.65), 0.0, 3.0));\n"
    "        finalColor = vec4(AMARELO[yidx], 1.0);\n"
    "        return;\n"
    "    }\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar * 0.92), 0.0, 4.0));\n"
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static RenderTexture2D      g_cena = { 0 };
static RenderTexture2D      g_final = { 0 };
static bool                 g_texPronta = false;
static int                  g_largura = 0;
static int                  g_altura = 0;
static int                  s_mousePendTipo = 0; // 0=nenhum, 1=obj, 2=porta, 3=suspeito
static int                  s_mousePendIdx  = -1;
static TerreoDestino        s_mousePendDest = TERREO_DEST_NENHUM;
static Vector3              s_mousePendPos  = { 0 };
static Vector3              s_pathPts[32];
static int                  s_pathCount = 0;
static int                  s_pathIdx   = 0;

static Sound                g_sndPasso3D = { 0 };
static Sound                g_sndPorta3D = { 0 };
static float                g_passoTimer = 0.0f;
static float                g_portaAnimTimer = 0.0f;
static TerreoDestino        g_portaDestPendente = TERREO_DEST_NENHUM;
static Vector3              g_portaAlvoPos = { 0 };
static bool                 g_guiaTemAlgoNoAndar = true;
static uint64_t             g_guiaSalasAlvoMask  = 0ULL;

static Sound CriarSomProcedural3D(int sampleCount, float freqIni, float freqFim, float noiseMix, float amp) {
    Wave w = { 0 };
    w.frameCount = (unsigned int)sampleCount;
    w.sampleRate = 22050;
    w.sampleSize = 16;
    w.channels   = 1;
    short *data = (short *)RL_MALLOC((size_t)sampleCount * sizeof(short));
    if (!data) return (Sound){ 0 };
    float phase = 0.0f;
    unsigned int seed = 1337u;
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / (float)sampleCount;
        float env = (t < 0.08f) ? (t / 0.08f) : expf(-5.5f * (t - 0.08f));
        float freq = freqIni + (freqFim - freqIni) * t;
        phase += 2.0f * PI * freq / 22050.0f;
        seed = seed * 1664525u + 1013904223u;
        float rnd = ((float)((seed >> 16) & 0xFFFF) / 32767.5f) - 1.0f;
        float s = (sinf(phase) * (1.0f - noiseMix) + rnd * noiseMix) * env * amp;
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        data[i] = (short)(s * 32767.0f);
    }
    w.data = data;
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

static bool EvidenciaDisponivelPorId(const char *id) {
    if (!id) return true;
    if (strcmp(id, "ev_maria_queixa") == 0)     return GameStateEvidenciaDisponivelHoje(0, "EV-01");
    if (strcmp(id, "ev_maria_devolvidos") == 0) return GameStateEvidenciaDisponivelHoje(0, "EV-02");
    if (strcmp(id, "ev_arthur_sobrecarga") == 0)return GameStateEvidenciaDisponivelHoje(1, "EV-03");
    if (strcmp(id, "ev_arthur_demissao") == 0)  return GameStateEvidenciaDisponivelHoje(1, "EV-04");
    if (strcmp(id, "ev_analu_terminal") == 0)   return GameStateEvidenciaDisponivelHoje(2, "EV-05");
    if (strcmp(id, "ev_jardim_sombras") == 0)   return GameStateEvidenciaDisponivelHoje(2, "EV-06");
    return true;
}

static bool PontoLivreGrid(Rectangle bounds, const PlayerColliderBox *cols, int numCols, float wx, float wz) {
    float margin = 0.28f;
    if (wx < bounds.x + margin || wx > bounds.x + bounds.width - margin) return false;
    if (wz < bounds.y + margin || wz > bounds.y + bounds.height - margin) return false;
    for (int i = 0; i < numCols; i++) {
        if (wx >= cols[i].minX - margin && wx <= cols[i].maxX + margin &&
            wz >= cols[i].minZ - margin && wz <= cols[i].maxZ + margin) {
            return false;
        }
    }
    return true;
}

static int CalcularCaminhoGrid(const TerreoSalaDef *def, const PlayerColliderBox *cols, int numCols,
                               Vector3 start, Vector3 goal, Vector3 *outPts, int maxPts) {
    if (!def || !outPts || maxPts <= 0) return 0;
    const float cell = 0.35f;
    int gw = (int)ceilf(def->boundsXZ.width / cell);
    int gh = (int)ceilf(def->boundsXZ.height / cell);
    if (gw < 2) gw = 2; if (gw > 48) gw = 48;
    if (gh < 2) gh = 2; if (gh > 48) gh = 48;

    bool blocked[48 * 48];
    int parent[48 * 48];
    int queue[48 * 48];
    for (int z = 0; z < gh; z++) {
        for (int x = 0; x < gw; x++) {
            float wx = def->boundsXZ.x + (x + 0.5f) * cell;
            float wz = def->boundsXZ.y + (z + 0.5f) * cell;
            int idx = z * gw + x;
            blocked[idx] = !PontoLivreGrid(def->boundsXZ, cols, numCols, wx, wz);
            parent[idx] = -2;
        }
    }

    int sx = (int)Clamp((start.x - def->boundsXZ.x) / cell, 0, gw - 1);
    int sz = (int)Clamp((start.z - def->boundsXZ.y) / cell, 0, gh - 1);
    int gx = (int)Clamp((goal.x - def->boundsXZ.x) / cell, 0, gw - 1);
    int gz = (int)Clamp((goal.z - def->boundsXZ.y) / cell, 0, gh - 1);

    // Se a celula inicial estiver na margem de um colider, pega a vizinha livre mais proxima
    if (blocked[sz * gw + sx]) {
        float bestD = 1e9f;
        int bsx = sx, bsz = sz;
        for (int z = 0; z < gh; z++) {
            for (int x = 0; x < gw; x++) {
                if (blocked[z * gw + x]) continue;
                float d = (float)((x - sx) * (x - sx) + (z - sz) * (z - sz));
                if (d < bestD) { bestD = d; bsx = x; bsz = z; }
            }
        }
        sx = bsx; sz = bsz;
    }

    // Se o destino clicado estiver dentro de um movel/parede, escolhe a celula livre mais proxima ao destino (levemente ponderada para o lado do player)
    if (blocked[gz * gw + gx]) {
        float bestScore = 1e9f;
        int bgx = gx, bgz = gz;
        for (int z = 0; z < gh; z++) {
            for (int x = 0; x < gw; x++) {
                if (blocked[z * gw + x]) continue;
                float wx = def->boundsXZ.x + (x + 0.5f) * cell;
                float wz = def->boundsXZ.y + (z + 0.5f) * cell;
                float dGoal = hypotf(wx - goal.x, wz - goal.z);
                float dStart = hypotf(wx - start.x, wz - start.z);
                float score = dGoal + 0.12f * dStart;
                if (score < bestScore) { bestScore = score; bgx = x; bgz = z; }
            }
        }
        gx = bgx; gz = bgz;
    }

    int startIdx = sz * gw + sx;
    int goalIdx  = gz * gw + gx;
    if (startIdx == goalIdx) {
        outPts[0] = (Vector3){ def->boundsXZ.x + (gx + 0.5f) * cell, 0.0f, def->boundsXZ.y + (gz + 0.5f) * cell };
        return 1;
    }

    int head = 0, tail = 0;
    queue[tail++] = startIdx;
    parent[startIdx] = -1;
    bool found = false;
    static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    static const int DZ[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

    while (head < tail) {
        int cur = queue[head++];
        if (cur == goalIdx) { found = true; break; }
        int cx = cur % gw;
        int cz = cur / gw;
        for (int k = 0; k < 8; k++) {
            int nx = cx + DX[k];
            int nz = cz + DZ[k];
            if (nx < 0 || nx >= gw || nz < 0 || nz >= gh) continue;
            int nIdx = nz * gw + nx;
            if (blocked[nIdx] || parent[nIdx] != -2) continue;
            // Evita cortar quinas de moveis na diagonal
            if (k >= 4 && (blocked[cz * gw + nx] || blocked[nz * gw + cx])) continue;
            parent[nIdx] = cur;
            queue[tail++] = nIdx;
        }
    }

    if (!found) return 0;

    int rev[128];
    int len = 0;
    for (int cur = goalIdx; cur != -1 && cur != startIdx && len < 128; cur = parent[cur]) {
        rev[len++] = cur;
    }
    int outCount = 0;
    int step = (len > maxPts) ? ((len + maxPts - 1) / maxPts) : 1;
    for (int i = len - 1; i >= 0 && outCount < maxPts; i -= step) {
        int idx = rev[i];
        int x = idx % gw;
        int z = idx / gw;
        outPts[outCount++] = (Vector3){ def->boundsXZ.x + (x + 0.5f) * cell, 0.0f, def->boundsXZ.y + (z + 0.5f) * cell };
    }
    if (outCount > 0 && (rev[0] != -1)) {
        int x = goalIdx % gw;
        int z = goalIdx / gw;
        outPts[outCount - 1] = (Vector3){ def->boundsXZ.x + (x + 0.5f) * cell, 0.0f, def->boundsXZ.y + (z + 0.5f) * cell };
    }
    return outCount;
}
static Shader               g_shader = { 0 };
static bool                 g_pronto = false;

static const TerreoSalaDef *g_salas[TERREO_NUM_SALAS] = { 0 };
static TerreoSalaId         g_salaAtual = TERREO_SALA_RECEPCAO;
static PlayerCharacter      g_player = { 0 };
static Camera3D             g_camAtual = { 0 };
static float                g_fpYaw = 0.0f;
static float                g_fpPitch = 0.0f;
static bool                 g_fpPrevAtivo = false;
static bool                 g_evRelatoriosAtiva = false;
static bool                 g_suspeitosNaSala[3] = { false, false, false };
static float                g_cooldownEntrada = 0.0f;
static bool                 g_mouseSobreInteragivel = false;
static bool                 g_hasProxInteragivel = false;
static bool                 g_hasClosestMouseTarget = false;
static Vector3              g_closestMouseTargetPos = { 0 };
static char                 g_dicaAcao[160] = { 0 };

Color TerreoLum(float l) {
    unsigned char v = (unsigned char)(Clamp(l, 0.0f, 1.0f) * 255.0f);
    return (Color){ v, v, v, 255 };
}

void TerreoCaixa(Vector3 centro, Vector3 tam, float topLum, float frontLum, float sideLum, float wireLum) {
    float hx = tam.x * 0.5f;
    float hy = tam.y * 0.5f;
    float hz = tam.z * 0.5f;
    float x0 = centro.x - hx, x1 = centro.x + hx;
    float y0 = centro.y - hy, y1 = centro.y + hy;
    float z0 = centro.z - hz, z1 = centro.z + hz;

    Color cTop  = TerreoLum(topLum);
    Color cBot  = TerreoLum(topLum * 0.55f);
    Color cFnt  = TerreoLum(frontLum);
    Color cBack = TerreoLum(frontLum * 0.88f);
    Color cSide = TerreoLum(sideLum);

    DrawTriangle3D((Vector3){ x0, y1, z0 }, (Vector3){ x0, y1, z1 }, (Vector3){ x1, y1, z1 }, cTop);
    DrawTriangle3D((Vector3){ x0, y1, z0 }, (Vector3){ x1, y1, z1 }, (Vector3){ x1, y1, z0 }, cTop);
    DrawTriangle3D((Vector3){ x0, y0, z0 }, (Vector3){ x1, y0, z0 }, (Vector3){ x1, y0, z1 }, cBot);
    DrawTriangle3D((Vector3){ x0, y0, z0 }, (Vector3){ x1, y0, z1 }, (Vector3){ x0, y0, z1 }, cBot);
    DrawTriangle3D((Vector3){ x0, y0, z1 }, (Vector3){ x1, y0, z1 }, (Vector3){ x1, y1, z1 }, cFnt);
    DrawTriangle3D((Vector3){ x0, y0, z1 }, (Vector3){ x1, y1, z1 }, (Vector3){ x0, y1, z1 }, cFnt);
    DrawTriangle3D((Vector3){ x1, y0, z0 }, (Vector3){ x0, y0, z0 }, (Vector3){ x0, y1, z0 }, cBack);
    DrawTriangle3D((Vector3){ x1, y0, z0 }, (Vector3){ x0, y1, z0 }, (Vector3){ x1, y1, z0 }, cBack);
    DrawTriangle3D((Vector3){ x1, y0, z1 }, (Vector3){ x1, y0, z0 }, (Vector3){ x1, y1, z0 }, cSide);
    DrawTriangle3D((Vector3){ x1, y0, z1 }, (Vector3){ x1, y1, z0 }, (Vector3){ x1, y1, z1 }, cSide);
    DrawTriangle3D((Vector3){ x0, y0, z0 }, (Vector3){ x0, y0, z1 }, (Vector3){ x0, y1, z1 }, cSide);
    DrawTriangle3D((Vector3){ x0, y0, z0 }, (Vector3){ x0, y1, z1 }, (Vector3){ x0, y1, z0 }, cSide);

    if (wireLum >= 0.0f) {
        DrawCubeWires(centro, tam.x, tam.y, tam.z, TerreoLum(wireLum));
    }
}

void TerreoPortaModelo(Vector3 pos, float rotYDeg, TerreoPortaTipo tipo) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    if (tipo == PORTA_TIPO_VIDRO) {
        TerreoCaixa((Vector3){ -0.62f, 1.12f, 0.0f }, (Vector3){ 0.07f, 2.24f, 0.12f }, 0.34f, 0.28f, 0.26f, 0.10f);
        TerreoCaixa((Vector3){  0.62f, 1.12f, 0.0f }, (Vector3){ 0.07f, 2.24f, 0.12f }, 0.34f, 0.28f, 0.26f, 0.10f);
        TerreoCaixa((Vector3){  0.00f, 2.21f, 0.0f }, (Vector3){ 1.32f, 0.08f, 0.12f }, 0.34f, 0.28f, 0.26f, 0.10f);
        TerreoCaixa((Vector3){  0.00f, 1.10f, 0.0f }, (Vector3){ 0.04f, 2.16f, 0.06f }, 0.28f, 0.22f, 0.22f, -1.0f);
        TerreoCaixa((Vector3){  0.00f, 0.88f, 0.0f }, (Vector3){ 1.18f, 0.12f, 0.02f }, 0.86f, 0.82f, 0.78f, -1.0f);
        TerreoCaixa((Vector3){  0.00f, 1.02f, 0.0f }, (Vector3){ 1.18f, 0.03f, 0.02f }, 0.86f, 0.82f, 0.78f, -1.0f);
        DrawLine3D((Vector3){ -0.45f, 1.30f, 0.02f }, (Vector3){ -0.15f, 1.95f, 0.02f }, TerreoLum(0.75f));
        DrawLine3D((Vector3){  0.15f, 1.25f, 0.02f }, (Vector3){  0.42f, 1.85f, 0.02f }, TerreoLum(0.75f));
        DrawCylinderEx((Vector3){ -0.12f, 0.82f, 0.06f }, (Vector3){ -0.12f, 1.32f, 0.06f }, 0.022f, 0.022f, 8, TerreoLum(0.90f));
        DrawCylinderEx((Vector3){  0.12f, 0.82f, 0.06f }, (Vector3){  0.12f, 1.32f, 0.06f }, 0.022f, 0.022f, 8, TerreoLum(0.90f));
    } else if (tipo == PORTA_TIPO_INDUSTRIAL) {
        TerreoCaixa((Vector3){ 0.0f, 1.12f, 0.0f }, (Vector3){ 1.18f, 2.24f, 0.14f }, 0.34f, 0.28f, 0.26f, 0.08f);
        TerreoCaixa((Vector3){ 0.0f, 1.09f, 0.02f }, (Vector3){ 1.04f, 2.14f, 0.07f }, 0.56f, 0.48f, 0.44f, 0.12f);
        TerreoCaixa((Vector3){ 0.14f, 1.32f, 0.06f }, (Vector3){ 0.22f, 0.92f, 0.02f }, 0.26f, 0.22f, 0.22f, 0.08f);
        TerreoCaixa((Vector3){ 0.14f, 1.32f, 0.075f }, (Vector3){ 0.14f, 0.82f, 0.01f }, 0.86f, 0.84f, 0.80f, -1.0f);
        TerreoCaixa((Vector3){ -0.26f, 2.06f, 0.07f }, (Vector3){ 0.34f, 0.08f, 0.06f }, 0.82f, 0.76f, 0.70f, 0.18f);
        DrawCylinderEx((Vector3){ 0.26f, 0.98f, 0.08f }, (Vector3){ 0.42f, 0.98f, 0.08f }, 0.024f, 0.020f, 8, TerreoLum(0.88f));
    } else {
        TerreoCaixa((Vector3){ 0.0f, 1.10f, 0.0f }, (Vector3){ 1.16f, 2.20f, 0.14f }, 0.36f, 0.30f, 0.28f, 0.08f);
        TerreoCaixa((Vector3){ 0.0f, 1.07f, 0.02f }, (Vector3){ 1.00f, 2.10f, 0.06f }, 0.70f, 0.64f, 0.58f, 0.14f);
        TerreoCaixa((Vector3){ 0.36f, 0.98f, 0.06f }, (Vector3){ 0.05f, 0.10f, 0.02f }, 0.20f, 0.16f, 0.16f, -1.0f);
        DrawCylinderEx((Vector3){ 0.25f, 0.99f, 0.085f }, (Vector3){ 0.39f, 0.99f, 0.085f }, 0.018f, 0.018f, 8, TerreoLum(0.24f));
    }

    rlPopMatrix();
}

void TerreoCadeira(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.20f, 0.23f, 0.04f, 10, TerreoLum(0.18f));
    DrawCylinder((Vector3){ 0.0f, 0.04f, 0.0f }, 0.032f, 0.036f, 0.38f, 8, TerreoLum(0.20f));
    DrawCylinder((Vector3){ 0.0f, 0.41f, 0.0f }, 0.22f, 0.21f, 0.07f, 12, TerreoLum(0.28f));
    rlPushMatrix();
    rlTranslatef(0.0f, 0.68f, -0.17f);
    rlScalef(1.35f, 1.25f, 0.35f);
    DrawCylinder((Vector3){ 0.0f, -0.16f, 0.0f }, 0.15f, 0.15f, 0.34f, 10, TerreoLum(0.24f));
    rlPopMatrix();

    rlPopMatrix();
}

void TerreoMesaRedonda(Vector3 pos, float raio, float altura) {
    DrawCylinder((Vector3){ pos.x, pos.y, pos.z }, raio * 0.45f, raio * 0.50f, 0.04f, 14, TerreoLum(0.22f));
    DrawCylinder((Vector3){ pos.x, pos.y + 0.04f, pos.z }, 0.055f, 0.065f, altura - 0.08f, 10, TerreoLum(0.24f));
    DrawCylinder((Vector3){ pos.x, pos.y + altura - 0.05f, pos.z }, raio, raio, 0.05f, 18, TerreoLum(0.74f));
}

void TerreoBanqueta(Vector3 pos, float altura) {
    DrawCylinder((Vector3){ pos.x, pos.y, pos.z }, 0.16f, 0.19f, 0.03f, 10, TerreoLum(0.20f));
    DrawCylinder((Vector3){ pos.x, pos.y + 0.03f, pos.z }, 0.030f, 0.035f, altura - 0.08f, 8, TerreoLum(0.24f));
    DrawCylinder((Vector3){ pos.x, pos.y + altura * 0.36f, pos.z }, 0.13f, 0.13f, 0.025f, 10, TerreoLum(0.45f));
    DrawCylinder((Vector3){ pos.x, pos.y + altura - 0.06f, pos.z }, 0.18f, 0.17f, 0.06f, 12, TerreoLum(0.32f));
}

void TerreoMonitor(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.09f, 0.10f, 0.025f, 10, TerreoLum(0.20f));
    DrawCylinder((Vector3){ 0.0f, 0.025f, -0.02f }, 0.022f, 0.026f, 0.16f, 8, TerreoLum(0.20f));
    TerreoCaixa((Vector3){ 0.0f, 0.22f, 0.01f }, (Vector3){ 0.46f, 0.26f, 0.035f }, 0.18f, 0.10f, 0.12f, 0.34f);

    // Teclado low-profile e mouse na frente do monitor
    TerreoCaixa((Vector3){ -0.03f, 0.012f, 0.18f }, (Vector3){ 0.34f, 0.018f, 0.12f }, 0.24f, 0.18f, 0.18f, 0.42f);
    TerreoCaixa((Vector3){  0.22f, 0.012f, 0.18f }, (Vector3){ 0.06f, 0.018f, 0.09f }, 0.26f, 0.20f, 0.20f, 0.42f);

    rlPopMatrix();
}

void TerreoCadeiraConfortavel(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Base e pes conicos
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            DrawCylinder((Vector3){ sx * 0.19f, 0.0f, sz * 0.19f }, 0.020f, 0.026f, 0.36f, 8, TerreoLum(0.18f));
        }
    }
    // Almofada grossa e arredondada do assento
    DrawCylinder((Vector3){ 0.0f, 0.38f, 0.0f }, 0.26f, 0.25f, 0.11f, 16, TerreoLum(0.68f));
    // Encosto curvo acolchoado + bracos arredondados
    DrawCapsule((Vector3){ -0.20f, 0.66f, -0.20f }, (Vector3){ 0.20f, 0.66f, -0.20f }, 0.11f, 8, 12, TerreoLum(0.74f));
    DrawCapsule((Vector3){ -0.23f, 0.56f, -0.16f }, (Vector3){ -0.23f, 0.54f, 0.14f }, 0.055f, 6, 8, TerreoLum(0.62f));
    DrawCapsule((Vector3){  0.23f, 0.56f, -0.16f }, (Vector3){  0.23f, 0.54f, 0.14f }, 0.055f, 6, 8, TerreoLum(0.62f));

    rlPopMatrix();
}

void TerreoBalcaoRecepcaoU(Vector3 pos) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);

    // Bracos laterais + arco frontal oco estilo recepcao
    TerreoCaixa((Vector3){ -0.95f, 0.48f, -0.35f }, (Vector3){ 0.36f, 0.96f, 1.15f }, 0.46f, 0.34f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){  0.95f, 0.48f, -0.35f }, (Vector3){ 0.36f, 0.96f, 1.15f }, 0.46f, 0.34f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ -0.95f, 0.99f, -0.35f }, (Vector3){ 0.42f, 0.06f, 1.20f }, 0.84f, 0.64f, 0.56f, 0.14f);
    TerreoCaixa((Vector3){  0.95f, 0.99f, -0.35f }, (Vector3){ 0.42f, 0.06f, 1.20f }, 0.84f, 0.64f, 0.56f, 0.14f);

    const int segs = 18;
    const float rxIn = 0.77f, rzIn = 0.46f;
    const float rxOut = 1.13f, rzOut = 0.76f;
    const float hTop = 1.02f;
    Color cOut = TerreoLum(0.42f), cIn = TerreoLum(0.28f), cTop = TerreoLum(0.84f);
    for (int i = 0; i < segs; i++) {
        float a0 = ((float)i / (float)segs) * PI;
        float a1 = ((float)(i + 1) / (float)segs) * PI;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        Vector3 o0B = { c0 * rxOut, 0.0f, 0.22f + s0 * rzOut }, o1B = { c1 * rxOut, 0.0f, 0.22f + s1 * rzOut };
        Vector3 o0T = { c0 * rxOut, hTop, 0.22f + s0 * rzOut }, o1T = { c1 * rxOut, hTop, 0.22f + s1 * rzOut };
        Vector3 i0B = { c0 * rxIn,  0.0f, 0.22f + s0 * rzIn  }, i1B = { c1 * rxIn,  0.0f, 0.22f + s1 * rzIn  };
        Vector3 i0T = { c0 * rxIn,  hTop, 0.22f + s0 * rzIn  }, i1T = { c1 * rxIn,  hTop, 0.22f + s1 * rzIn  };
        DrawTriangle3D(o0B, o1T, o1B, cOut); DrawTriangle3D(o0B, o0T, o1T, cOut);
        DrawTriangle3D(i0B, i1B, i1T, cIn);  DrawTriangle3D(i0B, i1T, i0T, cIn);
        DrawTriangle3D(i0T, o1T, o0T, cTop); DrawTriangle3D(i0T, i1T, o1T, cTop);
    }
    TerreoMonitor((Vector3){ -0.35f, 1.02f, 0.66f }, 192.0f);
    TerreoCadeira((Vector3){ 0.0f, 0.0f, -0.35f }, 0.0f);

    rlPopMatrix();
}

void TerreoSilhuetaNPC(Vector3 pos, float yawDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    rlScalef(1.18f, 1.18f, 1.18f);

    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.14f, 0.15f, 0.88f, 10, TerreoLum(0.15f));
    rlPushMatrix();
    rlTranslatef(0.0f, 1.15f, 0.0f);
    rlScalef(1.40f, 1.0f, 0.80f);
    DrawCylinder((Vector3){ 0.0f, -0.27f, 0.0f }, 0.155f, 0.135f, 0.54f, 12, TerreoLum(0.20f));
    DrawSphereEx((Vector3){ 0.0f, 0.24f, 0.0f }, 0.155f, 8, 10, TerreoLum(0.21f));
    rlPopMatrix();
    DrawCapsule((Vector3){ -0.24f, 1.35f, 0.0f }, (Vector3){ -0.26f, 0.94f, 0.02f }, 0.052f, 6, 8, TerreoLum(0.19f));
    DrawCapsule((Vector3){  0.24f, 1.35f, 0.0f }, (Vector3){  0.26f, 0.94f, 0.02f }, 0.052f, 6, 8, TerreoLum(0.19f));
    DrawCylinder((Vector3){ 0.0f, 1.40f, 0.0f }, 0.055f, 0.062f, 0.10f, 8, TerreoLum(0.28f));
    DrawSphereEx((Vector3){ 0.0f, 1.59f, 0.0f }, 0.120f, 10, 12, TerreoLum(0.32f));
    DrawSphereEx((Vector3){ 0.0f, 1.65f, -0.02f }, 0.124f, 8, 10, TerreoLum(0.11f));

    rlPopMatrix();
}

void TerreoSilhuetaSombra(Vector3 pos, float yawDeg, bool sentado) {
    PlayerCharacterDrawNpc3D(pos, yawDeg, sentado, (float)GetTime(), g_camAtual.position);
}

#if 0 // Fallback procedural legado mantido apenas para referencia
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yawDeg, 0.0f, 1.0f, 0.0f);

    // Sombra realista para recepcionista e funcionarios da cantina
    Color cSombra = TerreoLum(0.05f);
    Color cDet    = TerreoLum(0.11f);
    if (sentado) {
        // Pernas sentadas (coxas horizontais + panturrilhas ate o chao)
        for (int side = -1; side <= 1; side += 2) {
            float hx = (float)side * 0.095f;
            DrawCapsule((Vector3){ hx, 0.45f, 0.02f }, (Vector3){ hx, 0.44f, 0.36f }, 0.068f, 6, 8, cSombra);
            DrawCapsule((Vector3){ hx, 0.44f, 0.36f }, (Vector3){ hx, 0.04f, 0.38f }, 0.052f, 6, 8, cSombra);
        }
        // Tronco sentado (ombros arredondados + cintura)
        rlPushMatrix();
        rlTranslatef(0.0f, 0.78f, 0.01f);
        rlRotatef(5.0f, 1.0f, 0.0f, 0.0f);
        rlScalef(1.32f, 1.0f, 0.76f);
        DrawCylinder((Vector3){ 0.0f, -0.26f, 0.0f }, 0.125f, 0.145f, 0.50f, 14, cSombra);
        DrawSphereEx((Vector3){ 0.0f, 0.23f, 0.0f }, 0.142f, 8, 12, cDet);
        rlPopMatrix();
        // Bracos relaxados apoiados
        for (int side = -1; side <= 1; side += 2) {
            DrawCapsule((Vector3){ (float)side * 0.21f, 0.99f, 0.02f },
                        (Vector3){ (float)side * 0.17f, 0.75f, 0.14f }, 0.046f, 6, 8, cSombra);
            DrawCapsule((Vector3){ (float)side * 0.17f, 0.75f, 0.14f },
                        (Vector3){ (float)side * 0.12f, 0.68f, 0.34f }, 0.038f, 6, 8, cSombra);
        }
        // Pescoco + cabeca ovoide + cabelo
        DrawCylinder((Vector3){ 0.0f, 1.02f, 0.02f }, 0.048f, 0.044f, 0.10f, 10, cSombra);
        rlPushMatrix();
        rlTranslatef(0.0f, 1.20f, 0.03f);
        rlScalef(0.86f, 1.08f, 0.94f);
        DrawSphereEx((Vector3){ 0.0f, 0.0f, 0.0f }, 0.105f, 10, 12, cDet);
        rlPopMatrix();
    } else {
        // Pernas separadas + avental/calca
        for (int side = -1; side <= 1; side += 2) {
            float hx = (float)side * 0.090f;
            DrawCylinderEx((Vector3){ hx, 0.02f, 0.0f }, (Vector3){ hx, 0.86f, 0.0f }, 0.048f, 0.075f, 10, cSombra);
        }
        // Tronco com ombros anatomicos + avental discreto na frente
        rlPushMatrix();
        rlTranslatef(0.0f, 1.12f, 0.0f);
        rlScalef(1.32f, 1.0f, 0.78f);
        DrawCylinder((Vector3){ 0.0f, -0.26f, 0.0f }, 0.128f, 0.148f, 0.52f, 14, cSombra);
        DrawSphereEx((Vector3){ 0.0f, 0.25f, 0.0f }, 0.146f, 8, 12, cDet);
        rlPopMatrix();
        // Avental / frente de uniforme
        DrawCylinderEx((Vector3){ 0.0f, 0.68f, 0.03f }, (Vector3){ 0.0f, 1.32f, 0.025f }, 0.105f, 0.095f, 12, cDet);
        // Bracos com dobra natural no cotovelo
        for (int side = -1; side <= 1; side += 2) {
            DrawCapsule((Vector3){ (float)side * 0.21f, 1.34f, 0.0f },
                        (Vector3){ (float)side * 0.23f, 1.08f, -0.01f }, 0.046f, 6, 8, cSombra);
            DrawCapsule((Vector3){ (float)side * 0.23f, 1.08f, -0.01f },
                        (Vector3){ (float)side * 0.21f, 0.86f, 0.06f }, 0.038f, 6, 8, cSombra);
        }
        // Pescoco + cabeca esculpida
        DrawCylinder((Vector3){ 0.0f, 1.36f, 0.0f }, 0.050f, 0.045f, 0.10f, 10, cSombra);
        rlPushMatrix();
        rlTranslatef(0.0f, 1.54f, 0.01f);
        rlScalef(0.86f, 1.08f, 0.94f);
        DrawSphereEx((Vector3){ 0.0f, 0.0f, 0.0f }, 0.106f, 10, 12, cDet);
        rlPopMatrix();
    }

    rlPopMatrix();
}
#endif

void TerreoBebedouro(Vector3 pos) {
    // Bebedouro corporativo detalhado: base, corpo metalico, nicho com grade, 2 alavancas, porta-copos lateral e galao curvado
    TerreoCaixa((Vector3){ pos.x, pos.y + 0.04f, pos.z }, (Vector3){ 0.42f, 0.08f, 0.42f }, 0.22f, 0.18f, 0.16f, 0.08f);
    TerreoCaixa((Vector3){ pos.x, pos.y + 0.52f, pos.z }, (Vector3){ 0.38f, 0.88f, 0.38f }, 0.84f, 0.74f, 0.66f, 0.16f);
    // Friso frontal + nicho escuro das torneiras + grade pingadeira
    TerreoCaixa((Vector3){ pos.x - 0.145f, pos.y + 0.74f, pos.z }, (Vector3){ 0.12f, 0.26f, 0.26f }, 0.16f, 0.12f, 0.12f, -1.0f);
    TerreoCaixa((Vector3){ pos.x - 0.165f, pos.y + 0.60f, pos.z }, (Vector3){ 0.11f, 0.025f, 0.24f }, 0.38f, 0.28f, 0.26f, 0.12f);
    // Torneiras (gelada e natural)
    TerreoCaixa((Vector3){ pos.x - 0.15f, pos.y + 0.82f, pos.z - 0.055f }, (Vector3){ 0.05f, 0.04f, 0.03f }, 0.90f, 0.82f, 0.80f, -1.0f);
    TerreoCaixa((Vector3){ pos.x - 0.15f, pos.y + 0.82f, pos.z + 0.055f }, (Vector3){ 0.05f, 0.04f, 0.03f }, 0.30f, 0.24f, 0.24f, -1.0f);
    // Porta-copos cilindrico acoplado na lateral
    DrawCylinder((Vector3){ pos.x, pos.y + 0.58f, pos.z + 0.23f }, 0.042f, 0.042f, 0.34f, 10, TerreoLum(0.78f));
    DrawCylinder((Vector3){ pos.x, pos.y + 0.92f, pos.z + 0.23f }, 0.048f, 0.048f, 0.02f, 10, TerreoLum(0.24f));
    // Anel adaptador + galao de agua com ombros arredondados e nivel de agua
    DrawCylinder((Vector3){ pos.x, pos.y + 0.96f, pos.z }, 0.14f, 0.16f, 0.05f, 16, TerreoLum(0.24f));
    DrawCylinder((Vector3){ pos.x, pos.y + 1.01f, pos.z }, 0.175f, 0.185f, 0.28f, 18, TerreoLum(0.86f));
    DrawCylinder((Vector3){ pos.x, pos.y + 1.29f, pos.z }, 0.175f, 0.175f, 0.10f, 18, TerreoLum(0.94f));
    DrawCylinder((Vector3){ pos.x, pos.y + 1.39f, pos.z }, 0.065f, 0.175f, 0.09f, 16, TerreoLum(0.88f));
    DrawCylinderWires((Vector3){ pos.x, pos.y + 1.15f, pos.z }, 0.182f, 0.182f, 0.01f, 16, TerreoLum(0.56f));
    DrawCylinderWires((Vector3){ pos.x, pos.y + 1.25f, pos.z }, 0.182f, 0.182f, 0.01f, 16, TerreoLum(0.56f));
}

void TerreoImpressora(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Gabinete base com 2 gavetas de papel A4/A3 e puxadores
    TerreoCaixa((Vector3){ 0.0f, 0.36f, 0.0f }, (Vector3){ 0.72f, 0.72f, 0.56f }, 0.42f, 0.32f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ 0.0f, 0.20f, 0.285f }, (Vector3){ 0.58f, 0.24f, 0.015f }, 0.50f, 0.44f, 0.40f, 0.14f);
    TerreoCaixa((Vector3){ 0.0f, 0.50f, 0.285f }, (Vector3){ 0.58f, 0.24f, 0.015f }, 0.50f, 0.44f, 0.40f, 0.14f);
    TerreoCaixa((Vector3){ 0.0f, 0.20f, 0.298f }, (Vector3){ 0.16f, 0.025f, 0.015f }, 0.20f, 0.16f, 0.16f, -1.0f);
    TerreoCaixa((Vector3){ 0.0f, 0.50f, 0.298f }, (Vector3){ 0.16f, 0.025f, 0.015f }, 0.20f, 0.16f, 0.16f, -1.0f);
    // Corpo principal + nicho escuro de saida de folha + folha impressa
    TerreoCaixa((Vector3){ 0.0f, 0.90f, -0.02f }, (Vector3){ 0.70f, 0.36f, 0.52f }, 0.84f, 0.70f, 0.62f, 0.16f);
    TerreoCaixa((Vector3){ -0.08f, 0.86f, 0.245f }, (Vector3){ 0.38f, 0.12f, 0.02f }, 0.14f, 0.10f, 0.10f, -1.0f);
    TerreoCaixa((Vector3){ -0.08f, 0.83f, 0.31f }, (Vector3){ 0.28f, 0.01f, 0.16f }, 0.96f, 0.92f, 0.88f, -1.0f);
    // Tampa do scanner + alimentador automatico (ADF) + painel LCD inclinado com LED
    TerreoCaixa((Vector3){ 0.0f, 1.11f, -0.02f }, (Vector3){ 0.68f, 0.06f, 0.50f }, 0.32f, 0.24f, 0.22f, 0.12f);
    TerreoCaixa((Vector3){ -0.05f, 1.17f, -0.04f }, (Vector3){ 0.44f, 0.06f, 0.32f }, 0.88f, 0.78f, 0.70f, 0.16f);
    TerreoCaixa((Vector3){  0.22f, 1.06f,  0.26f }, (Vector3){ 0.18f, 0.09f, 0.04f }, 0.22f, 0.16f, 0.16f, 0.38f);
    DrawSphere((Vector3){ 0.28f, 1.07f, 0.285f }, 0.010f, (Color){ 255, 224, 48, 255 });

    rlPopMatrix();
}

void TerreoArmario(Vector3 pos, float rotYDeg, float largura, float altura, float prof) {
    // Gabinetes metalicos de escritorio com 4 gavetas horizontais, puxadores e porta-etiquetas
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    float rodapeH = 0.06f;
    float corpoH = altura - rodapeH;
    float fZ = prof * 0.5f;

    TerreoCaixa((Vector3){ 0.0f, rodapeH * 0.5f, -0.01f },
                (Vector3){ largura * 0.96f, rodapeH, prof * 0.94f },
                0.20f, 0.14f, 0.12f, 0.06f);
    TerreoCaixa((Vector3){ 0.0f, rodapeH + corpoH * 0.5f, 0.0f },
                (Vector3){ largura, corpoH, prof },
                0.56f, 0.44f, 0.38f, 0.14f);

    // 4 gavetas horizontais empilhadas
    const int numGav = 4;
    float gavH = (corpoH - 0.08f) / (float)numGav;
    for (int g = 0; g < numGav; g++) {
        float gy = rodapeH + 0.04f + (g + 0.5f) * gavH;
        TerreoCaixa((Vector3){ 0.0f, gy, fZ + 0.012f },
                    (Vector3){ largura * 0.88f, gavH * 0.84f, 0.022f },
                    0.50f, 0.40f, 0.34f, 0.14f);
        // Puxador horizontal metalico + porta-etiqueta
        TerreoCaixa((Vector3){ 0.0f, gy + gavH * 0.12f, fZ + 0.030f },
                    (Vector3){ largura * 0.26f, 0.024f, 0.022f },
                    0.22f, 0.16f, 0.14f, -1.0f);
        TerreoCaixa((Vector3){ 0.0f, gy - gavH * 0.14f, fZ + 0.026f },
                    (Vector3){ 0.12f, 0.045f, 0.008f },
                    0.82f, 0.74f, 0.68f, 0.24f);
    }

    rlPopMatrix();
}

void TerreoEscadaCaracol(Vector3 centro, float raioInt, float raioExt, float altBase, float altTopo,
                         int degraus, float angIniDeg, float angDeltaDeg, bool comMuretaExterna) {
    // Baseada em Escada.jpg e Escada2.jpg: degraus em leque + mureta curva continua + corrimao escuro
    float dAng = (angDeltaDeg * DEG2RAD) / (float)degraus;
    float a0Rad = angIniDeg * DEG2RAD;
    float dAlt = (altTopo - altBase) / (float)degraus;
    float muretaH = 0.88f;

    for (int i = 0; i < degraus; i++) {
        float aA = a0Rad + (float)i * dAng;
        float aB = a0Rad + (float)(i + 1) * dAng;
        float yTop = altBase + (float)(i + 1) * dAlt;
        float yBot = yTop - 0.14f;

        float ca = cosf(aA), sa = sinf(aA);
        float cb = cosf(aB), sb = sinf(aB);

        Vector3 iA = { centro.x + ca * raioInt, yTop, centro.z + sa * raioInt };
        Vector3 iB = { centro.x + cb * raioInt, yTop, centro.z + sb * raioInt };
        Vector3 eA = { centro.x + ca * raioExt, yTop, centro.z + sa * raioExt };
        Vector3 eB = { centro.x + cb * raioExt, yTop, centro.z + sb * raioExt };

        Vector3 eAb = { eA.x, yBot, eA.z };
        Vector3 eBb = { eB.x, yBot, eB.z };
        Vector3 iAb = { iA.x, yBot, iA.z };
        Vector3 iBb = { iB.x, yBot, iB.z };

        float lumPiso = (i % 2 == 0) ? 0.74f : 0.68f;
        Color cTop = TerreoLum(lumPiso);
        Color cEsp = TerreoLum(0.42f);
        Color cMur = TerreoLum(0.62f);

        // Piso do degrau em leque
        DrawTriangle3D(iA, eA, eB, cTop);
        DrawTriangle3D(iA, eB, iB, cTop);
        // Espelho frontal/traseiro do degrau
        DrawTriangle3D(iAb, eAb, eA, cEsp);
        DrawTriangle3D(iAb, eA, iA, cEsp);
        DrawTriangle3D(iBb, iB, eB, cEsp);
        DrawTriangle3D(iBb, eB, eBb, cEsp);

        // Mureta interna continua (Escada.jpg / Escada2.jpg)
        Vector3 mi0 = { iA.x, yBot, iA.z };
        Vector3 mi1 = { iB.x, yBot, iB.z };
        Vector3 mi0T = { iA.x, yTop + muretaH, iA.z };
        Vector3 mi1T = { iB.x, yTop + muretaH, iB.z };
        DrawTriangle3D(mi0, mi1, mi1T, cMur);
        DrawTriangle3D(mi0, mi1T, mi0T, cMur);
        DrawLine3D(mi0T, mi1T, TerreoLum(0.14f));

        // Mureta externa continua + corrimao
        if (comMuretaExterna) {
            Vector3 me0 = { eA.x, yBot, eA.z };
            Vector3 me1 = { eB.x, yBot, eB.z };
            Vector3 me0T = { eA.x, yTop + muretaH, eA.z };
            Vector3 me1T = { eB.x, yTop + muretaH, eB.z };
            DrawTriangle3D(me0, me1T, me1, cMur);
            DrawTriangle3D(me0, me0T, me1T, cMur);
            DrawCylinderEx(me0T, me1T, 0.035f, 0.035f, 6, TerreoLum(0.14f));
        }
        DrawLine3D(iA, eA, TerreoLum(0.24f));
    }
}

void TerreoElevador(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Moldura externa metalica do elevador
    TerreoCaixa((Vector3){ -0.82f, 1.18f, 0.0f }, (Vector3){ 0.14f, 2.36f, 0.16f }, 0.36f, 0.28f, 0.24f, 0.10f);
    TerreoCaixa((Vector3){  0.82f, 1.18f, 0.0f }, (Vector3){ 0.14f, 2.36f, 0.16f }, 0.36f, 0.28f, 0.24f, 0.10f);
    TerreoCaixa((Vector3){  0.00f, 2.32f, 0.0f }, (Vector3){ 1.78f, 0.20f, 0.16f }, 0.36f, 0.28f, 0.24f, 0.10f);

    // Duas folhas de porta metalica deslizante com fresta central
    TerreoCaixa((Vector3){ -0.37f, 1.10f, 0.01f }, (Vector3){ 0.72f, 2.20f, 0.06f }, 0.68f, 0.58f, 0.50f, 0.16f);
    TerreoCaixa((Vector3){  0.37f, 1.10f, 0.01f }, (Vector3){ 0.72f, 2.20f, 0.06f }, 0.68f, 0.58f, 0.50f, 0.16f);
    TerreoCaixa((Vector3){  0.00f, 1.10f, 0.03f }, (Vector3){ 0.02f, 2.20f, 0.03f }, 0.12f, 0.08f, 0.08f, -1.0f);

    // Visor digital de andar acima da porta
    TerreoCaixa((Vector3){ 0.00f, 2.32f, 0.09f }, (Vector3){ 0.34f, 0.12f, 0.02f }, 0.14f, 0.10f, 0.10f, 0.72f);

    // Painel de chamada ao lado direito com botao iluminado
    TerreoCaixa((Vector3){ 1.04f, 1.12f, 0.06f }, (Vector3){ 0.12f, 0.26f, 0.04f }, 0.24f, 0.18f, 0.16f, 0.10f);
    TerreoCaixa((Vector3){ 1.04f, 1.15f, 0.085f }, (Vector3){ 0.04f, 0.04f, 0.015f }, 0.92f, 0.90f, 0.85f, -1.0f);
    TerreoCaixa((Vector3){ 1.04f, 1.07f, 0.085f }, (Vector3){ 0.04f, 0.04f, 0.015f }, 0.92f, 0.90f, 0.85f, -1.0f);

    rlPopMatrix();
}

void TerreoRackServidor(Vector3 pos, float rotYDeg) {
    // Baseado em serverciberseguranca.jpg: rack preto perfurado alto com puxador vertical e LEDs
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    TerreoCaixa((Vector3){ 0.0f, 1.08f, 0.0f }, (Vector3){ 0.92f, 2.16f, 0.86f }, 0.16f, 0.10f, 0.12f, 0.24f);
    // Grade frontal perfurada + puxador vertical
    TerreoCaixa((Vector3){ 0.0f, 1.08f, 0.44f }, (Vector3){ 0.80f, 2.00f, 0.02f }, 0.20f, 0.16f, 0.14f, 0.30f);
    TerreoCaixa((Vector3){ 0.32f, 1.08f, 0.46f }, (Vector3){ 0.04f, 0.24f, 0.03f }, 0.42f, 0.36f, 0.30f, -1.0f);
    // Pequenos LEDs acesos por tras da grade
    for (int k = 0; k < 4; k++) {
        float ly = 0.55f + k * 0.38f;
        TerreoCaixa((Vector3){ -0.22f + (k % 2) * 0.16f, ly, 0.455f },
                    (Vector3){ 0.035f, 0.025f, 0.01f }, 0.94f, 0.92f, 0.88f, -1.0f);
    }

    rlPopMatrix();
}

void TerreoRoboPrototipo(Vector3 pos, float rotYDeg) {
    // Baseado em treinamentoia_prototipo.jpg: robo humanoide branco articulado com juntas escuras
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    Color cBranco = TerreoLum(0.90f);
    Color cSombra = TerreoLum(0.76f);
    Color cJunta  = TerreoLum(0.10f);

    // Pernas + juntas do quadril e joelhos
    for (int s = -1; s <= 1; s += 2) {
        float lx = (float)s * 0.11f;
        DrawCylinderEx((Vector3){ lx, 0.00f, 0.0f }, (Vector3){ lx, 0.44f, 0.0f }, 0.055f, 0.068f, 10, cSombra);
        DrawSphereEx((Vector3){ lx, 0.45f, 0.0f }, 0.052f, 8, 8, cJunta);
        DrawCylinderEx((Vector3){ lx, 0.46f, 0.0f }, (Vector3){ lx, 0.88f, 0.0f }, 0.068f, 0.082f, 10, cBranco);
    }
    // Quadril + cintura escura + tronco esculpido
    DrawCylinder((Vector3){ 0.0f, 0.86f, 0.0f }, 0.17f, 0.15f, 0.16f, 12, cSombra);
    DrawCylinder((Vector3){ 0.0f, 1.01f, 0.0f }, 0.12f, 0.12f, 0.06f, 10, cJunta);
    rlPushMatrix();
    rlTranslatef(0.0f, 1.28f, 0.0f);
    rlScalef(1.32f, 1.0f, 0.78f);
    DrawCylinder((Vector3){ 0.0f, -0.22f, 0.0f }, 0.145f, 0.175f, 0.48f, 14, cBranco);
    rlPopMatrix();

    // Bracos articulados com juntas nos ombros e cotovelos
    for (int s = -1; s <= 1; s += 2) {
        float sx = (float)s * 0.25f;
        DrawSphereEx((Vector3){ sx, 1.46f, 0.0f }, 0.055f, 8, 8, cJunta);
        DrawCapsule((Vector3){ sx, 1.44f, 0.0f }, (Vector3){ sx + (float)s * 0.03f, 1.18f, 0.0f }, 0.052f, 6, 8, cBranco);
        DrawSphereEx((Vector3){ sx + (float)s * 0.03f, 1.16f, 0.0f }, 0.042f, 8, 8, cJunta);
        DrawCapsule((Vector3){ sx + (float)s * 0.03f, 1.14f, 0.0f }, (Vector3){ sx + (float)s * 0.04f, 0.88f, 0.03f }, 0.044f, 6, 8, cSombra);
    }

    // Pescoco + cabeca lisa com visores escuros nos olhos
    DrawCylinder((Vector3){ 0.0f, 1.52f, 0.0f }, 0.050f, 0.055f, 0.08f, 10, cJunta);
    rlPushMatrix();
    rlTranslatef(0.0f, 1.69f, 0.0f);
    rlScalef(0.84f, 1.12f, 0.92f);
    DrawSphereEx((Vector3){ 0.0f, 0.0f, 0.0f }, 0.115f, 12, 12, cBranco);
    rlPopMatrix();
    TerreoCaixa((Vector3){ -0.045f, 1.72f, 0.098f }, (Vector3){ 0.036f, 0.016f, 0.02f }, 0.06f, 0.04f, 0.04f, -1.0f);
    TerreoCaixa((Vector3){  0.045f, 1.72f, 0.098f }, (Vector3){ 0.036f, 0.016f, 0.02f }, 0.06f, 0.04f, 0.04f, -1.0f);

    rlPopMatrix();
}

void TerreoPlanta(Vector3 pos, float escala) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlScalef(escala, escala, escala);

    // Vaso ceramico conico estilizado + borda arredondada
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.17f, 0.13f, 0.28f, 16, TerreoLum(0.24f));
    DrawCylinder((Vector3){ 0.0f, 0.27f, 0.0f }, 0.18f, 0.17f, 0.045f, 16, TerreoLum(0.32f));
    DrawCylinder((Vector3){ 0.0f, 0.30f, 0.0f }, 0.15f, 0.15f, 0.015f, 14, TerreoLum(0.14f));

    // Galhos finos com bifurcacoes em ziguezague (estilo relampago) e folhas finas
    for (int i = 0; i < 6; i++) {
        float ang = (float)i * (2.0f * PI / 6.0f) + 0.25f;
        float ca = cosf(ang), sa = sinf(ang);
        float px = -sa, pz = ca;
        Vector3 base = { ca * 0.02f, 0.31f, sa * 0.02f };
        Vector3 k1   = { ca * 0.09f + px * 0.04f, 0.44f, sa * 0.09f + pz * 0.04f };
        Vector3 k2   = { ca * 0.19f - px * 0.03f, 0.54f + (i % 2) * 0.04f, sa * 0.19f - pz * 0.03f };
        Vector3 tipA = { ca * 0.28f + px * 0.05f, 0.48f + (i % 2) * 0.03f, sa * 0.28f + pz * 0.05f };
        Vector3 tipB = { ca * 0.23f - px * 0.08f, 0.62f, sa * 0.23f - pz * 0.08f };
        DrawCylinderEx(base, k1, 0.012f, 0.009f, 5, TerreoLum(0.18f));
        DrawCylinderEx(k1, k2,   0.009f, 0.006f, 5, TerreoLum(0.18f));
        DrawCylinderEx(k2, tipA, 0.006f, 0.003f, 4, TerreoLum(0.20f));
        DrawCylinderEx(k2, tipB, 0.006f, 0.003f, 4, TerreoLum(0.20f));
        float lum = 0.34f + (float)(i % 3) * 0.07f;
        DrawCapsule(k2, tipA, 0.011f, 4, 6, TerreoLum(lum));
        DrawCapsule(k2, tipB, 0.010f, 4, 6, TerreoLum(lum + 0.08f));
    }
    DrawCylinderEx((Vector3){ 0.0f, 0.31f, 0.0f }, (Vector3){ 0.02f, 0.50f, -0.01f }, 0.013f, 0.008f, 5, TerreoLum(0.18f));
    DrawCapsule((Vector3){ 0.02f, 0.50f, -0.01f }, (Vector3){ 0.06f, 0.66f, 0.03f }, 0.011f, 4, 6, TerreoLum(0.46f));
    DrawCapsule((Vector3){ 0.02f, 0.50f, -0.01f }, (Vector3){ -0.04f, 0.63f, -0.05f }, 0.010f, 4, 6, TerreoLum(0.42f));

    rlPopMatrix();
}

void TerreoGabinetePC(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);
    TerreoCaixa((Vector3){ 0.0f, 0.19f, 0.0f }, (Vector3){ 0.17f, 0.38f, 0.36f }, 0.24f, 0.18f, 0.20f, 0.38f);
    TerreoCaixa((Vector3){ 0.0f, 0.28f, 0.182f }, (Vector3){ 0.13f, 0.10f, 0.01f }, 0.30f, 0.28f, 0.28f, -1.0f);
    TerreoCaixa((Vector3){ 0.0f, 0.11f, 0.182f }, (Vector3){ 0.12f, 0.14f, 0.01f }, 0.14f, 0.12f, 0.12f, -1.0f);
    DrawSphere((Vector3){ 0.045f, 0.34f, 0.186f }, 0.010f, (Color){ 255, 224, 48, 255 });
    rlPopMatrix();
}

void TerreoGeladeira(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);
    DrawCylinder((Vector3){ -0.34f, 0.0f,  0.28f }, 0.03f, 0.03f, 0.05f, 8, TerreoLum(0.18f));
    DrawCylinder((Vector3){  0.34f, 0.0f,  0.28f }, 0.03f, 0.03f, 0.05f, 8, TerreoLum(0.18f));
    DrawCylinder((Vector3){ -0.34f, 0.0f, -0.28f }, 0.03f, 0.03f, 0.05f, 8, TerreoLum(0.18f));
    DrawCylinder((Vector3){  0.34f, 0.0f, -0.28f }, 0.03f, 0.03f, 0.05f, 8, TerreoLum(0.18f));
    TerreoCaixa((Vector3){ 0.0f, 0.68f, 0.0f }, (Vector3){ 0.84f, 1.24f, 0.74f }, 0.80f, 0.72f, 0.64f, 0.18f);
    TerreoCaixa((Vector3){ 0.0f, 1.62f, 0.0f }, (Vector3){ 0.84f, 0.56f, 0.74f }, 0.86f, 0.76f, 0.68f, 0.18f);
    TerreoCaixa((Vector3){ 0.33f, 0.96f, 0.39f }, (Vector3){ 0.035f, 0.44f, 0.04f }, 0.30f, 0.24f, 0.24f, -1.0f);
    TerreoCaixa((Vector3){ 0.33f, 1.60f, 0.39f }, (Vector3){ 0.035f, 0.24f, 0.04f }, 0.30f, 0.24f, 0.24f, -1.0f);
    TerreoCaixa((Vector3){ -0.10f, 1.02f, 0.375f }, (Vector3){ 0.22f, 0.28f, 0.02f }, 0.18f, 0.14f, 0.14f, 0.42f);
    rlPopMatrix();
}

void TerreoCafeteira(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);
    TerreoCaixa((Vector3){ 0.0f, 0.025f, 0.0f }, (Vector3){ 0.34f, 0.05f, 0.32f }, 0.28f, 0.20f, 0.18f, 0.12f);
    TerreoCaixa((Vector3){ 0.0f, 0.21f, -0.08f }, (Vector3){ 0.32f, 0.34f, 0.14f }, 0.24f, 0.18f, 0.16f, 0.14f);
    TerreoCaixa((Vector3){ 0.0f, 0.34f,  0.03f }, (Vector3){ 0.30f, 0.09f, 0.24f }, 0.30f, 0.22f, 0.20f, 0.14f);
    DrawCylinder((Vector3){ 0.0f, 0.05f, 0.04f }, 0.085f, 0.095f, 0.16f, 12, TerreoLum(0.14f));
    DrawCylinder((Vector3){ 0.0f, 0.055f, 0.04f }, 0.090f, 0.090f, 0.03f, 12, TerreoLum(0.56f));
    DrawSphere((Vector3){ 0.10f, 0.34f, 0.155f }, 0.012f, (Color){ 255, 224, 48, 255 });
    rlPopMatrix();
}

void TerreoMicroondas(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);
    TerreoCaixa((Vector3){ 0.0f, 0.17f, 0.0f }, (Vector3){ 0.56f, 0.34f, 0.40f }, 0.72f, 0.62f, 0.56f, 0.16f);
    TerreoCaixa((Vector3){ -0.07f, 0.17f, 0.205f }, (Vector3){ 0.34f, 0.24f, 0.015f }, 0.14f, 0.10f, 0.10f, 0.40f);
    TerreoCaixa((Vector3){  0.19f, 0.17f, 0.205f }, (Vector3){ 0.11f, 0.24f, 0.015f }, 0.26f, 0.20f, 0.20f, 0.32f);
    TerreoCaixa((Vector3){  0.19f, 0.24f, 0.215f }, (Vector3){ 0.08f, 0.04f, 0.01f }, 0.85f, 0.85f, 0.85f, -1.0f);
    rlPopMatrix();
}

void TerreoDesenharNPCAtividade(int variantId, int ativ, Vector3 pos, float yawDeg, float tempo) {
    PlayerCharacterDrawNpc3DEx(pos, yawDeg, (NpcAtividade)ativ, variantId, tempo, g_camAtual.position);
}

void TerreoSofaCurvo(Vector3 pos, float rotYDeg, float largura) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    float hw = largura * 0.5f;
    // Pes conicos discretos
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            DrawCylinder((Vector3){ sx * (hw - 0.14f), 0.0f, sz * 0.28f }, 0.022f, 0.030f, 0.12f, 10, TerreoLum(0.16f));
        }
    }
    // Base + almofadas arredondadas (capsulas suaves, nada quadrado)
    DrawCapsule((Vector3){ -hw + 0.12f, 0.24f,  0.04f }, (Vector3){ hw - 0.12f, 0.24f,  0.04f }, 0.16f, 8, 12, TerreoLum(0.62f));
    DrawCapsule((Vector3){ -hw + 0.12f, 0.26f, -0.10f }, (Vector3){ hw - 0.12f, 0.26f, -0.10f }, 0.15f, 8, 12, TerreoLum(0.58f));
    // Encosto curvo acolchoado
    DrawCapsule((Vector3){ -hw + 0.10f, 0.56f, -0.28f }, (Vector3){ hw - 0.10f, 0.56f, -0.28f }, 0.14f, 8, 12, TerreoLum(0.68f));
    DrawCapsule((Vector3){ -hw + 0.12f, 0.42f, -0.26f }, (Vector3){ hw - 0.12f, 0.42f, -0.26f }, 0.13f, 8, 12, TerreoLum(0.64f));
    // Bracos laterais arredondados
    for (int sx = -1; sx <= 1; sx += 2) {
        float ax = (float)sx * (hw - 0.06f);
        DrawCapsule((Vector3){ ax, 0.42f, -0.24f }, (Vector3){ ax, 0.40f, 0.22f }, 0.10f, 6, 10, TerreoLum(0.66f));
    }

    rlPopMatrix();
}

void TerreoSofaRedondo(Vector3 pos, float rotYDeg, float raio) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Sofa curvo modular limpo com pes visiveis, assento uniforme e encosto acolchoado
    const int segs = 6;
    float angSpan = PI * 0.78f;
    float angStart = -angSpan * 0.5f;
    for (int i = 0; i <= segs; i++) {
        float a = angStart + (angSpan * (float)i) / (float)segs;
        float px = sinf(a) * raio;
        float pz = -cosf(a) * (raio * 0.45f) + raio * 0.35f;
        if (i % 2 == 0) {
            DrawCylinder((Vector3){ px, 0.0f, pz }, 0.024f, 0.030f, 0.14f, 8, TerreoLum(0.16f));
        }
        if (i < segs) {
            float a1 = angStart + (angSpan * (float)(i + 1)) / (float)segs;
            Vector3 s0 = { px, 0.26f, pz };
            Vector3 s1 = { sinf(a1) * raio, 0.26f, -cosf(a1) * (raio * 0.45f) + raio * 0.35f };
            DrawCapsule(s0, s1, 0.21f, 8, 12, TerreoLum(0.64f));

            Vector3 b0 = { sinf(a) * (raio + 0.18f), 0.58f, -cosf(a) * (raio * 0.45f + 0.18f) + raio * 0.35f };
            Vector3 b1 = { sinf(a1) * (raio + 0.18f), 0.58f, -cosf(a1) * (raio * 0.45f + 0.18f) + raio * 0.35f };
            DrawCapsule(b0, b1, 0.13f, 8, 12, TerreoLum(0.72f));
        }
    }

    rlPopMatrix();
}

static BoundingBox BoxInteractable(const SalaInteractable *it) {
    Vector3 h = Vector3Scale(it->size, 0.5f);
    return (BoundingBox){
        { it->position.x - h.x, it->position.y - h.y, it->position.z - h.z },
        { it->position.x + h.x, it->position.y + h.y, it->position.z + h.z }
    };
}

static void GarantirTexturas(int largura, int altura) {
    if (largura < 16) largura = 16;
    if (altura < 16) altura = 16;
    if (g_texPronta && g_largura == largura && g_altura == altura) return;
    if (g_texPronta) {
        UnloadRenderTexture(g_cena);
        UnloadRenderTexture(g_final);
    }
    g_largura = largura;
    g_altura  = altura;
    g_cena    = LoadRenderTexture(largura, altura);
    g_final   = LoadRenderTexture(largura, altura);
    SetTextureFilter(g_cena.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(g_final.texture, TEXTURE_FILTER_POINT);
    g_texPronta = true;
}

void Terreo3DSetViewportSize(int largura, int altura) {
    if (!g_pronto) return;
    GarantirTexturas(largura, altura);
}

void Terreo3DInit(int largura, int altura) {
    Terreo3DUnload();

    g_shader = LoadShaderFromMemory(NULL, TERREO_FS);
    g_salas[TERREO_SALA_RECEPCAO]        = SalaRecepcaoDef();
    g_salas[TERREO_SALA_LANCHONETE]      = SalaLanchoneteDef();
    g_salas[TERREO_SALA_JARDIM]          = SalaJardimDef();
    g_salas[TERREO_SALA_ESCRITORIO]      = SalaEscritorioDef();
    g_salas[TERREO_SALA_ESCRITORIO_2]    = SalaEscritorio2Def();
    g_salas[TERREO_SALA_CONVIVIO]        = SalaConvivioDef();
    g_salas[TERREO_SALA_HALL_1]          = SalaHall1Def();
    g_salas[TERREO_SALA_CONTABILIDADE_1] = SalaContabilidade1Def();
    g_salas[TERREO_SALA_CONTABILIDADE_2] = SalaContabilidade2Def();
    g_salas[TERREO_SALA_ARQUIVO_1]       = SalaArquivo1Def();
    g_salas[TERREO_SALA_CAMERAS]         = SalaCamerasDef();
    g_salas[TERREO_SALA_COPA_1]          = SalaCopa1Def();
    g_salas[TERREO_SALA_HALL_2]          = SalaHall2Def();
    g_salas[TERREO_SALA_CIBER_1]         = SalaCiber1Def();
    g_salas[TERREO_SALA_CIBER_2]         = SalaCiber2Def();
    g_salas[TERREO_SALA_SERVER_1]           = SalaServer1Def();
    g_salas[TERREO_SALA_SERVER_2]           = SalaServer2Def();
    g_salas[TERREO_SALA_HALL_3]             = SalaHall3Def();
    g_salas[TERREO_SALA_TREINO_ESCRITORIO]  = SalaTreinoEscritorioDef();
    g_salas[TERREO_SALA_TREINO_IA]          = SalaTreinoIADef();
    g_salas[TERREO_SALA_GARAGEM_PROTOTIPOS] = SalaGaragemPrototiposDef();
    g_salas[TERREO_SALA_HALL_4]             = SalaHall4Def();
    g_salas[TERREO_SALA_AUDITORIO]          = SalaAuditorioDef();
    g_salas[TERREO_SALA_REUNIAO_1]          = SalaReuniao1Def();
    g_salas[TERREO_SALA_REUNIAO_2]          = SalaReuniao2Def();
    g_salas[TERREO_SALA_SUBSOLO]            = SalaSubsoloDef();
    g_salas[TERREO_SALA_SUBSOLO_2]          = SalaSubsolo2Def();

    GarantirTexturas(largura, altura);
    Terreo3DDefinirSala(TERREO_SALA_RECEPCAO, TERREO_DEST_NENHUM);

    g_evRelatoriosAtiva = false;
    g_mouseSobreInteragivel = false;
    g_hasProxInteragivel = false;
    g_hasClosestMouseTarget = false;
    g_dicaAcao[0] = '\0';
    g_passoTimer = 0.0f;
    g_portaAnimTimer = 0.0f;
    g_portaDestPendente = TERREO_DEST_NENHUM;
    if (g_sndPasso3D.frameCount == 0) g_sndPasso3D = CriarSomProcedural3D(980, 150.0f, 70.0f, 0.55f, 0.35f);
    if (g_sndPorta3D.frameCount == 0) g_sndPorta3D = CriarSomProcedural3D(2400, 220.0f, 110.0f, 0.40f, 0.45f);
    g_pronto = true;
}

void Terreo3DUnload(void) {
    if (!g_pronto) return;
    if (g_sndPasso3D.frameCount > 0) { UnloadSound(g_sndPasso3D); g_sndPasso3D = (Sound){ 0 }; }
    if (g_sndPorta3D.frameCount > 0) { UnloadSound(g_sndPorta3D); g_sndPorta3D = (Sound){ 0 }; }
    if (g_texPronta) {
        UnloadRenderTexture(g_cena);
        UnloadRenderTexture(g_final);
        g_texPronta = false;
    }
    if (g_shader.id > 0) {
        UnloadShader(g_shader);
        g_shader = (Shader){ 0 };
    }
    PlayerCharacterUnload();
    g_pronto = false;
}

static bool DevPrimeiraPessoaAtiva(void) {
    return SettingsGetDevMode() && SettingsGetDevFirstPerson();
}

static void AtualizarCameraPrimeiraPessoa(const TerreoSalaDef *def, float dt, bool bloqueado) {
    if (!def) return;
    if (!bloqueado) {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 md = GetMouseDelta();
            g_fpYaw   -= md.x * 0.0045f;
            g_fpPitch -= md.y * 0.0045f;
        }
        const float yawSpd = 2.25f;
        const float pitchSpd = 1.65f;
        if (IsKeyDown(KEY_LEFT))  g_fpYaw   += yawSpd * dt;
        if (IsKeyDown(KEY_RIGHT)) g_fpYaw   -= yawSpd * dt;
        if (IsKeyDown(KEY_UP))    g_fpPitch += pitchSpd * dt;
        if (IsKeyDown(KEY_DOWN))  g_fpPitch -= pitchSpd * dt;
        g_fpPitch = Clamp(g_fpPitch, -1.25f, 1.25f);
    }

    if (g_player.interactType != PLAYER_ANIM_COMPUTER || g_player.interactTimer <= 0.0f) {
        g_player.facingYaw = g_fpYaw;
    }

    bool sentadoComp = (g_player.interactType == PLAYER_ANIM_COMPUTER && g_player.interactTimer > 0.0f);
    float headBob = sentadoComp ? 0.0f : sinf(g_player.walkCycle * 2.0f) * 0.032f * g_player.moveBlend;
    float eyeY = g_player.position.y + (sentadoComp ? 1.22f : 1.62f) + headBob;
    Vector3 eyePos = { g_player.position.x, eyeY, g_player.position.z };

    float cp = cosf(g_fpPitch);
    Vector3 fwd = { sinf(g_fpYaw) * cp, sinf(g_fpPitch), cosf(g_fpYaw) * cp };

    g_camAtual.position   = eyePos;
    g_camAtual.target     = (Vector3){ eyePos.x + fwd.x, eyePos.y + fwd.y, eyePos.z + fwd.z };
    g_camAtual.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    g_camAtual.fovy       = 68.0f;
    g_camAtual.projection = CAMERA_PERSPECTIVE;
}

static void AtualizarCameraSeguindo(const TerreoSalaDef *def, float dt, bool snap) {
    if (!def) return;
    if (DevPrimeiraPessoaAtiva()) {
        AtualizarCameraPrimeiraPessoa(def, dt, true);
        return;
    }
    float cx = def->boundsXZ.x + def->boundsXZ.width * 0.5f;
    float cz = def->boundsXZ.y + def->boundsXZ.height * 0.5f;
    float halfW = def->boundsXZ.width * 0.5f;
    float halfD = def->boundsXZ.height * 0.5f;

    const float camOffY = 5.45f;
    const float camOffZ = 7.45f;
    const float camFovy = 41.0f;

    if (SettingsGetCamera3DFixa()) {
        // Camera fixa em angulo alto da sala, fazendo tracking suave na posicao relativa do player
        float targetX = g_player.position.x;
        float targetY = 0.75f + g_player.position.y * 0.45f;
        float targetZ = g_player.position.z - 0.15f;
        Vector3 fixedPos = { cx, 0.85f + camOffY + 0.5f, cz + fmaxf(halfD + 2.4f, 5.8f) };

        if (snap) {
            g_camAtual.target     = (Vector3){ targetX, targetY, targetZ };
            g_camAtual.position   = fixedPos;
            g_camAtual.up         = def->camera.up;
            g_camAtual.fovy       = camFovy;
            g_camAtual.projection = CAMERA_PERSPECTIVE;
            return;
        }
        float t = 1.0f - expf(-8.0f * dt);
        g_camAtual.target.x += (targetX - g_camAtual.target.x) * t;
        g_camAtual.target.y += (targetY - g_camAtual.target.y) * t;
        g_camAtual.target.z += (targetZ - g_camAtual.target.z) * t;
        g_camAtual.position   = fixedPos;
        g_camAtual.fovy       = camFovy;
        return;
    }

    // Permite ver um pouco do preto alem das paredes nas extremidades sem cortar demais a sala
    float maxOffX = fmaxf(0.0f, halfW - 2.65f);
    float maxOffZ = fmaxf(0.0f, halfD - 2.15f);
    float targetX = Clamp(g_player.position.x, cx - maxOffX, cx + maxOffX);
    float targetZ = Clamp(g_player.position.z - 0.55f, cz - maxOffZ, cz + maxOffZ);
    float targetY = 0.85f + g_player.position.y * 0.45f;

    if (snap) {
        g_camAtual.target     = (Vector3){ targetX, targetY, targetZ };
        g_camAtual.position   = (Vector3){ targetX, targetY + camOffY, targetZ + camOffZ };
        g_camAtual.up         = def->camera.up;
        g_camAtual.fovy       = camFovy;
        g_camAtual.projection = CAMERA_PERSPECTIVE;
        return;
    }

    float t = 1.0f - expf(-6.8f * dt);
    g_camAtual.target.x += (targetX - g_camAtual.target.x) * t;
    g_camAtual.target.y += (targetY - g_camAtual.target.y) * t;
    g_camAtual.target.z += (targetZ - g_camAtual.target.z) * t;
    g_camAtual.position.x = g_camAtual.target.x;
    g_camAtual.position.y = g_camAtual.target.y + camOffY;
    g_camAtual.position.z = g_camAtual.target.z + camOffZ;
    g_camAtual.fovy       = camFovy;
}

void Terreo3DDefinirSala(TerreoSalaId sala, TerreoDestino vindoDe) {
    s_mousePendTipo = 0;
    s_pathCount = 0;
    s_pathIdx   = 0;
    PlayerCharacterClearMoveTarget(&g_player);
    if (sala < 0 || sala >= TERREO_NUM_SALAS) sala = TERREO_SALA_RECEPCAO;
    g_salaAtual = sala;
    const TerreoSalaDef *def = g_salas[sala];
    if (!def) return;

    Vector3 pos = def->defaultSpawnPos;
    float   yaw = def->defaultSpawnYaw;

    if (sala == TERREO_SALA_RECEPCAO) {
        if (vindoDe == TERREO_DEST_LANCHONETE) {
            pos = (Vector3){ -4.70f, 0.0f, 0.45f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_ESCRITORIO) {
            pos = (Vector3){ 4.70f, 0.0f, 1.35f };
            yaw = -PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_ESCADA_2D ||
                   vindoDe == TERREO_DEST_HALL_1 ||
                   vindoDe == TERREO_DEST_HALL_2 ||
                   vindoDe == TERREO_DEST_HALL_3 ||
                   vindoDe == TERREO_DEST_HALL_4 ||
                   vindoDe == TERREO_DEST_SUBSOLO ||
                   vindoDe == TERREO_DEST_SUBSOLO_2) {
            pos = (Vector3){ 4.35f, 0.0f, -2.90f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_SUBSOLO) {
        if (vindoDe == TERREO_DEST_SUBSOLO_2) {
            pos = (Vector3){ 4.95f, 0.0f, 1.65f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_LANCHONETE) {
        if (vindoDe == TERREO_DEST_RECEPCAO) {
            pos = (Vector3){ 2.95f, 0.0f, 4.20f };
            yaw = -PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_JARDIM) {
            pos = (Vector3){ 2.95f, 0.0f, -4.20f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_JARDIM) {
        if (vindoDe == TERREO_DEST_LANCHONETE) {
            pos = (Vector3){ -4.90f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_ESCRITORIO || vindoDe == TERREO_DEST_CONVIVIO) {
            pos = (Vector3){ 4.90f, 0.0f, 0.0f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_ESCRITORIO) {
        if (vindoDe == TERREO_DEST_RECEPCAO) {
            pos = (Vector3){ -4.65f, 0.0f, 1.65f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_JARDIM || vindoDe == TERREO_DEST_CONVIVIO) {
            pos = (Vector3){ 0.0f, 0.0f, -2.95f };
            yaw = 0.0f;
        } else if (vindoDe == TERREO_DEST_ESCRITORIO_2) {
            pos = (Vector3){ 4.55f, 0.0f, -0.85f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_ESCRITORIO_2) {
        if (vindoDe == TERREO_DEST_ESCRITORIO) {
            pos = (Vector3){ -4.55f, 0.0f, -0.85f };
            yaw = PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_CONVIVIO) {
        if (vindoDe == TERREO_DEST_JARDIM) {
            pos = (Vector3){ -3.85f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_ESCRITORIO) {
            pos = (Vector3){ 0.0f, 0.0f, 2.55f };
            yaw = PI;
        }
    } else if (sala == TERREO_SALA_HALL_1) {
        if (vindoDe == TERREO_DEST_RECEPCAO || vindoDe == TERREO_DEST_ESCADA_1 ||
            vindoDe == TERREO_DEST_HALL_2 || vindoDe == TERREO_DEST_HALL_3 ||
            vindoDe == TERREO_DEST_HALL_4 || vindoDe == TERREO_DEST_SUBSOLO) {
            pos = (Vector3){ 3.55f, 0.0f, -2.15f };
            yaw = 0.0f;
        } else if (vindoDe == TERREO_DEST_CONTABILIDADE_1) {
            pos = (Vector3){ 4.55f, 0.0f, 1.10f };
            yaw = -PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_COPA_1) {
            pos = (Vector3){ -4.55f, 0.0f, 0.45f };
            yaw = PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_CONTABILIDADE_1) {
        if (vindoDe == TERREO_DEST_HALL_1) {
            pos = (Vector3){ -4.65f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_CONTABILIDADE_2) {
            pos = (Vector3){ 4.65f, 0.0f, 0.0f };
            yaw = -PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_ARQUIVO_1) {
            pos = (Vector3){ 0.0f, 0.0f, -2.95f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_CONTABILIDADE_2) {
        if (vindoDe == TERREO_DEST_CONTABILIDADE_1 || vindoDe == TERREO_DEST_HALL_1) {
            pos = (Vector3){ -4.85f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_ARQUIVO_1) {
        if (vindoDe == TERREO_DEST_CONTABILIDADE_1) {
            pos = (Vector3){ 0.0f, 0.0f, 2.75f };
            yaw = PI;
        } else if (vindoDe == TERREO_DEST_SALA_CAMERAS) {
            pos = (Vector3){ 3.85f, 0.0f, 2.25f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_CAMERAS) {
        if (vindoDe == TERREO_DEST_ARQUIVO_1) {
            pos = (Vector3){ -2.55f, 0.0f, 1.50f };
            yaw = PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_COPA_1) {
        if (vindoDe == TERREO_DEST_HALL_1) {
            pos = (Vector3){ 3.95f, 0.0f, 0.45f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_HALL_2) {
        if (vindoDe == TERREO_DEST_CIBER_1) {
            pos = (Vector3){ -4.35f, 0.0f, 0.45f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_CIBER_2) {
            pos = (Vector3){ 4.35f, 0.0f, 1.10f };
            yaw = -PI * 0.5f;
        } else {
            pos = (Vector3){ 3.45f, 0.0f, -2.15f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_CIBER_1) {
        if (vindoDe == TERREO_DEST_HALL_2) {
            pos = (Vector3){ 4.35f, 0.0f, 0.45f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_CIBER_2) {
        if (vindoDe == TERREO_DEST_HALL_2) {
            pos = (Vector3){ -4.65f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_SERVER_1) {
            pos = (Vector3){ 0.0f, 0.0f, -2.85f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_SERVER_1) {
        if (vindoDe == TERREO_DEST_CIBER_2) {
            pos = (Vector3){ 0.0f, 0.0f, 2.65f };
            yaw = PI;
        } else if (vindoDe == TERREO_DEST_SERVER_2) {
            pos = (Vector3){ 3.65f, 0.0f, 0.0f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_SERVER_2) {
        if (vindoDe == TERREO_DEST_SERVER_1) {
            pos = (Vector3){ -3.25f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_HALL_3) {
        if (vindoDe == TERREO_DEST_TREINO_ESCRITORIO) {
            pos = (Vector3){ -4.35f, 0.0f, 0.45f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_TREINO_IA) {
            pos = (Vector3){ 4.35f, 0.0f, 1.10f };
            yaw = -PI * 0.5f;
        } else {
            pos = (Vector3){ 3.45f, 0.0f, -2.15f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_TREINO_ESCRITORIO) {
        if (vindoDe == TERREO_DEST_HALL_3) {
            pos = (Vector3){ 4.35f, 0.0f, 0.45f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_TREINO_IA) {
        if (vindoDe == TERREO_DEST_HALL_3) {
            pos = (Vector3){ -4.45f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_GARAGEM_PROTOTIPOS) {
            pos = (Vector3){ 0.35f, 0.0f, -2.85f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_GARAGEM_PROTOTIPOS) {
        if (vindoDe == TERREO_DEST_TREINO_IA) {
            pos = (Vector3){ 0.0f, 0.0f, 2.65f };
            yaw = PI;
        }
    } else if (sala == TERREO_SALA_HALL_4) {
        if (vindoDe == TERREO_DEST_AUDITORIO) {
            pos = (Vector3){ -4.45f, 0.0f, 0.45f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_REUNIAO_1) {
            pos = (Vector3){ 4.35f, 0.0f, 1.10f };
            yaw = -PI * 0.5f;
        } else {
            pos = (Vector3){ 3.45f, 0.0f, -2.15f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_AUDITORIO) {
        if (vindoDe == TERREO_DEST_HALL_4) {
            pos = (Vector3){ 4.95f, 0.0f, 0.45f };
            yaw = -PI * 0.5f;
        }
    } else if (sala == TERREO_SALA_REUNIAO_1) {
        if (vindoDe == TERREO_DEST_HALL_4) {
            pos = (Vector3){ -3.85f, 0.0f, 0.0f };
            yaw = PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_REUNIAO_2) {
            pos = (Vector3){ 0.0f, 0.0f, -2.65f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_REUNIAO_2) {
        if (vindoDe == TERREO_DEST_REUNIAO_1) {
            pos = (Vector3){ 0.0f, 0.0f, 2.65f };
            yaw = PI;
        }
    }

    PlayerCharacterInit(&g_player, pos, yaw);
    g_fpYaw = yaw;
    g_fpPitch = 0.0f;
    AtualizarCameraSeguindo(def, 0.0f, true);
    g_cooldownEntrada = 0.45f;
    g_mouseSobreInteragivel = false;
    g_hasProxInteragivel = false;
    g_hasClosestMouseTarget = false;
    g_dicaAcao[0] = '\0';
}

void Terreo3DSetEvidenciaAtiva(bool evRelatoriosAtiva) {
    g_evRelatoriosAtiva = evRelatoriosAtiva;
}

void Terreo3DSetSuspeitosSala(const bool presentes[3]) {
    for (int i = 0; i < 3; i++) {
        g_suspeitosNaSala[i] = presentes ? presentes[i] : false;
    }
}

void Terreo3DTriggerPlayerAnim(Vector3 targetPos, PlayerInteractAnim tipo, float duracao) {
    PlayerCharacterTriggerInteractEx(&g_player, targetPos, tipo, duracao);
}

void Terreo3DTriggerComputerSit(Vector3 seatPos, Vector3 monitorPos, float duracao) {
    PlayerCharacterTriggerComputerSit(&g_player, seatPos, monitorPos, duracao);
}

void Terreo3DTriggerComputerExit(void) {
    PlayerCharacterTriggerComputerExit(&g_player);
}

void Terreo3DTriggerPaperPickup(Vector3 paperPos, float duracao) {
    PlayerCharacterTriggerPaperPickup(&g_player, paperPos, duracao);
}

void Terreo3DTriggerPaperDrop(Vector3 paperPos, float duracao) {
    PlayerCharacterTriggerPaperDrop(&g_player, paperPos, duracao);
}

void Terreo3DSetGuiaInfo(bool temAlgoNoAndar, uint64_t salasAlvoMask) {
    g_guiaTemAlgoNoAndar = temAlgoNoAndar;
    g_guiaSalasAlvoMask  = salasAlvoMask;
}

static TerreoSalaId SalaPorDestino3D(TerreoDestino d) {
    switch (d) {
        case TERREO_DEST_RECEPCAO:           return TERREO_SALA_RECEPCAO;
        case TERREO_DEST_LANCHONETE:         return TERREO_SALA_LANCHONETE;
        case TERREO_DEST_JARDIM:             return TERREO_SALA_JARDIM;
        case TERREO_DEST_ESCRITORIO:         return TERREO_SALA_ESCRITORIO;
        case TERREO_DEST_ESCRITORIO_2:       return TERREO_SALA_ESCRITORIO_2;
        case TERREO_DEST_CONVIVIO:           return TERREO_SALA_CONVIVIO;
        case TERREO_DEST_HALL_1:             return TERREO_SALA_HALL_1;
        case TERREO_DEST_CONTABILIDADE_1:    return TERREO_SALA_CONTABILIDADE_1;
        case TERREO_DEST_CONTABILIDADE_2:    return TERREO_SALA_CONTABILIDADE_2;
        case TERREO_DEST_ARQUIVO_1:          return TERREO_SALA_ARQUIVO_1;
        case TERREO_DEST_SALA_CAMERAS:       return TERREO_SALA_CAMERAS;
        case TERREO_DEST_COPA_1:             return TERREO_SALA_COPA_1;
        case TERREO_DEST_HALL_2:             return TERREO_SALA_HALL_2;
        case TERREO_DEST_CIBER_1:            return TERREO_SALA_CIBER_1;
        case TERREO_DEST_CIBER_2:            return TERREO_SALA_CIBER_2;
        case TERREO_DEST_SERVER_1:           return TERREO_SALA_SERVER_1;
        case TERREO_DEST_SERVER_2:           return TERREO_SALA_SERVER_2;
        case TERREO_DEST_HALL_3:             return TERREO_SALA_HALL_3;
        case TERREO_DEST_TREINO_ESCRITORIO:  return TERREO_SALA_TREINO_ESCRITORIO;
        case TERREO_DEST_TREINO_IA:          return TERREO_SALA_TREINO_IA;
        case TERREO_DEST_GARAGEM_PROTOTIPOS: return TERREO_SALA_GARAGEM_PROTOTIPOS;
        case TERREO_DEST_HALL_4:             return TERREO_SALA_HALL_4;
        case TERREO_DEST_AUDITORIO:          return TERREO_SALA_AUDITORIO;
        case TERREO_DEST_REUNIAO_1:          return TERREO_SALA_REUNIAO_1;
        case TERREO_DEST_REUNIAO_2:          return TERREO_SALA_REUNIAO_2;
        case TERREO_DEST_SUBSOLO:            return TERREO_SALA_SUBSOLO;
        case TERREO_DEST_SUBSOLO_2:          return TERREO_SALA_SUBSOLO_2;
        default:                             return TERREO_NUM_SALAS;
    }
}

static bool SalaTemElevador(TerreoSalaId sid) {
    if (sid < 0 || sid >= TERREO_NUM_SALAS || !g_salas[sid]) return false;
    const TerreoSalaDef *d = g_salas[sid];
    for (int i = 0; i < d->numInteractables; i++) {
        if (d->interactables[i].id && strcmp(d->interactables[i].id, "elevador") == 0) return true;
    }
    return false;
}

// BFS no grafo de portas do andar atual: retorna o indice da porta em 'inicio'
// que leva pelo caminho mais curto ate uma sala em salasAlvoMask (ou ao elevador).
static int GuiaEncontrarPortaBFS(TerreoSalaId inicio, uint64_t salasAlvoMask, bool buscarElevador) {
    if (inicio < 0 || inicio >= TERREO_NUM_SALAS || !g_salas[inicio]) return -1;
    int firstDoor[TERREO_NUM_SALAS];
    bool vis[TERREO_NUM_SALAS];
    TerreoSalaId q[TERREO_NUM_SALAS];
    for (int i = 0; i < TERREO_NUM_SALAS; i++) { firstDoor[i] = -1; vis[i] = false; }

    int head = 0, tail = 0;
    vis[inicio] = true;
    const TerreoSalaDef *s0 = g_salas[inicio];
    for (int p = 0; p < s0->numPortas; p++) {
        TerreoSalaId nxt = SalaPorDestino3D(s0->portas[p].destino);
        if (nxt < 0 || nxt >= TERREO_NUM_SALAS || !g_salas[nxt] || vis[nxt]) continue;
        vis[nxt] = true;
        firstDoor[nxt] = p;
        q[tail++] = nxt;
    }

    while (head < tail) {
        TerreoSalaId cur = q[head++];
        if (buscarElevador) {
            if (SalaTemElevador(cur)) return firstDoor[cur];
        } else {
            if (((salasAlvoMask >> (int)cur) & 1ULL) != 0) return firstDoor[cur];
        }
        const TerreoSalaDef *sc = g_salas[cur];
        if (!sc) continue;
        for (int p = 0; p < sc->numPortas; p++) {
            TerreoSalaId nxt = SalaPorDestino3D(sc->portas[p].destino);
            if (nxt < 0 || nxt >= TERREO_NUM_SALAS || !g_salas[nxt] || vis[nxt]) continue;
            vis[nxt] = true;
            firstDoor[nxt] = firstDoor[cur];
            q[tail++] = nxt;
        }
    }
    return -1;
}

float Terreo3DGetChairPull(Vector3 chairPos) {
    return PlayerCharacterGetChairPull(&g_player, chairPos);
}

static void PosicaoSuspeitoNaSala(TerreoSalaId sala, int s, Vector3 *outPos, float *outYawDeg, NpcAtividade *outAtiv) {
    static const Vector3 POS_PADRAO[3] = {
        { -1.80f, 0.0f,  0.80f },
        {  0.00f, 0.0f,  0.80f },
        {  1.80f, 0.0f,  0.80f }
    };
    Vector3 p = POS_PADRAO[s % 3];
    float yaw = 0.0f;

    if (sala == TERREO_SALA_RECEPCAO) {
        static const Vector3 pts[3] = { { 1.60f, 0, -1.20f }, { -1.20f, 0, 1.35f }, { 2.85f, 0, 1.10f } };
        static const float yaws[3]  = { -35.0f, 25.0f, -120.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_LANCHONETE) {
        // Cantina: Arthur mais no cantinho superior esquerdo, bem longe das mesas
        static const Vector3 pts[3] = { { -1.40f, 0, -2.10f }, { -2.35f, 0, -4.85f }, { -1.35f, 0, 3.35f } };
        static const float yaws[3]  = { 35.0f, 35.0f, 25.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_JARDIM) {
        // Jardim: em pe pelas trilhas
        static const Vector3 pts[3] = { { -2.10f, 0, 0.85f }, { 0.85f, 0, 1.95f }, { 2.45f, 0, -0.95f } };
        static const float yaws[3]  = { 35.0f, -120.0f, -45.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_ESCRITORIO) {
        // ADM 1: corredores livres (baias ocupam x:-2.45..1.95, z:-1.25..0.15 e z:1.15..2.55)
        static const Vector3 pts[3] = { { -0.60f, 0, 0.65f }, { 3.15f, 0, 1.25f }, { 2.85f, 0, -2.05f } };
        static const float yaws[3]  = { 15.0f, -135.0f, -35.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_ESCRITORIO_2) {
        // ADM 2: corredores livres longe das baias e da parede direita (x:4.35..5.80)
        static const Vector3 pts[3] = { { -0.50f, 0, 0.65f }, { 3.15f, 0, 1.45f }, { 3.15f, 0, -1.65f } };
        static const float yaws[3]  = { 15.0f, -145.0f, -35.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_CONVIVIO) {
        // Convivio: em pe na area aberta, longe do sofa e das mesas
        static const Vector3 pts[3] = { { -1.80f, 0, 1.25f }, { 0.20f, 0, 1.35f }, { 2.20f, 0, 0.85f } };
        static const float yaws[3]  = { 25.0f, 180.0f, -35.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_CONTABILIDADE_1) {
        // Contabilidade 1: corredores amplos sem cruzar blocos de baias
        static const Vector3 pts[3] = { { -1.80f, 0, -0.05f }, { 3.85f, 0, 0.35f }, { 3.85f, 0, -1.55f } };
        static const float yaws[3]  = { 15.0f, -110.0f, -65.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_CONTABILIDADE_2) {
        // Contabilidade 2: longe da mesa comprida central (x:-1.40..3.20, z:-1.05..1.25)
        static const Vector3 pts[3] = { { -2.65f, 0, 0.25f }, { 0.60f, 0, 2.15f }, { 0.85f, 0, -2.05f } };
        static const float yaws[3]  = { 75.0f, 180.0f, 0.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_ARQUIVO_1) {
        static const Vector3 pts[3] = { { -2.05f, 0, 1.35f }, { 2.05f, 0, 1.35f }, { 0.00f, 0, -1.75f } };
        static const float yaws[3]  = { 135.0f, -135.0f, 0.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_COPA_1) {
        // Copa 1: longe das mesas redondas (z:0.30..2.00) e da bancada (z:-3.50..-2.50)
        static const Vector3 pts[3] = { { -2.10f, 0, -1.20f }, { 3.25f, 0, 1.40f }, { -0.35f, 0, -0.75f } };
        static const float yaws[3]  = { 20.0f, -130.0f, 10.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_CIBER_1 || sala == TERREO_SALA_CIBER_2 ||
               sala == TERREO_SALA_TREINO_ESCRITORIO || sala == TERREO_SALA_TREINO_IA) {
        static const Vector3 pts[3] = { { -1.60f, 0, 0.40f }, { 0.00f, 0, 0.40f }, { 1.60f, 0, 0.40f } };
        static const float yaws[3]  = { 25.0f, 0.0f, -25.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_SERVER_1 || sala == TERREO_SALA_SERVER_2 || sala == TERREO_SALA_GARAGEM_PROTOTIPOS) {
        static const Vector3 pts[3] = { { -1.85f, 0, -0.85f }, { 1.85f, 0, -0.85f }, { 0.00f, 0, 1.35f } };
        static const float yaws[3]  = { 90.0f, -90.0f, 180.0f };
        p = pts[s]; yaw = yaws[s];
    } else if (sala == TERREO_SALA_AUDITORIO || sala == TERREO_SALA_REUNIAO_1 || sala == TERREO_SALA_REUNIAO_2) {
        static const Vector3 pts[3] = { { -1.65f, 0, 0.65f }, { 0.00f, 0, 0.65f }, { 1.65f, 0, 0.65f } };
        static const float yaws[3]  = { 180.0f, 180.0f, 180.0f };
        p = pts[s]; yaw = yaws[s];
    }

    // Todos os suspeitos ficam sempre em pe com comportamento proprio:
    // 0 = Maria (supervisionando arquivos), 1 = Arthur (carregando papeis ou cocando a cabeca na copa/lanchonete), 2 = Ana Lucia (desconfiada)
    NpcAtividade ativ = NPC_ATIV_EM_PE;
    if (s == 0)      ativ = NPC_ATIV_LENDO;
    else if (s == 1) ativ = (sala == TERREO_SALA_COPA_1 || sala == TERREO_SALA_LANCHONETE) ? NPC_ATIV_COCANDO_CABECA : NPC_ATIV_CARREGANDO_PAPEIS;
    else             ativ = NPC_ATIV_DESCONFIADA;

    if (outPos)    *outPos = p;
    if (outYawDeg) *outYawDeg = yaw;
    if (outAtiv)   *outAtiv = ativ;
}

typedef struct {
    TerreoSalaId sala;
    Vector3      pos;
    float        yawDeg;
    NpcAtividade ativ;
    int          variante;
} ExtraNpcDef;

// Funcionarios adicionais nao interagiveis posicionados em cadeiras, sofas ou cantos discretos
static const ExtraNpcDef EXTRA_NPCS[] = {
    // Recepcao: perto da janela do jardim olhando para frente
    { TERREO_SALA_RECEPCAO,         {  0.70f, 0.0f, -3.55f },   0.0f, NPC_ATIV_LENDO,      1 },
    // Lanchonete (ampla): sentados exatamente nas cadeiras das mesas redondas
    { TERREO_SALA_LANCHONETE,       {  1.35f + 0.62f, 0.0f, -3.60f },  -90.0f, NPC_ATIV_COMENDO,    0 },
    { TERREO_SALA_LANCHONETE,       {  1.35f - 0.62f, 0.0f, -0.40f },   90.0f, NPC_ATIV_COMENDO,    3 },
    { TERREO_SALA_LANCHONETE,       { -1.15f + 0.62f, 0.0f,  1.40f },  -90.0f, NPC_ATIV_COMENDO,    2 },
    // Area de Convivio: 2 NPCs sentados no sofa redondo encostado na parede
    { TERREO_SALA_CONVIVIO,         { -1.65f, 0.0f, -2.55f },   18.0f, NPC_ATIV_SENTADO,    1 },
    { TERREO_SALA_CONVIVIO,         { -0.35f, 0.0f, -2.55f },  -18.0f, NPC_ATIV_SENTADO,    2 },
    // Escritorios: sentados exatamente nas cadeiras das estacoes de trabalho
    { TERREO_SALA_ESCRITORIO,       { -1.65f, 0.0f,  2.35f },  180.0f, NPC_ATIV_COMPUTADOR, 1 },
    { TERREO_SALA_ESCRITORIO_2,     {  4.58f, 0.0f, -1.60f },   90.0f, NPC_ATIV_COMPUTADOR, 2 },
    // Contabilidades: sentados exatamente nas cadeiras reais
    { TERREO_SALA_CONTABILIDADE_1,  { -1.80f, 0.0f,  0.70f },    0.0f, NPC_ATIV_COMPUTADOR, 0 },
    { TERREO_SALA_CONTABILIDADE_2,  {  0.90f, 0.0f,  0.95f },  180.0f, NPC_ATIV_COMPUTADOR, 3 },
    // Ciber: sentados nas cadeiras das fileiras
    { TERREO_SALA_CIBER_1,          {  1.95f, 0.0f, -2.05f },  180.0f, NPC_ATIV_COMPUTADOR, 1 },
    { TERREO_SALA_CIBER_2,          { -1.45f, 0.0f, -1.65f },    0.0f, NPC_ATIV_COMPUTADOR, 4 },
    // Auditorio: sentados nas fileiras de cadeiras
    { TERREO_SALA_AUDITORIO,        { -2.20f, 0.0f,  1.20f },  180.0f, NPC_ATIV_SENTADO,    0 },
    { TERREO_SALA_AUDITORIO,        {  2.20f, 0.0f,  1.20f },  180.0f, NPC_ATIV_SENTADO,    1 },
    // Arquivo: canto discreto lendo
    { TERREO_SALA_ARQUIVO_1,        { -3.10f, 0.0f, -2.35f },   45.0f, NPC_ATIV_LENDO,      3 },
    // Halls: sentados nos sofas curvos + recepcionista confinada dentro do balcao redondo em U
    { TERREO_SALA_HALL_1,           { -0.45f, 0.0f,  1.15f },  180.0f, NPC_ATIV_SENTADO,    2 },
    { TERREO_SALA_HALL_1,           { -3.85f, 0.0f, -2.40f },    0.0f, NPC_ATIV_COMPUTADOR, 1 },
    { TERREO_SALA_HALL_2,           { -0.35f, 0.0f,  1.15f },  180.0f, NPC_ATIV_SENTADO,    4 },
    { TERREO_SALA_HALL_2,           { -3.75f, 0.0f, -2.40f },    0.0f, NPC_ATIV_COMPUTADOR, 3 },
    { TERREO_SALA_HALL_3,           { -0.35f, 0.0f,  1.15f },  180.0f, NPC_ATIV_SENTADO,    1 },
    { TERREO_SALA_HALL_3,           { -3.75f, 0.0f, -2.40f },    0.0f, NPC_ATIV_COMPUTADOR, 5 },
    { TERREO_SALA_HALL_4,           { -0.35f, 0.0f,  1.15f },  180.0f, NPC_ATIV_SENTADO,    3 },
    { TERREO_SALA_HALL_4,           { -2.55f, 0.0f, -2.95f },    0.0f, NPC_ATIV_COMPUTADOR, 1 },
    // Treino IA / Escritorio
    { TERREO_SALA_TREINO_ESCRITORIO,{  2.40f, 0.0f, -0.55f },  180.0f, NPC_ATIV_COMPUTADOR, 2 },
    { TERREO_SALA_TREINO_IA,        { -1.55f, 0.0f,  0.85f },  180.0f, NPC_ATIV_COMPUTADOR, 5 },
};

void TerreoTV(Vector3 pos, float rotYDeg, float tempo) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);
    // Suporte traseiro na parede
    TerreoCaixa((Vector3){ 0.0f, 0.0f, -0.02f }, (Vector3){ 0.42f, 0.24f, 0.06f }, 0.16f, 0.14f, 0.14f, -1.0f);
    // Moldura escura da TV widescreen
    TerreoCaixa((Vector3){ 0.0f, 0.0f,  0.04f }, (Vector3){ 1.46f, 0.84f, 0.06f }, 0.14f, 0.12f, 0.12f, 0.24f);
    // Tela acesa com brilho suave animado
    float pulso = 0.72f + 0.06f * sinf(tempo * 2.4f);
    TerreoCaixa((Vector3){ 0.0f, 0.02f, 0.075f }, (Vector3){ 1.34f, 0.68f, 0.015f }, pulso, pulso, pulso * 0.9f, -1.0f);
    // Faixa de rodape estilo telejornal corporativo
    float barraX = -0.35f + 0.70f * (0.5f + 0.5f * sinf(tempo * 0.9f));
    TerreoCaixa((Vector3){ 0.0f, -0.25f, 0.086f }, (Vector3){ 1.32f, 0.11f, 0.01f }, 0.24f, 0.22f, 0.22f, -1.0f);
    TerreoCaixa((Vector3){ barraX, -0.25f, 0.092f }, (Vector3){ 0.34f, 0.05f, 0.008f }, 0.92f, 0.92f, 0.92f, -1.0f);
    // Grafico/bloco de apresentacao no canto da tela
    TerreoCaixa((Vector3){ -0.34f, 0.08f, 0.086f }, (Vector3){ 0.44f, 0.30f, 0.008f }, 0.45f, 0.42f, 0.42f, 0.18f);
    TerreoCaixa((Vector3){  0.26f, 0.15f, 0.086f }, (Vector3){ 0.50f, 0.07f, 0.008f }, 0.30f, 0.28f, 0.28f, -1.0f);
    TerreoCaixa((Vector3){  0.22f, 0.02f, 0.086f }, (Vector3){ 0.42f, 0.06f, 0.008f }, 0.36f, 0.34f, 0.34f, -1.0f);
    rlPopMatrix();
}

static PlayerPreviewAnim g_subsoloAnims[4] = {
    PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE
};

static PlayerPreviewAnim g_subsolo2Anims[6] = {
    PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE,
    PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE, PREVIEW_ANIM_IDLE
};

void TerreoDesenharPlayerPreview(bool isFemale, Vector3 pos, float yawDeg, float tempo) {
    PlayerCharacterDrawModelPreview3D(isFemale, pos, yawDeg, tempo, g_camAtual.position);
}

void TerreoDesenharPlayerVariant(int variantId, PlayerPreviewAnim anim, Vector3 pos, float yawDeg, float tempo) {
    PlayerCharacterDrawVariant3D(variantId, anim, pos, yawDeg, tempo, g_camAtual.position);
}

void TerreoDesenharPlayerTechVariant(int techId, PlayerPreviewAnim anim, Vector3 pos, float yawDeg, float tempo) {
    PlayerCharacterDrawTechVariant3D(techId, anim, pos, yawDeg, tempo, g_camAtual.position);
}

PlayerPreviewAnim TerreoSubsoloGetAnim(int idx) {
    if (idx < 0 || idx >= 4) return PREVIEW_ANIM_IDLE;
    return g_subsoloAnims[idx];
}

PlayerPreviewAnim TerreoSubsoloCiclarAnim(int idx) {
    if (idx < 0 || idx >= 4) return PREVIEW_ANIM_IDLE;
    g_subsoloAnims[idx] = (PlayerPreviewAnim)((g_subsoloAnims[idx] + 1) % PREVIEW_ANIM_COUNT);
    return g_subsoloAnims[idx];
}

PlayerPreviewAnim TerreoSubsolo2GetAnim(int idx) {
    if (idx < 0 || idx >= 6) return PREVIEW_ANIM_IDLE;
    return g_subsolo2Anims[idx];
}

PlayerPreviewAnim TerreoSubsolo2CiclarAnim(int idx) {
    if (idx < 0 || idx >= 6) return PREVIEW_ANIM_IDLE;
    g_subsolo2Anims[idx] = (PlayerPreviewAnim)((g_subsolo2Anims[idx] + 1) % PREVIEW_ANIM_COUNT);
    return g_subsolo2Anims[idx];
}

void Terreo3DAtualizarAvatarPlayer(void) {
    g_player.isFemale = GameStateIsPlayerFemale();
}

static void TerreoDesenharFuncionariosExtras(TerreoSalaId sala, float tempo) {
    int count = (int)(sizeof(EXTRA_NPCS) / sizeof(EXTRA_NPCS[0]));
    for (int i = 0; i < count; i++) {
        if (EXTRA_NPCS[i].sala != sala) continue;
        PlayerCharacterDrawNpc3DEx(EXTRA_NPCS[i].pos,
                                   EXTRA_NPCS[i].yawDeg,
                                   EXTRA_NPCS[i].ativ,
                                   EXTRA_NPCS[i].variante,
                                   tempo,
                                   g_camAtual.position);
    }
}

// Desenha um objeto de evidencia 3D seguindo a paleta de 5 tons do jogo com pulso luminoso elegante
// tipoModelo: 0=pasta/rascunho, 1=prancheta/protocolo, 2=oficio/envelope, 3=terminal/tela, 4=leitor/log
void TerreoEvidenciaBrilhante(Vector3 pos, float tempo, int tipoModelo) {
    float pulso = 0.5f + 0.5f * sinf(tempo * 4.2f);
    unsigned char hi = (unsigned char)(210 + (int)(45.0f * pulso));
    Color cForte  = (Color){ hi, (unsigned char)(hi * 242 / 255), (unsigned char)(hi * 230 / 255), 255 };
    Color cBrilho = (Color){ 189, 181, 168, 255 };
    Color cBorda  = (Color){ 125, 112, 113, 255 };

    // Anel luminoso pulsante na base do item
    float raioAnel = 0.22f + 0.04f * pulso;
    DrawCylinderWires((Vector3){ pos.x, pos.y + 0.015f, pos.z }, raioAnel, raioAnel, 0.012f, 20, cForte);

    if (tipoModelo == 5) {
        // Bolinha de papel amassada (poliedro fino na paleta do jogo)
        DrawSphereEx((Vector3){ pos.x, pos.y + 0.08f, pos.z }, 0.095f, 6, 7, cForte);
        DrawSphereEx((Vector3){ pos.x + 0.03f, pos.y + 0.10f, pos.z - 0.02f }, 0.065f, 5, 6, cBrilho);
        DrawSphereWires((Vector3){ pos.x, pos.y + 0.08f, pos.z }, 0.105f, 6, 7, cBorda);
    } else if (tipoModelo == 3) {
        // Terminal/Monitor fino: tela acesa na paleta marfim/cinza do jogo
        DrawCube((Vector3){ pos.x, pos.y + 0.22f, pos.z + 0.026f }, 0.44f, 0.24f, 0.014f, cForte);
        DrawCubeWires((Vector3){ pos.x, pos.y + 0.22f, pos.z + 0.02f }, 0.48f, 0.27f, 0.035f, cBrilho);
    } else if (tipoModelo == 4) {
        // Leitor/Console fino de Log de Ciberseguranca
        DrawCube((Vector3){ pos.x, pos.y + 0.045f, pos.z }, 0.34f, 0.06f, 0.24f, cBrilho);
        DrawCube((Vector3){ pos.x, pos.y + 0.11f, pos.z - 0.05f }, 0.28f, 0.09f, 0.025f, cForte);
        DrawCubeWires((Vector3){ pos.x, pos.y + 0.07f, pos.z }, 0.36f, 0.14f, 0.26f, cBorda);
    } else if (tipoModelo == 2) {
        // Oficio / Envelope fino lacrado
        DrawCube((Vector3){ pos.x, pos.y + 0.022f, pos.z }, 0.32f, 0.022f, 0.22f, cForte);
        DrawCube((Vector3){ pos.x, pos.y + 0.038f, pos.z + 0.02f }, 0.08f, 0.012f, 0.06f, cBorda);
        DrawCubeWires((Vector3){ pos.x, pos.y + 0.024f, pos.z }, 0.34f, 0.035f, 0.24f, cBrilho);
    } else if (tipoModelo == 1) {
        // Prancheta / Livro fino de Protocolo
        DrawCube((Vector3){ pos.x, pos.y + 0.028f, pos.z }, 0.30f, 0.036f, 0.38f, cBrilho);
        DrawCube((Vector3){ pos.x, pos.y + 0.054f, pos.z - 0.02f }, 0.24f, 0.018f, 0.30f, cForte);
        DrawCubeWires((Vector3){ pos.x, pos.y + 0.036f, pos.z }, 0.32f, 0.055f, 0.40f, cBorda);
    } else {
        // Pasta / Rascunho fino empilhado
        DrawCube((Vector3){ pos.x - 0.04f, pos.y + 0.026f, pos.z }, 0.28f, 0.038f, 0.22f, cBrilho);
        DrawCube((Vector3){ pos.x + 0.04f, pos.y + 0.056f, pos.z + 0.02f }, 0.24f, 0.026f, 0.18f, cForte);
        DrawCubeWires((Vector3){ pos.x, pos.y + 0.040f, pos.z }, 0.34f, 0.070f, 0.26f, cBorda);
    }

    // Beacon flutuante (losango sutil na paleta marfim do jogo)
    float bobY = pos.y + ((tipoModelo == 3) ? 0.64f : 0.42f) + 0.07f * sinf(tempo * 3.8f);
    rlPushMatrix();
    rlTranslatef(pos.x, bobY, pos.z);
    rlRotatef(tempo * 85.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder((Vector3){ 0.0f,  0.00f, 0.0f }, 0.085f, 0.00f, 0.13f, 4, cForte);
    DrawCylinder((Vector3){ 0.0f, -0.13f, 0.0f }, 0.00f, 0.085f, 0.13f, 4, cBrilho);
    rlPopMatrix();
}

static bool EhObjetoInterativoReal(const char *id) {
    if (!id) return false;
    return (strncmp(id, "ev_", 3) == 0 ||
            strcmp(id, "elevador") == 0 ||
            strncmp(id, "recepcionista", 13) == 0 ||
            strncmp(id, "bancada_ciber", 13) == 0 ||
            strstr(id, "terminal") != NULL ||
            strcmp(id, "mesa_relatorios") == 0 ||
            strcmp(id, "fonte_jardim") == 0 ||
            strcmp(id, "banco_jardim") == 0 ||
            strcmp(id, "cafeteira_copa_1") == 0 ||
            strcmp(id, "geladeira_copa_1") == 0 ||
            strcmp(id, "tv_lanchonete") == 0 ||
            strcmp(id, "tv_copa_1") == 0 ||
            strcmp(id, "bancada_vendedores") == 0 ||
            strncmp(id, "mesa_lanchonete_", 16) == 0 ||
            strcmp(id, "sofa_convivio") == 0 ||
            strcmp(id, "balcao_executivo_4") == 0 ||
            strncmp(id, "subsolo_modelo_", 15) == 0 ||
            strncmp(id, "subsolo2_modelo_", 16) == 0);
}

TerreoInteracaoResultado Terreo3DUpdate(float dt,
                                        Vector2 mouseJanela,
                                        bool sobreJanela,
                                        bool podeClicarObjetos,
                                        bool bloqueado) {
    TerreoInteracaoResultado res = { 0 };
    if (!g_pronto) return res;

    const TerreoSalaDef *def = g_salas[g_salaAtual];
    if (!def) return res;

    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;
    if (g_cooldownEntrada > 0.0f) g_cooldownEntrada -= dt;
    g_mouseSobreInteragivel = false;
    g_hasProxInteragivel = false;
    g_hasClosestMouseTarget = false;
    g_dicaAcao[0] = '\0';

    // 1) Atualiza movimento, colisoes com moveis + NPCs em pe e elevacao na escada
    // (Mesmo quando bloqueado por dialogo de pensamento/modal, mantem animacoes rodando e apenas trava input)
    PlayerColliderBox allCols[64];
    int numAllCols = 0;
    if (def->colliders && def->numColliders > 0) {
        int copyN = (def->numColliders < 54) ? def->numColliders : 54;
        for (int i = 0; i < copyN; i++) allCols[numAllCols++] = def->colliders[i];
    }
    int extraCount = (int)(sizeof(EXTRA_NPCS) / sizeof(EXTRA_NPCS[0]));
    for (int i = 0; i < extraCount && numAllCols < 62; i++) {
        if (EXTRA_NPCS[i].sala != g_salaAtual) continue;
        if (EXTRA_NPCS[i].ativ == NPC_ATIV_EM_PE || EXTRA_NPCS[i].ativ == NPC_ATIV_LENDO) {
            Vector3 p = EXTRA_NPCS[i].pos;
            allCols[numAllCols++] = (PlayerColliderBox){ p.x - 0.30f, p.z - 0.30f, p.x + 0.30f, p.z + 0.30f };
        }
    }
    for (int s = 0; s < 3 && numAllCols < 64; s++) {
        if (!g_suspeitosNaSala[s]) continue;
        Vector3 sp = { 0 };
        NpcAtividade sativ = NPC_ATIV_EM_PE;
        PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, NULL, &sativ);
        if (sativ == NPC_ATIV_EM_PE || sativ == NPC_ATIV_LENDO ||
            sativ == NPC_ATIV_CARREGANDO_PAPEIS || sativ == NPC_ATIV_DESCONFIADA) {
            allCols[numAllCols++] = (PlayerColliderBox){ sp.x - 0.30f, sp.z - 0.30f, sp.x + 0.30f, sp.z + 0.30f };
        }
    }

    // Avanca waypoints do caminho A*/BFS no modo Mouse
    if (!bloqueado && s_pathCount > 0 && s_pathIdx < s_pathCount) {
        float dWp = hypotf(s_pathPts[s_pathIdx].x - g_player.position.x,
                           s_pathPts[s_pathIdx].z - g_player.position.z);
        if (dWp <= 0.24f) {
            s_pathIdx++;
            if (s_pathIdx < s_pathCount) {
                PlayerCharacterSetMoveTarget(&g_player, s_pathPts[s_pathIdx]);
            }
        }
    }

    if (SettingsGetDevMode() && !bloqueado && IsKeyPressed(KEY_F1)) {
        SettingsSetDevFirstPerson(!SettingsGetDevFirstPerson());
    }
    bool fpAtivo = DevPrimeiraPessoaAtiva();
    if (fpAtivo && !g_fpPrevAtivo) {
        g_fpYaw = g_player.facingYaw;
        g_fpPitch = 0.0f;
    }

    PlayerCharacterUpdate(&g_player, dt, g_camAtual.position, g_camAtual.target,
                          def->boundsXZ, allCols, numAllCols, bloqueado || (g_portaAnimTimer > 0.0f));
    // Colisao circular real para a fonte central do Jardim
    if (g_salaAtual == TERREO_SALA_JARDIM) {
        float fx = 0.0f, fz = -0.65f;
        float dxF = g_player.position.x - fx;
        float dzF = g_player.position.z - fz;
        float distF = hypotf(dxF, dzF);
        float minDistF = 1.44f + g_player.radius;
        if (distF < minDistF) {
            if (distF < 1e-4f) { dxF = 0.0f; dzF = 1.0f; distF = 1.0f; }
            g_player.position.x = fx + (dxF / distF) * minDistF;
            g_player.position.z = fz + (dzF / distF) * minDistF;
        }
    }
    if (g_portaAnimTimer > 0.0f) {
        g_portaAnimTimer -= dt;
        // Passo suave e calmo atravessando a porta durante a animacao
        float dxP = g_portaAlvoPos.x - g_player.position.x;
        float dzP = g_portaAlvoPos.z - g_player.position.z;
        float distP = hypotf(dxP, dzP);
        if (distP > 0.08f) {
            float step = fminf(distP, 0.85f * dt);
            g_player.position.x += (dxP / distP) * step;
            g_player.position.z += (dzP / distP) * step;
        }
        if (g_portaAnimTimer <= 0.0f) {
            g_portaAnimTimer = 0.0f;
            res.acionouDestino = true;
            res.destino = g_portaDestPendente;
            g_portaDestPendente = TERREO_DEST_NENHUM;
            return res;
        }
        return res;
    }
    if (g_player.isMoving && !bloqueado) {
        float intervalo = (g_player.runBlend > 0.5f) ? 0.25f : 0.36f;
        g_passoTimer += dt;
        if (g_passoTimer >= intervalo) {
            g_passoTimer = 0.0f;
            if (g_sndPasso3D.frameCount > 0) {
                SetSoundVolume(g_sndPasso3D, 0.14f * SettingsGetSfxVolume());
                SetSoundPitch(g_sndPasso3D, 0.92f + ((float)GetRandomValue(0, 16) / 100.0f));
                PlaySound(g_sndPasso3D);
            }
        }
    } else {
        g_passoTimer = 0.28f;
    }
    if (def->heightFunc) {
        g_player.position.y = def->heightFunc(g_player.position.x, g_player.position.z);
    } else {
        g_player.position.y = 0.0f;
    }
    if (fpAtivo) {
        AtualizarCameraPrimeiraPessoa(def, dt, bloqueado);
    } else {
        AtualizarCameraSeguindo(def, dt, !fpAtivo && g_fpPrevAtivo);
    }
    g_fpPrevAtivo = fpAtivo;

    if (bloqueado) {
        return res;
    }

    // 2) Entrada automatica apenas em passagens abertas / lances de escada (portas exigem confirmacao com E)
    if (g_cooldownEntrada <= 0.0f) {
        for (int i = 0; i < def->numPortas; i++) {
            const SalaPorta *pt = &def->portas[i];
            if (pt->label && strncmp(pt->label, "Porta", 5) == 0) continue;
            if (CheckCollisionPointRec((Vector2){ g_player.position.x, g_player.position.z }, pt->walkZoneXZ)) {
                res.acionouDestino = true;
                res.destino = pt->destino;
                return res;
            }
        }
    }

    // 3) Proximidade para tecla E + dica no pensamento do player
    bool somenteMouse = GameStateGetControle3D() == CONTROLE_MOUSE;
    int proxObj = -1;
    TerreoDestino proxPorta = TERREO_DEST_NENHUM;
    const char *proxPortaLabel = NULL;
    float melhorDist = 1e9f;

    for (int i = 0; i < def->numPortas; i++) {
        const SalaPorta *pt = &def->portas[i];
        float d = hypotf(g_player.position.x - pt->triggerCenter.x,
                         g_player.position.z - pt->triggerCenter.z);
        float maxR = fminf(pt->triggerRadius, 1.15f);
        if (d <= maxR && d < melhorDist) {
            melhorDist     = d;
            proxPorta      = pt->destino;
            proxPortaLabel = pt->label;
        }
    }
    Vector2 playerFwd = { sinf(g_player.facingYaw), cosf(g_player.facingYaw) };
    for (int i = 0; i < def->numInteractables; i++) {
        const SalaInteractable *it = &def->interactables[i];
        if (somenteMouse && !EhObjetoInterativoReal(it->id)) continue;
        if (!g_evRelatoriosAtiva && it->id && strcmp(it->id, "mesa_relatorios") == 0) continue;
        if (!EvidenciaDisponivelPorId(it->id)) continue;
        float dx = it->position.x - g_player.position.x;
        float dz = it->position.z - g_player.position.z;
        float d = hypotf(dx, dz);
        bool ehEv = it->id && (strncmp(it->id, "ev_", 3) == 0 || strcmp(it->id, "mesa_relatorios") == 0);
        bool ehRecep = it->id && strncmp(it->id, "recepcionista", 13) == 0;
        bool ehFonte = it->id && strcmp(it->id, "fonte_jardim") == 0;
        float maxR = (ehRecep || ehFonte) ? fminf(it->interactRadius, 2.35f)
                                          : (ehEv ? fminf(it->interactRadius, 1.40f) : fminf(it->interactRadius, 1.35f));
        BoundingBox box = BoxInteractable(it);
        float maisProximoX = fminf(fmaxf(g_player.position.x, box.min.x), box.max.x);
        float maisProximoZ = fminf(fmaxf(g_player.position.z, box.min.z), box.max.z);
        float distanciaBorda = hypotf(g_player.position.x - maisProximoX,
                                      g_player.position.z - maisProximoZ);
        float effDist = ehEv ? (d - 0.25f) : distanciaBorda; // prioriza evidencias sobre moveis proximos
        if (distanciaBorda <= maxR && effDist < melhorDist) {
            if (ehEv && d > 0.15f) {
                float dot = (dx / d) * playerFwd.x + (dz / d) * playerFwd.y;
                if (dot < 0.50f) continue; // precisa estar virado de frente para a evidencia
            }
            // Verifica se ha OUTRO movel/decoracao bloqueando o caminho ate o objeto
            bool bloqueadoMovel = false;
            if (def->colliders && d > 0.25f) {
                for (int c = 0; c < def->numColliders && !bloqueadoMovel; c++) {
                    PlayerColliderBox b = def->colliders[c];
                    bool alvoDentro = (it->position.x >= b.minX - 0.15f && it->position.x <= b.maxX + 0.15f &&
                                       it->position.z >= b.minZ - 0.15f && it->position.z <= b.maxZ + 0.15f);
                    if (alvoDentro) continue;
                    for (int step = 1; step <= 4; step++) {
                        float t = (float)step / 5.0f;
                        float sx = g_player.position.x + dx * t;
                        float sz = g_player.position.z + dz * t;
                        if (sx >= b.minX && sx <= b.maxX && sz >= b.minZ && sz <= b.maxZ) {
                            bloqueadoMovel = true;
                            break;
                        }
                    }
                }
            }
            if (bloqueadoMovel) continue;

            melhorDist = ehEv ? (d - 0.25f) : d;
            proxObj    = i;
            proxPorta  = TERREO_DEST_NENHUM;
        }
    }

    static const char *NOMES_SUSP[3] = { "Maria Bianca", "Arthur", "Ana Lúcia" };
    int proxSusp = -1;
    for (int s = 0; s < 3; s++) {
        if (!g_suspeitosNaSala[s]) continue;
        Vector3 sp = { 0 };
        PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, NULL, NULL);
        float d = hypotf(g_player.position.x - sp.x, g_player.position.z - sp.z);
        if (d <= 1.38f && d < melhorDist) {
            melhorDist = d;
            proxSusp   = s;
            proxObj    = -1;
            proxPorta  = TERREO_DEST_NENHUM;
        }
    }

    // 4) Hover por raycast 3D
    int hoverObj = -1;
    int hoverSusp = -1;
    TerreoDestino hoverPorta = TERREO_DEST_NENHUM;
    const char *hoverPortaLabel = NULL;
    if (sobreJanela && g_texPronta) {
        Ray ray = GetScreenToWorldRayEx(mouseJanela, g_camAtual, g_largura, g_altura);
        float bestHit = 1e9f;

        for (int s = 0; s < 3; s++) {
            if (!g_suspeitosNaSala[s]) continue;
            Vector3 sp = { 0 };
            PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, NULL, NULL);
            BoundingBox bSusp = {
                { sp.x - 0.42f, sp.y,         sp.z - 0.42f },
                { sp.x + 0.42f, sp.y + 2.05f, sp.z + 0.42f }
            };
            RayCollision hit = GetRayCollisionBox(ray, bSusp);
            if (hit.hit && hit.distance < bestHit) {
                bestHit    = hit.distance;
                hoverSusp  = s;
                hoverObj   = -1;
                hoverPorta = TERREO_DEST_NENHUM;
            }
        }

        for (int i = 0; i < def->numPortas; i++) {
            RayCollision hit = GetRayCollisionBox(ray, def->portas[i].clickBox);
            if (hit.hit && hit.distance < bestHit) {
                bestHit         = hit.distance;
                hoverPorta      = def->portas[i].destino;
                hoverPortaLabel = def->portas[i].label;
                hoverSusp       = -1;
            }
        }
        for (int i = 0; i < def->numInteractables; i++) {
            if (!g_evRelatoriosAtiva && def->interactables[i].id &&
                strcmp(def->interactables[i].id, "mesa_relatorios") == 0) continue;
            if (!EvidenciaDisponivelPorId(def->interactables[i].id)) continue;
            // No modo Mouse/Ambos, ignora decoracoes para nao atrapalhar o andar por cliques
            if (!EhObjetoInterativoReal(def->interactables[i].id)) continue;
            const SalaInteractable *it = &def->interactables[i];
            RayCollision hit = (it->id && strcmp(it->id, "fonte_jardim") == 0)
                ? GetRayCollisionSphere(ray, (Vector3){ it->position.x, it->position.y, it->position.z }, 1.38f)
                : GetRayCollisionBox(ray, BoxInteractable(it));
            if (hit.hit && hit.distance < bestHit) {
                bestHit    = hit.distance;
                hoverObj   = i;
                hoverPorta = TERREO_DEST_NENHUM;
                hoverSusp  = -1;
            }
        }
        g_mouseSobreInteragivel = (hoverPorta != TERREO_DEST_NENHUM) || (hoverObj >= 0) || (hoverSusp >= 0);
    }

    ControleModo3D ctrlMode = GameStateGetControle3D();
    bool allowTeclado = (ctrlMode == CONTROLE_TECLADO || ctrlMode == CONTROLE_AMBOS);
    bool allowMouse   = (ctrlMode == CONTROLE_MOUSE   || ctrlMode == CONTROLE_AMBOS);

    g_hasProxInteragivel = (proxSusp >= 0) || (proxPorta != TERREO_DEST_NENHUM) || (proxObj >= 0);

    // Alvo interagivel real mais proximo (para a mini bolinha discreta no tutorial do modo Mouse)
    g_hasClosestMouseTarget = false;
    {
        float bestD = 1e9f;
        for (int s = 0; s < 3; s++) {
            if (!g_suspeitosNaSala[s]) continue;
            Vector3 sp = { 0 };
            PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, NULL, NULL);
            float d = hypotf(g_player.position.x - sp.x, g_player.position.z - sp.z);
            if (d < bestD) {
                bestD = d;
                g_hasClosestMouseTarget = true;
                g_closestMouseTargetPos = (Vector3){ sp.x, sp.y + 1.05f, sp.z };
            }
        }
        for (int i = 0; i < def->numInteractables; i++) {
            const SalaInteractable *it = &def->interactables[i];
            if (!g_evRelatoriosAtiva && it->id && strcmp(it->id, "mesa_relatorios") == 0) continue;
            if (!EvidenciaDisponivelPorId(it->id)) continue;
            if (!EhObjetoInterativoReal(it->id)) continue;
            float d = hypotf(g_player.position.x - it->position.x, g_player.position.z - it->position.z);
            if (d < bestD) {
                bestD = d;
                g_hasClosestMouseTarget = true;
                g_closestMouseTargetPos = (Vector3){ it->position.x, it->position.y + 0.45f, it->position.z };
            }
        }
        for (int i = 0; i < def->numPortas; i++) {
            const SalaPorta *pt = &def->portas[i];
            float d = hypotf(g_player.position.x - pt->triggerCenter.x, g_player.position.z - pt->triggerCenter.z);
            if (d < bestD) {
                bestD = d;
                g_hasClosestMouseTarget = true;
                g_closestMouseTargetPos = (Vector3){ pt->triggerCenter.x, pt->triggerCenter.y + 0.90f, pt->triggerCenter.z };
            }
        }
    }

    // Dica no pensamento do player
    const char *acaoTxt = (ctrlMode == CONTROLE_MOUSE) ? "Clique" : "Aperte E";
    if (proxSusp >= 0) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s para conversar com %s", acaoTxt, NOMES_SUSP[proxSusp]);
    } else if (proxObj >= 0 && def->interactables[proxObj].label) {
        bool ehSombras = def->interactables[proxObj].id && strcmp(def->interactables[proxObj].id, "ev_jardim_sombras") == 0;
        bool ehEvidencia = !ehSombras && def->interactables[proxObj].id && strncmp(def->interactables[proxObj].id, "ev_", 3) == 0;
        if (ehSombras) {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - %s para escutar", def->interactables[proxObj].label, acaoTxt);
        } else if (ehEvidencia) {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "Evidência - %s para examinar", acaoTxt);
        } else {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - %s para interagir", def->interactables[proxObj].label, acaoTxt);
        }
    } else if (proxPorta != TERREO_DEST_NENHUM && proxPortaLabel) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - %s para entrar", proxPortaLabel, acaoTxt);
    } else if (hoverSusp >= 0) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s para conversar com %s",
                 (ctrlMode == CONTROLE_MOUSE) ? "Clique" : "Aproxime-se e aperte E", NOMES_SUSP[hoverSusp]);
    } else if (hoverObj >= 0 && def->interactables[hoverObj].label) {
        bool ehSombrasH = def->interactables[hoverObj].id && strcmp(def->interactables[hoverObj].id, "ev_jardim_sombras") == 0;
        bool ehEvH = !ehSombrasH && def->interactables[hoverObj].id && strncmp(def->interactables[hoverObj].id, "ev_", 3) == 0;
        const char *lblH = ehEvH ? "Evidência" : def->interactables[hoverObj].label;
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - %s", lblH,
                 (ctrlMode == CONTROLE_MOUSE) ? "Clique para interagir" : "Aproxime-se e aperte E");
    } else if (hoverPorta != TERREO_DEST_NENHUM && hoverPortaLabel) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - %s", hoverPortaLabel,
                 (ctrlMode == CONTROLE_MOUSE) ? "Clique para entrar" : "Aproxime-se e aperte E para entrar");
    }

    // Se o jogador usou o teclado para andar, cancela eventual caminhada automatica do mouse
    if (allowTeclado && (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
                         IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT))) {
        s_mousePendTipo = 0;
        s_pathCount = 0;
        s_pathIdx   = 0;
    }

    // 5) Interacao via tecla E ou ENTER (proximidade)
    if (allowTeclado && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER))) {
        s_mousePendTipo = 0;
        PlayerCharacterClearMoveTarget(&g_player);
        if (proxSusp >= 0) {
            Vector3 sp = { 0 };
            PosicaoSuspeitoNaSala(g_salaAtual, proxSusp, &sp, NULL, NULL);
            PlayerCharacterTriggerInteract(&g_player, sp);
            res.acionouSuspeito = true;
            res.suspeitoIndex   = proxSusp;
            return res;
        }
        if (proxPorta != TERREO_DEST_NENHUM) {
            Vector3 alvoP = g_player.position;
            for (int i = 0; i < def->numPortas; i++) {
                if (def->portas[i].destino == proxPorta) {
                    alvoP = def->portas[i].triggerCenter;
                    break;
                }
            }
            PlayerCharacterTriggerInteractEx(&g_player, alvoP, PLAYER_ANIM_DOOR, 0.28f);
            if (g_sndPorta3D.frameCount > 0) {
                SetSoundVolume(g_sndPorta3D, 0.22f * SettingsGetSfxVolume());
                SetSoundPitch(g_sndPorta3D, 0.96f);
                PlaySound(g_sndPorta3D);
            }
            g_portaAnimTimer = 0.22f;
            g_portaDestPendente = proxPorta;
            g_portaAlvoPos = alvoP;
            return res;
        }
        if (proxObj >= 0) {
            if (def->interactables[proxObj].reachHand) {
                PlayerCharacterTriggerInteract(&g_player, def->interactables[proxObj].position);
            }
            res.acionouObjeto = true;
            res.objetoId      = def->interactables[proxObj].id;
            res.objetoLabel   = def->interactables[proxObj].label;
            res.flavorText    = def->interactables[proxObj].flavorText;
            res.objetoPos     = def->interactables[proxObj].position;
            return res;
        }
        if (g_salaAtual == TERREO_SALA_CAMERAS && g_player.position.z < 0.0f) {
            res.acionouCamerasSecretas = true;
            return res;
        }
    }

    // 6) Modo Mouse (Sims / Ragnarok / Dota): clique no chao para mover ou em objeto/NPC para ir ate ele e interagir
    if (allowMouse && podeClicarObjetos && sobreJanela && g_texPronta && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hoverSusp >= 0) {
            Vector3 sp = { 0 };
            PosicaoSuspeitoNaSala(g_salaAtual, hoverSusp, &sp, NULL, NULL);
            float dSusp = hypotf(g_player.position.x - sp.x, g_player.position.z - sp.z);
            if (dSusp <= 1.15f) {
                s_mousePendTipo = 0;
                PlayerCharacterClearMoveTarget(&g_player);
                PlayerCharacterTriggerInteract(&g_player, sp);
                res.acionouSuspeito = true;
                res.suspeitoIndex   = hoverSusp;
                return res;
            }
            s_pathCount = CalcularCaminhoGrid(def, allCols, numAllCols, g_player.position, sp, s_pathPts, 32);
            s_pathIdx = 0;
            Vector3 finalSp = (s_pathCount > 0) ? s_pathPts[s_pathCount - 1] : sp;
            PlayerCharacterSetMoveGoalFinal(&g_player, finalSp);
            if (s_pathCount > 0) PlayerCharacterSetMoveTarget(&g_player, s_pathPts[0]);
            else                 PlayerCharacterSetMoveTarget(&g_player, sp);
            s_mousePendTipo = 3;
            s_mousePendIdx  = hoverSusp;
            s_mousePendPos  = sp;
        } else if (hoverPorta != TERREO_DEST_NENHUM) {
            if (proxPorta == hoverPorta) {
                s_mousePendTipo = 0;
                s_pathCount = 0;
                PlayerCharacterClearMoveTarget(&g_player);
                Vector3 alvoP = g_player.position;
                for (int i = 0; i < def->numPortas; i++) {
                    if (def->portas[i].destino == hoverPorta) { alvoP = def->portas[i].triggerCenter; break; }
                }
                PlayerCharacterTriggerInteractEx(&g_player, alvoP, PLAYER_ANIM_DOOR, 0.28f);
                if (g_sndPorta3D.frameCount > 0) {
                    SetSoundVolume(g_sndPorta3D, 0.22f * SettingsGetSfxVolume());
                    SetSoundPitch(g_sndPorta3D, 0.96f);
                    PlaySound(g_sndPorta3D);
                }
                g_portaAnimTimer = 0.22f;
                g_portaDestPendente = hoverPorta;
                g_portaAlvoPos = alvoP;
                return res;
            }
            for (int i = 0; i < def->numPortas; i++) {
                if (def->portas[i].destino == hoverPorta) {
                    s_pathCount = CalcularCaminhoGrid(def, allCols, numAllCols, g_player.position, def->portas[i].triggerCenter, s_pathPts, 32);
                    s_pathIdx = 0;
                    Vector3 finalPt = (s_pathCount > 0) ? s_pathPts[s_pathCount - 1] : def->portas[i].triggerCenter;
                    PlayerCharacterSetMoveGoalFinal(&g_player, finalPt);
                    if (s_pathCount > 0) PlayerCharacterSetMoveTarget(&g_player, s_pathPts[0]);
                    else                 PlayerCharacterSetMoveTarget(&g_player, def->portas[i].triggerCenter);
                    s_mousePendTipo = 2;
                    s_mousePendDest = hoverPorta;
                    s_mousePendPos  = def->portas[i].triggerCenter;
                    break;
                }
            }
        } else if (hoverObj >= 0) {
            if (proxObj == hoverObj) {
                s_mousePendTipo = 0;
                s_pathCount = 0;
                PlayerCharacterClearMoveTarget(&g_player);
                if (def->interactables[hoverObj].reachHand) {
                    PlayerCharacterTriggerInteract(&g_player, def->interactables[hoverObj].position);
                }
                res.acionouObjeto = true;
                res.objetoId      = def->interactables[hoverObj].id;
                res.objetoLabel   = def->interactables[hoverObj].label;
                res.flavorText    = def->interactables[hoverObj].flavorText;
                res.objetoPos     = def->interactables[hoverObj].position;
                return res;
            }
            s_pathCount = CalcularCaminhoGrid(def, allCols, numAllCols, g_player.position, def->interactables[hoverObj].position, s_pathPts, 32);
            s_pathIdx = 0;
            Vector3 finalObj = (s_pathCount > 0) ? s_pathPts[s_pathCount - 1] : def->interactables[hoverObj].position;
            PlayerCharacterSetMoveGoalFinal(&g_player, finalObj);
            if (s_pathCount > 0) PlayerCharacterSetMoveTarget(&g_player, s_pathPts[0]);
            else                 PlayerCharacterSetMoveTarget(&g_player, def->interactables[hoverObj].position);
            s_mousePendTipo = 1;
            s_mousePendIdx  = hoverObj;
            s_mousePendPos  = def->interactables[hoverObj].position;
        } else {
            // Clique nos monitores secretos da Sala das Cameras (sem dica visual)
            if (g_salaAtual == TERREO_SALA_CAMERAS) {
                Ray rayCam = GetScreenToWorldRayEx(mouseJanela, g_camAtual, g_largura, g_altura);
                BoundingBox bCam = { { -2.45f, 0.70f, -2.75f }, { 2.45f, 2.20f, -1.35f } };
                if (GetRayCollisionBox(rayCam, bCam).hit) {
                    res.acionouCamerasSecretas = true;
                    return res;
                }
            }
            // Clique no chao: raycast no plano Y = 0 e calcula trajeto sem atravessar paredes/moveis
            Ray rayFloor = GetScreenToWorldRayEx(mouseJanela, g_camAtual, g_largura, g_altura);
            if (rayFloor.direction.y < -0.02f) {
                float tHit = -rayFloor.position.y / rayFloor.direction.y;
                if (tHit > 0.1f && tHit < 45.0f) {
                    Vector3 dst = {
                        Clamp(rayFloor.position.x + rayFloor.direction.x * tHit, def->boundsXZ.x + 0.35f, def->boundsXZ.x + def->boundsXZ.width - 0.35f),
                        0.0f,
                        Clamp(rayFloor.position.z + rayFloor.direction.z * tHit, def->boundsXZ.y + 0.35f, def->boundsXZ.y + def->boundsXZ.height - 0.35f)
                    };
                    s_pathCount = CalcularCaminhoGrid(def, allCols, numAllCols, g_player.position, dst, s_pathPts, 32);
                    s_pathIdx = 0;
                    Vector3 finalDst = (s_pathCount > 0) ? s_pathPts[s_pathCount - 1] : dst;
                    PlayerCharacterSetMoveGoalFinal(&g_player, finalDst);
                    if (s_pathCount > 0) PlayerCharacterSetMoveTarget(&g_player, s_pathPts[0]);
                    else                 PlayerCharacterSetMoveTarget(&g_player, dst);
                    s_mousePendTipo = 0;
                    res.clicouChaoMover = true;
                }
            }
        }
    }

    // Executa interacao pendente quando o personagem chega perto pelo modo Mouse
    if (allowMouse && s_mousePendTipo != 0) {
        if (s_mousePendTipo == 1 && s_mousePendIdx >= 0 && s_mousePendIdx < def->numInteractables) {
            float dPend = hypotf(s_mousePendPos.x - g_player.position.x, s_mousePendPos.z - g_player.position.z);
            if (dPend <= 1.38f) {
                // Vira o personagem na direcao do objeto para garantir alinhamento frontal
                g_player.facingYaw = atan2f(s_mousePendPos.x - g_player.position.x, s_mousePendPos.z - g_player.position.z);
                int idx = s_mousePendIdx;
                s_mousePendTipo = 0;
                PlayerCharacterClearMoveTarget(&g_player);
                if (def->interactables[idx].reachHand) {
                    PlayerCharacterTriggerInteract(&g_player, def->interactables[idx].position);
                }
                res.acionouObjeto = true;
                res.objetoId      = def->interactables[idx].id;
                res.objetoLabel   = def->interactables[idx].label;
                res.flavorText    = def->interactables[idx].flavorText;
                res.objetoPos     = def->interactables[idx].position;
                return res;
            }
            if (!g_player.hasMoveTarget) s_mousePendTipo = 0;
        } else if (s_mousePendTipo == 2 && proxPorta == s_mousePendDest) {
            s_mousePendTipo = 0;
            PlayerCharacterClearMoveTarget(&g_player);
            PlayerCharacterTriggerInteractEx(&g_player, s_mousePendPos, PLAYER_ANIM_DOOR, 0.28f);
            if (g_sndPorta3D.frameCount > 0) {
                SetSoundVolume(g_sndPorta3D, 0.22f * SettingsGetSfxVolume());
                SetSoundPitch(g_sndPorta3D, 0.96f);
                PlaySound(g_sndPorta3D);
            }
            g_portaAnimTimer = 0.22f;
            g_portaDestPendente = proxPorta;
            g_portaAlvoPos = s_mousePendPos;
            return res;
        } else if (s_mousePendTipo == 3 && proxSusp == s_mousePendIdx) {
            Vector3 sp = { 0 };
            PosicaoSuspeitoNaSala(g_salaAtual, proxSusp, &sp, NULL, NULL);
            s_mousePendTipo = 0;
            PlayerCharacterClearMoveTarget(&g_player);
            PlayerCharacterTriggerInteract(&g_player, sp);
            res.acionouSuspeito = true;
            res.suspeitoIndex   = proxSusp;
            return res;
        } else if (!g_player.hasMoveTarget) {
            s_mousePendTipo = 0;
        }
    }

    return res;
}

void Terreo3DRender(float tempo) {
    if (!g_pronto || !g_texPronta) return;
    const TerreoSalaDef *def = g_salas[g_salaAtual];
    if (!def) return;

    BeginTextureMode(g_cena);
    ClearBackground(def->bgColor);
    BeginMode3D(g_camAtual);

    if (def->drawFunc) {
        def->drawFunc(tempo, g_evRelatoriosAtiva);
    }
    TerreoDesenharFuncionariosExtras(g_salaAtual, tempo);
    for (int s = 0; s < 3; s++) {
        if (!g_suspeitosNaSala[s]) continue;
        Vector3 sp = { 0 };
        float syaw = 0.0f;
        NpcAtividade sativ = NPC_ATIV_EM_PE;
        PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, &syaw, &sativ);
        PlayerCharacterDrawSuspect3DEx(s, sp, syaw, sativ, tempo, g_camAtual.position);
    }
    PlayerCharacterSetShadowBounds(def->boundsXZ);
    g_player.isFemale = GameStateIsPlayerFemale();
    if (!DevPrimeiraPessoaAtiva()) {
        PlayerCharacterDraw3D(&g_player, g_camAtual.position);
    }

    // Seta guiando flutuante acima do player (aponta para evidencia, suspeito, elevador ou porta)
    if (SettingsGetSetaGuiando()) {
        Vector3 alvo = { 0 };
        bool temAlvo = false;
        float bestD = 1e9f;
        for (int i = 0; i < def->numInteractables; i++) {
            const SalaInteractable *it = &def->interactables[i];
            if (!it->id || strncmp(it->id, "ev_", 3) != 0) continue;
            if (!EvidenciaDisponivelPorId(it->id)) continue;
            float d = hypotf(it->position.x - g_player.position.x, it->position.z - g_player.position.z);
            if (d < bestD) { bestD = d; alvo = it->position; temAlvo = true; }
        }
        if (!temAlvo) {
            for (int s = 0; s < 3; s++) {
                if (!g_suspeitosNaSala[s]) continue;
                Vector3 sp = { 0 };
                PosicaoSuspeitoNaSala(g_salaAtual, s, &sp, NULL, NULL);
                float d = hypotf(sp.x - g_player.position.x, sp.z - g_player.position.z);
                if (d < bestD) { bestD = d; alvo = sp; temAlvo = true; }
            }
        }
        if (!temAlvo && g_guiaTemAlgoNoAndar && def->numPortas > 0) {
            int pIdx = GuiaEncontrarPortaBFS(g_salaAtual, g_guiaSalasAlvoMask, false);
            if (pIdx >= 0 && pIdx < def->numPortas) {
                alvo = def->portas[pIdx].triggerCenter;
                temAlvo = true;
            }
        }
        if (!temAlvo && !g_guiaTemAlgoNoAndar) {
            for (int i = 0; i < def->numInteractables; i++) {
                if (def->interactables[i].id && strcmp(def->interactables[i].id, "elevador") == 0) {
                    alvo = def->interactables[i].position; temAlvo = true; break;
                }
            }
            if (!temAlvo && def->numPortas > 0) {
                int pIdx = GuiaEncontrarPortaBFS(g_salaAtual, 0ULL, true);
                if (pIdx >= 0 && pIdx < def->numPortas) {
                    alvo = def->portas[pIdx].triggerCenter;
                    temAlvo = true;
                }
            }
        }
        if (temAlvo) {
            float dx = alvo.x - g_player.position.x;
            float dz = alvo.z - g_player.position.z;
            float ang = atan2f(dx, dz) * RAD2DEG;
            float bob = sinf(tempo * 5.0f) * 0.06f;
            rlPushMatrix();
            rlTranslatef(g_player.position.x, g_player.position.y + 2.15f + bob, g_player.position.z);
            rlRotatef(ang, 0.0f, 1.0f, 0.0f);
            DrawCylinderEx((Vector3){ 0.0f, 0.0f, -0.18f }, (Vector3){ 0.0f, 0.0f, 0.06f }, 0.035f, 0.035f, 6, (Color){ 255, 224, 48, 255 });
            DrawCylinderEx((Vector3){ 0.0f, 0.0f,  0.04f }, (Vector3){ 0.0f, 0.0f, 0.24f }, 0.095f, 0.005f, 8, (Color){ 255, 240, 120, 255 });
            rlPopMatrix();
        }
    }

    EndMode3D();
    EndTextureMode();

    BeginTextureMode(g_final);
    ClearBackground(def->bgColor);
    BeginShaderMode(g_shader);
    DrawTextureRec(g_cena.texture,
                   (Rectangle){ 0, 0, (float)g_largura, -(float)g_altura },
                   (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
    EndTextureMode();
}

Texture2D Terreo3DTextura(void) {
    return g_final.texture;
}

const char *Terreo3DProximaAcao(void) {
    return (g_dicaAcao[0] != '\0') ? g_dicaAcao : NULL;
}

bool Terreo3DMouseEmInteragivel(void) {
    return g_mouseSobreInteragivel;
}

bool Terreo3DHasProxInteragivel(void) {
    return g_hasProxInteragivel;
}

Vector2 Terreo3DPlayerHeadScreenPos(void) {
    if (!g_pronto) return (Vector2){ 149.0f, 110.0f };
    if (DevPrimeiraPessoaAtiva()) {
        return (Vector2){ g_largura * 0.5f, g_altura * 0.72f };
    }
    Vector3 head = { g_player.position.x, g_player.position.y + 1.92f, g_player.position.z };
    return GetWorldToScreenEx(head, g_camAtual, g_largura, g_altura);
}

Vector3 Terreo3DGetPlayerPos(void) {
    return g_player.position;
}

bool Terreo3DClosestInteractableScreenPos(Vector2 *outPosJanela) {
    if (!g_pronto || !g_hasClosestMouseTarget) return false;
    if (outPosJanela) {
        *outPosJanela = GetWorldToScreenEx(g_closestMouseTargetPos, g_camAtual, g_largura, g_altura);
    }
    return true;
}
