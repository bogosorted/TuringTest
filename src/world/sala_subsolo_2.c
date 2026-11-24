#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Subsolo 2 (Laboratorio Tecnologico de Prototipos de Player):
// 6 modelos tecnologicos refinados, totalmente distintos entre si, com fisica secundaria.
// Ao interagir com cada modelo, ele cicla entre: Idle -> Andando -> Correndo -> Computador -> Papel -> Porta.
#define SUB2_MIN_X -7.20f
#define SUB2_MAX_X 7.20f
#define SUB2_MIN_Z -4.40f
#define SUB2_MAX_Z 4.40f
#define SUB2_WALL_H 3.20f

static const float SUB2_MODEL_X[6] = { -5.00f, -3.00f, -1.00f, 1.00f, 3.00f, 5.00f };
static const float SUB2_MODEL_Z    = -0.85f;

static const PlayerColliderBox SUB2_COLLIDERS[] = {
    { -5.45f, -1.30f, -4.55f, -0.40f }, // Tech 0: Cyber-Trench
    { -3.45f, -1.30f, -2.55f, -0.40f }, // Tech 1: Holo-Detetive
    { -1.45f, -1.30f, -0.55f, -0.40f }, // Tech 2: Exo-Agente
    {  0.55f, -1.30f,  1.45f, -0.40f }, // Tech 3: Stealth-Courier
    {  2.55f, -1.30f,  3.45f, -0.40f }, // Tech 4: Bio-Analista
    {  4.55f, -1.30f,  5.45f, -0.40f }, // Tech 5: Chrono-Inspetor
};

