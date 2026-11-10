#ifndef SETTINGS_H
#define SETTINGS_H

#include "raylib.h"

// Configuracoes do jogador (menu de Opcoes).
// Volumes e tela cheia ficam salvos em "settings.cfg" na pasta de trabalho do jogo
// (a pasta do executavel, normalmente build/) e sao lidos ao abrir o jogo.
// A paleta vale so enquanto o jogo esta aberto: sempre comeca na Original.

#define PALETTE_NUM_STOPS 5   // tons de cada paleta, do mais escuro ao mais claro

void SettingsLoad(void);      // le settings.cfg (se nao existir, usa os padroes)
void SettingsSave(void);      // grava settings.cfg

// Volumes de 0.0 (mudo) a 1.0 (maximo)
float SettingsGetMusicVolume(void);
void  SettingsSetMusicVolume(float v);
float SettingsGetSfxVolume(void);    // multiplica o volume de todos os efeitos sonoros
void  SettingsSetSfxVolume(float v);

// Tela cheia (janela sem borda do tamanho do monitor) ou modo janela (1280x720)
bool SettingsGetFullscreen(void);
void SettingsSetFullscreen(bool ligado);
void SettingsApplyWindowMode(void);   // aplica na janela (chamar depois do InitWindow)

// Paletas de cor. A 0 e a original do jogo (sem pos-processamento).
int          SettingsPaletteCount(void);
int          SettingsGetPalette(void);
void         SettingsSetPalette(int indice);          // da a volta (aceita -1 e count)
const char  *SettingsPaletteName(int indice);
const Color *SettingsPaletteStops(int indice);        // PALETTE_NUM_STOPS cores
const Color *SettingsOriginalStops(void);             // rampa original (paleta 0)

#endif
