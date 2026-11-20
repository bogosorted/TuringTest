#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const float LAN_MIN_X = -4.70f;
static const float LAN_MAX_X =  4.70f;
static const float LAN_MIN_Z = -3.70f;
static const float LAN_MAX_Z =  3.70f;
static const float LAN_WALL_H = 2.85f;

// Lanchonete estilo loja de shopping:
// - Na parede esquerda, verticalmente: divisao entre o player e a bancada com vendedores atras
// - Mesas redondas organizadas na area centro-direita
// - Porta de vidro na direita-frente (Recepcao) e porta de vidro ao fundo (Jardim)
static const PlayerColliderBox LAN_COLLIDERS[] = {
    { -4.70f, -3.70f, -2.05f,  3.70f }, // Divisao + bancada vertical conectada de parede a parede
    { -0.25f,  0.65f,  1.35f,  2.25f }, // Mesa redonda 1 (centro-frente)
    { -0.25f, -2.05f,  1.35f, -0.45f }, // Mesa redonda 2 (centro-fundo)
    {  2.05f, -2.05f,  3.65f, -0.45f }, // Mesa redonda 3 (direita-fundo)
};

static const SalaInteractable LAN_INTERACTABLES[] = {
    {
        "bancada_vendedores",
        "Bancada da Lanchonete",
        { -2.95f, 1.00f,  0.20f },
        { 1.85f, 1.80f, 4.40f },
        2.35f,
        false,
        "Os vendedores aguardam atrás da bancada, mas você não precisa comprar nada agora."
    },
    {
        "mesa_lanchonete_1",
        "Mesa da Lanchonete",
        {  0.55f, 0.65f,  1.45f },
        { 1.35f, 0.90f, 1.35f },
        1.75f,
        true,
        "Uma mesa redonda limpa com guardanapos e um pequeno cardápio."
    },
    {
        "mesa_lanchonete_2",
        "Mesa da Lanchonete",
        {  2.85f, 0.65f, -1.25f },
        { 1.35f, 0.90f, 1.35f },
        1.75f,
        true,
        "Alguém deixou um recibo amassado ao lado de um copo de café vazio."
    }
};

static const SalaPorta LAN_PORTAS[] = {
    // Porta de vidro na direita-frente -> volta para Recepcao
    {
        TERREO_DEST_RECEPCAO,
        "Porta de Vidro (Recepção)",
        { {  4.35f, 0.0f,  0.45f }, {  4.85f, 2.30f,  1.85f } },
        {  4.40f, 0.0f,  1.15f },
        1.45f,
        {  4.10f,  0.50f, 0.65f, 1.30f }
    },
    // Porta de vidro ao fundo -> sai para o Jardim
    {
        TERREO_DEST_JARDIM,
        "Porta de Vidro (Jardim)",
        { { -0.15f, 0.0f, -3.85f }, {  1.25f, 2.30f, -3.40f } },
        {  0.55f, 0.0f, -3.45f },
        1.45f,
        { -0.10f, -3.75f, 1.30f, 0.62f }
    }
};

static void DesenharConjuntoMesaRedonda(Vector3 pos) {
    TerreoMesaRedonda(pos, 0.56f, 0.74f);
    TerreoCadeira((Vector3){ pos.x - 0.62f, 0.0f, pos.z },  90.0f);
    TerreoCadeira((Vector3){ pos.x + 0.62f, 0.0f, pos.z }, -90.0f);
    TerreoCadeira((Vector3){ pos.x, 0.0f, pos.z - 0.62f },   0.0f);
}

