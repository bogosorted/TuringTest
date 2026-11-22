#include "evidencia_holograma.h"
#include "raymath.h"
#include <math.h>
#include <string.h>
#include <stddef.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Shader identico a linguagem visual do mapa holografico da RASEC (5 tons + Bayer 4x4 + scanlines + varredura)
static const char *EV_HOLO_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float tempo;\n"
    "uniform float altura;\n"
    "out vec4 finalColor;\n"
    "const vec3 TONS[5] = vec3[5](\n"
    "    vec3(0.0),\n"
    "    vec3(32.0, 29.0, 31.0) / 255.0,\n"
    "    vec3(125.0, 112.0, 113.0) / 255.0,\n"
    "    vec3(189.0, 181.0, 168.0) / 255.0,\n"
    "    vec3(255.0, 242.0, 230.0) / 255.0\n"
    ");\n"
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    if (c.a < 0.01) discard;\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    float y = gl_FragCoord.y;\n"
    "    l *= (mod(y, 2.0) < 1.0) ? 1.0 : 0.955;\n"
    "    float faixa = fract(tempo * 0.22) * (altura + 28.0) - 14.0;\n"
    "    l += 0.07 * exp(-pow((y - faixa) / 5.0, 2.0));\n"
    "    l *= 0.992 + 0.008 * sin(tempo * 34.0);\n"
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar * 0.90), 0.0, 4.0));\n"
    "    if (idx == 0) discard;\n"
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static RenderTexture2D g_rtCena  = { 0 };
static RenderTexture2D g_rtFinal = { 0 };
static bool            g_rtPronta = false;
static int             g_rtW = 0, g_rtH = 0;
static Shader          g_shader = { 0 };
static int             g_locTempo = -1;
static int             g_locAltura = -1;
static bool            g_iniciado = false;
static Vector3         g_camPosAtual = { 0.0f, 0.0f, 4.0f };

static Color CorLum(float l) {
    unsigned char v = (unsigned char)(Clamp(l, 0.0f, 1.0f) * 255.0f);
    return (Color){ v, v, v, 255 };
}

// Iluminacao holografica: luz principal + preenchimento + brilho Fresnel nas bordas em relacao a camera
static Color SombrearNormal(Vector3 normalMundo, Vector3 pontoMundo, float baseLum) {
    Vector3 n = Vector3Normalize(normalMundo);
    Vector3 keyLight  = Vector3Normalize((Vector3){ -0.48f,  0.76f,  0.62f });
    Vector3 fillLight = Vector3Normalize((Vector3){  0.60f, -0.25f, -0.45f });
    Vector3 viewDir   = Vector3Normalize(Vector3Subtract(g_camPosAtual, pontoMundo));

    float ndl = fmaxf(Vector3DotProduct(n, keyLight), 0.0f);
    float fill = fmaxf(Vector3DotProduct(n, fillLight), 0.0f);
    float ndv = fabsf(Vector3DotProduct(n, viewDir));
    float fresnel = powf(Clamp(1.0f - ndv, 0.0f, 1.0f), 2.2f);

    float lum = baseLum * (0.34f + 0.58f * ndl + 0.16f * fill) + 0.18f * fresnel;
    return CorLum(Clamp(lum, 0.16f, 0.98f));
}

// Desenha um triangulo garantindo winding voltado para a camera (nunca sofre backface culling)
static void Tri3D(Vector3 a, Vector3 b, Vector3 c, Vector3 normalHint, float baseLum) {
    Vector3 ab = Vector3Subtract(b, a);
    Vector3 ac = Vector3Subtract(c, a);
    Vector3 nGeom = Vector3CrossProduct(ab, ac);
    if (Vector3LengthSqr(nGeom) < 1e-10f) return;

    Vector3 centro = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
    Vector3 nShade = (Vector3LengthSqr(normalHint) > 1e-6f) ? normalHint : nGeom;
    if (Vector3DotProduct(nShade, Vector3Subtract(g_camPosAtual, centro)) < 0.0f) {
        nShade = Vector3Negate(nShade);
    }
    Color cor = SombrearNormal(nShade, centro, baseLum);

    Vector3 paraCam = Vector3Subtract(g_camPosAtual, a);
    if (Vector3DotProduct(nGeom, paraCam) >= 0.0f) {
        DrawTriangle3D(a, b, c, cor);
    } else {
        DrawTriangle3D(a, c, b, cor);
    }
}

static void Quad3D(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 normalHint, float baseLum) {
    Tri3D(a, b, c, normalHint, baseLum);
    Tri3D(a, c, d, normalHint, baseLum);
}

static Vector3 Xform(Vector3 p, Matrix m) {
    return Vector3Transform(p, m);
}

static Vector3 XformDir(Vector3 n, Matrix m) {
    return Vector3Normalize((Vector3){
        n.x * m.m0 + n.y * m.m4 + n.z * m.m8,
        n.x * m.m1 + n.y * m.m5 + n.z * m.m9,
        n.x * m.m2 + n.y * m.m6 + n.z * m.m10
    });
}

static void LinhaM(Vector3 a, Vector3 b, Matrix m, float lum) {
    DrawLine3D(Xform(a, m), Xform(b, m), CorLum(lum));
}

