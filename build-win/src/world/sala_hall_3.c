#include "terreo_3d.h"
#include "rlgl.h"
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Hall 3 + Elevador (3o Andar - Treinamento de IA):
// - Esquerda: Treinamento Escritorio (treinamentoiaescritorio.jpg)
// - Direita: Setor de Treinamento de IA (treinamentoia.jpg + treinamentoia_prototipo.jpg)
// - Fundo-direito: Elevador
static const float H3_MIN_X = -5.20f;
static const float H3_MAX_X =  5.20f;
static const float H3_MIN_Z = -3.40f;
static const float H3_MAX_Z =  3.20f;
static const float H3_WALL_H = 3.05f;

static const PlayerColliderBox H3_COLLIDERS[] = {
    { -1.85f,  0.65f,  1.15f,  1.75f },
    { -4.95f,  2.15f, -4.15f,  2.95f },
    { -5.00f, -3.40f, -2.50f, -1.15f }, // Balcao redondo da recepcionista (bloqueia acesso por tras)
};

static const SalaInteractable H3_INTERACTABLES[] = {
    {
        "recepcionista_hall_3",
        "Recepcionista (3º Andar)",
        { -3.75f, 0.95f, -1.75f },
        { 1.45f, 1.35f, 1.05f },
        1.95f,
        false,
        "3º Andar — Treinamento de IA. Escritório à esquerda; Treino de IA e Garagem à direita."
    },
    {
        "diretorio_andar_3",
        "Diretório do 3º Andar",
        { -3.65f, 1.60f, -3.30f },
        { 1.25f, 0.90f, 0.35f },
        1.85f,
        false,
        "3º ANDAR - TREINAMENTO DE IA: Escritório de Treinamento (esquerda), Setor de Treinamento de IA e Garagem de Protótipos (direita)."
    },
    {
        "sofa_hall_3",
        "Sofá do Hall",
        { -0.35f, 0.55f, 1.20f },
        { 2.80f, 0.90f, 1.05f },
        1.95f,
        false,
        "O 3º andar tem cheiro de solda, ozônio e café forte."
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

static const SalaPorta H3_PORTAS[] = {
    {
        TERREO_DEST_TREINO_ESCRITORIO,
        "Porta (Treinamento Escritório)",
        { { -5.45f, 0.0f, -0.25f }, { -4.95f, 2.30f, 1.15f } },
        { -5.00f, 0.0f, 0.45f },
        1.40f,
        { -5.30f, -0.20f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_TREINO_IA,
        "Porta (Setor de Treinamento de IA)",
        { { 4.95f, 0.0f, 0.40f }, { 5.45f, 2.30f, 1.80f } },
        { 5.00f, 0.0f, 1.10f },
        1.40f,
        { 4.70f, 0.45f, 0.55f, 1.30f }
    }
};

static void DesenharHall3(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = H3_MAX_X - H3_MIN_X;
    float d = H3_MAX_Z - H3_MIN_Z;
    float cx = (H3_MIN_X + H3_MAX_X) * 0.5f;
    float cz = (H3_MIN_Z + H3_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.50f));
    Color corGrid = TerreoLum(0.38f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, H3_MIN_Z }, (Vector3){ x, 0.004f, H3_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ H3_MIN_X, 0.004f, z }, (Vector3){ H3_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, H3_WALL_H * 0.5f, H3_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, H3_WALL_H, 0.16f }, 0.32f, 0.40f, 0.28f, 0.12f);
    TerreoCaixa((Vector3){ H3_MIN_X - 0.08f, H3_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H3_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);
    TerreoCaixa((Vector3){ H3_MAX_X + 0.08f, H3_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H3_WALL_H, d }, 0.30f, 0.34f, 0.36f, 0.12f);

    TerreoPortaModelo((Vector3){ H3_MIN_X + 0.04f, 0.0f, 0.45f },  90.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ H3_MAX_X - 0.04f, 0.0f, 1.10f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    TerreoCaixa((Vector3){ -3.65f, 1.60f, H3_MIN_Z + 0.03f },
                (Vector3){ 1.15f, 0.80f, 0.04f }, 0.82f, 0.78f, 0.70f, 0.16f);

    TerreoCaixa((Vector3){ -0.35f, 0.01f, 1.20f }, (Vector3){ 3.40f, 0.02f, 1.65f }, 0.20f, 0.16f, 0.16f, -1.0f);
    TerreoSofaCurvo((Vector3){ -0.35f, 0.0f, 1.20f }, 180.0f, 2.50f);
    TerreoPlanta((Vector3){ -4.55f, 0.0f, 2.55f }, 1.25f);
    TerreoPlanta((Vector3){  4.55f, 0.0f, -2.55f }, 1.20f);

    // Balcao redondo em U da recepcionista do 3o andar
    TerreoBalcaoRecepcaoU((Vector3){ -3.75f, 0.0f, -2.05f });

    TerreoElevador((Vector3){ 3.45f, 0.0f, H3_MIN_Z + 0.04f }, 0.0f);
}

static const TerreoSalaDef SALA_HALL_3_DEF = {
    .id = TERREO_SALA_HALL_3,
    .camera = {
        .position   = { 0.0f, 5.75f, 8.60f },
        .target     = { 0.0f, 0.70f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { H3_MIN_X, H3_MIN_Z, H3_MAX_X - H3_MIN_X, H3_MAX_Z - H3_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { 3.45f, 0.0f, -2.15f },
    .defaultSpawnYaw  = 0.0f,
    .colliders        = H3_COLLIDERS,
    .numColliders     = (int)(sizeof(H3_COLLIDERS) / sizeof(H3_COLLIDERS[0])),
    .interactables    = H3_INTERACTABLES,
    .numInteractables = (int)(sizeof(H3_INTERACTABLES) / sizeof(H3_INTERACTABLES[0])),
    .portas           = H3_PORTAS,
    .numPortas        = (int)(sizeof(H3_PORTAS) / sizeof(H3_PORTAS[0])),
    .drawFunc         = DesenharHall3
};

const TerreoSalaDef *SalaHall3Def(void) {
    return &SALA_HALL_3_DEF;
}
