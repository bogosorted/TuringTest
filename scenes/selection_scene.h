#ifndef SELECTION_SCENE_H
#define SELECTION_SCENE_H

#include <stdbool.h>

// Funcionarios que o jogador pode escolher para interrogar
typedef enum {
    EMPLOYEE_ALPHA,
    EMPLOYEE_BETA,
    EMPLOYEE_GAMMA
} Employee;

void InitSelectionScene(void);
void UpdateDrawSelectionScene(void);
void UnloadSelectionScene(void);

// true quando o jogador escolheu um funcionario (hora de ir pro interrogatorio)
bool SelectionSceneShouldStartInterrogation(void);

// Qual funcionario foi escolhido (valido depois que ShouldStartInterrogation for true)
Employee SelectionSceneGetEmployee(void);

// true quando o jogador clicou em "Seguir para o proximo dia"
bool SelectionSceneShouldAdvanceDay(void);

// true quando o jogador clicou em "Seguir para o Veredito" (so aparece no ultimo dia)
bool SelectionSceneShouldGoToVerdict(void);

#endif
