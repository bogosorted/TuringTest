#include "terreo_3d.h"
#include "raymath.h"
#include "rlgl.h"
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const float REC_MIN_X = -5.80f;
static const float REC_MAX_X =  5.80f;
static const float REC_MIN_Z = -4.20f;
static const float REC_MAX_Z =  4.20f;
static const float REC_WALL_H = 4.20f;

// Recepcao ampla (sem portas de banheiro):
// - Balcao grande em formato de U redondo conectado a parede do fundo (player nao entra)
// - Sofas e poltronas claros sobre tapete escuro para alto contraste e facil leitura
// - Escadaria aberta no canto superior direito que o player sobe degrau por degrau
static const PlayerColliderBox REC_COLLIDERS[] = {
    { -5.35f, -4.20f, -1.55f, -1.05f }, // Balcao em U redondo conectado a parede (impede entrada)
    { -0.85f,  0.30f,  2.15f,  1.25f }, // Sofa de espera central
    { -1.95f,  1.45f, -1.00f,  2.40f }, // Poltrona esquerda
    {  2.30f,  1.45f,  3.25f,  2.40f }, // Poltrona direita
    { -0.05f,  1.50f,  1.35f,  2.35f }, // Mesinha de centro
    { -5.45f,  2.65f, -4.45f,  3.65f }, // Vaso decorativo frente-esquerda
};

static const SalaInteractable REC_INTERACTABLES[] = {
    {
        "recepcionista",
        "Recepcionista",
        { -3.45f, 1.05f, -1.95f },
        { 2.60f, 1.90f, 1.70f },
        2.65f,
        false,
        "Boa tarde! Os setores ficam pelas portas laterais."
    },
    {
        "area_espera",
        "Sofás de Espera",
        {  0.65f, 0.65f,  1.55f },
        { 2.90f, 1.15f, 1.95f },
        2.20f,
        false,
        "Sofá estofado, poltronas confortáveis e uma mesinha com arranjo floral para visitantes aguardarem atendimento."
    },
    {
        "elevador",
        "Elevador",
        { 4.35f, 1.15f, -3.95f },
        { 2.15f, 2.30f, 0.65f },
        1.85f,
        true,
        NULL
    }
};

static const SalaPorta REC_PORTAS[] = {
    // Esquerda: Porta de vidro transparente -> Lanchonete
    {
        TERREO_DEST_LANCHONETE,
        "Porta de Vidro (Lanchonete)",
        { { -5.95f, 0.0f, -0.25f }, { -5.45f, 2.30f,  1.15f } },
        { -5.50f, 0.0f,  0.45f },
        1.45f,
        { -5.85f, -0.20f, 0.65f, 1.30f }
    },
    // Canto direito inferior: Porta corporativa -> Setor Administrativo
    {
        TERREO_DEST_ESCRITORIO,
        "Porta (Setor Administrativo)",
        { {  5.45f, 0.0f,  0.65f }, {  5.95f, 2.30f,  2.05f } },
        {  5.50f, 0.0f,  1.35f },
        1.45f,
        {  5.20f,  0.70f, 0.65f, 1.30f }
    }
};

