#ifndef DIALOG_DATA_H
#define DIALOG_DATA_H

#include "dialog_system.h"

// Carrega a arvore de dialogo de um suspeito (0=Maria, 1=Arthur, 2=Ana Lucia) para o dia (1..3)
// Ja aplica a fala de abertura de acordo com GameStateGetVisaoJogador(suspeito).
bool LoadDialogTreeFromData(int suspeito, int day, DialogTree *tree);

// Retorna o nome legivel de uma evidencia (ex: "EV-01" -> "Ficha de RH consta apenas pai")
const char* GetEvidenciaNome(int suspeito, const char *ev_id);

// Lista as evidencias definidas para um suspeito (0=Maria, 1=Arthur, 2=Ana Lucia)
int         GetEvidenciaCount(int suspeito);
const char* GetEvidenciaIdByIndex(int suspeito, int index);

#endif // DIALOG_DATA_H
