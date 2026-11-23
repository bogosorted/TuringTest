#include "player_character.h"
#include "../core/game_state.h"
#include "../core/settings.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ============================================================================
// Colisoes 2D no plano XZ
// ============================================================================

static float WrapAngleRad(float a) {
    while (a > PI)  a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

static void ResolverColisaoCirculoAABB(float *cx, float *cz, float radius, PlayerColliderBox box) {
    float closestX = Clamp(*cx, box.minX, box.maxX);
    float closestZ = Clamp(*cz, box.minZ, box.maxZ);
    float dx = *cx - closestX;
    float dz = *cz - closestZ;
    float distSq = dx * dx + dz * dz;

    if (distSq >= radius * radius) return;

    if (distSq > 1e-8f) {
        float dist = sqrtf(distSq);
        float push = radius - dist;
        *cx += (dx / dist) * push;
        *cz += (dz / dist) * push;
        return;
    }

    float leftPen  = *cx - box.minX;
    float rightPen = box.maxX - *cx;
    float frontPen = *cz - box.minZ;
    float backPen  = box.maxZ - *cz;

    float minPen = leftPen;
    int side = 0;
    if (rightPen < minPen) { minPen = rightPen; side = 1; }
    if (frontPen < minPen) { minPen = frontPen; side = 2; }
    if (backPen  < minPen) { side = 3; }

    if (side == 0)      *cx = box.minX - radius;
    else if (side == 1) *cx = box.maxX + radius;
    else if (side == 2) *cz = box.minZ - radius;
    else                *cz = box.maxZ + radius;
}

static void ResolverPosicaoXZ(float *x, float *z, float radius,
                              Rectangle bounds,
                              const PlayerColliderBox *obstacles, int numObstacles) {
    float minBX = bounds.x + radius;
    float maxBX = bounds.x + bounds.width - radius;
    float minBZ = bounds.y + radius;
    float maxBZ = bounds.y + bounds.height - radius;

    for (int pass = 0; pass < 2; pass++) {
        if (obstacles && numObstacles > 0) {
            for (int i = 0; i < numObstacles; i++) {
                ResolverColisaoCirculoAABB(x, z, radius, obstacles[i]);
            }
        }
        *x = Clamp(*x, minBX, maxBX);
        *z = Clamp(*z, minBZ, maxBZ);
    }
}

// ============================================================================
// Locomocao Fluida
// ============================================================================

void PlayerCharacterInit(PlayerCharacter *p, Vector3 startPos, float startYaw) {
    if (!p) return;
    p->position          = startPos;
    p->velocity          = (Vector2){ 0.0f, 0.0f };
    p->radius            = 0.28f;
    p->speed             = 2.95f;
    p->facingYaw         = startYaw;
    p->walkCycle         = 0.0f;
    p->moveBlend         = 0.0f;
    p->idleTime          = 0.0f;
    p->interactTimer     = 0.0f;
    p->interactDur       = 0.42f;
    p->interactType      = PLAYER_ANIM_NONE;
    p->computerExitTimer = 0.0f;
    p->isMoving          = false;
    p->isFemale          = false;
}

void PlayerCharacterSetPose(PlayerCharacter *p, Vector3 pos, float yaw) {
    if (!p) return;
    p->position          = pos;
    p->velocity          = (Vector2){ 0.0f, 0.0f };
    p->facingYaw         = yaw;
    p->walkCycle         = 0.0f;
    p->moveBlend         = 0.0f;
    p->runBlend          = 0.0f;
    p->interactTimer     = 0.0f;
    p->interactDur       = 0.42f;
    p->interactType      = PLAYER_ANIM_NONE;
    p->computerExitTimer = 0.0f;
    p->moveTarget        = pos;
    p->moveGoalFinal     = pos;
    p->hasMoveTarget     = false;
    p->stuckTimer        = 0.0f;
    p->isMoving          = false;
}

void PlayerCharacterSetMoveTarget(PlayerCharacter *p, Vector3 target) {
    if (!p) return;
    p->moveTarget    = target;
    p->hasMoveTarget = true;
    p->stuckTimer    = 0.0f;
}

void PlayerCharacterSetMoveGoalFinal(PlayerCharacter *p, Vector3 goalFinal) {
    if (!p) return;
    p->moveGoalFinal = goalFinal;
}

void PlayerCharacterClearMoveTarget(PlayerCharacter *p) {
    if (!p) return;
    p->hasMoveTarget = false;
    p->stuckTimer    = 0.0f;
}

void PlayerCharacterTriggerInteractEx(PlayerCharacter *p, Vector3 targetPos, PlayerInteractAnim tipo, float duracao) {
    if (!p) return;
    float dx = targetPos.x - p->position.x;
    float dz = targetPos.z - p->position.z;
    if (dx * dx + dz * dz > 1e-4f) {
        p->facingYaw = atan2f(dx, dz);
    }
    p->interactType  = tipo;
    p->interactDur   = (duracao > 0.05f) ? duracao : 0.42f;
    p->interactTimer = p->interactDur;
}

void PlayerCharacterTriggerInteract(PlayerCharacter *p, Vector3 targetPos) {
    PlayerCharacterTriggerInteractEx(p, targetPos, PLAYER_ANIM_DOOR, 0.36f);
}

void PlayerCharacterTriggerComputerSit(PlayerCharacter *p, Vector3 seatPos, Vector3 monitorPos, float duracao) {
    if (!p) return;
    p->interactStartPos = p->position;
    p->interactSeatPos  = seatPos;
    p->interactLookPos  = monitorPos;
    p->interactType     = PLAYER_ANIM_COMPUTER;
    p->interactDur      = (duracao > 0.1f) ? duracao : 1.85f;
    p->interactTimer    = p->interactDur;
}

void PlayerCharacterTriggerComputerExit(PlayerCharacter *p) {
    if (!p) return;
    p->computerExitTimer = 0.72f;
}

float PlayerCharacterGetChairPull(const PlayerCharacter *p, Vector3 chairPos) {
    if (!p) return 0.0f;
    float dx = chairPos.x - p->interactSeatPos.x;
    float dz = chairPos.z - p->interactSeatPos.z;
    if (dx * dx + dz * dz > 0.09f) return 0.0f;

    if (p->interactType == PLAYER_ANIM_COMPUTER && p->interactTimer > 0.0f && p->interactDur > 0.01f) {
        float u = 1.0f - (p->interactTimer / p->interactDur);
        if (u >= 0.40f && u < 0.70f) return sinf(((u - 0.40f) / 0.30f) * PI * 0.5f) * 0.24f;
        if (u >= 0.70f && u < 0.96f) return (1.0f - (u - 0.70f) / 0.26f) * 0.24f;
        return 0.0f;
    }
    if (p->computerExitTimer > 0.0f) {
        float u = 1.0f - (p->computerExitTimer / 0.72f); // 0 -> 1 saindo
        return sinf(u * PI) * 0.24f;
    }
    return 0.0f;
}

void PlayerCharacterUpdate(PlayerCharacter *p,
                           float dt,
                           Vector3 camPos,
                           Vector3 camTarget,
                           Rectangle roomBoundsXZ,
                           const PlayerColliderBox *obstacles,
                           int numObstacles,
                           bool inputBloqueado) {
    if (!p) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;

    p->idleTime += dt;
    if (p->interactTimer > 0.0f) {
        p->interactTimer -= dt;
        if (p->interactTimer <= 0.0f) {
            p->interactTimer = 0.0f;
            p->interactType = PLAYER_ANIM_NONE;
        }
    }
    if (p->computerExitTimer > 0.0f) {
        p->computerExitTimer -= dt;
        if (p->computerExitTimer <= 0.0f) {
            p->computerExitTimer = 0.0f;
            p->moveBlend = 0.0f;
        } else {
            float u = 1.0f - (p->computerExitTimer / 0.72f);
            float stepOut = Clamp((u - 0.38f) / 0.62f, 0.0f, 1.0f);
            float smooth = stepOut * stepOut * (3.0f - 2.0f * stepOut);
            p->position.x = p->interactSeatPos.x + (p->interactStartPos.x - p->interactSeatPos.x) * smooth;
            p->position.z = p->interactSeatPos.z + (p->interactStartPos.z - p->interactSeatPos.z) * smooth;
            if (u > 0.38f) {
                p->walkCycle += dt * 7.5f;
                if (p->walkCycle > 2.0f * PI) p->walkCycle -= 2.0f * PI;
                p->moveBlend = sinf(stepOut * PI) * 0.65f;
            } else {
                p->moveBlend = 0.0f;
            }
        }
    }
    bool inComputerWalk = false;
    if (p->interactType == PLAYER_ANIM_COMPUTER && p->interactTimer > 0.0f && p->interactDur > 0.01f) {
        float u = 1.0f - (p->interactTimer / p->interactDur);
        if (u < 0.45f) {
            // Fase 1: caminha da posicao atual ate a cadeira
            inComputerWalk = true;
            float w = Clamp(u / 0.45f, 0.0f, 1.0f);
            float smooth = w * w * (3.0f - 2.0f * w);
            float dxW = p->interactSeatPos.x - p->interactStartPos.x;
            float dzW = p->interactSeatPos.z - p->interactStartPos.z;
            float distW = hypotf(dxW, dzW);
            p->position.x = p->interactStartPos.x + dxW * smooth;
            p->position.z = p->interactStartPos.z + dzW * smooth;
            if (distW > 0.06f) {
                float walkYaw = atan2f(dxW, dzW);
                float diffW = WrapAngleRad(walkYaw - p->facingYaw);
                p->facingYaw = WrapAngleRad(p->facingYaw + diffW * (1.0f - expf(-12.0f * dt)));
                p->walkCycle += dt * 8.5f;
                if (p->walkCycle > 2.0f * PI) p->walkCycle -= 2.0f * PI;
                p->moveBlend = sinf(w * PI) * 0.80f;
            } else {
                p->moveBlend *= (1.0f - expf(-12.0f * dt));
            }
        } else {
            // Fase 2 e 3: vira de frente para o monitor, puxa a cadeira e senta suavemente
            p->position.x = p->interactSeatPos.x;
            p->position.z = p->interactSeatPos.z;
            p->moveBlend *= (1.0f - expf(-14.0f * dt));
            float targetYaw = atan2f(p->interactLookPos.x - p->interactSeatPos.x,
                                     p->interactLookPos.z - p->interactSeatPos.z);
            float diffYaw = WrapAngleRad(targetYaw - p->facingYaw);
            p->facingYaw = WrapAngleRad(p->facingYaw + diffYaw * (1.0f - expf(-10.0f * dt)));
        }
    }

    bool travadoAnim = inputBloqueado ||
                       (p->interactTimer > 0.0f && p->interactType != PLAYER_ANIM_DOOR) ||
                       (p->computerExitTimer > 0.0f);

    ControleModo3D ctrlMode = GameStateGetControle3D();
    bool allowKeys = (ctrlMode == CONTROLE_TECLADO || ctrlMode == CONTROLE_AMBOS);
    if (!travadoAnim && ctrlMode == CONTROLE_MOUSE && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
        p->mouseRunToggle = !p->mouseRunToggle;
    }
    bool running = !travadoAnim && (allowKeys ? (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) : p->mouseRunToggle);
    float maxSpd = p->speed * (running ? 1.65f : 1.0f);

    float inRight = 0.0f;
    float inFwd   = 0.0f;
    bool fpAtivo = SettingsGetDevMode() && SettingsGetDevFirstPerson();
    if (!travadoAnim && allowKeys) {
        if (IsKeyDown(KEY_D) || (!fpAtivo && IsKeyDown(KEY_RIGHT))) inRight += 1.0f;
        if (IsKeyDown(KEY_A) || (!fpAtivo && IsKeyDown(KEY_LEFT)))  inRight -= 1.0f;
        if (IsKeyDown(KEY_W) || (!fpAtivo && IsKeyDown(KEY_UP)))    inFwd   += 1.0f;
        if (IsKeyDown(KEY_S) || (!fpAtivo && IsKeyDown(KEY_DOWN)))  inFwd   -= 1.0f;
    }

    Vector2 targetVel = { 0.0f, 0.0f };
    bool hasKeyInput = (inRight != 0.0f || inFwd != 0.0f);
    if (hasKeyInput) {
        p->hasMoveTarget = false;
        p->stuckTimer    = 0.0f;
    }
    bool hasInput = hasKeyInput || (!travadoAnim && p->hasMoveTarget);
    if (hasKeyInput) {
        Vector2 fwdXZ = { camTarget.x - camPos.x, camTarget.z - camPos.z };
        float fwdLen = sqrtf(fwdXZ.x * fwdXZ.x + fwdXZ.y * fwdXZ.y);
        if (fwdLen > 1e-4f) {
            fwdXZ.x /= fwdLen;
            fwdXZ.y /= fwdLen;
        } else {
            fwdXZ = (Vector2){ 0.0f, -1.0f };
        }
        Vector2 rightXZ = { -fwdXZ.y, fwdXZ.x };

        float moveX = rightXZ.x * inRight + fwdXZ.x * inFwd;
        float moveZ = rightXZ.y * inRight + fwdXZ.y * inFwd;
        float moveLen = sqrtf(moveX * moveX + moveZ * moveZ);
        if (moveLen > 1e-4f) {
            moveX /= moveLen;
            moveZ /= moveLen;
        }
        targetVel.x = moveX * maxSpd;
        targetVel.y = moveZ * maxSpd;

        float targetYaw = atan2f(moveX, moveZ);
        float diffYaw = WrapAngleRad(targetYaw - p->facingYaw);
        p->facingYaw = WrapAngleRad(p->facingYaw + diffYaw * (1.0f - expf(-12.5f * dt)));
    } else if (!travadoAnim && p->hasMoveTarget) {
        float dxM = p->moveTarget.x - p->position.x;
        float dzM = p->moveTarget.z - p->position.z;
        float distM = hypotf(dxM, dzM);
        if (distM <= 0.14f) {
            p->hasMoveTarget = false;
            p->stuckTimer    = 0.0f;
            hasInput         = false;
        } else {
            float dirX = dxM / distM;
            float dirZ = dzM / distM;
            targetVel.x = dirX * maxSpd;
            targetVel.y = dirZ * maxSpd;
            float targetYaw = atan2f(dirX, dirZ);
            float diffYaw = WrapAngleRad(targetYaw - p->facingYaw);
            p->facingYaw = WrapAngleRad(p->facingYaw + diffYaw * (1.0f - expf(-12.5f * dt)));
        }
    }

    // Aceleracao e desaceleracao exponencial suave (sem trancos mecanicos)
    float rate = hasInput ? 15.0f : 11.0f;
    float alpha = 1.0f - expf(-rate * dt);
    p->velocity.x += (targetVel.x - p->velocity.x) * alpha;
    p->velocity.y += (targetVel.y - p->velocity.y) * alpha;

    float velMag = hypotf(p->velocity.x, p->velocity.y);
    if (!hasInput && velMag < 0.03f) {
        p->velocity = (Vector2){ 0.0f, 0.0f };
        velMag = 0.0f;
    }

    float nx = p->position.x + p->velocity.x * dt;
    float nz = p->position.z + p->velocity.y * dt;
    if (!travadoAnim) {
        ResolverPosicaoXZ(&nx, &nz, p->radius, roomBoundsXZ, obstacles, numObstacles);
    }

    float actualDist = hypotf(nx - p->position.x, nz - p->position.z);
    p->position.x = nx;
    p->position.z = nz;

    if (!travadoAnim && p->hasMoveTarget && !hasKeyInput) {
        if (actualDist < 0.0025f) {
            p->stuckTimer += dt;
            if (p->stuckTimer > 0.30f) {
                p->hasMoveTarget = false;
                p->stuckTimer    = 0.0f;
            }
        } else {
            p->stuckTimer = 0.0f;
        }
    }

    float actualSpeed = (dt > 1e-5f) ? (actualDist / dt) : 0.0f;
    float speedRatio = Clamp(actualSpeed / p->speed, 0.0f, 1.0f);
    p->isMoving = (speedRatio > 0.05f);

    if (!inComputerWalk && p->computerExitTimer <= 0.0f) {
        float blendRate = p->isMoving ? 10.0f : 8.5f;
        p->moveBlend += (speedRatio - p->moveBlend) * (1.0f - expf(-blendRate * dt));
        if (p->moveBlend < 0.005f && !p->isMoving) p->moveBlend = 0.0f;

        float targetRun = (running && p->isMoving) ? 1.0f : 0.0f;
        p->runBlend += (targetRun - p->runBlend) * (1.0f - expf(-9.0f * dt));
        if (p->runBlend < 0.005f && targetRun == 0.0f) p->runBlend = 0.0f;

        if (p->moveBlend > 0.01f) {
            p->walkCycle += dt * (3.5f + 5.8f * speedRatio + 4.4f * p->runBlend);
            if (p->walkCycle > 2.0f * PI) p->walkCycle -= 2.0f * PI;
        }
    } else {
        p->runBlend = 0.0f;
    }
}

// ============================================================================
// Shader Noir Direcional + Rim Light e Construtor de Malhas Anatomicas
// ============================================================================

static const char *PLAYER_VS =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec3 vertexNormal;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "out vec3 fragPos;\n"
    "out vec3 fragNormal;\n"
    "out vec3 localPos;\n"
    "void main() {\n"
    "    vec4 wp = matModel * vec4(vertexPosition, 1.0);\n"
    "    fragPos = wp.xyz;\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    localPos = vertexPosition;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *PLAYER_FS =
    "#version 330\n"
    "in vec3 fragPos;\n"
    "in vec3 fragNormal;\n"
    "in vec3 localPos;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec3 viewPos;\n"
    "uniform float faceShadow;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 V = normalize(viewPos - fragPos);\n"
    "    vec3 L1 = normalize(vec3(0.42, 0.84, 0.56));\n"
    "    vec3 L2 = normalize(vec3(-0.62, 0.18, 0.48));\n"
    "    float wrap1 = max(dot(N, L1) * 0.62 + 0.38, 0.0);\n"
    "    float fill2 = max(dot(N, L2), 0.0) * 0.22;\n"
    "    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0) * 0.58;\n"
    "    float baseLum = colDiffuse.r;\n"
    "    float lit = baseLum * (0.28 + 0.78 * wrap1 + fill2) + rim * (0.55 + 0.45 * baseLum);\n"
    "    if (faceShadow > 0.5) {\n"
    "        // Rosto quase todo na sombra misteriosa, preservando contorno e volume sutil\n"
    "        float frontMask = smoothstep(-0.15, 0.55, N.z);\n"
    "        float shadowTone = mix(lit, baseLum * (0.14 + 0.18 * wrap1) + rim * 0.55, frontMask * 0.86);\n"
    "        lit = shadowTone;\n"
    "    }\n"
    "    lit = clamp(lit, 0.02, 0.96);\n"
    "    finalColor = vec4(lit, lit, lit, 1.0);\n"
    "}\n";

typedef struct {
    float y;
    float rx;
    float rz;
    float xOff;
    float zOff;
    float pinchFront; // estreita ou projeta a frente (+Z)
    float rippleAmp;  // pregas verticais sutis para tecido/cabelo
    float rippleFreq;
} RingSpec;

static Mesh BuildLoftedMesh(const RingSpec *rings, int numRings, int segs, bool capEnds) {
    int ringVerts = segs + 1;
    int vertCount = numRings * ringVerts + (capEnds ? 2 : 0);
    int triCount  = (numRings - 1) * segs * 2 + (capEnds ? segs * 2 : 0);

    float *verts = (float *)MemAlloc((unsigned int)(vertCount * 3) * sizeof(float));
    float *norms = (float *)MemAlloc((unsigned int)(vertCount * 3) * sizeof(float));
    unsigned short *indices = (unsigned short *)MemAlloc((unsigned int)(triCount * 3) * sizeof(unsigned short));
    memset(norms, 0, (size_t)(vertCount * 3) * sizeof(float));

    for (int r = 0; r < numRings; r++) {
        const RingSpec *sp = &rings[r];
        for (int s = 0; s <= segs; s++) {
            float theta = ((float)s / (float)segs) * 2.0f * PI;
            float ct = cosf(theta);
            float st = sinf(theta);
            float ripple = (sp->rippleAmp != 0.0f) ? (1.0f + sp->rippleAmp * cosf(theta * sp->rippleFreq)) : 1.0f;
            float rx = sp->rx * ripple;
            float rz = sp->rz * ripple;
            if (st > 0.0f && sp->pinchFront != 0.0f) {
                rz += sp->pinchFront * st;
                rx *= (1.0f - 0.14f * st * (sp->pinchFront > 0.0f ? 1.0f : 0.0f));
            }
            int idx = (r * ringVerts + s) * 3;
            verts[idx + 0] = sp->xOff + ct * rx;
            verts[idx + 1] = sp->y;
            verts[idx + 2] = sp->zOff + st * rz;
        }
    }

    int botCenterIdx = numRings * ringVerts;
    int topCenterIdx = botCenterIdx + 1;
    if (capEnds) {
        verts[botCenterIdx * 3 + 0] = rings[0].xOff;
        verts[botCenterIdx * 3 + 1] = rings[0].y;
        verts[botCenterIdx * 3 + 2] = rings[0].zOff;
        verts[topCenterIdx * 3 + 0] = rings[numRings - 1].xOff;
        verts[topCenterIdx * 3 + 1] = rings[numRings - 1].y;
        verts[topCenterIdx * 3 + 2] = rings[numRings - 1].zOff;
    }

    int ti = 0;
    for (int r = 0; r < numRings - 1; r++) {
        for (int s = 0; s < segs; s++) {
            unsigned short i0 = (unsigned short)(r * ringVerts + s);
            unsigned short i1 = (unsigned short)(r * ringVerts + s + 1);
            unsigned short i2 = (unsigned short)((r + 1) * ringVerts + s);
            unsigned short i3 = (unsigned short)((r + 1) * ringVerts + s + 1);

            indices[ti++] = i0; indices[ti++] = i2; indices[ti++] = i1;
            indices[ti++] = i1; indices[ti++] = i2; indices[ti++] = i3;
        }
    }

    if (capEnds) {
        for (int s = 0; s < segs; s++) {
            indices[ti++] = (unsigned short)botCenterIdx;
            indices[ti++] = (unsigned short)(s);
            indices[ti++] = (unsigned short)(s + 1);

            int topBase = (numRings - 1) * ringVerts;
            indices[ti++] = (unsigned short)topCenterIdx;
            indices[ti++] = (unsigned short)(topBase + s + 1);
            indices[ti++] = (unsigned short)(topBase + s);
        }
    }

    // Normais suaves ponderadas pela area dos triangulos
    for (int t = 0; t < triCount; t++) {
        int a = indices[t * 3 + 0];
        int b = indices[t * 3 + 1];
        int c = indices[t * 3 + 2];
        Vector3 va = { verts[a * 3], verts[a * 3 + 1], verts[a * 3 + 2] };
        Vector3 vb = { verts[b * 3], verts[b * 3 + 1], verts[b * 3 + 2] };
        Vector3 vc = { verts[c * 3], verts[c * 3 + 1], verts[c * 3 + 2] };
        Vector3 n = Vector3CrossProduct(Vector3Subtract(vb, va), Vector3Subtract(vc, va));
        int vIds[3] = { a, b, c };
        for (int k = 0; k < 3; k++) {
            norms[vIds[k] * 3 + 0] += n.x;
            norms[vIds[k] * 3 + 1] += n.y;
            norms[vIds[k] * 3 + 2] += n.z;
        }
    }
    // Unifica normais na costura (s=0 e s=segs) para evitar linha visivel
    for (int r = 0; r < numRings; r++) {
        int a = (r * ringVerts + 0) * 3;
        int b = (r * ringVerts + segs) * 3;
        float nx = norms[a + 0] + norms[b + 0];
        float ny = norms[a + 1] + norms[b + 1];
        float nz = norms[a + 2] + norms[b + 2];
        norms[a + 0] = norms[b + 0] = nx;
        norms[a + 1] = norms[b + 1] = ny;
        norms[a + 2] = norms[b + 2] = nz;
    }
    for (int v = 0; v < vertCount; v++) {
        Vector3 n = { norms[v * 3], norms[v * 3 + 1], norms[v * 3 + 2] };
        float len = Vector3Length(n);
        if (len > 1e-6f) {
            norms[v * 3 + 0] = n.x / len;
            norms[v * 3 + 1] = n.y / len;
            norms[v * 3 + 2] = n.z / len;
        } else {
            norms[v * 3 + 1] = 1.0f;
        }
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = vertCount;
    mesh.triangleCount = triCount;
    mesh.vertices = verts;
    mesh.normals = norms;
    mesh.indices = indices;
    UploadMesh(&mesh, false);
    return mesh;
}

typedef struct {
    Model           model;
    ModelAnimation *anims;
    int             animCount;
    float           baseLums[16];
    bool            loaded;
} CharGlbAsset;

typedef struct {
    bool     initialized;
    Shader   shader;
    int      locViewPos;
    int      locFaceShadow;
    Material mat;

    CharGlbAsset glb[4]; // 0=male01, 1=male02, 2=male03, 3=female01

    // Malhas compartilhadas / fallback procedural
    Mesh headM, headF;
    Mesh hairM, hairF;
    Mesh bangM1, bangM2;
    Mesh hairBackF, hairSideF;
    Mesh neckM, neckF;
    Mesh torsoM, torsoF;
    Mesh shirt;
    Mesh tie;
    Mesh coatSkirtM, coatSkirtF;
    Mesh coatFlapM, coatFlapF;
    Mesh lapel;
    Mesh collarBack;
    Mesh belt;
    Mesh lanyard;
    Mesh badge;
    Mesh shadowDisc;
    Mesh jointSphere;
    Mesh upperArmM, upperArmF;
    Mesh forearmM, forearmF;
    Mesh hand;
    Mesh thighM, thighF;
    Mesh calfM, calfF;
    Mesh shoe;
} PlayerAssets;

static PlayerAssets g_assets = { 0 };

static void EnsurePlayerAssets(void) {
    if (g_assets.initialized) return;

    g_assets.shader = LoadShaderFromMemory(PLAYER_VS, PLAYER_FS);
    g_assets.locViewPos    = GetShaderLocation(g_assets.shader, "viewPos");
    g_assets.locFaceShadow = GetShaderLocation(g_assets.shader, "faceShadow");
    g_assets.mat = LoadMaterialDefault();
    g_assets.mat.shader = g_assets.shader;

    static const char *glbPaths[4] = {
        "assets/models/char_male01.glb",
        "assets/models/char_male02.glb",
        "assets/models/char_male03.glb",
        "assets/models/char_female01.glb"
    };
    for (int i = 0; i < 4; i++) {
        if (FileExists(glbPaths[i])) {
            g_assets.glb[i].model = LoadModel(glbPaths[i]);
            g_assets.glb[i].anims = LoadModelAnimations(glbPaths[i], &g_assets.glb[i].animCount);
            for (int m = 0; m < g_assets.glb[i].model.materialCount && m < 16; m++) {
                Color c = g_assets.glb[i].model.materials[m].maps[MATERIAL_MAP_DIFFUSE].color;
                float lum = (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255.0f;
                // Contraste limpo para evitar aparencia manchada no shader noir
                g_assets.glb[i].baseLums[m] = Clamp(0.10f + lum * 1.18f, 0.10f, 0.90f);
                g_assets.glb[i].model.materials[m].shader = g_assets.shader;
            }
            g_assets.glb[i].loaded = (g_assets.glb[i].model.meshCount > 0);
        }
    }

    // Cabecas anatomicas (queixo afilado, macas do rosto, cranio)
    const RingSpec headRingsM[] = {
        { -0.105f, 0.032f, 0.036f, 0.0f,  0.022f,  0.014f, 0.0f, 0.0f }, // queixo
        { -0.072f, 0.056f, 0.068f, 0.0f,  0.012f,  0.018f, 0.0f, 0.0f }, // mandibula
        { -0.022f, 0.072f, 0.088f, 0.0f,  0.005f,  0.016f, 0.0f, 0.0f }, // macas / nariz
        {  0.032f, 0.078f, 0.095f, 0.0f, -0.002f,  0.008f, 0.0f, 0.0f }, // sobrancelha
        {  0.085f, 0.072f, 0.088f, 0.0f, -0.008f,  0.000f, 0.0f, 0.0f }, // testa alta
        {  0.120f, 0.042f, 0.052f, 0.0f, -0.010f,  0.000f, 0.0f, 0.0f }, // topo cranio
    };
    g_assets.headM = BuildLoftedMesh(headRingsM, 6, 20, true);

    const RingSpec headRingsF[] = {
        { -0.100f, 0.026f, 0.032f, 0.0f,  0.022f,  0.014f, 0.0f, 0.0f },
        { -0.068f, 0.048f, 0.062f, 0.0f,  0.012f,  0.016f, 0.0f, 0.0f },
        { -0.020f, 0.066f, 0.082f, 0.0f,  0.004f,  0.014f, 0.0f, 0.0f },
        {  0.032f, 0.073f, 0.090f, 0.0f, -0.002f,  0.006f, 0.0f, 0.0f },
        {  0.084f, 0.068f, 0.084f, 0.0f, -0.008f,  0.000f, 0.0f, 0.0f },
        {  0.118f, 0.040f, 0.050f, 0.0f, -0.010f,  0.000f, 0.0f, 0.0f },
    };
    g_assets.headF = BuildLoftedMesh(headRingsF, 6, 20, true);

    // Cabelo masculino (volume superior + nuca com leve textura)
    const RingSpec hairRingsM[] = {
        { -0.020f, 0.076f, 0.084f, 0.0f, -0.025f, -0.020f, 0.04f, 7.0f },
        {  0.036f, 0.084f, 0.098f, 0.0f, -0.016f, -0.008f, 0.05f, 8.0f },
        {  0.092f, 0.082f, 0.096f, 0.0f, -0.008f,  0.005f, 0.05f, 7.0f },
        {  0.130f, 0.052f, 0.064f, 0.0f, -0.008f,  0.004f, 0.03f, 6.0f },
    };
    g_assets.hairM = BuildLoftedMesh(hairRingsM, 4, 20, true);

    const RingSpec bangRings1[] = {
        { -0.065f, 0.010f, 0.008f,  0.018f, 0.012f, 0.0f, 0.0f, 0.0f },
        { -0.030f, 0.026f, 0.018f,  0.006f, 0.006f, 0.0f, 0.0f, 0.0f },
        {  0.015f, 0.032f, 0.020f, -0.010f, 0.000f, 0.0f, 0.0f, 0.0f },
        {  0.050f, 0.020f, 0.015f, -0.022f,-0.008f, 0.0f, 0.0f, 0.0f },
    };
    g_assets.bangM1 = BuildLoftedMesh(bangRings1, 4, 12, true);

    const RingSpec bangRings2[] = {
        { -0.055f, 0.009f, 0.007f, -0.016f, 0.010f, 0.0f, 0.0f, 0.0f },
        { -0.022f, 0.022f, 0.016f, -0.004f, 0.005f, 0.0f, 0.0f, 0.0f },
        {  0.020f, 0.028f, 0.018f,  0.012f, 0.000f, 0.0f, 0.0f, 0.0f },
        {  0.048f, 0.018f, 0.014f,  0.020f,-0.006f, 0.0f, 0.0f, 0.0f },
    };
    g_assets.bangM2 = BuildLoftedMesh(bangRings2, 4, 12, true);

    // Cabelo feminino (chanel/medio com caimento suave ate os ombros + mechas laterais)
    const RingSpec hairRingsF[] = {
        {  0.010f, 0.080f, 0.092f, 0.0f, -0.015f, -0.012f, 0.02f, 6.0f },
        {  0.062f, 0.084f, 0.098f, 0.0f, -0.010f,  0.002f, 0.02f, 6.0f },
        {  0.108f, 0.076f, 0.090f, 0.0f, -0.008f,  0.004f, 0.02f, 5.0f },
        {  0.132f, 0.044f, 0.056f, 0.0f, -0.008f,  0.000f, 0.00f, 0.0f },
    };
    g_assets.hairF = BuildLoftedMesh(hairRingsF, 4, 20, true);

    const RingSpec hairBackRingsF[] = {
        { -0.175f, 0.088f, 0.042f, 0.0f, -0.048f, 0.0f, 0.03f, 8.0f },
        { -0.095f, 0.095f, 0.052f, 0.0f, -0.044f, 0.0f, 0.02f, 8.0f },
        { -0.015f, 0.088f, 0.054f, 0.0f, -0.036f, 0.0f, 0.01f, 6.0f },
        {  0.055f, 0.074f, 0.046f, 0.0f, -0.026f, 0.0f, 0.00f, 0.0f },
    };
    g_assets.hairBackF = BuildLoftedMesh(hairBackRingsF, 4, 18, true);

    const RingSpec hairSideRingsF[] = {
        { -0.165f, 0.018f, 0.024f, 0.0f,  0.010f, 0.0f, 0.0f, 0.0f },
        { -0.095f, 0.028f, 0.038f, 0.0f,  0.004f, 0.0f, 0.0f, 0.0f },
        { -0.020f, 0.030f, 0.042f, 0.0f,  0.000f, 0.0f, 0.0f, 0.0f },
        {  0.045f, 0.022f, 0.030f, 0.0f, -0.006f, 0.0f, 0.0f, 0.0f },
    };
    g_assets.hairSideF = BuildLoftedMesh(hairSideRingsF, 4, 14, true);

    // Pescoco
    const RingSpec neckRingsM[] = {
        { 0.00f, 0.058f, 0.056f, 0, 0, 0, 0, 0 },
        { 0.06f, 0.052f, 0.052f, 0, 0, 0, 0, 0 },
        { 0.12f, 0.050f, 0.052f, 0, 0.004f, 0, 0, 0 },
    };
    g_assets.neckM = BuildLoftedMesh(neckRingsM, 3, 14, true);

    const RingSpec neckRingsF[] = {
        { 0.00f, 0.048f, 0.048f, 0, 0, 0, 0, 0 },
        { 0.06f, 0.043f, 0.044f, 0, 0, 0, 0, 0 },
        { 0.12f, 0.042f, 0.044f, 0, 0.004f, 0, 0, 0 },
    };
    g_assets.neckF = BuildLoftedMesh(neckRingsF, 3, 14, true);

    // Tronco masculino estilo Max Payne 2 (ombros largos, peito forte, cintura atletica)
    const RingSpec torsoRingsM[] = {
        { -0.26f, 0.150f, 0.104f, 0, -0.004f, 0.003f, 0, 0 },
        { -0.10f, 0.142f, 0.098f, 0,  0.000f, 0.004f, 0, 0 },
        {  0.06f, 0.178f, 0.116f, 0,  0.005f, 0.006f, 0, 0 },
        {  0.18f, 0.198f, 0.114f, 0,  0.003f, 0.004f, 0, 0 },
        {  0.25f, 0.136f, 0.090f, 0,  0.000f, 0.000f, 0, 0 },
    };
    g_assets.torsoM = BuildLoftedMesh(torsoRingsM, 5, 20, true);

    // Tronco feminino estilo Max Payne 2 (cintura definida, blazer estruturado)
    const RingSpec torsoRingsF[] = {
        { -0.26f, 0.144f, 0.100f, 0, -0.004f, 0.002f, 0, 0 },
        { -0.10f, 0.122f, 0.088f, 0,  0.000f, 0.002f, 0, 0 },
        {  0.06f, 0.152f, 0.104f, 0,  0.004f, 0.006f, 0, 0 },
        {  0.17f, 0.166f, 0.098f, 0,  0.002f, 0.002f, 0, 0 },
        {  0.24f, 0.116f, 0.078f, 0,  0.000f, 0.000f, 0, 0 },
    };
    g_assets.torsoF = BuildLoftedMesh(torsoRingsF, 5, 20, true);

    // Camisa / blusa interna (abertura em V limpa no centro)
    const RingSpec shirtRings[] = {
        { -0.24f, 0.092f, 0.076f, 0, 0.022f, 0.004f, 0, 0 },
        {  0.04f, 0.100f, 0.086f, 0, 0.024f, 0.006f, 0, 0 },
        {  0.22f, 0.080f, 0.070f, 0, 0.016f, 0.004f, 0, 0 },
    };
    g_assets.shirt = BuildLoftedMesh(shirtRings, 3, 16, true);

    // Gravata social bem definida
    const RingSpec tieRings[] = {
        { -0.20f, 0.022f, 0.010f, 0, 0, 0, 0, 0 },
        { -0.13f, 0.028f, 0.012f, 0, 0, 0, 0, 0 },
        {  0.10f, 0.016f, 0.010f, 0, 0, 0, 0, 0 },
        {  0.14f, 0.025f, 0.016f, 0, 0, 0, 0, 0 },
    };
    g_assets.tie = BuildLoftedMesh(tieRings, 4, 10, true);

    // Sobretudo / Trench Coat estilo Max Payne 2 (comprimento ate metade da coxa, movimento fluido)
    const RingSpec skirtRingsM[] = {
        { -0.38f, 0.192f, 0.132f, 0, -0.012f, 0.0f, 0.024f, 6.0f },
        { -0.20f, 0.168f, 0.116f, 0, -0.008f, 0.0f, 0.016f, 6.0f },
        {  0.00f, 0.146f, 0.100f, 0, -0.004f, 0.0f, 0.008f, 6.0f },
    };
    g_assets.coatSkirtM = BuildLoftedMesh(skirtRingsM, 3, 28, true);

    const RingSpec skirtRingsF[] = {
        { -0.34f, 0.178f, 0.124f, 0, -0.012f, 0.0f, 0.022f, 6.0f },
        { -0.17f, 0.150f, 0.104f, 0, -0.007f, 0.0f, 0.014f, 6.0f },
        {  0.00f, 0.124f, 0.086f, 0, -0.004f, 0.0f, 0.006f, 6.0f },
    };
    g_assets.coatSkirtF = BuildLoftedMesh(skirtRingsF, 3, 28, true);

    // Abas frontais abertas do sobretudo / blazer
    const RingSpec flapRingsM[] = {
        { -0.38f, 0.076f, 0.024f, 0, 0, 0, 0.02f, 4.0f },
        { -0.10f, 0.068f, 0.022f, 0, 0, 0, 0.01f, 4.0f },
        {  0.18f, 0.064f, 0.020f, 0, 0, 0, 0.00f, 0.0f },
    };
    g_assets.coatFlapM = BuildLoftedMesh(flapRingsM, 3, 18, true);

    const RingSpec flapRingsF[] = {
        { -0.34f, 0.066f, 0.020f, 0, 0, 0, 0.02f, 4.0f },
        { -0.08f, 0.056f, 0.018f, 0, 0, 0, 0.01f, 4.0f },
        {  0.17f, 0.056f, 0.018f, 0, 0, 0, 0.00f, 0.0f },
    };
    g_assets.coatFlapF = BuildLoftedMesh(flapRingsF, 3, 18, true);

    // Cordao e cracha corporativo discreto
    const RingSpec lanyardRings[] = {
        { -0.10f, 0.006f, 0.004f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.006f, 0.004f, 0, 0, 0, 0, 0 },
        {  0.10f, 0.006f, 0.004f, 0, 0, 0, 0, 0 },
    };
    g_assets.lanyard = BuildLoftedMesh(lanyardRings, 3, 8, true);

    const RingSpec badgeRings[] = {
        { -0.030f, 0.022f, 0.005f, 0, 0, 0, 0, 0 },
        {  0.030f, 0.022f, 0.005f, 0, 0, 0, 0, 0 },
    };
    g_assets.badge = BuildLoftedMesh(badgeRings, 2, 10, true);

    // Disco de sombra suave no piso + esfera suave para articulacoes realistas
    const RingSpec shadowRings[] = {
        { 0.000f, 0.24f, 0.20f, 0, 0, 0, 0, 0 },
        { 0.004f, 0.23f, 0.19f, 0, 0, 0, 0, 0 },
    };
    g_assets.shadowDisc = BuildLoftedMesh(shadowRings, 2, 20, true);
    g_assets.jointSphere = GenMeshSphere(1.0f, 10, 14);

    // Lapela e gola alta traseira
    const RingSpec lapelRings[] = {
        { -0.12f, 0.026f, 0.012f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.036f, 0.014f, 0, 0, 0, 0, 0 },
        {  0.12f, 0.022f, 0.012f, 0, 0, 0, 0, 0 },
    };
    g_assets.lapel = BuildLoftedMesh(lapelRings, 3, 10, true);

    const RingSpec collarRings[] = {
        { 0.00f, 0.095f, 0.078f, 0, -0.012f, -0.015f, 0, 0 },
        { 0.08f, 0.102f, 0.084f, 0, -0.018f, -0.020f, 0, 0 },
    };
    g_assets.collarBack = BuildLoftedMesh(collarRings, 2, 16, true);

    // Cinto feminino
    const RingSpec beltRings[] = {
        { -0.022f, 0.122f, 0.086f, 0, 0.002f, 0.004f, 0, 0 },
        {  0.022f, 0.122f, 0.086f, 0, 0.002f, 0.004f, 0, 0 },
    };
    g_assets.belt = BuildLoftedMesh(beltRings, 2, 20, true);

    // Bracos solidos estilo Max Payne 2
    const RingSpec uArmRingsM[] = {
        { -0.27f, 0.048f, 0.048f, 0, 0, 0, 0, 0 },
        { -0.14f, 0.056f, 0.056f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.064f, 0.062f, 0, 0, 0, 0, 0 },
        {  0.03f, 0.048f, 0.048f, 0, 0, 0, 0, 0 },
    };
    g_assets.upperArmM = BuildLoftedMesh(uArmRingsM, 4, 16, true);

    const RingSpec uArmRingsF[] = {
        { -0.26f, 0.040f, 0.040f, 0, 0, 0, 0, 0 },
        { -0.13f, 0.046f, 0.046f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.052f, 0.050f, 0, 0, 0, 0, 0 },
        {  0.025f,0.038f, 0.038f, 0, 0, 0, 0, 0 },
    };
    g_assets.upperArmF = BuildLoftedMesh(uArmRingsF, 4, 16, true);

    const RingSpec fArmRingsM[] = {
        { -0.25f, 0.038f, 0.034f, 0, 0, 0, 0, 0 },
        { -0.14f, 0.046f, 0.044f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.048f, 0.048f, 0, 0, 0, 0, 0 },
    };
    g_assets.forearmM = BuildLoftedMesh(fArmRingsM, 3, 16, true);

    const RingSpec fArmRingsF[] = {
        { -0.24f, 0.031f, 0.028f, 0, 0, 0, 0, 0 },
        { -0.13f, 0.038f, 0.036f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.040f, 0.040f, 0, 0, 0, 0, 0 },
    };
    g_assets.forearmF = BuildLoftedMesh(fArmRingsF, 3, 16, true);

    // Mao esculpida
    const RingSpec handRings[] = {
        { -0.105f, 0.020f, 0.011f, 0, 0.005f, 0, 0, 0 },
        { -0.055f, 0.030f, 0.016f, 0, 0.004f, 0, 0, 0 },
        {  0.000f, 0.026f, 0.017f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.hand = BuildLoftedMesh(handRings, 3, 14, true);

    // Pernas solidas estilo Max Payne 2
    const RingSpec thighRingsM[] = {
        { -0.42f, 0.068f, 0.070f, 0, 0.004f, 0, 0, 0 },
        { -0.22f, 0.086f, 0.090f, 0, 0.006f, 0, 0, 0 },
        {  0.00f, 0.096f, 0.098f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.thighM = BuildLoftedMesh(thighRingsM, 3, 16, true);

    const RingSpec thighRingsF[] = {
        { -0.41f, 0.058f, 0.060f, 0, 0.004f, 0, 0, 0 },
        { -0.21f, 0.074f, 0.078f, 0, 0.006f, 0, 0, 0 },
        {  0.00f, 0.084f, 0.086f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.thighF = BuildLoftedMesh(thighRingsF, 3, 16, true);

    const RingSpec calfRingsM[] = {
        { -0.40f, 0.048f, 0.050f, 0,  0.000f, 0, 0, 0 },
        { -0.20f, 0.066f, 0.070f, 0, -0.008f, 0, 0, 0 },
        {  0.00f, 0.066f, 0.068f, 0,  0.000f, 0, 0, 0 },
    };
    g_assets.calfM = BuildLoftedMesh(calfRingsM, 3, 16, true);

    const RingSpec calfRingsF[] = {
        { -0.39f, 0.042f, 0.044f, 0,  0.000f, 0, 0, 0 },
        { -0.19f, 0.056f, 0.060f, 0, -0.007f, 0, 0, 0 },
        {  0.00f, 0.057f, 0.059f, 0,  0.000f, 0, 0, 0 },
    };
    g_assets.calfF = BuildLoftedMesh(calfRingsF, 3, 16, true);

    // Sapato social realista
    const RingSpec shoeRings[] = {
        { -0.055f, 0.044f, 0.112f, 0,  0.052f, 0.012f, 0, 0 },
        { -0.015f, 0.042f, 0.104f, 0,  0.046f, 0.010f, 0, 0 },
        {  0.025f, 0.036f, 0.064f, 0,  0.014f, 0.000f, 0, 0 },
    };
    g_assets.shoe = BuildLoftedMesh(shoeRings, 3, 14, true);

    g_assets.initialized = true;
}

void PlayerCharacterUnload(void) {
    if (!g_assets.initialized) return;

    Mesh *meshes[] = {
        &g_assets.headM, &g_assets.headF, &g_assets.hairM, &g_assets.hairF,
        &g_assets.bangM1, &g_assets.bangM2, &g_assets.hairBackF, &g_assets.hairSideF,
        &g_assets.neckM, &g_assets.neckF, &g_assets.torsoM, &g_assets.torsoF,
        &g_assets.shirt, &g_assets.tie, &g_assets.coatSkirtM, &g_assets.coatSkirtF,
        &g_assets.coatFlapM, &g_assets.coatFlapF, &g_assets.lapel, &g_assets.collarBack,
        &g_assets.belt, &g_assets.lanyard, &g_assets.badge, &g_assets.shadowDisc, &g_assets.jointSphere,
        &g_assets.upperArmM, &g_assets.upperArmF,
        &g_assets.forearmM, &g_assets.forearmF, &g_assets.hand,
        &g_assets.thighM, &g_assets.thighF, &g_assets.calfM, &g_assets.calfF, &g_assets.shoe
    };
    for (int i = 0; i < (int)(sizeof(meshes) / sizeof(meshes[0])); i++) {
        if (meshes[i]->vboId != NULL || meshes[i]->vaoId != 0) {
            UnloadMesh(*meshes[i]);
        }
    }
    for (int i = 0; i < 4; i++) {
        if (g_assets.glb[i].anims && g_assets.glb[i].animCount > 0) {
            UnloadModelAnimations(g_assets.glb[i].anims, g_assets.glb[i].animCount);
        }
        if (g_assets.glb[i].loaded) {
            UnloadModel(g_assets.glb[i].model);
        }
    }
    if (g_assets.shader.id > 0) {
        UnloadShader(g_assets.shader);
    }
    memset(&g_assets, 0, sizeof(g_assets));
}

// ============================================================================
// Renderizacao Hierarquica com Iluminacao Noir
// ============================================================================

static void DrawPart(Mesh mesh, Matrix transform, float lum, bool faceShadow) {
    unsigned char v = (unsigned char)(Clamp(lum, 0.0f, 1.0f) * 255.0f);
    g_assets.mat.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ v, v, v, 255 };
    float fs = faceShadow ? 1.0f : 0.0f;
    SetShaderValue(g_assets.shader, g_assets.locFaceShadow, &fs, SHADER_UNIFORM_FLOAT);
    DrawMesh(mesh, g_assets.mat, transform);
}

static Rectangle g_shadowBoundsXZ = { -50.0f, -50.0f, 100.0f, 100.0f };
static bool      g_hasShadowBounds = false;

void PlayerCharacterSetShadowBounds(Rectangle boundsXZ) {
    g_shadowBoundsXZ = boundsXZ;
    g_hasShadowBounds = (boundsXZ.width > 0.1f && boundsXZ.height > 0.1f);
}

// Matriz de projecao planar de sombra real sobre o piso y = groundY, limitada dentro das paredes da sala
static Matrix MakePlanarShadowMatrix(float groundY, Vector3 charPos) {
    float sx = 0.36f;
    float sz = 0.26f;
    if (g_hasShadowBounds) {
        float maxH = 1.95f;
        float distRight = (g_shadowBoundsXZ.x + g_shadowBoundsXZ.width) - charPos.x;
        float distBack  = (g_shadowBoundsXZ.y + g_shadowBoundsXZ.height) - charPos.z;
        float distLeft  = charPos.x - g_shadowBoundsXZ.x;
        float distFront = charPos.z - g_shadowBoundsXZ.y;
        if (distRight < sx * maxH) sx = fmaxf(0.05f, (distRight - 0.08f) / maxH);
        if (distBack  < sz * maxH) sz = fmaxf(0.05f, (distBack  - 0.08f) / maxH);
        if (distLeft  < 0.25f) sx = fminf(sx, 0.20f);
        if (distFront < 0.25f) sz = fminf(sz, 0.16f);
    }
    return (Matrix){
        1.0f,  sx,   0.0f, -sx * groundY,
        0.0f,  0.0f, 0.0f,  groundY,
        0.0f,  sz,   1.0f, -sz * groundY,
        0.0f,  0.0f, 0.0f,  1.0f
    };
}

static void DrawPartWithShadow(Mesh mesh, Matrix transform, float lum, bool faceShadow, const Matrix *shadowProj) {
    DrawPart(mesh, transform, lum, faceShadow);
    if (shadowProj) {
        DrawPart(mesh, MatrixMultiply(transform, *shadowProj), 0.06f, false);
    }
}

static void DrawCharGlb(int idx, Matrix root,
                        int animA, float frameA,
                        int animB, float frameB, float blend,
                        float lumScale, float lumBias) {
    if (idx < 0 || idx >= 4 || !g_assets.glb[idx].loaded) return;
    CharGlbAsset *asset = &g_assets.glb[idx];

    if (asset->anims && asset->animCount > 0) {
        int a = (animA >= 0 && animA < asset->animCount) ? animA : 0;
        int b = (animB >= 0 && animB < asset->animCount) ? animB : a;
        float fa = fmodf(fabsf(frameA), (float)asset->anims[a].keyframeCount);
        float fb = fmodf(fabsf(frameB), (float)asset->anims[b].keyframeCount);
        if (a == b || blend <= 0.001f) {
            UpdateModelAnimation(asset->model, asset->anims[a], fa);
        } else if (blend >= 0.999f) {
            UpdateModelAnimation(asset->model, asset->anims[b], fb);
        } else {
            UpdateModelAnimationEx(asset->model, asset->anims[a], fa, asset->anims[b], fb, blend);
        }
    }

    float fs = 0.0f;
    SetShaderValue(g_assets.shader, g_assets.locFaceShadow, &fs, SHADER_UNIFORM_FLOAT);

    for (int i = 0; i < asset->model.meshCount; i++) {
        int matIdx = asset->model.meshMaterial[i];
        if (matIdx < 0 || matIdx >= asset->model.materialCount) matIdx = 0;
        float baseL = (matIdx < 16) ? asset->baseLums[matIdx] : 0.5f;
        float finalL = Clamp(baseL * lumScale + lumBias, 0.04f, 0.95f);
        unsigned char v = (unsigned char)(finalL * 255.0f);
        asset->model.materials[matIdx].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ v, v, v, 255 };
        DrawMesh(asset->model.meshes[i], asset->model.materials[matIdx], root);
    }
}

static Matrix MakeLocal(Vector3 pos, Vector3 rotDeg, Vector3 scale) {
    Matrix m = MatrixScale(scale.x, scale.y, scale.z);
    if (rotDeg.z != 0.0f) m = MatrixMultiply(m, MatrixRotateZ(rotDeg.z * DEG2RAD));
    if (rotDeg.x != 0.0f) m = MatrixMultiply(m, MatrixRotateX(rotDeg.x * DEG2RAD));
    if (rotDeg.y != 0.0f) m = MatrixMultiply(m, MatrixRotateY(rotDeg.y * DEG2RAD));
    m = MatrixMultiply(m, MatrixTranslate(pos.x, pos.y, pos.z));
    return m;
}

static void DrawFaceDetails(Matrix headM, bool fem, float lumMult) {
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(headM));
    unsigned char browV = (unsigned char)(28.0f * lumMult);
    unsigned char eyeV  = (unsigned char)(18.0f * lumMult);
    unsigned char whtV  = (unsigned char)(238.0f * lumMult);
    unsigned char noseV = (unsigned char)(148.0f * lumMult);
    unsigned char lipV  = fem ? (unsigned char)(112.0f * lumMult) : (unsigned char)(118.0f * lumMult);

    Color cBrow = (Color){ browV, (unsigned char)(browV * 0.9f), browV, 255 };
    Color cEye  = (Color){ eyeV, eyeV, (unsigned char)(eyeV + 4), 255 };
    Color cWht  = (Color){ whtV, (unsigned char)(whtV * 0.96f), (unsigned char)(whtV * 0.92f), 255 };
    Color cNose = (Color){ noseV, (unsigned char)(noseV * 0.93f), (unsigned char)(noseV * 0.89f), 255 };
    Color cLip  = (Color){ lipV, (unsigned char)(lipV * 0.84f), (unsigned char)(lipV * 0.82f), 255 };

    // Sobrancelhas
    DrawCube((Vector3){ -0.028f, 0.036f, 0.085f }, 0.024f, 0.005f, 0.010f, cBrow);
    DrawCube((Vector3){  0.028f, 0.036f, 0.085f }, 0.024f, 0.005f, 0.010f, cBrow);
    // Esclera + iris dos olhos
    DrawCube((Vector3){ -0.028f, 0.020f, 0.086f }, 0.020f, 0.010f, 0.008f, cWht);
    DrawCube((Vector3){  0.028f, 0.020f, 0.086f }, 0.020f, 0.010f, 0.008f, cWht);
    DrawCube((Vector3){ -0.027f, 0.020f, 0.089f }, 0.010f, 0.009f, 0.006f, cEye);
    DrawCube((Vector3){  0.027f, 0.020f, 0.089f }, 0.010f, 0.009f, 0.006f, cEye);
    // Nariz discreto
    DrawCube((Vector3){  0.000f, -0.004f, 0.095f }, 0.012f, 0.026f, 0.014f, cNose);
    // Boca
    DrawCube((Vector3){  0.000f, -0.044f, 0.084f }, fem ? 0.024f : 0.028f, 0.006f, 0.008f, cLip);
    rlPopMatrix();
}

static void DrawBadgeDetails(Matrix torsoBase, float offsetX) {
    DrawPart(g_assets.lanyard, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.11f, 0.022f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.04f, 1.02f, 1.04f }), torsoBase), 0.86f, false);
    Matrix badgeM = MatrixMultiply(MakeLocal((Vector3){ offsetX, -0.015f, 0.124f }, (Vector3){ -4.0f, 0, 0 }, (Vector3){ 1.38f, 1.45f, 1.25f }), torsoBase);
    DrawPart(g_assets.badge, badgeM, 1.00f, false);
    // Foto + faixa corporativa nitida na frente do cracha
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(badgeM));
    DrawCube((Vector3){ 0.0f, 0.020f, 0.006f }, 0.038f, 0.008f, 0.003f, (Color){ 42, 38, 42, 255 });
    DrawCube((Vector3){ -0.009f, -0.004f, 0.006f }, 0.014f, 0.018f, 0.003f, (Color){ 68, 62, 66, 255 });
    DrawCube((Vector3){ 0.010f, -0.002f, 0.006f }, 0.014f, 0.004f, 0.003f, (Color){ 110, 102, 98, 255 });
    DrawCube((Vector3){ 0.010f, -0.010f, 0.006f }, 0.014f, 0.004f, 0.003f, (Color){ 110, 102, 98, 255 });
    rlPopMatrix();
}

void PlayerCharacterDraw3D(const PlayerCharacter *p, Vector3 camPos) {
    if (!p) return;
    EnsurePlayerAssets();

    float viewArr[3] = { camPos.x, camPos.y, camPos.z };
    SetShaderValue(g_assets.shader, g_assets.locViewPos, viewArr, SHADER_UNIFORM_VEC3);

    bool fem = p->isFemale;
    float blend = p->moveBlend;
    float runB  = p->runBlend;
    float strideMult = 1.0f + 0.34f * runB;
    float s = sinf(p->walkCycle);
    float c2 = cosf(2.0f * p->walkCycle);

    // Movimento dinamico e fluido estilo Max Payne 2 (inclinacao, contrapeso e balanco natural)
    float idleBreath = sinf(p->idleTime * 1.9f) * 0.006f * (1.0f - blend);
    float walkBob    = (-c2 * 0.020f * strideMult) * blend;
    float bobY       = walkBob + idleBreath;

    float idleHipRoll = 1.8f * (1.0f - blend);
    float pelvisYaw   = s * 5.2f * blend + 2.0f * (1.0f - blend);
    float pelvisRoll  = s * 2.6f * blend + idleHipRoll;
    float torsoYaw    = -s * 6.0f * blend - 1.5f * (1.0f - blend);
    float forwardLean = (6.5f + 5.5f * runB) * blend + 1.0f * (1.0f - blend);

    float reach = 0.0f;
    float sitFactor = 0.0f;
    float paperInspect = 0.0f;
    if (p->interactTimer > 0.0f && p->interactDur > 0.01f) {
        float u = 1.0f - (p->interactTimer / p->interactDur); // 0 -> 1
        if (p->interactType == PLAYER_ANIM_COMPUTER) {
            if (u >= 0.45f && u < 0.72f) {
                // Fase 2: estende a mao para puxar a cadeira
                reach = sinf(((u - 0.45f) / 0.27f) * PI) * 0.65f;
            } else if (u >= 0.72f) {
                // Fase 3: senta suavemente na cadeira e digita
                float sU = Clamp((u - 0.72f) / 0.24f, 0.0f, 1.0f);
                sitFactor = sU * sU * (3.0f - 2.0f * sU);
            }
        } else if (p->interactType == PLAYER_ANIM_PAPER || p->interactType == PLAYER_ANIM_FILE) {
            paperInspect = sinf(u * PI);
        } else {
            reach = sinf(u * PI);
        }
    } else if (p->computerExitTimer > 0.0f) {
        float u = 1.0f - (p->computerExitTimer / 0.72f);
        sitFactor = 1.0f - Clamp(u / 0.38f, 0.0f, 1.0f);
    }

    // Escala robusta estilo Max Payne 2 (maior em largura e altura)
    float rootScale = fem ? 1.05f : 1.12f;
    Matrix root = MakeLocal(p->position,
                            (Vector3){ 0.0f, p->facingYaw * RAD2DEG, 0.0f },
                            (Vector3){ rootScale, rootScale, rootScale });

    // Marcador de destino final no chao (modo Mouse estilo Dota, cor neutra da paleta)
    if (p->hasMoveTarget) {
        float pulso = 0.5f + 0.5f * sinf(p->idleTime * 6.0f);
        float bob = 0.24f + 0.06f * sinf(p->idleTime * 5.0f);
        Vector3 gPos = p->moveGoalFinal;
        Color cRing  = (Color){ 189, 181, 168, 225 };
        Color cArrow = (Color){ 255, 242, 230, 255 };
        DrawCylinderWires((Vector3){ gPos.x, 0.015f, gPos.z },
                          0.20f + 0.06f * pulso, 0.20f + 0.06f * pulso, 0.015f, 18, cRing);
        DrawCylinderWires((Vector3){ gPos.x, 0.015f, gPos.z },
                          0.09f, 0.09f, 0.015f, 12, cArrow);
        rlPushMatrix();
        rlTranslatef(gPos.x, bob, gPos.z);
        rlRotatef(p->idleTime * 110.0f, 0.0f, 1.0f, 0.0f);
        // Seta apontando para baixo no ponto final clicado
        DrawCylinder((Vector3){ 0.0f, 0.08f, 0.0f }, 0.025f, 0.025f, 0.14f, 8, cArrow);
        DrawCylinder((Vector3){ 0.0f, -0.05f, 0.0f }, 0.09f, 0.00f, 0.13f, 4, cRing);
        rlPopMatrix();
    }

    // Projecao planar de sombra real do corpo no piso (sem disco circular preto)
    Matrix shadowProj = MakePlanarShadowMatrix(p->position.y + 0.008f, p->position);

    // Base do quadril (abaixa exatamente ate o assento da cadeira sem flutuar)
    float pelvY = (0.87f + bobY) * (1.0f - sitFactor) + 0.415f * sitFactor;
    float pelvZ = -0.05f * sitFactor;
    Matrix pelvisM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, pelvY, pelvZ }, (Vector3){ 0.0f, pelvisYaw * (1.0f - sitFactor), pelvisRoll * (1.0f - sitFactor) }, (Vector3){ 1, 1, 1 }),
        root
    );

    // 1) PERNAS ARTICULADAS COM JUNTAS SUAVES E PASSADA FLUIDA
    float hipX = fem ? 0.088f : 0.098f;
    for (int side = -1; side <= 1; side += 2) {
        float phase = (side < 0) ? p->walkCycle : (p->walkCycle + PI);
        float sp = sinf(phase);
        float cp = cosf(phase);
        float swingPhase = fmaxf(0.0f, cp);
        float legAng     = ((-sp * 22.0f) * blend + ((side < 0) ? 2.0f : -2.0f) * (1.0f - blend)) * (1.0f - sitFactor) + (-86.0f * sitFactor);
        float kneeBend   = ((swingPhase * 34.0f + 3.0f) * blend + ((side < 0) ? 2.5f : 4.5f) * (1.0f - blend)) * (1.0f - sitFactor) + (86.0f * sitFactor);
        float footPitch  = ((-legAng * 0.50f - cp * 7.0f) * blend) * (1.0f - sitFactor);

        Matrix hipM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * hipX, 0.0f, 0.04f * sitFactor }, (Vector3){ legAng, 0.0f, (float)side * (-1.2f + 4.5f * sitFactor) }, (Vector3){ 1, 1, 1 }),
            pelvisM
        );
        DrawPartWithShadow(fem ? g_assets.thighF : g_assets.thighM, hipM, 0.16f, false, &shadowProj);

        Matrix kneeM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.41f : -0.42f, 0.0f }, (Vector3){ kneeBend, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
            hipM
        );
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f), kneeM), 0.15f, false);
        DrawPartWithShadow(fem ? g_assets.calfF : g_assets.calfM, kneeM, 0.15f, false, &shadowProj);

        Matrix ankleM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.39f : -0.40f, 0.0f }, (Vector3){ footPitch - kneeBend * 0.45f * (1.0f - sitFactor), 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
            kneeM
        );
        DrawPartWithShadow(g_assets.shoe, ankleM, 0.06f, false, &shadowProj);
    }

    // 2) CAMADAS: CAMISA -> GRAVATA -> TRENCH COAT LONGO COM FISICA -> CRACHA
    float compLean = 7.5f * sitFactor + 6.0f * paperInspect;
    Matrix torsoBase = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.25f, 0.0f }, (Vector3){ forwardLean * (1.0f - sitFactor) + compLean, torsoYaw * (1.0f - sitFactor), -pelvisRoll * 0.65f * (1.0f - sitFactor) }, (Vector3){ 1, 1, 1 }),
        pelvisM
    );
    DrawPart(g_assets.shirt, torsoBase, 0.90f, false);
    if (!fem) {
        Matrix tieM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, 0.04f, 0.100f }, (Vector3){ 3.0f, 0, 0 }, (Vector3){ 1.0f, 1.0f, 1.0f }),
            torsoBase
        );
        DrawPart(g_assets.tie, tieM, 0.05f, false);
    } else {
        Matrix tieFemM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, 0.09f, 0.096f }, (Vector3){ 3.0f, 0, 0 }, (Vector3){ 0.65f, 0.58f, 0.72f }),
            torsoBase
        );
        DrawPart(g_assets.tie, tieFemM, 0.06f, false);
    }

    DrawPartWithShadow(fem ? g_assets.torsoF : g_assets.torsoM,
                       MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.0f, -0.010f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.04f, 1.01f, 1.02f }), torsoBase),
                       0.18f, false, &shadowProj);

    // Cracha corporativo nitido na camada frontal do peito
    DrawBadgeDetails(torsoBase, 0.04f);

    // Barra do sobretudo / trench coat com fisica dinamica mais expressiva ao andar/correr
    float coatSway = -sinf(p->walkCycle) * (10.5f * strideMult) * blend - (13.5f + 7.5f * runB) * blend - 18.0f * sitFactor;
    float coatLag  = cosf(p->walkCycle) * 6.5f * blend;
    Matrix skirtM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, -0.18f, -0.008f }, (Vector3){ coatSway, coatLag, -pelvisRoll * 0.75f }, (Vector3){ 1.03f, 1.0f, 1.02f }),
        torsoBase
    );
    DrawPartWithShadow(fem ? g_assets.coatSkirtF : g_assets.coatSkirtM, skirtM, 0.17f, false, &shadowProj);

    for (int side = -1; side <= 1; side += 2) {
        float flapX = (float)side * (fem ? 0.086f : 0.098f);
        Matrix flapM = MatrixMultiply(
            MakeLocal((Vector3){ flapX, -0.18f, 0.070f },
                      (Vector3){ coatSway * 0.90f + (float)side * coatLag * 0.65f, (float)side * 10.0f, (float)side * (fem ? 2.5f : 2.0f) },
                      (Vector3){ 1.02f, 1.0f, 1.0f }),
            torsoBase
        );
        DrawPart(fem ? g_assets.coatFlapF : g_assets.coatFlapM, flapM, 0.21f, false);

        Matrix lapelM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * (fem ? 0.064f : 0.072f), 0.10f, 0.094f },
                      (Vector3){ 4.0f, (float)side * 12.0f, (float)side * -16.0f },
                      (Vector3){ 0.88f, 1.04f, 0.88f }),
            torsoBase
        );
        DrawPart(g_assets.lapel, lapelM, 0.30f, false);
    }

    Matrix collarM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.21f, -0.010f }, (Vector3){ -6.0f, 0, 0 }, (Vector3){ 1.02f, 1.0f, 1.02f }),
        torsoBase
    );
    DrawPart(g_assets.collarBack, collarM, 0.28f, false);

    // 3) BRACOS COM JUNTAS ESFERICAS E ATRASO NATURAL DE MOVIMENTO (SEM PARECER LEGO)
    float shXBase = fem ? 0.175f : 0.212f;
    for (int side = -1; side <= 1; side += 2) {
        float legPhase = (side < 0) ? p->walkCycle : (p->walkCycle + PI);
        float armPhase = legPhase - 0.35f; // atraso natural de pendulo no braco
        float sp = sinf(armPhase);
        float armAng   = (sp * 19.0f) * blend + ((side > 0) ? -2.5f : 2.0f) * (1.0f - blend);
        float elbowAng = (-16.0f - fmaxf(0.0f, -sp) * 18.0f) * blend - (10.0f + sinf(p->idleTime * 1.9f) * 1.5f) * (1.0f - blend);
        float armOutZ  = (float)side * (fem ? 5.2f : 6.0f);
        float armYaw   = 0.0f;

        if (sitFactor > 0.05f) {
            float typeOsc = sinf(p->idleTime * 11.0f + (float)side * 2.2f);
            armAng   = -36.0f * sitFactor + typeOsc * 3.5f;
            elbowAng = -62.0f * sitFactor + typeOsc * 6.0f;
            armYaw   = (float)side * -14.0f * sitFactor;
        } else if (paperInspect > 0.05f) {
            float flipOsc = sinf(p->idleTime * 9.0f + (float)side * 1.4f);
            armAng   = -34.0f * paperInspect + flipOsc * 4.0f;
            elbowAng = -68.0f * paperInspect + flipOsc * 8.0f;
            armYaw   = (float)side * -16.0f * paperInspect;
        } else if (side < 0 && reach > 0.01f) {
            armAng = -64.0f * reach;
            elbowAng = -16.0f * (1.0f - 0.4f * reach);
        }

        Matrix shM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * shXBase, 0.18f, 0.0f },
                      (Vector3){ armAng, armYaw, armOutZ },
                      (Vector3){ 1, 1, 1 }),
            torsoBase
        );
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.048f : 0.056f, fem ? 0.048f : 0.056f, fem ? 0.048f : 0.056f), shM), fem ? 0.27f : 0.23f, false);
        DrawPartWithShadow(fem ? g_assets.upperArmF : g_assets.upperArmM, shM, fem ? 0.27f : 0.23f, false, &shadowProj);

        Matrix elbM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.26f : -0.27f, 0.0f },
                      (Vector3){ elbowAng, 0.0f, (float)side * -2.5f },
                      (Vector3){ 1, 1, 1 }),
            shM
        );
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.036f : 0.043f, fem ? 0.036f : 0.043f, fem ? 0.036f : 0.043f), elbM), fem ? 0.26f : 0.22f, false);
        DrawPartWithShadow(fem ? g_assets.forearmF : g_assets.forearmM, elbM, fem ? 0.26f : 0.22f, false, &shadowProj);

        Matrix handM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.24f : -0.25f, 0.008f },
                      (Vector3){ -8.0f - sp * 6.0f * blend, 0.0f, (float)side * -4.0f },
                      (Vector3){ 1, 1, 1 }),
            elbM
        );
        DrawPart(g_assets.hand, handM, 0.62f, false);
    }

    // 4) PESCOCO, CABECA (COR NORMAL) E CABELO COM FISICA
    Matrix neckM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.22f, 0.0f }, (Vector3){ -forwardLean * 0.55f, -torsoYaw * 0.5f, 0 }, (Vector3){ 1, 1, 1 }),
        torsoBase
    );
    DrawPart(fem ? g_assets.neckF : g_assets.neckM, neckM, 0.58f, false);

    Matrix headM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.19f, 0.012f }, (Vector3){ 4.0f, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
        neckM
    );
    DrawPartWithShadow(fem ? g_assets.headF : g_assets.headM, headM, 0.64f, false, &shadowProj);
    DrawFaceDetails(headM, fem, 1.0f);

    if (fem) {
        float hairLagX = -blend * 12.0f + sinf(p->walkCycle) * 8.5f * blend + sinf(p->idleTime * 1.6f) * 1.8f;
        float hairLagZ = -sinf(p->walkCycle) * 6.5f * blend + cosf(p->idleTime * 1.3f) * 1.5f;
        DrawPart(g_assets.hairF, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.06f, 1.05f, 1.06f }), headM), 0.09f, false);
        DrawPart(g_assets.hairBackF,
                 MatrixMultiply(MakeLocal((Vector3){ 0.0f, -0.01f, -0.005f }, (Vector3){ hairLagX, 0.0f, hairLagZ }, (Vector3){ 1.08f, 1.12f, 1.08f }), headM),
                 0.08f, false);
        for (int side = -1; side <= 1; side += 2) {
            Matrix sideHairM = MatrixMultiply(
                MakeLocal((Vector3){ (float)side * 0.072f, 0.0f, 0.005f },
                          (Vector3){ hairLagX * 0.75f, 0.0f, (float)side * -8.0f + hairLagZ * 0.65f },
                          (Vector3){ 1.06f, 1.10f, 1.06f }),
                headM
            );
            DrawPart(g_assets.hairSideF, sideHairM, 0.09f, false);
        }
        Matrix bangL = MatrixMultiply(
            MakeLocal((Vector3){ -0.028f, 0.068f, 0.078f }, (Vector3){ 12.0f + hairLagX * 0.45f, 18.0f, 16.0f }, (Vector3){ 0.95f, 0.90f, 1.0f }),
            headM
        );
        Matrix bangR = MatrixMultiply(
            MakeLocal((Vector3){  0.028f, 0.068f, 0.078f }, (Vector3){ 12.0f + hairLagX * 0.45f, -18.0f, -16.0f }, (Vector3){ 0.95f, 0.90f, 1.0f }),
            headM
        );
        DrawPart(g_assets.bangM2, bangL, 0.09f, false);
        DrawPart(g_assets.bangM1, bangR, 0.09f, false);
    } else {
        float bangSway = sinf(p->walkCycle) * 4.5f * blend;
        DrawPart(g_assets.hairM, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, -0.004f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.06f, 1.06f, 1.06f }), headM), 0.09f, false);
        Matrix bang1M = MatrixMultiply(
            MakeLocal((Vector3){ 0.014f, 0.074f, 0.082f }, (Vector3){ 10.0f + bangSway, -12.0f, -14.0f }, (Vector3){ 1.12f, 1.12f, 1.08f }),
            headM
        );
        Matrix bang2M = MatrixMultiply(
            MakeLocal((Vector3){ -0.024f, 0.072f, 0.080f }, (Vector3){ 8.0f - bangSway, 14.0f, 12.0f }, (Vector3){ 1.12f, 1.12f, 1.08f }),
            headM
        );
        DrawPart(g_assets.bangM1, bang1M, 0.10f, false);
        DrawPart(g_assets.bangM2, bang2M, 0.10f, false);
    }
}

