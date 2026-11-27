#include "terreo_3d.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Sala das Cameras (ao lado do Arquivo, 1o Andar):
// Sala secreta de monitoramento CFTV, baixa, escura e claustrofobica.
#define CAM_MIN_X -3.40f
#define CAM_MAX_X 3.25f
#define CAM_MIN_Z -2.75f
#define CAM_MAX_Z 2.45f
#define CAM_WALL_H 2.60f

static const PlayerColliderBox CAM_COLLIDERS[] = {
    { -2.45f, -2.55f,  2.45f, -1.35f }, // Bancada principal de monitoramento
    { -3.35f, -2.65f, -2.45f, -0.55f }, // Ala esquerda (armario de fitas + painel)
    {  2.40f, -2.65f,  3.20f, -0.45f }, // Ala direita (racks DVR + torre de controle)
    { -0.42f, -1.02f,  0.42f, -0.38f }, // Cadeira do operador
};

static const SalaPorta CAM_PORTAS[] = {
    {
        TERREO_DEST_ARQUIVO_1,
        "Porta (Arquivo)",
        { { -3.55f, 0.0f, 0.80f }, { -3.05f, 2.30f, 2.20f } },
        { -3.05f, 0.0f, 1.50f },
        1.40f,
        { -3.45f, 0.85f, 0.55f, 1.30f }
    }
};

