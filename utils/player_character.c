#include "player_character.h"
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
    p->position      = startPos;
    p->velocity      = (Vector2){ 0.0f, 0.0f };
    p->radius        = 0.28f;
    p->speed         = 2.95f;
    p->facingYaw     = startYaw;
    p->walkCycle     = 0.0f;
    p->moveBlend     = 0.0f;
    p->idleTime      = 0.0f;
    p->interactTimer = 0.0f;
    p->isMoving      = false;
    p->isFemale      = false;
}

void PlayerCharacterSetPose(PlayerCharacter *p, Vector3 pos, float yaw) {
    if (!p) return;
    p->position      = pos;
    p->velocity      = (Vector2){ 0.0f, 0.0f };
    p->facingYaw     = yaw;
    p->walkCycle     = 0.0f;
    p->moveBlend     = 0.0f;
    p->interactTimer = 0.0f;
    p->isMoving      = false;
}

void PlayerCharacterTriggerInteract(PlayerCharacter *p, Vector3 targetPos) {
    if (!p) return;
    float dx = targetPos.x - p->position.x;
    float dz = targetPos.z - p->position.z;
    if (dx * dx + dz * dz > 1e-4f) {
        p->facingYaw = atan2f(dx, dz);
    }
    p->interactTimer = 0.42f;
}