// Desenha humano articulado em camadas (CorpoBase->Cabelo->Camisa/Calca->Casaco->Cracha)
static void DrawContextualHuman(int styleId, bool fem, Vector3 pos, float yawDeg,
                                NpcAtividade ativ, float tempo, bool isBgNpc) {
    int glbIdx = -1;
    if (glbIdx >= 0 && g_assets.glb[glbIdx].loaded) {
        Vector3 sScale = fem ? (Vector3){ 0.94f, 0.97f, 0.94f } : (Vector3){ 1.00f, 1.01f, 1.00f };
        Matrix root = MakeLocal(pos, (Vector3){ 0.0f, yawDeg, 0.0f }, sScale);
        DrawPart(g_assets.shadowDisc,
                 MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.20f, 1.0f, 1.20f }), root),
                 0.10f, false);

        int animIdx = 0; // Idle
        if (ativ == NPC_ATIV_SENTADO) animIdx = 2;
        else if (ativ == NPC_ATIV_COMPUTADOR) animIdx = 3;
        else if (ativ == NPC_ATIV_COMENDO) animIdx = 4;

        float frame = tempo * (animIdx == 0 ? 9.0f : 8.0f) + (float)styleId * 5.0f;
        float lumScale = isBgNpc ? 0.42f : 1.04f;
        float lumBias  = isBgNpc ? 0.02f : 0.02f;
        DrawCharGlb(glbIdx, root, animIdx, frame, animIdx, frame, 0.0f, lumScale, lumBias);

        bool sentado = (ativ == NPC_ATIV_SENTADO || ativ == NPC_ATIV_COMPUTADOR || ativ == NPC_ATIV_COMENDO);
        float headY = sentado ? 1.11f : 1.54f;

        if (!isBgNpc) {
            DrawPart(g_assets.badge,
                     MatrixMultiply(MakeLocal((Vector3){ 0.05f, sentado ? 0.81f : 1.24f, 0.145f }, (Vector3){ -4.0f, 0, 0 }, (Vector3){ 1.1f, 1.1f, 1.1f }), root),
                     0.92f, false);
        }
        if (fem) {
            float hLum = isBgNpc ? 0.06f : 0.12f;
            Matrix hWorld = MatrixMultiply(MakeLocal((Vector3){ 0.0f, headY, 0.01f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.10f, 1.08f, 1.10f }), root);
            if (styleId != 2) {
                DrawPart(g_assets.hairBackF, hWorld, hLum, false);
                for (int side = -1; side <= 1; side += 2) {
                    DrawPart(g_assets.hairSideF,
                             MatrixMultiply(MakeLocal((Vector3){ (float)side * 0.072f, 0.0f, 0.01f }, (Vector3){ 0, 0, (float)side * -6.0f }, (Vector3){ 1, 1, 1 }), hWorld),
                             hLum, false);
                }
            }
            if (styleId == 2) {
                // Oculos 3D da Ana Lucia
                Matrix glBase = MatrixMultiply(MakeLocal((Vector3){ 0.0f, headY + 0.02f, 0.115f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }), root);
                rlPushMatrix();
                rlMultMatrixf(MatrixToFloat(glBase));
                DrawCubeWires((Vector3){ -0.036f, 0.0f, 0.0f }, 0.048f, 0.026f, 0.014f, (Color){ 20, 18, 20, 255 });
                DrawCubeWires((Vector3){  0.036f, 0.0f, 0.0f }, 0.048f, 0.026f, 0.014f, (Color){ 20, 18, 20, 255 });
                DrawCube((Vector3){ 0.0f, 0.004f, 0.0f }, 0.022f, 0.006f, 0.008f, (Color){ 20, 18, 20, 255 });
                rlPopMatrix();
            }
        }
        if (ativ == NPC_ATIV_COMENDO) {
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(root));
            DrawCylinder((Vector3){ 0.0f, 0.74f, 0.44f }, 0.13f, 0.11f, 0.018f, 16, (Color){ 215, 206, 194, 255 });
            DrawCylinder((Vector3){ 0.18f, 0.74f, 0.48f }, 0.036f, 0.032f, 0.075f, 12, (Color){ 165, 156, 146, 255 });
            rlPopMatrix();
        } else if (ativ == NPC_ATIV_LENDO) {
            // Prancheta segurada naturalmente junto ao torso
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(root));
            rlTranslatef(0.0f, 1.06f, 0.24f);
            rlRotatef(-36.0f, 1.0f, 0.0f, 0.0f);
            DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.22f, 0.014f, 0.28f, (Color){ 205, 196, 184, 255 });
            DrawCubeWires((Vector3){ 0.0f, 0.0f, 0.0f }, 0.22f, 0.014f, 0.28f, (Color){ 70, 64, 62, 255 });
            rlPopMatrix();
        }
        return;
    }

    float phase = tempo * (styleId == 1 ? 2.2f : 1.75f) + (float)styleId * 1.4f + pos.x * 0.7f + pos.z * 0.5f;
    float breath = sinf(phase) * 0.005f;
    bool sentado = (ativ == NPC_ATIV_SENTADO || ativ == NPC_ATIV_COMPUTADOR || ativ == NPC_ATIV_COMENDO);

    float swayYaw  = sentado ? (sinf(phase * 0.5f) * 1.8f)
                             : (ativ == NPC_ATIV_DESCONFIADA ? (sinf(tempo * 0.85f) * 1.4f)
                             : (sinf(phase * 0.65f) * (styleId == 1 ? 3.5f : 2.5f)));
    float headWave = sinf(tempo * 0.75f);
    float headHold = (headWave > 0.35f) ? (headWave - 0.35f) / 0.65f
                   : ((headWave < -0.35f) ? (headWave + 0.35f) / 0.65f : 0.0f);
    float headTurn = (ativ == NPC_ATIV_DESCONFIADA)
                     ? (headHold * 13.0f)
                     : (sinf(phase * 0.55f + 0.7f) * (ativ == NPC_ATIV_COMPUTADOR ? 3.5f : 6.5f));
    float headNod  = (ativ == NPC_ATIV_COMPUTADOR || ativ == NPC_ATIV_LENDO || ativ == NPC_ATIV_COMENDO)
                     ? (12.0f + sinf(phase * 1.1f) * 2.5f)
                     : (ativ == NPC_ATIV_DESCONFIADA ? (-1.5f + sinf(tempo * 1.1f) * 0.8f) : (sinf(phase * 0.90f) * 2.0f));

    float rootScale = isBgNpc ? (fem ? 1.01f : 1.06f) : (fem ? 1.07f : 1.14f);
    Matrix root = MakeLocal(pos, (Vector3){ 0.0f, yawDeg, 0.0f }, (Vector3){ rootScale, rootScale, rootScale });

    Matrix shadowProj = MakePlanarShadowMatrix(pos.y + 0.008f, pos);
    const Matrix *spPtr = (!isBgNpc && !sentado) ? &shadowProj : NULL;
    if (isBgNpc) {
        DrawPart(g_assets.shadowDisc,
                 MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.05f, 1.0f, 1.05f }), root),
                 0.10f, false);
    }

    float lumMult = isBgNpc ? 0.55f : 1.0f;
    float pelvY   = sentado ? (0.48f + breath * 0.5f) : (0.87f + breath);
    float pelvZ   = sentado ? -0.05f : 0.0f;

    Matrix pelvisM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, pelvY, pelvZ }, (Vector3){ 0.0f, swayYaw * 0.5f, sentado ? 0.0f : 1.2f }, (Vector3){ 1, 1, 1 }),
        root
    );

    float hipX = fem ? 0.082f : 0.092f;
    for (int side = -1; side <= 1; side += 2) {
        float legAng = sentado ? -86.0f : ((side < 0) ? 2.0f : -2.5f);
        float kneeB  = sentado ?  86.0f : ((side < 0) ? 2.5f : 5.0f);
        float legZ   = sentado ? 0.04f : 0.0f;
        float legOut = sentado ? ((float)side * 3.5f) : 0.0f;
        Matrix hipM = MatrixMultiply(MakeLocal((Vector3){ (float)side * hipX, 0.0f, legZ }, (Vector3){ legAng, 0, legOut }, (Vector3){ 1, 1, 1 }), pelvisM);
        float legLum = ((styleId == 0) ? 0.22f : 0.17f) * lumMult;
        DrawPartWithShadow(fem ? g_assets.thighF : g_assets.thighM, hipM, legLum, false, spPtr);
        Matrix kneeM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.41f : -0.42f, 0.0f }, (Vector3){ kneeB, 0, 0 }, (Vector3){ 1, 1, 1 }), hipM);
        if (!isBgNpc) {
            DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f), kneeM), legLum, false);
        }
        DrawPartWithShadow(fem ? g_assets.calfF : g_assets.calfM, kneeM, legLum * 0.92f, false, spPtr);
        Matrix ankleM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.39f : -0.40f, 0.0f }, (Vector3){ sentado ? 0.0f : (-kneeB * 0.45f), 0, 0 }, (Vector3){ 1, 1, 1 }), kneeM);
        DrawPartWithShadow(g_assets.shoe, ankleM, 0.07f * lumMult, false, spPtr);
    }

    float torsoPitch = (ativ == NPC_ATIV_COMPUTADOR || ativ == NPC_ATIV_COMENDO) ? 6.5f : (ativ == NPC_ATIV_LENDO ? 4.0f : 1.2f);
    Matrix torsoBase = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.25f, 0.0f }, (Vector3){ torsoPitch, swayYaw, sentado ? 0.0f : -0.8f }, (Vector3){ 1, 1, 1 }),
        pelvisM
    );

    float shirtLum = isBgNpc ? 0.48f : 0.92f;
    DrawPart(g_assets.shirt, torsoBase, shirtLum, false);
    if (styleId == 2 || (!isBgNpc && !fem)) {
        Matrix tieM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.08f, 0.092f }, (Vector3){ 3.0f, 0, 0 }, (Vector3){ 0.65f, 0.60f, 0.72f }), torsoBase);
        DrawPart(g_assets.tie, tieM, 0.08f, false);
    }

    float coatLum = ((styleId == 0) ? 0.44f : ((styleId == 1) ? 0.30f : 0.21f)) * lumMult;
    DrawPartWithShadow(fem ? g_assets.torsoF : g_assets.torsoM,
                       MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.0f, -0.010f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.03f, 1.01f, 1.02f }), torsoBase),
                       coatLum, false, spPtr);
    if (!isBgNpc) {
        DrawPartWithShadow(fem ? g_assets.coatSkirtF : g_assets.coatSkirtM,
                           MatrixMultiply(MakeLocal((Vector3){ 0.0f, -0.18f, -0.008f }, (Vector3){ sentado ? -18.0f : 0.0f, 0, 0 }, (Vector3){ 1.02f, 1.0f, 1.02f }), torsoBase),
                           coatLum, false, spPtr);
        for (int side = -1; side <= 1; side += 2) {
            Matrix flapM = MatrixMultiply(
                MakeLocal((Vector3){ (float)side * (fem ? 0.074f : 0.086f), -0.18f, 0.064f },
                          (Vector3){ sentado ? -16.0f : 0.0f, (float)side * 8.0f, (float)side * 1.5f }, (Vector3){ 1.0f, 1.0f, 1.0f }),
                torsoBase
            );
            DrawPart(fem ? g_assets.coatFlapF : g_assets.coatFlapM, flapM, coatLum + 0.03f, false);
            Matrix lapelM = MatrixMultiply(
                MakeLocal((Vector3){ (float)side * (fem ? 0.056f : 0.064f), 0.10f, 0.086f },
                          (Vector3){ 4.0f, (float)side * 12.0f, (float)side * -16.0f }, (Vector3){ 0.85f, 1.02f, 0.85f }),
                torsoBase
            );
            DrawPart(g_assets.lapel, lapelM, coatLum + 0.09f, false);
        }
        DrawPart(g_assets.collarBack,
                 MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.21f, -0.010f }, (Vector3){ -6.0f, 0, 0 }, (Vector3){ 1.02f, 1.0f, 1.02f }), torsoBase),
                 coatLum + 0.08f, false);
        DrawBadgeDetails(torsoBase, 0.03f);
    }

    float shXBase = fem ? 0.175f : 0.212f;
    for (int side = -1; side <= 1; side += 2) {
        float armAng   = (side < 0 ? 1.5f : -2.0f);
        float elbowAng = -9.0f - sinf(phase + (float)side) * 1.5f;
        float armOutZ  = (float)side * (fem ? 4.8f : 5.6f);
        float armYaw   = 0.0f;

        if (ativ == NPC_ATIV_COMPUTADOR) {
            float typeOsc = sinf(tempo * 9.5f + (float)side * 2.2f);
            armAng   = -36.0f + typeOsc * 2.5f;
            elbowAng = -62.0f + typeOsc * 5.0f;
            armYaw   = (float)side * -14.0f;
        } else if (ativ == NPC_ATIV_COMENDO) {
            if (side > 0) {
                float eatOsc = 0.5f + 0.5f * sinf(tempo * 3.4f + (float)styleId);
                armAng   = -34.0f - eatOsc * 18.0f;
                elbowAng = -60.0f - eatOsc * 24.0f;
                armYaw   = -16.0f;
            } else {
                armAng   = -26.0f;
                elbowAng = -64.0f;
                armYaw   = 12.0f;
            }
        } else if (ativ == NPC_ATIV_LENDO) {
            if (side > 0) {
                armAng   = -38.0f;
                elbowAng = -74.0f + sinf(tempo * 2.2f) * 3.0f;
                armYaw   = -18.0f;
            } else {
                armAng   = -10.0f;
                elbowAng = -24.0f;
            }
        } else if (ativ == NPC_ATIV_CARREGANDO_PAPEIS) {
            float carryBob = sinf(tempo * 2.8f) * 1.5f;
            armAng   = -32.0f + carryBob;
            elbowAng = -82.0f;
            armYaw   = (float)side * -22.0f;
        } else if (ativ == NPC_ATIV_DESCONFIADA) {
            // Postura executiva ereta e controlada: bracos cruzados / maos juntas a frente
            armAng   = -16.0f;
            elbowAng = -68.0f + sinf(tempo * 1.1f) * 1.0f;
            armYaw   = (float)side * -24.0f;
        } else if (ativ == NPC_ATIV_SENTADO) {
            armAng   = -18.0f;
            elbowAng = -48.0f + sinf(phase) * 2.0f;
            armYaw   = (float)side * -10.0f;
        }

        Matrix shM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * shXBase, 0.18f, 0.0f }, (Vector3){ armAng, armYaw, armOutZ }, (Vector3){ 1, 1, 1 }),
            torsoBase
        );
        DrawPart(fem ? g_assets.upperArmF : g_assets.upperArmM, shM, coatLum + 0.05f, false);
        Matrix elbM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.26f : -0.27f, 0.0f }, (Vector3){ elbowAng, 0, (float)side * -2.0f }, (Vector3){ 1, 1, 1 }),
            shM
        );
        DrawPart(fem ? g_assets.forearmF : g_assets.forearmM, elbM, coatLum + 0.04f, false);
        Matrix handM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.24f : -0.25f, 0.008f }, (Vector3){ -6.0f, 0, 0 }, (Vector3){ 1, 1, 1 }),
            elbM
        );
        DrawPart(g_assets.hand, handM, 0.64f * lumMult, false);

        if (side > 0 && ativ == NPC_ATIV_LENDO) {
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(handM));
            DrawCube((Vector3){ -0.02f, -0.06f, 0.04f }, 0.18f, 0.015f, 0.24f, (Color){ 210, 202, 190, 255 });
            rlPopMatrix();
        } else if (side > 0 && ativ == NPC_ATIV_COMENDO) {
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(handM));
            DrawCube((Vector3){ 0.0f, -0.05f, 0.06f }, 0.012f, 0.012f, 0.14f, (Color){ 195, 188, 178, 255 });
            rlPopMatrix();
        }
    }

    if (ativ == NPC_ATIV_CARREGANDO_PAPEIS) {
        // Pilha de papeis e pastas carregada por Arthur nos bracos
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(torsoBase));
        rlTranslatef(0.0f, -0.02f, 0.25f);
        rlRotatef(4.0f, 0.0f, 1.0f, 0.0f);
        DrawCube((Vector3){ 0.0f, 0.00f, 0.0f }, 0.28f, 0.025f, 0.22f, (Color){ 175, 164, 150, 255 });
        DrawCube((Vector3){ 0.01f, 0.025f, 0.0f }, 0.26f, 0.022f, 0.21f, (Color){ 238, 232, 222, 255 });
        DrawCube((Vector3){ -0.01f, 0.048f, 0.01f }, 0.25f, 0.020f, 0.20f, (Color){ 212, 204, 194, 255 });
        DrawCube((Vector3){ 0.0f, 0.070f, -0.01f }, 0.27f, 0.022f, 0.21f, (Color){ 246, 240, 230, 255 });
        DrawCubeWires((Vector3){ 0.0f, 0.035f, 0.0f }, 0.28f, 0.095f, 0.22f, (Color){ 80, 74, 72, 255 });
        rlPopMatrix();
    } else if (ativ == NPC_ATIV_COMENDO) {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(root));
        DrawCylinder((Vector3){ 0.0f, 0.74f, 0.42f }, 0.13f, 0.11f, 0.018f, 16, (Color){ 215, 206, 194, 255 });
        DrawCylinder((Vector3){ 0.18f, 0.74f, 0.46f }, 0.036f, 0.032f, 0.075f, 12, (Color){ 165, 156, 146, 255 });
        rlPopMatrix();
    }

    Matrix neckM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.22f, 0.0f }, (Vector3){ -1.0f, headTurn * 0.4f, 0 }, (Vector3){ 1, 1, 1 }),
        torsoBase
    );
    DrawPart(fem ? g_assets.neckF : g_assets.neckM, neckM, 0.62f * lumMult, false);

    Matrix headM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.19f, 0.012f }, (Vector3){ 3.5f + headNod, headTurn * 0.6f, 0 }, (Vector3){ 1, 1, 1 }),
        neckM
    );
    DrawPart(fem ? g_assets.headF : g_assets.headM, headM, 0.68f * lumMult, false);
    if (!isBgNpc) {
        DrawFaceDetails(headM, fem, lumMult);
    }

    if (fem) {
        float hLum = (styleId == 0 ? 0.14f : 0.16f) * lumMult;
        DrawPart(g_assets.hairF, headM, hLum, false);
        if (styleId != 2) {
            DrawPart(g_assets.hairBackF, headM, hLum * 0.9f, false);
            for (int side = -1; side <= 1; side += 2) {
                Matrix sideHairM = MatrixMultiply(MakeLocal((Vector3){ (float)side * 0.068f, 0.01f, 0.005f }, (Vector3){ 0, 0, (float)side * -7.0f }, (Vector3){ 1, 1, 1 }), headM);
                DrawPart(g_assets.hairSideF, sideHairM, hLum, false);
            }
        }
        DrawPart(g_assets.bangM2, MatrixMultiply(MakeLocal((Vector3){ -0.028f, 0.068f, 0.080f }, (Vector3){ 14.0f, 20.0f, 18.0f }, (Vector3){ 1.0f, 0.90f, 1.0f }), headM), hLum, false);
        DrawPart(g_assets.bangM1, MatrixMultiply(MakeLocal((Vector3){  0.034f, 0.064f, 0.078f }, (Vector3){ 12.0f, -14.0f, -12.0f }, (Vector3){ 0.85f, 0.85f, 1.0f }), headM), hLum, false);
        if (styleId == 2) {
            Matrix glBase = MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.024f, 0.095f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }), headM);
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(glBase));
            DrawCubeWires((Vector3){ -0.032f, 0.0f, 0.0f }, 0.044f, 0.024f, 0.012f, (Color){ 20, 18, 20, 255 });
            DrawCubeWires((Vector3){  0.032f, 0.0f, 0.0f }, 0.044f, 0.024f, 0.012f, (Color){ 20, 18, 20, 255 });
            DrawCube((Vector3){ 0.0f, 0.004f, 0.0f }, 0.020f, 0.005f, 0.008f, (Color){ 20, 18, 20, 255 });
            rlPopMatrix();
        }
    } else {
        float hLum = 0.09f * lumMult;
        DrawPart(g_assets.hairM, headM, hLum, false);
        DrawPart(g_assets.bangM1, MatrixMultiply(MakeLocal((Vector3){  0.012f, 0.082f, 0.072f }, (Vector3){ -18.0f, -10.0f, -12.0f }, (Vector3){ 1.1f, 1.15f, 1.1f }), headM), hLum + 0.01f, false);
        DrawPart(g_assets.bangM2, MatrixMultiply(MakeLocal((Vector3){ -0.022f, 0.084f, 0.068f }, (Vector3){ -22.0f, 14.0f, 10.0f }, (Vector3){ 1.1f, 1.15f, 1.1f }), headM), hLum + 0.01f, false);
    }
}

