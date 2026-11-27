#include "terreo_3d.h"
#include "rlgl.h"
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Subsolo 3D (area de testes/showcase de modelos do player):
// 4 modelos aprimorados (2 Masculinos + 2 Femininos). Ao interagir com cada um,
// ele cicla entre: Idle -> Andando -> Correndo -> Computador -> Papel/Mapa -> Porta.
#define SUB_MIN_X -6.00f
#define SUB_MAX_X 6.00f
#define SUB_MIN_Z -4.20f
#define SUB_MAX_Z 4.20f
#define SUB_WALL_H 3.10f

static const float SUB_MODEL_X[4] = { -3.60f, -1.20f, 1.20f, 3.60f };
#define SUB_MODEL_Z -0.60f

static const PlayerColliderBox SUB_COLLIDERS[] = {
    { -4.05f, -1.05f, -3.15f, -0.15f }, // Modelo 0 (Masculino Noir)
    { -1.65f, -1.05f, -0.75f, -0.15f }, // Modelo 1 (Masculino Corporativo)
    {  0.75f, -1.05f,  1.65f, -0.15f }, // Modelo 2 (Feminino Noir)
    {  3.15f, -1.05f,  4.05f, -0.15f }, // Modelo 3 (Feminino Executivo)
};

static const SalaInteractable SUB_INTERACTABLES[] = {
    {
        "elevador",
        "Elevador da RASEC",
        { -4.20f, 1.25f, -4.02f },
        { 1.75f, 2.50f, 0.50f },
        2.10f,
        false,
        NULL
    },
    {
        "subsolo_modelo_0",
        "Masc. A (Sobretudo + Óculos)",
        { -3.60f, 1.00f, -0.60f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo_modelo_1",
        "Masc. B (Blazer + Colete)",
        { -1.20f, 1.00f, -0.60f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo_modelo_2",
        "Fem. A (Sobretudo + Echarpe)",
        {  1.20f, 1.00f, -0.60f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    },
    {
        "subsolo_modelo_3",
        "Fem. B (Blazer + Coque + Óculos)",
        {  3.60f, 1.00f, -0.60f },
        { 1.00f, 2.00f, 1.00f },
        1.85f,
        false,
        NULL
    }
};

static const SalaPorta SUB_PORTAS[] = {
    {
        TERREO_DEST_SUBSOLO_2,
        "Porta (Subsolo 2 - Modelos Tech)",
        { { 5.55f, 0.0f, 0.95f }, { 6.10f, 2.30f, 2.35f } },
        { 5.65f, 0.0f, 1.65f },
        1.45f,
        { 5.40f, 1.00f, 0.55f, 1.30f }
    }
};

static void DesenharSubsolo(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = SUB_MAX_X - SUB_MIN_X;
    float d = SUB_MAX_Z - SUB_MIN_Z;
    float cx = (SUB_MIN_X + SUB_MAX_X) * 0.5f;
    float cz = (SUB_MIN_Z + SUB_MAX_Z) * 0.5f;

    // Piso e grade minimalista
    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.46f));
    Color corGrid = TerreoLum(0.34f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, SUB_MIN_Z }, (Vector3){ x, 0.004f, SUB_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.0f) {
        DrawLine3D((Vector3){ SUB_MIN_X, 0.004f, z }, (Vector3){ SUB_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes simples (retangulos vazios)
    TerreoCaixa((Vector3){ cx, SUB_WALL_H * 0.5f, SUB_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, SUB_WALL_H, 0.16f }, 0.28f, 0.34f, 0.24f, 0.10f);
    TerreoCaixa((Vector3){ SUB_MIN_X - 0.08f, SUB_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SUB_WALL_H, d }, 0.26f, 0.30f, 0.32f, 0.10f);
    TerreoCaixa((Vector3){ SUB_MAX_X + 0.08f, SUB_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SUB_WALL_H, d }, 0.26f, 0.30f, 0.32f, 0.10f);

    // Elevador na parede do fundo para subir de volta + porta direita para o Subsolo 2
    TerreoElevador((Vector3){ -4.20f, 0.0f, SUB_MIN_Z + 0.02f }, 0.0f);
    TerreoPortaModelo((Vector3){ SUB_MAX_X - 0.04f, 0.0f, 1.65f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    // 4 bases + 4 variantes aprimoradas do player com animacoes interativas
    for (int i = 0; i < 4; i++) {
        float mx = SUB_MODEL_X[i];
        PlayerPreviewAnim anim = TerreoSubsoloGetAnim(i);
        TerreoCaixa((Vector3){ mx, 0.03f, SUB_MODEL_Z }, (Vector3){ 1.05f, 0.06f, 1.05f }, 0.60f, 0.46f, 0.40f, 0.16f);

        if (anim == PREVIEW_ANIM_COMPUTER) {
            // Cadeira e bancada com monitor para visualizar a animacao de computador
            TerreoCadeira((Vector3){ mx, 0.06f, SUB_MODEL_Z - 0.08f }, 0.0f);
            TerreoCaixa((Vector3){ mx, 0.42f, SUB_MODEL_Z + 0.58f }, (Vector3){ 0.90f, 0.72f, 0.42f }, 0.58f, 0.44f, 0.38f, 0.14f);
            TerreoMonitor((Vector3){ mx, 0.78f, SUB_MODEL_Z + 0.64f }, 180.0f);
        }

        TerreoDesenharPlayerVariant(i, anim, (Vector3){ mx, 0.06f, SUB_MODEL_Z }, 0.0f, tempo);
    }
}

static const TerreoSalaDef SALA_SUBSOLO_DEF = {
    .id = TERREO_SALA_SUBSOLO,
    .camera = {
        .position   = { 0.0f, 5.10f, 8.80f },
        .target     = { 0.0f, 0.85f, -0.50f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 42.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { SUB_MIN_X, SUB_MIN_Z, SUB_MAX_X - SUB_MIN_X, SUB_MAX_Z - SUB_MIN_Z },
    .bgColor          = { 20, 18, 20, 255 },
    .defaultSpawnPos  = { -4.20f, 0.0f, -2.55f },
    .defaultSpawnYaw  = 0.35f,
    .colliders        = SUB_COLLIDERS,
    .numColliders     = (int)(sizeof(SUB_COLLIDERS) / sizeof(SUB_COLLIDERS[0])),
    .interactables    = SUB_INTERACTABLES,
    .numInteractables = (int)(sizeof(SUB_INTERACTABLES) / sizeof(SUB_INTERACTABLES[0])),
    .portas           = SUB_PORTAS,
    .numPortas        = (int)(sizeof(SUB_PORTAS) / sizeof(SUB_PORTAS[0])),
    .drawFunc         = DesenharSubsolo
};

const TerreoSalaDef *SalaSubsoloDef(void) {
    return &SALA_SUBSOLO_DEF;
}
