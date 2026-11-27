#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define ESC3D_MIN_X -3.20f
#define ESC3D_MAX_X 3.20f
#define ESC3D_MIN_Z -3.20f
#define ESC3D_MAX_Z 3.20f
#define ESC3D_WALL_H 3.20f

// Baseado visualmente em escadaria.zip (Stairs_5_BaseColor-1K):
// - Piso e patamar em pedra/concreto cinza medio (~0.56)
// - Degraus de madeira escura (~0.18) com filete frontal
// - Corrimao e balaustres escuros (~0.10)

static float EscadaTerreoAltura(float x, float z) {
    if (x >= 0.85f && x <= 2.30f && z <= 1.40f && z >= -1.80f) {
        float t = (1.40f - z) / 3.20f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return t * 1.60f;
    }
    if (x >= 0.85f && x <= 2.30f && z < -1.80f) return 1.60f;
    return 0.0f;
}

static float Escada1Altura(float x, float z) {
    if (x >= 0.85f && x <= 2.30f && z >= -1.40f && z <= 1.80f) {
        float t = (z + 1.40f) / 3.20f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return -t * 1.15f;
    }
    if (x >= 0.85f && x <= 2.30f && z > 1.80f) return -1.15f;
    return 0.0f;
}

static void DesenharEstruturaEscada3D(bool temSubir, bool temDescer) {
    float w = ESC3D_MAX_X - ESC3D_MIN_X;
    float d = ESC3D_MAX_Z - ESC3D_MIN_Z;

    // Patamar principal em pedra/concreto cinza
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.44f);
    for (float x = -3.0f; x <= 3.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, ESC3D_MIN_Z }, (Vector3){ x, 0.004f, ESC3D_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ ESC3D_MIN_X, 0.004f, z }, (Vector3){ ESC3D_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes (fundo, esquerda com porta, direita)
    TerreoCaixa((Vector3){ 0.0f, ESC3D_WALL_H * 0.5f, ESC3D_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, ESC3D_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ ESC3D_MIN_X - 0.08f, ESC3D_WALL_H * 0.5f, 0.0f },
                (Vector3){ 0.16f, ESC3D_WALL_H, d }, 0.32f, 0.38f, 0.36f, 0.12f);
    TerreoCaixa((Vector3){ ESC3D_MAX_X + 0.08f, ESC3D_WALL_H * 0.5f, 0.0f },
                (Vector3){ 0.16f, ESC3D_WALL_H, d }, 0.32f, 0.38f, 0.36f, 0.12f);

    // Porta para o andar atual (esquerda)
    TerreoPortaModelo((Vector3){ ESC3D_MIN_X + 0.04f, 0.0f, 0.0f }, 90.0f, PORTA_TIPO_INDUSTRIAL);

    // Guarda-corpo central separando o patamar do poco da escada
    TerreoCaixa((Vector3){ 0.78f, 0.50f, 0.0f }, (Vector3){ 0.08f, 1.00f, 2.80f }, 0.14f, 0.10f, 0.10f, 0.05f);
    for (int b = -2; b <= 2; b++) {
        TerreoCaixa((Vector3){ 0.78f, 0.48f, b * 0.55f }, (Vector3){ 0.04f, 0.96f, 0.04f }, 0.24f, 0.18f, 0.18f, -1.0f);
    }

    // Lance subindo (x = 1.55, de z = 1.40 ate z = -1.80)
    if (temSubir) {
        const int degraus = 10;
        float zIni = 1.40f;
        float zFim = -1.80f;
        float stepZ = (zFim - zIni) / (float)degraus;
        float stepY = 1.60f / (float)degraus;
        for (int i = 0; i < degraus; i++) {
            float cz = zIni + ((float)i + 0.5f) * stepZ;
            float topY = ((float)i + 1.0f) * stepY;
            // Base de concreto abaixo do degrau
            TerreoCaixa((Vector3){ 1.55f, topY * 0.5f, cz },
                        (Vector3){ 1.32f, topY, fabsf(stepZ) * 0.98f },
                        0.48f, 0.40f, 0.36f, 0.12f);
            // Piso do degrau em madeira escura (Stairs_5)
            TerreoCaixa((Vector3){ 1.55f, topY + 0.025f, cz },
                        (Vector3){ 1.36f, 0.05f, fabsf(stepZ) * 1.04f },
                        0.20f, 0.14f, 0.12f, 0.06f);
        }
        // Patamar superior de concreto
        TerreoCaixa((Vector3){ 1.55f, 0.80f, -2.50f },
                    (Vector3){ 1.40f, 1.60f, 1.40f },
                    0.56f, 0.44f, 0.38f, 0.12f);
        // Corrimao inclinado escuro
        DrawCylinderEx((Vector3){ 0.88f, 0.96f, 1.35f }, (Vector3){ 0.88f, 2.52f, -1.80f }, 0.032f, 0.032f, 8, TerreoLum(0.10f));
        DrawCylinderEx((Vector3){ 2.22f, 0.96f, 1.35f }, (Vector3){ 2.22f, 2.52f, -1.80f }, 0.032f, 0.032f, 8, TerreoLum(0.10f));
    }

    // Lance descendo (x = 1.55, de z = -1.40 ate z = 1.80)
    if (temDescer) {
        const int degraus = 8;
        float zIni = -1.40f;
        float zFim = 1.80f;
        float stepZ = (zFim - zIni) / (float)degraus;
        float stepY = -1.15f / (float)degraus;
        for (int i = 0; i < degraus; i++) {
            float cz = zIni + ((float)i + 0.5f) * stepZ;
            float topY = ((float)i + 1.0f) * stepY;
            TerreoCaixa((Vector3){ 1.55f, topY + 0.025f, cz },
                        (Vector3){ 1.36f, 0.06f, fabsf(stepZ) * 1.02f },
                        0.20f, 0.14f, 0.12f, 0.06f);
        }
        DrawCylinderEx((Vector3){ 0.88f, 0.96f, -1.35f }, (Vector3){ 0.88f, -0.15f, 1.75f }, 0.032f, 0.032f, 8, TerreoLum(0.10f));
        DrawCylinderEx((Vector3){ 2.22f, 0.96f, -1.35f }, (Vector3){ 2.22f, -0.15f, 1.75f }, 0.032f, 0.032f, 8, TerreoLum(0.10f));
    }

    // Placa indicativa de andar na parede do fundo
    TerreoCaixa((Vector3){ -1.95f, 1.85f, ESC3D_MIN_Z + 0.03f },
                (Vector3){ 1.15f, 0.45f, 0.04f }, 0.82f, 0.78f, 0.70f, 0.16f);
}

