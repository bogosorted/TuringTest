#ifndef PLAYER_CHARACTER_H
#define PLAYER_CHARACTER_H

#include "raylib.h"
#include <stdbool.h>

// Caixa de colisao 2D no plano do chao (XZ) para paredes, colunas e moveis.
typedef struct {
    float minX;
    float minZ;
    float maxX;
    float maxZ;
} PlayerColliderBox;

typedef enum {
    PLAYER_ANIM_NONE = 0,
    PLAYER_ANIM_DOOR,      // nao bloqueia movimento: estende a mao na macaneta
    PLAYER_ANIM_PAPER,     // mexendo no papel / documento / pasta
    PLAYER_ANIM_FILE,      // mexendo no arquivo / gaveta
    PLAYER_ANIM_COMPUTER,  // senta na cadeira e acessa o terminal
    PLAYER_ANIM_PAPER_WALK // caminha ate a bolinha de papel e pega
} PlayerInteractAnim;

// Personagem 3D realista com shader noir e animacao fluida controlado por WASD / Setas.
typedef struct {
    Vector3 position;      // posicao no piso (ou degrau em y)
    Vector2 velocity;      // velocidade atual no plano XZ (inercia fluida)
    float   radius;        // raio de colisao no plano XZ
    float   speed;         // velocidade maxima de deslocamento (unidades/s)
    float   facingYaw;     // rotacao em torno de Y (0 = +Z)
    float   walkCycle;     // fase do ciclo de caminhada
    float   moveBlend;     // fator suave 0..1 entre parado e andando
    float   runBlend;      // fator suave 0..1 entre caminhada e corrida (Shift)
    float   idleTime;      // acumulador para respiracao sutil em idle
    float   interactTimer; // animacao de interacao
    float   interactDur;   // duracao total da animacao atual
    PlayerInteractAnim interactType;
    Vector3 interactStartPos;
    Vector3 interactSeatPos;
    Vector3 interactLookPos;
    float   computerExitTimer; // animacao de sair do computador
    Vector3 moveTarget;        // proximo waypoint de movimento no modo mouse
    Vector3 moveGoalFinal;     // destino final do clique (para desenhar a seta estilo Dota)
    bool    hasMoveTarget;
    bool    mouseRunToggle;    // Shift alterna entre correr e andar no modo Mouse
    float   stuckTimer;
    bool    isMoving;
    bool    isFemale;      // true = modelo feminino (retratos 1,3,4); false = masculino (2,5,6)
} PlayerCharacter;

void PlayerCharacterInit(PlayerCharacter *p, Vector3 startPos, float startYaw);
void PlayerCharacterSetPose(PlayerCharacter *p, Vector3 pos, float yaw);
void PlayerCharacterSetMoveTarget(PlayerCharacter *p, Vector3 target);
void PlayerCharacterSetMoveGoalFinal(PlayerCharacter *p, Vector3 goalFinal);
void PlayerCharacterClearMoveTarget(PlayerCharacter *p);
void PlayerCharacterTriggerInteract(PlayerCharacter *p, Vector3 targetPos);
void PlayerCharacterTriggerInteractEx(PlayerCharacter *p, Vector3 targetPos, PlayerInteractAnim tipo, float duracao);
void PlayerCharacterTriggerComputerSit(PlayerCharacter *p, Vector3 seatPos, Vector3 monitorPos, float duracao);
void PlayerCharacterTriggerComputerExit(PlayerCharacter *p);
void PlayerCharacterTriggerPaperPickup(PlayerCharacter *p, Vector3 paperPos, float duracao);
void PlayerCharacterTriggerPaperDrop(PlayerCharacter *p, Vector3 paperPos, float duracao);
float PlayerCharacterGetChairPull(const PlayerCharacter *p, Vector3 chairPos);
void PlayerCharacterSetShadowBounds(Rectangle boundsXZ);

// Atualiza movimento relativo a camera e resolve colisoes contra limites da sala e obstaculos.
void PlayerCharacterUpdate(PlayerCharacter *p,
                           float dt,
                           Vector3 camPos,
                           Vector3 camTarget,
                           Rectangle roomBoundsXZ,
                           const PlayerColliderBox *obstacles,
                           int numObstacles,
                           bool inputBloqueado);

// Atividades contextuais para suspeitos e funcionarios nas salas 3D
typedef enum {
    NPC_ATIV_EM_PE = 0,
    NPC_ATIV_SENTADO,
    NPC_ATIV_COMPUTADOR,
    NPC_ATIV_COMENDO,
    NPC_ATIV_LENDO,
    NPC_ATIV_CARREGANDO_PAPEIS,
    NPC_ATIV_DESCONFIADA,
    NPC_ATIV_CONVERSANDO,
    NPC_ATIV_CANSADO,
    NPC_ATIV_CAIXA,
    NPC_ATIV_COCANDO_CABECA
} NpcAtividade;

// Desenha o personagem 3D com iluminacao noir direcional + rim light.
typedef enum {
    PREVIEW_ANIM_IDLE = 0,
    PREVIEW_ANIM_WALK,
    PREVIEW_ANIM_RUN,
    PREVIEW_ANIM_COMPUTER,
    PREVIEW_ANIM_PAPER,
    PREVIEW_ANIM_DOOR,
    PREVIEW_ANIM_COUNT
} PlayerPreviewAnim;

void PlayerCharacterDraw3D(const PlayerCharacter *p, Vector3 camPos);
void PlayerCharacterDrawModelPreview3D(bool isFemale, Vector3 pos, float yawDeg, float tempo, Vector3 camPos);
void PlayerCharacterDrawVariant3D(int variantId, PlayerPreviewAnim anim, Vector3 pos, float yawDeg, float tempo, Vector3 camPos);
void PlayerCharacterDrawTechVariant3D(int techId, PlayerPreviewAnim anim, Vector3 pos, float yawDeg, float tempo, Vector3 camPos);
void PlayerCharacterDrawSuspect3D(int suspeito, Vector3 pos, float yawDeg, float tempo, Vector3 camPos);
void PlayerCharacterDrawSuspect3DEx(int suspeito, Vector3 pos, float yawDeg, NpcAtividade ativ, float tempo, Vector3 camPos);
void PlayerCharacterDrawNpc3D(Vector3 pos, float yawDeg, bool sentado, float tempo, Vector3 camPos);
void PlayerCharacterDrawNpc3DEx(Vector3 pos, float yawDeg, NpcAtividade ativ, int variante, float tempo, Vector3 camPos);
void PlayerCharacterUnload(void);

#endif
