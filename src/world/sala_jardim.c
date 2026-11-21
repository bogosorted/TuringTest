#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const float JAR_MIN_X = -6.20f;
static const float JAR_MAX_X =  6.20f;
static const float JAR_MIN_Z = -7.60f;
static const float JAR_MAX_Z =  4.40f;

// Jardim amplo e aesthetic:
// - Fonte circular grande melhorada no centro (x=0, z=-0.65)
// - Pequeno laguinho artificial com cachoeira de pedras no gramado norte (x=-2.50, z=-5.80)
// - 4 arvores gigantes com galhos crescendo para fora do topo da camera (sem cimento)
// - Mesas de descanso diretamente no gramado (sem quadrados de cimento ao redor)
static const PlayerColliderBox JAR_COLLIDERS[] = {
    { -1.45f, -2.10f,  1.45f,  0.80f }, // Fonte circular central
    { -5.35f, -6.75f, -3.75f, -5.15f }, // Arvore gigante fundo-esquerdo
    {  3.75f, -6.75f,  5.35f, -5.15f }, // Arvore gigante fundo-direito
    { -5.20f,  1.75f, -3.70f,  3.25f }, // Arvore gigante frente-esquerdo
    {  3.70f,  1.75f,  5.20f,  3.25f }, // Arvore gigante frente-direito
    { -0.95f, -4.55f,  0.95f, -3.70f }, // Banco norte
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
        "ev_jardim_sombras",
        "Funcionários conversando",
        { 2.50f, 0.95f, 2.45f },
        { 1.80f, 1.45f, 1.40f },
        2.35f,
        false,
        "Dois funcionários conversam baixo na mesa do jardim."
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
    // Direita -> vai para Area de Convivio
    {
        TERREO_DEST_CONVIVIO,
        "Porta (Área de Convívio)",
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

    // Sombra projetada no gramado dentro dos limites do mapa (sem cimento embaixo)
    DrawCylinder((Vector3){ 0.0f, 0.008f, 0.0f }, 1.28f, 1.28f, 0.010f, 20, TerreoLum(0.14f));
    DrawCylinder((Vector3){ 0.0f, 0.012f, 0.0f }, 0.85f, 0.85f, 0.010f, 18, TerreoLum(0.10f));

    // Tronco alto e robusto que cresce para fora do topo da camera (sem mostrar o topo!)
    DrawCylinderEx((Vector3){ 0.0f, 0.00f, 0.0f }, (Vector3){ 0.06f, 4.60f, -0.04f }, 0.24f, 0.13f, 10, TerreoLum(0.16f));
    DrawCylinderEx((Vector3){ 0.0f, 0.00f, 0.0f }, (Vector3){ -0.30f, 0.03f,  0.16f }, 0.10f, 0.03f, 6, TerreoLum(0.15f));
    DrawCylinderEx((Vector3){ 0.0f, 0.00f, 0.0f }, (Vector3){  0.28f, 0.03f, -0.18f }, 0.10f, 0.03f, 6, TerreoLum(0.15f));

    // Galhos organicos altos abrindo-se acima da linha de visao
    DrawCylinderEx((Vector3){ 0.03f, 3.10f, 0.0f }, (Vector3){ -1.15f, 4.25f,  0.45f }, 0.095f, 0.040f, 8, TerreoLum(0.16f));
    DrawCylinderEx((Vector3){ 0.04f, 3.35f, 0.0f }, (Vector3){  1.10f, 4.45f, -0.35f }, 0.095f, 0.040f, 8, TerreoLum(0.16f));
    DrawCylinderEx((Vector3){ 0.03f, 3.60f, 0.0f }, (Vector3){  0.15f, 4.55f,  1.05f }, 0.085f, 0.035f, 8, TerreoLum(0.16f));

    // Folhagem organica densa bem alta (cresce para fora da tela, deixando apenas bordas baixas visiveis)
    DrawSphereEx((Vector3){  0.00f, 5.10f,  0.00f }, 1.65f, 10, 14, TerreoLum(0.36f));
    DrawSphereEx((Vector3){ -1.05f, 4.45f,  0.42f }, 1.05f, 9,  12, TerreoLum(0.30f));
    DrawSphereEx((Vector3){  1.02f, 4.60f, -0.32f }, 1.10f, 9,  12, TerreoLum(0.42f));
    DrawSphereEx((Vector3){  0.14f, 4.55f,  0.96f }, 0.98f, 9,  12, TerreoLum(0.34f));
    // Pequenos cachos de folhas pendendo dos galhos mais baixos
    DrawSphereEx((Vector3){ -1.18f, 3.92f,  0.52f }, 0.44f, 7, 9, TerreoLum(0.28f));
    DrawSphereEx((Vector3){  1.08f, 4.02f,  0.22f }, 0.42f, 7, 9, TerreoLum(0.36f));

    rlPopMatrix();
}

static void DesenharLaguinhoCachoeira(Vector3 pos, float tempo) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);

    // Borda natural de pedras ao redor do laguinho artificial
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 1.25f, 1.32f, 0.14f, 20, TerreoLum(0.40f));
    float waterLum = 0.80f + 0.06f * sinf(tempo * 2.4f);
    DrawCylinder((Vector3){ 0.0f, 0.13f, 0.0f }, 1.08f, 1.08f, 0.02f, 20, TerreoLum(waterLum));

    // Pedras empilhadas ao fundo formando a nascente da cachoeira
    DrawSphereEx((Vector3){  0.00f, 0.42f, -0.82f }, 0.46f, 8, 10, TerreoLum(0.34f));
    DrawSphereEx((Vector3){ -0.45f, 0.30f, -0.74f }, 0.34f, 7,  9, TerreoLum(0.38f));
    DrawSphereEx((Vector3){  0.46f, 0.32f, -0.72f }, 0.36f, 7,  9, TerreoLum(0.32f));
    DrawSphereEx((Vector3){  0.05f, 0.72f, -0.90f }, 0.32f, 7,  9, TerreoLum(0.42f));

    // Queda d'agua animada descendo das pedras para o laguinho
    for (int k = -2; k <= 2; k++) {
        float xk = (float)k * 0.09f;
        float off = fmodf(tempo * 1.4f + (float)(k + 2) * 0.2f, 1.0f);
        Vector3 top = { xk, 0.68f - off * 0.12f, -0.58f };
        Vector3 bot = { xk * 1.15f, 0.14f, -0.32f + off * 0.10f };
        DrawLine3D(top, bot, TerreoLum(0.94f));
    }
    // Ondas concentricas na agua onde a cachoeira cai
    float rWave = 0.14f + fmodf(tempo * 0.35f, 0.48f);
    DrawCylinderWires((Vector3){ 0.0f, 0.155f, -0.28f }, rWave, rWave, 0.008f, 14, TerreoLum(0.95f));

    rlPopMatrix();
}