// Desenha um monitor CFTV industrial com moldura profunda, sinal REC piscante e feed estilizado
static void DesenharMonitorCFTV(Vector3 pos, float rotYDeg, float w, float h, int camIdx, float tempo) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotYDeg, 0.0f, 1.0f, 0.0f);

    // Carcaca CRT / industrial profunda e escura
    TerreoCaixa((Vector3){ 0.0f, 0.0f, -0.10f }, (Vector3){ w, h, 0.20f }, 0.14f, 0.10f, 0.11f, 0.22f);
    // Moldura frontal saliente (efeito tubo/recesso)
    TerreoCaixa((Vector3){ 0.0f, 0.0f, 0.01f }, (Vector3){ w * 0.96f, h * 0.94f, 0.03f }, 0.08f, 0.06f, 0.07f, 0.18f);

    // Brilho base da tela com leve cintilacao de tubo
    float flicker = 0.04f * sinf(tempo * 17.0f + (float)camIdx * 2.1f)
                  + 0.03f * sinf(tempo * 3.1f + (float)camIdx * 1.4f);
    float lumTela = 0.68f + flicker;
    float sw = w * 0.84f;
    float sh = h * 0.78f;
    TerreoCaixa((Vector3){ 0.0f, 0.01f, 0.026f }, (Vector3){ sw, sh, 0.01f }, lumTela, lumTela, lumTela, -1.0f);

    // Conteudo do feed CFTV (linhas de perspectiva / silhuetas / grade)
    int tipo = camIdx % 5;
    if (tipo == 0) {
        // Monitor central: divisao 2x2 (quad-view de 4 cameras)
        TerreoCaixa((Vector3){ 0.0f, 0.01f, 0.033f }, (Vector3){ sw, 0.018f, 0.004f }, 0.10f, 0.10f, 0.10f, -1.0f);
        TerreoCaixa((Vector3){ 0.0f, 0.01f, 0.033f }, (Vector3){ 0.018f, sh, 0.004f }, 0.10f, 0.10f, 0.10f, -1.0f);
        // Pequenas silhuetas/elementos em cada quadrante
        TerreoCaixa((Vector3){ -sw * 0.25f,  sh * 0.20f, 0.034f }, (Vector3){ sw * 0.22f, sh * 0.18f, 0.004f }, 0.28f, 0.28f, 0.28f, -1.0f);
        TerreoCaixa((Vector3){  sw * 0.22f,  sh * 0.18f, 0.034f }, (Vector3){ sw * 0.12f, sh * 0.28f, 0.004f }, 0.22f, 0.22f, 0.22f, -1.0f);
        TerreoCaixa((Vector3){ -sw * 0.22f, -sh * 0.20f, 0.034f }, (Vector3){ sw * 0.30f, sh * 0.10f, 0.004f }, 0.32f, 0.32f, 0.32f, -1.0f);
        TerreoCaixa((Vector3){  sw * 0.26f, -sh * 0.18f, 0.034f }, (Vector3){ sw * 0.16f, sh * 0.22f, 0.004f }, 0.24f, 0.24f, 0.24f, -1.0f);
    } else if (tipo == 1) {
        // Feed de corredor: paredes em perspectiva e porta ao fundo
        TerreoCaixa((Vector3){ 0.0f, 0.02f, 0.033f }, (Vector3){ sw * 0.30f, sh * 0.56f, 0.004f }, 0.24f, 0.24f, 0.24f, -1.0f);
        TerreoCaixa((Vector3){ -sw * 0.30f, 0.01f, 0.033f }, (Vector3){ sw * 0.18f, sh * 0.80f, 0.004f }, 0.42f, 0.42f, 0.42f, -1.0f);
        TerreoCaixa((Vector3){  sw * 0.30f, 0.01f, 0.033f }, (Vector3){ sw * 0.18f, sh * 0.80f, 0.004f }, 0.42f, 0.42f, 0.42f, -1.0f);
    } else if (tipo == 2) {
        // Feed de sala/escritorio: mesa + vulto
        TerreoCaixa((Vector3){ 0.0f, -sh * 0.18f, 0.033f }, (Vector3){ sw * 0.58f, sh * 0.18f, 0.004f }, 0.26f, 0.26f, 0.26f, -1.0f);
        float vultoX = 0.08f * sinf(tempo * 0.7f + (float)camIdx);
        TerreoCaixa((Vector3){ vultoX * sw, 0.06f, 0.034f }, (Vector3){ sw * 0.14f, sh * 0.36f, 0.004f }, 0.16f, 0.16f, 0.16f, -1.0f);
    } else if (tipo == 3) {
        // Feed de servidores / arquivo: colunas verticais
        for (int k = -1; k <= 1; k++) {
            TerreoCaixa((Vector3){ k * sw * 0.26f, 0.0f, 0.033f }, (Vector3){ sw * 0.16f, sh * 0.74f, 0.004f }, 0.24f, 0.24f, 0.24f, -1.0f);
        }
    } else {
        // Feed com interferencia / barra de varredura passando
        float scanY = fmodf(tempo * 0.65f + (float)camIdx * 0.37f, 1.0f) - 0.5f;
        TerreoCaixa((Vector3){ 0.0f, scanY * sh * 0.80f, 0.034f }, (Vector3){ sw * 0.96f, sh * 0.14f, 0.004f }, 0.94f, 0.94f, 0.94f, -1.0f);
        TerreoCaixa((Vector3){ 0.0f, -scanY * sh * 0.55f, 0.034f }, (Vector3){ sw * 0.92f, sh * 0.07f, 0.004f }, 0.20f, 0.20f, 0.20f, -1.0f);
    }

    // Barra de timestamp escura no rodape da tela
    TerreoCaixa((Vector3){ 0.0f, -sh * 0.38f, 0.035f }, (Vector3){ sw * 0.92f, sh * 0.11f, 0.004f }, 0.12f, 0.12f, 0.12f, -1.0f);

    // Indicador REC piscando no canto superior direito
    bool recOn = fmodf(tempo + (float)camIdx * 0.35f, 1.2f) < 0.65f;
    float recLum = recOn ? 0.98f : 0.22f;
    TerreoCaixa((Vector3){ sw * 0.38f, sh * 0.36f, 0.036f }, (Vector3){ 0.045f, 0.045f, 0.006f }, recLum, recLum, recLum, -1.0f);

    // LED de energia na borda inferior da moldura
    TerreoCaixa((Vector3){ w * 0.36f, -h * 0.42f, 0.028f }, (Vector3){ 0.028f, 0.018f, 0.008f }, 0.90f, 0.90f, 0.90f, -1.0f);

    rlPopMatrix();
}