// Caixa orientada com faces sombreadas + arestas nitidas estilo holograma
static void CaixaM(Vector3 centro, Vector3 tam, float baseLum, float wireLum, Matrix m) {
    Vector3 h = Vector3Scale(tam, 0.5f);
    Vector3 v[8];
    for (int i = 0; i < 8; i++) {
        Vector3 loc = {
            centro.x + ((i & 1) ? h.x : -h.x),
            centro.y + ((i & 2) ? h.y : -h.y),
            centro.z + ((i & 4) ? h.z : -h.z)
        };
        v[i] = Xform(loc, m);
    }
    static const int F[6][4] = {
        { 1, 3, 7, 5 }, { 0, 4, 6, 2 },
        { 2, 6, 7, 3 }, { 0, 1, 5, 4 },
        { 4, 5, 7, 6 }, { 0, 2, 3, 1 }
    };
    static const Vector3 N[6] = {
        { 1, 0, 0 }, { -1, 0, 0 },
        { 0, 1, 0 }, { 0, -1, 0 },
        { 0, 0, 1 }, { 0, 0, -1 }
    };
    for (int f = 0; f < 6; f++) {
        Vector3 nw = XformDir(N[f], m);
        Quad3D(v[F[f][0]], v[F[f][1]], v[F[f][2]], v[F[f][3]], nw, baseLum);
    }
    if (wireLum > 0.01f) {
        static const int E[12][2] = {
            {0,1},{2,3},{4,5},{6,7},
            {0,2},{1,3},{4,6},{5,7},
            {0,4},{1,5},{2,6},{3,7}
        };
        Color cw = CorLum(wireLum);
        for (int e = 0; e < 12; e++) {
            DrawLine3D(v[E[e][0]], v[E[e][1]], cw);
        }
    }
}

// Prisma / tronco de cone facetado ao longo do eixo Y local (y0..y1, raios r0 e r1)
static void PrismaY(float y0, float y1, float r0, float r1, int lados,
                    float baseLum, float wireLum, bool tampas, Matrix m) {
    if (lados < 3) lados = 3;
    if (lados > 24) lados = 24;
    Vector3 p0[25], p1[25];
    for (int i = 0; i <= lados; i++) {
        float a = (float)i * (2.0f * PI / (float)lados);
        float ca = cosf(a), sa = sinf(a);
        p0[i] = Xform((Vector3){ ca * r0, y0, sa * r0 }, m);
        p1[i] = Xform((Vector3){ ca * r1, y1, sa * r1 }, m);
    }
    Vector3 c0 = Xform((Vector3){ 0.0f, y0, 0.0f }, m);
    Vector3 c1 = Xform((Vector3){ 0.0f, y1, 0.0f }, m);
    Vector3 nBot = XformDir((Vector3){ 0.0f, -1.0f, 0.0f }, m);
    Vector3 nTop = XformDir((Vector3){ 0.0f,  1.0f, 0.0f }, m);

    for (int i = 0; i < lados; i++) {
        float am = ((float)i + 0.5f) * (2.0f * PI / (float)lados);
        Vector3 nLat = XformDir((Vector3){ cosf(am), (r0 - r1) * 0.35f, sinf(am) }, m);
        if (r1 > 1e-4f && r0 > 1e-4f) {
            Quad3D(p0[i], p0[i + 1], p1[i + 1], p1[i], nLat, baseLum);
        } else if (r1 <= 1e-4f) {
            Tri3D(p0[i], p0[i + 1], c1, nLat, baseLum);
        } else {
            Tri3D(c0, p1[i + 1], p1[i], nLat, baseLum);
        }

        if (tampas) {
            if (r0 > 1e-4f) Tri3D(c0, p0[i + 1], p0[i], nBot, baseLum * 0.9f);
            if (r1 > 1e-4f) Tri3D(c1, p1[i], p1[i + 1], nTop, baseLum * 1.05f);
        }
        if (wireLum > 0.01f) {
            Color cw = CorLum(wireLum);
            if (r0 > 1e-4f) DrawLine3D(p0[i], p0[i + 1], cw);
            if (r1 > 1e-4f) DrawLine3D(p1[i], p1[i + 1], cw);
            if (i % 2 == 0) DrawLine3D(p0[i], p1[i], CorLum(wireLum * 0.80f));
        }
    }
}

// Base projetora holografica circular flutuando sob o artefato
static void DesenharRetoricaBaseHolo(float yBase, float raio, float tempo, float opacidade) {
    if (opacidade <= 0.05f) return;
    int segs = 36;
    float lumAnel = 0.50f * opacidade;
    float lumTick = 0.72f * opacidade;
    for (int i = 0; i < segs; i++) {
        float a0 = (float)i * (2.0f * PI / (float)segs) + tempo * 0.25f;
        float a1 = ((float)i + 0.65f) * (2.0f * PI / (float)segs) + tempo * 0.25f;
        DrawLine3D((Vector3){ cosf(a0) * raio, yBase, sinf(a0) * raio },
                   (Vector3){ cosf(a1) * raio, yBase, sinf(a1) * raio }, CorLum(lumAnel));
        if (i % 3 == 0) {
            float ri = raio * 0.82f;
            DrawLine3D((Vector3){ cosf(a0) * ri, yBase, sinf(a0) * ri },
                       (Vector3){ cosf(a0) * (raio * 0.94f), yBase, sinf(a0) * (raio * 0.94f) }, CorLum(lumTick));
        }
    }
    float rIn = raio * 0.42f;
    for (int i = 0; i < 24; i += 2) {
        float a0 = (float)i * (2.0f * PI / 24.0f) - tempo * 0.4f;
        float a1 = (float)(i + 1) * (2.0f * PI / 24.0f) - tempo * 0.4f;
        DrawLine3D((Vector3){ cosf(a0) * rIn, yBase, sinf(a0) * rIn },
                   (Vector3){ cosf(a1) * rIn, yBase, sinf(a1) * rIn }, CorLum(0.42f * opacidade));
    }
}

