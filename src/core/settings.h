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

// Tutorial no Dia 1 (por padrao ativado)
bool SettingsGetTutorial(void);
void SettingsSetTutorial(bool ligado);

// Modo Desenvolvedor / Dev Tools (persiste ate ser desativado manualmente)
bool SettingsGetDevMode(void);
void SettingsSetDevMode(bool ligado);

// Efeito CRT (TV de tubo)
bool SettingsGetCRT(void);
void SettingsSetCRT(bool ligado);

// Opcoes exclusivas do Modo Dev (ambas OFF por padrao):
// - Auto-Reset: ao iniciar, reseta configuracoes/paleta e inicia com CRT ligado
// - Tech Night: temporizador suave 08:00 -> 17:00 (permanece ativo mesmo se Dev Mode for desligado)
bool SettingsGetAutoReset(void);
void SettingsSetAutoReset(bool ligado);
void SettingsResetToDefaultWithCRT(void);
bool SettingsGetTechNight(void);
void SettingsSetTechNight(bool ligado);
bool SettingsGetDevUnlockSubsolo(void);
void SettingsSetDevUnlockSubsolo(bool ligado);
bool SettingsGetDevUnlockUpperFloors(void);
void SettingsSetDevUnlockUpperFloors(bool ligado);

// Reinicio completo da aplicacao (usado pelo botao Dev Mode)
void SettingsRequestAppRestart(void);
bool SettingsIsAppRestartRequested(void);
bool SettingsConsumeAppRestart(void);

// Camera 3D: false = padrao (segue o player), true = fixa com tracking
bool SettingsGetCamera3DFixa(void);
void SettingsSetCamera3DFixa(bool fixa);

// Paletas de cor. A 0 e a original do jogo (sem pos-processamento).
int          SettingsPaletteCount(void);
int          SettingsGetPalette(void);
void         SettingsSetPalette(int indice);          // da a volta (aceita -1 e count)
const char  *SettingsPaletteName(int indice);
const Color *SettingsPaletteStops(int indice);        // PALETTE_NUM_STOPS cores
const Color *SettingsOriginalStops(void);             // rampa original (paleta 0)

#endif