void PlayerCharacterDrawModelPreview3D(bool isFemale, Vector3 pos, float yawDeg, float tempo, Vector3 camPos) {
    PlayerCharacter dummy;
    PlayerCharacterInit(&dummy, pos, yawDeg * DEG2RAD);
    dummy.isFemale = isFemale;
    dummy.idleTime = tempo;
    PlayerCharacterDraw3D(&dummy, camPos);
}

// Desenha variantes aprimoradas do modelo do player (0=Masc Noir, 1=Masc Corp, 2=Fem Noir, 3=Fem Exec)
// com todas as animacoes testaveis (idle, andar, correr, computador, papel/mapa, porta).
void PlayerCharacterDrawVariant3D(int variantId, PlayerPreviewAnim anim, Vector3 pos, float yawDeg, float tempo, Vector3 camPos) {
    EnsurePlayerAssets();
    if (!g_assets.initialized) return;

    float viewArr[3] = { camPos.x, camPos.y, camPos.z };
    SetShaderValue(g_assets.shader, g_assets.locViewPos, viewArr, SHADER_UNIFORM_VEC3);

    int v = (variantId < 0) ? 0 : (variantId % 4);
    bool fem         = (v == 2 || v == 3);
    bool longCoat    = (v == 0 || v == 2); // V0 e V2: sobretudo longo; V1 e V3: blazer executivo
    bool hasGlasses  = (v == 0 || v == 3);
    bool hasScarf    = (v == 2);
    bool hasBunHair  = (v == 3);
    bool hasWatch    = (v == 1);

    float coatLum  = (v == 0) ? 0.16f : ((v == 1) ? 0.36f : ((v == 2) ? 0.20f : 0.42f));
    float lapelLum = (v == 0) ? 0.30f : ((v == 1) ? 0.48f : ((v == 2) ? 0.34f : 0.56f));
    float pantLum  = (v == 0) ? 0.15f : ((v == 1) ? 0.18f : ((v == 2) ? 0.16f : 0.20f));
    float shirtLum = (v == 3) ? 0.84f : 0.92f;

    float blend = 0.0f;
    float runB  = 0.0f;
    float walkCycle = 0.0f;
    float sitFactor = 0.0f;
    float paperInspect = 0.0f;
    float reach = 0.0f;

    if (anim == PREVIEW_ANIM_WALK) {
        blend = 1.0f;
        walkCycle = tempo * 6.5f;
    } else if (anim == PREVIEW_ANIM_RUN) {
        blend = 1.0f;
        runB  = 1.0f;
        walkCycle = tempo * 10.5f;
    } else if (anim == PREVIEW_ANIM_COMPUTER) {
        sitFactor = 1.0f;
    } else if (anim == PREVIEW_ANIM_PAPER) {
        paperInspect = 0.85f + 0.15f * sinf(tempo * 2.2f);
    } else if (anim == PREVIEW_ANIM_DOOR) {
        float cyc = fmodf(tempo, 2.0f);
        reach = (cyc < 1.0f) ? sinf(cyc * PI) : 0.0f;
    }

    float strideMult = 1.0f + 0.36f * runB;
    float s  = sinf(walkCycle);
    float c2 = cosf(2.0f * walkCycle);

    float idleBreath  = sinf(tempo * 1.9f) * 0.006f * (1.0f - blend);
    float walkBob     = (-c2 * 0.022f * strideMult) * blend;
    float bobY        = walkBob + idleBreath;
    float idleHipRoll = 1.4f * (1.0f - blend);
    float pelvisYaw   = s * 5.5f * blend + 1.5f * (1.0f - blend);
    float pelvisRoll  = s * 2.6f * blend + idleHipRoll;
    float torsoYaw    = -s * 6.2f * blend - 1.2f * (1.0f - blend);
    float forwardLean = (6.5f + 6.5f * runB) * blend + 1.0f * (1.0f - blend);

    float rootScale = fem ? 1.06f : 1.13f;
    Matrix root = MakeLocal(pos, (Vector3){ 0.0f, yawDeg, 0.0f }, (Vector3){ rootScale, rootScale, rootScale });
    Matrix shadowProj = MakePlanarShadowMatrix(pos.y + 0.008f, pos);

    float pelvY = (0.87f + bobY) * (1.0f - sitFactor) + 0.42f * sitFactor;
    float pelvZ = -0.05f * sitFactor;
    Matrix pelvisM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, pelvY, pelvZ },
                  (Vector3){ 0.0f, pelvisYaw * (1.0f - sitFactor), pelvisRoll * (1.0f - sitFactor) },
                  (Vector3){ 1, 1, 1 }),
        root
    );

    // 1) Pernas articuladas
    float hipX = fem ? 0.086f : 0.098f;
    for (int side = -1; side <= 1; side += 2) {
        float phase = (side < 0) ? walkCycle : (walkCycle + PI);
        float sp = sinf(phase);
        float cp = cosf(phase);
        float swingPhase = fmaxf(0.0f, cp);
        float legAng   = ((-sp * 24.0f * strideMult) * blend + ((side < 0) ? 2.0f : -2.0f) * (1.0f - blend)) * (1.0f - sitFactor) + (-86.0f * sitFactor);
        float kneeBend = ((swingPhase * 36.0f * strideMult + 3.0f) * blend + ((side < 0) ? 2.5f : 4.5f) * (1.0f - blend)) * (1.0f - sitFactor) + (86.0f * sitFactor);
        float footPitch = ((-legAng * 0.50f - cp * 7.0f) * blend) * (1.0f - sitFactor);

        Matrix hipM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * hipX, 0.0f, 0.04f * sitFactor },
                      (Vector3){ legAng, 0.0f, (float)side * (-1.0f + 4.5f * sitFactor) },
                      (Vector3){ 1, 1, 1 }),
            pelvisM
        );
        DrawPartWithShadow(fem ? g_assets.thighF : g_assets.thighM, hipM, pantLum, false, &shadowProj);
        Matrix kneeM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.41f : -0.42f, 0.0f }, (Vector3){ kneeBend, 0, 0 }, (Vector3){ 1, 1, 1 }), hipM);
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f, fem ? 0.056f : 0.064f), kneeM), pantLum * 0.95f, false);
        DrawPartWithShadow(fem ? g_assets.calfF : g_assets.calfM, kneeM, pantLum * 0.92f, false, &shadowProj);
        Matrix ankleM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.39f : -0.40f, 0.0f }, (Vector3){ footPitch - kneeBend * 0.45f * (1.0f - sitFactor), 0, 0 }, (Vector3){ 1, 1, 1 }), kneeM);
        DrawPartWithShadow(g_assets.shoe, ankleM, 0.06f, false, &shadowProj);
    }

    // 2) Tronco, camisa, cinto com fivela, colete/paleto/sobretudo, botoes e cracha
    float compLean = 8.0f * sitFactor + 7.0f * paperInspect;
    Matrix torsoBase = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.25f, 0.0f },
                  (Vector3){ forwardLean * (1.0f - sitFactor) + compLean, torsoYaw * (1.0f - sitFactor), -pelvisRoll * 0.65f * (1.0f - sitFactor) },
                  (Vector3){ 1, 1, 1 }),
        pelvisM
    );

    DrawPart(g_assets.shirt, torsoBase, shirtLum, false);
    if (v == 1) {
        // Colete escuro interno sob o blazer claro (Masc Corp)
        DrawPart(g_assets.torsoM, MatrixMultiply(MakeLocal((Vector3){ 0.0f, -0.02f, 0.008f }, (Vector3){ 0, 0, 0 }, (Vector3){ 0.97f, 0.95f, 0.98f }), torsoBase), 0.14f, false);
    } else if (!fem) {
        DrawPart(g_assets.tie, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.04f, 0.100f }, (Vector3){ 3.0f, 0, 0 }, (Vector3){ 1.0f, 1.0f, 1.0f }), torsoBase), 0.06f, false);
    } else if (v == 3) {
        DrawPart(g_assets.tie, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.09f, 0.096f }, (Vector3){ 3.0f, 0, 0 }, (Vector3){ 0.65f, 0.58f, 0.72f }), torsoBase), 0.10f, false);
    }

    DrawPartWithShadow(fem ? g_assets.torsoF : g_assets.torsoM,
                       MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.0f, -0.010f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.04f, 1.01f, 1.02f }), torsoBase),
                       coatLum, false, &shadowProj);

    // Cinto marcado na cintura + fivela metalica
    DrawPart(g_assets.belt, MatrixMultiply(MakeLocal((Vector3){ 0.0f, -0.165f, 0.002f }, (Vector3){ 0, 0, 0 }, (Vector3){ fem ? 0.98f : 1.06f, 1.0f, 1.04f }), torsoBase), 0.08f, false);
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(torsoBase));
    DrawCube((Vector3){ 0.0f, -0.165f, 0.118f }, 0.046f, 0.032f, 0.012f, (Color){ 215, 206, 194, 255 });
    // Botoes verticais do paleto/sobretudo
    DrawCube((Vector3){ 0.035f, -0.04f, 0.112f }, 0.014f, 0.014f, 0.008f, (Color){ 170, 162, 152, 255 });
    DrawCube((Vector3){ 0.035f, -0.10f, 0.112f }, 0.014f, 0.014f, 0.008f, (Color){ 170, 162, 152, 255 });
    rlPopMatrix();

    // Echarpe / gola alta clara (Fem Noir)
    if (hasScarf) {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(torsoBase));
        DrawCylinder((Vector3){ 0.0f, 0.19f, 0.006f }, 0.095f, 0.108f, 0.075f, 14, (Color){ 218, 210, 198, 255 });
        DrawCube((Vector3){ -0.03f, 0.09f, 0.106f }, 0.068f, 0.15f, 0.022f, (Color){ 204, 196, 184, 255 });
        rlPopMatrix();
    }

    DrawBadgeDetails(torsoBase, 0.04f);

    // Barra do sobretudo (longa em V0/V2, blazer curto em V1/V3)
    float coatSway = -sinf(walkCycle) * (10.5f * strideMult) * blend - (13.5f + 7.5f * runB) * blend - 18.0f * sitFactor;
    float coatLag  = cosf(walkCycle) * 6.5f * blend;
    float skirtScaleY = longCoat ? 1.0f : 0.44f;
    Matrix skirtM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, -0.18f, -0.008f }, (Vector3){ coatSway, coatLag, -pelvisRoll * 0.75f }, (Vector3){ 1.03f, skirtScaleY, 1.02f }),
        torsoBase
    );
    DrawPartWithShadow(fem ? g_assets.coatSkirtF : g_assets.coatSkirtM, skirtM, coatLum - 0.01f, false, &shadowProj);

    for (int side = -1; side <= 1; side += 2) {
        float flapX = (float)side * (fem ? 0.086f : 0.098f);
        Matrix flapM = MatrixMultiply(
            MakeLocal((Vector3){ flapX, -0.18f, 0.070f },
                      (Vector3){ coatSway * 0.90f + (float)side * coatLag * 0.65f, (float)side * 10.0f, (float)side * (fem ? 2.5f : 2.0f) },
                      (Vector3){ 1.02f, skirtScaleY, 1.0f }),
            torsoBase
        );
        DrawPart(fem ? g_assets.coatFlapF : g_assets.coatFlapM, flapM, coatLum + 0.03f, false);

        Matrix lapelM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * (fem ? 0.064f : 0.072f), 0.10f, 0.094f },
                      (Vector3){ 4.0f, (float)side * 12.0f, (float)side * -16.0f },
                      (Vector3){ 0.88f, 1.04f, 0.88f }),
            torsoBase
        );
        DrawPart(g_assets.lapel, lapelM, lapelLum, false);
    }
    DrawPart(g_assets.collarBack, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.21f, -0.010f }, (Vector3){ -6.0f, 0, 0 }, (Vector3){ 1.02f, 1.0f, 1.02f }), torsoBase), lapelLum - 0.02f, false);

    // 3) Bracos com punhos brancos + relogio + prancheta na animacao de papel
    float shXBase = fem ? 0.175f : 0.212f;
    for (int side = -1; side <= 1; side += 2) {
        float legPhase = (side < 0) ? walkCycle : (walkCycle + PI);
        float armPhase = legPhase - 0.35f;
        float sp = sinf(armPhase);
        float armAng   = (sp * 21.0f * strideMult) * blend + ((side > 0) ? -2.5f : 2.0f) * (1.0f - blend);
        float elbowAng = (-18.0f - fmaxf(0.0f, -sp) * 22.0f * strideMult) * blend - (10.0f + sinf(tempo * 1.9f) * 1.5f) * (1.0f - blend);
        float armOutZ  = (float)side * (fem ? 5.2f : 6.0f);
        float armYaw   = 0.0f;

        if (sitFactor > 0.05f) {
            float typeOsc = sinf(tempo * 11.0f + (float)side * 2.2f);
            armAng   = -36.0f * sitFactor + typeOsc * 3.5f;
            elbowAng = -62.0f * sitFactor + typeOsc * 6.0f;
            armYaw   = (float)side * -14.0f * sitFactor;
        } else if (paperInspect > 0.05f) {
            float flipOsc = sinf(tempo * 7.5f + (float)side * 1.4f);
            armAng   = -34.0f * paperInspect + flipOsc * 3.5f;
            elbowAng = -68.0f * paperInspect + flipOsc * 6.5f;
            armYaw   = (float)side * -16.0f * paperInspect;
        } else if (side < 0 && reach > 0.01f) {
            armAng = -66.0f * reach;
            elbowAng = -16.0f * (1.0f - 0.4f * reach);
        }

        Matrix shM = MatrixMultiply(MakeLocal((Vector3){ (float)side * shXBase, 0.18f, 0.0f }, (Vector3){ armAng, armYaw, armOutZ }, (Vector3){ 1, 1, 1 }), torsoBase);
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.048f : 0.056f, fem ? 0.048f : 0.056f, fem ? 0.048f : 0.056f), shM), coatLum + 0.05f, false);
        DrawPartWithShadow(fem ? g_assets.upperArmF : g_assets.upperArmM, shM, coatLum + 0.05f, false, &shadowProj);

        Matrix elbM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.26f : -0.27f, 0.0f }, (Vector3){ elbowAng, 0, (float)side * -2.5f }, (Vector3){ 1, 1, 1 }), shM);
        DrawPart(g_assets.jointSphere, MatrixMultiply(MatrixScale(fem ? 0.036f : 0.043f, fem ? 0.036f : 0.043f, fem ? 0.036f : 0.043f), elbM), coatLum + 0.04f, false);
        DrawPartWithShadow(fem ? g_assets.forearmF : g_assets.forearmM, elbM, coatLum + 0.04f, false, &shadowProj);

        // Punho branco discreto da camisa + relogio no pulso esquerdo (V1)
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(elbM));
        DrawCylinder((Vector3){ 0.0f, fem ? -0.225f : -0.235f, 0.0f }, 0.036f, 0.034f, 0.026f, 10, (Color){ 232, 224, 214, 255 });
        if (hasWatch && side < 0) {
            DrawCylinder((Vector3){ 0.0f, -0.242f, 0.0f }, 0.038f, 0.038f, 0.014f, 10, (Color){ 32, 29, 31, 255 });
            DrawCube((Vector3){ 0.0f, -0.242f, 0.036f }, 0.020f, 0.016f, 0.008f, (Color){ 215, 206, 194, 255 });
        }
        rlPopMatrix();

        Matrix handM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, fem ? -0.24f : -0.25f, 0.008f }, (Vector3){ -8.0f - sp * 6.0f * blend, 0, (float)side * -4.0f }, (Vector3){ 1, 1, 1 }), elbM);
        DrawPart(g_assets.hand, handM, 0.64f, false);

        if (paperInspect > 0.3f && side > 0) {
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(handM));
            DrawCube((Vector3){ -0.05f, -0.04f, 0.05f }, 0.20f, 0.015f, 0.26f, (Color){ 230, 222, 210, 255 });
            DrawCubeWires((Vector3){ -0.05f, -0.04f, 0.05f }, 0.20f, 0.015f, 0.26f, (Color){ 42, 38, 40, 255 });
            rlPopMatrix();
        }
    }

    // 4) Pescoco, cabeca, oculos e penteados distintos
    Matrix neckM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.22f, 0.0f }, (Vector3){ -forwardLean * 0.55f, -torsoYaw * 0.5f, 0 }, (Vector3){ 1, 1, 1 }), torsoBase);
    DrawPart(fem ? g_assets.neckF : g_assets.neckM, neckM, 0.58f, false);
    Matrix headM = MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.19f, 0.012f }, (Vector3){ 4.0f + 8.0f * paperInspect, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }), neckM);
    DrawPartWithShadow(fem ? g_assets.headF : g_assets.headM, headM, 0.64f, false, &shadowProj);
    DrawFaceDetails(headM, fem, 1.0f);

    if (hasGlasses) {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(headM));
        DrawCubeWires((Vector3){ -0.030f, 0.020f, 0.095f }, 0.042f, 0.022f, 0.008f, (Color){ 20, 18, 20, 255 });
        DrawCubeWires((Vector3){  0.030f, 0.020f, 0.095f }, 0.042f, 0.022f, 0.008f, (Color){ 20, 18, 20, 255 });
        DrawCube((Vector3){ 0.0f, 0.024f, 0.095f }, 0.018f, 0.005f, 0.008f, (Color){ 20, 18, 20, 255 });
        rlPopMatrix();
    }

    if (fem) {
        float hairLagX = -blend * 12.0f + sinf(walkCycle) * 8.5f * blend + sinf(tempo * 1.6f) * 1.6f;
        float hairLagZ = -sinf(walkCycle) * 6.5f * blend + cosf(tempo * 1.3f) * 1.4f;
        DrawPart(g_assets.hairF, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.06f, 1.05f, 1.06f }), headM), 0.09f, false);
        if (hasBunHair) {
            // Coque elegante na nuca (Fem Exec)
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(headM));
            DrawSphereEx((Vector3){ 0.0f, 0.025f, -0.105f }, 0.056f, 10, 10, (Color){ 26, 23, 25, 255 });
            rlPopMatrix();
        } else {
            DrawPart(g_assets.hairBackF, MatrixMultiply(MakeLocal((Vector3){ 0.0f, -0.01f, -0.005f }, (Vector3){ hairLagX, 0, hairLagZ }, (Vector3){ 1.08f, 1.12f, 1.08f }), headM), 0.08f, false);
            for (int side = -1; side <= 1; side += 2) {
                DrawPart(g_assets.hairSideF, MatrixMultiply(MakeLocal((Vector3){ (float)side * 0.072f, 0.0f, 0.005f }, (Vector3){ hairLagX * 0.75f, 0, (float)side * -8.0f + hairLagZ * 0.65f }, (Vector3){ 1.06f, 1.10f, 1.06f }), headM), 0.09f, false);
            }
        }
        DrawPart(g_assets.bangM2, MatrixMultiply(MakeLocal((Vector3){ -0.028f, 0.068f, 0.078f }, (Vector3){ 12.0f, 18.0f, 16.0f }, (Vector3){ 0.95f, 0.90f, 1.0f }), headM), 0.09f, false);
        DrawPart(g_assets.bangM1, MatrixMultiply(MakeLocal((Vector3){  0.028f, 0.068f, 0.078f }, (Vector3){ 12.0f, -18.0f, -16.0f }, (Vector3){ 0.95f, 0.90f, 1.0f }), headM), 0.09f, false);
    } else {
        float bangSway = sinf(walkCycle) * 4.5f * blend;
        float hairScaleY = (v == 1) ? 1.14f : 1.06f;
        DrawPart(g_assets.hairM, MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, -0.004f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.06f, hairScaleY, 1.06f }), headM), 0.09f, false);
        DrawPart(g_assets.bangM1, MatrixMultiply(MakeLocal((Vector3){  0.014f, 0.074f, 0.082f }, (Vector3){ 10.0f + bangSway, -12.0f, -14.0f }, (Vector3){ 1.12f, 1.12f, 1.08f }), headM), 0.10f, false);
        DrawPart(g_assets.bangM2, MatrixMultiply(MakeLocal((Vector3){ -0.024f, 0.072f, 0.080f }, (Vector3){  8.0f - bangSway,  14.0f,  12.0f }, (Vector3){ 1.12f, 1.12f, 1.08f }), headM), 0.10f, false);
    }
}