// -----------------------------------------------------------------------------
// 1) HOLO_EV_CANETA: Caneta esferografica de clique + clipe + emblema de ABUTRE/AVE
// -----------------------------------------------------------------------------
static void DesenharModeloCaneta(Matrix mObj) {
    // Leve inclinacao natural da caneta para leitura elegante no angulo inicial
    Matrix m = MatrixMultiply(MatrixRotateZ(-0.34f), mObj);

    // Corpo principal sextavado/oitavado (estilo caneta executiva/madeira-preta da foto MesaCaneta)
    PrismaY(-1.05f, 1.02f, 0.135f, 0.135f, 8, 0.44f, 0.82f, true, m);

    // Aneis de empunhadura (grip) na parte inferior do corpo
    for (int g = 0; g < 3; g++) {
        float yg = -0.92f + g * 0.11f;
        PrismaY(yg, yg + 0.045f, 0.143f, 0.143f, 8, 0.62f, 0.90f, true, m);
    }

    // Ponteira conica metalica + esfera da ponta esferografica
    PrismaY(-1.46f, -1.05f, 0.024f, 0.132f, 8, 0.78f, 0.96f, true, m);
    PrismaY(-1.55f, -1.46f, 0.008f, 0.024f, 6, 0.95f, 1.00f, true, m);

    // Anel superior metalico + botao de clique (plunger) no topo
    PrismaY(1.02f, 1.11f, 0.142f, 0.142f, 8, 0.80f, 0.96f, true, m);
    PrismaY(1.11f, 1.38f, 0.082f, 0.082f, 8, 0.74f, 0.95f, true, m);
    PrismaY(1.38f, 1.42f, 0.072f, 0.055f, 8, 0.90f, 1.00f, true, m);

    // Clipe lateral metalico (igual a referencia MesaCaneta.jpg)
    CaixaM((Vector3){ 0.165f, 1.05f, 0.0f }, (Vector3){ 0.07f, 0.06f, 0.055f }, 0.82f, 0.98f, m);
    CaixaM((Vector3){ 0.195f, 0.66f, 0.0f }, (Vector3){ 0.026f, 0.82f, 0.055f }, 0.84f, 0.98f, m);
    CaixaM((Vector3){ 0.175f, 0.27f, 0.0f }, (Vector3){ 0.045f, 0.06f, 0.055f }, 0.88f, 1.00f, m);

    // EMBLEMA DE AVE / ABUTRE gravado em alto-relevo luminoso na face frontal do corpo (z = +0.142)
    // Quando o jogador gira e da zoom (estilo RE4), descobre nitidamente a silhueta da ave!
    {
        float z = 0.144f;
        // Plaqueta sutil escura sob a gravacao para dar contraste maximo ao abutre
        CaixaM((Vector3){ 0.0f, 0.16f, 0.134f }, (Vector3){ 0.19f, 0.66f, 0.012f }, 0.26f, 0.58f, m);

        // Corpo + peito da ave em relevo 3D
        Vector3 nFrente = XformDir((Vector3){ 0, 0, 1 }, m);
        Tri3D(Xform((Vector3){  0.00f, 0.31f, z }, m),
              Xform((Vector3){ -0.032f, 0.14f, z }, m),
              Xform((Vector3){  0.032f, 0.14f, z }, m), nFrente, 0.95f);
        Tri3D(Xform((Vector3){ -0.032f, 0.14f, z }, m),
              Xform((Vector3){  0.00f, -0.02f, z }, m),
              Xform((Vector3){  0.032f, 0.14f, z }, m), nFrente, 0.92f);

        // Asa esquerda aberta + penas primarias (abutre)
        Tri3D(Xform((Vector3){ -0.015f, 0.25f, z }, m),
              Xform((Vector3){ -0.095f, 0.37f, z }, m),
              Xform((Vector3){ -0.078f, 0.19f, z }, m), nFrente, 0.96f);
        LinhaM((Vector3){ -0.095f, 0.37f, z + 0.003f }, (Vector3){ -0.105f, 0.43f, z + 0.003f }, m, 1.0f);
        LinhaM((Vector3){ -0.088f, 0.31f, z + 0.003f }, (Vector3){ -0.104f, 0.35f, z + 0.003f }, m, 1.0f);
        LinhaM((Vector3){ -0.082f, 0.25f, z + 0.003f }, (Vector3){ -0.098f, 0.27f, z + 0.003f }, m, 1.0f);

        // Asa direita aberta + penas primarias
        Tri3D(Xform((Vector3){  0.015f, 0.25f, z }, m),
              Xform((Vector3){  0.078f, 0.19f, z }, m),
              Xform((Vector3){  0.095f, 0.37f, z }, m), nFrente, 0.96f);
        LinhaM((Vector3){  0.095f, 0.37f, z + 0.003f }, (Vector3){  0.105f, 0.43f, z + 0.003f }, m, 1.0f);
        LinhaM((Vector3){  0.088f, 0.31f, z + 0.003f }, (Vector3){  0.104f, 0.35f, z + 0.003f }, m, 1.0f);
        LinhaM((Vector3){  0.082f, 0.25f, z + 0.003f }, (Vector3){  0.098f, 0.27f, z + 0.003f }, m, 1.0f);

        // Pescoco curvado + cabeca + bico adunco caracteristico de abutre
        LinhaM((Vector3){ 0.00f, 0.31f, z + 0.003f }, (Vector3){ -0.016f, 0.38f, z + 0.003f }, m, 1.0f);
        Tri3D(Xform((Vector3){ -0.022f, 0.38f, z + 0.002f }, m),
              Xform((Vector3){  0.018f, 0.42f, z + 0.002f }, m),
              Xform((Vector3){  0.012f, 0.37f, z + 0.002f }, m), nFrente, 1.0f);
        // Bico curvo para baixo
        LinhaM((Vector3){ 0.018f, 0.41f, z + 0.004f }, (Vector3){ 0.048f, 0.39f, z + 0.004f }, m, 1.0f);
        LinhaM((Vector3){ 0.048f, 0.39f, z + 0.004f }, (Vector3){ 0.040f, 0.365f, z + 0.004f }, m, 1.0f);

        // Cauda em leque + poleiro inferior
        Tri3D(Xform((Vector3){  0.00f, -0.01f, z }, m),
              Xform((Vector3){ -0.036f, -0.12f, z }, m),
              Xform((Vector3){  0.036f, -0.12f, z }, m), nFrente, 0.94f);
        LinhaM((Vector3){ -0.055f, -0.03f, z + 0.003f }, (Vector3){ 0.055f, -0.03f, z + 0.003f }, m, 0.95f);
    }
}

