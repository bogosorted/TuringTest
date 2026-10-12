#ifndef INTRO_SCENE_H
#define INTRO_SCENE_H

#include <stdbool.h>

void InitIntroScene(void);
void UpdateDrawIntroScene(void);
void UnloadIntroScene(void);

bool IntroSceneShouldContinue(void);

#endif
