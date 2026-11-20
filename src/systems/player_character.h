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

// Personagem 3D realista com shader noir e animacao fluida controlado por WASD / Setas.
typedef struct {
    Vector3 position;      // posicao no piso (ou degrau em y)
    Vector2 velocity;      // velocidade atual no plano XZ (inercia fluida)
    float   radius;        // raio de colisao no plano XZ
    float   speed;         // velocidade maxima de deslocamento (unidades/s)
    float   facingYaw;     // rotacao em torno de Y (0 = +Z)
    float   walkCycle;     // fase do ciclo de caminhada
    float   moveBlend;     // fator suave 0..1 entre parado e andando
    float   idleTime;      // acumulador para respiracao sutil em idle
    float   interactTimer; // animacao de estender a mao ao interagir
    bool    isMoving;
    bool    isFemale;      // true = modelo feminino (retratos 1,3,4); false = masculino (2,5,6)
} PlayerCharacter;

void PlayerCharacterInit(PlayerCharacter *p, Vector3 startPos, float startYaw);
void PlayerCharacterSetPose(PlayerCharacter *p, Vector3 pos, float yaw);
void PlayerCharacterTriggerInteract(PlayerCharacter *p, Vector3 targetPos);

// Atualiza movimento relativo a camera e resolve colisoes contra limites da sala e obstaculos.
void PlayerCharacterUpdate(PlayerCharacter *p,
                           float dt,
                           Vector3 camPos,
                           Vector3 camTarget,
                           Rectangle roomBoundsXZ,
                           const PlayerColliderBox *obstacles,
                           int numObstacles);

// Desenha o personagem 3D com iluminacao noir direcional + rim light.
void PlayerCharacterDraw3D(const PlayerCharacter *p, Vector3 camPos);
void PlayerCharacterUnload(void);

#endif
