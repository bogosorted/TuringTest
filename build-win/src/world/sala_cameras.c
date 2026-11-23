#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Sala das Cameras (ao lado do Arquivo, 1o Andar):
// Sala secreta de monitoramento sem itens interagiveis visiveis.
static const float CAM_MIN_X = -3.40f;
static const float CAM_MAX_X =  3.40f;
static const float CAM_MIN_Z = -2.80f;
static const float CAM_MAX_Z =  2.80f;
static const float CAM_WALL_H = 2.95f;

static const PlayerColliderBox CAM_COLLIDERS[] = {
    { -2.45f, -2.55f,  2.45f, -1.35f }, // Bancada com monitores de seguranca
    { -0.45f, -1.05f,  0.45f, -0.35f }, // Cadeira do operador
    {  2.45f, -2.55f,  3.25f, -1.55f }, // Rack de gravacao DVR
};

static const SalaPorta CAM_PORTAS[] = {
    {
        TERREO_DEST_ARQUIVO_1,
        "Porta (Arquivo)",
        { { -3.55f, 0.0f, 0.80f }, { -3.05f, 2.30f, 2.20f } },
        { -3.05f, 0.0f, 1.50f },
        1.40f,
        { -3.45f, 0.85f, 0.55f, 1.30f }
    }
};

static void DesenharSalaCameras(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = CAM_MAX_X - CAM_MIN_X;
    float d = CAM_MAX_Z - CAM_MIN_Z;
    float cx = (CAM_MIN_X + CAM_MAX_X) * 0.5f;
    float cz = (CAM_MIN_Z + CAM_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.36f));
    Color corGrid = TerreoLum(0.28f);
    for (float x = -3.0f; x <= 3.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, CAM_MIN_Z }, (Vector3){ x, 0.004f, CAM_MAX_Z }, corGrid);
    }
    for (float z = -2.0f; z <= 2.01f; z += 1.0f) {
        DrawLine3D((Vector3){ CAM_MIN_X, 0.004f, z }, (Vector3){ CAM_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, CAM_WALL_H * 0.5f, CAM_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, CAM_WALL_H, 0.16f }, 0.30f, 0.36f, 0.26f, 0.12f);
    TerreoCaixa((Vector3){ CAM_MIN_X - 0.08f, CAM_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CAM_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.12f);
    TerreoCaixa((Vector3){ CAM_MAX_X + 0.08f, CAM_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CAM_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.12f);

    TerreoPortaModelo((Vector3){ CAM_MIN_X + 0.04f, 0.0f, 1.50f }, 90.0f, PORTA_TIPO_INDUSTRIAL);

    // Bancada de monitoramento + cadeira + rack DVR
    TerreoCaixa((Vector3){ 0.0f, 0.74f, -1.95f }, (Vector3){ 4.60f, 0.07f, 1.00f }, 0.52f, 0.38f, 0.34f, 0.14f);
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCylinder((Vector3){ sx * 2.10f, 0.0f, -2.35f }, 0.04f, 0.04f, 0.72f, 8, TerreoLum(0.18f));
        DrawCylinder((Vector3){ sx * 2.10f, 0.0f, -1.55f }, 0.04f, 0.04f, 0.72f, 8, TerreoLum(0.18f));
    }
    TerreoCadeiraConfortavel((Vector3){ 0.0f, 0.0f, -0.72f }, 180.0f);
    TerreoRackServidor((Vector3){ 2.85f, 0.0f, -2.05f }, 0.0f);

    // Fileira inferior e superior de monitores de CFTV
    for (int i = 0; i < 4; i++) {
        float mx = -1.50f + i * 1.00f;
        TerreoMonitor((Vector3){ mx, 0.78f, -2.05f }, 0.0f);
        // Painel de TV/monitor suspenso na parede
        TerreoCaixa((Vector3){ mx, 1.82f, -2.62f }, (Vector3){ 0.86f, 0.54f, 0.06f }, 0.26f, 0.20f, 0.20f, 0.10f);
        float lumTela = 0.62f + 0.10f * sinf(tempo * 2.0f + (float)i * 1.3f);
        TerreoCaixa((Vector3){ mx, 1.82f, -2.58f }, (Vector3){ 0.78f, 0.46f, 0.02f }, lumTela, lumTela, lumTela, 0.14f);
    }
}

static const TerreoSalaDef SALA_CAMERAS_DEF = {
    .id = TERREO_SALA_CAMERAS,
    .camera = {
        .position   = { 0.0f, 5.10f, 7.20f },
        .target     = { 0.0f, 0.80f, -0.35f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { CAM_MIN_X, CAM_MIN_Z, CAM_MAX_X - CAM_MIN_X, CAM_MAX_Z - CAM_MIN_Z },
    .bgColor          = { 18, 16, 18, 255 },
    .defaultSpawnPos  = { -2.55f, 0.0f, 1.50f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = CAM_COLLIDERS,
    .numColliders     = (int)(sizeof(CAM_COLLIDERS) / sizeof(CAM_COLLIDERS[0])),
    .interactables    = NULL,
    .numInteractables = 0,
    .portas           = CAM_PORTAS,
    .numPortas        = (int)(sizeof(CAM_PORTAS) / sizeof(CAM_PORTAS[0])),
    .drawFunc         = DesenharSalaCameras
};

const TerreoSalaDef *SalaCamerasDef(void) {
    return &SALA_CAMERAS_DEF;
}
