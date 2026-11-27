#include "terreo_3d.h"
#include "rlgl.h"
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Recepcao do 4o Andar (Ultimo Andar - Executivo):
// - Elevador no fundo-direito
// - Esquerda: Auditorio
// - Direita: Porta da Sala do CEO (TRANCADA - apenas aviso, sem interior modelado!)
#define H4_MIN_X -5.40f
#define H4_MAX_X 5.40f
#define H4_MIN_Z -3.40f
#define H4_MAX_Z 3.20f
#define H4_WALL_H 3.15f

static const PlayerColliderBox H4_COLLIDERS[] = {
    { -3.85f, -3.40f, -1.25f, -1.65f }, // Balcao executivo em U (bloqueia acesso por tras)
    { -1.85f,  0.65f,  1.15f,  1.75f }, // Sofa de espera executivo
    { -5.15f,  2.15f, -4.35f,  2.95f }, // Vaso canto esquerdo
};

static const SalaInteractable H4_INTERACTABLES[] = {
    {
        "balcao_executivo_4",
        "Recepcionista (4º Andar)",
        { -2.55f, 0.90f, -2.65f },
        { 2.25f, 1.30f, 1.10f },
        2.00f,
        false,
        "Recepcionista: \"Bem-vindo ao 4º Andar — Executivo e Presidência. À esquerda fica o Auditório; à direita ficam as Salas de Reunião da diretoria.\""
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

static const SalaPorta H4_PORTAS[] = {
    {
        TERREO_DEST_AUDITORIO,
        "Porta (Auditório)",
        { { -5.65f, 0.0f, -0.25f }, { -5.15f, 2.30f, 1.15f } },
        { -5.20f, 0.0f, 0.45f },
        1.40f,
        { -5.50f, -0.20f, 0.55f, 1.30f }
    },
    {
        TERREO_DEST_REUNIAO_1,
        "Porta (Sala de Reunião 1)",
        { { 5.15f, 0.0f, 0.40f }, { 5.65f, 2.30f, 1.80f } },
        { 5.20f, 0.0f, 1.10f },
        1.40f,
        { 4.90f, 0.45f, 0.55f, 1.30f }
    }
};

static void DesenharHall4(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = H4_MAX_X - H4_MIN_X;
    float d = H4_MAX_Z - H4_MIN_Z;
    float cx = (H4_MIN_X + H4_MAX_X) * 0.5f;
    float cz = (H4_MIN_Z + H4_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.60f));
    Color corGrid = TerreoLum(0.48f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, H4_MIN_Z }, (Vector3){ x, 0.004f, H4_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ H4_MIN_X, 0.004f, z }, (Vector3){ H4_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, H4_WALL_H * 0.5f, H4_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, H4_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ H4_MIN_X - 0.08f, H4_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H4_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ H4_MAX_X + 0.08f, H4_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H4_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Esquerda -> Auditorio | Direita -> Sala de Reuniao 1
    TerreoPortaModelo((Vector3){ H4_MIN_X + 0.04f, 0.0f, 0.45f },  90.0f, PORTA_TIPO_VIDRO);
    TerreoPortaModelo((Vector3){ H4_MAX_X - 0.04f, 0.0f, 1.10f }, -90.0f, PORTA_TIPO_VIDRO);

    // Balcao executivo + painel
    TerreoCaixa((Vector3){ -2.55f, 0.52f, -2.65f }, (Vector3){ 2.30f, 1.04f, 0.90f }, 0.72f, 0.54f, 0.46f, 0.14f);
    TerreoMonitor((Vector3){ -2.55f, 1.06f, -2.65f }, 0.0f);

    // Sofa curvo executivo + plantas
    TerreoCaixa((Vector3){ -0.35f, 0.01f, 1.20f }, (Vector3){ 3.40f, 0.02f, 1.65f }, 0.22f, 0.18f, 0.18f, -1.0f);
    TerreoSofaCurvo((Vector3){ -0.35f, 0.0f, 1.20f }, 180.0f, 2.50f);
    TerreoPlanta((Vector3){ -4.75f, 0.0f, 2.55f }, 1.30f);
    TerreoPlanta((Vector3){  4.55f, 0.0f, -2.55f }, 1.25f);

    TerreoElevador((Vector3){ 3.45f, 0.0f, H4_MIN_Z + 0.04f }, 0.0f);
}

static const TerreoSalaDef SALA_HALL_4_DEF = {
    .id = TERREO_SALA_HALL_4,
    .camera = {
        .position   = { 0.0f, 5.75f, 8.60f },
        .target     = { 0.0f, 0.70f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { H4_MIN_X, H4_MIN_Z, H4_MAX_X - H4_MIN_X, H4_MAX_Z - H4_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 3.45f, 0.0f, -2.15f },
    .defaultSpawnYaw  = 0.0f,
    .colliders        = H4_COLLIDERS,
    .numColliders     = (int)(sizeof(H4_COLLIDERS) / sizeof(H4_COLLIDERS[0])),
    .interactables    = H4_INTERACTABLES,
    .numInteractables = (int)(sizeof(H4_INTERACTABLES) / sizeof(H4_INTERACTABLES[0])),
    .portas           = H4_PORTAS,
    .numPortas        = (int)(sizeof(H4_PORTAS) / sizeof(H4_PORTAS[0])),
    .drawFunc         = DesenharHall4
};

const TerreoSalaDef *SalaHall4Def(void) {
    return &SALA_HALL_4_DEF;
}
