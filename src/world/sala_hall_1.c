#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Hall 1 + Elevador (PrimeiroAndarRASEC3D.png):
// - Esquerda: porta para Copa
// - Direita (frente): porta para Contabilidade 1
// - Fundo-direito: Elevador conectando os andares (-1 bloqueado, 0, 1, 2, 3)
static const float H1_MIN_X = -5.40f;
static const float H1_MAX_X =  5.40f;
static const float H1_MIN_Z = -3.40f;
static const float H1_MAX_Z =  3.20f;
static const float H1_WALL_H = 3.05f;

static const PlayerColliderBox H1_COLLIDERS[] = {
    { -1.95f,  0.65f,  1.05f,  1.75f }, // Sofa central compacto
    { -5.15f,  2.15f, -4.35f,  2.95f }, // Vaso canto esquerdo
    { -5.10f, -3.40f, -2.60f, -1.15f }, // Balcao redondo da recepcionista (bloqueia acesso por tras)
};

static const SalaInteractable H1_INTERACTABLES[] = {
    {
        "recepcionista_hall_1",
        "Recepcionista (1º Andar)",
        { -3.85f, 0.95f, -1.75f },
        { 1.45f, 1.35f, 1.05f },
        1.95f,
        false,
        "Recepcionista: \"Bem-vindo ao 1º Andar — Setor de Contabilidade. À esquerda fica a Copa dos funcionários; à direita ficam as salas de Contabilidade 1 e 2 e o Arquivo.\""
    },
    {
        "diretorio_andar_1",
        "Diretório do 1º Andar",
        { -3.85f, 1.60f, -3.30f },
        { 1.25f, 0.90f, 0.35f },
        1.85f,
        false,
        "1º ANDAR - CONTABILIDADE: Copa à esquerda, Contabilidade 1 à direita (com acesso ao Arquivo e Contabilidade 2)."
    },
    {
        "sofa_hall_1",
        "Sofá do Hall",
        { -0.45f, 0.55f, 1.20f },
        { 2.80f, 0.90f, 1.05f },
        1.95f,
        false,
        "Hall do 1º Andar. Use o elevador ao fundo para mudar de andar."
    },
    {
        "elevador",
        "Elevador",
        { 3.55f, 1.15f, -3.15f },
        { 2.15f, 2.30f, 0.65f },
        1.85f,
        true,
        NULL
    }
};

static const SalaPorta H1_PORTAS[] = {
    // Esquerda -> Copa
    {
        TERREO_DEST_COPA_1,
        "Porta (Copa)",
        { { -5.65f, 0.0f, -0.25f }, { -5.15f, 2.30f, 1.15f } },
        { -5.20f, 0.0f, 0.45f },
        1.40f,
        { -5.50f, -0.20f, 0.55f, 1.30f }
    },
    // Direita (frente) -> Contabilidade 1 (PrimeiroAndarRASEC3D.png)
    {
        TERREO_DEST_CONTABILIDADE_1,
        "Porta (Contabilidade 1)",
        { { 5.15f, 0.0f, 0.40f }, { 5.65f, 2.30f, 1.80f } },
        { 5.20f, 0.0f, 1.10f },
        1.40f,
        { 4.90f, 0.45f, 0.55f, 1.30f }
    }
};

static void DesenharHall1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = H1_MAX_X - H1_MIN_X;
    float d = H1_MAX_Z - H1_MIN_Z;
    float cx = (H1_MIN_X + H1_MAX_X) * 0.5f;
    float cz = (H1_MIN_Z + H1_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.58f));
    Color corGrid = TerreoLum(0.45f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, H1_MIN_Z }, (Vector3){ x, 0.004f, H1_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ H1_MIN_X, 0.004f, z }, (Vector3){ H1_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes
    TerreoCaixa((Vector3){ cx, H1_WALL_H * 0.5f, H1_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, H1_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ H1_MIN_X - 0.08f, H1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ H1_MAX_X + 0.08f, H1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, H1_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Portas: Copa (esq) e Contabilidade 1 (dir-frente)
    TerreoPortaModelo((Vector3){ H1_MIN_X + 0.04f, 0.0f, 0.45f },  90.0f, PORTA_TIPO_VIDRO);
    TerreoPortaModelo((Vector3){ H1_MAX_X - 0.04f, 0.0f, 1.10f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // Diretorio do andar
    TerreoCaixa((Vector3){ -3.85f, 1.60f, H1_MIN_Z + 0.03f },
                (Vector3){ 1.15f, 0.80f, 0.04f }, 0.82f, 0.78f, 0.70f, 0.16f);

    // Tapete + Sofa curvo + Plantas
    TerreoCaixa((Vector3){ -0.45f, 0.01f, 1.20f }, (Vector3){ 3.40f, 0.02f, 1.65f }, 0.22f, 0.18f, 0.18f, -1.0f);
    TerreoSofaCurvo((Vector3){ -0.45f, 0.0f, 1.20f }, 180.0f, 2.50f);
    TerreoPlanta((Vector3){ -4.75f, 0.0f, 2.55f }, 1.30f);
    TerreoPlanta((Vector3){  4.75f, 0.0f, -2.65f }, 1.25f);

    // Balcao redondo em U da recepcionista do 1o andar
    TerreoBalcaoRecepcaoU((Vector3){ -3.85f, 0.0f, -2.05f });

    // Elevador no fundo-direito
    TerreoElevador((Vector3){ 3.55f, 0.0f, H1_MIN_Z + 0.04f }, 0.0f);
}

static const TerreoSalaDef SALA_HALL_1_DEF = {
    .id = TERREO_SALA_HALL_1,
    .camera = {
        .position   = { 0.0f, 5.75f, 8.60f },
        .target     = { 0.0f, 0.70f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { H1_MIN_X, H1_MIN_Z, H1_MAX_X - H1_MIN_X, H1_MAX_Z - H1_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 3.55f, 0.0f, -2.15f },
    .defaultSpawnYaw  = 0.0f,
    .colliders        = H1_COLLIDERS,
    .numColliders     = (int)(sizeof(H1_COLLIDERS) / sizeof(H1_COLLIDERS[0])),
    .interactables    = H1_INTERACTABLES,
    .numInteractables = (int)(sizeof(H1_INTERACTABLES) / sizeof(H1_INTERACTABLES[0])),
    .portas           = H1_PORTAS,
    .numPortas        = (int)(sizeof(H1_PORTAS) / sizeof(H1_PORTAS[0])),
    .drawFunc         = DesenharHall1
};

const TerreoSalaDef *SalaHall1Def(void) {
    return &SALA_HALL_1_DEF;
}
