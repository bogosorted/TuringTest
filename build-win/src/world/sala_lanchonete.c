#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Planta baixa do terreo (TerreoRASEC3D.png):
// A Lanchonete/Cantina ocupa toda a altura vertical esquerda do terreo e cresce para a esquerda (<) para dar espaco as mesas.
#define LAN_MIN_X -5.60f
#define LAN_MAX_X 3.90f
#define LAN_MIN_Z -6.20f
#define LAN_MAX_Z 6.20f
#define LAN_WALL_H 2.85f

static const PlayerColliderBox LAN_COLLIDERS[] = {
    { -5.60f, -6.20f, -3.20f,  6.20f }, // Divisao + bancada vertical na parede esquerda
    {  0.55f, -4.40f,  2.15f, -2.80f }, // Mesa redonda 1 (fundo)
    {  0.55f, -1.20f,  2.15f,  0.40f }, // Mesa redonda 2 (centro)
    {  0.55f,  2.40f,  2.15f,  4.00f }, // Mesa redonda 3 (frente)
    { -1.95f,  0.60f, -0.35f,  2.20f }, // Mesa redonda 4 (meio-esquerda)
};

static const SalaInteractable LAN_INTERACTABLES[] = {
    {
        "bancada_vendedores",
        "Bancada da Lanchonete",
        { -4.05f, 1.00f,  0.00f },
        { 1.75f, 1.80f, 7.60f },
        2.35f,
        false,
        "Os vendedores aguardam atrás da longa bancada da cantina, mas você não precisa comprar nada agora."
    },
    {
        "mesa_lanchonete_1",
        "Mesa da Lanchonete",
        {  1.35f, 0.65f, -3.60f },
        { 1.35f, 0.90f, 1.35f },
        1.75f,
        true,
        "Uma mesa redonda limpa com guardanapos e um pequeno cardápio."
    },
    {
        "mesa_lanchonete_2",
        "Mesa da Lanchonete",
        {  1.35f, 0.65f,  3.20f },
        { 1.35f, 0.90f, 1.35f },
        1.75f,
        true,
        "Alguém deixou um recibo amassado ao lado de um copo de café vazio."
    },
    {
        "tv_lanchonete",
        "TV da Lanchonete",
        { -0.35f, 2.05f, -6.05f },
        { 1.65f, 0.95f, 0.35f },
        2.35f,
        false,
        "A TV da cantina transmite o boletim diário da RASEC e o cardápio do almoço."
    }
};

