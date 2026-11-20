#include "setas_3d.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

// Seta apontando para +x, em pixels: haste + ponta, extrudada em z
#define SETA_RAIO_CLIQUE 15.0f
#define SETA_PROF        9.0f
#define SETA_ESCALA      1.0f
#define SETA_INCLINACAO  0.0f     // setas retas (alinhadas com a tela)
#define SETA_TOMBO       0.0f

static const Vector2 CONTORNO[7] = {
    { -12.0f, -4.0f }, { 2.0f, -4.0f }, { 2.0f, -10.0f }, { 14.0f, 0.0f },
    { 2.0f, 10.0f }, { 2.0f, 4.0f }, { -12.0f, 4.0f },
};

// Dithering na rampa da paleta (sem efeitos de holograma: a seta faz parte da cena)
static const char *SETAS_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "const vec3 TONS[5] = vec3[5](vec3(0.0), vec3(32.0, 29.0, 31.0) / 255.0,\n"
    "    vec3(125.0, 112.0, 113.0) / 255.0, vec3(189.0, 181.0, 168.0) / 255.0, vec3(255.0, 242.0, 230.0) / 255.0);\n"
    "const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    if (c.a < 0.5) discard;\n"
    "    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    ivec2 p = ivec2(mod(gl_FragCoord.xy, 4.0));\n"
    "    float limiar = float(BAYER[p.y * 4 + p.x]) / 16.0 - 0.5;\n"
    "    int idx = int(clamp(floor(l * 4.0 + 0.5 + limiar), 0.0, 4.0));\n"
    "    finalColor = vec4(TONS[idx], 1.0);\n"
    "}\n";

static RenderTexture2D g_cena, g_final;
static Shader          g_shader;
static Camera3D        g_camera;
static int             g_largura, g_altura;
static Vector2         g_pos[NUM_SETAS];
static float           g_escalaHover[NUM_SETAS];
static bool            g_pronto = false;

static Color Lum(float l) {
    unsigned char v = (unsigned char)(Clamp(l, 0.0f, 1.0f) * 255.0f);
    return (Color){ v, v, v, 255 };
}

void Setas3DInit(int largura, int altura, const Vector2 posicoes[NUM_SETAS]) {
    Setas3DUnload();
    g_largura = largura;
    g_altura = altura;
    for (int i = 0; i < NUM_SETAS; i++) {
        g_pos[i] = posicoes[i];
        g_escalaHover[i] = 0.0f;
    }
    g_cena  = LoadRenderTexture(largura, altura);
    g_final = LoadRenderTexture(largura, altura);
    SetTextureFilter(g_cena.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(g_final.texture, TEXTURE_FILTER_POINT);
    g_shader = LoadShaderFromMemory(NULL, SETAS_FS);

    // Camera ortografica: 1 unidade do mundo = 1 pixel da textura, y para cima
    g_camera = (Camera3D){ 0 };
    g_camera.position = (Vector3){ 0.0f, 0.0f, 200.0f };
    g_camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f };
    g_camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    g_camera.fovy     = (float)altura;
    g_camera.projection = CAMERA_ORTHOGRAPHIC;
    g_pronto = true;
}

void Setas3DUnload(void) {
    if (!g_pronto) return;
    UnloadRenderTexture(g_cena);
    UnloadRenderTexture(g_final);
    UnloadShader(g_shader);
    g_pronto = false;
}

// Luz fixa vindo de cima/esquerda/frente: o brilho de cada face depende da normal
static float Sombreamento(Vector3 normal, Matrix rot, float base) {
    Vector3 n = Vector3Normalize(Vector3Transform(normal, rot));
    Vector3 luz = Vector3Normalize((Vector3){ -0.4f, 0.6f, 1.0f });
    float d = Vector3DotProduct(n, luz);
    return base * (0.35f + 0.65f * fmaxf(d, 0.0f));
}