static void DesenharLanchonete(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;

    float w = LAN_MAX_X - LAN_MIN_X;
    float d = LAN_MAX_Z - LAN_MIN_Z;
    float cx = (LAN_MIN_X + LAN_MAX_X) * 0.5f;
    float cz = (LAN_MIN_Z + LAN_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.58f));
    Color corGrid = TerreoLum(0.42f);
    for (float x = -4.0f; x <= 4.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, LAN_MIN_Z }, (Vector3){ x, 0.004f, LAN_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ LAN_MIN_X, 0.004f, z }, (Vector3){ LAN_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes
    TerreoCaixa((Vector3){ cx, LAN_WALL_H * 0.5f, LAN_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, LAN_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ LAN_MIN_X - 0.08f, LAN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, LAN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ LAN_MAX_X + 0.08f, LAN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, LAN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Portas de vidro transparentes (Recepcao na direita-frente, Jardim ao fundo)
    TerreoPortaModelo((Vector3){ LAN_MAX_X - 0.04f, 0.0f, 1.15f }, -90.0f, PORTA_TIPO_VIDRO);
    TerreoPortaModelo((Vector3){ 0.55f, 0.0f, LAN_MIN_Z + 0.04f },   0.0f, PORTA_TIPO_VIDRO);

    // PAREDE ESQUERDA VERTICAL: Divisao + Bancada conectada ate a parede (z: -3.70..+3.70)
    TerreoCaixa((Vector3){ -2.80f, 0.52f, 0.0f }, (Vector3){ 0.82f, 1.04f, 7.40f }, 0.36f, 0.28f, 0.26f, 0.12f);
    TerreoCaixa((Vector3){ -2.80f, 1.06f, 0.0f }, (Vector3){ 0.92f, 0.06f, 7.40f }, 0.82f, 0.62f, 0.54f, 0.16f);

    // Divisoria superior envidracada conectada de ponta a ponta
    for (int p = 0; p < 5; p++) {
        float pz = -3.45f + p * 1.72f;
        DrawCylinder((Vector3){ -2.42f, 1.09f, pz }, 0.03f, 0.03f, 1.20f, 8, TerreoLum(0.24f));
    }
    TerreoCaixa((Vector3){ -2.42f, 2.28f, 0.0f }, (Vector3){ 0.06f, 0.08f, 7.40f }, 0.28f, 0.22f, 0.22f, 0.10f);

    // Vitrines, caixa e cafeteira sobre a bancada vertical
    TerreoCaixa((Vector3){ -2.75f, 1.28f,  1.25f }, (Vector3){ 0.48f, 0.40f, 1.25f }, 0.80f, 0.72f, 0.66f, 0.20f);
    TerreoCaixa((Vector3){ -2.75f, 1.22f, -0.45f }, (Vector3){ 0.34f, 0.28f, 0.36f }, 0.22f, 0.18f, 0.16f, 0.35f);
    TerreoCaixa((Vector3){ -2.85f, 1.30f, -2.15f }, (Vector3){ 0.44f, 0.48f, 0.56f }, 0.28f, 0.20f, 0.18f, 0.12f);

    // 2 vendedores como sombras puras atras da bancada (sem dialogo importante)
    TerreoSilhuetaSombra((Vector3){ -3.85f, 0.0f, -1.25f }, 90.0f, false);
    TerreoSilhuetaSombra((Vector3){ -3.85f, 0.0f,  1.05f }, 90.0f, false);

    // 4 banquinhos alinhados ao longo da bancada vertical
    for (int b = 0; b < 4; b++) {
        TerreoBanqueta((Vector3){ -2.18f, 0.0f, -2.25f + b * 1.25f }, 0.68f);
    }

    // 3 mesas redondas organizadas na area centro-direita
    DesenharConjuntoMesaRedonda((Vector3){ 0.55f, 0.0f,  1.45f });
    DesenharConjuntoMesaRedonda((Vector3){ 0.55f, 0.0f, -1.25f });
    DesenharConjuntoMesaRedonda((Vector3){ 2.85f, 0.0f, -1.25f });
}

static const TerreoSalaDef SALA_LANCHONETE_DEF = {
    .id = TERREO_SALA_LANCHONETE,
    .camera = {
        .position   = { -0.25f, 5.45f, 8.20f },
        .target     = {  0.00f, 0.70f, -0.20f },
        .up         = {  0.00f, 1.00f,  0.00f },
        .fovy       = 40.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { LAN_MIN_X, LAN_MIN_Z, LAN_MAX_X - LAN_MIN_X, LAN_MAX_Z - LAN_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 3.55f, 0.0f, 1.15f },
    .defaultSpawnYaw  = -PI * 0.5f,
    .colliders        = LAN_COLLIDERS,
    .numColliders     = (int)(sizeof(LAN_COLLIDERS) / sizeof(LAN_COLLIDERS[0])),
    .interactables    = LAN_INTERACTABLES,
    .numInteractables = (int)(sizeof(LAN_INTERACTABLES) / sizeof(LAN_INTERACTABLES[0])),
    .portas           = LAN_PORTAS,
    .numPortas        = (int)(sizeof(LAN_PORTAS) / sizeof(LAN_PORTAS[0])),
    .drawFunc         = DesenharLanchonete
};

const TerreoSalaDef *SalaLanchoneteDef(void) {
    return &SALA_LANCHONETE_DEF;
}
