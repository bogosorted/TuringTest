#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Copa do 1o Andar (Contabilidade):
// Bancada com pia/cafeteira, geladeira corporativa, bebedouro e mesa redonda com banquetas.
static const float COPA1_MIN_X = -4.20f;
static const float COPA1_MAX_X =  4.20f;
static const float COPA1_MIN_Z = -3.40f;
static const float COPA1_MAX_Z =  3.40f;
static const float COPA1_WALL_H = 2.85f;

static const PlayerColliderBox COPA1_COLLIDERS[] = {
    { -4.05f, -3.35f, -2.95f, -2.25f }, // Geladeira
    { -2.85f, -3.35f,  1.65f, -2.45f }, // Bancada da pia/cafeteira
    {  2.65f, -3.30f,  3.55f, -2.45f }, // Bebedouro
    { -0.45f,  0.15f,  1.25f,  1.85f }, // Mesa redonda
};

static const SalaInteractable COPA1_INTERACTABLES[] = {
    {
        "geladeira_copa_1",
        "Geladeira da Copa",
        { -3.50f, 1.00f, -2.80f },
        { 1.00f, 1.95f, 0.95f },
        1.85f,
        true,
        "Uma geladeira corporativa com potes etiquetados por funcionários da Contabilidade."
    },
    {
        "cafeteira_copa_1",
        "Cafeteira e Bancada",
        { -0.60f, 0.90f, -2.90f },
        { 2.60f, 1.15f, 0.95f },
        1.95f,
        true,
        "O cheiro de café recém-passado preenche a copa. Há xícaras limpas alinhadas ao lado."
    },
    {
        "mesa_copa_1",
        "Mesa da Copa",
        { 0.40f, 0.65f, 1.00f },
        { 1.40f, 0.95f, 1.40f },
        1.75f,
        true,
        "Mesa pequena de descanso usada nos intervalos entre fechamentos contábeis."
    }
};

static const SalaPorta COPA1_PORTAS[] = {
    {
        TERREO_DEST_HALL_1,
        "Porta (Hall 1º Andar)",
        { { 3.95f, 0.0f, -0.25f }, { 4.45f, 2.30f, 1.15f } },
        { 4.00f, 0.0f, 0.45f },
        1.40f,
        { 3.75f, -0.20f, 0.55f, 1.30f }
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

    // Geladeira
    TerreoCaixa((Vector3){ -3.50f, 0.96f, -2.85f }, (Vector3){ 0.86f, 1.92f, 0.76f }, 0.84f, 0.76f, 0.66f, 0.18f);
    TerreoCaixa((Vector3){ -3.18f, 1.25f, -2.44f }, (Vector3){ 0.04f, 0.42f, 0.04f }, 0.24f, 0.20f, 0.20f, -1.0f);

    // Bancada da copa + cafeteira + micro-ondas
    TerreoCaixa((Vector3){ -0.60f, 0.44f, -2.90f }, (Vector3){ 4.30f, 0.88f, 0.76f }, 0.68f, 0.54f, 0.46f, 0.14f);
    TerreoCaixa((Vector3){ -1.45f, 1.06f, -2.90f }, (Vector3){ 0.56f, 0.36f, 0.42f }, 0.26f, 0.20f, 0.18f, 0.10f);
    TerreoCaixa((Vector3){  0.35f, 1.06f, -2.90f }, (Vector3){ 0.36f, 0.38f, 0.34f }, 0.22f, 0.16f, 0.14f, 0.10f);

    // Bebedouro
    TerreoBebedouro((Vector3){ 3.10f, 0.0f, -2.90f });

    // Mesa redonda + 3 banquetas
    TerreoMesaRedonda((Vector3){ 0.40f, 0.0f, 1.00f }, 0.62f, 0.76f);
    TerreoBanqueta((Vector3){ -0.45f, 0.0f, 1.00f }, 0.56f);
    TerreoBanqueta((Vector3){  1.25f, 0.0f, 1.00f }, 0.56f);
    TerreoBanqueta((Vector3){  0.40f, 0.0f, 0.15f }, 0.56f);
}

static const TerreoSalaDef SALA_COPA_1_DEF = {
    .id = TERREO_SALA_COPA_1,
    .camera = {
        .position   = { 0.0f, 5.25f, 7.80f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { COPA1_MIN_X, COPA1_MIN_Z, COPA1_MAX_X - COPA1_MIN_X, COPA1_MAX_Z - COPA1_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 3.55f, 0.0f, 0.45f },
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