// -----------------------------------------------------------------------------
// 2) HOLO_EV_CRACHA: Cracha de funcionario RASEC + mosquetao + cordao + verso
// -----------------------------------------------------------------------------
static void DesenharModeloCracha(Matrix m) {
    // Porta-cracha rigido com abertura superior vazada (slot punch real em 4 barras)
    // Parte principal abaixo do furo
    CaixaM((Vector3){  0.00f, -0.32f, 0.0f }, (Vector3){ 1.46f, 1.88f, 0.075f }, 0.38f, 0.84f, m);
    // Ombreiras laterais ao lado do furo + barra do topo acima do furo
    CaixaM((Vector3){ -0.50f,  0.71f, 0.0f }, (Vector3){ 0.46f, 0.18f, 0.075f }, 0.42f, 0.84f, m);
    CaixaM((Vector3){  0.50f,  0.71f, 0.0f }, (Vector3){ 0.46f, 0.18f, 0.075f }, 0.42f, 0.84f, m);
    CaixaM((Vector3){  0.00f,  0.84f, 0.0f }, (Vector3){ 1.46f, 0.08f, 0.075f }, 0.46f, 0.88f, m);

    // Cartao interno inserido (frente z = +0.042)
    CaixaM((Vector3){ 0.00f, -0.34f, 0.005f }, (Vector3){ 1.30f, 1.72f, 0.072f }, 0.56f, 0.72f, m);

    // --- FRENTE DO CRACHA (z > 0): faixa "ACESSO FUNCIONARIO" + foto 3D em relevo + texto + codigo de barras ---
    CaixaM((Vector3){ 0.00f,  0.30f, 0.046f }, (Vector3){ 1.22f, 0.34f, 0.018f }, 0.30f, 0.92f, m);
    // Duas linhas tipograficas na faixa vermelha superior ("ACESSO" / "FUNCIONARIO")
    CaixaM((Vector3){ 0.00f,  0.36f, 0.058f }, (Vector3){ 0.62f, 0.045f, 0.010f }, 0.94f, 1.00f, m);
    CaixaM((Vector3){ 0.00f,  0.24f, 0.058f }, (Vector3){ 0.94f, 0.045f, 0.010f }, 0.94f, 1.00f, m);

    // Moldura da foto 3x4 + busto 3D esculpido em relevo
    CaixaM((Vector3){ 0.00f, -0.14f, 0.045f }, (Vector3){ 0.54f, 0.58f, 0.014f }, 0.30f, 0.88f, m);
    // Cabeca e ombros do funcionario no retrato
    PrismaY(-0.12f, 0.03f, 0.105f, 0.095f, 8, 0.86f, 0.98f, true,
            MatrixMultiply(MatrixMultiply(MatrixRotateX(PI * 0.5f), MatrixTranslate(0.0f, -0.05f, 0.09f)), m));
    CaixaM((Vector3){ 0.00f, -0.06f, 0.058f }, (Vector3){ 0.17f, 0.21f, 0.022f }, 0.84f, 0.96f, m);
    CaixaM((Vector3){ 0.00f, -0.20f, 0.056f }, (Vector3){ 0.09f, 0.08f, 0.018f }, 0.78f, 0.92f, m);
    CaixaM((Vector3){ 0.00f, -0.32f, 0.058f }, (Vector3){ 0.42f, 0.17f, 0.020f }, 0.82f, 0.96f, m);

    // Linhas de identificacao ("Texto Identificado" / dados do titular)
    CaixaM((Vector3){ 0.00f, -0.54f, 0.048f }, (Vector3){ 0.76f, 0.042f, 0.012f }, 0.88f, 0.96f, m);
    CaixaM((Vector3){ 0.00f, -0.64f, 0.047f }, (Vector3){ 0.58f, 0.028f, 0.010f }, 0.72f, 0.85f, m);
    CaixaM((Vector3){ 0.00f, -0.72f, 0.047f }, (Vector3){ 0.64f, 0.028f, 0.010f }, 0.72f, 0.85f, m);

    // Codigo de barras 3D na base frontal (13 barras verticais de larguras variadas)
    {
        static const float largs[13] = { 0.035f, 0.018f, 0.042f, 0.018f, 0.025f, 0.040f, 0.018f, 0.032f, 0.018f, 0.042f, 0.020f, 0.028f, 0.035f };
        float bx = -0.46f;
        for (int b = 0; b < 13; b++) {
            float w = largs[b];
            CaixaM((Vector3){ bx + w * 0.5f, -0.96f, 0.050f }, (Vector3){ w, 0.24f, 0.014f }, 0.92f, 1.00f, m);
            bx += w + 0.043f;
        }
    }

    // --- VERSO DO CRACHA (z < 0, visivel ao virar em 180 graus!) ---
    // Tarja magnetica superior + chip RFID + cristais de gelo no canto (esquecido na geladeira!)
    CaixaM((Vector3){  0.00f,  0.28f, -0.046f }, (Vector3){ 1.26f, 0.24f, 0.014f }, 0.24f, 0.86f, m);
    CaixaM((Vector3){ -0.34f, -0.15f, -0.048f }, (Vector3){ 0.30f, 0.24f, 0.016f }, 0.84f, 0.98f, m);
    LinhaM((Vector3){ -0.49f, -0.15f, -0.058f }, (Vector3){ -0.19f, -0.15f, -0.058f }, m, 0.98f);
    LinhaM((Vector3){ -0.34f, -0.27f, -0.058f }, (Vector3){ -0.34f, -0.03f, -0.058f }, m, 0.98f);
    // Cristais geometricos de geada/frio no canto inferior do verso (pista da geladeira!)
    for (int k = 0; k < 3; k++) {
        float a = (float)k * (PI / 3.0f);
        float cx = 0.36f, cy = -0.78f, r = 0.16f;
        LinhaM((Vector3){ cx - cosf(a) * r, cy - sinf(a) * r, -0.052f },
               (Vector3){ cx + cosf(a) * r, cy + sinf(a) * r, -0.052f }, m, 0.96f);
    }

    // --- MOSQUETAO METALICO + CORDAO (LANYARD) 3D ACIMA DO CRACHA ---
    // Argola passando pelo furo do porta-cracha + pino giratorio + gancho do mosquetao
    CaixaM((Vector3){ 0.00f, 0.84f, 0.0f }, (Vector3){ 0.22f, 0.18f, 0.12f }, 0.82f, 0.98f, m);
    PrismaY(0.92f, 1.18f, 0.065f, 0.055f, 6, 0.86f, 1.00f, true, m);
    CaixaM((Vector3){ 0.00f, 1.24f, 0.0f }, (Vector3){ 0.26f, 0.08f, 0.06f }, 0.84f, 0.98f, m);

    // Fita de pescoco (lanyard) subindo em laco 3D e descendo por tras do cracha (igual a foto!)
    {
        const int NSEG = 14;
        for (int s = 0; s < NSEG; s++) {
            float t0 = (float)s / (float)NSEG;
            float t1 = (float)(s + 1) / (float)NSEG;
            float ang0 = t0 * PI;
            float ang1 = t1 * PI;
            Vector3 c0 = { -0.10f + 0.30f * sinf(ang0 * 1.15f), 1.26f + 0.56f * sinf(ang0), -0.05f - 0.14f * sinf(ang0) };
            Vector3 c1 = { -0.10f + 0.30f * sinf(ang1 * 1.15f), 1.26f + 0.56f * sinf(ang1), -0.05f - 0.14f * sinf(ang1) };
            Vector3 w0 = { 0.0f, 0.0f, 0.065f };
            Quad3D(Xform(Vector3Subtract(c0, w0), m), Xform(Vector3Add(c0, w0), m),
                   Xform(Vector3Add(c1, w0), m),      Xform(Vector3Subtract(c1, w0), m),
                   (Vector3){ 0, 0, 0 }, 0.46f);
            LinhaM(Vector3Subtract(c0, w0), Vector3Subtract(c1, w0), m, 0.88f);
            LinhaM(Vector3Add(c0, w0),      Vector3Add(c1, w0),      m, 0.88f);
        }
        // Aba da fita descendo levemente por tras do lado direito (fiel a CrachaFuncionario.png)
        CaixaM((Vector3){ 0.24f, 0.88f, -0.075f }, (Vector3){ 0.17f, 0.82f, 0.022f }, 0.40f, 0.80f, m);
    }
}

