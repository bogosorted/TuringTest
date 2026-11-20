#ifndef INTRO_SCENE_H
#define INTRO_SCENE_H

#include <stdbool.h>

bool InitIntroScene(void);
void UpdateDrawIntroScene(void);
void UnloadIntroScene(void);

bool IntroSceneShouldContinue(void);

#endif
