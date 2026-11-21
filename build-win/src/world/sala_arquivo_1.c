#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Sala de Arquivo (3o escritorio do 1o Andar):
// Repleta dos novos armarios/gabinetes (baseados em 3D_cabinet_7) + mesa de triagem e impressora.
static const float ARQ_MIN_X = -4.60f;
static const float ARQ_MAX_X =  4.60f;
static const float ARQ_MIN_Z = -3.60f;
static const float ARQ_MAX_Z =  3.60f;
static const float ARQ_WALL_H = 2.95f;

static const PlayerColliderBox ARQ_COLLIDERS[] = {
    { -4.45f, -3.55f,  4.45f, -2.75f }, // Fileira de armarios ao fundo
    { -4.45f, -1.80f, -3.25f,  1.80f }, // Armarios parede esquerda
    {  3.25f, -1.80f,  4.45f,  1.80f }, // Armarios parede direita
    { -1.35f, -0.65f,  1.35f,  0.55f }, // Mesa central de triagem
};

static const SalaInteractable ARQ_INTERACTABLES[] = {
    {
        "armarios_arquivo_fundo",
        "Armários de Arquivo",
        { 0.0f, 1.00f, -3.15f },
        { 5.60f, 1.85f, 0.85f },
        2.15f,
        true,
        "Armários numerados de aço cinza guardam contratos, notas fiscais e dossiês dos últimos anos."
    },
    {
        "mesa_triagem_arquivo",
        "Mesa de Triagem",
        { 0.0f, 0.85f, -0.05f },
        { 2.50f, 1.00f, 1.15f },
        1.95f,
        true,
        "Uma mesa de consulta com terminal de catálogo e uma impressora para segunda via de documentos."
    }
};

static const SalaPorta ARQ_PORTAS[] = {
    {
        TERREO_DEST_CONTABILIDADE_1,
        "Porta (Contabilidade 1)",
        { { -0.70f, 0.0f, 3.35f }, { 0.70f, 2.30f, 3.75f } },
        { 0.00f, 0.0f, 3.35f },
        1.40f,
        { -0.65f, 3.10f, 1.30f, 0.58f }
    }
};

static void DesenharArquivo1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = ARQ_MAX_X - ARQ_MIN_X;
    float d = ARQ_MAX_Z - ARQ_MIN_Z;
    float cx = (ARQ_MIN_X + ARQ_MAX_X) * 0.5f;
    float cz = (ARQ_MIN_Z + ARQ_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.52f));
    Color corGrid = TerreoLum(0.40f);
    for (float x = -4.0f; x <= 4.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, ARQ_MIN_Z }, (Vector3){ x, 0.004f, ARQ_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ ARQ_MIN_X, 0.004f, z }, (Vector3){ ARQ_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, ARQ_WALL_H * 0.5f, ARQ_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, ARQ_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ ARQ_MIN_X - 0.08f, ARQ_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ARQ_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ ARQ_MAX_X + 0.08f, ARQ_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ARQ_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, ARQ_MAX_Z - 0.04f }, 180.0f, PORTA_TIPO_ESCRITORIO);
    // Porta decorativa a direita para Sala de Manutencao (PrimeiroAndarRASEC3D.png)
    TerreoPortaModelo((Vector3){ ARQ_MAX_X - 0.04f, 0.0f, -1.80f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    // Fileira completa de armarios novos (3D_cabinet_7) ao fundo e nas laterais
    for (int i = 0; i < 7; i++) {
        TerreoArmario((Vector3){ -3.60f + i * 1.20f, 0.0f, -3.18f }, 0.0f, 1.08f, 1.95f, 0.62f);
    }
    for (int i = 0; i < 3; i++) {
        float kz = -1.25f + i * 1.25f;
        TerreoArmario((Vector3){ -4.15f, 0.0f, kz },  90.0f, 1.08f, 1.95f, 0.62f);
        TerreoArmario((Vector3){  4.15f, 0.0f, kz }, -90.0f, 1.08f, 1.95f, 0.62f);
    }

    // Mesa central de triagem + monitor + impressora
    TerreoCaixa((Vector3){ 0.0f, 0.74f, -0.05f }, (Vector3){ 2.40f, 0.06f, 1.00f }, 0.68f, 0.50f, 0.44f, 0.14f);
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCylinder((Vector3){ sx * 1.05f, 0.0f, -0.45f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.20f));
        DrawCylinder((Vector3){ sx * 1.05f, 0.0f,  0.35f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.20f));
    }
    TerreoMonitor((Vector3){ -0.55f, 0.77f, -0.05f }, 0.0f);
    TerreoImpressora((Vector3){ 0.65f, 0.0f, -0.05f }, 0.0f);
}

static const TerreoSalaDef SALA_ARQUIVO_1_DEF = {
    .id = TERREO_SALA_ARQUIVO_1,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.10f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { ARQ_MIN_X, ARQ_MIN_Z, ARQ_MAX_X - ARQ_MIN_X, ARQ_MAX_Z - ARQ_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 0.0f, 0.0f, 2.75f },
    .defaultSpawnYaw  = PI,
    .colliders        = ARQ_COLLIDERS,
    .numColliders     = (int)(sizeof(ARQ_COLLIDERS) / sizeof(ARQ_COLLIDERS[0])),
    .interactables    = ARQ_INTERACTABLES,
    .numInteractables = (int)(sizeof(ARQ_INTERACTABLES) / sizeof(ARQ_INTERACTABLES[0])),
    .portas           = ARQ_PORTAS,
    .numPortas        = (int)(sizeof(ARQ_PORTAS) / sizeof(ARQ_PORTAS[0])),
    .drawFunc         = DesenharArquivo1
};

const TerreoSalaDef *SalaArquivo1Def(void) {
    return &SALA_ARQUIVO_1_DEF;
}