// -----------------------------------------------------------------------------
// 3) HOLO_EV_RELATORIOS: Pilha organica de pastas e relatorios a fazer
// -----------------------------------------------------------------------------
static void DesenharModeloRelatorios(Matrix mObj) {
    static const struct { float dx, dz, yaw, w, d, tabPos; } pastas[6] = {
        {  0.00f,  0.02f, -0.07f, 2.18f, 1.62f, -0.45f },
        { -0.05f, -0.03f,  0.05f, 2.14f, 1.58f,  0.12f },
        {  0.06f,  0.04f, -0.04f, 2.16f, 1.60f,  0.48f },
        { -0.03f,  0.01f,  0.09f, 2.12f, 1.56f, -0.22f },
        {  0.04f, -0.02f, -0.06f, 2.15f, 1.58f,  0.34f },
        { -0.02f,  0.00f,  0.03f, 2.14f, 1.58f, -0.08f }
    };

    float yCursor = -0.58f;
    for (int i = 0; i < 6; i++) {
        float espMiolo = 0.115f;
        float espCapa  = 0.022f;
        Matrix mp = MatrixMultiply(
            MatrixMultiply(MatrixRotateY(pastas[i].yaw),
                           MatrixTranslate(pastas[i].dx, yCursor, pastas[i].dz)),
            mObj);

        // Capa inferior da pasta
        CaixaM((Vector3){ 0.0f, espCapa * 0.5f, 0.0f },
               (Vector3){ pastas[i].w, espCapa, pastas[i].d }, 0.50f, 0.82f, mp);
        // Miolo grosso de folhas brancas empilhadas
        CaixaM((Vector3){ 0.02f, espCapa + espMiolo * 0.5f, 0.01f },
               (Vector3){ pastas[i].w - 0.08f, espMiolo, pastas[i].d - 0.07f }, 0.78f, 0.92f, mp);
        // Estrias de folhas nas laterais
        float yMeio = espCapa + espMiolo * 0.5f;
        LinhaM((Vector3){ -(pastas[i].w * 0.5f - 0.04f), yMeio,  (pastas[i].d * 0.5f - 0.03f) },
               (Vector3){  (pastas[i].w * 0.5f - 0.04f), yMeio,  (pastas[i].d * 0.5f - 0.03f) }, mp, 0.96f);
        // Capa superior da pasta + aba lateral identificadora (index tab)
        CaixaM((Vector3){ 0.0f, espCapa + espMiolo + espCapa * 0.5f, 0.0f },
               (Vector3){ pastas[i].w, espCapa, pastas[i].d }, 0.56f, 0.86f, mp);
        CaixaM((Vector3){ pastas[i].w * 0.5f + 0.05f, espCapa + espMiolo + espCapa * 0.5f, pastas[i].tabPos },
               (Vector3){ 0.12f, espCapa, 0.34f }, 0.68f, 0.94f, mp);

        // Folha avulsa saindo em angulo no meio da pilha (igual a foto PilhaRelatorios.jpg)
        if (i == 2) {
            Matrix mf = MatrixMultiply(MatrixRotateY(0.22f), mp);
            CaixaM((Vector3){ 0.14f, espCapa + espMiolo * 0.8f, 0.10f },
                   (Vector3){ 1.90f, 0.015f, 1.38f }, 0.88f, 0.98f, mf);
        }

        yCursor += espCapa * 2.0f + espMiolo + 0.014f;
    }

    // --- TOPO DA PILHA: Etiqueta + Folha de Relatorio presa com CLIPE DE PAPEL ---
    // Mostra linhas de dados alinhadas ("nao estao incorretos") + campo de assinatura vazio ("a fazer")!
    {
        Matrix mt = MatrixMultiply(
            MatrixMultiply(MatrixRotateY(0.03f), MatrixTranslate(-0.02f, yCursor + 0.012f, 0.0f)),
            mObj);
        // Folha de relatorio no topo
        CaixaM((Vector3){ 0.12f, 0.012f, 0.02f }, (Vector3){ 1.62f, 0.020f, 1.34f }, 0.82f, 0.96f, mt);

        // Cabecalho + tabela alinhada com dados corretos na folha superior
        CaixaM((Vector3){ -0.22f, 0.026f, -0.46f }, (Vector3){ 0.72f, 0.008f, 0.10f }, 0.34f, 0.92f, mt);
        for (int r = 0; r < 5; r++) {
            float zr = -0.24f + r * 0.14f;
            LinhaM((Vector3){ -0.56f, 0.026f, zr }, (Vector3){ 0.52f, 0.026f, zr }, mt, 0.42f);
            // Pequeno "check" [v] ao lado das linhas indicando que os calculos estao certos
            LinhaM((Vector3){ 0.58f, 0.027f, zr },        (Vector3){ 0.62f, 0.027f, zr + 0.04f }, mt, 0.98f);
            LinhaM((Vector3){ 0.62f, 0.027f, zr + 0.04f }, (Vector3){ 0.69f, 0.027f, zr - 0.04f }, mt, 0.98f);
        }
        // Caixa retangular de assinatura/aprovacao VAZIA no rodape ("relatorios para fazer")
        CaixaM((Vector3){ 0.38f, 0.025f, 0.48f }, (Vector3){ 0.62f, 0.006f, 0.20f }, 0.32f, 0.96f, mt);

        // Clipe de papel metalico 3D prendendo o canto superior esquerdo
        float cx = -0.56f, cz = -0.56f, cy = 0.032f;
        LinhaM((Vector3){ cx, cy, cz - 0.10f },        (Vector3){ cx, cy, cz + 0.18f }, mt, 1.0f);
        LinhaM((Vector3){ cx, cy, cz + 0.18f },        (Vector3){ cx + 0.09f, cy, cz + 0.18f }, mt, 1.0f);
        LinhaM((Vector3){ cx + 0.09f, cy, cz + 0.18f }, (Vector3){ cx + 0.09f, cy, cz - 0.06f }, mt, 1.0f);
        LinhaM((Vector3){ cx + 0.09f, cy, cz - 0.06f }, (Vector3){ cx + 0.03f, cy, cz - 0.06f }, mt, 1.0f);
        LinhaM((Vector3){ cx + 0.03f, cy, cz - 0.06f }, (Vector3){ cx + 0.03f, cy, cz + 0.12f }, mt, 1.0f);
    }
}

