#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// Quantos dias de interrogatorio existem antes do veredito
#define GAME_NUM_DAYS 3
#define GAME_NUM_SUSPECTS 3
#define GAME_MAX_EVIDENCIAS 16
#define GAME_NUM_PLAYERS 6

typedef enum {
    VOZ_GRAVE = 0,
    VOZ_MEDIA = 1,
    VOZ_SUAVE = 2
} TomVozJogador;

typedef enum {
    MODO_2D = 0,
    MODO_3D = 1
} ModoJogo;

// Visao individual de cada personagem quanto ao jogador
typedef enum {
    VISAO_DUVIDA    = 0,
    VISAO_CONFIANCA = 1,
    VISAO_PRESSAO   = 2
} VisaoJogador;

void     GameStateSetMode(ModoJogo modo);
ModoJogo GameStateGetMode(void);
bool     GameStateIs3DMode(void);

void GameStateNewRun(void);   // comeca uma partida nova (volta para o dia 1)
void GameStateNextDay(void);  // avanca um dia
int  GameStateGetDay(void);   // dia atual (1..GAME_NUM_DAYS)

// Tutorial contextual (ativo durante a partida quando habilitado nas opcoes)
bool GameStateIsTutorialActive(void);
void GameStateCompleteTutorial(void);

// Configuracao de tutorial no setup (ativado por padrao)
bool GameStateGetTutorialConfig(void);
void GameStateSetTutorialConfig(bool enabled);

// Visao individual de cada suspeito (0=Maria, 1=Arthur, 2=Ana Lucia)
void         GameStateSetVisaoJogador(int suspeito, VisaoJogador visao);
VisaoJogador GameStateGetVisaoJogador(int suspeito);

// Registro de evidencias vistas (em dialogos ou locais)
// Retorna true se a evidencia ainda nao estava registrada (recem-adquirida)
bool GameStateRegistrarEvidencia(int suspeito, const char *ev_id);
bool GameStateTemEvidencia(int suspeito, const char *ev_id);

// Rastreia se o jogador usou Enviesamento, Deepfake ou Injecao contra cada suspeito na run
void GameStateSetViesUsado(int suspeito, bool usado);
bool GameStateGetViesUsado(int suspeito);
void GameStateSetDeepfakeUsado(int suspeito, bool usado);
bool GameStateGetDeepfakeUsado(int suspeito);
void GameStateSetInjecaoUsada(int suspeito, bool usada);
bool GameStateGetInjecaoUsada(int suspeito);
int  GameStateCountSpecialToolsUsed(void);

// Nome, retrato (0..5), genero do retrato e tom de voz do jogador, alem das estatisticas de pontuacao da run
void          GameStateSetPlayerName(const char *name);
const char   *GameStateGetPlayerName(void);
void          GameStateSetPlayerAvatar(int avatarIdx);
int           GameStateGetPlayerAvatar(void);
bool          GameStateIsPlayerFemale(void);
void          GameStateSetPlayerVoiceTone(TomVozJogador tom);
TomVozJogador GameStateGetPlayerVoiceTone(void);
void           GameStateAddRemainingTokens(int tokens);
int         GameStateGetRemainingTokensTotal(void);
void        GameStateRegisterHintUsed(void);
int         GameStateGetUnusedHintsCount(void);
bool        GameStateAnyViesUsed(void);
bool        GameStateAnyInjecaoUsed(void);
int         GameStateCountEvidences(void);

#endif
