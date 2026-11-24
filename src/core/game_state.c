#include "game_state.h"
#include "settings.h"
#include "raylib.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int          g_day = 1;
static bool         g_tutorialConfig = true; // Por padrao ativado no setup
static bool         g_tutorialActive = true;
static VisaoJogador g_visaoJogador[GAME_NUM_SUSPECTS] = { VISAO_DUVIDA, VISAO_DUVIDA, VISAO_DUVIDA };
static bool         g_evidencias[GAME_NUM_SUSPECTS][GAME_MAX_EVIDENCIAS] = { { false } };
static uint64_t     g_dialogFlags = 0;
static bool         g_conversouNoDia[GAME_NUM_SUSPECTS][GAME_NUM_DAYS + 1] = { { false } };
static bool         g_viesUsado[GAME_NUM_SUSPECTS] = { false };
static int          g_viesDia[GAME_NUM_SUSPECTS] = { 0 };
static bool         g_deepfakeUsado[GAME_NUM_SUSPECTS] = { false };
static int          g_deepfakeDia[GAME_NUM_SUSPECTS] = { 0 };
static bool         g_injecaoUsada[GAME_NUM_SUSPECTS] = { false };
static int          g_injecaoDia[GAME_NUM_SUSPECTS] = { 0 };
static char           g_playerName[9] = "AGENTE";
static int            g_playerAvatar = 0;
static TomVozJogador  g_playerVoiceTone = VOZ_SUAVE;
static ModoJogo       g_gameMode = MODO_3D;
static bool           g_hardMode = false;
static ControleModo3D g_controle3D = CONTROLE_AMBOS;
static int            g_evUnlockDay[6] = { 1, 1, 2, 2, 3, 1 };
static int            g_ev06CollectedDay = 0;
static float          g_techNightHours = 8.0f;
static int            g_tokensRestantesTotal = 0;
static int            g_dicasUsadasTotal = 0;

static int ParseEvIndex(const char *ev_id) {
    if (!ev_id) return -1;
    // Aceita "EV-01" .. "EV-16"
    if (strncmp(ev_id, "EV-", 3) == 0) {
        int num = atoi(ev_id + 3);
        if (num >= 1 && num <= GAME_MAX_EVIDENCIAS) {
            return num - 1;
        }
    }
    return -1;
}

void GameStateSetMode(ModoJogo modo) {
    g_gameMode = (modo == MODO_2D) ? MODO_2D : MODO_3D;
}

ModoJogo GameStateGetMode(void) {
    return g_gameMode;
}

bool GameStateIs3DMode(void) {
    return g_gameMode == MODO_3D;
}

void GameStateSetHardMode(bool hard) {
    g_hardMode = hard;
}

bool GameStateIsHardMode(void) {
    return g_hardMode;
}

void GameStateSetControle3D(ControleModo3D ctrl) {
    if (ctrl < CONTROLE_TECLADO || ctrl > CONTROLE_AMBOS) ctrl = CONTROLE_AMBOS;
    g_controle3D = ctrl;
}

ControleModo3D GameStateGetControle3D(void) {
    return g_controle3D;
}

void GameStateNewRun(void) {
    g_tutorialConfig = SettingsGetTutorial();
    g_tutorialActive = g_tutorialConfig;
    g_day = 1; // Inicia direto no Dia 1; tutorial aparece de forma contextual durante a partida
    g_techNightHours = 8.0f;
    g_ev06CollectedDay = 0;
    g_tokensRestantesTotal = 0;
    g_dicasUsadasTotal = 0;
    g_dialogFlags = 0;
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        g_visaoJogador[i] = VISAO_DUVIDA;
        g_viesUsado[i] = false;
        g_viesDia[i] = 0;
        g_deepfakeUsado[i] = false;
        g_deepfakeDia[i] = 0;
        g_injecaoUsada[i] = false;
        g_injecaoDia[i] = 0;
        for (int d = 0; d <= GAME_NUM_DAYS; d++) {
            g_conversouNoDia[i][d] = false;
        }
        for (int e = 0; e < GAME_MAX_EVIDENCIAS; e++) {
            g_evidencias[i][e] = false;
        }
    }
    // Randomiza em qual dia (1, 2 ou 3) as evidencias 0..4 ficam disponiveis; EV-06 (5) fica disponivel todos os dias
    int order[5] = { 0, 1, 2, 3, 4 };
    for (int i = 4; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
    for (int i = 0; i < 5; i++) {
        g_evUnlockDay[order[i]] = 1 + (i / 2);
    }
    g_evUnlockDay[5] = 1;
}