static void DesenharSalaCameras(float tempo, bool evRelatoriosAtiva) {
    (void)evRelatoriosAtiva;
    float w = CAM_MAX_X - CAM_MIN_X;
    float d = CAM_MAX_Z - CAM_MIN_Z;
    float cx = (CAM_MIN_X + CAM_MAX_X) * 0.5f;
    float cz = (CAM_MIN_Z + CAM_MAX_Z) * 0.5f;

    // Piso escuro antiestatico com grade apertada
    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.16f));
    Color corGrid = TerreoLum(0.10f);
    for (float x = CAM_MIN_X + 0.5f; x < CAM_MAX_X; x += 0.55f) {
        DrawLine3D((Vector3){ x, 0.004f, CAM_MIN_Z }, (Vector3){ x, 0.004f, CAM_MAX_Z }, corGrid);
    }
    for (float z = CAM_MIN_Z + 0.5f; z < CAM_MAX_Z; z += 0.55f) {
        DrawLine3D((Vector3){ CAM_MIN_X, 0.004f, z }, (Vector3){ CAM_MAX_X, 0.004f, z }, corGrid);
    }

    // Reflexo suave das telas no piso diante da bancada (efeito sala escura iluminada pelos monitores)
    DrawPlane((Vector3){ 0.0f, 0.006f, -0.65f }, (Vector2){ 3.80f, 1.50f }, TerreoLum(0.24f));

    // Paredes escuras e pesadas (vibe bunker de seguranca)
    TerreoCaixa((Vector3){ cx, CAM_WALL_H * 0.5f, CAM_MIN_Z - 0.10f },
                (Vector3){ w + 0.40f, CAM_WALL_H, 0.20f }, 0.14f, 0.16f, 0.12f, 0.06f);
    TerreoCaixa((Vector3){ CAM_MIN_X - 0.10f, CAM_WALL_H * 0.5f, cz },
                (Vector3){ 0.20f, CAM_WALL_H, d }, 0.13f, 0.15f, 0.16f, 0.06f);
    TerreoCaixa((Vector3){ CAM_MAX_X + 0.10f, CAM_WALL_H * 0.5f, cz },
                (Vector3){ 0.20f, CAM_WALL_H, d }, 0.13f, 0.15f, 0.16f, 0.06f);

    // Pilares estruturais nos cantos + vigas baixas no topo das paredes (sensacao de teto baixo)
    TerreoCaixa((Vector3){ CAM_MIN_X + 0.12f, CAM_WALL_H * 0.5f, CAM_MIN_Z + 0.12f },
                (Vector3){ 0.28f, CAM_WALL_H, 0.28f }, 0.20f, 0.18f, 0.16f, 0.08f);
    TerreoCaixa((Vector3){ CAM_MAX_X - 0.12f, CAM_WALL_H * 0.5f, CAM_MIN_Z + 0.12f },
                (Vector3){ 0.28f, CAM_WALL_H, 0.28f }, 0.20f, 0.18f, 0.16f, 0.08f);
    TerreoCaixa((Vector3){ cx, CAM_WALL_H - 0.12f, CAM_MIN_Z + 0.20f },
                (Vector3){ w, 0.24f, 0.40f }, 0.18f, 0.14f, 0.14f, 0.08f);
    TerreoCaixa((Vector3){ CAM_MIN_X + 0.18f, CAM_WALL_H - 0.12f, cz },
                (Vector3){ 0.36f, 0.24f, d }, 0.18f, 0.14f, 0.14f, 0.08f);
    TerreoCaixa((Vector3){ CAM_MAX_X - 0.18f, CAM_WALL_H - 0.12f, cz },
                (Vector3){ 0.36f, 0.24f, d }, 0.18f, 0.14f, 0.14f, 0.08f);

    // Duto de ventilacao pesado e eletrocalha de cabos cruzando o fundo
    DrawCylinderEx((Vector3){ CAM_MIN_X + 0.20f, 2.38f, CAM_MIN_Z + 0.22f },
                   (Vector3){ CAM_MAX_X - 0.20f, 2.38f, CAM_MIN_Z + 0.22f }, 0.12f, 0.12f, 10, TerreoLum(0.28f));
    TerreoCaixa((Vector3){ cx, 2.18f, CAM_MIN_Z + 0.16f }, (Vector3){ 4.80f, 0.08f, 0.22f }, 0.24f, 0.20f, 0.18f, 0.10f);

    // Feixes de cabos descendo da eletrocalha para os monitores e racks
    for (int i = 0; i < 6; i++) {
        float kx = -2.00f + i * 0.80f;
        DrawCylinderEx((Vector3){ kx, 2.18f, CAM_MIN_Z + 0.12f },
                       (Vector3){ kx, 0.78f, CAM_MIN_Z + 0.12f }, 0.025f, 0.025f, 6, TerreoLum(0.08f));
    }

    // Porta metalica industrial + leitor de cracha + luz de alerta acima da porta
    TerreoPortaModelo((Vector3){ CAM_MIN_X + 0.04f, 0.0f, 1.50f }, 90.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoCaixa((Vector3){ CAM_MIN_X + 0.05f, 1.25f, 0.78f }, (Vector3){ 0.06f, 0.18f, 0.11f }, 0.24f, 0.20f, 0.20f, 0.10f);
    TerreoCaixa((Vector3){ CAM_MIN_X + 0.08f, 1.29f, 0.78f }, (Vector3){ 0.02f, 0.03f, 0.03f }, 0.90f, 0.90f, 0.90f, -1.0f);
    float ledPorta = (fmodf(tempo, 2.0f) < 1.0f) ? 0.92f : 0.25f;
    TerreoCaixa((Vector3){ CAM_MIN_X + 0.10f, 2.34f, 1.50f }, (Vector3){ 0.10f, 0.09f, 0.18f }, ledPorta, ledPorta, ledPorta, -1.0f);

    // Camera dome/fixa de vigilancia interna no canto superior direito vigiando a propria sala
    TerreoCaixa((Vector3){ CAM_MAX_X - 0.16f, 2.28f, CAM_MIN_Z + 0.35f }, (Vector3){ 0.16f, 0.12f, 0.26f }, 0.28f, 0.22f, 0.24f, 0.10f);
    TerreoCaixa((Vector3){ CAM_MAX_X - 0.22f, 2.24f, CAM_MIN_Z + 0.48f }, (Vector3){ 0.08f, 0.08f, 0.08f }, 0.88f, 0.88f, 0.88f, -1.0f);

    // Bancada central de controle em U + cadeira do operador
    TerreoCaixa((Vector3){ 0.0f, 0.74f, -1.85f }, (Vector3){ 4.65f, 0.07f, 0.95f }, 0.34f, 0.24f, 0.22f, 0.12f);
    TerreoCaixa((Vector3){ 0.0f, 0.37f, -2.26f }, (Vector3){ 4.50f, 0.72f, 0.10f }, 0.20f, 0.15f, 0.14f, 0.08f);
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCylinder((Vector3){ sx * 2.15f, 0.0f, -1.45f }, 0.045f, 0.045f, 0.72f, 8, TerreoLum(0.14f));
    }

    // Console mesa: teclado, mesa de corte CFTV com botoes luminosos e joystick PTZ
    TerreoCaixa((Vector3){ 0.0f, 0.79f, -1.56f }, (Vector3){ 1.10f, 0.04f, 0.34f }, 0.22f, 0.18f, 0.16f, 0.30f);
    for (int b = 0; b < 6; b++) {
        float bx = -0.38f + b * 0.11f;
        float bLum = (fmodf(tempo * 1.8f + (float)b * 0.4f, 1.8f) < 0.9f) ? 0.92f : 0.35f;
        TerreoCaixa((Vector3){ bx, 0.815f, -1.52f }, (Vector3){ 0.055f, 0.015f, 0.055f }, bLum, bLum, bLum, -1.0f);
    }
    // Joystick PTZ na mesa a direita do teclado
    TerreoCaixa((Vector3){ 0.40f, 0.815f, -1.54f }, (Vector3){ 0.14f, 0.03f, 0.14f }, 0.26f, 0.20f, 0.20f, 0.12f);
    DrawCylinder((Vector3){ 0.40f, 0.83f, -1.54f }, 0.016f, 0.016f, 0.11f, 8, TerreoLum(0.42f));
    DrawSphereEx((Vector3){ 0.40f, 0.95f, -1.54f }, 0.030f, 6, 8, TerreoLum(0.18f));

    TerreoCadeiraConfortavel((Vector3){ 0.0f, 0.0f, -0.70f }, 180.0f);

    // Ala direita: 2 Racks DVR/NVR altos apertando o espaco + torre de gravacao
    TerreoRackServidor((Vector3){ 2.78f, 0.0f, -2.15f }, -90.0f);
    TerreoRackServidor((Vector3){ 2.78f, 0.0f, -1.05f }, -90.0f);

    // Ala esquerda: Armario metalico de fitas/HDs de backup + quadro eletrico
    TerreoArmario((Vector3){ -2.92f, 0.0f, -1.95f }, 90.0f, 1.15f, 2.05f, 0.56f);
    TerreoCaixa((Vector3){ -2.92f, 1.35f, -0.95f }, (Vector3){ 0.48f, 0.82f, 0.58f }, 0.26f, 0.22f, 0.20f, 0.12f);
    TerreoCaixa((Vector3){ -2.67f, 1.52f, -0.95f }, (Vector3){ 0.02f, 0.06f, 0.06f }, 0.94f, 0.94f, 0.94f, -1.0f);

    // Gabinetes/gravadores nas pontas da bancada
    TerreoGabinetePC((Vector3){ -2.05f, 0.775f, -1.95f }, 0.0f);
    TerreoGabinetePC((Vector3){  2.05f, 0.775f, -1.95f }, 0.0f);

    // Fileira inferior de monitores CRT inclinados sobre a bancada (5 monitores em arco fechado)
    DesenharMonitorCFTV((Vector3){ -1.55f, 1.01f, -1.96f },  14.0f, 0.62f, 0.44f, 1, tempo);
    DesenharMonitorCFTV((Vector3){ -0.82f, 1.01f, -2.06f },   6.0f, 0.64f, 0.44f, 2, tempo);
    DesenharMonitorCFTV((Vector3){  0.00f, 1.03f, -2.10f },   0.0f, 0.78f, 0.48f, 0, tempo);
    DesenharMonitorCFTV((Vector3){  0.82f, 1.01f, -2.06f },  -6.0f, 0.64f, 0.44f, 3, tempo);
    DesenharMonitorCFTV((Vector3){  1.55f, 1.01f, -1.96f }, -14.0f, 0.62f, 0.44f, 4, tempo);

    // Parede de Video (Video Wall) no fundo: suporte metalico + 2 fileiras suspensas de monitores CFTV
    TerreoCaixa((Vector3){ 0.0f, 1.78f, -2.56f }, (Vector3){ 4.30f, 0.96f, 0.08f }, 0.12f, 0.08f, 0.08f, 0.20f);
    // Monitor mestre central grande (quad-view) + 4 monitores laterais em 2 niveis
    DesenharMonitorCFTV((Vector3){  0.00f, 1.80f, -2.42f }, 0.0f, 1.24f, 0.84f, 0, tempo);
    DesenharMonitorCFTV((Vector3){ -1.16f, 2.02f, -2.44f }, 5.0f, 0.84f, 0.44f, 1, tempo);
    DesenharMonitorCFTV((Vector3){ -1.16f, 1.54f, -2.44f }, 5.0f, 0.84f, 0.44f, 4, tempo);
    DesenharMonitorCFTV((Vector3){  1.16f, 2.02f, -2.44f }, -5.0f, 0.84f, 0.44f, 2, tempo);
    DesenharMonitorCFTV((Vector3){  1.16f, 1.54f, -2.44f }, -5.0f, 0.84f, 0.44f, 3, tempo);
    // Monitores auxiliares nas bordas extremas da parede de video
    DesenharMonitorCFTV((Vector3){ -1.94f, 1.78f, -2.38f },  12.0f, 0.56f, 0.72f, 3, tempo);
    DesenharMonitorCFTV((Vector3){  1.94f, 1.78f, -2.38f }, -12.0f, 0.56f, 0.72f, 1, tempo);
}

static const TerreoSalaDef SALA_CAMERAS_DEF = {
    .id = TERREO_SALA_CAMERAS,
    .camera = {
        .position   = { 0.0f, 4.35f, 6.05f },
        .target     = { 0.0f, 0.95f, -0.50f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 38.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { CAM_MIN_X, CAM_MIN_Z, CAM_MAX_X - CAM_MIN_X, CAM_MAX_Z - CAM_MIN_Z },
    .bgColor          = { 8, 7, 8, 255 },
    .defaultSpawnPos  = { -2.55f, 0.0f, 1.50f },
    .defaultSpawnYaw  = PI * 0.5f,
    .colliders        = CAM_COLLIDERS,
    .numColliders     = (int)(sizeof(CAM_COLLIDERS) / sizeof(CAM_COLLIDERS[0])),
    .interactables    = NULL,
    .numInteractables = 0,
    .portas           = CAM_PORTAS,
    .numPortas        = (int)(sizeof(CAM_PORTAS) / sizeof(CAM_PORTAS[0])),
    .drawFunc         = DesenharSalaCameras
};

const TerreoSalaDef *SalaCamerasDef(void) {
    return &SALA_CAMERAS_DEF;
}