static void DesenharSeta(float brilho, Matrix rot, bool sombra) {
    float zf = SETA_PROF / 2.0f, zt = -SETA_PROF / 2.0f;
    const Vector2 *c = CONTORNO;

    // Frente (z+) e verso (z-): haste (quad) + ponta (triangulo)
    Color frente = sombra ? Lum(0.04f) : Lum(Sombreamento((Vector3){ 0, 0, 1 }, rot, brilho));
    Color verso  = sombra ? Lum(0.04f) : Lum(Sombreamento((Vector3){ 0, 0, -1 }, rot, brilho));
    for (int k = 0; k < 2; k++) {
        float z = k == 0 ? zf : zt;
        Color cor = k == 0 ? frente : verso;
        Vector3 a = { c[0].x, c[0].y, z }, b = { c[1].x, c[1].y, z }, e = { c[5].x, c[5].y, z }, f = { c[6].x, c[6].y, z };
        Vector3 p2 = { c[2].x, c[2].y, z }, p3 = { c[3].x, c[3].y, z }, p4 = { c[4].x, c[4].y, z };
        DrawTriangle3D(a, b, e, cor);
        DrawTriangle3D(a, e, f, cor);
        DrawTriangle3D(p2, p3, p4, cor);
    }

    // Laterais da extrusao
    for (int i = 0; i < 7; i++) {
        Vector2 p = c[i], q = c[(i + 1) % 7];
        Vector3 normal = Vector3Normalize((Vector3){ q.y - p.y, -(q.x - p.x), 0.0f });
        Color cor = sombra ? Lum(0.04f) : Lum(Sombreamento(normal, rot, brilho));
        Vector3 a = { p.x, p.y, zf }, b = { q.x, q.y, zf }, d = { q.x, q.y, zt }, e = { p.x, p.y, zt };
        DrawTriangle3D(a, b, d, cor);
        DrawTriangle3D(a, d, e, cor);
    }

    if (sombra) return;
    // Contorno escuro da frente para destacar da foto
    for (int i = 0; i < 7; i++) {
        Vector2 p = c[i], q = c[(i + 1) % 7];
        DrawLine3D((Vector3){ p.x, p.y, zf + 0.2f }, (Vector3){ q.x, q.y, zf + 0.2f }, Lum(0.05f));
    }
}

void Setas3DRender(const bool visiveis[NUM_SETAS], int hover, float tempo) {
    if (!g_pronto) return;
    static const float ANGULO[NUM_SETAS] = { 90.0f, -90.0f, 180.0f, 0.0f };   // cima, baixo, esq, dir
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    BeginTextureMode(g_cena);
    ClearBackground(BLANK);
    BeginMode3D(g_camera);
    rlDisableBackfaceCulling();
    for (int i = 0; i < NUM_SETAS; i++) {
        float alvo = (i == hover) ? 1.0f : 0.0f;
        g_escalaHover[i] += (alvo - g_escalaHover[i]) * fminf(1.0f, dt * 12.0f);
        if (!visiveis[i]) continue;

        float h = g_escalaHover[i];
        float rad = ANGULO[i] * DEG2RAD;
        float empurra = sinf(tempo * 1.2f) * 0.8f + h * 1.5f;               // flutuacao bem leve na direcao da seta
        Vector3 centro = { g_pos[i].x - g_largura / 2.0f + cosf(rad) * empurra,
                           g_altura / 2.0f - g_pos[i].y + sinf(rad) * empurra, 0.0f };
        float escala = SETA_ESCALA * (1.0f + 0.12f * h);

        // Mesma inclinacao para todas, em relacao a tela (assim todas tem o mesmo volume e luz)
        Matrix rot = MatrixMultiply(MatrixMultiply(MatrixRotateZ(rad), MatrixRotateY(SETA_INCLINACAO * DEG2RAD)),
                                    MatrixRotateX(SETA_TOMBO * DEG2RAD));
        // Sombra escura deslocada (atras) e depois a seta
        for (int k = 0; k < 2; k++) {
            bool sombra = (k == 0);
            rlPushMatrix();
            rlTranslatef(centro.x + (sombra ? 2.5f : 0.0f), centro.y - (sombra ? 3.0f : 0.0f), sombra ? -40.0f : 0.0f);
            rlRotatef(SETA_TOMBO, 1, 0, 0);
            rlRotatef(SETA_INCLINACAO, 0, 1, 0);
            rlRotatef(ANGULO[i], 0, 0, 1);
            rlScalef(escala, escala, escala);
            DesenharSeta(0.80f + 0.05f * sinf(tempo * 1.6f) + 0.15f * h, rot, sombra);   // brilho respirando de leve
            rlPopMatrix();
        }
    }
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();

    BeginTextureMode(g_final);
    ClearBackground(BLANK);
    BeginShaderMode(g_shader);
    DrawTextureRec(g_cena.texture, (Rectangle){ 0, 0, (float)g_largura, -(float)g_altura }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
    EndTextureMode();
}

Texture2D Setas3DTextura(void) {
    return g_final.texture;
}

int Setas3DEm(Vector2 ponto, const bool visiveis[NUM_SETAS]) {
    for (int i = 0; i < NUM_SETAS; i++) {
        if (visiveis[i] && CheckCollisionPointCircle(ponto, g_pos[i], SETA_RAIO_CLIQUE)) return i;
    }
    return -1;
}

Vector2 Setas3DPos(int seta) {
    if (seta < 0 || seta >= NUM_SETAS) return (Vector2){ 0, 0 };
    return g_pos[seta];
}
