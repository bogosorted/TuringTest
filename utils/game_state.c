#include "game_state.h"

static int  g_day = 1;
static bool g_tutorialActive = true;

void GameStateNewRun(void) {
    g_day = 1;
    g_tutorialActive = true;
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
