#ifndef MENU_SCENE_H
#define MENU_SCENE_H

#include <stdbool.h>

bool InitMenuScene(void);
void UpdateDrawMenuScene(void);
void UnloadMenuScene(void);
bool MenuSceneShouldClose(void);
bool MenuSceneShouldStartGame(void);


#endif
