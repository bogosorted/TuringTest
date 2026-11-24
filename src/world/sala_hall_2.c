#include "terreo_3d.h"
#include "rlgl.h"
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Hall 2 + Elevador (2o Andar - Ciberseguranca):
// - Esquerda: Escritorio Ciberseguranca 1 (entrada mais simples)
// - Direita: Escritorio Ciberseguranca 2 (estacoes em fileiras)
// - Fundo-direito: Elevador
#define H2_MIN_X -5.20f
#define H2_MAX_X 5.20f
#define H2_MIN_Z -3.40f
#define H2_MAX_Z 3.20f
#define H2_WALL_H 3.05f

static const PlayerColliderBox H2_COLLIDERS[] = {
    { -1.85f,  0.65f,  1.15f,  1.75f }, // Sofa central
    { -4.95f,  2.15f, -4.15f,  2.95f }, // Vaso canto esquerdo
    { -5.00f, -3.40f, -2.50f, -1.15f }, // Balcao redondo da recepcionista (bloqueia acesso por tras)
};

static const SalaInteractable H2_INTERACTABLES[] = {
    {
        "recepcionista_hall_2",
        "Recepcionista (2º Andar)",
        { -3.75f, 0.95f, -1.75f },
        { 1.45f, 1.35f, 1.05f },
        1.95f,
        false,
        "2º Andar — Cibersegurança. Escritório 1 à esquerda; Escritório 2 e Servidores à direita."
    },
    {
        "diretorio_andar_2",
        "Diretório do 2º Andar",
        { -3.65f, 1.60f, -3.30f },
        { 1.25f, 0.90f, 0.35f },
        1.85f,
        false,
        "2º ANDAR - CIBERSEGURANÇA: Escritório 1 (esquerda), Escritório 2 e Salas de Servidores (direita)."
    },
    {
        "sofa_hall_2",
        "Sofá do Hall",
        { -0.35f, 0.55f, 1.20f },
        { 2.80f, 0.90f, 1.05f },
        1.95f,
        false,
        "O 2º andar tem iluminação mais fria e portas reforçadas para a área de servidores."
    },
    {
        "elevador",
        "Elevador",
        { 3.45f, 1.15f, -3.15f },
        { 2.15f, 2.30f, 0.65f },
        1.85f,
        true,
        NULL
    }
};

static const SalaPorta H2_PORTAS[] = {
    {
        TERREO_DEST_CIBER_1,
        "Porta (Cibersegurança 1)",
        { { -5.45f, 0.0f, -0.25f }, { -4.95f, 2.30f, 1.15f } },
        { -5.00f, 0.0f, 0.45f },
        1.40f,
        { -5.30f, -0.20f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_CIBER_2,
        "Porta (Cibersegurança 2)",
        { { 4.95f, 0.0f, 0.40f }, { 5.45f, 2.30f, 1.80f } },
        { 5.00f, 0.0f, 1.10f },
        1.40f,
        { 4.70f, 0.45f, 0.55f, 1.30f }
    }
};

static void DesenharHall2(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = H2_MAX_X - H2_MIN_X;
    float d = H2_MAX_Z - H2_MIN_Z;
    float cx = (H2_MIN_X + H2_MAX_X) * 0.5f;
    float cz = (H2_MIN_Z + H2_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.52f));
    Color corGrid = TerreoLum(0.40f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, H2_MIN_Z }, (Vector3){ x, 0.004f, H2_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ H2_MIN_X, 0.004f, z }, (Vector3){ H2_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, H2_WALL_H * 0.5f, H2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, H2_WALL_H, 0.16f }, 0.32f, 0.40f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ H2_MIN_X - 0.08f, H2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H2_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);
    TerreoCaixa((Vector3){ H2_MAX_X + 0.08f, H2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H2_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);

    TerreoPortaModelo((Vector3){ H2_MIN_X + 0.04f, 0.0f, 0.45f },  90.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ H2_MAX_X - 0.04f, 0.0f, 1.10f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    TerreoCaixa((Vector3){ -3.65f, 1.60f, H2_MIN_Z + 0.03f },
                (Vector3){ 1.15f, 0.80f, 0.04f }, 0.82f, 0.78f, 0.70f, 0.16f);

    TerreoCaixa((Vector3){ -0.35f, 0.01f, 1.20f }, (Vector3){ 3.40f, 0.02f, 1.65f }, 0.20f, 0.16f, 0.16f, -1.0f);
    TerreoSofaCurvo((Vector3){ -0.35f, 0.0f, 1.20f }, 180.0f, 2.50f);
    TerreoPlanta((Vector3){ -4.55f, 0.0f, 2.55f }, 1.25f);
    TerreoPlanta((Vector3){  4.55f, 0.0f, -2.55f }, 1.20f);

    // Balcao redondo em U da recepcionista do 2o andar
    TerreoBalcaoRecepcaoU((Vector3){ -3.75f, 0.0f, -2.05f });

    TerreoElevador((Vector3){ 3.45f, 0.0f, H2_MIN_Z + 0.04f }, 0.0f);
}

static const TerreoSalaDef SALA_HALL_2_DEF = {
    .id = TERREO_SALA_HALL_2,
    .camera = {
        .position   = { 0.0f, 5.75f, 8.60f },
        .target     = { 0.0f, 0.70f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { H2_MIN_X, H2_MIN_Z, H2_MAX_X - H2_MIN_X, H2_MAX_Z - H2_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { 3.45f, 0.0f, -2.15f },
    .defaultSpawnYaw  = 0.0f,
    .colliders        = H2_COLLIDERS,
    .numColliders     = (int)(sizeof(H2_COLLIDERS) / sizeof(H2_COLLIDERS[0])),
    .interactables    = H2_INTERACTABLES,
    .numInteractables = (int)(sizeof(H2_INTERACTABLES) / sizeof(H2_INTERACTABLES[0])),
    .portas           = H2_PORTAS,
    .numPortas        = (int)(sizeof(H2_PORTAS) / sizeof(H2_PORTAS[0])),
    .drawFunc         = DesenharHall2
};

const TerreoSalaDef *SalaHall2Def(void) {
    return &SALA_HALL_2_DEF;
}
