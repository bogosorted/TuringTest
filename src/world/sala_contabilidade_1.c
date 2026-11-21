#include "terreo_3d.h"
#include "game_state.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Contabilidade 1 (baseado em EscritorioContabilidade.jpg):
// - Baias amplas com divisorias de madeira clara e faixa superior de vidro fosco
// - Vasos de plantas em cima da divisoria principal
// - Fileira de novos armarios (3D_cabinet_7) ao fundo
static const float C1_MIN_X = -5.60f;
static const float C1_MAX_X =  5.60f;
static const float C1_MIN_Z = -4.00f;
static const float C1_MAX_Z =  3.80f;
static const float C1_WALL_H = 3.00f;

static const PlayerColliderBox C1_COLLIDERS[] = {
    { -3.10f,  0.55f,  2.90f,  1.95f }, // Bloco de baias frontal + jardineira divisoria
    { -3.10f, -2.05f,  2.90f, -0.65f }, // Bloco de baias traseiro
    { -4.90f, -3.95f,  3.20f, -3.15f }, // Armarios encostados na parede do fundo
};

static const SalaInteractable C1_INTERACTABLES[] = {
    {
        "ev_maria_devolvidos",
        "Protocolo de Relatórios Devolvidos",
        { 1.55f, 0.90f, 1.25f },
        { 0.95f, 0.85f, 0.95f },
        2.10f,
        true,
        "Protocolo de devolução com 14 relatórios rejeitados e anotações manuais de Maria Bianca."
    },
    {
        "baias_contabilidade_1",
        "Baias da Contabilidade",
        { 0.0f, 0.90f, 1.25f },
        { 4.60f, 1.20f, 1.35f },
        2.25f,
        true,
        "Estações de trabalho da Contabilidade com divisórias de vidro fosco e plantas decorativas."
    },
    {
        "armarios_contabilidade_1",
        "Armários de Balancetes",
        { -1.20f, 1.00f, -3.50f },
        { 3.80f, 1.75f, 0.85f },
        2.10f,
        true,
        "Armários metálicos trancados com pastas fiscais e balancetes mensais da RASEC."
    }
};

static const SalaPorta C1_PORTAS[] = {
    // Esquerda -> Hall 1
    {
        TERREO_DEST_HALL_1,
        "Porta (Hall 1º Andar)",
        { { -5.80f, 0.0f, -0.70f }, { -5.35f, 2.30f, 0.70f } },
        { -5.35f, 0.0f, 0.00f },
        1.40f,
        { -5.70f, -0.65f, 0.55f, 1.30f }
    },
    // Cima (fundo) -> Arquivo
    {
        TERREO_DEST_ARQUIVO_1,
        "Porta (Arquivo)",
        { { -0.70f, 0.0f, -4.15f }, { 0.70f, 2.30f, -3.70f } },
        { 0.00f, 0.0f, -3.75f },
        1.40f,
        { -0.65f, -4.08f, 1.30f, 0.58f }
    },
    // Direita -> Contabilidade 2
    {
        TERREO_DEST_CONTABILIDADE_2,
        "Porta (Contabilidade 2)",
        { { 5.35f, 0.0f, -0.70f }, { 5.80f, 2.30f, 0.70f } },
        { 5.35f, 0.0f, 0.00f },
        1.40f,
        { 5.10f, -0.65f, 0.55f, 1.30f }
    }
};

