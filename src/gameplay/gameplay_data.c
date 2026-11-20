#include "gameplay_data.h"
#include "game_state.h"
#include "settings.h"

const InfoLocal LOCAIS[NUM_LOCAIS] = {
    { "ADM. 1",          "SETOR ADMINISTRATIVO",   "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "CANTINA",         "LANCHONETE DA RASEC",    "",             "assets/sprites/locais/terreo/spr_background_cantina.png" },
    { "RH",              "RH DA RASEC",            "Vazio",        "assets/sprites/locais/1_andar/spr_background_rh.png" },
    { "CONTAB.",         "SETOR DE CONTABILIDADE", "Vazio",        "assets/sprites/locais/1_andar/spr_background_mesa_maria.png" },
    { "AUDIT.",          "AUDITÓRIO",              "Vazio",        "assets/sprites/locais/2_andar/spr_background_sala_reuniao.png" },
    { "COPA",            "COPA DO 2º ANDAR",       "Vazio",        "assets/sprites/locais/2_andar/spr_background_geladeira_coffe.png" },
    { "ESCADA",          "ESCADARIA - TÉRREO",     "Escadas",      FUNDO_ESCADA_MEIO },
    { "ESCADA",          "ESCADARIA - 1º ANDAR",   "Escadas",      FUNDO_ESCADA_MEIO },
    { "ESCADA",          "ESCADARIA - 2º ANDAR",   "Último andar", FUNDO_ESCADA_ULTIMO },
    { "SUBSOLO",         "SUBSOLO - ENTRADA",      "Vazio",        "assets/sprites/locais/subsolo/local_comeco.png" },
    { "BIFURC.",         "SUBSOLO - ESCOLHA",      "Vazio",        "assets/sprites/locais/subsolo/local_escolha.png" },
    { "ÚLT. ESCR.",      "ÚLTIMO ESCRITÓRIO",      "Vazio",        "assets/sprites/locais/subsolo/last_office.png" },
    { "VEREDITO",        "SALA DO VEREDITO",       "Vazio",        "assets/sprites/veredito/spr_veredito_sala.png" },
    { "RECRE.",          "ÁREA DE RECREAÇÃO",      "Vazio",        "assets/sprites/locais/subsolo/area_recreacao_abandonada.png" },
    { "RECEPÇÃO",        "RECEPÇÃO DA RASEC",      "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "JARDIM",          "JARDIM / DESCANSO",      "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
    { "ADM. 2",          "SETOR ADM. - ALA ESQ.",  "",             "assets/sprites/locais/terreo/spr_background_teste.png" },
};

static const LocalRasec PLANTA_ANDAR_3D[HOLO_NUM_ANDARES][NUM_POS_ANDAR] = {
    { LOCAL_RECEPCAO,     LOCAL_CANTINA, LOCAL_ESCADA_TERREO },
    { LOCAL_MESA_MARIA,   LOCAL_RH,      LOCAL_ESCADA_1 },
    { LOCAL_SALA_REUNIAO, LOCAL_COPA,    LOCAL_ESCADA_2 },
};

static const LocalRasec PLANTA_ANDAR_2D[HOLO_NUM_ANDARES][NUM_POS_ANDAR] = {
    { LOCAL_ESCRITORIO,   LOCAL_CANTINA, LOCAL_ESCADA_TERREO },
    { LOCAL_MESA_MARIA,   LOCAL_RH,      LOCAL_ESCADA_1 },
    { LOCAL_SALA_REUNIAO, LOCAL_COPA,    LOCAL_ESCADA_2 },
};

const HoloSala HOLO_SALAS[NUM_LOCAIS] = {
    { 0, 1, { { 4.55f, 1.35f, 1.25f, 2.05f } }, false },
    { 0, 1, { { 0.25f, 1.40f, 1.25f, 2.00f } }, false },
    { 1, 2, { { 0.25f, 0.35f, 1.50f, 1.65f }, { 0.25f, 2.00f, 0.95f, 1.60f } }, false },
    { 1, 1, { { 2.35f, 0.25f, 2.95f, 1.40f } }, false },
    { 2, 1, { { 2.05f, 0.25f, 3.65f, 1.50f } }, false },
    { 2, 1, { { 0.25f, 1.45f, 1.50f, 2.00f } }, false },
    { 0, 1, { { 6.30f, 0.25f, 1.00f, 1.15f } }, true },
    { 1, 1, { { 6.30f, 0.25f, 1.00f, 3.45f } }, true },
    { 2, 1, { { 6.30f, 0.25f, 1.00f, 3.45f } }, true },
    { -1, 1, { { 6.30f, 0.25f, 1.00f, 3.45f } }, false },
    { -1, 1, { { 3.05f, 1.65f, 1.70f, 1.60f } }, false },
    { -1, 1, { { 0.25f, 1.55f, 1.75f, 1.85f } }, false },
    { -1, 1, { { 3.05f, 0.20f, 1.70f, 1.05f } }, false },
    { -1, 1, { { 0.25f, 0.20f, 1.75f, 1.05f } }, false },
    { 0, 1, { { 2.05f, 2.05f, 1.95f, 1.45f } }, false },
    { 0, 1, { { 2.05f, 0.20f, 2.55f, 1.00f } }, false },
    { 0, 1, { { 6.30f, 1.75f, 1.05f, 1.70f } }, false },
};

const HoloCorredor HOLO_CORREDORES[] = {
    { 0, { 1.50f, 2.55f, 0.55f, 0.50f } }, { 0, { 4.00f, 2.55f, 0.55f, 0.50f } },
    { 0, { 5.80f, 2.25f, 0.50f, 0.50f } },
    { 0, { 1.50f, 0.55f, 0.55f, 0.50f } }, { 0, { 4.60f, 0.55f, 0.55f, 0.80f } },
    { 0, { 3.90f, 1.20f, 2.40f, 0.35f } },
    { 1, { 1.75f, 2.45f, 4.55f, 0.65f } }, { 1, { 3.55f, 1.65f, 0.55f, 0.80f } },
    { 2, { 1.75f, 2.45f, 4.55f, 0.65f } }, { 2, { 3.55f, 1.75f, 0.55f, 0.70f } },
    { -1, { 4.75f, 2.20f, 1.55f, 0.55f } },
    { -1, { 2.00f, 2.20f, 1.05f, 0.55f } },
    { -1, { 3.65f, 1.25f, 0.50f, 0.40f } },
    { -1, { 0.85f, 1.25f, 0.50f, 0.30f } },
};
const int NUM_HOLO_CORREDORES = (int)(sizeof HOLO_CORREDORES / sizeof HOLO_CORREDORES[0]);

const HoloSala HOLO_SALAS_2D[NUM_LOCAIS_2D] = {
    { 0, 2, { { 1.6f, 0.2f, 3.2f, 1.1f }, { 1.6f, 1.3f, 0.9f, 0.45f } }, false },
    { 0, 1, { { 0.2f, 0.9f, 1.2f, 2.1f } }, false },
    { 1, 2, { { 0.2f, 0.3f, 1.2f, 1.3f }, { 0.2f, 1.6f, 0.75f, 1.3f } }, false },
    { 1, 1, { { 1.9f, 0.2f, 2.5f, 1.15f } }, false },
    { 2, 1, { { 1.6f, 0.2f, 3.3f, 1.25f } }, false },
    { 2, 1, { { 0.2f, 1.2f, 1.2f, 1.6f } }, false },
    { 0, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    { 1, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    { 2, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, true },
    { -1, 1, { { 5.1f, 0.2f, 0.9f, 2.8f } }, false },
    { -1, 1, { { 2.4f, 1.3f, 1.4f, 1.3f } }, false },
    { -1, 1, { { 0.2f, 1.2f, 1.4f, 1.5f } }, false },
    { -1, 1, { { 2.4f, 0.15f, 1.4f, 0.85f } }, false },
    { -1, 1, { { 0.2f, 0.15f, 1.4f, 0.85f } }, false },
};

const HoloCorredor HOLO_CORREDORES_2D[] = {
    { 0, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 0, { 2.9f, 1.3f,  0.45f, 0.65f } },
    { 1, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 1, { 2.9f, 1.35f, 0.45f, 0.6f } },
    { 2, { 1.4f, 1.95f, 3.7f, 0.55f } }, { 2, { 2.9f, 1.45f, 0.45f, 0.5f } },
    { -1, { 3.8f, 1.75f, 1.3f, 0.45f } },
    { -1, { 1.6f, 1.75f, 0.8f, 0.45f } },
    { -1, { 2.9f, 1.0f,  0.4f, 0.3f } },
    { -1, { 0.7f, 1.0f,  0.4f, 0.2f } },
};
const int NUM_HOLO_CORREDORES_2D = (int)(sizeof HOLO_CORREDORES_2D / sizeof HOLO_CORREDORES_2D[0]);

const char *NOMES_ANDARES[HOLO_NUM_ANDARES] = { "TÉRREO", "1º ANDAR", "2º ANDAR" };

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

unsigned GameplayLocaisAcessiveis(int de) {
    if (GameStateIs3DMode()) {
        if (de == LOCAL_RECEPCAO)     return (1u << LOCAL_CANTINA) | (1u << LOCAL_ESCRITORIO) | (1u << LOCAL_ESCADA_TERREO);
        if (de == LOCAL_CANTINA)      return (1u << LOCAL_RECEPCAO) | (1u << LOCAL_JARDIM);
        if (de == LOCAL_JARDIM)       return (1u << LOCAL_CANTINA) | (1u << LOCAL_ESCRITORIO);
        if (de == LOCAL_ESCRITORIO)   return (1u << LOCAL_RECEPCAO) | (1u << LOCAL_JARDIM) | (1u << LOCAL_ESCRITORIO_2);
        if (de == LOCAL_ESCRITORIO_2) return (1u << LOCAL_ESCRITORIO);
    }

    unsigned bits = 0;
    for (int d = 0; d < NUM_SETAS; d++) {
        int destino = GameplayDestinoSeta(de, d);
        if (destino >= 0) bits |= 1u << destino;
    }
    int dSub = GameplayDestinoEscadaSubir(de);
    int dDes = GameplayDestinoEscadaDescer(de);
    if (dSub >= 0) bits |= 1u << dSub;
    if (dDes >= 0) bits |= 1u << dDes;
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
    return loc == LOCAL_RECEPCAO || loc == LOCAL_CANTINA ||
           loc == LOCAL_JARDIM   || loc == LOCAL_ESCRITORIO ||
           loc == LOCAL_ESCRITORIO_2;
}

TerreoSalaId GameplayTerreoSalaPorLocal(LocalRasec loc) {
    switch (loc) {
        case LOCAL_RECEPCAO:     return TERREO_SALA_RECEPCAO;
        case LOCAL_CANTINA:      return TERREO_SALA_LANCHONETE;
        case LOCAL_JARDIM:       return TERREO_SALA_JARDIM;
        case LOCAL_ESCRITORIO:   return TERREO_SALA_ESCRITORIO;
        case LOCAL_ESCRITORIO_2: return TERREO_SALA_ESCRITORIO_2;
        default:                 return TERREO_SALA_RECEPCAO;
    }
}

TerreoDestino GameplayTerreoDestinoPorLocal(LocalRasec loc) {
    switch (loc) {
        case LOCAL_RECEPCAO:      return TERREO_DEST_RECEPCAO;
        case LOCAL_CANTINA:       return TERREO_DEST_LANCHONETE;
        case LOCAL_JARDIM:        return TERREO_DEST_JARDIM;
        case LOCAL_ESCRITORIO:    return TERREO_DEST_ESCRITORIO;
        case LOCAL_ESCRITORIO_2:  return TERREO_DEST_ESCRITORIO_2;
        case LOCAL_ESCADA_TERREO: return TERREO_DEST_ESCADA_2D;
        default:                  return TERREO_DEST_NENHUM;
    }
}

LocalRasec GameplayLocalPorTerreoDestino(TerreoDestino dest) {
    switch (dest) {
        case TERREO_DEST_RECEPCAO:     return LOCAL_RECEPCAO;
        case TERREO_DEST_LANCHONETE:   return LOCAL_CANTINA;
        case TERREO_DEST_JARDIM:       return LOCAL_JARDIM;
        case TERREO_DEST_ESCRITORIO:   return LOCAL_ESCRITORIO;
        case TERREO_DEST_ESCRITORIO_2: return LOCAL_ESCRITORIO_2;
        case TERREO_DEST_ESCADA_2D:    return LOCAL_ESCADA_TERREO;
        default:                       return LOCAL_RECEPCAO;
    }
}
