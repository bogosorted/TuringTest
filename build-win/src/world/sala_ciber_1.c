#include "terreo_3d.h"
#include "game_state.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Escritorio Ciberseguranca 1 (baseado em ciberseguranca.zip / IndoorEnvironmentHDRI003):
// - Bancadas em L/curva ao longo das paredes com varios monitores e gabinetes
// - Janelas claras ao fundo + duto de ar cilindrico no alto
// - Armarios (3D_cabinet_7) na lateral
static const float CB1_MIN_X = -5.40f;
static const float CB1_MAX_X =  5.40f;
static const float CB1_MIN_Z = -3.60f;
static const float CB1_MAX_Z =  3.40f;
static const float CB1_WALL_H = 3.00f;

static const PlayerColliderBox CB1_COLLIDERS[] = {
    { -5.25f, -3.45f,  2.85f, -2.35f }, // Bancada longa ao fundo
    { -5.25f, -2.35f, -4.15f,  2.25f }, // Bancada lateral esquerda em L
    {  3.75f, -3.45f,  5.25f, -2.45f }, // Armarios canto direito-fundo
};

static const SalaInteractable CB1_INTERACTABLES[] = {
    {
        "bancada_ciber_1",
        "Bancada de Monitoramento",
        { -1.20f, 0.90f, -2.90f },
        { 5.40f, 1.15f, 1.05f },
        2.15f,
        true,
        "Estações de triagem de incidentes e logs de rede. As telas exibem alertas rotineiros."
    },
    {
        "armarios_ciber_1",
        "Armários de Equipamentos",
        { 4.50f, 1.00f, -2.95f },
        { 1.45f, 1.85f, 0.85f },
        1.95f,
        true,
        "Armários metálicos com cabos de rede sobressalentes e tokens de hardware."
    }
};

static const SalaPorta CB1_PORTAS[] = {
    {
        TERREO_DEST_HALL_2,
        "Porta (Hall 2º Andar)",
        { { 5.15f, 0.0f, -0.25f }, { 5.65f, 2.30f, 1.15f } },
        { 5.20f, 0.0f, 0.45f },
        1.40f,
        { 4.90f, -0.20f, 0.55f, 1.30f }
    }
};

static void DesenharCiber1(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = CB1_MAX_X - CB1_MIN_X;
    float d = CB1_MAX_Z - CB1_MIN_Z;
    float cx = (CB1_MIN_X + CB1_MAX_X) * 0.5f;
    float cz = (CB1_MIN_Z + CB1_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.54f));
    Color corGrid = TerreoLum(0.42f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, CB1_MIN_Z }, (Vector3){ x, 0.004f, CB1_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ CB1_MIN_X, 0.004f, z }, (Vector3){ CB1_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, CB1_WALL_H * 0.5f, CB1_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, CB1_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ CB1_MIN_X - 0.08f, CB1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CB1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ CB1_MAX_X + 0.08f, CB1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CB1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Janelas ao fundo e duto de ar metalico no alto (ciberseguranca.zip)
    TerreoCaixa((Vector3){ -0.80f, 1.85f, CB1_MIN_Z + 0.03f }, (Vector3){ 3.20f, 1.15f, 0.05f }, 0.90f, 0.86f, 0.80f, 0.16f);
    DrawCylinderEx((Vector3){ -5.20f, 2.75f, -3.10f }, (Vector3){ 5.20f, 2.75f, -3.10f }, 0.16f, 0.16f, 12, TerreoLum(0.68f));

    TerreoPortaModelo((Vector3){ CB1_MAX_X - 0.04f, 0.0f, 0.45f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // Bancadas em L ao fundo e esquerda + monitores + cadeiras
    TerreoCaixa((Vector3){ -1.20f, 0.74f, -2.90f }, (Vector3){ 7.80f, 0.05f, 0.95f }, 0.76f, 0.62f, 0.56f, 0.14f);
    TerreoCaixa((Vector3){ -4.70f, 0.74f, -0.10f }, (Vector3){ 0.95f, 0.05f, 4.70f }, 0.76f, 0.62f, 0.56f, 0.14f);
    for (int i = 0; i < 4; i++) {
        float mx = -3.80f + i * 1.85f;
        TerreoMonitor((Vector3){ mx, 0.77f, -3.10f }, 0.0f);
        TerreoCadeira((Vector3){ mx, 0.0f,  -2.05f }, 180.0f);
    }
    for (int j = 0; j < 2; j++) {
        float mz = -0.95f + j * 1.95f;
        TerreoMonitor((Vector3){ -4.90f, 0.77f, mz }, 90.0f);
        TerreoCadeira((Vector3){ -3.85f, 0.0f,  mz }, -90.0f);
    }
    TerreoSilhuetaSombra((Vector3){ -1.95f, 0.0f, -2.05f }, 180.0f, true);

    TerreoArmario((Vector3){ 4.50f, 0.0f, -2.95f }, 0.0f, 1.15f, 1.85f, 0.60f);
}

static const TerreoSalaDef SALA_CIBER_1_DEF = {
    .id = TERREO_SALA_CIBER_1,
    .camera = {
        .position   = { 0.0f, 5.65f, 8.30f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { CB1_MIN_X, CB1_MIN_Z, CB1_MAX_X - CB1_MIN_X, CB1_MAX_Z - CB1_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { 4.35f, 0.0f, 0.45f },
    .defaultSpawnYaw  = -PI * 0.5f,
    .colliders        = CB1_COLLIDERS,
    .numColliders     = (int)(sizeof(CB1_COLLIDERS) / sizeof(CB1_COLLIDERS[0])),
    .interactables    = CB1_INTERACTABLES,
    .numInteractables = (int)(sizeof(CB1_INTERACTABLES) / sizeof(CB1_INTERACTABLES[0])),
    .portas           = CB1_PORTAS,
    .numPortas        = (int)(sizeof(CB1_PORTAS) / sizeof(CB1_PORTAS[0])),
    .drawFunc         = DesenharCiber1
};

const TerreoSalaDef *SalaCiber1Def(void) {
    return &SALA_CIBER_1_DEF;
}
