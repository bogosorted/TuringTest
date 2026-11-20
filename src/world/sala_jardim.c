#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const float JAR_MIN_X = -6.20f;
static const float JAR_MAX_X =  6.20f;
static const float JAR_MIN_Z = -4.40f;
static const float JAR_MAX_Z =  4.40f;

// Jardim amplo e organizado (simetrico, paisagismo corporativo de empresa multimilionaria):
// - Fonte circular grande no centro (x=0, z=-0.15)
// - 4 arvores organicas simetricas nos canteiros dos cantos
// - 2 bancos ripados de jardim simetricos (norte e sul-centro/laterais)
// - 2 mesinhas redondas de descanso com cadeiras (sem mesas de escritorio!)
static const PlayerColliderBox JAR_COLLIDERS[] = {
    { -1.35f, -1.50f,  1.35f,  1.20f }, // Fonte circular central
    { -5.20f, -3.70f, -3.90f, -2.40f }, // Arvore canteiro fundo-esquerdo
    {  3.90f, -3.70f,  5.20f, -2.40f }, // Arvore canteiro fundo-direito
    { -5.20f,  2.15f, -3.90f,  3.45f }, // Arvore canteiro frente-esquerdo
    {  3.90f,  2.15f,  5.20f,  3.45f }, // Arvore canteiro frente-direito
    { -0.95f, -3.20f,  0.95f, -2.35f }, // Banco norte
    { -3.25f,  1.75f, -1.75f,  3.15f }, // Mesinha redonda descanso esquerda
    {  1.75f,  1.75f,  3.25f,  3.15f }, // Mesinha redonda descanso direita
};

static const SalaInteractable JAR_INTERACTABLES[] = {
    {
        "fonte_jardim",
        "Fonte Central",
        { 0.00f, 0.70f, -0.15f },
        { 2.60f, 1.40f, 2.60f },
        2.35f,
        false,
        "Uma fonte circular imponente no centro do pátio corporativo. Os jatos d'água criam um ruído constante que abafa as conversas."
    },
    {
        "banco_jardim",
        "Banco de Jardim",
        { 0.00f, 0.50f, -2.75f },
        { 1.85f, 0.90f, 0.90f },
        1.85f,
        false,
        "Banco ripado voltado para a fonte central e para a fachada envidraçada da RASEC."
    },
    {
        "mesa_descanso_jardim",
        "Mesa de Descanso",
        { -2.50f, 0.65f,  2.45f },
        { 1.40f, 0.95f, 1.40f },
        1.85f,
        true,
        "Área externa de descanso para os funcionários. Cadeiras de pátio ao redor de uma mesa redonda sob a sombra das árvores."
    },
    {
        "arvore_jardim",
        "Árvore do Pátio",
        { -4.55f, 1.40f, -3.05f },
        { 1.45f, 2.60f, 1.45f },
        2.05f,
        false,
        "Árvores bem cuidadas compõem o paisagismo simétrico do pátio interno da RASEC."
    }
};

static const SalaPorta JAR_PORTAS[] = {
    // Esquerda -> volta para Lanchonete
    {
        TERREO_DEST_LANCHONETE,
        "Porta (Lanchonete)",
        { { -6.35f, 0.0f, -0.70f }, { -5.85f, 2.30f,  0.70f } },
        { -5.90f, 0.0f,  0.00f },
        1.45f,
        { -6.25f, -0.65f, 0.65f, 1.30f }
    },
    // Direita -> vai para Setor Administrativo
    {
        TERREO_DEST_ESCRITORIO,
        "Porta (Setor Administrativo)",
        { {  5.85f, 0.0f, -0.70f }, {  6.35f, 2.30f,  0.70f } },
        {  5.90f, 0.0f,  0.00f },
        1.45f,
        {  5.60f, -0.65f, 0.65f, 1.30f }
    }
};

static void DesenharArvoreOrganica(Vector3 pos, float escala) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlScalef(escala, escala, escala);

    // Canteiro circular de pedra ao redor da arvore
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.62f, 0.65f, 0.14f, 16, TerreoLum(0.46f));
    DrawCylinder((Vector3){ 0.0f, 0.13f, 0.0f }, 0.52f, 0.54f, 0.03f, 14, TerreoLum(0.24f));

    // Tronco cilindrico organico
    DrawCylinderEx((Vector3){ 0.0f, 0.12f, 0.0f }, (Vector3){ 0.04f, 1.48f, -0.02f }, 0.12f, 0.08f, 10, TerreoLum(0.18f));
    DrawCylinderEx((Vector3){ 0.02f, 1.05f, 0.0f }, (Vector3){ -0.28f, 1.55f, 0.12f }, 0.055f, 0.035f, 8, TerreoLum(0.18f));
    DrawCylinderEx((Vector3){ 0.02f, 1.15f, 0.0f }, (Vector3){  0.28f, 1.58f, -0.10f }, 0.055f, 0.035f, 8, TerreoLum(0.18f));

    // Copa organica formada por esferas sobrepostas (nada de cubos!)
    DrawSphereEx((Vector3){  0.00f, 1.92f,  0.00f }, 0.62f, 10, 14, TerreoLum(0.42f));
    DrawSphereEx((Vector3){ -0.34f, 1.72f,  0.22f }, 0.44f, 8,  12, TerreoLum(0.36f));
    DrawSphereEx((Vector3){  0.34f, 1.76f, -0.18f }, 0.46f, 8,  12, TerreoLum(0.48f));
    DrawSphereEx((Vector3){  0.05f, 2.28f,  0.05f }, 0.45f, 8,  12, TerreoLum(0.54f));

    rlPopMatrix();
}

