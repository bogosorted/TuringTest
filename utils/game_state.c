#include "game_state.h"
#include "settings.h"
#include <stdlib.h>
#include <string.h>

static int          g_day = 1;
static bool         g_tutorialConfig = true; // Por padrao ativado no setup
static bool         g_tutorialActive = true;
static VisaoJogador g_visaoJogador[GAME_NUM_SUSPECTS] = { VISAO_DUVIDA, VISAO_DUVIDA, VISAO_DUVIDA };
static bool         g_evidencias[GAME_NUM_SUSPECTS][GAME_MAX_EVIDENCIAS] = { { false } };
static bool         g_viesUsado[GAME_NUM_SUSPECTS] = { false };
static bool         g_injecaoUsada[GAME_NUM_SUSPECTS] = { false };
static char         g_playerName[9] = "AGENTE";
static int          g_tokensRestantesTotal = 0;
static int          g_dicasUsadasTotal = 0;

static int ParseEvIndex(const char *ev_id) {
    if (!ev_id) return -1;
    // Aceita "EV-01" .. "EV-16"
    if (strncmp(ev_id, "EV-", 3) == 0) {
        int num = atoi(ev_id + 3);
        if (num >= 1 && num <= GAME_MAX_EVIDENCIAS) {
            return num - 1;
        }
    }
    return -1;
}

void GameStateNewRun(void) {
    g_day = 1;
    g_tutorialConfig = SettingsGetTutorial();
    g_tutorialActive = g_tutorialConfig;
    g_tokensRestantesTotal = 0;
    g_dicasUsadasTotal = 0;
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        g_visaoJogador[i] = VISAO_DUVIDA;
        g_viesUsado[i] = false;
        g_injecaoUsada[i] = false;
        for (int e = 0; e < GAME_MAX_EVIDENCIAS; e++) {
            g_evidencias[i][e] = false;
        }
    }
}

void GameStateNextDay(void) {
    if (g_day < GAME_NUM_DAYS) g_day++;
    g_tutorialActive = false;
}

int GameStateGetDay(void) {
    return g_day;
}

bool GameStateIsTutorialActive(void) {
    return (g_day == 1) && g_tutorialConfig && g_tutorialActive;
}

void GameStateCompleteTutorial(void) {
    g_tutorialActive = false;
}

bool GameStateGetTutorialConfig(void) {
    return g_tutorialConfig;
}

void GameStateSetTutorialConfig(bool enabled) {
    g_tutorialConfig = enabled;
    if (g_day == 1) {
        g_tutorialActive = enabled;
    }
}

void GameStateSetVisaoJogador(int suspeito, VisaoJogador visao) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_visaoJogador[suspeito] = visao;
}

VisaoJogador GameStateGetVisaoJogador(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return VISAO_DUVIDA;
    return g_visaoJogador[suspeito];
}

bool GameStateRegistrarEvidencia(int suspeito, const char *ev_id) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    int idx = ParseEvIndex(ev_id);
    if (idx < 0) return false;
    if (g_evidencias[suspeito][idx]) return false;
    g_evidencias[suspeito][idx] = true;
    return true;
}

bool GameStateTemEvidencia(int suspeito, const char *ev_id) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    int idx = ParseEvIndex(ev_id);
    if (idx < 0) return false;
    return g_evidencias[suspeito][idx];
}

void GameStateSetViesUsado(int suspeito, bool usado) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_viesUsado[suspeito] = usado;
}

bool GameStateGetViesUsado(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    return g_viesUsado[suspeito];
}

void GameStateSetInjecaoUsada(int suspeito, bool usada) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_injecaoUsada[suspeito] = usada;
}

bool GameStateGetInjecaoUsada(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    return g_injecaoUsada[suspeito];
}

void GameStateSetPlayerName(const char *name) {
    int j = 0;
    if (name) {
        for (int i = 0; name[i] != '\0' && j < 8; i++) {
            unsigned char c = (unsigned char)name[i];
            if (c >= 33 && c <= 126 && c != '=') {
                g_playerName[j++] = (char)c;
            }
        }
    }
    if (j == 0) {
        strcpy(g_playerName, "AGENTE");
    } else {
        g_playerName[j] = '\0';
    }
}

const char *GameStateGetPlayerName(void) {
    return g_playerName;
}

void GameStateAddRemainingTokens(int tokens) {
    if (tokens > 0) g_tokensRestantesTotal += tokens;
}

int GameStateGetRemainingTokensTotal(void) {
    return g_tokensRestantesTotal;
}

void GameStateRegisterHintUsed(void) {
    if (g_dicasUsadasTotal < GAME_NUM_DAYS) g_dicasUsadasTotal++;
}

int GameStateGetUnusedHintsCount(void) {
    int unused = GAME_NUM_DAYS - g_dicasUsadasTotal;
    return (unused > 0) ? unused : 0;
}

bool GameStateAnyViesUsed(void) {
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        if (g_viesUsado[i]) return true;
    }
    return false;
}

bool GameStateAnyInjecaoUsed(void) {
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        if (g_injecaoUsada[i]) return true;
    }
    return false;
}

int GameStateCountEvidences(void) {
    int total = 0;
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        for (int e = 0; e < GAME_MAX_EVIDENCIAS; e++) {
            if (g_evidencias[i][e]) total++;
        }
    }
    return total;
}