// -----------------------------------------------------------------------------
// Modelos 3D contextuais para as pistas da RASEC
// -----------------------------------------------------------------------------
static void DesenharModeloMemorando(Matrix m, bool comErro) {
    if (!comErro) {
        // EV-01: Denuncia descartada ao RH (Maria) — Folha de papel com marcas de dobras de ter sido amassada/descartada
        CaixaM((Vector3){ 0.0f,  0.00f,  0.00f }, (Vector3){ 1.56f, 2.12f, 0.036f }, 0.80f, 0.96f, m);
        // Abas e facetas levemente inclinadas simulando papel desamassado
        Matrix mTop = MatrixMultiply(MatrixRotateX(0.11f), m);
        CaixaM((Vector3){ 0.0f,  0.66f,  0.03f }, (Vector3){ 1.54f, 0.72f, 0.024f }, 0.84f, 0.96f, mTop);
        Matrix mBot = MatrixMultiply(MatrixRotateX(-0.09f), m);
        CaixaM((Vector3){ 0.0f, -0.66f, -0.02f }, (Vector3){ 1.54f, 0.72f, 0.024f }, 0.76f, 0.94f, mBot);

        // Linhas de vinco diagonal (papel amassado e reaberto)
        LinhaM((Vector3){ -0.76f,  0.34f, 0.028f }, (Vector3){  0.76f,  0.12f, 0.028f }, m, 0.52f);
        LinhaM((Vector3){ -0.76f, -0.22f, 0.028f }, (Vector3){  0.76f, -0.42f, 0.028f }, m, 0.52f);
        LinhaM((Vector3){ -0.32f,  1.02f, 0.028f }, (Vector3){  0.24f, -1.02f, 0.028f }, m, 0.48f);

        // Cabecalho "DENUNCIA AO RH" + linhas do texto da queixa + assinatura Maria Bianca
        CaixaM((Vector3){ -0.18f,  0.78f, 0.035f }, (Vector3){ 0.96f, 0.11f, 0.012f }, 0.32f, 0.94f, m);
        for (int l = 0; l < 6; l++) {
            float yl = 0.44f - l * 0.21f;
            float lw = (l == 5) ? 0.10f : 0.54f;
            LinhaM((Vector3){ -0.56f, yl, 0.032f }, (Vector3){ lw, yl, 0.032f }, m, 0.36f);
        }
        // Carimbo / selo "DESCARTADO" inclinado no canto inferior
        Matrix mCar = MatrixMultiply(MatrixRotateZ(0.18f), m);
        CaixaM((Vector3){ 0.24f, -0.72f, 0.036f }, (Vector3){ 0.78f, 0.22f, 0.014f }, 0.34f, 0.98f, mCar);
        return;
    }

    // EV-05 / EV-02: Terminal Corporativo / Relatorio com Alerta Critico [!]
    // Monitor widescreen + base de mesa + janela holografica de logs suspeitos
    CaixaM((Vector3){ 0.0f,  0.18f, -0.06f }, (Vector3){ 2.16f, 1.38f, 0.12f }, 0.42f, 0.88f, m);
    CaixaM((Vector3){ 0.0f,  0.18f,  0.01f }, (Vector3){ 1.98f, 1.20f, 0.03f }, 0.26f, 0.94f, m);
    // Pedestal e base do monitor
    CaixaM((Vector3){ 0.0f, -0.64f, -0.08f }, (Vector3){ 0.22f, 0.36f, 0.10f }, 0.48f, 0.86f, m);
    CaixaM((Vector3){ 0.0f, -0.82f,  0.00f }, (Vector3){ 0.92f, 0.07f, 0.52f }, 0.52f, 0.90f, m);

    // Barra de titulo do terminal (a.lucia - 03:08h) + linhas de log na tela
    CaixaM((Vector3){ 0.0f,  0.68f,  0.035f }, (Vector3){ 1.88f, 0.12f, 0.015f }, 0.64f, 0.96f, m);
    for (int l = 0; l < 4; l++) {
        float yl = 0.42f - l * 0.22f;
        float xEnd = (l == 2) ? 0.22f : 0.78f;
        LinhaM((Vector3){ -0.84f, yl, 0.038f }, (Vector3){ xEnd, yl, 0.038f }, m, 0.92f);
    }

    // Janela pop-up 3D projetada a frente da tela com alerta [!] de Acesso Negado / Historico Suspeito
    CaixaM((Vector3){ 0.42f,  0.08f,  0.22f }, (Vector3){ 0.88f, 0.76f, 0.04f }, 0.36f, 0.98f, m);
    CaixaM((Vector3){ 0.42f,  0.18f,  0.26f }, (Vector3){ 0.11f, 0.32f, 0.06f }, 0.96f, 1.00f, m);
    CaixaM((Vector3){ 0.42f, -0.10f,  0.26f }, (Vector3){ 0.11f, 0.10f, 0.06f }, 0.96f, 1.00f, m);
}