static const SalaPorta LAN_PORTAS[] = {
    // Porta de vidro na parede direita-frente -> volta para Recepcao (bem separada da porta do Jardim)
    {
        TERREO_DEST_RECEPCAO,
        "Porta de Vidro (Recepção)",
        { {  3.55f, 0.0f,  3.50f }, {  4.05f, 2.30f,  4.90f } },
        {  3.60f, 0.0f,  4.20f },
        1.45f,
        {  3.35f,  3.55f, 0.65f, 1.30f }
    },
    // Porta de vidro na parede direita-fundo -> sai para o Jardim
    {
        TERREO_DEST_JARDIM,
        "Porta de Vidro (Jardim)",
        { {  3.55f, 0.0f, -4.90f }, {  4.05f, 2.30f, -3.50f } },
        {  3.60f, 0.0f, -4.20f },
        1.45f,
        {  3.35f, -4.85f, 0.65f, 1.30f }
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
    for (float x = -5.0f; x <= 3.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, LAN_MIN_Z }, (Vector3){ x, 0.004f, LAN_MAX_Z }, corGrid);
    }
    for (float z = -6.0f; z <= 6.01f; z += 1.0f) {
        DrawLine3D((Vector3){ LAN_MIN_X, 0.004f, z }, (Vector3){ LAN_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes
    TerreoCaixa((Vector3){ cx, LAN_WALL_H * 0.5f, LAN_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, LAN_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ LAN_MIN_X - 0.08f, LAN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, LAN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ LAN_MAX_X + 0.08f, LAN_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, LAN_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Portas de vidro transparentes na parede direita (bem separadas: Jardim ao fundo, Recepcao a frente)
    // + pequena porta decorativa de cozinha/preparo na parede ESQUERDA atras da bancada (sem interacao/acesso)
    TerreoPortaModelo((Vector3){ LAN_MAX_X - 0.04f, 0.0f,  4.20f }, -90.0f, PORTA_TIPO_VIDRO);
    TerreoPortaModelo((Vector3){ LAN_MAX_X - 0.04f, 0.0f, -4.20f }, -90.0f, PORTA_TIPO_VIDRO);
    rlPushMatrix();
    rlTranslatef(LAN_MIN_X + 0.04f, 0.0f, -3.55f);
    rlRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    rlScalef(0.76f, 0.88f, 1.0f);
    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, 0.0f }, 0.0f, PORTA_TIPO_ESCRITORIO);
    rlPopMatrix();

    // TVs corporativas na lanchonete (parede do fundo e parede direita entre as portas)
    TerreoTV((Vector3){ -0.35f, 2.05f, LAN_MIN_Z + 0.08f },   0.0f, tempo);
    TerreoTV((Vector3){ LAN_MAX_X - 0.08f, 2.05f, 0.00f },  -90.0f, tempo);

    // PAREDE ESQUERDA VERTICAL: Divisao + Bancada conectada de ponta a ponta (z: -6.20..+6.20)
    TerreoCaixa((Vector3){ -4.05f, 0.52f, 0.0f }, (Vector3){ 0.82f, 1.04f, d }, 0.36f, 0.28f, 0.26f, 0.12f);
    TerreoCaixa((Vector3){ -4.05f, 1.06f, 0.0f }, (Vector3){ 0.92f, 0.06f, d }, 0.82f, 0.62f, 0.54f, 0.16f);

    // Divisoria superior envidracada conectada de ponta a ponta
    for (int p = 0; p < 8; p++) {
        float pz = -5.75f + p * 1.64f;
        DrawCylinder((Vector3){ -3.66f, 1.09f, pz }, 0.03f, 0.03f, 1.20f, 8, TerreoLum(0.24f));
    }
    TerreoCaixa((Vector3){ -3.66f, 2.28f, 0.0f }, (Vector3){ 0.06f, 0.08f, d }, 0.28f, 0.22f, 0.22f, 0.10f);

    // Vitrine detalhada com bandejas e salgados/doces, caixa registradora e cafeteira
    {
        // Base e vidro da vitrine de salgados
        TerreoCaixa((Vector3){ -4.00f, 1.13f,  2.35f }, (Vector3){ 0.52f, 0.08f, 1.50f }, 0.34f, 0.28f, 0.26f, 0.12f);
        DrawCubeWires((Vector3){ -4.00f, 1.35f, 2.35f }, 0.48f, 0.38f, 1.44f, TerreoLum(0.86f));
        TerreoCaixa((Vector3){ -4.00f, 1.32f,  2.35f }, (Vector3){ 0.44f, 0.02f, 1.38f }, 0.76f, 0.70f, 0.64f, -1.0f);
        // Bandejas com itens a venda (salgados, sanduiches e doces)
        for (int it = 0; it < 4; it++) {
            float iz = 1.85f + it * 0.33f;
            TerreoCaixa((Vector3){ -4.00f, 1.20f, iz }, (Vector3){ 0.30f, 0.05f, 0.22f }, 0.88f, 0.78f, 0.72f, 0.20f);
            DrawSphere((Vector3){ -4.00f, 1.38f, iz }, 0.065f, TerreoLum(0.70f + (it % 2) * 0.16f));
        }
        // Caixa registradora detalhada (gaveta + monitor POS + teclado + leitor)
        TerreoCaixa((Vector3){ -4.02f, 1.15f, -0.25f }, (Vector3){ 0.38f, 0.10f, 0.42f }, 0.26f, 0.20f, 0.18f, 0.14f);
        TerreoCaixa((Vector3){ -4.06f, 1.32f, -0.25f }, (Vector3){ 0.05f, 0.24f, 0.32f }, 0.18f, 0.12f, 0.12f, 0.46f);
        TerreoCaixa((Vector3){ -3.90f, 1.22f, -0.25f }, (Vector3){ 0.12f, 0.04f, 0.28f }, 0.34f, 0.28f, 0.28f, 0.42f);
        // Pilha de copos e potes ao lado da cafeteira
        DrawCylinder((Vector3){ -4.05f, 1.09f, -1.75f }, 0.045f, 0.038f, 0.18f, 10, TerreoLum(0.92f));
        DrawCylinder((Vector3){ -4.05f, 1.09f, -1.55f }, 0.045f, 0.038f, 0.14f, 10, TerreoLum(0.86f));
        DrawCylinder((Vector3){ -4.18f, 1.09f, -1.65f }, 0.060f, 0.060f, 0.20f, 10, TerreoLum(0.74f));
    }
    TerreoCafeteira((Vector3){ -4.10f, 1.09f, -2.85f }, 90.0f);

    // 2 trabalhadores atras da bancada: um cansado e outro mexendo no caixa
    TerreoDesenharNPCAtividade(1, 8 /* NPC_ATIV_CANSADO */, (Vector3){ -4.90f, 0.0f, -1.85f }, 90.0f, tempo);
    TerreoDesenharNPCAtividade(3, 9 /* NPC_ATIV_CAIXA */,   (Vector3){ -4.85f, 0.0f, -0.25f }, 90.0f, tempo);

    // 6 banquinhos alinhados ao longo da bancada vertical
    for (int b = 0; b < 6; b++) {
        TerreoBanqueta((Vector3){ -3.38f, 0.0f, -4.15f + b * 1.66f }, 0.68f);
    }

    // 4 mesas redondas bem espacadas da bancada e das portas
    DesenharConjuntoMesaRedonda((Vector3){  1.35f, 0.0f, -3.60f });
    DesenharConjuntoMesaRedonda((Vector3){  1.35f, 0.0f, -0.40f });
    DesenharConjuntoMesaRedonda((Vector3){  1.35f, 0.0f,  3.20f });
    DesenharConjuntoMesaRedonda((Vector3){ -1.15f, 0.0f,  1.40f });
}

static const TerreoSalaDef SALA_LANCHONETE_DEF = {
    .id = TERREO_SALA_LANCHONETE,
    .camera = {
        .position   = { -0.60f, 6.40f, 10.60f },
        .target     = { -0.60f, 0.70f, 0.00f },
        .up         = {  0.00f, 1.00f, 0.00f },
        .fovy       = 41.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { LAN_MIN_X, LAN_MIN_Z, LAN_MAX_X - LAN_MIN_X, LAN_MAX_Z - LAN_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 2.95f, 0.0f, 4.20f },
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
