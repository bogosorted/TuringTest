#include "game_state.h"

static int g_day = 1;

void GameStateNewRun(void) {
    g_day = 1;
}

void GameStateNextDay(void) {
    if (g_day < GAME_NUM_DAYS) g_day++;
}

int GameStateGetDay(void) {
    return g_day;
}
