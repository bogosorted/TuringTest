#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Escritorio Ciberseguranca 2 (baseado em escritoriociberseguranca.jpg):
// - Fileiras paralelas de mesas brancas com base cinza e multiplos monitores enfileirados
// - Persianas claras nas janelas ao fundo
// - Esquerda: volta ao Hall 2 | Fundo: porta industrial para Sala de Servidores 1
static const float CB2_MIN_X = -5.60f;
static const float CB2_MAX_X =  5.60f;
static const float CB2_MIN_Z = -3.80f;
static const float CB2_MAX_Z =  3.60f;
static const float CB2_WALL_H = 3.00f;

static const PlayerColliderBox CB2_COLLIDERS[] = {
    { -3.65f,  0.65f,  3.65f,  1.95f }, // Fileira 1 de estacoes
    { -3.65f, -1.75f,  3.65f, -0.45f }, // Fileira 2 de estacoes
};

static const SalaInteractable CB2_INTERACTABLES[] = {
    {
        "fileiras_ciber_2",
        "Estações de Análise de Rede",
        { 0.0f, 0.90f, 1.30f },
        { 6.80f, 1.15f, 1.25f },
        2.25f,
        true,
        "Fileiras organizadas de computadores usados pela equipe de Cibersegurança."
    },
    {
        "persianas_ciber_2",
        "Janelas com Persianas",
        { -3.20f, 1.65f, -3.70f },
        { 3.00f, 1.40f, 0.35f },
        1.95f,
        false,
        "As persianas verticais permanecem fechadas para evitar reflexos nos monitores."
    }
};

static const SalaPorta CB2_PORTAS[] = {
    {
        TERREO_DEST_HALL_2,
        "Porta (Hall 2º Andar)",
        { { -5.85f, 0.0f, -0.70f }, { -5.35f, 2.30f, 0.70f } },
        { -5.40f, 0.0f, 0.00f },
        1.40f,
        { -5.70f, -0.65f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_SERVER_1,
        "Porta (Sala de Servidores 1)",
        { { -0.70f, 0.0f, -3.95f }, { 0.70f, 2.30f, -3.50f } },
        { 0.00f, 0.0f, -3.55f },
        1.40f,
        { -0.65f, -3.88f, 1.30f, 0.58f }
    }
};

static void DesenharFileiraCiber(float zCentro) {
    // Mesa branca com base cinza (escritoriociberseguranca.jpg)
    TerreoCaixa((Vector3){ 0.0f, 0.36f, zCentro }, (Vector3){ 7.00f, 0.70f, 0.78f }, 0.44f, 0.36f, 0.32f, 0.12f);
    TerreoCaixa((Vector3){ 0.0f, 0.74f, zCentro }, (Vector3){ 7.10f, 0.06f, 0.86f }, 0.90f, 0.82f, 0.74f, 0.18f);
    for (int i = -2; i <= 2; i++) {
        float px = (float)i * 1.45f;
        TerreoMonitor((Vector3){ px, 0.77f, zCentro + 0.12f }, 180.0f);
        TerreoGabinetePC((Vector3){ px - 0.44f, 0.77f, zCentro + 0.10f }, 180.0f);
        TerreoCadeira((Vector3){ px, 0.0f,  zCentro - 0.55f }, 0.0f);
    }
}

static void DesenharCiber2(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = CB2_MAX_X - CB2_MIN_X;
    float d = CB2_MAX_Z - CB2_MIN_Z;
    float cx = (CB2_MIN_X + CB2_MAX_X) * 0.5f;
    float cz = (CB2_MIN_Z + CB2_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.44f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, CB2_MIN_Z }, (Vector3){ x, 0.004f, CB2_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ CB2_MIN_X, 0.004f, z }, (Vector3){ CB2_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, CB2_WALL_H * 0.5f, CB2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, CB2_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ CB2_MIN_X - 0.08f, CB2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CB2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ CB2_MAX_X + 0.08f, CB2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CB2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Persianas claras nas laterais da porta dos servidores
    TerreoCaixa((Vector3){ -3.20f, 1.75f, CB2_MIN_Z + 0.03f }, (Vector3){ 3.20f, 1.35f, 0.05f }, 0.84f, 0.78f, 0.72f, 0.22f);
    TerreoCaixa((Vector3){  3.20f, 1.75f, CB2_MIN_Z + 0.03f }, (Vector3){ 3.20f, 1.35f, 0.05f }, 0.84f, 0.78f, 0.72f, 0.22f);

    TerreoPortaModelo((Vector3){ CB2_MIN_X + 0.04f, 0.0f, 0.00f }, 90.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, CB2_MIN_Z + 0.04f },  0.0f, PORTA_TIPO_INDUSTRIAL);

    DesenharFileiraCiber(1.30f);
    DesenharFileiraCiber(-1.10f);
    TerreoSilhuetaSombra((Vector3){ 1.45f, 0.0f, -1.65f }, 0.0f, true);
}

static const TerreoSalaDef SALA_CIBER_2_DEF = {
    .id = TERREO_SALA_CIBER_2,
    .camera = {
        .position   = { 0.0f, 5.85f, 8.50f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { CB2_MIN_X, CB2_MIN_Z, CB2_MAX_X - CB2_MIN_X, CB2_MAX_Z - CB2_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { -4.65f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = CB2_COLLIDERS,
    .numColliders     = (int)(sizeof(CB2_COLLIDERS) / sizeof(CB2_COLLIDERS[0])),
    .interactables    = CB2_INTERACTABLES,
    .numInteractables = (int)(sizeof(CB2_INTERACTABLES) / sizeof(CB2_INTERACTABLES[0])),
    .portas           = CB2_PORTAS,
    .numPortas        = (int)(sizeof(CB2_PORTAS) / sizeof(CB2_PORTAS[0])),
    .drawFunc         = DesenharCiber2
};

const TerreoSalaDef *SalaCiber2Def(void) {
    return &SALA_CIBER_2_DEF;
}
