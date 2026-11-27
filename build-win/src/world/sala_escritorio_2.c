#include "terreo_3d.h"
#include "game_state.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define ESC2_MIN_X -5.80f
#define ESC2_MAX_X 5.80f
#define ESC2_MIN_Z -4.20f
#define ESC2_MAX_Z 4.20f
#define ESC2_WALL_H 3.00f

// Setor Administrativo 2 (conectado ao Setor ADM 1 com porta para voltar na parede esquerda!):
// - Caminho da porta (x=-5.8..-4.2, z=-2.0..0.0) totalmente LIVRE
static const PlayerColliderBox ESC2_COLLIDERS[] = {
    { -5.50f, -4.05f, -1.65f, -2.95f }, // Gaveteiros + 3 impressoras no canto esquerdo superior
    { -2.45f,  1.15f,  1.95f,  2.55f }, // Fileira 1 de baias
    { -2.45f, -1.25f,  1.95f,  0.15f }, // Fileira 2 de baias
    {  4.35f, -2.30f,  5.80f,  2.30f }, // Mesas encostadas na parede direita
    { -5.50f,  2.95f, -4.70f,  3.75f }, // Bebedouro no canto inferior esquerdo
    {  4.70f,  3.10f,  5.50f,  3.90f }, // Planta ornamental no canto inferior direito
    {  1.60f, -4.10f,  3.00f, -3.30f }, // Arquivo de gavetas sob o quadro
};

static const SalaInteractable ESC2_INTERACTABLES[] = {
    {
        "ev_arthur_sobrecarga",
        "Computador (Memorando)",
        { 4.65f, 0.95f, 1.60f },
        { 0.95f, 0.95f, 0.95f },
        1.30f,
        true,
        "Uma tela ligada na bancada direita exibindo um memorando interno sobre o setor."
    },
    {
        "ev_analu_terminal",
        "Terminal de Ana Lúcia",
        { -1.65f, 0.98f, -0.15f },
        { 0.95f, 0.95f, 0.95f },
        1.30f,
        true,
        "Terminal destravado de Ana Lúcia exibindo histórico suspeito de madrugada."
    },
    {
        "impressoras_ala2",
        "Central de Impressão",
        { -2.95f, 1.00f, -3.55f },
        { 3.10f, 1.45f, 1.05f },
        2.15f,
        true,
        "Várias impressoras corporativas zumbem em fila imprimindo pilhas intermináveis de planilhas."
    },
    {
        "quadro_branco_adm2",
        "Quadros Brancos",
        { -0.20f, 1.75f, -3.95f },
        { 3.20f, 1.40f, 0.45f },
        2.20f,
        false,
        "Quadros brancos com fluxogramas, metas do trimestre e lembretes de auditoria."
    },
    {
        "planta_adm2",
        "Planta Decorativa",
        { 5.10f, 0.90f, 3.50f },
        { 0.80f, 1.40f, 0.80f },
        1.75f,
        false,
        "Uma planta ornamental bem cuidada em um vaso de cerâmica geométrica."
    },
    {
        "quadro_adm2",
        "Quadro Institucional",
        { 2.30f, 1.70f, -3.95f },
        { 1.50f, 1.10f, 0.40f },
        1.90f,
        false,
        "Quadro com diretrizes e organograma do Setor Administrativo."
    },
    {
        "funcionario_focado_3",
        "Funcionário Trabalhando",
        { -0.25f, 0.95f,  2.35f },
        { 0.85f, 1.40f, 0.85f },
        1.75f,
        false,
        "Um funcionário trabalhando, ele está muito focado para falar com você."
    },
    {
        "funcionario_focado_4",
        "Funcionário Trabalhando",
        {  1.15f, 0.95f, -0.05f },
        { 0.85f, 1.40f, 0.85f },
        1.75f,
        false,
        "Um funcionário trabalhando, ele está muito focado para falar com você."
    },
    {
        "bebedouro_adm2",
        "Bebedouro",
        { -5.10f, 0.85f, 3.35f },
        { 0.70f, 1.55f, 0.70f },
        1.75f,
        true,
        "Outro bebedouro corporativo. Copos descartáveis empilhados ao lado."
    }
};

static const SalaPorta ESC2_PORTAS[] = {
    // Porta na parede esquerda -> volta para Setor ADM 1
    {
        TERREO_DEST_ESCRITORIO,
        "Porta (Setor Administrativo 1)",
        { { -5.95f, 0.0f, -1.60f }, { -5.45f, 2.30f, -0.10f } },
        { -5.50f, 0.0f, -0.85f },
        1.45f,
        { -5.85f, -1.55f, 0.65f, 1.40f }
    }
};