// ----------------- ESCADA TERREO -----------------
static const PlayerColliderBox ESC_TERREO_COLLIDERS[] = {
    { 0.65f, -1.55f, 0.90f, 1.45f },   // Guarda-corpo central
    { 2.25f, -3.20f, 3.20f, 3.20f },   // Lateral direita da escada
};

static const SalaInteractable ESC_TERREO_INTERACTABLES[] = {
    {
        "placa_escada_terreo",
        "Placa da Escadaria (Térreo)",
        { -1.95f, 1.85f, -3.10f },
        { 1.25f, 0.65f, 0.40f },
        1.85f,
        false,
        "TÉRREO - Administração, Recepção, Lanchonete e Jardim. Suba a escada para acessar o 1º Andar (Contabilidade)."
    }
};

static const SalaPorta ESC_TERREO_PORTAS[] = {
    {
        TERREO_DEST_RECEPCAO,
        "Porta (Recepção - Térreo)",
        { { -3.35f, 0.0f, -0.70f }, { -2.85f, 2.30f, 0.70f } },
        { -2.90f, 0.0f, 0.0f },
        1.35f,
        { -3.25f, -0.65f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_ESCADA_1,
        "Subir Escada (1º Andar)",
        { { 0.85f, 1.00f, -2.95f }, { 2.25f, 2.80f, -1.75f } },
        { 1.55f, 1.40f, -2.10f },
        1.45f,
        { 0.85f, -2.95f, 1.40f, 1.10f }
    }
};

static void DesenharEscadaTerreo(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    DesenharEstruturaEscada3D(true, false);
}

static const TerreoSalaDef SALA_ESCADA_TERREO_DEF = {
    .id = TERREO_SALA_ESCADA_TERREO,
    .camera = {
        .position   = { 0.0f, 5.10f, 7.60f },
        .target     = { 0.0f, 0.80f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 38.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { ESC3D_MIN_X, ESC3D_MIN_Z, ESC3D_MAX_X - ESC3D_MIN_X, ESC3D_MAX_Z - ESC3D_MIN_Z },
    .bgColor          = { 20, 18, 20, 255 },
    .defaultSpawnPos  = { -2.35f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = ESC_TERREO_COLLIDERS,
    .numColliders     = (int)(sizeof(ESC_TERREO_COLLIDERS) / sizeof(ESC_TERREO_COLLIDERS[0])),
    .interactables    = ESC_TERREO_INTERACTABLES,
    .numInteractables = (int)(sizeof(ESC_TERREO_INTERACTABLES) / sizeof(ESC_TERREO_INTERACTABLES[0])),
    .portas           = ESC_TERREO_PORTAS,
    .numPortas        = (int)(sizeof(ESC_TERREO_PORTAS) / sizeof(ESC_TERREO_PORTAS[0])),
    .heightFunc       = EscadaTerreoAltura,
    .drawFunc         = DesenharEscadaTerreo
};

const TerreoSalaDef *SalaEscadaTerreoDef(void) {
    return &SALA_ESCADA_TERREO_DEF;
}

// ----------------- ESCADA 1o ANDAR -----------------
static const PlayerColliderBox ESC_1_COLLIDERS[] = {
    { 0.65f, -1.55f, 0.90f, 1.45f },
    { 2.25f, -3.20f, 3.20f, 3.20f },
};

static const SalaInteractable ESC_1_INTERACTABLES[] = {
    {
        "placa_escada_1",
        "Placa da Escadaria (1º Andar)",
        { -1.95f, 1.85f, -3.10f },
        { 1.25f, 0.65f, 0.40f },
        1.85f,
        false,
        "1º ANDAR - Setor de Contabilidade, Arquivo e Copa."
    }
};

static const SalaPorta ESC_1_PORTAS[] = {
    {
        TERREO_DEST_HALL_1,
        "Porta (1º Andar - Contabilidade)",
        { { -3.35f, 0.0f, -0.70f }, { -2.85f, 2.30f, 0.70f } },
        { -2.90f, 0.0f, 0.0f },
        1.35f,
        { -3.25f, -0.65f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_CONTABILIDADE_2,
        "Porta (Contabilidade 2)",
        { { -1.15f, 0.0f, -3.35f }, { 0.05f, 2.30f, -2.90f } },
        { -0.55f, 0.0f, -2.95f },
        1.35f,
        { -1.15f, -3.28f, 1.20f, 0.55f }
    },
    {
        TERREO_DEST_ESCADA_2D,
        "Descer Escada (Térreo)",
        { { 0.85f, -1.40f, 1.75f }, { 2.25f, 0.60f, 2.95f } },
        { 1.55f, -0.90f, 2.10f },
        1.45f,
        { 0.85f, 1.75f, 1.40f, 1.15f }
    }
};

static void DesenharEscada1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    DesenharEstruturaEscada3D(false, true);
    TerreoPortaModelo((Vector3){ -0.55f, 0.0f, ESC3D_MIN_Z + 0.04f }, 0.0f, PORTA_TIPO_ESCRITORIO);
}

static const TerreoSalaDef SALA_ESCADA_1_DEF = {
    .id = TERREO_SALA_ESCADA_1,
    .camera = {
        .position   = { 0.0f, 5.10f, 7.60f },
        .target     = { 0.0f, 0.80f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 38.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { ESC3D_MIN_X, ESC3D_MIN_Z, ESC3D_MAX_X - ESC3D_MIN_X, ESC3D_MAX_Z - ESC3D_MIN_Z },
    .bgColor          = { 20, 18, 20, 255 },
    .defaultSpawnPos  = { 0.15f, 0.0f, 1.35f },
    .defaultSpawnYaw  = PI,
    .colliders        = ESC_1_COLLIDERS,
    .numColliders     = (int)(sizeof(ESC_1_COLLIDERS) / sizeof(ESC_1_COLLIDERS[0])),
    .interactables    = ESC_1_INTERACTABLES,
    .numInteractables = (int)(sizeof(ESC_1_INTERACTABLES) / sizeof(ESC_1_INTERACTABLES[0])),
    .portas           = ESC_1_PORTAS,
    .numPortas        = (int)(sizeof(ESC_1_PORTAS) / sizeof(ESC_1_PORTAS[0])),
    .heightFunc       = Escada1Altura,
    .drawFunc         = DesenharEscada1
};

const TerreoSalaDef *SalaEscada1Def(void) {
    return &SALA_ESCADA_1_DEF;
}
