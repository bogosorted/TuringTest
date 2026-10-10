#ifndef GAME_STATE_H
#define GAME_STATE_H

// Quantos dias de interrogatorio existem antes do veredito
#define GAME_NUM_DAYS 3

void GameStateNewRun(void);   // comeca uma partida nova (volta para o dia 1)
void GameStateNextDay(void);  // avanca um dia
int  GameStateGetDay(void);   // dia atual (1..GAME_NUM_DAYS)

#endif
