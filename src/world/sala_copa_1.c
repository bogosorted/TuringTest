#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Copa do 1o Andar (Contabilidade):
// Bancada com pia/cafeteira, geladeira corporativa, bebedouro, 2 mesas redondas e porta decorativa de banheiro.
static const float COPA1_MIN_X = -4.80f;
static const float COPA1_MAX_X =  4.80f;
static const float COPA1_MIN_Z = -3.60f;
static const float COPA1_MAX_Z =  3.60f;
static const float COPA1_WALL_H = 2.95f;

static const PlayerColliderBox COPA1_COLLIDERS[] = {
    { -4.55f, -3.50f, -3.45f, -2.35f }, // Geladeira
    { -3.25f, -3.50f,  1.65f, -2.50f }, // Bancada da pia/cafeteira
    {  2.75f, -3.45f,  3.75f, -2.50f }, // Bebedouro
    { -2.95f,  0.30f, -1.25f,  2.00f }, // Mesa redonda 1 (esquerda)
    {  0.50f,  0.30f,  2.20f,  2.00f }, // Mesa redonda 2 (direita)
};

static const SalaInteractable COPA1_INTERACTABLES[] = {
    {
        "geladeira_copa_1",
        "Geladeira da Copa",
        { -4.00f, 1.00f, -2.95f },
        { 1.00f, 1.95f, 0.95f },
        1.85f,
        true,
        "Uma geladeira corporativa com potes etiquetados por funcionários da Contabilidade."
    },
    {
        "cafeteira_copa_1",
        "Cafeteira e Bancada",
        { -0.80f, 0.90f, -3.00f },
        { 2.60f, 1.15f, 0.95f },
        1.95f,
        true,
        "O cheiro de café recém-passado preenche a copa. Há xícaras limpas alinhadas ao lado."
    },
    {
        "mesa_copa_1",
        "Mesa da Copa",
        { -2.10f, 0.65f, 1.15f },
        { 1.40f, 0.95f, 1.40f },
        1.75f,
        true,
        "Mesa de descanso usada nos intervalos entre fechamentos contábeis."
    },
    {
        "mesa_copa_2",
        "Mesa da Copa",
        { 1.35f, 0.65f, 1.15f },
        { 1.40f, 0.95f, 1.40f },
        1.75f,
        true,
        "Segunda mesa de refeições da copa do 1º andar."
    },
    {
        "banheiro_copa_1",
        "Banheiro de Funcionários",
        { -4.70f, 1.10f, -0.35f },
        { 0.45f, 2.20f, 1.35f },
        1.65f,
        true,
        "Banheiro de Funcionários — porta fechada para manutenção."
    }
};

static const SalaPorta COPA1_PORTAS[] = {
    {
        TERREO_DEST_HALL_1,
        "Porta (Hall 1º Andar)",
        { { 4.45f, 0.0f, -0.25f }, { 5.00f, 2.30f, 1.15f } },
        { 4.55f, 0.0f, 0.45f },
        1.55f,
        { 4.30f, -0.20f, 0.60f, 1.30f }
    }
};