static void DesenharBaiaCorporativa2(Vector3 centro, int numPostos, float larguraPosto, float profTotal) {
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
            TerreoGabinetePC((Vector3){ cx + 0.44f, 0.77f, centro.z - 0.16f }, 0.0f);
            Vector3 cPos = { cx, 0.0f, centro.z + profTotal * 0.5f + 0.24f };
            float pull = Terreo3DGetChairPull(cPos);
            TerreoCadeira((Vector3){ cPos.x, 0.0f, cPos.z + pull }, 180.0f);
        }
    }
}

static void DesenharEscritorio2(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;

    float w = ESC2_MAX_X - ESC2_MIN_X;
    float d = ESC2_MAX_Z - ESC2_MIN_Z;
    float cx = (ESC2_MIN_X + ESC2_MAX_X) * 0.5f;
    float cz = (ESC2_MIN_Z + ESC2_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.42f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, ESC2_MIN_Z }, (Vector3){ x, 0.004f, ESC2_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.0f) {
        DrawLine3D((Vector3){ ESC2_MIN_X, 0.004f, z }, (Vector3){ ESC2_MAX_X, 0.004f, z }, corGrid);
    }

    // Paredes fechadas com porta corporativa na parede esquerda para voltar ao Setor ADM 1
    TerreoCaixa((Vector3){ cx, ESC2_WALL_H * 0.5f, ESC2_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, ESC2_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ ESC2_MIN_X - 0.08f, ESC2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ESC2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ ESC2_MAX_X + 0.08f, ESC2_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, ESC2_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Porta para voltar ao Setor ADM 1 (parede esquerda, completamente desobstruida!)
    TerreoPortaModelo((Vector3){ ESC2_MIN_X + 0.04f, 0.0f, -0.85f }, 90.0f, PORTA_TIPO_ESCRITORIO);

    // Quadros brancos corporativos na parede do fundo (substituindo a janela que dava para outra sala)
    for (int j = 0; j < 2; j++) {
        float qx = -0.90f + j * 1.65f;
        // Moldura metalica
        TerreoCaixa((Vector3){ qx, 1.75f, ESC2_MIN_Z + 0.02f },
                    (Vector3){ 1.42f, 1.02f, 0.04f }, 0.30f, 0.24f, 0.24f, 0.10f);
        // Superficie branca
        TerreoCaixa((Vector3){ qx, 1.75f, ESC2_MIN_Z + 0.04f },
                    (Vector3){ 1.32f, 0.92f, 0.02f }, 0.90f, 0.88f, 0.84f, -1.0f);
        // Bandeja de pincel na base
        TerreoCaixa((Vector3){ qx, 1.24f, ESC2_MIN_Z + 0.07f },
                    (Vector3){ 0.85f, 0.03f, 0.06f }, 0.34f, 0.28f, 0.28f, 0.12f);
        // Anotacoes / fluxograma desenhado
        DrawLine3D((Vector3){ qx - 0.50f, 1.98f, ESC2_MIN_Z + 0.055f },
                   (Vector3){ qx + 0.45f, 1.98f, ESC2_MIN_Z + 0.055f }, TerreoLum(0.24f));
        DrawLine3D((Vector3){ qx - 0.45f, 1.75f, ESC2_MIN_Z + 0.055f },
                   (Vector3){ qx + 0.25f, 1.75f, ESC2_MIN_Z + 0.055f }, TerreoLum(0.32f));
        DrawLine3D((Vector3){ qx - 0.48f, 1.54f, ESC2_MIN_Z + 0.055f },
                   (Vector3){ qx + 0.38f, 1.54f, ESC2_MIN_Z + 0.055f }, TerreoLum(0.28f));
    }

    // Quadro de diretrizes/metas corporativas + armario/gabinete detalhado de 4 gavetas
    TerreoCaixa((Vector3){ 2.30f, 1.70f, ESC2_MIN_Z + 0.03f },
                (Vector3){ 1.40f, 0.90f, 0.04f }, 0.78f, 0.72f, 0.65f, 0.18f);
    TerreoArmario((Vector3){ 2.30f, 0.0f, -3.70f }, 0.0f, 1.20f, 0.92f, 0.55f);

    // Gaveteiro detalhado + 3 Impressoras alinhadas ao lado no fundo-esquerdo
    TerreoArmario((Vector3){ -5.05f, 0.0f, -3.55f }, 0.0f, 0.74f, 1.50f, 0.65f);
    TerreoImpressora((Vector3){ -4.05f, 0.0f, -3.55f }, 0.0f);
    TerreoImpressora((Vector3){ -3.10f, 0.0f, -3.55f }, 0.0f);
    TerreoImpressora((Vector3){ -2.15f, 0.0f, -3.55f }, 0.0f);

    // Baias centrais repetidas + funcionarios como sombras puras
    DesenharBaiaCorporativa2((Vector3){ -0.25f, 0.0f,  1.75f }, 3, 1.40f, 0.84f);
    TerreoSilhuetaSombra((Vector3){ -0.25f, 0.0f,  2.38f }, 180.0f, true);

    DesenharBaiaCorporativa2((Vector3){ -0.25f, 0.0f, -0.65f }, 3, 1.40f, 0.84f);
    if (GameStateEvidenciaDisponivelHoje(2, "EV-05") && !GameStateTemEvidencia(2, "EV-05")) {
        TerreoEvidenciaBrilhante((Vector3){ -1.65f, 0.77f, -0.81f }, tempo, 3);
    }
    TerreoSilhuetaSombra((Vector3){  1.15f, 0.0f, -0.02f }, 180.0f, true);

    // Fileira direita completa encostada na parede direita
    TerreoCaixa((Vector3){ 5.30f, 0.74f, 0.00f }, (Vector3){ 0.88f, 0.05f, 4.40f }, 0.68f, 0.48f, 0.44f, 0.14f);
    if (GameStateEvidenciaDisponivelHoje(1, "EV-03") && !GameStateTemEvidencia(1, "EV-03")) {
        rlPushMatrix();
        rlTranslatef(5.45f, 0.77f, 1.60f);
        rlRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        TerreoEvidenciaBrilhante((Vector3){ 0.0f, 0.0f, 0.0f }, tempo, 3);
        rlPopMatrix();
    }
    DrawCylinder((Vector3){ 5.05f, 0.0f, -2.05f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    DrawCylinder((Vector3){ 5.05f, 0.0f,  2.05f }, 0.035f, 0.035f, 0.72f, 8, TerreoLum(0.22f));
    for (int k = 0; k < 3; k++) {
        float kz = -1.60f + k * 1.60f;
        TerreoMonitor((Vector3){ 5.45f, 0.77f, kz }, -90.0f);
        TerreoGabinetePC((Vector3){ 5.45f, 0.77f, kz + 0.42f }, -90.0f);
        Vector3 cPos = { 4.58f, 0.0f, kz };
        float pull = Terreo3DGetChairPull(cPos);
        TerreoCadeira((Vector3){ cPos.x - pull, 0.0f, cPos.z }, 90.0f);
    }

    // Bebedouro no canto inferior esquerdo
    TerreoBebedouro((Vector3){ -5.10f, 0.0f, 3.35f });

    // Planta decorativa ornamental no canto inferior direito
    TerreoPlanta((Vector3){ 5.10f, 0.0f, 3.50f }, 1.45f);
}

static const TerreoSalaDef SALA_ESCRITORIO_2_DEF = {
    .id = TERREO_SALA_ESCRITORIO_2,
    .camera = {
        .position   = { 0.35f, 5.95f, 9.10f },
        .target     = { 0.00f, 0.75f, -0.25f },
        .up         = { 0.00f, 1.00f,  0.00f },
        .fovy       = 40.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { ESC2_MIN_X, ESC2_MIN_Z, ESC2_MAX_X - ESC2_MIN_X, ESC2_MAX_Z - ESC2_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { -4.55f, 0.0f, -0.85f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = ESC2_COLLIDERS,
    .numColliders     = (int)(sizeof(ESC2_COLLIDERS) / sizeof(ESC2_COLLIDERS[0])),
    .interactables    = ESC2_INTERACTABLES,
    .numInteractables = (int)(sizeof(ESC2_INTERACTABLES) / sizeof(ESC2_INTERACTABLES[0])),
    .portas           = ESC2_PORTAS,
    .numPortas        = (int)(sizeof(ESC2_PORTAS) / sizeof(ESC2_PORTAS[0])),
    .drawFunc         = DesenharEscritorio2
};

const TerreoSalaDef *SalaEscritorio2Def(void) {
    return &SALA_ESCRITORIO_2_DEF;
}