static const SalaInteractable SUB2_INTERACTABLES[] = {
    {
        "elevador",
        "Elevador da RASEC",
        { -5.50f, 1.25f, -4.22f },
        { 1.75f, 2.50f, 0.50f },
        2.10f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_0",
        "Estilo 1: Pixar / Arcane (Estilizado 3D)",
        { -5.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_1",
        "Estilo 2: Anime / JRPG (Persona / NieR)",
        { -3.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_2",
        "Estilo 3: Realista Anatômico High-Poly",
        { -1.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_3",
        "Estilo 4: Modern Vinyl / Toy-3D (Zelda LA)",
        {  1.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_4",
        "Estilo 5: Androide Cerâmico (Detroit / Portal)",
        {  3.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo2_modelo_5",
        "Estilo 6: Sculpted Noir Indie (Dishonored / Inside)",
        {  5.00f, 1.00f, -0.85f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    }
};

static const SalaPorta SUB2_PORTAS[] = {
    {
        TERREO_DEST_SUBSOLO,
        "Porta (Subsolo 1)",
        { { -7.30f, 0.0f, 0.95f }, { -6.75f, 2.30f, 2.35f } },
        { -6.85f, 0.0f, 1.65f },
        1.45f,
        { -7.15f, 1.00f, 0.55f, 1.30f }
    }
};

static void DesenharSubsolo2(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = SUB2_MAX_X - SUB2_MIN_X;
    float d = SUB2_MAX_Z - SUB2_MIN_Z;
    float cx = (SUB2_MIN_X + SUB2_MAX_X) * 0.5f;
    float cz = (SUB2_MIN_Z + SUB2_MAX_Z) * 0.5f;

    // Piso escuro tecnologico com grade fina
    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.38f));
    Color corGrid = TerreoLum(0.26f);
    for (float x = -7.0f; x <= 7.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, SUB2_MIN_Z }, (Vector3){ x, 0.004f, SUB2_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.0f) {
        DrawLine3D((Vector3){ SUB2_MIN_X, 0.004f, z }, (Vector3){ SUB2_MAX_X, 0.004f, z }, corGrid);
    }

    // Faixa luminosa no piso guiando pela galeria dos 6 prototipos
    DrawLine3D((Vector3){ -5.8f, 0.008f, 0.45f }, (Vector3){ 5.8f, 0.008f, 0.45f }, TerreoLum(0.76f));
    DrawLine3D((Vector3){ -5.8f, 0.008f, -2.15f }, (Vector3){ 5.8f, 0.008f, -2.15f }, TerreoLum(0.62f));

    // Paredes do laboratorio subterraneo
    TerreoCaixa((Vector3){ cx, SUB2_WALL_H * 0.5f, SUB2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, SUB2_WALL_H, 0.16f }, 0.24f, 0.30f, 0.20f, 0.10f);
    TerreoCaixa((Vector3){ SUB2_MIN_X - 0.08f, SUB2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SUB2_WALL_H, d }, 0.22f, 0.26f, 0.28f, 0.10f);
    TerreoCaixa((Vector3){ SUB2_MAX_X + 0.08f, SUB2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SUB2_WALL_H, d }, 0.22f, 0.26f, 0.28f, 0.10f);

    // Painel tecnico ao fundo + elevador + porta de retorno para o Subsolo 1
    TerreoElevador((Vector3){ -5.50f, 0.0f, SUB2_MIN_Z + 0.02f }, 0.0f);
    TerreoPortaModelo((Vector3){ SUB2_MIN_X + 0.04f, 0.0f, 1.65f }, 90.0f, PORTA_TIPO_INDUSTRIAL);

    // 6 pedestais circulares tecnologicos + 6 modelos tecnologicos refinados
    for (int i = 0; i < 6; i++) {
        float mx = SUB2_MODEL_X[i];
        PlayerPreviewAnim anim = TerreoSubsolo2GetAnim(i);
        float pulso = 0.5f + 0.5f * sinf(tempo * 3.0f + (float)i * 0.9f);

        // Pedestal cilindrico baixo com aro luminoso pulsante
        DrawCylinder((Vector3){ mx, 0.0f, SUB2_MODEL_Z }, 0.56f, 0.62f, 0.06f, 18, TerreoLum(0.30f));
        DrawCylinderWires((Vector3){ mx, 0.064f, SUB2_MODEL_Z }, 0.55f, 0.55f, 0.012f, 18, TerreoLum(0.72f + 0.20f * pulso));

        if (anim == PREVIEW_ANIM_COMPUTER) {
            TerreoCadeira((Vector3){ mx, 0.06f, SUB2_MODEL_Z - 0.08f }, 0.0f);
            TerreoCaixa((Vector3){ mx, 0.42f, SUB2_MODEL_Z + 0.58f }, (Vector3){ 0.90f, 0.72f, 0.42f }, 0.54f, 0.42f, 0.36f, 0.14f);
            TerreoMonitor((Vector3){ mx, 0.78f, SUB2_MODEL_Z + 0.64f }, 180.0f);
        }

        TerreoDesenharPlayerTechVariant(i, anim, (Vector3){ mx, 0.06f, SUB2_MODEL_Z }, 0.0f, tempo);
    }
}

static const TerreoSalaDef SALA_SUBSOLO_2_DEF = {
    .id = TERREO_SALA_SUBSOLO_2,
    .camera = {
        .position   = { 0.0f, 5.45f, 10.60f },
        .target     = { 0.0f, 0.85f, -0.75f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 43.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { SUB2_MIN_X, SUB2_MIN_Z, SUB2_MAX_X - SUB2_MIN_X, SUB2_MAX_Z - SUB2_MIN_Z },
    .bgColor          = { 16, 15, 18, 255 },
    .defaultSpawnPos  = { -6.10f, 0.0f, 1.65f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = SUB2_COLLIDERS,
    .numColliders     = (int)(sizeof(SUB2_COLLIDERS) / sizeof(SUB2_COLLIDERS[0])),
    .interactables    = SUB2_INTERACTABLES,
    .numInteractables = (int)(sizeof(SUB2_INTERACTABLES) / sizeof(SUB2_INTERACTABLES[0])),
    .portas           = SUB2_PORTAS,
    .numPortas        = (int)(sizeof(SUB2_PORTAS) / sizeof(SUB2_PORTAS[0])),
    .drawFunc         = DesenharSubsolo2
};

const TerreoSalaDef *SalaSubsolo2Def(void) {
    return &SALA_SUBSOLO_2_DEF;
}
