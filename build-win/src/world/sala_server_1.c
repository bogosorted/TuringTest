#include "terreo_3d.h"
#include "rlgl.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Sala de Servidores 1 (baseada em serverciberseguranca.jpg):
// - Corredor de racks pretos perfurados com cabos no teto e LEDs
// - Embaixo: volta para Ciberseguranca 2 | Direita: Sala de Servidores 2
static const float SRV1_MIN_X = -4.60f;
static const float SRV1_MAX_X =  4.60f;
static const float SRV1_MIN_Z = -3.60f;
static const float SRV1_MAX_Z =  3.40f;
static const float SRV1_WALL_H = 3.05f;

static const PlayerColliderBox SRV1_COLLIDERS[] = {
    { -3.75f, -2.55f, -2.45f,  2.15f }, // Fileira esquerda de racks
    {  1.55f, -2.55f,  2.85f,  2.15f }, // Fileira direita de racks
};

static const SalaInteractable SRV1_INTERACTABLES[] = {
    {
        "racks_server_1",
        "Racks de Servidores (Ala 1)",
        { -3.10f, 1.15f, -0.20f },
        { 1.25f, 2.20f, 4.50f },
        2.15f,
        true,
        "Fileiras de racks pretos perfurados zumbem sob refrigeração constante. LEDs piscam atrás das grades."
    },
    {
        "cabos_server_1",
        "Eletrocalha de Dados",
        { 0.20f, 1.15f, -3.45f },
        { 2.00f, 1.80f, 0.50f },
        1.85f,
        false,
        "Feixes grossos de fibra óptica sobem pelas eletrocalhas conectando os racks principais."
    }
};

static const SalaPorta SRV1_PORTAS[] = {
    {
        TERREO_DEST_CIBER_2,
        "Porta (Cibersegurança 2)",
        { { -0.70f, 0.0f, 3.15f }, { 0.70f, 2.30f, 3.55f } },
        { 0.00f, 0.0f, 3.15f },
        1.40f,
        { -0.65f, 2.90f, 1.30f, 0.58f }
    },
    {
        TERREO_DEST_SERVER_2,
        "Porta (Sala de Servidores 2)",
        { { 4.35f, 0.0f, -0.70f }, { 4.85f, 2.30f, 0.70f } },
        { 4.40f, 0.0f, 0.00f },
        1.40f,
        { 4.10f, -0.65f, 0.55f, 1.30f }
    }
};

static void DesenharServer1(float tempo, bool evRelatoriosAtiva) {
    (void)tempo;
    (void)evRelatoriosAtiva;
    float w = SRV1_MAX_X - SRV1_MIN_X;
    float d = SRV1_MAX_Z - SRV1_MIN_Z;
    float cx = (SRV1_MIN_X + SRV1_MAX_X) * 0.5f;
    float cz = (SRV1_MIN_Z + SRV1_MAX_Z) * 0.5f;

    DrawPlane((Vector3){ cx, 0.0f, cz }, (Vector2){ w, d }, TerreoLum(0.46f));
    Color corGrid = TerreoLum(0.34f);
    for (float x = -4.0f; x <= 4.01f; x += 0.8f) {
        DrawLine3D((Vector3){ x, 0.004f, SRV1_MIN_Z }, (Vector3){ x, 0.004f, SRV1_MAX_Z }, corGrid);
    }
    for (float z = -3.0f; z <= 3.01f; z += 0.8f) {
        DrawLine3D((Vector3){ SRV1_MIN_X, 0.004f, z }, (Vector3){ SRV1_MAX_X, 0.004f, z }, corGrid);
    }

    TerreoCaixa((Vector3){ cx, SRV1_WALL_H * 0.5f, SRV1_MIN_Z - 0.08f },
                (Vector3){ w + 0.32f, SRV1_WALL_H, 0.16f }, 0.30f, 0.36f, 0.26f, 0.10f);
    TerreoCaixa((Vector3){ SRV1_MIN_X - 0.08f, SRV1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SRV1_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.10f);
    TerreoCaixa((Vector3){ SRV1_MAX_X + 0.08f, SRV1_WALL_H * 0.5f, cz },
                (Vector3){ 0.16f, SRV1_WALL_H, d }, 0.28f, 0.32f, 0.34f, 0.10f);

    TerreoPortaModelo((Vector3){ 0.0f, 0.0f, SRV1_MAX_Z - 0.04f }, 180.0f, PORTA_TIPO_INDUSTRIAL);
    TerreoPortaModelo((Vector3){ SRV1_MAX_X - 0.04f, 0.0f, 0.00f }, -90.0f, PORTA_TIPO_INDUSTRIAL);

    // Eletrocalhas no alto sobre as fileiras de racks (serverciberseguranca.jpg)
    TerreoCaixa((Vector3){ -3.10f, 2.65f, -0.20f }, (Vector3){ 0.70f, 0.08f, 4.80f }, 0.38f, 0.30f, 0.26f, 0.14f);
    TerreoCaixa((Vector3){  2.20f, 2.65f, -0.20f }, (Vector3){ 0.70f, 0.08f, 4.80f }, 0.38f, 0.30f, 0.26f, 0.14f);

    // Fileiras de racks de servidor
    for (int i = 0; i < 4; i++) {
        float rz = -1.95f + i * 1.15f;
        TerreoRackServidor((Vector3){ -3.10f, 0.0f, rz },  90.0f);
        TerreoRackServidor((Vector3){  2.20f, 0.0f, rz }, -90.0f);
    }
}

static const TerreoSalaDef SALA_SERVER_1_DEF = {
    .id = TERREO_SALA_SERVER_1,
    .camera = {
        .position   = { 0.0f, 5.45f, 8.10f },
        .target     = { 0.0f, 0.85f, -0.20f },
        .up         = { 0.0f, 1.00f,  0.00f },
        .fovy       = 39.0f,
        .projection = CAMERA_PERSPECTIVE
    },
    .boundsXZ         = { SRV1_MIN_X, SRV1_MIN_Z, SRV1_MAX_X - SRV1_MIN_X, SRV1_MAX_Z - SRV1_MIN_Z },
    .bgColor          = { 18, 16, 18, 255 },
    .defaultSpawnPos  = { 0.0f, 0.0f, 2.65f },
    .defaultSpawnYaw  = PI,
    .colliders        = SRV1_COLLIDERS,
    .numColliders     = (int)(sizeof(SRV1_COLLIDERS) / sizeof(SRV1_COLLIDERS[0])),
    .interactables    = SRV1_INTERACTABLES,
    .numInteractables = (int)(sizeof(SRV1_INTERACTABLES) / sizeof(SRV1_INTERACTABLES[0])),
    .portas           = SRV1_PORTAS,
    .numPortas        = (int)(sizeof(SRV1_PORTAS) / sizeof(SRV1_PORTAS[0])),
    .drawFunc         = DesenharServer1
};

const TerreoSalaDef *SalaServer1Def(void) {
    return &SALA_SERVER_1_DEF;
}