static void DesenharTufosGrama(void) {
    static const Vector3 TUFOS[] = {
        { -3.20f, 0.0f, -4.90f }, { -1.95f, 0.0f, -5.20f }, {  1.95f, 0.0f, -5.15f }, {  3.25f, 0.0f, -4.85f },
        { -3.65f, 0.0f, -2.55f }, { -2.45f, 0.0f, -2.15f }, {  2.45f, 0.0f, -2.20f }, {  3.70f, 0.0f, -2.60f },
        { -5.30f, 0.0f, -1.65f }, {  5.25f, 0.0f, -1.70f }, { -5.15f, 0.0f,  1.55f }, {  5.15f, 0.0f,  1.55f },
        { -0.95f, 0.0f,  2.85f }, {  0.95f, 0.0f,  2.95f }, { -3.85f, 0.0f,  3.55f }, {  3.85f, 0.0f,  3.55f },
    };
    for (int i = 0; i < (int)(sizeof(TUFOS) / sizeof(TUFOS[0])); i++) {
        Vector3 p = TUFOS[i];
        DrawCapsule(p, (Vector3){ p.x - 0.06f, 0.16f, p.z + 0.02f }, 0.015f, 4, 6, TerreoLum(0.42f));
        DrawCapsule(p, (Vector3){ p.x + 0.05f, 0.19f, p.z - 0.02f }, 0.015f, 4, 6, TerreoLum(0.48f));
        DrawCapsule(p, (Vector3){ p.x + 0.01f, 0.14f, p.z + 0.06f }, 0.014f, 4, 6, TerreoLum(0.38f));
    }
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
    // Praca circular central ao redor da fonte + caminho para o banco norte (mesas ficam direto na grama, sem cimento!)
    DrawCylinder((Vector3){ 0.0f, 0.004f, -0.65f }, 2.75f, 2.75f, 0.015f, 28, TerreoLum(0.66f));
    DrawPlane((Vector3){ 0.0f, 0.01f, -2.95f }, (Vector2){ 2.40f, 2.40f }, TerreoLum(0.62f));

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

    // 3) Fonte circular central melhorada (duas bordas, ondulacoes na agua, bacia dupla e jatos)
    Vector3 fPos = { 0.00f, 0.0f, -0.65f };
    DrawCylinder(fPos, 1.42f, 1.48f, 0.22f, 28, TerreoLum(0.64f));
    DrawCylinder((Vector3){ fPos.x, 0.20f, fPos.z }, 1.30f, 1.34f, 0.20f, 28, TerreoLum(0.74f));
    float waterLum = 0.86f + 0.05f * sinf(tempo * 2.6f);
    DrawCylinder((Vector3){ fPos.x, 0.39f, fPos.z }, 1.18f, 1.18f, 0.03f, 28, TerreoLum(waterLum));
    // Aneis concentricos na agua da fonte
    float rRip = 0.35f + fmodf(tempo * 0.45f, 0.72f);
    DrawCylinderWires((Vector3){ fPos.x, 0.425f, fPos.z }, rRip, rRip, 0.006f, 20, TerreoLum(0.96f));
    DrawCylinder((Vector3){ fPos.x, 0.41f, fPos.z }, 0.22f, 0.30f, 0.64f, 18, TerreoLum(0.62f));
    DrawCylinder((Vector3){ fPos.x, 1.02f, fPos.z }, 0.62f, 0.24f, 0.16f, 20, TerreoLum(0.78f));
    DrawCylinder((Vector3){ fPos.x, 1.18f, fPos.z }, 0.10f, 0.14f, 0.32f, 14, TerreoLum(0.68f));
    DrawCylinder((Vector3){ fPos.x, 1.48f, fPos.z }, 0.26f, 0.10f, 0.08f, 16, TerreoLum(0.82f));
    for (int j = 0; j < 8; j++) {
        float ang = (float)j * (PI / 4.0f) + tempo * 0.45f;
        Vector3 pTop = { fPos.x + cosf(ang) * 0.24f, 1.72f, fPos.z + sinf(ang) * 0.24f };
        Vector3 pOut = { fPos.x + cosf(ang) * 0.92f, 0.43f, fPos.z + sinf(ang) * 0.92f };
        DrawLine3D((Vector3){ fPos.x, 1.56f, fPos.z }, pTop, TerreoLum(0.96f));
        DrawLine3D(pTop, pOut, TerreoLum(0.88f));
    }

    // 4) 4 Arvores gigantes crescendo para fora do topo da camera + gramas aesthetic
    DesenharArvoreOrganica((Vector3){ -4.55f, 0.0f, -5.95f }, 1.15f);
    DesenharArvoreOrganica((Vector3){  4.55f, 0.0f, -5.95f }, 1.15f);
    DesenharArvoreOrganica((Vector3){ -4.45f, 0.0f,  2.55f }, 1.05f);
    DesenharArvoreOrganica((Vector3){  4.45f, 0.0f,  2.55f }, 1.05f);
    DesenharTufosGrama();

    // 5) Banco ripado de jardim + 2 mesas redondas de descanso + 2 silhuetas conversando na mesa direita
    DesenharBancoOrganico((Vector3){ 0.00f, 0.0f, -4.15f }, 0.0f);
    DesenharMesaPatio((Vector3){ -2.50f, 0.0f, 2.45f });
    DesenharMesaPatio((Vector3){  2.50f, 0.0f, 2.45f });

    // Duas silhuetas (sombras) sentadas fixas nas cadeiras da mesa direita (sem pulsar!)
    float balanco1 = sinf(tempo * 2.1f) * 3.5f;
    float balanco2 = sinf(tempo * 1.9f + 1.8f) * 3.5f;
    PlayerCharacterDrawNpc3DEx((Vector3){ 2.50f - 0.62f, 0.0f, 2.45f },  90.0f + balanco1, NPC_ATIV_SENTADO, 0, tempo, (Vector3){ 0.0f, 6.5f, 10.5f });
    PlayerCharacterDrawNpc3DEx((Vector3){ 2.50f + 0.62f, 0.0f, 2.45f }, -90.0f + balanco2, NPC_ATIV_SENTADO, 2, tempo + 1.4f, (Vector3){ 0.0f, 6.5f, 10.5f });
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