void GameStateNextDay(void) {
    if (g_day < GAME_NUM_DAYS) g_day++;
}

void GameStateUpdateTechNightClock(float dt) {
    if (dt <= 0.0f) return;
    if (dt > 0.05f) dt = 0.05f;
    // Tech Night (1 semana = 3 dias, no maximo 8 minutos totais com margem de ~30s pro Veredito):
    // 450s nos 3 dias (~150s por dia = 3h no jogo por dia, avancando de 08:00 ate 17:00)
    float dayFloor = 8.0f + (float)(g_day - 1) * 3.0f;
    float dayCeil  = dayFloor + 3.0f;
    if (dayCeil > 17.0f) dayCeil = 17.0f;
    if (g_techNightHours < dayFloor) {
        // Alcanca suavemente o inicio do novo dia caso o jogador conclua mais rapido
        g_techNightHours += (dayFloor - g_techNightHours) * 0.90f * dt + 0.12f * dt;
        if (g_techNightHours > dayFloor) g_techNightHours = dayFloor;
    } else if (g_techNightHours < dayCeil) {
        g_techNightHours += (3.0f / 150.0f) * dt;
        if (g_techNightHours > dayCeil) g_techNightHours = dayCeil;
    }
}

float GameStateGetTechNightClockHours(void) {
    return g_techNightHours;
}

int GameStateGetDay(void) {
    return g_day;
}

bool GameStateIsTutorialActive(void) {
    return g_tutorialConfig && g_tutorialActive;
}

void GameStateCompleteTutorial(void) {
    g_tutorialActive = false;
}

bool GameStateGetTutorialConfig(void) {
    return g_tutorialConfig;
}

void GameStateSetTutorialConfig(bool enabled) {
    g_tutorialConfig = enabled;
    g_tutorialActive = enabled;
}

void GameStateSetVisaoJogador(int suspeito, VisaoJogador visao) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_visaoJogador[suspeito] = visao;
}

VisaoJogador GameStateGetVisaoJogador(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return VISAO_DUVIDA;
    return g_visaoJogador[suspeito];
}

bool GameStateRegistrarEvidencia(int suspeito, const char *ev_id) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    int idx = ParseEvIndex(ev_id);
    if (idx < 0) return false;
    if (g_evidencias[suspeito][idx]) return false;
    g_evidencias[suspeito][idx] = true;
    if (suspeito == 2 && idx == 5) {
        g_ev06CollectedDay = g_day;
    }
    return true;
}

int GameStateGetEv06CollectedDay(void) {
    return g_ev06CollectedDay;
}

bool GameStateTemEvidencia(int suspeito, const char *ev_id) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    int idx = ParseEvIndex(ev_id);
    if (idx < 0) return false;
    return g_evidencias[suspeito][idx];
}

bool GameStateEvidenciaDisponivelHoje(int suspeito, const char *ev_id) {
    (void)suspeito;
    int idx = ParseEvIndex(ev_id);
    if (idx < 0 || idx >= 6) return true;
    return g_day >= g_evUnlockDay[idx];
}

void GameStateSetDialogFlag(int flagBit) {
    if (flagBit <= 0 || flagBit >= 64) return;
    g_dialogFlags |= (1ULL << (unsigned int)flagBit);
}

bool GameStateHasDialogFlag(int flagBit) {
    if (flagBit <= 0 || flagBit >= 64) return true;
    return (g_dialogFlags & (1ULL << (unsigned int)flagBit)) != 0;
}

void GameStateRegistrarConversa(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    if (g_day >= 1 && g_day <= GAME_NUM_DAYS) {
        g_conversouNoDia[suspeito][g_day] = true;
    }
}

bool GameStateJaConversou(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    for (int d = 1; d <= GAME_NUM_DAYS; d++) {
        if (g_conversouNoDia[suspeito][d]) return true;
    }
    return false;
}

bool GameStateJaConversouAntesDeHoje(int suspeito) {
    return GameStateUltimoDiaConversaAnterior(suspeito) > 0;
}

int GameStateUltimoDiaConversaAnterior(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return 0;
    for (int d = g_day - 1; d >= 1; d--) {
        if (g_conversouNoDia[suspeito][d]) return d;
    }
    return 0;
}

bool GameStateConversouNoDia(int suspeito, int dia) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    if (dia < 1 || dia > GAME_NUM_DAYS) return false;
    return g_conversouNoDia[suspeito][dia];
}

