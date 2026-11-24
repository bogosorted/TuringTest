#ifndef VERDICT_SCENE_H
#define VERDICT_SCENE_H

#include <stdbool.h>

bool InitVerdictScene(void);
void UpdateDrawVerdictScene(void);
void UnloadVerdictScene(void);

// true quando o jogador clicou em "Tentar de novo" (volta para o menu)
bool VerdictSceneShouldRestart(void);

// true enquanto a musica de suspense do veredito esta ativa
bool VerdictSceneSuspenseAtivo(void);

#endif
