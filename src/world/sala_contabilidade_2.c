#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Contabilidade 2 (baseado em EscritorioContabilidade2.jpg):
// Mais larga horizontalmente (para os lados), com area menor de apoio/estantes em L
// conectada a mesa comprida de madeira central. Acesso via Contabilidade 1.
static const float C2_MIN_X = -5.80f;
static const float C2_MAX_X =  5.80f;
static const float C2_MIN_Z = -3.10f;
static const float C2_MAX_Z =  3.10f;
static const float C2_WALL_H = 2.95f;

static const PlayerColliderBox C2_COLLIDERS[] = {
    { -1.40f, -1.05f,  3.20f,  1.25f }, // Mesa comprida horizontal de madeira + cadeiras
    { -5.65f, -2.95f, -3.55f, -1.85f }, // Estantes metalicas na area menor ao fundo-esq
    {  4.45f, -2.95f,  5.65f, -1.85f }, // Armario canto direito
    {  3.60f,  1.45f,  5.65f,  2.95f }, // Ala em L para a direita (area de impressora/apoio)
};

static const SalaInteractable C2_INTERACTABLES[] = {
    {
        "mesa_contabilidade_2",
        "Mesa de Auditoria Contábil",
        { 0.90f, 0.85f, 0.10f },
        { 4.30f, 0.90f, 1.70f },
        2.20f,
        true,
        "Uma mesa comprida de madeira com notebooks, fones e pastas azuis de conciliação bancária."
    },
    {
        "estante_contabilidade_2",
        "Estantes com Caixas e Plantas",
        { -4.60f, 1.10f, -2.40f },
        { 2.00f, 1.90f, 1.05f },
        2.05f,
        true,
        "Estantes metálicas abertas na área de apoio, guardando caixas de notas fiscais e plantas."
    }
};

static const SalaPorta C2_PORTAS[] = {
    {
        TERREO_DEST_CONTABILIDADE_1,
        "Porta (Contabilidade 1)",
        { { -5.95f, 0.0f, -0.70f }, { -5.45f, 2.30f, 0.70f } },
        { -5.50f, 0.0f, 0.00f },
        1.40f,
        { -5.85f, -0.65f, 0.55f, 1.30f }
    }
};

static void DesenharEstanteMetalica(Vector3 pos, float largX) {
    float hx = largX * 0.5f;
    float hz = 0.34f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            TerreoCaixa((Vector3){ pos.x + sx * hx, 1.00f, pos.z + sz * hz },
                        (Vector3){ 0.05f, 2.00f, 0.05f }, 0.14f, 0.10f, 0.10f, -1.0f);
        }
    }
    for (int p = 0; p < 4; p++) {
        float py = 0.20f + p * 0.55f;
        TerreoCaixa((Vector3){ pos.x, py, pos.z }, (Vector3){ largX, 0.04f, 0.72f }, 0.52f, 0.42f, 0.38f, 0.12f);
    }
    TerreoCaixa((Vector3){ pos.x - 0.35f, 0.38f, pos.z }, (Vector3){ 0.52f, 0.32f, 0.45f }, 0.64f, 0.54f, 0.48f, 0.14f);
    TerreoPlanta((Vector3){ pos.x + 0.35f, 1.32f, pos.z }, 0.85f);
}

static void DesenharContabilidade2(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = C2_MAX_X - C2_MIN_X;
    float d = C2_MAX_Z - C2_MIN_Z;
    float cx = (C2_MIN_X + C2_MAX_X) * 0.5f;
    float cz = (C2_MIN_Z + C2_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.50f));
    Color corGrid = TerreoLum(0.38f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, C2_MIN_Z }, (Vector3){ x, 0.004f, C2_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ C2_MIN_X, 0.004f, z }, (Vector3){ C2_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, C2_WALL_H * 0.5f, C2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, C2_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ C2_MIN_X - 0.08f, C2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, C2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ C2_MAX_X + 0.08f, C2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, C2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Janela grande ao fundo-direita
    TerreoCaixa((Vector3){ 1.40f, 1.75f, C2_MIN_Z + 0.03f },
                (Vector3){ 4.20f, 1.35f, 0.05f }, 0.90f, 0.86f, 0.80f, 0.16f);

    TerreoPortaModelo((Vector3){ C2_MIN_X + 0.04f, 0.0f, 0.0f }, 90.0f, PORTA_TIPO_ESCRITORIO);

    // Mesa comprida horizontal de madeira (para os lados)
    TerreoCaixa((Vector3){ 0.90f, 0.74f, 0.10f }, (Vector3){ 4.40f, 0.06f, 1.45f }, 0.62f, 0.46f, 0.42f, 0.14f);
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCylinder((Vector3){ 0.90f + sx * 1.95f, 0.0f, -0.50f }, 0.04f, 0.04f, 0.72f, 8, TerreoLum(0.18f));
        DrawCylinder((Vector3){ 0.90f + sx * 1.95f, 0.0f,  0.70f }, 0.04f, 0.04f, 0.72f, 8, TerreoLum(0.18f));
    }

    // Notebooks/Monitores + pastas + cadeiras ao longo da mesa
    for (int k = -1; k <= 1; k++) {
        float kx = 0.90f + k * 1.35f;
        TerreoMonitor((Vector3){ kx, 0.77f, -0.15f }, 0.0f);
        TerreoCadeira((Vector3){ kx, 0.0f, 0.95f }, 180.0f);
        TerreoCaixa((Vector3){ kx + 0.30f, 0.785f, 0.35f }, (Vector3){ 0.28f, 0.04f, 0.24f }, 0.76f, 0.66f, 0.58f, 0.20f);
    }

    // Area menor de apoio (estantes metalicas) + armario + ala em L para a direita
    DesenharEstanteMetalica((Vector3){ -4.60f, 0.0f, -2.40f }, 1.95f);
    TerreoArmario((Vector3){ 5.05f, 0.0f, -2.45f }, 0.0f, 0.95f, 1.85f, 0.60f);
    TerreoCaixa((Vector3){ 4.65f, 0.74f, 2.20f }, (Vector3){ 1.90f, 0.05f, 1.15f }, 0.60f, 0.44f, 0.40f, 0.14f);
    TerreoImpressora((Vector3){ 4.65f, 0.0f, 2.20f }, -90.0f);
}

static const TerreoSalaDef SALA_CONTABILIDADE_2_DEF = {
    .id = TERREO_SALA_CONTABILIDADE_2,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.20f },
        .target     = { 0.0f, 0.75f, -0.15f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { C2_MIN_X, C2_MIN_Z, C2_MAX_X - C2_MIN_X, C2_MAX_Z - C2_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { -4.85f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = C2_COLLIDERS,
    .numColliders     = (int)(sizeof(C2_COLLIDERS) / sizeof(C2_COLLIDERS[0])),
    .interactables    = C2_INTERACTABLES,
    .numInteractables = (int)(sizeof(C2_INTERACTABLES) / sizeof(C2_INTERACTABLES[0])),
    .portas           = C2_PORTAS,
    .numPortas        = (int)(sizeof(C2_PORTAS) / sizeof(C2_PORTAS[0])),
    .drawFunc         = DesenharContabilidade2
};

const TerreoSalaDef *SalaContabilidade2Def(void) {
    return &SALA_CONTABILIDADE_2_DEF;
}
