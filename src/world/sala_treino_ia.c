#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Setor de Treinamento de IA (juncao de treinamentoia.jpg + treinamentoia_prototipo.jpg):
// - Parede tecnica ao fundo com eletrocalhas, cabos, trilhos de disjuntores e caixas de medidores eletricos
// - Prototipo humanoide branco articulado em destaque ao lado da bancada de testes
// - Esquerda: Hall 3 | Fundo-direita: Garagem de Prototipos
static const float TIA_MIN_X = -5.40f;
static const float TIA_MAX_X =  5.40f;
static const float TIA_MIN_Z = -3.80f;
static const float TIA_MAX_Z =  3.60f;
static const float TIA_WALL_H = 3.10f;

static const PlayerColliderBox TIA_COLLIDERS[] = {
    { -4.60f, -3.65f, -1.60f, -2.55f }, // Quadros eletricos e caixas de energia na base da parede
    {  2.20f, -2.95f,  3.60f, -1.75f }, // Pedestal + prototipo humanoide
    { -2.40f,  0.45f,  1.60f,  1.75f }, // Bancada de calibracao
};

static const SalaInteractable TIA_INTERACTABLES[] = {
    {
        "painel_energia_ia",
        "Painel Elétrico de Alta Carga",
        { -3.10f, 1.45f, -3.35f },
        { 3.60f, 2.30f, 0.90f },
        2.20f,
        true,
        "Disjuntores, eletrocalhas e medidores industriais alimentam os experimentos de IA deste andar."
    },
    {
        "robo_prototipo",
        "Protótipo Humanoide",
        { 2.90f, 1.15f, -2.35f },
        { 1.25f, 2.15f, 1.15f },
        2.05f,
        false,
        "Um protótipo humanoide branco articulado desligado. Seus olhos escuros parecem observar o laboratório."
    },
    {
        "bancada_calibracao_ia",
        "Bancada de Calibração",
        { -0.40f, 0.85f, 1.10f },
        { 3.60f, 1.10f, 1.25f },
        2.05f,
        true,
        "Sensores, multímetros e terminais conectados ao chassi de testes."
    }
};

static const SalaPorta TIA_PORTAS[] = {
    {
        TERREO_DEST_HALL_3,
        "Porta (Hall 3º Andar)",
        { { -5.65f, 0.0f, -0.70f }, { -5.15f, 2.30f, 0.70f } },
        { -5.20f, 0.0f, 0.00f },
        1.40f,
        { -5.50f, -0.65f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_GARAGEM_PROTOTIPOS,
        "Porta (Garagem de Protótipos)",
        { { -0.35f, 0.0f, -3.95f }, { 1.05f, 2.30f, -3.50f } },
        { 0.35f, 0.0f, -3.55f },
        1.40f,
        { -0.30f, -3.88f, 1.30f, 0.58f }
    }
};

static void DesenharTreinoIA(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = TIA_MAX_X - TIA_MIN_X;
    float d = TIA_MAX_Z - TIA_MIN_Z;
    float cx = (TIA_MIN_X + TIA_MAX_X) * 0.5f;
    float cz = (TIA_MIN_Z + TIA_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.54f));
    Color corGrid = TerreoLum(0.42f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, TIA_MIN_Z }, (Vector3){ x, 0.004f, TIA_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ TIA_MIN_X, 0.004f, z }, (Vector3){ TIA_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, TIA_WALL_H * 0.5f, TIA_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, TIA_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ TIA_MIN_X - 0.08f, TIA_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, TIA_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ TIA_MAX_X + 0.08f, TIA_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, TIA_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    TerreoPortaModelo((Vector3){ TIA_MIN_X + 0.04f, 0.0f, 0.00f }, 90.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ 0.35f, 0.0f, TIA_MIN_Z + 0.04f },  0.0f, PORTA_TIPO_INDUSTRIAL);

    // Parede tecnica (treinamentoia.jpg): eletrocalha superior, trilho de disjuntores e caixas de medidores
    TerreoCaixa((Vector3){ -3.10f, 2.75f, -3.45f }, (Vector3){ 4.10f, 0.08f, 0.55f }, 0.46f, 0.38f, 0.34f, 0.16f);
    TerreoCaixa((Vector3){ -3.10f, 2.05f, -3.72f }, (Vector3){ 3.40f, 0.24f, 0.08f }, 0.72f, 0.64f, 0.56f, 0.18f);
    for (int m = 0; m < 4; m++) {
        float mx = -4.25f + m * 0.78f;
        TerreoCaixa((Vector3){ mx, 1.15f, -3.66f }, (Vector3){ 0.52f, 0.60f, 0.18f }, 0.80f, 0.74f, 0.66f, 0.16f);
        DrawLine3D((Vector3){ mx, 1.45f, -3.66f }, (Vector3){ mx, 2.72f, -3.55f }, TerreoLum(0.12f));
    }
    TerreoCaixa((Vector3){ -2.35f, 0.48f, -3.15f }, (Vector3){ 1.10f, 0.96f, 0.65f }, 0.68f, 0.58f, 0.50f, 0.16f);

    // Prototipo humanoide branco articulado (treinamentoia_prototipo.jpg) sobre base
    TerreoCaixa((Vector3){ 2.90f, 0.06f, -2.35f }, (Vector3){ 1.05f, 0.12f, 0.95f }, 0.30f, 0.22f, 0.20f, 0.12f);
    TerreoRoboPrototipo((Vector3){ 2.90f, 0.12f, -2.35f }, -18.0f);

    // Bancada central de calibracao
    TerreoCaixa((Vector3){ -0.40f, 0.74f, 1.10f }, (Vector3){ 3.80f, 0.06f, 1.05f }, 0.64f, 0.48f, 0.42f, 0.14f);
    TerreoMonitor((Vector3){ -1.25f, 0.77f, 1.05f }, 0.0f);
    TerreoGabinetePC((Vector3){ -1.72f, 0.77f, 1.05f }, 0.0f);
    TerreoMonitor((Vector3){  0.45f, 0.77f, 1.05f }, 0.0f);
    TerreoGabinetePC((Vector3){  0.92f, 0.77f, 1.05f }, 0.0f);
    TerreoCadeira((Vector3){ -0.40f, 0.0f,  1.85f }, 180.0f);
}

static const TerreoSalaDef SALA_TREINO_IA_DEF = {
    .id = TERREO_SALA_TREINO_IA,
    .camera = {
        .position   = { 0.0f, 5.75f, 8.40f },
        .target     = { 0.0f, 0.80f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { TIA_MIN_X, TIA_MIN_Z, TIA_MAX_X - TIA_MIN_X, TIA_MAX_Z - TIA_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { -4.45f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = TIA_COLLIDERS,
    .numColliders     = (int)(sizeof(TIA_COLLIDERS) / sizeof(TIA_COLLIDERS[0])),
    .interactables    = TIA_INTERACTABLES,
    .numInteractables = (int)(sizeof(TIA_INTERACTABLES) / sizeof(TIA_INTERACTABLES[0])),
    .portas           = TIA_PORTAS,
    .numPortas        = (int)(sizeof(TIA_PORTAS) / sizeof(TIA_PORTAS[0])),
    .drawFunc         = DesenharTreinoIA
};

const TerreoSalaDef *SalaTreinoIADef(void) {
    return &SALA_TREINO_IA_DEF;
}
