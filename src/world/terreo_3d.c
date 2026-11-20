#include "terreo_3d.h"
#include "game_state.h"
#include "settings.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stddef.h>
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
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar * 0.92), 0.0, 4.0));\n"
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static RenderTexture2D      g_cena = { 0 };
static RenderTexture2D      g_final = { 0 };
static bool                 g_texPronta = false;
static int                  g_largura = 0;
static int                  g_altura = 0;
static Shader               g_shader = { 0 };
static bool                 g_pronto = false;

static const TerreoSalaDef *g_salas[TERREO_NUM_SALAS] = { 0 };
static TerreoSalaId         g_salaAtual = TERREO_SALA_RECEPCAO;
static PlayerCharacter      g_player = { 0 };
static Camera3D             g_camAtual = { 0 };
static bool                 g_evRelatoriosAtiva = false;
static float                g_cooldownEntrada = 0.0f;
static bool                 g_mouseSobreInteragivel = false;
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
    rlPushMatrix();
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

void TerreoBebedouro(Vector3 pos) {
    // Gabinete arredondado + galao cilindrico no topo
    DrawCylinder((Vector3){ pos.x, pos.y, pos.z }, 0.24f, 0.26f, 0.96f, 14, TerreoLum(0.76f));
    DrawCylinder((Vector3){ pos.x, pos.y + 0.96f, pos.z }, 0.21f, 0.21f, 0.42f, 14, TerreoLum(0.88f));
    DrawCylinder((Vector3){ pos.x, pos.y + 1.38f, pos.z }, 0.08f, 0.20f, 0.08f, 12, TerreoLum(0.84f));
    TerreoCaixa((Vector3){ pos.x - 0.16f, pos.y + 0.78f, pos.z }, (Vector3){ 0.08f, 0.16f, 0.18f }, 0.28f, 0.22f, 0.22f, -1.0f);
}

void TerreoImpressora(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Gabinete base + corpo da multifuncional + bandeja superior com papel
    TerreoCaixa((Vector3){ 0.0f, 0.38f, 0.0f }, (Vector3){ 0.72f, 0.76f, 0.56f }, 0.42f, 0.32f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ 0.0f, 0.96f, 0.0f }, (Vector3){ 0.68f, 0.42f, 0.52f }, 0.84f, 0.70f, 0.62f, 0.16f);
    TerreoCaixa((Vector3){ 0.0f, 1.20f, 0.02f }, (Vector3){ 0.42f, 0.05f, 0.34f }, 0.94f, 0.84f, 0.76f, 0.18f);

    rlPopMatrix();
}

void TerreoArmario(Vector3 pos, float rotYDeg, float largura, float altura, float prof) {
    // Baseado visualmente em 3D_cabinet_7: corpo metalico cinza fosco, rodape escuro,
    // 2 portas com paineis chanfrados/recuados, puxadores verticais e plaquinha de etiqueta.
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    float rodapeH = 0.08f;
    float corpoH = altura - rodapeH;
    float fZ = prof * 0.5f;

    // Rodape escuro recuado
    TerreoCaixa((Vector3){ 0.0f, rodapeH * 0.5f, -0.01f },
                (Vector3){ largura * 0.96f, rodapeH, prof * 0.94f },
                0.20f, 0.14f, 0.12f, 0.06f);
    // Corpo principal cinza fosco medio
    TerreoCaixa((Vector3){ 0.0f, rodapeH + corpoH * 0.5f, 0.0f },
                (Vector3){ largura, corpoH, prof },
                0.56f, 0.46f, 0.40f, 0.14f);

    // Duas portas frontais com moldura e painel interno chanfrado
    float portaW = largura * 0.45f;
    float portaH = corpoH * 0.90f;
    float portaY = rodapeH + corpoH * 0.5f;
    for (int side = -1; side <= 1; side += 2) {
        float px = (float)side * (largura * 0.24f);
        // Folha da porta
        TerreoCaixa((Vector3){ px, portaY, fZ + 0.012f },
                    (Vector3){ portaW, portaH, 0.024f },
                    0.52f, 0.42f, 0.36f, 0.14f);
        // Painel interno chanfrado (detalhe caracteristico de 3D_cabinet_7)
        TerreoCaixa((Vector3){ px, portaY, fZ + 0.026f },
                    (Vector3){ portaW * 0.72f, portaH * 0.76f, 0.010f },
                    0.40f, 0.34f, 0.30f, 0.18f);
        // Puxador metalico vertical proximo ao centro
        float puxX = px - (float)side * (portaW * 0.34f);
        TerreoCaixa((Vector3){ puxX, portaY + 0.04f, fZ + 0.040f },
                    (Vector3){ 0.028f, 0.16f, 0.024f },
                    0.22f, 0.16f, 0.14f, -1.0f);
    }

    // Plaquinha de numeracao/etiqueta no canto superior direito da porta esquerda
    TerreoCaixa((Vector3){ -largura * 0.16f, rodapeH + corpoH * 0.84f, fZ + 0.032f },
                (Vector3){ 0.11f, 0.05f, 0.008f },
                0.24f, 0.18f, 0.16f, 0.70f);

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

    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.14f, 0.18f, 0.26f, 10, TerreoLum(0.24f));
    DrawCylinder((Vector3){ 0.0f, 0.24f, 0.0f }, 0.17f, 0.15f, 0.04f, 10, TerreoLum(0.18f));
    DrawSphereEx((Vector3){ 0.0f, 0.38f, 0.0f }, 0.18f, 8, 8, TerreoLum(0.36f));
    DrawSphereEx((Vector3){ -0.10f, 0.44f, 0.06f }, 0.12f, 6, 8, TerreoLum(0.46f));
    DrawSphereEx((Vector3){  0.11f, 0.42f, -0.05f }, 0.12f, 6, 8, TerreoLum(0.42f));

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

    GarantirTexturas(largura, altura);
    Terreo3DDefinirSala(TERREO_SALA_RECEPCAO, TERREO_DEST_NENHUM);

    g_evRelatoriosAtiva = false;
    g_mouseSobreInteragivel = false;
    g_dicaAcao[0] = '\0';
    g_pronto = true;
}

