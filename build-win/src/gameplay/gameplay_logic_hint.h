#ifndef GAMEPLAY_LOGIC_HINT_H
#define GAMEPLAY_LOGIC_HINT_H

#include "raylib.h"
#include <stdbool.h>

// Inicializa / reseta estado da Dica Logica no inicio da cena ou de um novo dia
void LogicHintInit(Texture2D iconAceso);
void LogicHintResetDay(void);
void LogicHintResetRun(void);
void LogicHintUnload(void);

// Controle da entrevista com tokens infinitos (concedida ao acertar a dica)
void LogicHintEndInterview(void);

// Estado e interacao
bool LogicHintIsAvailable(bool dialogLoaded);
bool LogicHintIsOpen(void);
void LogicHintOpen(bool dialogLoaded, bool *tutDicaVisto);
void LogicHintClose(void);

bool LogicHintUpdate(void);
void LogicHintDraw(const char *suspectName);

#endif // GAMEPLAY_LOGIC_HINT_H