void GameStateSetViesUsado(int suspeito, bool usado) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_viesUsado[suspeito] = usado;
    if (usado && g_viesDia[suspeito] == 0) g_viesDia[suspeito] = g_day;
    if (!usado) g_viesDia[suspeito] = 0;
}

bool GameStateGetViesUsado(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    return g_viesUsado[suspeito];
}

int GameStateGetViesDia(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return 0;
    return g_viesDia[suspeito];
}

void GameStateSetDeepfakeUsado(int suspeito, bool usado) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_deepfakeUsado[suspeito] = usado;
    if (usado && g_deepfakeDia[suspeito] == 0) g_deepfakeDia[suspeito] = g_day;
    if (!usado) g_deepfakeDia[suspeito] = 0;
}

bool GameStateGetDeepfakeUsado(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    return g_deepfakeUsado[suspeito];
}

int GameStateGetDeepfakeDia(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return 0;
    return g_deepfakeDia[suspeito];
}

void GameStateSetInjecaoUsada(int suspeito, bool usada) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return;
    g_injecaoUsada[suspeito] = usada;
    if (usada && g_injecaoDia[suspeito] == 0) g_injecaoDia[suspeito] = g_day;
    if (!usada) g_injecaoDia[suspeito] = 0;
}

bool GameStateGetInjecaoUsada(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return false;
    return g_injecaoUsada[suspeito];
}

int GameStateGetInjecaoDia(int suspeito) {
    if (suspeito < 0 || suspeito >= GAME_NUM_SUSPECTS) return 0;
    return g_injecaoDia[suspeito];
}

int GameStateCountSpecialToolsUsed(void) {
    int count = 0;
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        if (g_viesUsado[i]) count++;
        if (g_deepfakeUsado[i]) count++;
        if (g_injecaoUsada[i]) count++;
    }
    return count;
}

void GameStateSetPlayerName(const char *name) {
    int j = 0;
    if (name) {
        for (int i = 0; name[i] != '\0' && j < 8; i++) {
            unsigned char c = (unsigned char)name[i];
            if (c >= 33 && c <= 126 && c != '=') {
                g_playerName[j++] = (char)c;
            }
        }
    }
    if (j == 0) {
        strcpy(g_playerName, "AGENTE");
    } else {
        g_playerName[j] = '\0';
    }
}

const char *GameStateGetPlayerName(void) {
    return g_playerName;
}

void GameStateSetPlayerAvatar(int avatarIdx) {
    if (avatarIdx < 0) avatarIdx = 0;
    if (avatarIdx >= GAME_NUM_PLAYERS) avatarIdx = GAME_NUM_PLAYERS - 1;
    g_playerAvatar = avatarIdx;
}

int GameStateGetPlayerAvatar(void) {
    return g_playerAvatar;
}

bool GameStateIsPlayerFemale(void) {
    // Retratos 0, 2 e 3 (Players 1, 3 e 4) sao femininos; 1, 4 e 5 (Players 2, 5 e 6) sao masculinos
    return (g_playerAvatar == 0 || g_playerAvatar == 2 || g_playerAvatar == 3);
}

void GameStateSetPlayerVoiceTone(TomVozJogador tom) {
    if (tom < VOZ_GRAVE || tom > VOZ_SUAVE) tom = VOZ_MEDIA;
    g_playerVoiceTone = tom;
}

TomVozJogador GameStateGetPlayerVoiceTone(void) {
    return g_playerVoiceTone;
}

void GameStateAddRemainingTokens(int tokens) {
    if (tokens > 0) g_tokensRestantesTotal += tokens;
}

int GameStateGetRemainingTokensTotal(void) {
    return g_tokensRestantesTotal;
}

void GameStateRegisterHintUsed(void) {
    if (g_dicasUsadasTotal < GAME_NUM_DAYS) g_dicasUsadasTotal++;
}

int GameStateGetUnusedHintsCount(void) {
    int unused = GAME_NUM_DAYS - g_dicasUsadasTotal;
    return (unused > 0) ? unused : 0;
}

bool GameStateAnyViesUsed(void) {
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        if (g_viesUsado[i]) return true;
    }
    return false;
}

bool GameStateAnyInjecaoUsed(void) {
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        if (g_injecaoUsada[i]) return true;
    }
    return false;
}

int GameStateCountEvidences(void) {
    int total = 0;
    for (int i = 0; i < GAME_NUM_SUSPECTS; i++) {
        for (int e = 0; e < GAME_MAX_EVIDENCIAS; e++) {
            if (g_evidencias[i][e]) total++;
        }
    }
    return total;
}
