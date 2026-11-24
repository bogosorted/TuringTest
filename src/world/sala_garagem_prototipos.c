#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Garagem de Prototipos (3o Andar, conectada ao Setor de Treinamento de IA):
// - Unidades de prototipos em montagem, bancadas tecnicas e armarios de pecas
static const float GP_MIN_X = -4.80f;
static const float GP_MAX_X =  4.80f;
static const float GP_MIN_Z = -3.60f;
static const float GP_MAX_Z =  3.40f;
static const float GP_WALL_H = 3.10f;

static const PlayerColliderBox GP_COLLIDERS[] = {
    { -3.85f, -3.25f, -1.75f, -1.95f }, // Bancada de montagem esquerda
    {  1.55f, -3.25f,  4.45f, -2.15f }, // Armarios de pecas direita
    { -0.65f, -2.55f,  0.85f, -1.35f }, // Plataforma de testes com robo
};

static const SalaInteractable GP_INTERACTABLES[] = {
    {
        "plataforma_prototipos",
        "Plataforma de Testes",
        { 0.10f, 1.15f, -1.95f },
        { 1.45f, 2.15f, 1.25f },
        2.05f,
        false,
        "Outro protótipo humanoide em fase de montagem e alinhamento de juntas."
    },
    {
        "bancada_garagem",
        "Bancada de Ferramentas",
        { -2.80f, 0.90f, -2.60f },
        { 2.00f, 1.15f, 1.15f },
        1.95f,
        true,
        "Chaves de precisão, servomotores reservas e carcaças plásticas."
    }
};

static const SalaPorta GP_PORTAS[] = {
    {
        TERREO_DEST_TREINO_IA,
        "Porta (Setor de Treinamento de IA)",
        { { -0.70f, 0.0f, 3.15f }, { 0.70f, 2.30f, 3.55f } },
        { 0.00f, 0.0f, 3.15f },
        1.40f,
        { -0.65f, 2.90f, 1.30f, 0.58f }
    }
};

static void DesenharGaragemPrototipos(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = GP_MAX_X - GP_MIN_X;
    float d = GP_MAX_Z - GP_MIN_Z;
    float cx = (GP_MIN_X + GP_MAX_X) * 0.5f;
    float cz = (GP_MIN_Z + GP_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.48f));
    Color corGrid = TerreoLum(0.36f);
    for (float x = -4.0f; x <= 4.01f; x += 0.8f) {
        DrawLine3D((Vector3){ x, 0.004f, GP_MIN_Z }, (Vector3){ x, 0.004f, GP_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 0.8f) {
        DrawLine3D((Vector3){ GP_MIN_X, 0.004f, z }, (Vector3){ GP_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, GP_WALL_H * 0.5f, GP_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, GP_WALL_H, 0.16f }, 0.30f, 0.38f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ GP_MIN_X - 0.08f, GP_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, GP_WALL_H, d }, 0.28f, 0.34f, 0.36f, 0.12f);
    TerreoCaixa((Vector3){ GP_MAX_X + 0.08f, GP_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, GP_WALL_H, d }, 0.28f, 0.34f, 0.36f, 0.12f);

    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, GP_MAX_Z - 0.04f }, 180.0f, PORTA_TIPO_INDUSTRIAL);

    // Plataforma central + prototipo
    TerreoCaixa((Vector3){ 0.10f, 0.08f, -1.95f }, (Vector3){ 1.35f, 0.16f, 1.15f }, 0.34f, 0.24f, 0.22f, 0.16f);
    TerreoRoboPrototipo((Vector3){ 0.10f, 0.16f, -1.95f }, 10.0f);

    // Bancada de montagem + armarios de pecas (TerreoArmario)
    TerreoCaixa((Vector3){ -2.80f, 0.74f, -2.60f }, (Vector3){ 1.95f, 0.06f, 1.00f }, 0.60f, 0.44f, 0.40f, 0.14f);
    TerreoMonitor((Vector3){ -2.80f, 0.77f, -2.65f }, 0.0f);
    TerreoGabinetePC((Vector3){ -2.32f, 0.77f, -2.62f }, 0.0f);
    TerreoArmario((Vector3){ 2.25f, 0.0f, -2.85f }, 0.0f, 1.05f, 1.85f, 0.60f);
    TerreoArmario((Vector3){ 3.45f, 0.0f, -2.85f }, 0.0f, 1.05f, 1.85f, 0.60f);
}

static const TerreoSalaDef SALA_GARAGEM_PROTOTIPOS_DEF = {
    .id = TERREO_SALA_GARAGEM_PROTOTIPOS,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.10f },
        .target     = { 0.0f, 0.80f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { GP_MIN_X, GP_MIN_Z, GP_MAX_X - GP_MIN_X, GP_MAX_Z - GP_MIN_Z },
    .bgColor          = { 20, 18, 20, 255 },
    .defaultSpawnPos  = { 0.0f, 0.0f, 2.65f },
    .defaultSpawnYaw  = PI,
    .colliders        = GP_COLLIDERS,
    .numColliders     = (int)(sizeof(GP_COLLIDERS) / sizeof(GP_COLLIDERS[0])),
    .interactables    = GP_INTERACTABLES,
    .numInteractables = (int)(sizeof(GP_INTERACTABLES) / sizeof(GP_INTERACTABLES[0])),
    .portas           = GP_PORTAS,
    .numPortas        = (int)(sizeof(GP_PORTAS) / sizeof(GP_PORTAS[0])),
    .drawFunc         = DesenharGaragemPrototipos
};

const TerreoSalaDef *SalaGaragemPrototiposDef(void) {
    return &SALA_GARAGEM_PROTOTIPOS_DEF;
}