void Terreo3DUnload(void) {
    if (!g_pronto) return;
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

static void AtualizarCameraSeguindo(const TerreoSalaDef *def, float dt, bool snap) {
    if (!def) return;
    float cx = def->boundsXZ.x + def->boundsXZ.width * 0.5f;
    float cz = def->boundsXZ.y + def->boundsXZ.height * 0.5f;
    float halfW = def->boundsXZ.width * 0.5f;
    float halfD = def->boundsXZ.height * 0.5f;

    const float camOffY = 4.35f;
    const float camOffZ = 6.55f;
    const float camFovy = 36.5f;

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
    float maxOffZ = fmaxf(0.0f, halfD - 1.45f);
    float targetX = Clamp(g_player.position.x, cx - maxOffX, cx + maxOffX);
    float targetZ = Clamp(g_player.position.z - 0.20f, cz - maxOffZ, cz + maxOffZ);
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
        } else if (vindoDe == TERREO_DEST_ESCADA_2D || vindoDe == TERREO_DEST_HALL_1) {
            pos = (Vector3){ 4.35f, 0.0f, -2.90f };
            yaw = 0.0f;
        }
    } else if (sala == TERREO_SALA_LANCHONETE) {
        if (vindoDe == TERREO_DEST_RECEPCAO) {
            pos = (Vector3){ 3.55f, 0.0f, 1.15f };
            yaw = -PI * 0.5f;
        } else if (vindoDe == TERREO_DEST_JARDIM) {
            pos = (Vector3){ 0.55f, 0.0f, -2.75f };
            yaw = 0.0f;
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
        if (vindoDe == TERREO_DEST_RECEPCAO || vindoDe == TERREO_DEST_ESCADA_1) {
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
        }
    } else if (sala == TERREO_SALA_COPA_1) {
        if (vindoDe == TERREO_DEST_HALL_1) {
            pos = (Vector3){ 3.55f, 0.0f, 0.45f };
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
    AtualizarCameraSeguindo(def, 0.0f, true);
    g_cooldownEntrada = 0.45f;
    g_mouseSobreInteragivel = false;
    g_dicaAcao[0] = '\0';
}

void Terreo3DSetEvidenciaAtiva(bool evRelatoriosAtiva) {
    g_evRelatoriosAtiva = evRelatoriosAtiva;
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
    g_dicaAcao[0] = '\0';

    if (bloqueado) {
        g_player.isMoving = false;
        if (g_player.interactTimer > 0.0f) {
            g_player.interactTimer -= dt;
            if (g_player.interactTimer < 0.0f) g_player.interactTimer = 0.0f;
        }
        return res;
    }

    // 1) Atualiza movimento, colisoes e elevacao na escada (quando aplicavel)
    PlayerCharacterUpdate(&g_player, dt, g_camAtual.position, g_camAtual.target,
                          def->boundsXZ, def->colliders, def->numColliders);
    if (def->heightFunc) {
        g_player.position.y = def->heightFunc(g_player.position.x, g_player.position.z);
    } else {
        g_player.position.y = 0.0f;
    }
    AtualizarCameraSeguindo(def, dt, false);

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
    int proxObj = -1;
    TerreoDestino proxPorta = TERREO_DEST_NENHUM;
    const char *proxPortaLabel = NULL;
    float melhorDist = 1e9f;

    for (int i = 0; i < def->numPortas; i++) {
        const SalaPorta *pt = &def->portas[i];
        float d = hypotf(g_player.position.x - pt->triggerCenter.x,
                         g_player.position.z - pt->triggerCenter.z);
        if (d <= pt->triggerRadius && d < melhorDist) {
            melhorDist     = d;
            proxPorta      = pt->destino;
            proxPortaLabel = pt->label;
        }
    }
    for (int i = 0; i < def->numInteractables; i++) {
        const SalaInteractable *it = &def->interactables[i];
        if (!g_evRelatoriosAtiva && it->id && strcmp(it->id, "mesa_relatorios") == 0) continue;
        float d = hypotf(g_player.position.x - it->position.x,
                         g_player.position.z - it->position.z);
        if (d <= it->interactRadius && d < melhorDist) {
            melhorDist = d;
            proxObj    = i;
            proxPorta  = TERREO_DEST_NENHUM;
        }
    }

    // 4) Hover por raycast 3D
    int hoverObj = -1;
    TerreoDestino hoverPorta = TERREO_DEST_NENHUM;
    const char *hoverPortaLabel = NULL;
    if (sobreJanela && g_texPronta) {
        Ray ray = GetScreenToWorldRayEx(mouseJanela, g_camAtual, g_largura, g_altura);
        float bestHit = 1e9f;

        for (int i = 0; i < def->numPortas; i++) {
            RayCollision hit = GetRayCollisionBox(ray, def->portas[i].clickBox);
            if (hit.hit && hit.distance < bestHit) {
                bestHit         = hit.distance;
                hoverPorta      = def->portas[i].destino;
                hoverPortaLabel = def->portas[i].label;
            }
        }
        for (int i = 0; i < def->numInteractables; i++) {
            if (!g_evRelatoriosAtiva && def->interactables[i].id &&
                strcmp(def->interactables[i].id, "mesa_relatorios") == 0) continue;
            RayCollision hit = GetRayCollisionBox(ray, BoxInteractable(&def->interactables[i]));
            if (hit.hit && hit.distance < bestHit) {
                bestHit    = hit.distance;
                hoverObj   = i;
                hoverPorta = TERREO_DEST_NENHUM;
            }
        }
        g_mouseSobreInteragivel = (hoverPorta != TERREO_DEST_NENHUM) || (hoverObj >= 0);
    }

    // Dica no pensamento do player
    if (proxObj >= 0 && def->interactables[proxObj].label) {
        bool ehPistaAtiva = g_evRelatoriosAtiva && strcmp(def->interactables[proxObj].id, "mesa_relatorios") == 0;
        if (ehPistaAtiva) {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Aperte E para examinar", def->interactables[proxObj].label);
        } else {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Aperte E para interagir", def->interactables[proxObj].label);
        }
    } else if (proxPorta != TERREO_DEST_NENHUM && proxPortaLabel) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Aperte E para entrar", proxPortaLabel);
    } else if (hoverObj >= 0 && def->interactables[hoverObj].label) {
        bool ehPistaAtiva = g_evRelatoriosAtiva && strcmp(def->interactables[hoverObj].id, "mesa_relatorios") == 0;
        if (ehPistaAtiva) {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Clique para examinar", def->interactables[hoverObj].label);
        } else {
            snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Clique para interagir", def->interactables[hoverObj].label);
        }
    } else if (hoverPorta != TERREO_DEST_NENHUM && hoverPortaLabel) {
        snprintf(g_dicaAcao, sizeof(g_dicaAcao), "%s - Aproxime-se e aperte E para entrar", hoverPortaLabel);
    }

    // 5) Interacao via tecla E (proximidade) ou clique esquerdo (raycast apenas quando proximo em portas)
    if (IsKeyPressed(KEY_E)) {
        if (proxPorta != TERREO_DEST_NENHUM) {
            res.acionouDestino = true;
            res.destino        = proxPorta;
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
            return res;
        }
    }

    if (podeClicarObjetos && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hoverPorta != TERREO_DEST_NENHUM && hoverPorta == proxPorta) {
            res.acionouDestino = true;
            res.destino        = hoverPorta;
            return res;
        }
        if (hoverObj >= 0) {
            if (def->interactables[hoverObj].reachHand) {
                PlayerCharacterTriggerInteract(&g_player, def->interactables[hoverObj].position);
            }
            res.acionouObjeto = true;
            res.objetoId      = def->interactables[hoverObj].id;
            res.objetoLabel   = def->interactables[hoverObj].label;
            res.flavorText    = def->interactables[hoverObj].flavorText;
            return res;
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
    g_player.isFemale = GameStateIsPlayerFemale();
    PlayerCharacterDraw3D(&g_player, g_camAtual.position);

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