void PlayerCharacterDrawSuspect3DEx(int suspeito, Vector3 pos, float yawDeg, NpcAtividade ativ, float tempo, Vector3 camPos) {
    EnsurePlayerAssets();
    if (!g_assets.initialized || suspeito < 0 || suspeito > 2) return;

    float viewArr[3] = { camPos.x, camPos.y, camPos.z };
    SetShaderValue(g_assets.shader, g_assets.locViewPos, viewArr, SHADER_UNIFORM_VEC3);

    float pulso = 0.5f + 0.5f * sinf(tempo * 4.2f + (float)suspeito);
    Color cOuro = (Color){ 255, (unsigned char)(215 + (int)(35.0f * pulso)), 36, 255 };
    DrawCylinderWires((Vector3){ pos.x, pos.y + 0.018f, pos.z }, 0.42f + 0.04f * pulso, 0.42f + 0.04f * pulso, 0.018f, 20, cOuro);
    float baseBeaconY = (ativ == NPC_ATIV_SENTADO || ativ == NPC_ATIV_COMPUTADOR || ativ == NPC_ATIV_COMENDO) ? 1.66f : 2.02f;
    float bobBeacon = pos.y + baseBeaconY + 0.06f * sinf(tempo * 4.0f + (float)suspeito);
    rlPushMatrix();
    rlTranslatef(pos.x, bobBeacon, pos.z);
    rlRotatef(tempo * 90.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder((Vector3){ 0.0f,  0.00f, 0.0f }, 0.090f, 0.00f, 0.13f, 4, (Color){ 255, 240, 56, 255 });
    DrawCylinder((Vector3){ 0.0f, -0.13f, 0.0f }, 0.00f, 0.090f, 0.13f, 4, cOuro);
    rlPopMatrix();

    bool fem = (suspeito == 0 || suspeito == 2);
    DrawContextualHuman(suspeito, fem, pos, yawDeg, ativ, tempo, false);
}

void PlayerCharacterDrawSuspect3D(int suspeito, Vector3 pos, float yawDeg, float tempo, Vector3 camPos) {
    PlayerCharacterDrawSuspect3DEx(suspeito, pos, yawDeg, NPC_ATIV_EM_PE, tempo, camPos);
}

void PlayerCharacterDrawNpc3DEx(Vector3 pos, float yawDeg, NpcAtividade ativ, int variante, float tempo, Vector3 camPos) {
    EnsurePlayerAssets();
    if (!g_assets.initialized) return;

    float viewArr[3] = { camPos.x, camPos.y, camPos.z };
    SetShaderValue(g_assets.shader, g_assets.locViewPos, viewArr, SHADER_UNIFORM_VEC3);

    bool fem = ((variante & 1) == 1);
    DrawContextualHuman(3 + (variante % 3), fem, pos, yawDeg, ativ, tempo, true);
}

void PlayerCharacterDrawNpc3D(Vector3 pos, float yawDeg, bool sentado, float tempo, Vector3 camPos) {
    int hash = (int)(fabsf(roundf(pos.x * 2.0f) * 13.0f + roundf(pos.z * 2.0f) * 7.0f));
    PlayerCharacterDrawNpc3DEx(pos, yawDeg, sentado ? NPC_ATIV_SENTADO : NPC_ATIV_EM_PE, hash, tempo, camPos);
}