void PlayerCharacterUpdate(PlayerCharacter *p,
                           float dt,
                           Vector3 camPos,
                           Vector3 camTarget,
                           Rectangle roomBoundsXZ,
                           const PlayerColliderBox *obstacles,
                           int numObstacles) {
    if (!p) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;

    p->idleTime += dt;
    if (p->interactTimer > 0.0f) {
        p->interactTimer -= dt;
        if (p->interactTimer < 0.0f) p->interactTimer = 0.0f;
    }

    float inRight = 0.0f;
    float inFwd   = 0.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) inRight += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  inRight -= 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    inFwd   += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  inFwd   -= 1.0f;

    Vector2 targetVel = { 0.0f, 0.0f };
    bool hasInput = (inRight != 0.0f || inFwd != 0.0f);
    if (hasInput) {
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
        targetVel.x = moveX * p->speed;
        targetVel.y = moveZ * p->speed;

        float targetYaw = atan2f(moveX, moveZ);
        float diffYaw = WrapAngleRad(targetYaw - p->facingYaw);
        p->facingYaw = WrapAngleRad(p->facingYaw + diffYaw * (1.0f - expf(-12.5f * dt)));
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
    ResolverPosicaoXZ(&nx, &nz, p->radius, roomBoundsXZ, obstacles, numObstacles);

    float actualDist = hypotf(nx - p->position.x, nz - p->position.z);
    p->position.x = nx;
    p->position.z = nz;

    float actualSpeed = (dt > 1e-5f) ? (actualDist / dt) : 0.0f;
    float speedRatio = Clamp(actualSpeed / p->speed, 0.0f, 1.0f);
    p->isMoving = (speedRatio > 0.05f);

    // Blend suave entre idle e caminhada
    float blendRate = p->isMoving ? 10.0f : 8.5f;
    p->moveBlend += (speedRatio - p->moveBlend) * (1.0f - expf(-blendRate * dt));
    if (p->moveBlend < 0.005f && !p->isMoving) p->moveBlend = 0.0f;

    if (p->moveBlend > 0.01f) {
        // Cadencia de caminhada apressada fluida (~9.3 rad/s), sem parecer corrida
        p->walkCycle += dt * (3.5f + 5.8f * speedRatio);
        if (p->walkCycle > 2.0f * PI) p->walkCycle -= 2.0f * PI;
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
    bool     initialized;
    Shader   shader;
    int      locViewPos;
    int      locFaceShadow;
    Material mat;

    // Malhas compartilhadas / por genero
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

    // Tronco masculino esguio e atletico (ombros definidos, cintura enxuta)
    const RingSpec torsoRingsM[] = {
        { -0.26f, 0.142f, 0.094f, 0, -0.004f, 0.003f, 0, 0 },
        { -0.10f, 0.132f, 0.088f, 0,  0.000f, 0.004f, 0, 0 },
        {  0.06f, 0.162f, 0.102f, 0,  0.005f, 0.006f, 0, 0 },
        {  0.18f, 0.180f, 0.100f, 0,  0.003f, 0.004f, 0, 0 },
        {  0.25f, 0.124f, 0.080f, 0,  0.000f, 0.000f, 0, 0 },
    };
    g_assets.torsoM = BuildLoftedMesh(torsoRingsM, 5, 22, true);

    // Tronco feminino (cintura natural e caimento reto/suave de blazer corporativo)
    const RingSpec torsoRingsF[] = {
        { -0.26f, 0.144f, 0.096f, 0, -0.004f, 0.002f, 0, 0 },
        { -0.10f, 0.130f, 0.088f, 0,  0.000f, 0.002f, 0, 0 },
        {  0.06f, 0.146f, 0.098f, 0,  0.004f, 0.004f, 0, 0 },
        {  0.17f, 0.158f, 0.092f, 0,  0.002f, 0.002f, 0, 0 },
        {  0.24f, 0.108f, 0.072f, 0,  0.000f, 0.000f, 0, 0 },
    };
    g_assets.torsoF = BuildLoftedMesh(torsoRingsF, 5, 22, true);

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

    // Barra do casaco / blazer curto na altura do quadril (Max Payne / Estagiaria)
    const RingSpec skirtRingsM[] = {
        { -0.14f, 0.168f, 0.114f, 0, -0.008f, 0.0f, 0.025f, 6.0f },
        { -0.07f, 0.158f, 0.108f, 0, -0.006f, 0.0f, 0.015f, 6.0f },
        {  0.00f, 0.148f, 0.100f, 0, -0.004f, 0.0f, 0.008f, 6.0f },
    };
    g_assets.coatSkirtM = BuildLoftedMesh(skirtRingsM, 3, 24, true);

    const RingSpec skirtRingsF[] = {
        { -0.11f, 0.156f, 0.108f, 0, -0.008f, 0.0f, 0.022f, 6.0f },
        { -0.05f, 0.140f, 0.098f, 0, -0.005f, 0.0f, 0.012f, 6.0f },
        {  0.00f, 0.124f, 0.088f, 0, -0.004f, 0.0f, 0.006f, 6.0f },
    };
    g_assets.coatSkirtF = BuildLoftedMesh(skirtRingsF, 3, 24, true);

    // Abas frontais abertas do casaco / blazer (altura do quadril)
    const RingSpec flapRingsM[] = {
        { -0.14f, 0.068f, 0.022f, 0, 0, 0, 0.02f, 4.0f },
        {  0.02f, 0.064f, 0.022f, 0, 0, 0, 0.01f, 4.0f },
        {  0.18f, 0.062f, 0.020f, 0, 0, 0, 0.00f, 0.0f },
    };
    g_assets.coatFlapM = BuildLoftedMesh(flapRingsM, 3, 14, true);

    const RingSpec flapRingsF[] = {
        { -0.11f, 0.058f, 0.018f, 0, 0, 0, 0.02f, 4.0f },
        {  0.03f, 0.050f, 0.018f, 0, 0, 0, 0.01f, 4.0f },
        {  0.17f, 0.054f, 0.018f, 0, 0, 0, 0.00f, 0.0f },
    };
    g_assets.coatFlapF = BuildLoftedMesh(flapRingsF, 3, 14, true);

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

    // Disco de sombra suave no piso
    const RingSpec shadowRings[] = {
        { 0.000f, 0.24f, 0.20f, 0, 0, 0, 0, 0 },
        { 0.004f, 0.23f, 0.19f, 0, 0, 0, 0, 0 },
    };
    g_assets.shadowDisc = BuildLoftedMesh(shadowRings, 2, 20, true);

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

    // Bracos anatomicos (deltoide -> cotovelo -> antebraco -> pulso fino)
    const RingSpec uArmRingsM[] = {
        { -0.27f, 0.044f, 0.044f, 0, 0, 0, 0, 0 },
        { -0.14f, 0.052f, 0.052f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.058f, 0.056f, 0, 0, 0, 0, 0 },
        {  0.03f, 0.042f, 0.042f, 0, 0, 0, 0, 0 },
    };
    g_assets.upperArmM = BuildLoftedMesh(uArmRingsM, 4, 14, true);

    const RingSpec uArmRingsF[] = {
        { -0.26f, 0.037f, 0.037f, 0, 0, 0, 0, 0 },
        { -0.13f, 0.043f, 0.043f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.048f, 0.046f, 0, 0, 0, 0, 0 },
        {  0.025f,0.035f, 0.035f, 0, 0, 0, 0, 0 },
    };
    g_assets.upperArmF = BuildLoftedMesh(uArmRingsF, 4, 14, true);

    const RingSpec fArmRingsM[] = {
        { -0.25f, 0.034f, 0.030f, 0, 0, 0, 0, 0 },
        { -0.14f, 0.042f, 0.040f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.044f, 0.044f, 0, 0, 0, 0, 0 },
    };
    g_assets.forearmM = BuildLoftedMesh(fArmRingsM, 3, 14, true);

    const RingSpec fArmRingsF[] = {
        { -0.24f, 0.028f, 0.024f, 0, 0, 0, 0, 0 },
        { -0.13f, 0.035f, 0.033f, 0, 0, 0, 0, 0 },
        {  0.00f, 0.037f, 0.037f, 0, 0, 0, 0, 0 },
    };
    g_assets.forearmF = BuildLoftedMesh(fArmRingsF, 3, 14, true);

    // Mao esculpida (palma + dedos afilados)
    const RingSpec handRings[] = {
        { -0.105f, 0.018f, 0.010f, 0, 0.006f, 0, 0, 0 },
        { -0.055f, 0.028f, 0.015f, 0, 0.004f, 0, 0, 0 },
        {  0.000f, 0.024f, 0.016f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.hand = BuildLoftedMesh(handRings, 3, 12, true);

    // Pernas anatomicas (coxa com curva de quadriceps -> joelho -> panturrilha -> tornozelo fino)
    const RingSpec thighRingsM[] = {
        { -0.42f, 0.062f, 0.064f, 0, 0.004f, 0, 0, 0 },
        { -0.22f, 0.078f, 0.082f, 0, 0.006f, 0, 0, 0 },
        {  0.00f, 0.088f, 0.090f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.thighM = BuildLoftedMesh(thighRingsM, 3, 16, true);

    const RingSpec thighRingsF[] = {
        { -0.41f, 0.054f, 0.056f, 0, 0.004f, 0, 0, 0 },
        { -0.21f, 0.068f, 0.072f, 0, 0.006f, 0, 0, 0 },
        {  0.00f, 0.076f, 0.078f, 0, 0.000f, 0, 0, 0 },
    };
    g_assets.thighF = BuildLoftedMesh(thighRingsF, 3, 16, true);

    const RingSpec calfRingsM[] = {
        { -0.40f, 0.044f, 0.046f, 0,  0.000f, 0, 0, 0 },
        { -0.20f, 0.060f, 0.064f, 0, -0.008f, 0, 0, 0 },
        {  0.00f, 0.060f, 0.062f, 0,  0.000f, 0, 0, 0 },
    };
    g_assets.calfM = BuildLoftedMesh(calfRingsM, 3, 16, true);

    const RingSpec calfRingsF[] = {
        { -0.39f, 0.038f, 0.040f, 0,  0.000f, 0, 0, 0 },
        { -0.19f, 0.052f, 0.056f, 0, -0.007f, 0, 0, 0 },
        {  0.00f, 0.052f, 0.054f, 0,  0.000f, 0, 0, 0 },
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
        &g_assets.belt, &g_assets.lanyard, &g_assets.badge, &g_assets.shadowDisc,
        &g_assets.upperArmM, &g_assets.upperArmF,
        &g_assets.forearmM, &g_assets.forearmF, &g_assets.hand,
        &g_assets.thighM, &g_assets.thighF, &g_assets.calfM, &g_assets.calfF, &g_assets.shoe
    };
    for (int i = 0; i < (int)(sizeof(meshes) / sizeof(meshes[0])); i++) {
        if (meshes[i]->vboId != NULL || meshes[i]->vaoId != 0) {
            UnloadMesh(*meshes[i]);
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

static Matrix MakeLocal(Vector3 pos, Vector3 rotDeg, Vector3 scale) {
    Matrix m = MatrixScale(scale.x, scale.y, scale.z);
    if (rotDeg.z != 0.0f) m = MatrixMultiply(m, MatrixRotateZ(rotDeg.z * DEG2RAD));
    if (rotDeg.x != 0.0f) m = MatrixMultiply(m, MatrixRotateX(rotDeg.x * DEG2RAD));
    if (rotDeg.y != 0.0f) m = MatrixMultiply(m, MatrixRotateY(rotDeg.y * DEG2RAD));
    m = MatrixMultiply(m, MatrixTranslate(pos.x, pos.y, pos.z));
    return m;
}

void PlayerCharacterDraw3D(const PlayerCharacter *p, Vector3 camPos) {
    if (!p) return;
    EnsurePlayerAssets();

    float viewArr[3] = { camPos.x, camPos.y, camPos.z };
    SetShaderValue(g_assets.shader, g_assets.locViewPos, viewArr, SHADER_UNIFORM_VEC3);

    bool fem = p->isFemale;
    float blend = p->moveBlend;
    float s = sinf(p->walkCycle);
    float c2 = cosf(2.0f * p->walkCycle);

    // Respiracao sutil em idle + deslize suave na caminhada (sem pulos de corrida)
    float idleBreath = sinf(p->idleTime * 1.9f) * 0.005f * (1.0f - blend);
    float walkBob    = (-c2 * 0.008f) * blend;
    float bobY       = walkBob + idleBreath;

    // Postura idle natural + contra-rotacao discreta na marcha
    float idleHipRoll = 1.6f * (1.0f - blend);
    float pelvisYaw   = s * 3.5f * blend + 2.0f * (1.0f - blend);
    float pelvisRoll  = s * 1.8f * blend + idleHipRoll;
    float torsoYaw    = -s * 4.0f * blend - 1.5f * (1.0f - blend);
    float forwardLean = 2.0f * blend + 1.0f * (1.0f - blend);

    float reach = 0.0f;
    if (p->interactTimer > 0.0f) {
        float t = p->interactTimer / 0.42f;
        reach = sinf(t * PI);
    }

    // Altura realista (masculino ~1.88m esguio, feminino ~1.80m)
    float rootScale = fem ? 0.97f : 1.02f;
    Matrix root = MakeLocal(p->position,
                            (Vector3){ 0.0f, p->facingYaw * RAD2DEG, 0.0f },
                            (Vector3){ rootScale, rootScale, rootScale });

    // Sombra dupla suave ancorada no piso (nucleo + penumbra)
    DrawPart(g_assets.shadowDisc,
             MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.006f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.28f, 1.0f, 1.28f }), root),
             0.11f, false);
    DrawPart(g_assets.shadowDisc,
             MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.010f, 0.0f }, (Vector3){ 0, 0, 0 }, (Vector3){ 0.84f, 1.0f, 0.84f }), root),
             0.03f, false);

    // Base do quadril
    Matrix pelvisM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.87f + bobY, 0.0f }, (Vector3){ 0.0f, pelvisYaw, pelvisRoll }, (Vector3){ 1, 1, 1 }),
        root
    );

    // 1) PERNAS ARTICULADAS COM PASSADA FLUIDA DE CAMINHADA (SEM CORRIDA)
    float hipX = fem ? 0.088f : 0.098f;
    for (int side = -1; side <= 1; side += 2) {
        float phase = (side < 0) ? p->walkCycle : (p->walkCycle + PI);
        float sp = sinf(phase);
        float cp = cosf(phase);
        float swingPhase = fmaxf(0.0f, cp);
        float legAng     = (-sp * 16.5f) * blend + ((side < 0) ? 2.0f : -2.0f) * (1.0f - blend);
        float kneeBend   = (swingPhase * 22.0f + 2.0f) * blend + ((side < 0) ? 2.5f : 4.5f) * (1.0f - blend);
        float footPitch  = (-legAng * 0.45f - cp * 5.0f) * blend;

        Matrix hipM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * hipX, 0.0f, 0.0f }, (Vector3){ legAng, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
            pelvisM
        );
        DrawPart(fem ? g_assets.thighF : g_assets.thighM, hipM, 0.16f, false);

        Matrix kneeM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.41f : -0.42f, 0.0f }, (Vector3){ kneeBend, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
            hipM
        );
        DrawPart(fem ? g_assets.calfF : g_assets.calfM, kneeM, 0.15f, false);

        Matrix ankleM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.39f : -0.40f, 0.0f }, (Vector3){ footPitch - kneeBend * 0.45f, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
            kneeM
        );
        DrawPart(g_assets.shoe, ankleM, 0.06f, false);
    }

    // 2) CAMADA DE TERNO / BLAZER SOBRE CAMISA BRANCA E GRAVATA
    Matrix torsoBase = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.25f, 0.0f }, (Vector3){ forwardLean, torsoYaw, -pelvisRoll * 0.65f }, (Vector3){ 1, 1, 1 }),
        pelvisM
    );
    // Camisa clara interna + gravata (menor na feminina, social padrao no masculino)
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

    // Camada externa do terno / blazer estruturado cobrindo laterais e costas
    DrawPart(fem ? g_assets.torsoF : g_assets.torsoM,
             MatrixMultiply(MakeLocal((Vector3){ 0.0f, 0.0f, -0.010f }, (Vector3){ 0, 0, 0 }, (Vector3){ 1.04f, 1.01f, 1.02f }), torsoBase),
             0.18f, false);

    // Barra do paleto / blazer na altura do quadril
    float coatSway = -sinf(p->walkCycle) * 2.0f * blend;
    Matrix skirtM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, -0.18f, -0.008f }, (Vector3){ coatSway, 0, 0 }, (Vector3){ 1.03f, 1.0f, 1.02f }),
        torsoBase
    );
    DrawPart(fem ? g_assets.coatSkirtF : g_assets.coatSkirtM, skirtM, 0.17f, false);

    for (int side = -1; side <= 1; side += 2) {
        float flapX = (float)side * (fem ? 0.082f : 0.094f);
        Matrix flapM = MatrixMultiply(
            MakeLocal((Vector3){ flapX, -0.18f, 0.068f },
                      (Vector3){ coatSway * 0.6f, (float)side * 8.0f, (float)side * (fem ? 2.0f : 1.5f) },
                      (Vector3){ 1.02f, 1.0f, 1.0f }),
            torsoBase
        );
        DrawPart(fem ? g_assets.coatFlapF : g_assets.coatFlapM, flapM, 0.21f, false);

        // Lapelas claras do paleto emoldurando a camisa e a gravata
        Matrix lapelM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * (fem ? 0.062f : 0.070f), 0.10f, 0.092f },
                      (Vector3){ 4.0f, (float)side * 12.0f, (float)side * -16.0f },
                      (Vector3){ 0.85f, 1.02f, 0.85f }),
            torsoBase
        );
        DrawPart(g_assets.lapel, lapelM, 0.30f, false);
    }

    Matrix collarM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.21f, -0.010f }, (Vector3){ -6.0f, 0, 0 }, (Vector3){ 1.02f, 1.0f, 1.02f }),
        torsoBase
    );
    DrawPart(g_assets.collarBack, collarM, 0.28f, false);

    // 3) BRACOS RELAXADOS DE CAMINHADA (SEM COTOVELO DE CORRIDA)
    float shXBase = fem ? 0.180f : 0.204f;
    for (int side = -1; side <= 1; side += 2) {
        float legPhase = (side < 0) ? p->walkCycle : (p->walkCycle + PI);
        float sp = sinf(legPhase);
        float armAng   = (sp * 11.5f) * blend + ((side > 0) ? -2.5f : 1.5f) * (1.0f - blend);
        float elbowAng = (-8.5f - fmaxf(0.0f, -sp) * 6.5f) * blend - (9.0f + sinf(p->idleTime * 1.9f) * 1.2f) * (1.0f - blend);
        float armOutZ  = (float)side * (fem ? 7.5f : 8.0f);

        if (side < 0 && reach > 0.01f) {
            armAng = -64.0f * reach;
            elbowAng = -16.0f * (1.0f - 0.4f * reach);
        }

        Matrix shM = MatrixMultiply(
            MakeLocal((Vector3){ (float)side * shXBase, 0.18f, 0.0f },
                      (Vector3){ armAng, 0.0f, armOutZ },
                      (Vector3){ 1, 1, 1 }),
            torsoBase
        );
        DrawPart(fem ? g_assets.upperArmF : g_assets.upperArmM, shM, fem ? 0.27f : 0.23f, false);

        Matrix elbM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.26f : -0.27f, 0.0f },
                      (Vector3){ elbowAng, 0.0f, (float)side * -2.5f },
                      (Vector3){ 1, 1, 1 }),
            shM
        );
        DrawPart(fem ? g_assets.forearmF : g_assets.forearmM, elbM, fem ? 0.26f : 0.22f, false);

        Matrix handM = MatrixMultiply(
            MakeLocal((Vector3){ 0.0f, fem ? -0.24f : -0.25f, 0.008f },
                      (Vector3){ -6.0f, 0.0f, (float)side * -4.0f },
                      (Vector3){ 1, 1, 1 }),
            elbM
        );
        DrawPart(g_assets.hand, handM, 0.62f, false);
    }

    // 4) PESCOCO, CABECA (COR NORMAL) E CABELO
    Matrix neckM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.22f, 0.0f }, (Vector3){ -forwardLean * 0.6f, -torsoYaw * 0.5f, 0 }, (Vector3){ 1, 1, 1 }),
        torsoBase
    );
    DrawPart(fem ? g_assets.neckF : g_assets.neckM, neckM, 0.58f, false);

    Matrix headM = MatrixMultiply(
        MakeLocal((Vector3){ 0.0f, 0.19f, 0.012f }, (Vector3){ 4.0f, 0.0f, 0.0f }, (Vector3){ 1, 1, 1 }),
        neckM
    );
    DrawPart(fem ? g_assets.headF : g_assets.headM, headM, 0.64f, false);

    if (fem) {
        DrawPart(g_assets.hairF, headM, 0.08f, false);
        DrawPart(g_assets.hairBackF, headM, 0.07f, false);
        for (int side = -1; side <= 1; side += 2) {
            Matrix sideHairM = MatrixMultiply(
                MakeLocal((Vector3){ (float)side * 0.068f, 0.01f, 0.005f },
                          (Vector3){ 0.0f, 0.0f, (float)side * -7.0f },
                          (Vector3){ 1, 1, 1 }),
                headM
            );
            DrawPart(g_assets.hairSideF, sideHairM, 0.08f, false);
        }
        Matrix bangL = MatrixMultiply(
            MakeLocal((Vector3){ -0.025f, 0.068f, 0.078f }, (Vector3){ 12.0f, 18.0f, 16.0f }, (Vector3){ 0.9f, 0.85f, 1.0f }),
            headM
        );
        Matrix bangR = MatrixMultiply(
            MakeLocal((Vector3){  0.025f, 0.068f, 0.078f }, (Vector3){ 12.0f, -18.0f, -16.0f }, (Vector3){ 0.9f, 0.85f, 1.0f }),
            headM
        );
        DrawPart(g_assets.bangM2, bangL, 0.08f, false);
        DrawPart(g_assets.bangM1, bangR, 0.08f, false);
    } else {
        DrawPart(g_assets.hairM, headM, 0.08f, false);
        Matrix bang1M = MatrixMultiply(
            MakeLocal((Vector3){ 0.012f, 0.070f, 0.082f }, (Vector3){ 14.0f, -10.0f, -12.0f }, (Vector3){ 1, 1, 1 }),
            headM
        );
        Matrix bang2M = MatrixMultiply(
            MakeLocal((Vector3){ -0.022f, 0.066f, 0.080f }, (Vector3){ 12.0f, 14.0f, 10.0f }, (Vector3){ 1, 1, 1 }),
            headM
        );
        DrawPart(g_assets.bangM1, bang1M, 0.08f, false);
        DrawPart(g_assets.bangM2, bang2M, 0.08f, false);
    }
}