static void DesenharModeloRelogioTempo(Matrix m) {
    // Relogio/calendario de mesa (contradicao de 3, 4 e 5 anos de experiencia)
    PrismaY(-0.14f, 0.14f, 1.02f, 1.02f, 12, 0.48f, 0.90f, true, MatrixMultiply(MatrixRotateX(PI * 0.5f), m));
    PrismaY(-0.16f, 0.16f, 0.86f, 0.86f, 12, 0.72f, 0.94f, true, MatrixMultiply(MatrixRotateX(PI * 0.5f), m));
    // Ponteiros em conflito + base de apoio
    CaixaM((Vector3){ 0.0f,  0.26f, 0.18f }, (Vector3){ 0.07f, 0.52f, 0.04f }, 0.96f, 1.00f, m);
    CaixaM((Vector3){ 0.24f, 0.00f, 0.18f }, (Vector3){ 0.48f, 0.06f, 0.04f }, 0.94f, 1.00f, m);
    LinhaM((Vector3){ 0.0f, 0.0f, 0.19f }, (Vector3){ -0.42f, -0.36f, 0.19f }, m, 1.00f);
    CaixaM((Vector3){ 0.0f, -1.06f, 0.0f }, (Vector3){ 1.24f, 0.16f, 0.44f }, 0.56f, 0.90f, m);
}

static void DesenharModeloLeitorSetor(Matrix m) {
    // Terminal de controle de acesso entre setores (teclado 3x3 + fenda de cracha)
    CaixaM((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.44f, 2.10f, 0.26f }, 0.44f, 0.88f, m);
    CaixaM((Vector3){ 0.38f, 0.0f, 0.12f }, (Vector3){ 0.24f, 1.88f, 0.16f }, 0.34f, 0.92f, m);
    CaixaM((Vector3){ -0.22f, 0.62f, 0.15f }, (Vector3){ 0.74f, 0.36f, 0.04f }, 0.82f, 0.98f, m);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            CaixaM((Vector3){ -0.46f + c * 0.24f, 0.12f - r * 0.26f, 0.16f },
                   (Vector3){ 0.16f, 0.16f, 0.05f }, 0.78f, 0.96f, m);
        }
    }
}

static void DesenharModeloGravador(Matrix m) {
    // Gravador de voz portatil (depoimento confidencial)
    CaixaM((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.32f, 2.12f, 0.30f }, 0.46f, 0.88f, m);
    CaixaM((Vector3){ 0.0f, 0.42f, 0.16f }, (Vector3){ 1.02f, 0.76f, 0.04f }, 0.32f, 0.92f, m);
    for (int r = -1; r <= 1; r += 2) {
        PrismaY(-0.02f, 0.05f, 0.18f, 0.18f, 8, 0.88f, 0.98f, true,
                MatrixMultiply(MatrixMultiply(MatrixRotateX(PI * 0.5f), MatrixTranslate(r * 0.26f, 0.42f, 0.16f)), m));
    }
    for (int g = 0; g < 5; g++) {
        LinhaM((Vector3){ -0.42f, -0.22f - g * 0.12f, 0.165f },
               (Vector3){  0.42f, -0.22f - g * 0.12f, 0.165f }, m, 0.86f);
    }
    CaixaM((Vector3){ 0.70f, 0.52f, 0.0f }, (Vector3){ 0.10f, 0.24f, 0.12f }, 0.92f, 1.00f, m);
}

void EvidenciaHoloInit(void) {
    EvidenciaHoloUnload();
    g_shader    = LoadShaderFromMemory(NULL, EV_HOLO_FS);
    g_locTempo  = GetShaderLocation(g_shader, "tempo");
    g_locAltura = GetShaderLocation(g_shader, "altura");
    g_rtPronta  = false;
    g_rtW = g_rtH = 0;
    g_iniciado  = true;
}

void EvidenciaHoloUnload(void) {
    if (!g_iniciado) return;
    if (g_rtPronta) {
        UnloadRenderTexture(g_rtCena);
        UnloadRenderTexture(g_rtFinal);
        g_rtPronta = false;
    }
    if (g_shader.id > 0) {
        UnloadShader(g_shader);
        g_shader = (Shader){ 0 };
    }
    g_iniciado = false;
}

