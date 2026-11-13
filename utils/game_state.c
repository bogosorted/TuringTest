#include "game_state.h"
#include <stdlib.h>
#include <string.h>

static int          g_day = 1;
static bool         g_tutorialActive = true;
static VisaoJogador g_visaoJogador[GAME_NUM_SUSPECTS] = { VISAO_DUVIDA, VISAO_DUVIDA, VISAO_DUVIDA };
static bool         g_evidencias[GAME_NUM_SUSPECTS][GAME_MAX_EVIDENCIAS] = { { false } };
static bool         g_viesUsado[GAME_NUM_SUSPECTS] = { false };
static bool         g_injecaoUsada[GAME_NUM_SUSPECTS] = { false };

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
    g_tutorialActive = true;
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
    return (g_day == 1) && g_tutorialActive;
}

void GameStateCompleteTutorial(void) {
    g_tutorialActive = false;
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