static void DesenharPoltronaClara(Vector3 pos, float rotYDeg) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Pes escuros contrastando com o tapete e assento
    DrawCylinder((Vector3){ -0.28f, 0.0f, -0.26f }, 0.032f, 0.025f, 0.14f, 8, TerreoLum(0.14f));
    DrawCylinder((Vector3){  0.28f, 0.0f, -0.26f }, 0.032f, 0.025f, 0.14f, 8, TerreoLum(0.14f));
    DrawCylinder((Vector3){ -0.28f, 0.0f,  0.26f }, 0.032f, 0.025f, 0.14f, 8, TerreoLum(0.14f));
    DrawCylinder((Vector3){  0.28f, 0.0f,  0.26f }, 0.032f, 0.025f, 0.14f, 8, TerreoLum(0.14f));

    // Assento claro com contorno e encosto envolvente bem legivel
    rlPushMatrix();
    rlTranslatef(0.0f, 0.25f, 0.0f);
    rlScalef(1.12f, 0.48f, 1.05f);
    DrawCylinder((Vector3){ 0.0f, -0.20f, 0.0f }, 0.35f, 0.35f, 0.44f, 14, TerreoLum(0.88f));
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.0f, 0.64f, -0.22f);
    rlScalef(1.22f, 1.0f, 0.55f);
    DrawCylinder((Vector3){ 0.0f, -0.26f, 0.0f }, 0.33f, 0.33f, 0.52f, 14, TerreoLum(0.80f));
    rlPopMatrix();

    // Bracos arredondados
    DrawCapsule((Vector3){ -0.35f, 0.50f, -0.18f }, (Vector3){ -0.35f, 0.50f, 0.24f }, 0.09f, 6, 8, TerreoLum(0.82f));
    DrawCapsule((Vector3){  0.35f, 0.50f, -0.18f }, (Vector3){  0.35f, 0.50f, 0.24f }, 0.09f, 6, 8, TerreoLum(0.82f));

    rlPopMatrix();
}

static void DesenharVasoPlantas(Vector3 pos, float escala) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlScalef(escala, escala, escala);

    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.28f, 0.22f, 0.62f, 14, TerreoLum(0.24f));
    DrawSphereEx((Vector3){  0.00f, 0.92f,  0.00f }, 0.30f, 8, 10, TerreoLum(0.46f));
    DrawSphereEx((Vector3){ -0.16f, 0.80f,  0.12f }, 0.22f, 7, 9,  TerreoLum(0.38f));
    DrawSphereEx((Vector3){  0.16f, 0.82f, -0.10f }, 0.22f, 7, 9,  TerreoLum(0.54f));

    rlPopMatrix();
}

