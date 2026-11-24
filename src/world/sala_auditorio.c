#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Auditorio (4o Andar, baseado em auditorio.zip / IndoorEnvironmentHDRI017):
// - Mesas brancas em U / fileiras com cadeiras escuras voltadas para o telao de projecao ao fundo
// - Placas acusticas suspensas e pulpito de apresentacao
#define AUD_MIN_X -6.00f
#define AUD_MAX_X 6.00f
#define AUD_MIN_Z -4.20f
#define AUD_MAX_Z 4.00f
#define AUD_WALL_H 3.40f

static const PlayerColliderBox AUD_COLLIDERS[] = {
    { -3.80f, -1.65f,  3.80f, -0.55f }, // Fileira frontal de mesas em U
    { -3.80f,  0.45f,  3.80f,  1.65f }, // Fileira traseira de mesas
    { -2.65f, -3.55f, -1.55f, -2.65f }, // Pulpito
};

static const SalaInteractable AUD_INTERACTABLES[] = {
    {
        "telao_auditorio",
        "Telão de Projeção",
        { 0.0f, 1.85f, -4.05f },
        { 3.80f, 2.00f, 0.45f },
        2.35f,
        false,
        "O telão principal exibe o logotipo corporativo da RASEC em modo de espera."
    },
    {
        "pulpito_auditorio",
        "Púlpito de Apresentação",
        { -2.10f, 0.90f, -3.10f },
        { 1.15f, 1.35f, 0.95f },
        1.85f,
        true,
        "Um púlpito com microfone usado nas apresentações trimestrais da diretoria."
    },
    {
        "mesas_auditorio",
        "Bancadas do Auditório",
        { 0.0f, 0.70f, 1.05f },
        { 6.80f, 1.00f, 1.15f },
        2.15f,
        false,
        "Fileiras curvas de mesas claras e cadeiras escuras com acústica tratada."
    }
};

static const SalaPorta AUD_PORTAS[] = {
    {
        TERREO_DEST_HALL_4,
        "Porta (Recepção 4º Andar)",
        { { 5.75f, 0.0f, -0.25f }, { 6.20f, 2.30f, 1.15f } },
        { 5.80f, 0.0f, 0.45f },
        1.40f,
        { 5.50f, -0.20f, 0.55f, 1.30f }
    }
};

static void DesenharAuditorio(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = AUD_MAX_X - AUD_MIN_X;
    float d = AUD_MAX_Z - AUD_MIN_Z;
    float cx = (AUD_MIN_X + AUD_MAX_X) * 0.5f;
    float cz = (AUD_MIN_Z + AUD_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.52f));
    Color corGrid = TerreoLum(0.40f);
    for (float x = -5.0f; x <= 5.01f; x += 1.0f) {
        DrawLine3D((Vector3){ x, 0.004f, AUD_MIN_Z }, (Vector3){ x, 0.004f, AUD_MAX_Z }, corGrid);
    }
    for (float z = -4.0f; z <= 4.01f; z += 1.0f) {
        DrawLine3D((Vector3){ AUD_MIN_X, 0.004f, z }, (Vector3){ AUD_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, AUD_WALL_H * 0.5f, AUD_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, AUD_WALL_H, 0.16f }, 0.34f, 0.42f, 0.30f, 0.12f);
    TerreoCaixa((Vector3){ AUD_MIN_X - 0.08f, AUD_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, AUD_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);
    TerreoCaixa((Vector3){ AUD_MAX_X + 0.08f, AUD_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, AUD_WALL_H, d }, 0.32f, 0.36f, 0.38f, 0.12f);

    TerreoPortaModelo((Vector3){ AUD_MAX_X - 0.04f, 0.0f, 0.45f }, -90.0f, PORTA_TIPO_VIDRO);

    // Telao de projecao ao fundo (auditorio.zip)
    TerreoCaixa((Vector3){ 0.0f, 1.95f, AUD_MIN_Z + 0.06f }, (Vector3){ 3.60f, 1.90f, 0.06f }, 0.92f, 0.88f, 0.80f, 0.16f);
    // Pulpito
    TerreoCaixa((Vector3){ -2.10f, 0.58f, -3.10f }, (Vector3){ 0.72f, 1.16f, 0.58f }, 0.78f, 0.68f, 0.60f, 0.16f);

    // 2 fileiras de mesas claras e cadeiras escuras
    float zFilas[2] = { -1.10f, 1.05f };
    for (int f = 0; f < 2; f++) {
        float zf = zFilas[f];
        TerreoCaixa((Vector3){ 0.0f, 0.74f, zf }, (Vector3){ 7.20f, 0.05f, 0.75f }, 0.82f, 0.72f, 0.64f, 0.16f);
        for (int c = -2; c <= 2; c++) {
            TerreoCadeira((Vector3){ c * 1.45f, 0.0f, zf + 0.55f }, 180.0f);
        }
    }
}

static const TerreoSalaDef SALA_AUDITORIO_DEF = {
    .id = TERREO_SALA_AUDITORIO,
    .camera = {
        .position   = { 0.0f, 6.05f, 8.80f },
        .target     = { 0.0f, 0.85f, -0.30f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 40.5f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { AUD_MIN_X, AUD_MIN_Z, AUD_MAX_X - AUD_MIN_X, AUD_MAX_Z - AUD_MIN_Z },
    .bgColor          = { 22, 20, 22, 255 },
    .defaultSpawnPos  = { 4.95f, 0.0f, 0.45f },
    .defaultSpawnYaw  = -PI * 0.5f,
    .colliders        = AUD_COLLIDERS,
    .numColliders     = (int)(sizeof(AUD_COLLIDERS) / sizeof(AUD_COLLIDERS[0])),
    .interactables    = AUD_INTERACTABLES,
    .numInteractables = (int)(sizeof(AUD_INTERACTABLES) / sizeof(AUD_INTERACTABLES[0])),
    .portas           = AUD_PORTAS,
    .numPortas        = (int)(sizeof(AUD_PORTAS) / sizeof(AUD_PORTAS[0])),
    .drawFunc         = DesenharAuditorio
};

const TerreoSalaDef *SalaAuditorioDef(void) {
    return &SALA_AUDITORIO_DEF;
}
