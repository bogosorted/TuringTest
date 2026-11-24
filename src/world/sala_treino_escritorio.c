#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Treinamento Escritorio (baseado em treinamentoiaescritorio.jpg):
// - Mesas escuras de desenvolvimento com monitores (incluindo monitor vertical), gabinetes PC com ventoinhas e muitos cabos
// - Luminarias pendentes pretas suspensas sobre as mesas e janelas amplas ao fundo
static const float TE_MIN_X = -5.40f;
static const float TE_MAX_X =  5.40f;
static const float TE_MIN_Z = -3.60f;
static const float TE_MAX_Z =  3.40f;
static const float TE_WALL_H = 3.10f;

static const PlayerColliderBox TE_COLLIDERS[] = {
    { -3.55f, -1.95f,  2.55f, -0.55f }, // Ilha de desenvolvimento 1
    { -3.55f,  0.65f,  2.55f,  1.95f }, // Ilha de desenvolvimento 2
};

static const SalaInteractable TE_INTERACTABLES[] = {
    {
        "estacao_dev_ia",
        "Estação de Treinamento de Modelos",
        { -0.50f, 0.90f, -1.25f },
        { 5.60f, 1.15f, 1.30f },
        2.15f,
        true,
        "Monitores verticais com código Python e logs de treinamento de redes neurais rodam sem parar nos gabinetes."
    },
    {
        "pc_gpu_ia",
        "Workstation GPU",
        { 1.65f, 0.90f, 1.30f },
        { 1.80f, 1.15f, 1.15f },
        1.95f,
        true,
        "Gabinetes com ventoinhas iluminadas e placas de vídeo dedicadas para treinar modelos localmente."
    }
};

static const SalaPorta TE_PORTAS[] = {
    {
        TERREO_DEST_HALL_3,
        "Porta (Hall 3º Andar)",
        { { 5.15f, 0.0f, -0.25f }, { 5.65f, 2.30f, 1.15f } },
        { 5.20f, 0.0f, 0.45f },
        1.40f,
        { 4.90f, -0.20f, 0.55f, 1.30f }
    }
};

static void DesenharIlhaDev(float zCentro) {
    // Tampo escuro de desenvolvimento (treinamentoiaescritorio.jpg)
    TerreoCaixa((Vector3){ -0.50f, 0.74f, zCentro }, (Vector3){ 5.80f, 0.05f, 1.10f }, 0.24f, 0.16f, 0.14f, 0.30f);
    for (int i = -1; i <= 1; i++) {
        float px = -0.50f + i * 1.90f;
        TerreoMonitor((Vector3){ px - 0.18f, 0.77f, zCentro }, 0.0f);
        // Monitor vertical ao lado
        TerreoCaixa((Vector3){ px + 0.24f, 1.02f, zCentro + 0.04f }, (Vector3){ 0.24f, 0.44f, 0.04f }, 0.18f, 0.10f, 0.12f, 0.44f);
        // Gabinete PC torre detalhado
        TerreoGabinetePC((Vector3){ px + 0.55f, 0.77f, zCentro - 0.10f }, 0.0f);
        TerreoCadeira((Vector3){ px, 0.0f, zCentro + 0.72f }, 180.0f);
    }
    // Luminarias pendentes pretas acima da mesa
    for (int l = -1; l <= 1; l += 2) {
        float lx = -0.50f + l * 1.40f;
        DrawCylinder((Vector3){ lx, 2.55f, zCentro }, 0.24f, 0.08f, 0.28f, 12, TerreoLum(0.14f));
        DrawCylinderEx((Vector3){ lx, 2.83f, zCentro }, (Vector3){ lx, 3.30f, zCentro }, 0.015f, 0.015f, 6, TerreoLum(0.18f));
    }
}

static void DesenharTreinoEscritorio(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = TE_MAX_X - TE_MIN_X;
    float d = TE_MAX_Z - TE_MIN_Z;
    float cx = (TE_MIN_X + TE_MAX_X) * 0.5f;
    float cz = (TE_MIN_Z + TE_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.48f));
    Color corGrid = TerreoLum(0.36f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, TE_MIN_Z }, (Vector3){ x, 0.004f, TE_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ TE_MIN_X, 0.004f, z }, (Vector3){ TE_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, TE_WALL_H * 0.5f, TE_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, TE_WALL_H, 0.16f }, 0.32f, 0.40f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ TE_MIN_X - 0.08f, TE_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, TE_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);
    TerreoCaixa((Vector3){ TE_MAX_X + 0.08f, TE_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, TE_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);

    // Janelas grandes e cortinas escuras ao fundo
    TerreoCaixa((Vector3){ -2.20f, 1.80f, TE_MIN_Z + 0.03f }, (Vector3){ 2.60f, 1.65f, 0.05f }, 0.90f, 0.88f, 0.82f, 0.16f);
    TerreoCaixa((Vector3){  2.20f, 1.80f, TE_MIN_Z + 0.03f }, (Vector3){ 2.60f, 1.65f, 0.05f }, 0.90f, 0.88f, 0.82f, 0.16f);
    TerreoCaixa((Vector3){  0.00f, 1.65f, TE_MIN_Z + 0.06f }, (Vector3){ 0.70f, 2.75f, 0.12f }, 0.18f, 0.12f, 0.12f, -1.0f);

    TerreoPortaModelo((Vector3){ TE_MAX_X - 0.04f, 0.0f, 0.45f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    DesenharIlhaDev(-1.25f);
    DesenharIlhaDev(1.30f);
    TerreoSilhuetaSombra((Vector3){ -2.40f, 0.0f, -0.55f }, 180.0f, true);
}

static const TerreoSalaDef SALA_TREINO_ESCRITORIO_DEF = {
    .id = TERREO_SALA_TREINO_ESCRITORIO,
    .camera = {
        .position   = { 0.0f, 5.65f, 8.30f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { TE_MIN_X, TE_MIN_Z, TE_MAX_X - TE_MIN_X, TE_MAX_Z - TE_MIN_Z },
    .bgColor          = { 20, 18, 20, 255 },
    .defaultSpawnPos  = { 4.35f, 0.0f, 0.45f },
    .defaultSpawnYaw  = -PI * 0.5f,
    .colliders        = TE_COLLIDERS,
    .numColliders     = (int)(sizeof(TE_COLLIDERS) / sizeof(TE_COLLIDERS[0])),
    .interactables    = TE_INTERACTABLES,
    .numInteractables = (int)(sizeof(TE_INTERACTABLES) / sizeof(TE_INTERACTABLES[0])),
    .portas           = TE_PORTAS,
    .numPortas        = (int)(sizeof(TE_PORTAS) / sizeof(TE_PORTAS[0])),
    .drawFunc         = DesenharTreinoEscritorio
};

const TerreoSalaDef *SalaTreinoEscritorioDef(void) {
    return &SALA_TREINO_ESCRITORIO_DEF;
}
