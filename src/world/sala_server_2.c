#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Sala de Servidores 2 (ala restrita ao fundo, baseada em serverciberseguranca.jpg):
// - Corredor estreito de racks de alta densidade + terminal de diagnostico
// - Esquerda: volta para Sala de Servidores 1
static const float SRV2_MIN_X = -4.20f;
static const float SRV2_MAX_X =  4.20f;
static const float SRV2_MIN_Z = -3.40f;
static const float SRV2_MAX_Z =  3.40f;
static const float SRV2_WALL_H = 3.05f;

static const PlayerColliderBox SRV2_COLLIDERS[] = {
    { -2.45f, -3.20f,  3.65f, -1.95f }, // Fileira de racks ao fundo
    {  1.55f,  0.35f,  3.65f,  1.65f }, // Bancada de diagnostico
};

static const SalaInteractable SRV2_INTERACTABLES[] = {
    {
        "racks_server_2",
        "Servidores Centrais (Ala 2)",
        { 0.60f, 1.15f, -2.60f },
        { 5.60f, 2.20f, 1.25f },
        2.15f,
        true,
        "Servidores principais de armazenamento e backup da RASEC. Acesso restrito à Cibersegurança."
    },
    {
        "terminal_server_2",
        "Terminal de Diagnóstico",
        { 2.60f, 0.90f, 1.00f },
        { 1.85f, 1.15f, 1.15f },
        1.90f,
        true,
        "Um carrinho/bancada de manutenção conectado diretamente aos racks centrais."
    }
};

static const SalaPorta SRV2_PORTAS[] = {
    {
        TERREO_DEST_SERVER_1,
        "Porta (Sala de Servidores 1)",
        { { -4.45f, 0.0f, -0.70f }, { -3.95f, 2.30f, 0.70f } },
        { -4.00f, 0.0f, 0.00f },
        1.40f,
        { -4.30f, -0.65f, 0.55f, 1.30f }
    }
};

static void DesenharServer2(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = SRV2_MAX_X - SRV2_MIN_X;
    float d = SRV2_MAX_Z - SRV2_MIN_Z;
    float cx = (SRV2_MIN_X + SRV2_MAX_X) * 0.5f;
    float cz = (SRV2_MIN_Z + SRV2_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.46f));
    Color corGrid = TerreoLum(0.34f);
    for (float x = -4.0f; x <= 4.01f; x += 0.8f) {
        DrawLine3D((Vector3){ x, 0.004f, SRV2_MIN_Z }, (Vector3){ x, 0.004f, SRV2_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 0.8f) {
        DrawLine3D((Vector3){ SRV2_MIN_X, 0.004f, z }, (Vector3){ SRV2_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, SRV2_WALL_H * 0.5f, SRV2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, SRV2_WALL_H, 0.16f }, 0.30f, 0.36f, 0.26f, 0.10f);
    TerreoCaixa((Vector3){ SRV2_MIN_X - 0.08f, SRV2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SRV2_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.10f);
    TerreoCaixa((Vector3){ SRV2_MAX_X + 0.08f, SRV2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SRV2_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.10f);

    TerreoPortaModelo((Vector3){ SRV2_MIN_X + 0.04f, 0.0f, 0.00f }, 90.0f, PORTA_TIPO_INDUSTRIAL);

    // Eletrocalha superior + fileira de 5 racks
    TerreoCaixa((Vector3){ 0.60f, 2.65f, -2.60f }, (Vector3){ 5.80f, 0.08f, 0.70f }, 0.38f, 0.30f, 0.26f, 0.14f);
    for (int i = 0; i < 5; i++) {
        TerreoRackServidor((Vector3){ -1.75f + i * 1.18f, 0.0f, -2.60f }, 0.0f);
    }

    // Bancada de diagnostico
    TerreoCaixa((Vector3){ 2.60f, 0.74f, 1.00f }, (Vector3){ 1.85f, 0.05f, 1.00f }, 0.60f, 0.44f, 0.40f, 0.14f);
    TerreoMonitor((Vector3){ 2.60f, 0.77f, 1.00f }, 180.0f);
    TerreoGabinetePC((Vector3){ 2.12f, 0.77f, 1.00f }, 180.0f);
    TerreoCadeira((Vector3){ 2.60f, 0.0f, 0.15f }, 0.0f);
}

static const TerreoSalaDef SALA_SERVER_2_DEF = {
    .id = TERREO_SALA_SERVER_2,
    .camera = {
        .position   = { 0.0f, 5.25f, 7.80f },
        .target     = { 0.0f, 0.85f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { SRV2_MIN_X, SRV2_MIN_Z, SRV2_MAX_X - SRV2_MIN_X, SRV2_MAX_Z - SRV2_MIN_Z },
    .bgColor          = { 18, 16, 18, 255 },
    .defaultSpawnPos  = { -3.25f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = SRV2_COLLIDERS,
    .numColliders     = (int)(sizeof(SRV2_COLLIDERS) / sizeof(SRV2_COLLIDERS[0])),
    .interactables    = SRV2_INTERACTABLES,
    .numInteractables = (int)(sizeof(SRV2_INTERACTABLES) / sizeof(SRV2_INTERACTABLES[0])),
    .portas           = SRV2_PORTAS,
    .numPortas        = (int)(sizeof(SRV2_PORTAS) / sizeof(SRV2_PORTAS[0])),
    .drawFunc         = DesenharServer2
};

const TerreoSalaDef *SalaServer2Def(void) {
    return &SALA_SERVER_2_DEF;
}
