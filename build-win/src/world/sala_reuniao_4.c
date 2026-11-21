#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Salas de Reuniao 1 e 2 (4o Andar - Brainstorm):
// - Mesa grande redonda central com 6 cadeiras ao redor
// - Quadro branco de brainstorm na parede
// - Sala de Reuniao 1: Esquerda -> Hall 4 | Fundo -> Reuniao 2 | Direita -> Sala do CEO (Trancada)
// - Sala de Reuniao 2: Frente (embaixo) -> Reuniao 1
static const float RN_MIN_X = -4.80f;
static const float RN_MAX_X =  4.80f;
static const float RN_MIN_Z = -3.60f;
static const float RN_MAX_Z =  3.40f;
static const float RN_WALL_H = 3.15f;

static const PlayerColliderBox RN_COLLIDERS[] = {
    { -1.75f, -1.75f,  1.75f,  1.65f }, // Mesa grande redonda + cadeiras ao redor
};

static void DesenharEstruturaReuniao(bool ehSala1) {
    float w = RN_MAX_X - RN_MIN_X;
    float d = RN_MAX_Z - RN_MIN_Z;
    float cx = (RN_MIN_X + RN_MAX_X) * 0.5f;
    float cz = (RN_MIN_Z + RN_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.44f);
    for (float x = -4.0f; x <= 4.01f; x += 0.8f) {
        DrawLine3D((Vector3){ x, 0.004f, RN_MIN_Z }, (Vector3){ x, 0.004f, RN_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 0.8f) {
        DrawLine3D((Vector3){ RN_MIN_X, 0.004f, z }, (Vector3){ RN_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, RN_WALL_H * 0.5f, RN_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, RN_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ RN_MIN_X - 0.08f, RN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, RN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ RN_MAX_X + 0.08f, RN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, RN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    if (ehSala1) {
        TerreoPortaModelo((Vector3){ RN_MIN_X + 0.04f, 0.0f, 0.00f },  90.0f, PORTA_TIPO_VIDRO);
        TerreoPortaModelo((Vector3){ 0.0f, 0.0f, RN_MIN_Z + 0.04f },   0.0f, PORTA_TIPO_VIDRO);
        TerreoPortaModelo((Vector3){ RN_MAX_X - 0.04f, 0.0f, 0.00f }, -90.0f, PORTA_TIPO_INDUSTRIAL);
        // Quadro branco ao lado esquerdo da porta do fundo
        TerreoCaixa((Vector3){ -2.55f, 1.70f, RN_MIN_Z + 0.03f }, (Vector3){ 2.10f, 1.05f, 0.04f }, 0.90f, 0.86f, 0.78f, 0.16f);
    } else {
        TerreoPortaModelo((Vector3){ 0.0f, 0.0f, RN_MAX_Z - 0.04f }, 180.0f, PORTA_TIPO_VIDRO);
        // Quadro branco centralizado ao fundo
        TerreoCaixa((Vector3){ 0.0f, 1.70f, RN_MIN_Z + 0.03f }, (Vector3){ 2.80f, 1.15f, 0.04f }, 0.90f, 0.86f, 0.78f, 0.16f);
    }

    // Tapete circular + Mesa grande redonda de brainstorm + 6 cadeiras ao redor
    DrawCylinder((Vector3){ 0.0f, 0.006f, -0.05f }, 2.15f, 2.15f, 0.01f, 28, TerreoLum(0.24f));
    TerreoMesaRedonda((Vector3){ 0.0f, 0.0f, -0.05f }, 1.25f, 0.76f);
    for (int i = 0; i < 6; i++) {
        float angDeg = (float)i * 60.0f;
        float angRad = angDeg * DEG2RAD;
        float cxCad = sinf(angRad) * 1.62f;
        float czCad = -0.05f + cosf(angRad) * 1.62f;
        TerreoCadeira((Vector3){ cxCad, 0.0f, czCad }, angDeg + 180.0f);
    }
    TerreoPlanta((Vector3){ -4.15f, 0.0f, -2.95f }, 1.15f);
}

// ----------------- SALA DE REUNIAO 1 -----------------
static const SalaInteractable RN1_INTERACTABLES[] = {
    {
        "mesa_reuniao_1",
        "Mesa de Brainstorm (Sala 1)",
        { 0.0f, 0.75f, -0.05f },
        { 2.60f, 1.00f, 2.60f },
        2.35f,
        true,
        "Uma grande mesa redonda cercada por cadeiras para reuniões de brainstorm da diretoria."
    },
    {
        "quadro_reuniao_1",
        "Quadro de Ideias",
        { -2.55f, 1.70f, -3.50f },
        { 2.15f, 1.10f, 0.35f },
        1.95f,
        false,
        "Esquemas apagados às pressas ainda deixam marcas fracas de fluxogramas no quadro branco."
    },
    {
        "porta_ceo_trancada",
        "Porta (Sala do CEO - Trancada)",
        { 4.55f, 1.15f, 0.00f },
        { 0.65f, 2.30f, 1.40f },
        1.75f,
        true,
        "A Sala do CEO está trancada por biometria. Acesso restrito apenas à presidência."
    }
};

static const SalaPorta RN1_PORTAS[] = {
    {
        TERREO_DEST_HALL_4,
        "Porta (Recepção 4º Andar)",
        { { -5.05f, 0.0f, -0.70f }, { -4.55f, 2.30f, 0.70f } },
        { -4.60f, 0.0f, 0.00f },
        1.40f,
        { -4.90f, -0.65f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_REUNIAO_2,
        "Porta (Sala de Reunião 2)",
        { { -0.70f, 0.0f, -3.75f }, { 0.70f, 2.30f, -3.30f } },
        { 0.00f, 0.0f, -3.35f },
        1.40f,
        { -0.65f, -3.68f, 1.30f, 0.58f }
    }
};

static void DesenharReuniao1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    DesenharEstruturaReuniao(true);
}

static const TerreoSalaDef SALA_REUNIAO_1_DEF = {
    .id = TERREO_SALA_REUNIAO_1,
    .camera = {
        .position   = { 0.0f, 5.65f, 8.20f },
        .target     = { 0.0f, 0.75f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { RN_MIN_X, RN_MIN_Z, RN_MAX_X - RN_MIN_X, RN_MAX_Z - RN_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { -3.85f, 0.0f, 0.0f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = RN_COLLIDERS,
    .numColliders     = (int)(sizeof(RN_COLLIDERS) / sizeof(RN_COLLIDERS[0])),
    .interactables    = RN1_INTERACTABLES,
    .numInteractables = (int)(sizeof(RN1_INTERACTABLES) / sizeof(RN1_INTERACTABLES[0])),
    .portas           = RN1_PORTAS,
    .numPortas        = (int)(sizeof(RN1_PORTAS) / sizeof(RN1_PORTAS[0])),
    .drawFunc         = DesenharReuniao1
};

const TerreoSalaDef *SalaReuniao1Def(void) {
    return &SALA_REUNIAO_1_DEF;
}

// ----------------- SALA DE REUNIAO 2 -----------------
static const SalaInteractable RN2_INTERACTABLES[] = {
    {
        "mesa_reuniao_2",
        "Mesa de Brainstorm (Sala 2)",
        { 0.0f, 0.75f, -0.05f },
        { 2.60f, 1.00f, 2.60f },
        2.35f,
        true,
        "Outra sala de brainstorm idêntica, com uma grande mesa redonda e cadeiras ao redor."
    },
    {
        "quadro_reuniao_2",
        "Quadro Branco",
        { 0.0f, 1.70f, -3.50f },
        { 2.85f, 1.20f, 0.35f },
        2.00f,
        false,
        "O quadro branco está completamente limpo."
    }
};

static const SalaPorta RN2_PORTAS[] = {
    {
        TERREO_DEST_REUNIAO_1,
        "Porta (Sala de Reunião 1)",
        { { -0.70f, 0.0f, 3.15f }, { 0.70f, 2.30f, 3.55f } },
        { 0.00f, 0.0f, 3.15f },
        1.40f,
        { -0.65f, 2.90f, 1.30f, 0.58f }
    }
};

static void DesenharReuniao2(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    DesenharEstruturaReuniao(false);
}

static const TerreoSalaDef SALA_REUNIAO_2_DEF = {
    .id = TERREO_SALA_REUNIAO_2,
    .camera = {
        .position   = { 0.0f, 5.65f, 8.20f },
        .target     = { 0.0f, 0.75f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { RN_MIN_X, RN_MIN_Z, RN_MAX_X - RN_MIN_X, RN_MAX_Z - RN_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 0.0f, 0.0f, 2.65f },
    .defaultSpawnYaw  = PI,
    .colliders        = RN_COLLIDERS,
    .numColliders     = (int)(sizeof(RN_COLLIDERS) / sizeof(RN_COLLIDERS[0])),
    .interactables    = RN2_INTERACTABLES,
    .numInteractables = (int)(sizeof(RN2_INTERACTABLES) / sizeof(RN2_INTERACTABLES[0])),
    .portas           = RN2_PORTAS,
    .numPortas        = (int)(sizeof(RN2_PORTAS) / sizeof(RN2_PORTAS[0])),
    .drawFunc         = DesenharReuniao2
};

const TerreoSalaDef *SalaReuniao2Def(void) {
    return &SALA_REUNIAO_2_DEF;
}
