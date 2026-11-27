#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Area de Convivio (TerreoRASEC3D.png):
// - Acima do Escritorio Adm 1 (porta embaixo)
// - A direita do Jardim (porta a esquerda)
// - A esquerda dos Banheiros (porta decorativa a direita)
#define CONV_MIN_X -4.80f
#define CONV_MAX_X 4.80f
#define CONV_MIN_Z -3.40f
#define CONV_MAX_Z 3.40f
#define CONV_WALL_H 2.95f

static const PlayerColliderBox CONV_COLLIDERS[] = {
    { -2.45f, -3.35f,  0.45f, -1.85f }, // Sofa redondo encostado na parede
    { -1.70f, -0.95f, -0.30f,  0.25f }, // Mesinha de centro
    {  1.55f, -2.45f,  3.15f, -0.85f }, // Mesa redonda de convivio
    {  3.75f, -3.25f,  4.65f, -2.35f }, // Bebedouro
    { -4.55f, -3.20f, -3.75f, -2.35f }, // Vaso de planta
    {  3.75f,  2.25f,  4.55f,  3.05f }, // Vaso de planta canto direito
};

static const SalaInteractable CONV_INTERACTABLES[] = {
    {
        "sofa_convivio",
        "Área de Descanso",
        { -1.00f, 0.55f, -2.00f },
        { 2.60f, 0.90f, 1.00f },
        1.95f,
        false,
        "Sofás confortáveis e uma mesa de centro para os funcionários descansarem nos intervalos."
    },
    {
        "mesa_convivio",
        "Mesa de Convívio",
        { 2.35f, 0.65f, -1.65f },
        { 1.40f, 0.95f, 1.40f },
        1.85f,
        true,
        "Mesa redonda de convivência ao lado do bebedouro e dos banheiros do térreo."
    },
    {
        "porta_banheiros_terreo",
        "Banheiros (Térreo)",
        { 4.45f, 1.15f, -0.20f },
        { 0.60f, 2.20f, 1.30f },
        1.55f,
        false,
        "Banheiros do térreo. Não há nada de relevante para a investigação lá dentro."
    }
};

static const SalaPorta CONV_PORTAS[] = {
    // Esquerda -> Jardim
    {
        TERREO_DEST_JARDIM,
        "Porta (Jardim)",
        { { -4.95f, 0.0f, -0.70f }, { -4.45f, 2.30f, 0.70f } },
        { -4.50f, 0.0f, 0.00f },
        1.40f,
        { -4.85f, -0.65f, 0.55f, 1.30f }
    },
    // Embaixo -> Escritorio Adm 1
    {
        TERREO_DEST_ESCRITORIO,
        "Porta (Setor Administrativo 1)",
        { { -0.70f, 0.0f, 3.15f }, { 0.70f, 2.30f, 3.55f } },
        { 0.00f, 0.0f, 3.15f },
        1.40f,
        { -0.65f, 2.90f, 1.30f, 0.58f }
    }
};

static void DesenharConvivio(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = CONV_MAX_X - CONV_MIN_X;
    float d = CONV_MAX_Z - CONV_MIN_Z;
    float cx = (CONV_MIN_X + CONV_MAX_X) * 0.5f;
    float cz = (CONV_MIN_Z + CONV_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.56f));
    Color corGrid = TerreoLum(0.43f);
    for (float x = -4.0f; x <= 4.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, CONV_MIN_Z }, (Vector3){ x, 0.004f, CONV_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 1.0f) {
        DrawLine3D((Vector3){ CONV_MIN_X, 0.004f, z }, (Vector3){ CONV_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, CONV_WALL_H * 0.5f, CONV_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, CONV_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ CONV_MIN_X - 0.08f, CONV_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CONV_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ CONV_MAX_X + 0.08f, CONV_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, CONV_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Portas: Jardim (esq), Adm 1 (frente/baixo), Banheiros (dir, decorativa)
    TerreoPortaModelo((Vector3){ CONV_MIN_X + 0.04f, 0.0f, 0.00f },   90.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ 0.00f, 0.0f, CONV_MAX_Z - 0.04f },  180.0f, PORTA_TIPO_ESCRITORIO);
    TerreoPortaModelo((Vector3){ CONV_MAX_X - 0.04f, 0.0f, -0.20f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // Tapete + Sofa redondo encostado na parede + Mesinha de centro com perna visivel
    TerreoCaixa((Vector3){ -1.00f, 0.01f, -1.35f }, (Vector3){ 3.50f, 0.02f, 2.95f }, 0.24f, 0.20f, 0.18f, -1.0f);
    TerreoSofaRedondo((Vector3){ -1.00f, 0.0f, -2.65f }, 0.0f, 1.35f);
    TerreoMesaRedonda((Vector3){ -1.00f, 0.0f, -0.35f }, 0.52f, 0.56f);

    // Mesa redonda + banquetas + bebedouro + plantas
    TerreoMesaRedonda((Vector3){ 2.35f, 0.0f, -1.65f }, 0.65f, 0.76f);
    TerreoBanqueta((Vector3){ 1.50f, 0.0f, -1.65f }, 0.56f);
    TerreoBanqueta((Vector3){ 3.20f, 0.0f, -1.65f }, 0.56f);
    TerreoBebedouro((Vector3){ 4.20f, 0.0f, -2.80f });
    TerreoPlanta((Vector3){ -4.15f, 0.0f, -2.75f }, 1.35f);
    TerreoPlanta((Vector3){  4.15f, 0.0f,  2.65f }, 1.25f);

    // Dois funcionarios conversando entre si (animados)
    TerreoDesenharNPCAtividade(1, 7 /* NPC_ATIV_CONVERSANDO */, (Vector3){ -2.10f, 0.0f, 0.95f },  68.0f, tempo);
    TerreoDesenharNPCAtividade(2, 7 /* NPC_ATIV_CONVERSANDO */, (Vector3){ -1.25f, 0.0f, 1.18f }, -112.0f, tempo + 1.4f);
}

static const TerreoSalaDef SALA_CONVIVIO_DEF = {
    .id = TERREO_SALA_CONVIVIO,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.10f },
        .target     = { 0.0f, 0.75f, -0.25f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { CONV_MIN_X, CONV_MIN_Z, CONV_MAX_X - CONV_MIN_X, CONV_MAX_Z - CONV_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 0.0f, 0.0f, 2.55f },
    .defaultSpawnYaw  = PI,
    .colliders        = CONV_COLLIDERS,
    .numColliders     = (int)(sizeof(CONV_COLLIDERS) / sizeof(CONV_COLLIDERS[0])),
    .interactables    = CONV_INTERACTABLES,
    .numInteractables = (int)(sizeof(CONV_INTERACTABLES) / sizeof(CONV_INTERACTABLES[0])),
    .portas           = CONV_PORTAS,
    .numPortas        = (int)(sizeof(CONV_PORTAS) / sizeof(CONV_PORTAS[0])),
    .drawFunc         = DesenharConvivio
};

const TerreoSalaDef *SalaConvivioDef(void) {
    return &SALA_CONVIVIO_DEF;
}
