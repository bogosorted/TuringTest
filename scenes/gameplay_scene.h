#ifndef GAMEPLAY_SCENE_H
#define GAMEPLAY_SCENE_H

#include <stdbool.h>

void InitGameplayScene(void);
void UpdateDrawGameplayScene(void);
void UnloadGameplayScene(void);
bool GameplaySceneShouldClose(void);

#endif