static void DesenharBancoOrganico(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Pes metalicos cilindricos curvados
    for (int side = -1; side <= 1; side += 2) {
        float x = (float)side * 0.65f;
        DrawCylinderEx((Vector3){ x, 0.0f,  0.22f }, (Vector3){ x, 0.42f,  0.20f }, 0.032f, 0.032f, 8, TerreoLum(0.18f));
        DrawCylinderEx((Vector3){ x, 0.0f, -0.22f }, (Vector3){ x, 0.42f, -0.20f }, 0.032f, 0.032f, 8, TerreoLum(0.18f));
        DrawCylinderEx((Vector3){ x, 0.42f, -0.20f }, (Vector3){ x, 0.80f, -0.28f }, 0.030f, 0.028f, 8, TerreoLum(0.18f));
    }
    // Ripas arredondadas (capsulas) do assento e encosto
    for (int r = 0; r < 3; r++) {
        float zr = -0.14f + r * 0.14f;
        DrawCapsule((Vector3){ -0.78f, 0.43f, zr }, (Vector3){ 0.78f, 0.43f, zr }, 0.036f, 5, 8, TerreoLum(0.56f));
    }
    for (int r = 0; r < 2; r++) {
        float yr = 0.60f + r * 0.14f;
        float zr = -0.23f - r * 0.03f;
        DrawCapsule((Vector3){ -0.78f, yr, zr }, (Vector3){ 0.78f, yr, zr }, 0.036f, 5, 8, TerreoLum(0.52f));
    }

    rlPopMatrix();
}

static void DesenharMesaPatio(Vector3 pos) {
    TerreoMesaRedonda(pos, 0.52f, 0.68f);
    TerreoCadeira((Vector3){ pos.x - 0.62f, 0.0f, pos.z },  90.0f);
    TerreoCadeira((Vector3){ pos.x + 0.62f, 0.0f, pos.z }, -90.0f);
}

