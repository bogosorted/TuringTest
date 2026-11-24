#include "terreo_3d.h"
#include "game_state.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const float ESC_MIN_X = -5.80f;
static const float ESC_MAX_X =  5.80f;
static const float ESC_MIN_Z = -4.20f;
static const float ESC_MAX_Z =  4.20f;
static const float ESC_WALL_H = 3.00f;

// Setor Administrativo 1:
// - Gaveteiros + impressora AO LADO no canto esquerdo superior
// - 2 fileiras centrais de baias + fileira encostada na parede direita
// - Bebedouro no canto inferior direito
// - Funcionarios nao-dialogaveis trabalhando focados
// - Porta na parede direita (z = -0.85) ligando ao Setor ADM 2
static const PlayerColliderBox ESC_COLLIDERS[] = {
    { -5.50f, -4.05f, -2.65f, -2.95f }, // Gaveteiros + impressora ao lado (canto esquerdo superior)
    { -2.45f,  1.15f,  1.95f,  2.55f }, // Fileira 1 de baias
    { -2.45f, -1.25f,  1.95f,  0.15f }, // Fileira 2 de baias
    {  4.25f,  0.45f,  5.80f,  2.85f }, // Mesas encostadas na parede direita (abaixo da porta!)
    {  4.85f,  3.05f,  5.65f,  3.85f }, // Bebedouro no canto inferior direito
};

static const SalaInteractable ESC_INTERACTABLES[] = {
    {
        "ev_maria_queixa",
        "Bolinha de Papel Amassada",
        { 1.35f, 0.90f, 1.85f },
        { 1.15f, 0.95f, 1.05f },
        1.95f,
        true,
        "Uma bolinha de papel amassada jogada no canto da mesa."
    },
    {
        "impressora_setor",
        "Impressora e Arquivos",
        { -3.25f, 1.05f, -3.55f },
        { 2.35f, 1.50f, 1.05f },
        2.05f,
        true,
        "Impressora corporativa posicionada ao lado dos gaveteiros de arquivo no canto superior esquerdo."
    },
    {
        "bebedouro_adm",
        "Bebedouro",
        { 5.25f, 0.85f, 3.45f },
        { 0.70f, 1.55f, 0.70f },
        1.75f,
        true,
        "Um bebedouro de galão no canto inferior direito. A água está bem gelada."
    },
    {
        "funcionario_focado_1",
        "Funcionário Trabalhando",
        { -1.65f, 0.95f, -0.25f },
        { 0.80f, 1.40f, 0.80f },
        1.75f,
        false,
        "Um funcionário trabalhando, ele está muito focado para falar com você."
    },
    {
        "funcionario_focado_2",
        "Funcionário Trabalhando",
        { 4.65f, 0.95f, 2.15f },
        { 0.80f, 1.40f, 0.80f },
        1.75f,
        false,
        "Um funcionário trabalhando, ele está muito focado para falar com você."
    },
    {
        "quadro_avisos_adm",
        "Quadro de Metas",
        { 2.15f, 1.65f, -4.10f },
        { 1.60f, 0.95f, 0.40f },
        1.90f,
        false,
        "Quadro de metas do Setor Administrativo. Alguém sublinhou o prazo da supervisão duas vezes."
    }
};

static const SalaPorta ESC_PORTAS[] = {
    // Esquerda inferior -> volta para a Recepcao
    {
        TERREO_DEST_RECEPCAO,
        "Porta (Recepção)",
        { { -5.95f, 0.0f,  0.95f }, { -5.45f, 2.30f,  2.35f } },
        { -5.50f, 0.0f,  1.65f },
        1.45f,
        { -5.85f,  1.00f, 0.65f, 1.30f }
    },
    // Direita -> vai para o Setor ADM 2
    {
        TERREO_DEST_ESCRITORIO_2,
        "Porta (Setor Administrativo 2)",
        { {  5.45f, 0.0f, -1.60f }, {  5.95f, 2.30f, -0.10f } },
        {  5.50f, 0.0f, -0.85f },
        1.45f,
        {  5.20f, -1.55f, 0.65f, 1.40f }
    },
    // Fundo -> vai para a Area de Convivio
    {
        TERREO_DEST_CONVIVIO,
        "Porta (Área de Convívio)",
        { { -0.70f, 0.0f, -4.35f }, {  0.70f, 2.30f, -3.90f } },
        {  0.00f, 0.0f, -3.95f },
        1.45f,
        { -0.65f, -4.25f, 1.30f, 0.62f }
    }
};

