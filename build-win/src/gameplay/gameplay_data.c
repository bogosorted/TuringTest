#include "gameplay_data.h"
#include "game_state.h"
#include "settings.h"

const InfoLocal LOCAIS[NUM_LOCAIS] = {
    { "ADM. 1",          "SETOR ADMINISTRATIVO",   "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "CANTINA",         "LANCHONETE DA RASEC",    "",             "assets/sprites/locais/terreo/spr_background_cantina.png" },
    { "RH",              "RH DA RASEC",            "Vazio",        "assets/sprites/locais/1_andar/spr_background_rh.png" },
    { "CONTAB.",         "SETOR DE CONTABILIDADE", "Vazio",        "assets/sprites/locais/1_andar/spr_background_contabilidade.png" },
    { "AUDIT.",          "AUDITÓRIO",              "Vazio",        "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "COPA",            "COPA DO 4º ANDAR",       "Vazio",        "assets/sprites/locais/2_andar/spr_background_geladeira_coffe.png" },
    { "ESCADA",          "ESCADARIA - TÉRREO",     "Escadas",      FUNDO_ESCADA_MEIO },
    { "ESCADA",          "ESCADARIA - 1º ANDAR",   "Escadas",      FUNDO_ESCADA_MEIO },
    { "ESCADA",          "ESCADARIA - 2º ANDAR",   "Escadas",      FUNDO_ESCADA_MEIO },
    { "SUBSOLO",         "SUBSOLO - ENTRADA",      "Vazio",        "assets/sprites/locais/subsolo/local_comeco.png" },
    { "BIFURC.",         "SUBSOLO - ESCOLHA",      "Vazio",        "assets/sprites/locais/subsolo/local_escolha.png" },
    { "ÚLT. ESCR.",      "ÚLTIMO ESCRITÓRIO",      "Vazio",        "assets/sprites/locais/subsolo/last_office.png" },
    { "VEREDITO",        "SALA DO VEREDITO",       "Vazio",        "assets/sprites/veredito/spr_veredito_sala.png" },
    { "RECRE.",          "ÁREA DE RECREAÇÃO",      "Vazio",        "assets/sprites/locais/subsolo/area_recreacao_abandonada.png" },
    { "RECEPÇÃO",        "RECEPÇÃO DA RASEC",      "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "JARDIM",          "JARDIM / DESCANSO",      "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "ADM. 2",          "SETOR ADM. - ALA ESQ.",  "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "HALL 1º",         "1º ANDAR - CONTABILIDADE","",            "assets/sprites/locais/1_andar/spr_background_rh.png" },
    { "CONTAB. 1",       "CONTABILIDADE 1",        "",             "assets/sprites/locais/1_andar/spr_background_mesa_maria.png" },
    { "CONTAB. 2",       "CONTABILIDADE 2",        "",             "assets/sprites/locais/1_andar/spr_background_mesa_maria.png" },
    { "ARQUIVO",         "ARQUIVO CONTÁBIL",       "",             "assets/sprites/locais/1_andar/spr_background_rh.png" },
    { "COPA",            "COPA DO 1º ANDAR",       "",             "assets/sprites/locais/2_andar/spr_background_geladeira_coffe.png" },
    { "CONVÍVIO",        "ÁREA DE CONVÍVIO",       "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "HALL 2º",         "2º ANDAR - CIBERSEG.",   "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "CIBER. 1",        "CIBERSEGURANÇA 1",       "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "CIBER. 2",        "CIBERSEGURANÇA 2",       "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "SERVER 1",        "SALA DE SERVIDORES 1",   "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "SERVER 2",        "SALA DE SERVIDORES 2",   "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "HALL 3º",         "3º ANDAR - TREINO DE IA","",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "IA ESCR.",        "TREINAMENTO ESCRITÓRIO", "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "TREINO IA",       "SETOR DE TREINO DE IA",  "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "PROTÓTIPOS",      "GARAGEM DE PROTÓTIPOS",  "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "HALL 4º",         "4º ANDAR - EXECUTIVO",   "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "AUDITÓRIO",       "AUDITÓRIO DA RASEC",     "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "SALA CEO",        "SALA DO CEO (TRANCADA)", "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "REUNIÃO 1",       "SALA DE REUNIÃO 1",      "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "REUNIÃO 2",       "SALA DE REUNIÃO 2",      "",             "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "ESCADA",          "ESCADARIA - 3º ANDAR",   "Escadas",      FUNDO_ESCADA_MEIO },
    { "ESCADA",          "ESCADARIA - 4º ANDAR",   "Último andar", FUNDO_ESCADA_ULTIMO },
    { "CIBER.",          "SETOR DE CIBERSEGURANÇA","Vazio",        "assets/sprites/locais/2_andar/spr_background_ciberseguranca.png" },
    { "SERVIDORES",      "SALA DE SERVIDORES",     "Vazio",        "assets/sprites/locais/2_andar/spr_background_servidores.png" },
    { "TREINO IA 1",     "TREINAMENTO DE IA 1",    "Vazio",        "assets/sprites/locais/3_andar/spr_background_treino_ia_1.png" },
    { "TREINO IA 2",     "TREINAMENTO DE IA 2",    "Vazio",        "assets/sprites/locais/3_andar/spr_background_treino_ia_2.png" },
};

static const LocalRasec PLANTA_ANDAR_3D[HOLO_NUM_ANDARES][NUM_POS_ANDAR] = {
    { LOCAL_RECEPCAO, LOCAL_CANTINA,           LOCAL_ESCADA_TERREO },
    { LOCAL_HALL_1,   LOCAL_COPA_1,            LOCAL_ESCADA_1 },
    { LOCAL_HALL_2,   LOCAL_CIBER_1,           LOCAL_ESCADA_2 },
    { LOCAL_HALL_3,   LOCAL_TREINO_ESCRITORIO, LOCAL_ESCADA_2 },
    { LOCAL_HALL_4,   LOCAL_AUDITORIO,         LOCAL_ESCADA_2 },
};

static const LocalRasec PLANTA_ANDAR_2D[HOLO_NUM_ANDARES][NUM_POS_ANDAR] = {
    { LOCAL_ESCRITORIO,   LOCAL_CANTINA,       LOCAL_ESCADA_TERREO },
    { LOCAL_MESA_MARIA,   LOCAL_RH,            LOCAL_ESCADA_1 },
    { LOCAL_2D_CIBER,     LOCAL_2D_SERVIDORES, LOCAL_ESCADA_2 },
    { LOCAL_2D_TREINO_1,  LOCAL_2D_TREINO_2,   LOCAL_ESCADA_3 },
    { LOCAL_SALA_REUNIAO, LOCAL_COPA,          LOCAL_ESCADA_4 },
};

const HoloSala HOLO_SALAS[NUM_LOCAIS] = {
    // TERREO (TerreoRASEC3D.png):
    // Col 1 (x:0.20..1.65): Lanchonete (altura total)
    // Col 2 (x:1.75..3.85): Jardim (cima) + Recepcao+Escada (baixo)
    // Col 3 (x:3.95..5.85): Area de Convivio (cima) + Escritorio Adm 1 (baixo)
    // Col 4 (x:5.95..7.40): [Banheiros decorativo] (cima) + Escritorio Adm 2 (baixo)
    { 0, 1, { { 3.95f, 2.05f, 1.90f, 1.75f } }, false }, // LOCAL_ESCRITORIO (Adm 1)
    { 0, 1, { { 0.20f, 0.20f, 1.45f, 3.60f } }, false }, // LOCAL_CANTINA (Lanchonete coluna esquerda)
    { -1, 2, { { 0.25f, 0.35f, 1.50f, 1.65f }, { 0.25f, 2.00f, 0.95f, 1.60f } }, false }, // LOCAL_RH (2D)
    { -1, 1, { { 2.35f, 0.25f, 2.95f, 1.40f } }, false }, // LOCAL_MESA_MARIA (2D)
    { -1, 1, { { 2.05f, 0.25f, 3.65f, 1.50f } }, false }, // LOCAL_SALA_REUNIAO (2D)
    { -1, 1, { { 0.25f, 1.45f, 1.50f, 2.00f } }, false }, // LOCAL_COPA (2D)
    { -1, 1, { { 3.10f, 2.15f, 0.65f, 0.65f } }, true },  // LOCAL_ESCADA_TERREO
    { -1, 1, { { 3.10f, 2.15f, 0.65f, 0.65f } }, true },  // LOCAL_ESCADA_1
    { -1, 1, { { 3.10f, 2.15f, 0.65f, 0.65f } }, true },  // LOCAL_ESCADA_2
    { -1, 1, { { 6.30f, 0.25f, 1.00f, 3.45f } }, false }, // LOCAL_SUBSOLO_COMECO
    { -1, 1, { { 3.05f, 1.65f, 1.70f, 1.60f } }, false }, // LOCAL_SUBSOLO_ESCOLHA
    { -1, 1, { { 0.25f, 1.55f, 1.75f, 1.85f } }, false }, // LOCAL_SUBSOLO_LAST_OFFICE
    { -1, 1, { { 3.05f, 0.20f, 1.70f, 1.05f } }, false }, // LOCAL_SUBSOLO_VEREDITO
    { -1, 1, { { 0.25f, 0.20f, 1.75f, 1.05f } }, false }, // LOCAL_SUBSOLO_RECREACAO
    { 0, 1, { { 1.75f, 2.05f, 2.10f, 1.75f } }, false },  // LOCAL_RECEPCAO (+ Escada)
    { 0, 1, { { 1.75f, 0.20f, 2.10f, 1.75f } }, false },  // LOCAL_JARDIM (ceu aberto)
    { 0, 1, { { 5.95f, 2.05f, 1.45f, 1.75f } }, false },  // LOCAL_ESCRITORIO_2 (Adm 2)
    // 1o ANDAR (PrimeiroAndarRASEC3D.png) - exatamente alinhado acima do Terreo:
    { 1, 1, { { 1.75f, 2.05f, 2.10f, 1.75f } }, false },  // LOCAL_HALL_1 (+ Escada)
    { 1, 1, { { 3.95f, 2.05f, 1.90f, 1.75f } }, false },  // LOCAL_CONTABILIDADE_1
    { 1, 1, { { 5.95f, 2.05f, 1.45f, 1.75f } }, false },  // LOCAL_CONTABILIDADE_2
    { 1, 1, { { 3.95f, 0.20f, 1.90f, 1.75f } }, false },  // LOCAL_ARQUIVO_1
    { 1, 1, { { 0.20f, 1.35f, 1.45f, 2.45f } }, false },  // LOCAL_COPA_1
    { 0, 1, { { 3.95f, 0.20f, 1.90f, 1.75f } }, false },  // LOCAL_CONVIVIO (Terreo, acima Adm 1)
    // 2o ANDAR (Ciberseguranca + Salas de Servidores, alinhado acima do 1o Andar):
    { 2, 1, { { 1.75f, 2.05f, 2.10f, 1.75f } }, false },  // LOCAL_HALL_2 (+ Elevador)
    { 2, 1, { { 0.20f, 1.35f, 1.45f, 2.45f } }, false },  // LOCAL_CIBER_1
    { 2, 1, { { 3.95f, 2.05f, 1.90f, 1.75f } }, false },  // LOCAL_CIBER_2
    { 2, 1, { { 3.95f, 0.20f, 1.90f, 1.75f } }, false },  // LOCAL_SERVER_1
    { 2, 1, { { 5.95f, 0.20f, 1.45f, 1.75f } }, false },  // LOCAL_SERVER_2
    // 3o ANDAR (Treinamento de IA: 3 salas + Hall 3):
    { 3, 1, { { 1.75f, 2.05f, 2.10f, 1.75f } }, false },  // LOCAL_HALL_3 (+ Elevador)
    { 3, 1, { { 0.20f, 1.35f, 1.45f, 2.45f } }, false },  // LOCAL_TREINO_ESCRITORIO
    { 3, 1, { { 3.95f, 2.05f, 1.90f, 1.75f } }, false },  // LOCAL_TREINO_IA
    { 3, 1, { { 3.95f, 0.20f, 1.90f, 1.75f } }, false },  // LOCAL_GARAGEM_PROTOTIPOS
    // 4o ANDAR (Executivo: Auditorio + Hall 4 + Reuniao 1 + Reuniao 2 + Sala do CEO trancada):
    { 4, 1, { { 1.75f, 2.05f, 2.10f, 1.75f } }, false },  // LOCAL_HALL_4 (+ Elevador)
    { 4, 1, { { 0.20f, 1.35f, 1.45f, 2.45f } }, false },  // LOCAL_AUDITORIO
    { 4, 1, { { 5.95f, 2.05f, 1.45f, 1.75f } }, false },  // LOCAL_CEO (Trancada - visivel no holograma)
    { 4, 1, { { 3.95f, 2.05f, 1.90f, 1.75f } }, false },  // LOCAL_REUNIAO_1
    { 4, 1, { { 3.95f, 0.20f, 1.90f, 1.75f } }, false },  // LOCAL_REUNIAO_2
    { -1, 0, { { 0 } }, true },                           // LOCAL_ESCADA_3 (2D)
    { -1, 0, { { 0 } }, true },                           // LOCAL_ESCADA_4 (2D)
    { -1, 0, { { 0 } }, false },                          // LOCAL_2D_CIBER (2D)
    { -1, 0, { { 0 } }, false },                          // LOCAL_2D_SERVIDORES (2D)
    { -1, 0, { { 0 } }, false },                          // LOCAL_2D_TREINO_1 (2D)
    { -1, 0, { { 0 } }, false },                          // LOCAL_2D_TREINO_2 (2D)
};

const HoloCorredor HOLO_CORREDORES[] = {
    // Terreo: Lanchonete <-> Jardim, Lanchonete <-> Recepcao, Jardim <-> Convivio, Recepcao <-> Adm1 <-> Adm2, Adm1 <-> Convivio
    { 0, { 1.65f, 0.85f, 0.10f, 0.45f } },
    { 0, { 1.65f, 2.70f, 0.10f, 0.45f } },
    { 0, { 3.85f, 0.85f, 0.10f, 0.45f } },
    { 0, { 3.85f, 2.70f, 0.10f, 0.45f } },
    { 0, { 5.85f, 2.70f, 0.10f, 0.45f } },
    { 0, { 4.65f, 1.95f, 0.50f, 0.10f } },
    // 1o Andar: Copa <-> Hall 1 <-> Contab 1 <-> Contab 2, Contab 1 <-> Arquivo
    { 1, { 1.65f, 2.70f, 0.10f, 0.45f } },
    { 1, { 3.85f, 2.70f, 0.10f, 0.45f } },
    { 1, { 5.85f, 2.70f, 0.10f, 0.45f } },
    { 1, { 4.65f, 1.95f, 0.50f, 0.10f } },
    // 2o Andar: Ciber 1 <-> Hall 2 <-> Ciber 2 <-> Server 1 <-> Server 2
    { 2, { 1.65f, 2.70f, 0.10f, 0.45f } },
    { 2, { 3.85f, 2.70f, 0.10f, 0.45f } },
    { 2, { 4.65f, 1.95f, 0.50f, 0.10f } },
    { 2, { 5.85f, 0.85f, 0.10f, 0.45f } },
    // 3o Andar: Treino Escr <-> Hall 3 <-> Treino IA <-> Garagem Prototipos
    { 3, { 1.65f, 2.70f, 0.10f, 0.45f } },
    { 3, { 3.85f, 2.70f, 0.10f, 0.45f } },
    { 3, { 4.65f, 1.95f, 0.50f, 0.10f } },
    // 4o Andar: Auditorio <-> Hall 4 <-> Reuniao 1 <-> Reuniao 2 / Sala CEO (trancada)
    { 4, { 1.65f, 2.70f, 0.10f, 0.45f } },
    { 4, { 3.85f, 2.70f, 0.10f, 0.45f } },
    { 4, { 5.85f, 2.70f, 0.10f, 0.45f } },
    { 4, { 4.65f, 1.95f, 0.50f, 0.10f } },
    { -1, { 4.75f, 2.20f, 1.55f, 0.55f } },
    { -1, { 2.00f, 2.20f, 1.05f, 0.55f } },
    { -1, { 3.65f, 1.25f, 0.50f, 0.40f } },
    { -1, { 0.85f, 1.25f, 0.50f, 0.30f } },
};
const int NUM_HOLO_CORREDORES = (int)(sizeof HOLO_CORREDORES / sizeof HOLO_CORREDORES[0]);

const HoloSala HOLO_SALAS_2D[NUM_LOCAIS_2D] = {
    [LOCAL_ESCRITORIO]          = { 0, 2, { { 1.6f, 0.2f, 3.2f, 1.1f }, { 1.6f, 1.3f, 0.9f, 0.45f } }, false },
    [LOCAL_CANTINA]             = { 0, 1, { { 0.2f, 0.9f, 1.2f, 2.1f } }, false },
    [LOCAL_RH]                  = { 1, 2, { { 0.2f, 0.3f, 1.2f, 1.3f }, { 0.2f, 1.6f, 0.75f, 1.3f } }, false },
    [LOCAL_MESA_MARIA]          = { 1, 1, { { 1.9f, 0.2f, 2.5f, 1.15f } }, false },
    [LOCAL_SALA_REUNIAO]        = { 4, 1, { { 1.6f, 0.2f, 3.3f, 1.25f } }, false },
    [LOCAL_COPA]                = { 4, 1, { { 0.2f, 1.2f, 1.2f, 1.6f } }, false },
    [LOCAL_ESCADA_TERREO]       = { 0, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    [LOCAL_ESCADA_1]            = { 1, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    [LOCAL_ESCADA_2]            = { 2, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    [LOCAL_SUBSOLO_COMECO]      = { -1, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, false },
    [LOCAL_SUBSOLO_ESCOLHA]     = { -1, 1, { { 2.4f, 1.3f, 1.4f, 1.3f } }, false },
    [LOCAL_SUBSOLO_LAST_OFFICE] = { -1, 1, { { 0.2f, 1.2f, 1.4f, 1.5f } }, false },
    [LOCAL_SUBSOLO_VEREDITO]    = { -1, 1, { { 2.4f, 0.15f, 1.4f, 0.85f } }, false },
    [LOCAL_SUBSOLO_RECREACAO]   = { -1, 1, { { 0.2f, 0.15f, 1.4f, 0.85f } }, false },
    [LOCAL_ESCADA_3]            = { 3, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    [LOCAL_ESCADA_4]            = { 4, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    [LOCAL_2D_CIBER]            = { 2, 1, { { 1.8f, 0.2f, 2.8f, 1.2f } }, false },
    [LOCAL_2D_SERVIDORES]       = { 2, 1, { { 0.2f, 0.8f, 1.2f, 2.0f } }, false },
    [LOCAL_2D_TREINO_1]         = { 3, 1, { { 1.7f, 0.2f, 3.0f, 1.2f } }, false },
    [LOCAL_2D_TREINO_2]         = { 3, 1, { { 0.2f, 0.5f, 1.2f, 2.3f } }, false },
};

const HoloCorredor HOLO_CORREDORES_2D[] = {
    { 0, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 0, { 2.9f, 1.3f,  0.45f, 0.65f } },
    { 1, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 1, { 2.9f, 1.35f, 0.45f, 0.6f } },
    { 2, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 2, { 2.9f, 1.40f, 0.45f, 0.55f } },
    { 3, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 3, { 2.9f, 1.40f, 0.45f, 0.55f } },
    { 4, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 4, { 2.9f, 1.45f, 0.45f, 0.5f } },
    { -1, { 3.8f, 1.75f, 1.3f, 0.45f } },
    { -1, { 1.6f, 1.75f, 0.8f, 0.45f } },
    { -1, { 2.9f, 1.0f,  0.4f, 0.3f } },
    { -1, { 0.7f, 1.0f,  0.4f, 0.2f } },
};
const int NUM_HOLO_CORREDORES_2D = (int)(sizeof HOLO_CORREDORES_2D / sizeof HOLO_CORREDORES_2D[0]);

const char *NOMES_ANDARES[HOLO_NUM_ANDARES] = { "TÉRREO", "1º ANDAR", "2º ANDAR", "3º ANDAR", "4º ANDAR" };

const Vector2 SETAS_POS[NUM_SETAS] = {
    { 149.0f,  52.0f },
    { 149.0f, 192.0f },
    {  26.0f, 118.0f },
    { 272.0f, 118.0f },
};

LocalRasec GameplayPlantaAndar(int andar, int pos) {
    return GameStateIs3DMode() ? PLANTA_ANDAR_3D[andar][pos] : PLANTA_ANDAR_2D[andar][pos];
}

const HoloSala *GameplayHoloSalasAtivas(void) {
    return GameStateIs3DMode() ? HOLO_SALAS : HOLO_SALAS_2D;
}

int GameplayNumHoloSalasAtivas(void) {
    return GameStateIs3DMode() ? NUM_LOCAIS : NUM_LOCAIS_2D;
}

const char *GameplayNomeCurtoLocal(int i) {
    if (!GameStateIs3DMode()) {
        if (i == LOCAL_ESCRITORIO) return "SETOR ADM.";
        if (i == LOCAL_CANTINA)    return "LANCHONETE";
    }
    return LOCAIS[i].nome;
}

LocalRasec GameplayLocalInicial(void) {
    return GameStateIs3DMode() ? LOCAL_RECEPCAO : LOCAL_ESCRITORIO;
}

int GameplayDestinoEscadaSubir(int de) {
    const HoloSala *salas = GameplayHoloSalasAtivas();
    if (!salas[de].escada) return -1;
    int andar = salas[de].andar;
    return (andar + 1 < HOLO_NUM_ANDARES) ? (int)GameplayPlantaAndar(andar + 1, POS_ESCADA) : -1;
}

int GameplayDestinoEscadaDescer(int de) {
    const HoloSala *salas = GameplayHoloSalasAtivas();
    if (!salas[de].escada) return -1;
    int andar = salas[de].andar;
    if (de == LOCAL_ESCADA_TERREO) return SettingsGetDevMode() ? LOCAL_SUBSOLO_COMECO : -1;
    return (andar > 0) ? (int)GameplayPlantaAndar(andar - 1, POS_ESCADA) : -1;
}

int GameplayDestinoSeta(int de, int seta) {
    if (de == LOCAL_SUBSOLO_COMECO) {
        if (!SettingsGetDevMode()) return -1;
        if (seta == SETA_CIMA)  return LOCAL_SUBSOLO_ESCOLHA;
        if (seta == SETA_BAIXO) return LOCAL_ESCADA_TERREO;
        return -1;
    }
    if (de == LOCAL_SUBSOLO_ESCOLHA) {
        if (!SettingsGetDevMode()) return -1;
        if (seta == SETA_ESQUERDA) return LOCAL_SUBSOLO_LAST_OFFICE;
        if (seta == SETA_DIREITA)  return LOCAL_SUBSOLO_VEREDITO;
        if (seta == SETA_BAIXO)    return LOCAL_SUBSOLO_COMECO;
        return -1;
    }
    if (de == LOCAL_SUBSOLO_VEREDITO) {
        if (!SettingsGetDevMode()) return -1;
        return (seta == SETA_BAIXO) ? LOCAL_SUBSOLO_ESCOLHA : -1;
    }
    if (de == LOCAL_SUBSOLO_LAST_OFFICE) {
        if (!SettingsGetDevMode()) return -1;
        if (seta == SETA_CIMA)  return LOCAL_SUBSOLO_RECREACAO;
        if (seta == SETA_BAIXO) return LOCAL_SUBSOLO_ESCOLHA;
        return -1;
    }
    if (de == LOCAL_SUBSOLO_RECREACAO) {
        if (!SettingsGetDevMode()) return -1;
        return (seta == SETA_BAIXO) ? LOCAL_SUBSOLO_LAST_OFFICE : -1;
    }
    const HoloSala *salas = GameplayHoloSalasAtivas();
    int andar = salas[de].andar;
    int central = (int)GameplayPlantaAndar(andar, POS_CIMA);
    if (salas[de].escada) {
        if (seta == SETA_ESQUERDA) return central;
        return -1;
    }
    if (de == central) {
        if (seta == SETA_ESQUERDA) return (int)GameplayPlantaAndar(andar, POS_ESQUERDA);
        if (seta == SETA_DIREITA)  return (int)GameplayPlantaAndar(andar, POS_ESCADA);
        return -1;
    }
    return (seta == SETA_DIREITA) ? central : -1;
}

uint64_t GameplayLocaisAcessiveis(int de) {
    if (GameStateIs3DMode()) {
        if (de == LOCAL_RECEPCAO)           return (1ULL << LOCAL_CANTINA) | (1ULL << LOCAL_ESCRITORIO) | (1ULL << LOCAL_HALL_1);
        if (de == LOCAL_CANTINA)            return (1ULL << LOCAL_RECEPCAO) | (1ULL << LOCAL_JARDIM);
        if (de == LOCAL_JARDIM)             return (1ULL << LOCAL_CANTINA) | (1ULL << LOCAL_CONVIVIO);
        if (de == LOCAL_CONVIVIO)           return (1ULL << LOCAL_JARDIM) | (1ULL << LOCAL_ESCRITORIO);
        if (de == LOCAL_ESCRITORIO)         return (1ULL << LOCAL_RECEPCAO) | (1ULL << LOCAL_CONVIVIO) | (1ULL << LOCAL_ESCRITORIO_2);
        if (de == LOCAL_ESCRITORIO_2)       return (1ULL << LOCAL_ESCRITORIO);
        if (de == LOCAL_HALL_1)             return (1ULL << LOCAL_RECEPCAO) | (1ULL << LOCAL_COPA_1) | (1ULL << LOCAL_CONTABILIDADE_1);
        if (de == LOCAL_CONTABILIDADE_1)    return (1ULL << LOCAL_HALL_1) | (1ULL << LOCAL_ARQUIVO_1) | (1ULL << LOCAL_CONTABILIDADE_2);
        if (de == LOCAL_CONTABILIDADE_2)    return (1ULL << LOCAL_CONTABILIDADE_1);
        if (de == LOCAL_ARQUIVO_1)          return (1ULL << LOCAL_CONTABILIDADE_1);
        if (de == LOCAL_COPA_1)             return (1ULL << LOCAL_HALL_1);
        if (de == LOCAL_HALL_2)             return (1ULL << LOCAL_HALL_1) | (1ULL << LOCAL_CIBER_1) | (1ULL << LOCAL_CIBER_2);
        if (de == LOCAL_CIBER_1)            return (1ULL << LOCAL_HALL_2);
        if (de == LOCAL_CIBER_2)            return (1ULL << LOCAL_HALL_2) | (1ULL << LOCAL_SERVER_1);
        if (de == LOCAL_SERVER_1)           return (1ULL << LOCAL_CIBER_2) | (1ULL << LOCAL_SERVER_2);
        if (de == LOCAL_SERVER_2)           return (1ULL << LOCAL_SERVER_1);
        if (de == LOCAL_HALL_3)             return (1ULL << LOCAL_HALL_2) | (1ULL << LOCAL_TREINO_ESCRITORIO) | (1ULL << LOCAL_TREINO_IA);
        if (de == LOCAL_TREINO_ESCRITORIO)  return (1ULL << LOCAL_HALL_3);
        if (de == LOCAL_TREINO_IA)          return (1ULL << LOCAL_HALL_3) | (1ULL << LOCAL_GARAGEM_PROTOTIPOS);
        if (de == LOCAL_GARAGEM_PROTOTIPOS) return (1ULL << LOCAL_TREINO_IA);
        if (de == LOCAL_HALL_4)             return (1ULL << LOCAL_HALL_3) | (1ULL << LOCAL_AUDITORIO) | (1ULL << LOCAL_REUNIAO_1);
        if (de == LOCAL_AUDITORIO)          return (1ULL << LOCAL_HALL_4);
        if (de == LOCAL_REUNIAO_1)          return (1ULL << LOCAL_HALL_4) | (1ULL << LOCAL_REUNIAO_2);
        if (de == LOCAL_REUNIAO_2)          return (1ULL << LOCAL_REUNIAO_1);
    }

    uint64_t bits = 0;
    for (int d = 0; d < NUM_SETAS; d++) {
        int destino = GameplayDestinoSeta(de, d);
        if (destino >= 0) bits |= 1ULL << destino;
    }
    int dSub = GameplayDestinoEscadaSubir(de);
    int dDes = GameplayDestinoEscadaDescer(de);
    if (dSub >= 0) bits |= 1ULL << dSub;
    if (dDes >= 0) bits |= 1ULL << dDes;
    return bits;
}

const char *GameplayTextoSeta(int de, int seta) {
    int destino = GameplayDestinoSeta(de, seta);
    if (destino < 0) return "";
    const HoloSala *salas = GameplayHoloSalasAtivas();
    if (salas[de].andar < 0 || salas[destino].andar < 0) return GameplayNomeCurtoLocal(destino);
    if (salas[de].escada && salas[destino].escada)
        return TextFormat("%s: %s", seta == SETA_CIMA ? "SUBIR" : "DESCER", NOMES_ANDARES[salas[destino].andar]);
    if (salas[destino].escada) return "ESCADARIA";
    if (salas[de].escada) return TextFormat("IR PARA: %s", GameplayNomeCurtoLocal(destino));
    if (destino == (int)GameplayPlantaAndar(salas[de].andar, POS_CIMA)) return TextFormat("VOLTAR: %s", GameplayNomeCurtoLocal(destino));
    return GameplayNomeCurtoLocal(destino);
}

bool GameplayLocalEhTerreo3D(LocalRasec loc) {
    if (!GameStateIs3DMode()) return false;
    return loc == LOCAL_RECEPCAO           || loc == LOCAL_CANTINA ||
           loc == LOCAL_JARDIM             || loc == LOCAL_ESCRITORIO ||
           loc == LOCAL_ESCRITORIO_2       || loc == LOCAL_CONVIVIO ||
           loc == LOCAL_HALL_1             || loc == LOCAL_CONTABILIDADE_1 ||
           loc == LOCAL_CONTABILIDADE_2    || loc == LOCAL_ARQUIVO_1 ||
           loc == LOCAL_COPA_1             || loc == LOCAL_HALL_2 ||
           loc == LOCAL_CIBER_1            || loc == LOCAL_CIBER_2 ||
           loc == LOCAL_SERVER_1           || loc == LOCAL_SERVER_2 ||
           loc == LOCAL_HALL_3             || loc == LOCAL_TREINO_ESCRITORIO ||
           loc == LOCAL_TREINO_IA          || loc == LOCAL_GARAGEM_PROTOTIPOS ||
           loc == LOCAL_HALL_4             || loc == LOCAL_AUDITORIO ||
           loc == LOCAL_REUNIAO_1          || loc == LOCAL_REUNIAO_2;
}

TerreoSalaId GameplayTerreoSalaPorLocal(LocalRasec loc) {
    switch (loc) {
        case LOCAL_RECEPCAO:           return TERREO_SALA_RECEPCAO;
        case LOCAL_CANTINA:            return TERREO_SALA_LANCHONETE;
        case LOCAL_JARDIM:             return TERREO_SALA_JARDIM;
        case LOCAL_ESCRITORIO:         return TERREO_SALA_ESCRITORIO;
        case LOCAL_ESCRITORIO_2:       return TERREO_SALA_ESCRITORIO_2;
        case LOCAL_CONVIVIO:           return TERREO_SALA_CONVIVIO;
        case LOCAL_ESCADA_TERREO:      return TERREO_SALA_ESCADA_TERREO;
        case LOCAL_ESCADA_1:           return TERREO_SALA_ESCADA_1;
        case LOCAL_HALL_1:             return TERREO_SALA_HALL_1;
        case LOCAL_CONTABILIDADE_1:    return TERREO_SALA_CONTABILIDADE_1;
        case LOCAL_CONTABILIDADE_2:    return TERREO_SALA_CONTABILIDADE_2;
        case LOCAL_ARQUIVO_1:          return TERREO_SALA_ARQUIVO_1;
        case LOCAL_COPA_1:             return TERREO_SALA_COPA_1;
        case LOCAL_HALL_2:             return TERREO_SALA_HALL_2;
        case LOCAL_CIBER_1:            return TERREO_SALA_CIBER_1;
        case LOCAL_CIBER_2:            return TERREO_SALA_CIBER_2;
        case LOCAL_SERVER_1:           return TERREO_SALA_SERVER_1;
        case LOCAL_SERVER_2:           return TERREO_SALA_SERVER_2;
        case LOCAL_HALL_3:             return TERREO_SALA_HALL_3;
        case LOCAL_TREINO_ESCRITORIO:  return TERREO_SALA_TREINO_ESCRITORIO;
        case LOCAL_TREINO_IA:          return TERREO_SALA_TREINO_IA;
        case LOCAL_GARAGEM_PROTOTIPOS: return TERREO_SALA_GARAGEM_PROTOTIPOS;
        case LOCAL_HALL_4:             return TERREO_SALA_HALL_4;
        case LOCAL_AUDITORIO:          return TERREO_SALA_AUDITORIO;
        case LOCAL_REUNIAO_1:          return TERREO_SALA_REUNIAO_1;
        case LOCAL_REUNIAO_2:          return TERREO_SALA_REUNIAO_2;
        default:                       return TERREO_SALA_RECEPCAO;
    }
}

TerreoDestino GameplayTerreoDestinoPorLocal(LocalRasec loc) {
    switch (loc) {
        case LOCAL_RECEPCAO:           return TERREO_DEST_RECEPCAO;
        case LOCAL_CANTINA:            return TERREO_DEST_LANCHONETE;
        case LOCAL_JARDIM:             return TERREO_DEST_JARDIM;
        case LOCAL_ESCRITORIO:         return TERREO_DEST_ESCRITORIO;
        case LOCAL_ESCRITORIO_2:       return TERREO_DEST_ESCRITORIO_2;
        case LOCAL_CONVIVIO:           return TERREO_DEST_CONVIVIO;
        case LOCAL_ESCADA_TERREO:      return TERREO_DEST_ESCADA_2D;
        case LOCAL_ESCADA_1:           return TERREO_DEST_ESCADA_1;
        case LOCAL_HALL_1:             return TERREO_DEST_HALL_1;
        case LOCAL_CONTABILIDADE_1:    return TERREO_DEST_CONTABILIDADE_1;
        case LOCAL_CONTABILIDADE_2:    return TERREO_DEST_CONTABILIDADE_2;
        case LOCAL_ARQUIVO_1:          return TERREO_DEST_ARQUIVO_1;
        case LOCAL_COPA_1:             return TERREO_DEST_COPA_1;
        case LOCAL_HALL_2:             return TERREO_DEST_HALL_2;
        case LOCAL_CIBER_1:            return TERREO_DEST_CIBER_1;
        case LOCAL_CIBER_2:            return TERREO_DEST_CIBER_2;
        case LOCAL_SERVER_1:           return TERREO_DEST_SERVER_1;
        case LOCAL_SERVER_2:           return TERREO_DEST_SERVER_2;
        case LOCAL_HALL_3:             return TERREO_DEST_HALL_3;
        case LOCAL_TREINO_ESCRITORIO:  return TERREO_DEST_TREINO_ESCRITORIO;
        case LOCAL_TREINO_IA:          return TERREO_DEST_TREINO_IA;
        case LOCAL_GARAGEM_PROTOTIPOS: return TERREO_DEST_GARAGEM_PROTOTIPOS;
        case LOCAL_HALL_4:             return TERREO_DEST_HALL_4;
        case LOCAL_AUDITORIO:          return TERREO_DEST_AUDITORIO;
        case LOCAL_REUNIAO_1:          return TERREO_DEST_REUNIAO_1;
        case LOCAL_REUNIAO_2:          return TERREO_DEST_REUNIAO_2;
        default:                       return TERREO_DEST_NENHUM;
    }
}

LocalRasec GameplayLocalPorTerreoDestino(TerreoDestino dest) {
    switch (dest) {
        case TERREO_DEST_RECEPCAO:           return LOCAL_RECEPCAO;
        case TERREO_DEST_LANCHONETE:         return LOCAL_CANTINA;
        case TERREO_DEST_JARDIM:             return LOCAL_JARDIM;
        case TERREO_DEST_ESCRITORIO:         return LOCAL_ESCRITORIO;
        case TERREO_DEST_ESCRITORIO_2:       return LOCAL_ESCRITORIO_2;
        case TERREO_DEST_CONVIVIO:           return LOCAL_CONVIVIO;
        case TERREO_DEST_ESCADA_2D:          return LOCAL_ESCADA_TERREO;
        case TERREO_DEST_ESCADA_1:           return LOCAL_ESCADA_1;
        case TERREO_DEST_HALL_1:             return LOCAL_HALL_1;
        case TERREO_DEST_CONTABILIDADE_1:    return LOCAL_CONTABILIDADE_1;
        case TERREO_DEST_CONTABILIDADE_2:    return TERREO_DEST_CONTABILIDADE_2;
        case TERREO_DEST_ARQUIVO_1:          return LOCAL_ARQUIVO_1;
        case TERREO_DEST_COPA_1:             return LOCAL_COPA_1;
        case TERREO_DEST_HALL_2:             return LOCAL_HALL_2;
        case TERREO_DEST_CIBER_1:            return LOCAL_CIBER_1;
        case TERREO_DEST_CIBER_2:            return LOCAL_CIBER_2;
        case TERREO_DEST_SERVER_1:           return LOCAL_SERVER_1;
        case TERREO_DEST_SERVER_2:           return LOCAL_SERVER_2;
        case TERREO_DEST_HALL_3:             return LOCAL_HALL_3;
        case TERREO_DEST_TREINO_ESCRITORIO:  return LOCAL_TREINO_ESCRITORIO;
        case TERREO_DEST_TREINO_IA:          return LOCAL_TREINO_IA;
        case TERREO_DEST_GARAGEM_PROTOTIPOS: return LOCAL_GARAGEM_PROTOTIPOS;
        case TERREO_DEST_HALL_4:             return LOCAL_HALL_4;
        case TERREO_DEST_AUDITORIO:          return LOCAL_AUDITORIO;
        case TERREO_DEST_REUNIAO_1:          return LOCAL_REUNIAO_1;
        case TERREO_DEST_REUNIAO_2:          return LOCAL_REUNIAO_2;
        default:                             return LOCAL_RECEPCAO;
    }
}