static void DesenharRecepcao(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;

    float w = REC_MAX_X - REC_MIN_X;
    float d = REC_MAX_Z - REC_MIN_Z;
    float cx = (REC_MIN_X + REC_MAX_X) * 0.5f;
    float cz = (REC_MIN_Z + REC_MAX_Z) * 0.5f;

    // Piso claro corporativo
    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.62f));
    Color corJunta = TerreoLum(0.46f);
    for (float x = -5.0f; x <= 5.01f; x += 1.25f) {
        DrawLine3D((Vector3){ x, 0.004f, REC_MIN_Z }, (Vector3){ x, 0.004f, REC_MAX_Z }, corJunta);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.25f) {
        DrawLine3D((Vector3){ REC_MIN_X, 0.004f, z }, (Vector3){ REC_MAX_X, 0.004f, z }, corJunta);
    }

    // Tapete ESCURO contrastante no centro-frente para destacar os sofas brancos/claros!
    TerreoCaixa((Vector3){ 0.65f, 0.01f, 1.75f }, (Vector3){ 5.50f, 0.02f, 3.30f }, 0.28f, 0.20f, 0.20f, 0.12f);

    // Paredes (parede do fundo aberta ao redor da janela para revelar a arvore e o jardim)
    {
        float wx = 0.70f, wy = 1.75f, ww = 3.10f, wh = 1.60f;
        float winL = wx - ww * 0.5f; // -0.85
        float winR = wx + ww * 0.5f; // +2.25
        float winB = wy - wh * 0.5f; //  0.95
        float winT = wy + wh * 0.5f; //  2.55

        // Trecho esquerdo da parede do fundo (REC_MIN_X .. winL)
        float wLeft = winL - (REC_MIN_X - 0.16f);
        TerreoCaixa((Vector3){ (REC_MIN_X - 0.16f) + wLeft * 0.5f, REC_WALL_H * 0.5f, REC_MIN_Z - 0.08f },
                    (Vector3){ wLeft, REC_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
        // Trecho direito da parede do fundo (winR .. REC_MAX_X)
        float wRight = (REC_MAX_X + 0.16f) - winR;
        TerreoCaixa((Vector3){ winR + wRight * 0.5f, REC_WALL_H * 0.5f, REC_MIN_Z - 0.08f },
                    (Vector3){ wRight, REC_WALL_H, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
        // Peitoril abaixo da janela (0 .. winB)
        TerreoCaixa((Vector3){ wx, winB * 0.5f, REC_MIN_Z - 0.08f },
                    (Vector3){ ww, winB, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
        // Bandeira acima da janela (winT .. REC_WALL_H)
        float hTop = REC_WALL_H - winT;
        TerreoCaixa((Vector3){ wx, winT + hTop * 0.5f, REC_MIN_Z - 0.08f },
                    (Vector3){ ww, hTop, 0.16f }, 0.34f, 0.40f, 0.30f, 0.12f);
    }
    TerreoCaixa((Vector3){ REC_MIN_X - 0.08f, REC_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, REC_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ REC_MAX_X + 0.08f, REC_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, REC_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    // Painel corporativo atras do balcao da recepcao
    TerreoCaixa((Vector3){ -3.45f, 1.55f, REC_MIN_Z + 0.03f },
                (Vector3){ 3.55f, 2.35f, 0.05f }, 0.20f, 0.16f, 0.16f, 0.08f);
    TerreoCaixa((Vector3){ -3.45f, 2.15f, REC_MIN_Z + 0.07f },
                (Vector3){ 1.55f, 0.26f, 0.03f }, 0.80f, 0.72f, 0.64f, 0.18f);

    // Janela panoramica na parede do fundo (entre o balcao e a escada) com arvore e jardim visiveis!
    {
        float wx = 0.70f, wy = 1.75f, ww = 3.10f, wh = 1.60f;
        // Fundo externo claro (ceu do jardim, contido atras da janela sem vazar acima da parede)
        TerreoCaixa((Vector3){ wx, wy, REC_MIN_Z - 1.35f }, (Vector3){ ww + 0.50f, wh + 0.40f, 0.06f },
                    0.78f, 0.84f, 0.72f, -1.0f);
        // Gramado e mureta baixa do jardim
        TerreoCaixa((Vector3){ wx, wy - 0.75f, REC_MIN_Z - 0.80f }, (Vector3){ ww + 0.90f, 0.36f, 0.95f },
                    0.26f, 0.20f, 0.20f, -1.0f);
        // Arvore principal bem visivel no centro-esquerda da janela (tronco, galhos e copa em camadas)
        DrawCylinder((Vector3){ wx - 0.35f, wy - 0.62f, REC_MIN_Z - 0.68f }, 0.08f, 0.12f, 1.18f, 10, TerreoLum(0.14f));
        DrawCylinderEx((Vector3){ wx - 0.35f, wy + 0.08f, REC_MIN_Z - 0.68f },
                       (Vector3){ wx - 0.68f, wy + 0.42f, REC_MIN_Z - 0.65f }, 0.05f, 0.03f, 8, TerreoLum(0.14f));
        DrawCylinderEx((Vector3){ wx - 0.35f, wy + 0.14f, REC_MIN_Z - 0.68f },
                       (Vector3){ wx + 0.02f, wy + 0.46f, REC_MIN_Z - 0.66f }, 0.05f, 0.03f, 8, TerreoLum(0.14f));
        DrawSphereEx((Vector3){ wx - 0.35f, wy + 0.56f, REC_MIN_Z - 0.68f }, 0.46f, 10, 12, TerreoLum(0.24f));
        DrawSphereEx((Vector3){ wx - 0.68f, wy + 0.38f, REC_MIN_Z - 0.64f }, 0.34f, 9, 10, TerreoLum(0.30f));
        DrawSphereEx((Vector3){ wx + 0.04f, wy + 0.42f, REC_MIN_Z - 0.66f }, 0.36f, 9, 10, TerreoLum(0.32f));
        // Arbustos ao lado da arvore
        DrawSphereEx((Vector3){ wx + 0.78f, wy - 0.36f, REC_MIN_Z - 0.62f }, 0.34f, 8, 10, TerreoLum(0.22f));
        DrawSphereEx((Vector3){ wx + 1.12f, wy - 0.42f, REC_MIN_Z - 0.60f }, 0.26f, 8, 10, TerreoLum(0.28f));

        // Moldura escura da janela + divisorias (montantes)
        TerreoCaixa((Vector3){ wx, wy + wh * 0.5f, REC_MIN_Z + 0.04f }, (Vector3){ ww + 0.14f, 0.09f, 0.14f }, 0.24f, 0.18f, 0.18f, 0.08f);
        TerreoCaixa((Vector3){ wx, wy - wh * 0.5f, REC_MIN_Z + 0.04f }, (Vector3){ ww + 0.18f, 0.10f, 0.18f }, 0.68f, 0.52f, 0.46f, 0.12f);
        TerreoCaixa((Vector3){ wx - ww * 0.5f, wy, REC_MIN_Z + 0.04f }, (Vector3){ 0.09f, wh, 0.14f }, 0.24f, 0.18f, 0.18f, 0.08f);
        TerreoCaixa((Vector3){ wx + ww * 0.5f, wy, REC_MIN_Z + 0.04f }, (Vector3){ 0.09f, wh, 0.14f }, 0.24f, 0.18f, 0.18f, 0.08f);
        TerreoCaixa((Vector3){ wx - ww * 0.17f, wy, REC_MIN_Z + 0.04f }, (Vector3){ 0.05f, wh, 0.08f }, 0.22f, 0.16f, 0.16f, -1.0f);
        TerreoCaixa((Vector3){ wx + ww * 0.17f, wy, REC_MIN_Z + 0.04f }, (Vector3){ 0.05f, wh, 0.08f }, 0.22f, 0.16f, 0.16f, -1.0f);
        // Brilho sutil no vidro
        DrawLine3D((Vector3){ wx - 1.25f, wy - 0.50f, REC_MIN_Z + 0.06f }, (Vector3){ wx - 0.85f, wy + 0.55f, REC_MIN_Z + 0.06f }, TerreoLum(0.84f));
        DrawLine3D((Vector3){ wx + 0.25f, wy - 0.40f, REC_MIN_Z + 0.06f }, (Vector3){ wx + 0.65f, wy + 0.50f, REC_MIN_Z + 0.06f }, TerreoLum(0.84f));
    }

    // Porta de vidro transparente a esquerda (Lanchonete) e porta comum no canto direito inferior (Setor Adm.)
    TerreoPortaModelo((Vector3){ REC_MIN_X + 0.04f, 0.0f, 0.45f },  90.0f, PORTA_TIPO_VIDRO);
    TerreoPortaModelo((Vector3){ REC_MAX_X - 0.04f, 0.0f, 1.35f }, -90.0f, PORTA_TIPO_ESCRITORIO);

    // BALCAO GRANDE EM U OCO conectado a parede do fundo (canto superior esquerdo)
    // Bracos laterais ligados ate z = -4.20 + arco frontal oco para a cadeira e recepcionista!
    TerreoCaixa((Vector3){ -4.95f, 0.52f, -3.20f }, (Vector3){ 0.64f, 1.04f, 1.96f }, 0.46f, 0.34f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ -1.95f, 0.52f, -3.20f }, (Vector3){ 0.64f, 1.04f, 1.96f }, 0.46f, 0.34f, 0.30f, 0.12f);
    // Arco frontal oco e perfeitamente liso (24 segmentos continuos sem caixas quebradas)
    {
        const int segs = 24;
        const float cxU = -3.45f, czU = -2.22f;
        const float rxIn = 1.24f, rzIn = 0.62f;
        const float rxOut = 1.80f, rzOut = 1.06f;
        const float hBase = 1.04f, hTop = 1.10f;
        Color cOut  = TerreoLum(0.42f);
        Color cIn   = TerreoLum(0.28f);
        Color cTop  = TerreoLum(0.84f);
        Color cEdge = TerreoLum(0.58f);

        for (int i = 0; i < segs; i++) {
            float a0 = ((float)i / (float)segs) * PI;
            float a1 = ((float)(i + 1) / (float)segs) * PI;
            float c0 = cosf(a0), s0 = sinf(a0);
            float c1 = cosf(a1), s1 = sinf(a1);

            Vector3 o0B = { cxU + c0 * rxOut, 0.0f,  czU + s0 * rzOut };
            Vector3 o1B = { cxU + c1 * rxOut, 0.0f,  czU + s1 * rzOut };
            Vector3 o0T = { cxU + c0 * rxOut, hTop,  czU + s0 * rzOut };
            Vector3 o1T = { cxU + c1 * rxOut, hTop,  czU + s1 * rzOut };

            Vector3 i0B = { cxU + c0 * rxIn,  0.0f,  czU + s0 * rzIn };
            Vector3 i1B = { cxU + c1 * rxIn,  0.0f,  czU + s1 * rzIn };
            Vector3 i0T = { cxU + c0 * rxIn,  hTop,  czU + s0 * rzIn };
            Vector3 i1T = { cxU + c1 * rxIn,  hTop,  czU + s1 * rzIn };

            // Parede externa do arco (winding anti-horario para fora +Z)
            DrawTriangle3D(o0B, o1T, o1B, cOut);
            DrawTriangle3D(o0B, o0T, o1T, cOut);
            // Parede interna do arco (winding anti-horario para dentro -Z)
            DrawTriangle3D(i0B, i1B, i1T, cIn);
            DrawTriangle3D(i0B, i1T, i0T, cIn);
            // Tampo superior arredondado (winding anti-horario para cima +Y)
            DrawTriangle3D(i0T, o1T, o0T, cTop);
            DrawTriangle3D(i0T, i1T, o1T, cTop);
            // Friso sutil na borda do tampo
            DrawLine3D((Vector3){ o0T.x, hBase, o0T.z }, (Vector3){ o1T.x, hBase, o1T.z }, cEdge);
        }
    }
    // Tampos superiores dos bracos laterais do U
    TerreoCaixa((Vector3){ -4.95f, 1.07f, -3.20f }, (Vector3){ 0.72f, 0.06f, 2.00f }, 0.84f, 0.64f, 0.56f, 0.14f);
    TerreoCaixa((Vector3){ -1.95f, 1.07f, -3.20f }, (Vector3){ 0.72f, 0.06f, 2.00f }, 0.84f, 0.64f, 0.56f, 0.14f);

    // Monitor, livro de registros, cadeira e recepcionista sentada dentro do U
    TerreoMonitor((Vector3){ -4.10f, 1.10f, -1.65f }, 18.0f);
    TerreoCaixa((Vector3){ -2.80f, 1.12f, -1.60f }, (Vector3){ 0.40f, 0.04f, 0.28f }, 0.92f, 0.76f, 0.70f, 0.18f);
    TerreoCadeira((Vector3){ -3.45f, 0.0f, -3.05f }, 8.0f);
    TerreoSilhuetaSombra((Vector3){ -3.45f, 0.0f, -3.00f }, 8.0f, true);

    // Elevador no canto superior direito
    TerreoElevador((Vector3){ 4.35f, 0.0f, REC_MIN_Z + 0.04f }, 0.0f);

    // Area de espera central: SOFA CLARO BEM DEFINIDO (almofadas + bracos + encosto + base escura)
    // Base de madeira escura
    TerreoCaixa((Vector3){ 0.65f, 0.10f, 0.80f }, (Vector3){ 2.84f, 0.16f, 0.84f }, 0.22f, 0.16f, 0.14f, 0.08f);
    // 3 almofadas de assento bem separadas e claras (0.90)
    for (int a = -1; a <= 1; a++) {
        TerreoCaixa((Vector3){ 0.65f + a * 0.86f, 0.30f, 0.84f }, (Vector3){ 0.82f, 0.22f, 0.72f }, 0.90f, 0.74f, 0.68f, 0.22f);
        TerreoCaixa((Vector3){ 0.65f + a * 0.86f, 0.62f, 0.50f }, (Vector3){ 0.82f, 0.42f, 0.18f }, 0.86f, 0.70f, 0.64f, 0.22f);
    }
    // Bracos largos do sofa
    TerreoCaixa((Vector3){ -0.74f, 0.42f, 0.80f }, (Vector3){ 0.22f, 0.46f, 0.84f }, 0.84f, 0.68f, 0.62f, 0.20f);
    TerreoCaixa((Vector3){  2.04f, 0.42f, 0.80f }, (Vector3){ 0.22f, 0.46f, 0.84f }, 0.84f, 0.68f, 0.62f, 0.20f);

    DesenharPoltronaClara((Vector3){ -1.48f, 0.0f, 1.88f },  72.0f);
    DesenharPoltronaClara((Vector3){  2.78f, 0.0f, 1.88f }, -72.0f);

    // Mesinha de centro redonda com arranjo floral bem visivel
    TerreoMesaRedonda((Vector3){ 0.65f, 0.0f, 1.92f }, 0.58f, 0.44f);
    DrawCylinder((Vector3){ 0.65f, 0.44f, 1.92f }, 0.11f, 0.08f, 0.20f, 12, TerreoLum(0.92f));
    DrawSphereEx((Vector3){ 0.65f, 0.72f, 1.92f }, 0.16f, 8, 10, TerreoLum(0.42f));
    DrawSphereEx((Vector3){ 0.57f, 0.78f, 1.96f }, 0.07f, 6, 8, TerreoLum(0.94f));
    DrawSphereEx((Vector3){ 0.73f, 0.76f, 1.88f }, 0.07f, 6, 8, TerreoLum(0.94f));

    // Vaso grande no canto
    DesenharVasoPlantas((Vector3){ -4.95f, 0.0f, 3.15f }, 1.0f);
}

static const TerreoSalaDef SALA_RECEPCAO_DEF = {
    .id = TERREO_SALA_RECEPCAO,
    .camera = {
        .position   = { 0.35f, 5.95f, 9.10f },
        .target     = { 0.00f, 0.75f, -0.25f },
        .up         = { 0.00f, 1.00f,  0.00f },
        .fovy       = 40.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { REC_MIN_X, REC_MIN_Z, REC_MAX_X - REC_MIN_X, REC_MAX_Z - REC_MIN_Z },
    .bgColor          = { 24, 22, 24, 255 },
    .defaultSpawnPos  = { 0.65f, 0.0f, 2.85f },
    .defaultSpawnYaw  = PI,
    .colliders        = REC_COLLIDERS,
    .numColliders     = (int)(sizeof(REC_COLLIDERS) / sizeof(REC_COLLIDERS[0])),
    .interactables    = REC_INTERACTABLES,
    .numInteractables = (int)(sizeof(REC_INTERACTABLES) / sizeof(REC_INTERACTABLES[0])),
    .portas           = REC_PORTAS,
    .numPortas        = (int)(sizeof(REC_PORTAS) / sizeof(REC_PORTAS[0])),
    .drawFunc         = DesenharRecepcao
};

const TerreoSalaDef *SalaRecepcaoDef(void) {
    return &SALA_RECEPCAO_DEF;
}