static void DesenharCopa1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = COPA1_MAX_X - COPA1_MIN_X;
    float d = COPA1_MAX_Z - COPA1_MIN_Z;
    float cx = (COPA1_MIN_X + COPA1_MAX_X) * 0.5f;
    float cz = (COPA1_MIN_Z + COPA1_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.60f));
    Color corGrid = TerreoLum(0.46f);
    for (float x = -4.0f; x <= 4.01f; x += 0.8f) {
        DrawLine3D((Vector3){ x, 0.004f, COPA1_MIN_Z }, (Vector3){ x, 0.004f, COPA1_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 0.8f) {
        DrawLine3D((Vector3){ COPA1_MIN_X, 0.004f, z }, (Vector3){ COPA1_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, COPA1_WALL_H * 0.5f, COPA1_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, COPA1_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ COPA1_MIN_X - 0.08f, COPA1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, COPA1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ COPA1_MAX_X + 0.08f, COPA1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, COPA1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    TerreoPortaModelo((Vector3){ COPA1_MAX_X - 0.04f, 0.0f, 0.45f }, -90.0f, PORTA_TIPO_VIDRO);
    // Porta decorativa do Banheiro de Funcionarios (sem entrada) + placa indicativa
    TerreoPortaModelo((Vector3){ COPA1_MIN_X + 0.04f, 0.0f, -0.35f },  90.0f, PORTA_TIPO_ESCRITORIO);
    TerreoCaixa((Vector3){ COPA1_MIN_X + 0.05f, 1.55f, -1.25f }, (Vector3){ 0.03f, 0.24f, 0.34f }, 0.88f, 0.80f, 0.72f, 0.14f);

    // Geladeira
    TerreoCaixa((Vector3){ -4.00f, 0.96f, -2.95f }, (Vector3){ 0.88f, 1.92f, 0.78f }, 0.84f, 0.76f, 0.66f, 0.18f);
    TerreoCaixa((Vector3){ -3.66f, 1.25f, -2.52f }, (Vector3){ 0.04f, 0.42f, 0.04f }, 0.24f, 0.20f, 0.20f, -1.0f);

    // Bancada da copa + cafeteira + micro-ondas
    TerreoCaixa((Vector3){ -0.80f, 0.44f, -3.00f }, (Vector3){ 4.50f, 0.88f, 0.78f }, 0.68f, 0.54f, 0.46f, 0.14f);
    TerreoCaixa((Vector3){ -1.65f, 1.06f, -3.00f }, (Vector3){ 0.56f, 0.36f, 0.42f }, 0.26f, 0.20f, 0.18f, 0.10f);
    TerreoCaixa((Vector3){  0.15f, 1.06f, -3.00f }, (Vector3){ 0.36f, 0.38f, 0.34f }, 0.22f, 0.16f, 0.14f, 0.10f);

    // Bebedouro
    TerreoBebedouro((Vector3){ 3.25f, 0.0f, -3.00f });

    // Mesa redonda 1 (esquerda) + 3 cadeiras confortaveis acolchoadas
    TerreoMesaRedonda((Vector3){ -2.10f, 0.0f, 1.15f }, 0.62f, 0.76f);
    TerreoCadeiraConfortavel((Vector3){ -2.95f, 0.0f, 1.15f },  90.0f);
    TerreoCadeiraConfortavel((Vector3){ -1.25f, 0.0f, 1.15f }, -90.0f);
    TerreoCadeiraConfortavel((Vector3){ -2.10f, 0.0f, 0.30f },   0.0f);

    // Mesa redonda 2 (direita) + 3 cadeiras confortaveis acolchoadas
    TerreoMesaRedonda((Vector3){ 1.35f, 0.0f, 1.15f }, 0.62f, 0.76f);
    TerreoCadeiraConfortavel((Vector3){ 0.50f, 0.0f, 1.15f },  90.0f);
    TerreoCadeiraConfortavel((Vector3){ 2.20f, 0.0f, 1.15f }, -90.0f);
    TerreoCadeiraConfortavel((Vector3){ 1.35f, 0.0f, 0.30f },   0.0f);
}

static const TerreoSalaDef SALA_COPA_1_DEF = {
    .id = TERREO_SALA_COPA_1,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.10f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { COPA1_MIN_X, COPA1_MIN_Z, COPA1_MAX_X - COPA1_MIN_X, COPA1_MAX_Z - COPA1_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 3.95f, 0.0f, 0.45f },
    .defaultSpawnYaw  = -PI * 0.5f,
    .colliders        = COPA1_COLLIDERS,
    .numColliders     = (int)(sizeof(COPA1_COLLIDERS) / sizeof(COPA1_COLLIDERS[0])),
    .interactables    = COPA1_INTERACTABLES,
    .numInteractables = (int)(sizeof(COPA1_INTERACTABLES) / sizeof(COPA1_INTERACTABLES[0])),
    .portas           = COPA1_PORTAS,
    .numPortas        = (int)(sizeof(COPA1_PORTAS) / sizeof(COPA1_PORTAS[0])),
    .drawFunc         = DesenharCopa1
};

const TerreoSalaDef *SalaCopa1Def(void) {
    return &SALA_COPA_1_DEF;
}
