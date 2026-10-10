#ifndef GAMEPLAY_SCENE_H
#define GAMEPLAY_SCENE_H

#include <stdbool.h>

void InitGameplayScene(void);
void UpdateDrawGameplayScene(void);
void UnloadGameplayScene(void);
bool GameplaySceneShouldClose(void);
void GameplaySceneSetFuncionario(int indice);
bool GameplaySceneGoToSelection(void);

// Ciclo de dias: recarrega os dialogos (todos voltam a "nao vistos")
void GameplaySceneResetDialogues(void);

// true quando o dia pode terminar: tokens acabaram OU nao ha mais escolhas nos 3 funcionarios
bool GameplaySceneDayFinished(void);

#endif