static void DesenharJardim(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;

    float w = JAR_MAX_X - JAR_MIN_X;
    float d = JAR_MAX_Z - JAR_MIN_Z;
    float cx = (JAR_MIN_X + JAR_MAX_X) * 0.5f;
    float cz = (JAR_MIN_Z + JAR_MAX_Z) * 0.5f;

    // 1) Gramado corporativo escuro e canteiros organizados + caminhos de pedra simetricos
    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.30f));

    // Eixo horizontal de pedra ligando Lanchonete (esquerda) <-> Fonte <-> Setor Adm. (direita)
    DrawPlane((Vector3){ 0.0f, 0.01f, 0.0f }, (Vector2){ w - 0.20f, 2.30f }, TerreoLum(0.64f));
    // Praca circular central ao redor da fonte + caminho para os bancos/mesas
    DrawCylinder((Vector3){ 0.0f, 0.004f, -0.15f }, 2.75f, 2.75f, 0.015f, 28, TerreoLum(0.66f));
    DrawPlane((Vector3){ 0.0f, 0.01f, -2.55f }, (Vector2){ 2.60f, 2.20f }, TerreoLum(0.62f));
    DrawPlane((Vector3){ -2.50f, 0.01f, 2.00f }, (Vector2){ 2.10f, 2.00f }, TerreoLum(0.62f));
    DrawPlane((Vector3){  2.50f, 0.01f, 2.00f }, (Vector2){ 2.10f, 2.00f }, TerreoLum(0.62f));

    // 2) Fachada envidracada do edificio ao fundo (inspirada em Jardmi2.jpg, ceu aberto acima!)
    TerreoCaixa((Vector3){ cx, 0.55f, JAR_MIN_Z - 0.10f },
                (Vector3){ w + 0.32f, 1.10f, 0.20f }, 0.46f, 0.38f, 0.34f, 0.14f);
    // Pilares de vidro/metal da fachada ao fundo
    for (float gx = -5.4f; gx <= 5.41f; gx += 1.35f) {
        DrawCylinder((Vector3){ gx, 1.10f, JAR_MIN_Z - 0.08f }, 0.05f, 0.05f, 1.45f, 8, TerreoLum(0.24f));
    }
    DrawLine3D((Vector3){ JAR_MIN_X, 2.55f, JAR_MIN_Z - 0.08f },
               (Vector3){ JAR_MAX_X, 2.55f, JAR_MIN_Z - 0.08f }, TerreoLum(0.32f));

    // Paredes laterais com portas industriais de acesso externo (baseadas em Porta3.jpg)
    TerreoCaixa((Vector3){ JAR_MIN_X - 0.10f, 1.50f, cz },
                (Vector3){ 0.20f, 3.00f, d }, 0.36f, 0.40f, 0.42f, 0.14f);
    TerreoCaixa((Vector3){ JAR_MAX_X + 0.10f, 1.50f, cz },
                (Vector3){ 0.20f, 3.00f, d }, 0.36f, 0.40f, 0.42f, 0.14f);
    TerreoPortaModelo((Vector3){ JAR_MIN_X + 0.04f, 0.0f, 0.00f },  90.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ JAR_MAX_X - 0.04f, 0.0f, 0.00f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    // 3) Fonte circular central grande (inspirada em Jardmi2.jpg)
    Vector3 fPos = { 0.00f, 0.0f, -0.15f };
    DrawCylinder(fPos, 1.32f, 1.38f, 0.36f, 24, TerreoLum(0.70f));
    float waterLum = 0.86f + 0.05f * sinf(tempo * 2.6f);
    DrawCylinder((Vector3){ fPos.x, 0.35f, fPos.z }, 1.16f, 1.16f, 0.04f, 24, TerreoLum(waterLum));
    DrawCylinder((Vector3){ fPos.x, 0.38f, fPos.z }, 0.24f, 0.30f, 0.62f, 16, TerreoLum(0.62f));
    DrawCylinder((Vector3){ fPos.x, 0.96f, fPos.z }, 0.56f, 0.24f, 0.16f, 18, TerreoLum(0.76f));
    DrawCylinder((Vector3){ fPos.x, 1.12f, fPos.z }, 0.10f, 0.14f, 0.26f, 12, TerreoLum(0.68f));
    // Jatos d'agua estilizados em arco
    for (int j = 0; j < 6; j++) {
        float ang = (float)j * (PI / 3.0f) + tempo * 0.4f;
        Vector3 pTop = { fPos.x + cosf(ang) * 0.22f, 1.48f, fPos.z + sinf(ang) * 0.22f };
        Vector3 pOut = { fPos.x + cosf(ang) * 0.85f, 0.42f, fPos.z + sinf(ang) * 0.85f };
        DrawLine3D((Vector3){ fPos.x, 1.36f, fPos.z }, pTop, TerreoLum(0.94f));
        DrawLine3D(pTop, pOut, TerreoLum(0.88f));
    }

    // 4) 4 Arvores organicas nos canteiros simetricos dos cantos
    DesenharArvoreOrganica((Vector3){ -4.55f, 0.0f, -3.05f }, 1.05f);
    DesenharArvoreOrganica((Vector3){  4.55f, 0.0f, -3.05f }, 1.05f);
    DesenharArvoreOrganica((Vector3){ -4.55f, 0.0f,  2.80f }, 0.95f);
    DesenharArvoreOrganica((Vector3){  4.55f, 0.0f,  2.80f }, 0.95f);

    // 5) Banco ripado de jardim + 2 mesas redondas de descanso (sem mesas de escritorio!)
    DesenharBancoOrganico((Vector3){ 0.00f, 0.0f, -2.75f }, 0.0f);
    DesenharMesaPatio((Vector3){ -2.50f, 0.0f, 2.45f });
    DesenharMesaPatio((Vector3){  2.50f, 0.0f, 2.45f });
}

static const TerreoSalaDef SALA_JARDIM_DEF = {
    .id = TERREO_SALA_JARDIM,
    .camera = {
        .position   = { 0.00f, 6.20f, 9.55f },
        .target     = { 0.00f, 0.70f, -0.20f },
        .up         = { 0.00f, 1.00f,  0.00f },
        .fovy       = 41.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { JAR_MIN_X, JAR_MIN_Z, JAR_MAX_X - JAR_MIN_X, JAR_MAX_Z - JAR_MIN_Z },
    .bgColor          = { 125, 112, 113, 255 },
    .defaultSpawnPos  = { -4.90f, 0.0f, 0.00f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = JAR_COLLIDERS,
    .numColliders     = (int)(sizeof(JAR_COLLIDERS) / sizeof(JAR_COLLIDERS[0])),
    .interactables    = JAR_INTERACTABLES,
    .numInteractables = (int)(sizeof(JAR_INTERACTABLES) / sizeof(JAR_INTERACTABLES[0])),
    .portas           = JAR_PORTAS,
    .numPortas        = (int)(sizeof(JAR_PORTAS) / sizeof(JAR_PORTAS[0])),
    .drawFunc         = DesenharJardim
};

const TerreoSalaDef *SalaJardimDef(void) {
    return &SALA_JARDIM_DEF;
}
