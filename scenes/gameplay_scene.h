#ifndef GAMEPLAY_SCENE_H
#define GAMEPLAY_SCENE_H

#include <stdbool.h>

void InitGameplayScene(void);
void UpdateDrawGameplayScene(void);
void UnloadGameplayScene(void);
bool GameplaySceneShouldClose(void);
void GameplaySceneSetFuncionario(int indice);
bool GameplaySceneGoToSelection(void);
bool GameplaySceneShouldAdvanceDay(void);
bool GameplaySceneShouldGoToVerdict(void);
bool GameplaySceneShouldReturnToMenu(void);

// Ciclo de dias: recarrega os dialogos (todos voltam a "nao vistos")
void GameplaySceneResetDialogues(void);

// true quando o dia pode terminar: tokens acabaram OU nao ha mais escolhas nos 3 funcionarios
bool GameplaySceneDayFinished(void);

// Todos os itens da prancheta para um suspeito (0 = Maria Bianca, 1 = Arthur, 2 = Ana Lucia),
// com "marcados[i]" dizendo se foi marcado ao longo dos dias. Devolve quantos itens existem.
int GameplaySceneItensPrancheta(int suspeito, const char **itens, bool *marcados, int max);

#endif