TipoHoloEvidencia EvidenciaHoloTipoPorId(int suspeito, const char *ev_id) {
    if (!ev_id || ev_id[0] == '\0') return HOLO_EV_NENHUM;
    if (suspeito == 0) {
        if (strcmp(ev_id, "EV-01") == 0) return HOLO_EV_MEMORANDO;      // Queixa no RH contra Ana Lucia
        if (strcmp(ev_id, "EV-02") == 0) return HOLO_EV_RELATORIO_ERRO; // Registro de relatorios devolvidos
        return HOLO_EV_MEMORANDO;
    }
    if (suspeito == 1) {
        if (strcmp(ev_id, "EV-03") == 0) return HOLO_EV_RELATORIOS;     // Reclamacao de sobrecarga / acobertamento
        if (strcmp(ev_id, "EV-04") == 0) return HOLO_EV_MEMORANDO;      // Pedido de desligamento do Arthur
        return HOLO_EV_RELATORIOS;
    }
    if (suspeito == 2) {
        if (strcmp(ev_id, "EV-05") == 0) return HOLO_EV_RELATORIO_ERRO; // Historico suspeito no terminal
        if (strcmp(ev_id, "EV-06") == 0) return HOLO_EV_LEITOR_SETOR;   // Log de acesso a Ciberseguranca
        return HOLO_EV_LEITOR_SETOR;
    }
    return HOLO_EV_MEMORANDO;
}

void EvidenciaHoloResetarCamera(EstadoCameraHoloEv *cam, TipoHoloEvidencia tipo) {
    if (!cam) return;
    memset(cam, 0, sizeof(*cam));
    cam->zoom = 1.0f;
    if (tipo == HOLO_EV_RELATORIOS) {
        cam->yaw   = -0.52f;
        cam->pitch =  0.38f;
    } else if (tipo == HOLO_EV_CANETA) {
        cam->yaw   =  0.22f;
        cam->pitch =  0.14f;
    } else if (tipo == HOLO_EV_CRACHA) {
        cam->yaw   = -0.24f;
        cam->pitch =  0.12f;
    } else {
        cam->yaw   = -0.32f;
        cam->pitch =  0.20f;
    }
}

static void GarantirRT(int w, int h) {
    if (w < 16) w = 16;
    if (h < 16) h = 16;
    if (g_rtPronta && g_rtW == w && g_rtH == h) return;
    if (g_rtPronta) {
        UnloadRenderTexture(g_rtCena);
        UnloadRenderTexture(g_rtFinal);
    }
    g_rtW = w;
    g_rtH = h;
    g_rtCena  = LoadRenderTexture(w, h);
    g_rtFinal = LoadRenderTexture(w, h);
    SetTextureFilter(g_rtCena.texture,  TEXTURE_FILTER_POINT);
    SetTextureFilter(g_rtFinal.texture, TEXTURE_FILTER_POINT);
    g_rtPronta = true;
}

void EvidenciaHoloRender(int largura, int altura, TipoHoloEvidencia tipo,
                         const EstadoCameraHoloEv *cam, float tempo) {
    if (!g_iniciado || tipo == HOLO_EV_NENHUM || !cam) return;
    GarantirRT(largura, altura);

    float yaw   = cam->yaw + (cam->interagiu ? 0.0f : sinf(tempo * 0.65f) * 0.28f);
    float pitch = Clamp(cam->pitch, -1.48f, 1.48f);
    float roll  = cam->roll;
    float zoom  = Clamp(cam->zoom, 0.55f, 2.80f);
    float flutuaY = sinf(tempo * 2.1f) * 0.055f;

    float distBase = (tipo == HOLO_EV_RELATORIOS) ? 4.35f : 4.15f;
    float dist = distBase / zoom;

    Camera3D c3d = { 0 };
    c3d.target     = (Vector3){ cam->panOffset.x, cam->panOffset.y, 0.0f };
    c3d.position   = (Vector3){ cam->panOffset.x, cam->panOffset.y + 0.12f, dist };
    c3d.up         = (Vector3){ sinf(roll), cosf(roll), 0.0f };
    c3d.fovy       = 38.0f;
    c3d.projection = CAMERA_PERSPECTIVE;
    g_camPosAtual  = c3d.position;

    // Rotacao estilo inspecao de artefato RE4: o proprio objeto gira em torno do centro (turntable + tilt)
    Matrix mObj = MatrixMultiply(
        MatrixMultiply(MatrixRotateY(yaw), MatrixRotateX(pitch)),
        MatrixTranslate(0.0f, flutuaY, 0.0f)
    );

    BeginTextureMode(g_rtCena);
    ClearBackground(BLANK);
    BeginMode3D(c3d);

    float opBase = Clamp(1.25f - (zoom - 1.0f) * 0.65f, 0.15f, 0.90f);
    float yPedestal = (tipo == HOLO_EV_RELATORIOS) ? -0.92f : -1.52f;
    DesenharRetoricaBaseHolo(yPedestal, 1.35f, tempo, opBase);

    switch (tipo) {
        case HOLO_EV_CANETA:         DesenharModeloCaneta(mObj);            break;
        case HOLO_EV_CRACHA:         DesenharModeloCracha(mObj);            break;
        case HOLO_EV_RELATORIOS:     DesenharModeloRelatorios(mObj);        break;
        case HOLO_EV_MEMORANDO:      DesenharModeloMemorando(mObj, false);  break;
        case HOLO_EV_RELATORIO_ERRO: DesenharModeloMemorando(mObj, true);   break;
        case HOLO_EV_RELOGIO_TEMPO:  DesenharModeloRelogioTempo(mObj);      break;
        case HOLO_EV_LEITOR_SETOR:   DesenharModeloLeitorSetor(mObj);       break;
        case HOLO_EV_GRAVADOR:       DesenharModeloGravador(mObj);          break;
        default: break;
    }

    EndMode3D();
    EndTextureMode();

    float altF = (float)g_rtH;
    SetShaderValue(g_shader, g_locTempo,  &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_shader, g_locAltura, &altF,  SHADER_UNIFORM_FLOAT);

    BeginTextureMode(g_rtFinal);
    ClearBackground(BLANK);
    BeginShaderMode(g_shader);
    DrawTextureRec(g_rtCena.texture,
                   (Rectangle){ 0, 0, (float)g_rtW, -(float)g_rtH },
                   (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
    EndTextureMode();
}

Texture2D EvidenciaHoloTextura(void) {
    return g_rtFinal.texture;
}