static void DesenharBaiaContabilidade(Vector3 centro, bool comPlantas) {
    float totalW = 5.40f;
    // Mureta/divisoria frontal de madeira + vidro fosco (EscritorioContabilidade.jpg)
    TerreoCaixa((Vector3){ centro.x, 0.45f, centro.z + 0.55f }, (Vector3){ totalW, 0.90f, 0.14f }, 0.68f, 0.56f, 0.48f, 0.14f);
    TerreoCaixa((Vector3){ centro.x, 1.08f, centro.z + 0.55f }, (Vector3){ totalW, 0.34f, 0.05f }, 0.88f, 0.84f, 0.78f, 0.28f);

    // Tampo de trabalho + monitores + cadeiras
    TerreoCaixa((Vector3){ centro.x, 0.74f, centro.z }, (Vector3){ totalW - 0.15f, 0.05f, 0.95f }, 0.72f, 0.54f, 0.48f, 0.14f);
    for (int i = -1; i <= 1; i++) {
        float px = centro.x + i * 1.70f;
        TerreoMonitor((Vector3){ px, 0.77f, centro.z + 0.25f }, 180.0f);
        TerreoCadeira((Vector3){ px, 0.0f, centro.z - 0.55f }, 0.0f);
    }

    if (comPlantas) {
        for (int i = -2; i <= 2; i++) {
            TerreoPlanta((Vector3){ centro.x + i * 1.15f, 0.92f, centro.z + 0.55f }, 0.72f);
        }
    }
}

static void DesenharContabilidade1(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = C1_MAX_X - C1_MIN_X;
    float d = C1_MAX_Z - C1_MIN_Z;
    float cx = (C1_MIN_X + C1_MAX_X) * 0.5f;
    float cz = (C1_MIN_Z + C1_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.54f));
    // Padrao de placas no piso remetendo ao forro modular da foto
    Color corGrid = TerreoLum(0.42f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, C1_MIN_Z }, (Vector3){ x, 0.004f, C1_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ C1_MIN_X, 0.004f, z }, (Vector3){ C1_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, C1_WALL_H * 0.5f, C1_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, C1_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ C1_MIN_X - 0.08f, C1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, C1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ C1_MAX_X + 0.08f, C1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, C1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    TerreoPortaModelo((Vector3){ C1_MIN_X + 0.04f, 0.0f, 0.0f },  90.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, C1_MIN_Z + 0.04f },   0.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ C1_MAX_X - 0.04f, 0.0f, 0.0f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // Fileira de armarios novos (3D_cabinet_7) ao fundo (lados da porta do Arquivo)
    for (int i = 0; i < 3; i++) {
        TerreoArmario((Vector3){ -4.20f + i * 1.15f, 0.0f, -3.55f }, 0.0f, 1.05f, 1.85f, 0.60f);
        TerreoArmario((Vector3){  1.80f + i * 1.15f, 0.0f, -3.55f }, 0.0f, 1.05f, 1.85f, 0.60f);
    }

    DesenharBaiaContabilidade((Vector3){ -0.10f, 0.0f,  1.25f }, true);
    if (!GameStateTemEvidencia(0, "EV-02")) {
        TerreoEvidenciaBrilhante((Vector3){ 1.55f, 0.77f, 1.25f }, tempo, 1);
    }
    DesenharBaiaContabilidade((Vector3){ -0.10f, 0.0f, -1.35f }, false);
    TerreoSilhuetaSombra((Vector3){ -1.80f, 0.0f, -1.90f }, 0.0f, true);
}

static const TerreoSalaDef SALA_CONTABILIDADE_1_DEF = {
    .id = TERREO_SALA_CONTABILIDADE_1,
    .camera = {
        .position   = { 0.0f, 5.85f, 8.60f },
        .target     = { 0.0f, 0.75f, -0.30f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { C1_MIN_X, C1_MIN_Z, C1_MAX_X - C1_MIN_X, C1_MAX_Z - C1_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { -4.65f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = C1_COLLIDERS,
    .numColliders     = (int)(sizeof(C1_COLLIDERS) / sizeof(C1_COLLIDERS[0])),
    .interactables    = C1_INTERACTABLES,
    .numInteractables = (int)(sizeof(C1_INTERACTABLES) / sizeof(C1_INTERACTABLES[0])),
    .portas           = C1_PORTAS,
    .numPortas        = (int)(sizeof(C1_PORTAS) / sizeof(C1_PORTAS[0])),
    .drawFunc         = DesenharContabilidade1
};

const TerreoSalaDef *SalaContabilidade1Def(void) {
    return &SALA_CONTABILIDADE_1_DEF;
}