static void DesenharBaiaCorporativa(Vector3 centro, int numPostos, float larguraPosto, float profTotal) {
    float totalW = numPostos * larguraPosto;
    float x0 = centro.x - totalW * 0.5f;

    TerreoCaixa((Vector3){ centro.x, 0.74f, centro.z },
                (Vector3){ totalW, 0.05f, profTotal }, 0.68f, 0.48f, 0.44f, 0.14f);

    DrawCylinder((Vector3){ x0 + 0.08f, 0.0f, centro.z - profTotal * 0.35f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    DrawCylinder((Vector3){ x0 + 0.08f, 0.0f, centro.z + profTotal * 0.35f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    DrawCylinder((Vector3){ x0 + totalW - 0.08f, 0.0f, centro.z - profTotal * 0.35f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    DrawCylinder((Vector3){ x0 + totalW - 0.08f, 0.0f, centro.z + profTotal * 0.35f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));

    TerreoCaixa((Vector3){ centro.x, 0.98f, centro.z - profTotal * 0.46f },
                (Vector3){ totalW + 0.04f, 0.96f, 0.05f }, 0.40f, 0.32f, 0.28f, 0.12f);

    for (int i = 0; i <= numPostos; i++) {
        float px = x0 + i * larguraPosto;
        TerreoCaixa((Vector3){ px, 0.98f, centro.z },
                    (Vector3){ 0.045f, 0.96f, profTotal + 0.04f }, 0.40f, 0.30f, 0.32f, 0.12f);

        if (i < numPostos) {
            float cx = px + larguraPosto * 0.5f;
            TerreoMonitor((Vector3){ cx, 0.77f, centro.z - 0.16f }, 0.0f);
            TerreoGabinetePC((Vector3){ cx + 0.42f, 0.77f, centro.z - 0.14f }, 0.0f);
            TerreoCadeira((Vector3){ cx, 0.0f, centro.z + profTotal * 0.5f + 0.24f }, 180.0f);
        }
    }
}

static void DesenharPilhaPapeis(Vector3 pos, bool destaqueEv) {
    float lTop = destaqueEv ? 0.98f : 0.94f;
    TerreoCaixa((Vector3){ pos.x - 0.12f, pos.y + 0.08f, pos.z }, (Vector3){ 0.26f, 0.16f, 0.22f }, lTop, 0.80f, 0.74f, 0.20f);
    TerreoCaixa((Vector3){ pos.x + 0.14f, pos.y + 0.12f, pos.z - 0.04f }, (Vector3){ 0.24f, 0.24f, 0.20f }, lTop, 0.82f, 0.76f, 0.20f);
    TerreoCaixa((Vector3){ pos.x + 0.02f, pos.y + 0.04f, pos.z + 0.16f }, (Vector3){ 0.22f, 0.08f, 0.18f }, lTop, 0.78f, 0.72f, 0.20f);
}

static void DesenharEscritorio(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;

    float w = ESC_MAX_X - ESC_MIN_X;
    float d = ESC_MAX_Z - ESC_MIN_Z;
    float cx = (ESC_MIN_X + ESC_MAX_X) * 0.5f;
    float cz = (ESC_MIN_Z + ESC_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.42f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, ESC_MIN_Z }, (Vector3){ x, 0.004f, ESC_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.0f) {
        DrawLine3D((Vector3){ ESC_MIN_X, 0.004f, z }, (Vector3){ ESC_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes fechadas (esquerda -> Recepcao, fundo -> Jardim, direita -> Setor ADM 2)
    TerreoCaixa((Vector3){ cx, ESC_WALL_H * 0.5f, ESC_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, ESC_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ ESC_MIN_X - 0.08f, ESC_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ESC_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ ESC_MAX_X + 0.08f, ESC_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ESC_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Portas: esquerda inferior (Recepcao), fundo (Jardim) e direita (Setor ADM 2)
    TerreoPortaModelo((Vector3){ ESC_MIN_X + 0.04f, 0.0f,  1.65f },  90.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ 0.00f, 0.0f, ESC_MIN_Z + 0.04f },   0.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ ESC_MAX_X - 0.04f, 0.0f, -0.85f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // Quadro de avisos ao fundo-direita
    TerreoCaixa((Vector3){ 2.15f, 1.65f, ESC_MIN_Z + 0.03f },
                (Vector3){ 1.50f, 0.90f, 0.04f }, 0.80f, 0.76f, 0.68f, 0.18f);

    // 1) CANTO ESQUERDO SUPERIOR: Armarios (modelo 3D_cabinet_7) + Impressora AO LADO
    TerreoArmario((Vector3){ -5.05f, 0.0f, -3.55f }, 0.0f, 0.78f, 1.55f, 0.62f);
    TerreoArmario((Vector3){ -4.20f, 0.0f, -3.55f }, 0.0f, 0.78f, 1.55f, 0.62f);
    TerreoImpressora((Vector3){ -3.25f, 0.0f, -3.55f }, 0.0f);

    // 2) FILEIRAS CENTRAIS DE BAIAS + EVIDENCIA BRILHANTE (EV-01: Bolinha de papel amassada da Maria)
    DesenharBaiaCorporativa((Vector3){ -0.25f, 0.0f,  1.75f }, 3, 1.40f, 0.84f);
    if (GameStateEvidenciaDisponivelHoje(0, "EV-01") && !GameStateTemEvidencia(0, "EV-01")) {
        TerreoEvidenciaBrilhante((Vector3){ 1.35f, 0.765f, 1.85f }, tempo, 5);
    } else {
        DrawSphereEx((Vector3){ 1.35f, 0.82f, 1.85f }, 0.07f, 5, 6, TerreoLum(0.86f));
    }

    DesenharBaiaCorporativa((Vector3){ -0.25f, 0.0f, -0.65f }, 3, 1.40f, 0.84f);
    // Funcionario como sombra pura trabalhando na baia do meio
    TerreoSilhuetaSombra((Vector3){ -1.65f, 0.0f, -0.02f }, 180.0f, true);

    // 3) FILEIRA ENCOSTADA NA PAREDE DIREITA (abaixo da porta para Setor ADM 2)
    TerreoCaixa((Vector3){ 5.30f, 0.74f, 1.60f }, (Vector3){ 0.88f, 0.05f, 2.35f }, 0.68f, 0.48f, 0.44f, 0.14f);
    DrawCylinder((Vector3){ 5.05f, 0.0f, 0.60f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    DrawCylinder((Vector3){ 5.05f, 0.0f, 2.60f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    for (int k = 0; k < 2; k++) {
        float kz = 0.95f + k * 1.20f;
        TerreoMonitor((Vector3){ 5.45f, 0.77f, kz }, -90.0f);
        TerreoGabinetePC((Vector3){ 5.40f, 0.77f, kz + 0.40f }, -90.0f);
        TerreoCadeira((Vector3){ 4.58f, 0.0f,  kz },  90.0f);
    }
    TerreoSilhuetaSombra((Vector3){ 4.62f, 0.0f, 2.15f }, 90.0f, true);

    // 4) BEBEDOURO NO CANTO INFERIOR DIREITO
    TerreoBebedouro((Vector3){ 5.25f, 0.0f, 3.45f });
}

static const TerreoSalaDef SALA_ESCRITORIO_DEF = {
    .id = TERREO_SALA_ESCRITORIO,
    .camera = {
        .position   = { 0.35f, 5.95f, 9.10f },
        .target     = { 0.00f, 0.75f, -0.25f },
        .up         = { 0.00f, 1.00f,  0.00f },
        .fovy       = 40.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { ESC_MIN_X, ESC_MIN_Z, ESC_MAX_X - ESC_MIN_X, ESC_MAX_Z - ESC_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { -4.65f, 0.0f, 1.65f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = ESC_COLLIDERS,
    .numColliders     = (int)(sizeof(ESC_COLLIDERS) / sizeof(ESC_COLLIDERS[0])),
    .interactables    = ESC_INTERACTABLES,
    .numInteractables = (int)(sizeof(ESC_INTERACTABLES) / sizeof(ESC_INTERACTABLES[0])),
    .portas           = ESC_PORTAS,
    .numPortas        = (int)(sizeof(ESC_PORTAS) / sizeof(ESC_PORTAS[0])),
    .drawFunc         = DesenharEscritorio
};

const TerreoSalaDef *SalaEscritorioDef(void) {
    return &SALA_ESCRITORIO_DEF;
}
