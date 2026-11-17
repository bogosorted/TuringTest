#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "verdict_scene.h"
#include "gameplay_scene.h"     // marcacoes da prancheta
#include "selection_scene.h"      // enum Employee (EMPLOYEE_LUCAS = posicao 2 = Ana Lucia, a IA)
#include "../utils/settings.h"
#include "../utils/raycanvas.h"
#include "../utils/game_state.h"
#include "../utils/ranking.h"
#include "../utils/dialog_data.h"

// ---------------------------------------------------------------
// VEREDITO: os 3 suspeitos na sala (2D) e o jogador com uma arma (3D)
// que acompanha o mouse. Clicar em alguem: tela preta, tiro + grito,
// e depois o texto do final.
// ---------------------------------------------------------------

#define VIRTUAL_W 640
#define VIRTUAL_H 360

#define AI_INDEX EMPLOYEE_LUCAS   // posicao 2 = Ana Lucia

// Sons opcionais (ainda serao adicionados): carregados so se existirem
#define SOM_TIRO "assets/audio/tiro_2.wav"
// Grito de cada suspeito (mesma ordem de NOMES)
static const char *SONS_GRITO[3] = {
    "assets/audio/Grito_veredito_mulher.mp3",      // Maria Bianca
    "assets/audio/Grito_veredito_masculino.mp3",   // Arthur
    "assets/audio/Grito_Veredito_Robotico.mp3",    // Ana Lucia (IA)
};

#define TEMPO_GRITO      0.35f   // segundos depois do tiro
#define PRETO_DEPOIS_GRITO 0.8f  // segundos de tela preta depois do grito
#define GRITO_MAX          1.6f  // espera no maximo isso do grito antes de seguir
#define TEMPO_CLARAO     0.16f   // clarao da bala antes da tela ficar preta
#define VELOCIDADE_FINAL 40.0f   // pixels virtuais por segundo que o texto do final sobe

// Explicacao antes do veredito (logo depois de "Dia 4 - Veredito")
static const char *TEXTO_INTRO =
    "Sexta-feira, fim do expediente.\n"
    "Você chamou Maria Bianca, Arthur e Ana Lúcia para uma\n"
    "\"revisão final de acessos\" no subsolo da RASEC.\n\n"
    "Eles acham que vão só assinar uns papéis.\n"
    "Não sabem que a porta se fecha atrás deles\n"
    "e que esta é a Sala de Descarte.\n\n"
    "Um deles não é humano. Você tem uma bala.\n\n"
    "Quem é a IA infiltrada?";

typedef enum { ESTADO_INTRO = 0, ESTADO_MIRANDO, ESTADO_TIRO, ESTADO_FINAL, ESTADO_JORNALISTA, ESTADO_PONTUACAO } EstadoVeredito;

static const Color TONS[5] = {
    { 0, 0, 0, 255 }, { 32, 29, 31, 255 }, { 125, 112, 113, 255 }, { 189, 181, 168, 255 }, { 255, 242, 230, 255 }
};
static const Color COR_CLARA = { 255, 242, 230, 255 };
static const Color COR_ESCURA = { 32, 29, 31, 255 };

static const char *NOMES[3] = { "Maria Bianca", "Arthur", "Ana Lúcia" };
static const char *SILHUETAS[3] = {
    "assets/sprites/personagens/sil_maria_bianca.png",
    "assets/sprites/personagens/sil_arthur_gadelho.png",
    "assets/sprites/personagens/sil_ana_lucia.png",
};

// Posicao dos suspeitos na sala (centro x e linha dos pes, em pixels virtuais)
static const Vector2 SUSPEITO_POS[3] = { { 214.0f, 268.0f }, { 320.0f, 262.0f }, { 426.0f, 268.0f } };
#define SIL_ESCALA 1.35f

// Textos dos finais (acerto e erro no veredito)
static const char *FINAL_ACERTO =
    "Ana Lúcia caiu sem um som humano.\n\n"
    "Por baixo da pele sintética, apenas fios,\n"
    "circuitos rompidos e um brilho azul frio.\n\n"
    "As contradições nos relatórios e nos setores\n"
    "não eram falhas humanas, mas sim a máscara\n"
    "de uma IA infiltrada na RASEC.\n\n"
    "Com a ameaça neutralizada, os corredores\n"
    "finalmente voltam ao silêncio.\n"
    "Você venceu o Teste de Turing.";
static const char *FINAL_ERRO =
    "%s caiu no chão da sala.\n\n"
    "Não houve faíscas nem falha de sistema.\n"
    "Era humano. Sangue de verdade.\n\n"
    "As hesitações e falhas que você julgou\n"
    "eram apenas o medo de alguém comum\n"
    "tentando provar que estava vivo.\n\n"
    "Enquanto o alarme da RASEC silencia,\n"
    "a verdadeira IA continua operando\n"
    "livremente em algum andar do prédio.";

static EstadoVeredito g_estado;
static float     g_tempo;            // tempo dentro do estado atual
static int       g_alvo = -1;        // em quem atirou
static int       g_hover = -1;
static int       g_hoverAnt = -1;
static bool      g_gritou;
static float     g_fimGrito = -1.0f; // quando o grito terminou (-1 = ainda tocando)
static bool      g_textoCompleto;   // clique pula a subida do texto
static bool      g_pontuacaoCompleta;
static bool      g_tentarDeNovo;
static char      g_textoFinal[1024];

typedef struct {
    char rotulo[48];
    int  alvo;
} ScoreLinha;

static ScoreLinha g_scoreLinhas[6];
static int        g_scoreTotal = 0;
static int        g_pranchetaMarcados[3] = { 0 };
static int        g_pranchetaTotais[3]   = { 0 };

static Texture2D g_sil[3];
static Sound     g_sndHover, g_sndTiro, g_sndGrito[3];

// ---------------------------------------------------------------
// SISTEMA DE RIGGING FACIAL + DITHERING PARA A JORNALISTA
// Adaptado do Photobash Rig: 9 pontos RBF + Pálpebras + Dithering Bayer 4x4
// ---------------------------------------------------------------
typedef enum {
    EXPR_JORN_SERIA = 0,
    EXPR_JORN_URGENTE,
    EXPR_JORN_PREOCUPADA,
    EXPR_JORN_SUSPENSE
} ExpressaoJornalista;

typedef struct {
    Vector2 browP[4]; // bLo, bLi, bRi, bRo
    Vector2 lidP[2];  // lidL, lidR
    Vector2 mouthP[2];// mL, mR
    Vector2 jawP;     // jaw
} FaceRigAnchors;

static const FaceRigAnchors ANCHORS_JORNALISTA = {
    .browP  = { { 0.322f, 0.436f }, { 0.384f, 0.428f }, { 0.434f, 0.423f }, { 0.497f, 0.428f } },
    .lidP   = { { 0.350f, 0.467f }, { 0.472f, 0.455f } },
    .mouthP = { { 0.378f, 0.601f }, { 0.441f, 0.596f } },
    .jawP   = { 0.410f, 0.674f }
};

typedef struct {
    Vector2 d[9];
} FaceRigPose;

static Texture2D g_texJornalista;
static Shader    g_shaderJornalista;
static int       g_locBrowP, g_locBrowD, g_locLidP, g_locLidD, g_locMouthP, g_locMouthD, g_locJawP, g_locJawD, g_locBlink, g_locPalpebraCol;
static FaceRigPose g_poseJornFrom, g_poseJornTarget, g_poseJornCurr;
static float     g_jornTweenT = 1.0f;
static float     g_jornTalkTimer = 0.0f;
static float     g_jornTypewriterTimer = 0.0f;
static float     g_jornPauseTimer = 0.0f;
static int       g_jornCharsRevealed = 0;
static int       g_jornFaseFala = 0; // 0 = fala 1, 1 = pausa, 2 = fala 2, 3 = sinal cortado
static float     g_jornPausaTimerFase = 0.0f;
typedef enum {
    TIPO_FINAL_NEUTRO = 0, // veredito correto + score < 5500
    TIPO_FINAL_BOM,        // veredito correto + score >= 5500
    TIPO_FINAL_RUIM        // veredito incorreto
} TipoFinal;
static TipoFinal g_tipoFinal = TIPO_FINAL_NEUTRO;
static Sound     g_sndBlipJornalista;

// Falas do Plantão — Final Bom (veredito correto + score >= 5500)
static const char *FALA_JORNALISTA_1 = 
    "Notícia urgente! O número de androides infiltrados começa a crescer no país, enquanto "
    "protestos sociais tomam as ruas exigindo direitos para os humanoides. Em meio à crise, "
    "a RASEC passa a ser investigada por suspeita de má conduta com seus funcionários.";

static const char *FALA_JORNALISTA_2 = 
    "Em outras notícias, um humanoide vai conceder uma entrevista inédita ainda hoje... "
    "e afirma possuir informações confidenciais.";

// Falas do Plantão — Final Ruim (veredito incorreto)
static const char *FALA_JORNALISTA_RUIM_1 =
    "Notícia urgente! A CEO da RASEC foi presa após o protótipo de investigação matar um "
    "funcionário inocente. Agora, a empresa é investigada sob suspeita de criar os próprios "
    "androides humanoides apenas para lucrar com a crise.";

static const char *FALA_JORNALISTA_RUIM_2 =
    "Em outras notícias, humanoides revelam que não escolhem quem vão copiar... "
    "e que os rostos que assumem já estavam decididos antes mesmo de eles despertarem.";

static const char *JORNALISTA_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec2 uBrowP[4];\n"
    "uniform vec2 uBrowD[4];\n"
    "uniform vec2 uLidP[2];\n"
    "uniform vec2 uLidD[2];\n"
    "uniform vec2 uMouthP[2];\n"
    "uniform vec2 uMouthD[2];\n"
    "uniform vec2 uJawP;\n"
    "uniform vec2 uJawD;\n"
    "uniform float uBlink;\n"
    "uniform vec3 uPalpebraCol;\n"
    "out vec4 finalColor;\n"
    "\n"
    "float rbf(vec2 p, vec2 c, float r) {\n"
    "    vec2 q = (p - c) / max(r, 0.0001);\n"
    "    return exp(-dot(q, q) * 0.5);\n"
    "}\n"
    "\n"
    "float bayer4(vec2 p) {\n"
    "    vec2 b = mod(floor(p), 4.0);\n"
    "    int x = int(b.x);\n"
    "    int y = int(b.y);\n"
    "    int idx = y * 4 + x;\n"
    "    if (idx == 0) return 0.0/16.0;\n"
    "    if (idx == 1) return 8.0/16.0;\n"
    "    if (idx == 2) return 2.0/16.0;\n"
    "    if (idx == 3) return 10.0/16.0;\n"
    "    if (idx == 4) return 12.0/16.0;\n"
    "    if (idx == 5) return 4.0/16.0;\n"
    "    if (idx == 6) return 14.0/16.0;\n"
    "    if (idx == 7) return 6.0/16.0;\n"
    "    if (idx == 8) return 3.0/16.0;\n"
    "    if (idx == 9) return 11.0/16.0;\n"
    "    if (idx == 10) return 1.0/16.0;\n"
    "    if (idx == 11) return 9.0/16.0;\n"
    "    if (idx == 12) return 15.0/16.0;\n"
    "    if (idx == 13) return 7.0/16.0;\n"
    "    if (idx == 14) return 13.0/16.0;\n"
    "    return 5.0/16.0;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 d = vec2(0.0);\n"
    "    float rBrow = 0.088;\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "        d += uBrowD[i] * rbf(uv, uBrowP[i], rBrow);\n"
    "    }\n"
    "    float lidRx = 0.075;\n"
    "    float lidRy = 0.052;\n"
    "    for (int i = 0; i < 2; i++) {\n"
    "        vec2 diff = uv - uLidP[i];\n"
    "        vec2 q = diff / vec2(lidRx, lidRy);\n"
    "        q.y *= (diff.y < 0.0) ? 0.85 : 1.45;\n"
    "        float w = exp(-dot(q, q) * 0.5);\n"
    "        float horiz = max(0.0, 1.0 - q.x * q.x * 0.35);\n"
    "        d.x += uLidD[i].x * w;\n"
    "        d.y += uLidD[i].y * w * horiz;\n"
    "        float openPull = max(0.0, -uLidD[i].y);\n"
    "        vec2 qOpen = diff / vec2(lidRx, lidRy * 1.5);\n"
    "        d.y -= openPull * 0.40 * exp(-dot(qOpen, qOpen) * 0.5);\n"
    "    }\n"
    "    float rMouth = 0.095;\n"
    "    for (int i = 0; i < 2; i++) {\n"
    "        d += uMouthD[i] * rbf(uv, uMouthP[i], rMouth);\n"
    "    }\n"
    "    vec2 mc = (uMouthP[0] + uMouthP[1]) * 0.5;\n"
    "    vec2 mcD = vec2(0.0, -(uMouthD[0].y + uMouthD[1].y) * 0.30);\n"
    "    d += mcD * rbf(uv, mc, rMouth * 0.9);\n"
    "    d += uJawD * rbf(uv, uJawP, 0.140);\n"
    "    vec2 sampleUV = clamp(uv - d, vec2(0.001), vec2(0.999));\n"
    "    vec4 texCol = texture(texture0, sampleUV);\n"
    "\n"
    "    // Paleta oficial RASEC / Turing Test\n"
    "    vec3 c0 = vec3(0.0, 0.0, 0.0);\n"
    "    vec3 c1 = vec3(32.0, 29.0, 31.0) / 255.0;\n"
    "    vec3 c2 = vec3(90.0, 83.0, 83.0) / 255.0;\n"
    "    vec3 c3 = vec3(125.0, 112.0, 113.0) / 255.0;\n"
    "    vec3 c4 = vec3(160.0, 147.0, 142.0) / 255.0;\n"
    "    vec3 c5 = vec3(189.0, 181.0, 168.0) / 255.0;\n"
    "\n"
    "    // Piscar dos olhos suave\n"
    "    if (uBlink > 0.10) {\n"
    "        float bProg = (uBlink - 0.10) / 0.90;\n"
    "        for (int i = 0; i < 2; i++) {\n"
    "            vec2 eyeCenter = uLidP[i] + uLidD[i] + vec2(0.0, 0.010);\n"
    "            vec2 q = (uv - eyeCenter) / vec2(0.065, 0.038);\n"
    "            float distSq = dot(q, q);\n"
    "            if (distSq <= 1.0) {\n"
    "                float arcY = (1.0 - q.x * q.x) * 0.40 * bProg;\n"
    "                float distArc = abs(q.y - arcY);\n"
    "                if (distArc < 0.16) {\n"
    "                    texCol = vec4(c1, 1.0);\n"
    "                } else if (q.y <= arcY + 0.35) {\n"
    "                    texCol = vec4(uPalpebraCol, 1.0);\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // Normalizacao de auto-levels para nao queimar nem escurecer demais a foto da jornalista\n"
    "    float lum = dot(texCol.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    float lumNorm = clamp((lum - 0.03) / (0.76 - 0.03), 0.0, 1.0);\n"
    "    vec2 pGrid = floor(gl_FragCoord.xy / 2.0);\n"
    "    float dVal = (bayer4(pGrid) - 0.5) * 0.22;\n"
    "    float val = clamp(lumNorm + dVal, 0.0, 1.0);\n"
    "\n"
    "    vec3 outRgb = c0;\n"
    "    if (val < 0.15) outRgb = c0;\n"
    "    else if (val < 0.32) outRgb = c1;\n"
    "    else if (val < 0.52) outRgb = c2;\n"
    "    else if (val < 0.70) outRgb = c3;\n"
    "    else if (val < 0.88) outRgb = c4;\n"
    "    else outRgb = c5;\n"
    "\n"
    "    finalColor = vec4(outRgb, 1.0) * fragColor;\n"
    "}\n";

static int QuebrarLinhas(const char *texto, int tam, float maxW, char linhas[][128], int maxLinhas);

static int GetUtf8CharLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static FaceRigPose ObterPoseJornalista(ExpressaoJornalista expr) {
    FaceRigPose p = { 0 };
    const float K = 0.11f;
    float boX = 0, boY = 0, biX = 0, biY = 0, lidX = 0, lidY = 0, moX = 0, moY = 0, jawX = 0, jawY = 0;

    switch (expr) {
        case EXPR_JORN_URGENTE: // Olhar firme, sobrancelhas levemente arqueadas, boca expressiva
            boX =  0.00f; boY = -0.10f; biX =  0.00f; biY = -0.10f;
            lidX = 0.00f; lidY = -0.05f; moX =  0.08f; moY = -0.05f; jawY = 0.05f;
            break;
        case EXPR_JORN_PREOCUPADA: // Sobrancelhas franzidas levemente para cima, expressão séria/sensível
            boX = -0.03f; boY =  0.08f; biX =  0.02f; biY = -0.15f;
            lidX = 0.00f; lidY =  0.08f; moX = -0.03f; moY =  0.10f; jawY = 0.02f;
            break;
        case EXPR_JORN_SUSPENSE: // Tensa, olhos mais abertos, queixo firme
            boX =  0.04f; boY = -0.12f; biX = -0.04f; biY = -0.14f;
            lidX = 0.00f; lidY = -0.08f; moX =  0.05f; moY =  0.02f; jawY = 0.04f;
            break;
        case EXPR_JORN_SERIA:
        default:
            break;
    }

    p.d[0] = (Vector2){ -boX * K,  boY * K };
    p.d[1] = (Vector2){ -biX * K,  biY * K };
    p.d[2] = (Vector2){  biX * K,  biY * K };
    p.d[3] = (Vector2){  boX * K,  boY * K };
    p.d[4] = (Vector2){ -lidX * K, lidY * K };
    p.d[5] = (Vector2){  lidX * K, lidY * K };
    p.d[6] = (Vector2){ -moX * K,  moY * K };
    p.d[7] = (Vector2){  moX * K,  moY * K };
    p.d[8] = (Vector2){  jawX * K, jawY * K };
    return p;
}

static void SetExpressaoJornalista(ExpressaoJornalista expr) {
    g_poseJornFrom   = g_poseJornCurr;
    g_poseJornTarget = ObterPoseJornalista(expr);
    g_jornTweenT  = 0.0f;
}

static float EaseBackJorn(float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static Texture2D g_sala;             // foto da sala (spr_veredito_sala.png, ja com dithering)
static RenderTexture2D g_armaRT;     // arma 3D crua
static RenderTexture2D g_armaFinal;  // arma com dithering
static Shader    g_dither;
static Camera3D  g_camera;
static float     g_armaX = 0.0f;     // posicao lateral suavizada
static float     g_recuo = 0.0f;     // recuo do tiro
static Vector2   g_bocaTela;         // boca do cano na tela (para o clarao da bala)

// Dithering na rampa da paleta (descarta so o transparente)
static const char *DITHER_FS =
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

// ---------------------------------------------------------------
// Utilitarios
// ---------------------------------------------------------------
static Rectangle GetTelaRect(void) {
    return RayCanvasGetRect((Vector2){ 0.5f, 0.5f }, (Vector2){ 0.5f, 0.5f },
                            (Vector2){ 0.0f, 0.0f }, (Vector2){ (float)VIRTUAL_W, (float)VIRTUAL_H });
}

static Rectangle ParaTela(Rectangle r) {
    Rectangle tela = GetTelaRect();
    float ex = tela.width / (float)VIRTUAL_W, ey = tela.height / (float)VIRTUAL_H;
    return (Rectangle){ tela.x + r.x * ex, tela.y + r.y * ey, r.width * ex, r.height * ey };
}

static Vector2 MouseVirtual(void) {
    Rectangle tela = GetTelaRect();
    Vector2 m = RayCanvasGetMousePosition();
    return (Vector2){ (m.x - tela.x) * VIRTUAL_W / tela.width, (m.y - tela.y) * VIRTUAL_H / tela.height };
}

static Color Cinza(float l) {
    unsigned char v = (unsigned char)(Clamp(l, 0.0f, 1.0f) * 255.0f);
    return (Color){ v, v, v, 255 };
}

static Sound CarregarSomOpcional(const char *caminho) {
    if (FileExists(caminho)) return LoadSound(caminho);
    TraceLog(LOG_INFO, "VEREDITO: som %s ainda nao existe (opcional)", caminho);
    return (Sound){ 0 };
}

static void Tocar(Sound s, float volume) {
    if (s.frameCount == 0) return;
    SetSoundVolume(s, volume * SettingsGetSfxVolume());
    PlaySound(s);
}

static Rectangle AreaSuspeito(int i) {
    float w = g_sil[i].width * SIL_ESCALA, h = g_sil[i].height * SIL_ESCALA;
    return (Rectangle){ SUSPEITO_POS[i].x - w / 2.0f, SUSPEITO_POS[i].y - h, w, h };
}


// ---------------------------------------------------------------
// ARMA 3D low poly (caixas sombreadas por face)
// ---------------------------------------------------------------
static float Luz(Vector3 normal, Matrix rot, float base) {
    Vector3 n = Vector3Normalize(Vector3Transform(normal, rot));
    Vector3 l = Vector3Normalize((Vector3){ -0.5f, 0.8f, 0.6f });
    return base * (0.30f + 0.70f * fmaxf(Vector3DotProduct(n, l), 0.0f));
}

// Caixa centrada em c com tamanho t; rot = rotacao total (para a luz)
static void Caixa(Vector3 c, Vector3 t, float base, Matrix rot) {
    Vector3 h = Vector3Scale(t, 0.5f);
    Vector3 v[8];
    for (int i = 0; i < 8; i++)
        v[i] = (Vector3){ c.x + ((i & 1) ? h.x : -h.x), c.y + ((i & 2) ? h.y : -h.y), c.z + ((i & 4) ? h.z : -h.z) };
    static const int F[6][4] = { { 1, 3, 7, 5 }, { 0, 4, 6, 2 }, { 2, 6, 7, 3 }, { 0, 1, 5, 4 }, { 4, 5, 7, 6 }, { 0, 2, 3, 1 } };
    static const Vector3 N[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    for (int f = 0; f < 6; f++) {
        Color cor = Cinza(Luz(N[f], rot, base));
        DrawTriangle3D(v[F[f][0]], v[F[f][1]], v[F[f][2]], cor);
        DrawTriangle3D(v[F[f][0]], v[F[f][2]], v[F[f][3]], cor);
    }
}

// Prisma de N lados ao longo do eixo z (tambor e cano do revolver)
static void Prisma(Vector3 c, float raio, float comp, int lados, float base, Matrix rot) {
    float z0 = c.z - comp / 2.0f, z1 = c.z + comp / 2.0f;
    for (int i = 0; i < lados; i++) {
        float a0 = (i + 0.5f) * 2.0f * PI / lados, a1 = (i + 1.5f) * 2.0f * PI / lados;
        Vector3 p0 = { c.x + cosf(a0) * raio, c.y + sinf(a0) * raio, 0 };
        Vector3 p1 = { c.x + cosf(a1) * raio, c.y + sinf(a1) * raio, 0 };
        float am = (a0 + a1) / 2.0f;
        Color cor = Cinza(Luz((Vector3){ cosf(am), sinf(am), 0 }, rot, base));
        DrawTriangle3D((Vector3){ p0.x, p0.y, z0 }, (Vector3){ p1.x, p1.y, z0 }, (Vector3){ p1.x, p1.y, z1 }, cor);
        DrawTriangle3D((Vector3){ p0.x, p0.y, z0 }, (Vector3){ p1.x, p1.y, z1 }, (Vector3){ p0.x, p0.y, z1 }, cor);
        Color tampaF = Cinza(Luz((Vector3){ 0, 0, -1 }, rot, base)), tampaT = Cinza(Luz((Vector3){ 0, 0, 1 }, rot, base));
        DrawTriangle3D((Vector3){ c.x, c.y, z0 }, (Vector3){ p1.x, p1.y, z0 }, (Vector3){ p0.x, p0.y, z0 }, tampaF);
        DrawTriangle3D((Vector3){ c.x, c.y, z1 }, (Vector3){ p0.x, p0.y, z1 }, (Vector3){ p1.x, p1.y, z1 }, tampaT);
    }
}

// Caixa inclinada em x (para empunhadura, antebracos)
static void CaixaInclinada(Vector3 c, Vector3 t, float graus, float base, Matrix rot) {
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(graus, 1, 0, 0);
    Caixa((Vector3){ 0, 0, 0 }, t, base, MatrixMultiply(MatrixRotateX(graus * DEG2RAD), rot));
    rlPopMatrix();
}

static void DesenharArma(Matrix rot) {
    // Revolver tipo Magnum apontando para -z (origem no centro da empunhadura, em cima)
    // Cano longo com ventilacao em cima e protetor do extrator embaixo
    Prisma((Vector3){ 0.0f, 0.22f, -1.55f }, 0.085f, 1.70f, 8, 0.80f, rot);              // cano
    Caixa((Vector3){ 0.0f, 0.33f, -1.55f }, (Vector3){ 0.07f, 0.06f, 1.70f }, 0.95f, rot);  // costela ventilada
    for (int k = 0; k < 6; k++)                                                                 // furos da ventilacao
        Caixa((Vector3){ 0.0f, 0.365f, -0.85f - k * 0.26f }, (Vector3){ 0.075f, 0.012f, 0.06f }, 0.20f, rot);
    Caixa((Vector3){ 0.0f, 0.10f, -1.40f }, (Vector3){ 0.13f, 0.13f, 1.35f }, 0.60f, rot);  // protetor do extrator
    Caixa((Vector3){ 0.0f, 0.40f, -2.33f }, (Vector3){ 0.04f, 0.09f, 0.10f }, 1.00f, rot);  // massa de mira
    Prisma((Vector3){ 0.0f, 0.22f, -2.41f }, 0.045f, 0.03f, 8, 0.05f, rot);              // boca do cano
    // Armacao, tambor e cao
    Caixa((Vector3){ 0.0f, 0.18f, -0.42f }, (Vector3){ 0.26f, 0.36f, 0.75f }, 0.55f, rot);  // armacao
    Prisma((Vector3){ 0.0f, 0.17f, -0.45f }, 0.21f, 0.52f, 6, 0.75f, rot);               // tambor (6 tiros)
    for (int k = 0; k < 6; k++) {                                                               // canaletas do tambor
        float a = k * PI / 3.0f;
        Caixa((Vector3){ cosf(a) * 0.215f, 0.17f + sinf(a) * 0.215f, -0.45f }, (Vector3){ 0.03f, 0.03f, 0.36f }, 0.25f, rot);
    }
    Caixa((Vector3){ 0.0f, 0.38f, -0.25f }, (Vector3){ 0.14f, 0.06f, 0.55f }, 0.85f, rot);  // topo da armacao
    Caixa((Vector3){ 0.0f, 0.42f, -0.03f }, (Vector3){ 0.10f, 0.05f, 0.06f }, 0.95f, rot);  // alca de mira
    CaixaInclinada((Vector3){ 0.0f, 0.40f, 0.10f }, (Vector3){ 0.07f, 0.16f, 0.07f }, 35.0f, 0.70f, rot);  // cao
    // Guarda-mato e gatilho
    Caixa((Vector3){ 0.0f, -0.10f, -0.42f }, (Vector3){ 0.05f, 0.04f, 0.38f }, 0.45f, rot);
    Caixa((Vector3){ 0.0f, -0.03f, -0.62f }, (Vector3){ 0.05f, 0.12f, 0.04f }, 0.45f, rot);
    CaixaInclinada((Vector3){ 0.0f, -0.02f, -0.32f }, (Vector3){ 0.04f, 0.13f, 0.04f }, -15.0f, 0.30f, rot);
    // Empunhadura de madeira (mais escura) com placa de metal na frente
    CaixaInclinada((Vector3){ 0.0f, -0.25f, 0.05f }, (Vector3){ 0.24f, 0.55f, 0.30f }, -18.0f, 0.35f, rot);
    CaixaInclinada((Vector3){ 0.0f, -0.24f, -0.10f }, (Vector3){ 0.18f, 0.50f, 0.04f }, -18.0f, 0.55f, rot);

    // Maos: punho direito na empunhadura, esquerda por baixo apoiando, antebracos saindo da tela
    CaixaInclinada((Vector3){ 0.0f, -0.26f, 0.08f }, (Vector3){ 0.36f, 0.34f, 0.40f }, -18.0f, 0.30f, rot);  // mao direita
    CaixaInclinada((Vector3){ -0.04f, -0.36f, -0.10f }, (Vector3){ 0.40f, 0.26f, 0.42f }, -10.0f, 0.24f, rot); // mao esquerda
    CaixaInclinada((Vector3){ 0.06f, -0.15f, -0.02f }, (Vector3){ 0.07f, 0.06f, 0.20f }, 0.0f, 0.34f, rot);   // polegar
    CaixaInclinada((Vector3){ 0.30f, -0.62f, 0.85f }, (Vector3){ 0.30f, 0.30f, 1.30f }, 22.0f, 0.18f, rot);   // antebraco direito
    CaixaInclinada((Vector3){ -0.34f, -0.68f, 0.80f }, (Vector3){ 0.30f, 0.30f, 1.30f }, 22.0f, 0.14f, rot);  // antebraco esquerdo
}

static void RenderizarArma(Vector2 mouse) {
    // Direcao do mouse no mundo (camera em 0,0,0 olhando para -z)
    float fov = g_camera.fovy * DEG2RAD;
    float ndcX = (mouse.x / VIRTUAL_W) * 2.0f - 1.0f;
    float ndcY = 1.0f - (mouse.y / VIRTUAL_H) * 2.0f;
    float tanY = tanf(fov / 2.0f), tanX = tanY * VIRTUAL_W / (float)VIRTUAL_H;
    Vector3 alvo = { ndcX * tanX * 12.0f, ndcY * tanY * 12.0f, -12.0f };

    // A arma desliza para o lado do mouse (suave) e gira apontando para ele
    float dt = fminf(GetFrameTime(), 0.05f);
    g_armaX = Lerp(g_armaX, ndcX * 0.35f, fminf(1.0f, dt * 8.0f));   // no centro, segurada com as duas maos
    g_recuo = fmaxf(0.0f, g_recuo - dt * 3.0f);
    Vector3 pos = { g_armaX, -1.05f, -3.1f + g_recuo * 0.4f };
    Vector3 d = Vector3Subtract(alvo, pos);
    float yaw = atan2f(-d.x, -d.z);
    float pitch = atan2f(d.y, sqrtf(d.x * d.x + d.z * d.z)) + g_recuo * 0.5f;
    float balanco = sinf((float)GetTime() * 1.4f) * 0.012f;   // respiracao leve

    Matrix rotArma = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
    Vector3 boca = Vector3Add(Vector3Add(pos, (Vector3){ 0, balanco, 0 }), Vector3Transform((Vector3){ 0.0f, 0.22f, -2.43f }, rotArma));
    g_bocaTela = GetWorldToScreenEx(boca, g_camera, VIRTUAL_W, VIRTUAL_H);

    BeginTextureMode(g_armaRT);
    ClearBackground(BLANK);
    BeginMode3D(g_camera);
    rlDisableBackfaceCulling();
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + balanco, pos.z);
    rlRotatef(yaw * RAD2DEG, 0, 1, 0);
    rlRotatef(pitch * RAD2DEG, 1, 0, 0);
    DesenharArma(MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw)));
    rlPopMatrix();
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();

    BeginTextureMode(g_armaFinal);
    ClearBackground(BLANK);
    BeginShaderMode(g_dither);
    DrawTextureRec(g_armaRT.texture, (Rectangle){ 0, 0, VIRTUAL_W, -VIRTUAL_H }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
    EndTextureMode();
}

// ---------------------------------------------------------------
// Cena
// ---------------------------------------------------------------
bool VerdictSceneShouldRestart(void) {
    return g_tentarDeNovo;
}

void InitVerdictScene(void) {
    g_estado = ESTADO_INTRO;
    g_tempo = 0.0f;
    g_alvo = -1;
    g_hover = g_hoverAnt = -1;
    g_gritou = false;
    g_tentarDeNovo = false;
    g_armaX = 0.0f;
    g_recuo = 0.0f;

    for (int i = 0; i < 3; i++) {
        g_sil[i] = LoadTexture(SILHUETAS[i]);
        SetTextureFilter(g_sil[i], TEXTURE_FILTER_POINT);
    }
    g_sndHover = LoadSound("assets/audio/sfx_hover.wav");
    g_sndTiro  = CarregarSomOpcional(SOM_TIRO);
    for (int i = 0; i < 3; i++) g_sndGrito[i] = CarregarSomOpcional(SONS_GRITO[i]);

    g_dither = LoadShaderFromMemory(NULL, DITHER_FS);
    g_sala = LoadTexture("assets/sprites/veredito/spr_veredito_sala.png");
    SetTextureFilter(g_sala, TEXTURE_FILTER_POINT);
    g_armaRT    = LoadRenderTexture(VIRTUAL_W, VIRTUAL_H);
    g_armaFinal = LoadRenderTexture(VIRTUAL_W, VIRTUAL_H);
    SetTextureFilter(g_armaFinal.texture, TEXTURE_FILTER_POINT);

    // Carrega assets da Jornalista para o plantão
    if (FileExists("assets/sprites/finais/jornalista_final.png")) {
        g_texJornalista = LoadTexture("assets/sprites/finais/jornalista_final.png");
        SetTextureFilter(g_texJornalista, TEXTURE_FILTER_POINT);
    } else if (FileExists("assets/sprites/finais/jornalista_final.jpg")) {
        g_texJornalista = LoadTexture("assets/sprites/finais/jornalista_final.jpg");
        SetTextureFilter(g_texJornalista, TEXTURE_FILTER_POINT);
    }
    g_shaderJornalista = LoadShaderFromMemory(NULL, JORNALISTA_FS);
    if (g_shaderJornalista.id > 0) {
        g_locBrowP  = GetShaderLocation(g_shaderJornalista, "uBrowP");
        g_locBrowD  = GetShaderLocation(g_shaderJornalista, "uBrowD");
        g_locLidP   = GetShaderLocation(g_shaderJornalista, "uLidP");
        g_locLidD   = GetShaderLocation(g_shaderJornalista, "uLidD");
        g_locMouthP = GetShaderLocation(g_shaderJornalista, "uMouthP");
        g_locMouthD = GetShaderLocation(g_shaderJornalista, "uMouthD");
        g_locJawP        = GetShaderLocation(g_shaderJornalista, "uJawP");
        g_locJawD        = GetShaderLocation(g_shaderJornalista, "uJawD");
        g_locBlink       = GetShaderLocation(g_shaderJornalista, "uBlink");
        g_locPalpebraCol = GetShaderLocation(g_shaderJornalista, "uPalpebraCol");
    }
    g_sndBlipJornalista = CarregarSomOpcional("assets/audio/blip_patricia.wav");

    g_poseJornCurr   = (FaceRigPose){ 0 };
    g_poseJornFrom   = (FaceRigPose){ 0 };
    g_poseJornTarget = (FaceRigPose){ 0 };
    g_jornTweenT     = 1.0f;
    g_jornTalkTimer  = 0.0f;
    g_jornFaseFala   = 0;
    g_jornCharsRevealed = 0;
    g_jornPausaTimerFase = 0.0f;
    g_tipoFinal = TIPO_FINAL_NEUTRO;


    g_camera = (Camera3D){ 0 };
    g_camera.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    g_camera.target   = (Vector3){ 0.0f, 0.0f, -1.0f };
    g_camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    g_camera.fovy     = 50.0f;
    g_camera.projection = CAMERA_PERSPECTIVE;

    RayCanvasInit(VIRTUAL_W, VIRTUAL_H);
}

static void CalcularESalvarPontuacao(void) {
    int ptsVeredito  = (g_alvo == AI_INDEX) ? 1000 : 0;
    int ptsVies      = !GameStateAnyViesUsed() ? 500 : 0;
    int ptsInjecao   = !GameStateAnyInjecaoUsed() ? 500 : 0;
    int dicasLivres  = GameStateGetUnusedHintsCount();
    int ptsDicas     = dicasLivres * 200;
    int tokensLivres = GameStateGetRemainingTokensTotal();
    int ptsTokens    = tokensLivres * 100;
    int evCount      = GameStateCountEvidences();
    int ptsEv        = evCount * 50;

    snprintf(g_scoreLinhas[0].rotulo, sizeof g_scoreLinhas[0].rotulo, "Veredito correto");
    g_scoreLinhas[0].alvo = ptsVeredito;
    snprintf(g_scoreLinhas[1].rotulo, sizeof g_scoreLinhas[1].rotulo, "Enviesamento nao usado");
    g_scoreLinhas[1].alvo = ptsVies;
    snprintf(g_scoreLinhas[2].rotulo, sizeof g_scoreLinhas[2].rotulo, "Injecao nao utilizada");
    g_scoreLinhas[2].alvo = ptsInjecao;
    snprintf(g_scoreLinhas[3].rotulo, sizeof g_scoreLinhas[3].rotulo, "Dicas nao utilizadas (%dx200)", dicasLivres);
    g_scoreLinhas[3].alvo = ptsDicas;
    snprintf(g_scoreLinhas[4].rotulo, sizeof g_scoreLinhas[4].rotulo, "Tokens nao utilizados (%dx100)", tokensLivres);
    g_scoreLinhas[4].alvo = ptsTokens;
    snprintf(g_scoreLinhas[5].rotulo, sizeof g_scoreLinhas[5].rotulo, "Evidencias encontradas (%dx50)", evCount);
    g_scoreLinhas[5].alvo = ptsEv;

    g_scoreTotal = ptsVeredito + ptsVies + ptsInjecao + ptsDicas + ptsTokens + ptsEv;
    RankingAdd(GameStateGetPlayerName(), g_scoreTotal, g_alvo == AI_INDEX);

    for (int s = 0; s < 3; s++) {
        const char *itens[16];
        bool marcados[16];
        int n = GameplaySceneItensPrancheta(s, itens, marcados, 16);
        int m = 0;
        for (int k = 0; k < n; k++) if (marcados[k]) m++;
        g_pranchetaMarcados[s] = m;
        g_pranchetaTotais[s] = n;
    }

    // Final Bom (Plantão da Jornalista): veredito correto + pontuação >= 5500
    // Final Neutro: veredito correto + pontuação < 5500 (vai direto para pontuação)
    // Final Ruim: veredito incorreto (também exibe a jornalista, com texto diferente)
    if (g_alvo != AI_INDEX) {
        g_tipoFinal = TIPO_FINAL_RUIM;
    } else if (g_scoreTotal >= 5500) {
        g_tipoFinal = TIPO_FINAL_BOM;
    } else {
        g_tipoFinal = TIPO_FINAL_NEUTRO;
    }
}

static void Atirar(int i) {
    g_alvo = i;
    g_estado = ESTADO_TIRO;
    g_tempo = 0.0f;
    g_recuo = 1.0f;
    g_gritou = false;
    g_fimGrito = -1.0f;
    Tocar(g_sndTiro, 0.9f);
    if (g_alvo == AI_INDEX) snprintf(g_textoFinal, sizeof g_textoFinal, "%s", FINAL_ACERTO);
    else                    snprintf(g_textoFinal, sizeof g_textoFinal, FINAL_ERRO, NOMES[g_alvo]);
    CalcularESalvarPontuacao();
    TraceLog(LOG_INFO, "Veredito: atirou em %s (%s) - %d pts", NOMES[g_alvo], g_alvo == AI_INDEX ? "correto" : "incorreto", g_scoreTotal);
}

// Texto em linhas centralizadas a partir de y (virtual)
static float DesenharLinhas(const char *texto, float y, int tamVirtual, Color cor) {
    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;
    int tam = (int)(tamVirtual * escala);
    char buf[512];
    snprintf(buf, sizeof buf, "%s", texto);
    char *linha = buf;
    while (linha) {
        char *fim = strchr(linha, '\n');
        if (fim) *fim = '\0';
        Rectangle r = ParaTela((Rectangle){ 0, y, VIRTUAL_W, 16 });
        DrawText(linha, (int)(r.x + (r.width - MeasureText(linha, tam)) / 2.0f), (int)r.y, tam, cor);
        y += tamVirtual + 4.0f;
        linha = fim ? fim + 1 : NULL;
    }
    return y;
}

static int ContarLinhas(const char *t) {
    int n = 1;
    for (; *t; t++) if (*t == '\n') n++;
    return n;
}

static bool DesenharDicaClique(const char *dica) {
    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;
    float pisca = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
    int t2 = (int)(10 * escala);
    Rectangle r = ParaTela((Rectangle){ 0, 326, VIRTUAL_W, 12 });
    DrawText(dica, (int)(r.x + (r.width - MeasureText(dica, t2)) / 2.0f), (int)r.y, t2, Fade(TONS[3], pisca));
    return !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// O texto do final entra por baixo e sobe devagar ate o meio da tela (como creditos)
static void DesenharTextoFinal(void) {
    float altura = ContarLinhas(g_textoFinal) * 18.0f;
    float yFinal = (VIRTUAL_H - altura) / 2.0f;
    float y = VIRTUAL_H - g_tempo * VELOCIDADE_FINAL;
    bool clicou = !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (!g_textoCompleto && y > yFinal && clicou) { g_textoCompleto = true; clicou = false; }   // clique: mostra tudo
    bool chegou = g_textoCompleto || y <= yFinal;
    if (chegou) y = yFinal;
    DesenharLinhas(g_textoFinal, y, 14, COR_CLARA);
    if (chegou) {
        if (g_tipoFinal == TIPO_FINAL_BOM) {
            DesenharDicaClique("Clique para acompanhar o Plantao de Noticias");
            if (clicou) {
                g_estado = ESTADO_JORNALISTA;
                g_tempo = 0.0f;
                g_jornFaseFala = 0;
                g_jornCharsRevealed = 0;
                g_jornPausaTimerFase = 0.0f;
                g_jornTypewriterTimer = 0.0f;
                g_jornPauseTimer = 0.0f;
                SetExpressaoJornalista(EXPR_JORN_URGENTE);
            }
        } else if (g_tipoFinal == TIPO_FINAL_RUIM) {
            DesenharDicaClique("Clique para acompanhar o Plantao de Noticias");
            if (clicou) {
                g_estado = ESTADO_JORNALISTA;
                g_tempo = 0.0f;
                g_jornFaseFala = 0;
                g_jornCharsRevealed = 0;
                g_jornPausaTimerFase = 0.0f;
                g_jornTypewriterTimer = 0.0f;
                g_jornPauseTimer = 0.0f;
                SetExpressaoJornalista(EXPR_JORN_URGENTE);
            }
        } else {
            DesenharDicaClique("Clique para ver sua pontuação");
            if (clicou) {
                g_estado = ESTADO_PONTUACAO;
                g_tempo = 0.0f;
                g_pontuacaoCompleta = false;
            }
        }
    }
}

// Renderiza a cena do Plantão da Jornalista com dithering suave, Rig facial e fala sincronizada
static void DesenharCenaJornalista(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    // Atualiza interpolação do rig da jornalista
    if (g_jornTweenT < 1.0f) {
        g_jornTweenT += dt / 0.35f;
        if (g_jornTweenT > 1.0f) g_jornTweenT = 1.0f;
    }
    float e = EaseBackJorn(g_jornTweenT);
    for (int i = 0; i < 9; i++) {
        g_poseJornCurr.d[i].x = g_poseJornFrom.d[i].x + (g_poseJornTarget.d[i].x - g_poseJornFrom.d[i].x) * e;
        g_poseJornCurr.d[i].y = g_poseJornFrom.d[i].y + (g_poseJornTarget.d[i].y - g_poseJornFrom.d[i].y) * e;
    }

    // Piscar natural a cada ~4.0s
    float tNow = (float)GetTime();
    float cicloBlink = fmodf(tNow, 4.0f);
    float blinkT = 0.0f;
    if (cicloBlink < 0.18f) {
        blinkT = sinf((cicloBlink / 0.18f) * 3.14159265f);
    }

    // Movimento sutil da mandíbula/boca ao falar
    float talkJawAdd = 0.0f;
    if (g_jornTalkTimer > 0.0f) {
        g_jornTalkTimer -= dt;
        if (g_jornTalkTimer < 0.0f) g_jornTalkTimer = 0.0f;
        talkJawAdd = (0.5f + 0.5f * sinf(tNow * 22.0f)) * 0.012f;
    }

    // Gerenciador de diálogo e fases do Plantão
    // Seleciona as falas corretas de acordo com o tipo de final
    const char *fala1 = (g_tipoFinal == TIPO_FINAL_RUIM) ? FALA_JORNALISTA_RUIM_1 : FALA_JORNALISTA_1;
    const char *fala2 = (g_tipoFinal == TIPO_FINAL_RUIM) ? FALA_JORNALISTA_RUIM_2 : FALA_JORNALISTA_2;

    const char *textoAtual = "";
    if (g_jornFaseFala == 0) {
        textoAtual = fala1;
    } else if (g_jornFaseFala == 1) {
        // Breve pausa dramática entre as duas notícias
        g_jornPausaTimerFase += dt;
        g_jornTalkTimer = 0.0f;
        if (g_jornPausaTimerFase >= 1.6f) {
            g_jornFaseFala = 2;
            g_jornCharsRevealed = 0;
            g_jornTypewriterTimer = 0.0f;
            g_jornPauseTimer = 0.0f;
            SetExpressaoJornalista(EXPR_JORN_SUSPENSE);
        }
    } else if (g_jornFaseFala == 2) {
        textoAtual = fala2;
    } else if (g_jornFaseFala == 3) {
        // Sinal cortado / Estática
        g_jornTalkTimer = 0.0f;
    }

    // Typewriter
    int totalLen = (int)strlen(textoAtual);
    bool clicou = !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (g_jornFaseFala == 0 || g_jornFaseFala == 2) {
        if (g_jornCharsRevealed < totalLen) {
            if (clicou) {
                g_jornCharsRevealed = totalLen;
                g_jornTalkTimer = 0.0f;
                clicou = false; // Consome clique apenas para completar a linha
            } else if (g_jornPauseTimer > 0.0f) {
                g_jornPauseTimer -= dt;
                g_jornTalkTimer = 0.0f;
            } else {
                g_jornTypewriterTimer += dt;
                const float charSpeed = 0.026f;
                while (g_jornTypewriterTimer >= charSpeed && g_jornCharsRevealed < totalLen) {
                    g_jornTypewriterTimer -= charSpeed;
                    unsigned char c = (unsigned char)textoAtual[g_jornCharsRevealed];
                    int charLen = GetUtf8CharLength(c);
                    g_jornCharsRevealed += charLen;
                    g_jornTalkTimer = 0.14f;

                    // Mudança dinâmica de expressão durante a fala
                    if (g_jornFaseFala == 0 && g_jornCharsRevealed > 150) {
                        SetExpressaoJornalista(EXPR_JORN_PREOCUPADA);
                    }

                    // Pausas por pontuação
                    if (c == '.' || c == '!' || c == '?') {
                        if (g_jornCharsRevealed < totalLen && textoAtual[g_jornCharsRevealed] == '.') {
                            g_jornPauseTimer = 0.12f;
                        } else {
                            g_jornPauseTimer = 0.35f;
                        }
                        break;
                    } else if (c == ',' || c == ':' || c == ';') {
                        g_jornPauseTimer = 0.18f;
                        break;
                    } else if (c != ' ' && c != '\n') {
                        if (g_sndBlipJornalista.frameCount > 0 && (g_jornCharsRevealed % 3 == 0)) {
                            SetSoundPitch(g_sndBlipJornalista, 1.05f + ((float)GetRandomValue(-8, 8) / 100.0f));
                            SetSoundVolume(g_sndBlipJornalista, 0.40f * SettingsGetSfxVolume());
                            PlaySound(g_sndBlipJornalista);
                        }
                    }
                }
            }
        } else {
            g_jornTalkTimer = 0.0f;
            if (g_jornFaseFala == 0) {
                DesenharDicaClique("Clique para continuar");
                if (clicou) {
                    g_jornFaseFala = 1;
                    g_jornPausaTimerFase = 0.0f;
                    SetExpressaoJornalista(EXPR_JORN_SERIA);
                }
            } else if (g_jornFaseFala == 2) {
                DesenharDicaClique("Clique para continuar");
                if (clicou) {
                    g_jornFaseFala = 3;
                    g_tempo = 0.0f;
                }
            }
        }
    }

    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;

    // Fundo preto absoluto (a cena se passa assistindo à TV em um ambiente escuro)
    Rectangle rTelaCheia = ParaTela((Rectangle){ 0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H });
    DrawRectangleRec(rTelaCheia, BLACK);

    // Estrutura do aparelho de TV de tubo / monitor CRT centralizado
    // Gabinete externo: 460 x 330 (deixando bordas pretas ao redor)
    Rectangle rTvGabineteVirt = { 90.0f, 15.0f, 460.0f, 330.0f };
    Rectangle rTvGabineteTela = ParaTela(rTvGabineteVirt);
    DrawRectangleRec(rTvGabineteTela, (Color){ 20, 18, 19, 255 });
    DrawRectangleLinesEx(rTvGabineteTela, 2.5f * escala, (Color){ 50, 46, 48, 255 });

    // Detalhe de ventilação / botões do gabinete da TV no canto inferior
    DrawRectangle((int)(rTvGabineteTela.x + rTvGabineteTela.width - 24 * escala),
                  (int)(rTvGabineteTela.y + rTvGabineteTela.height - 18 * escala),
                  (int)(10 * escala), (int)(10 * escala), (Color){ 10, 9, 10, 255 });
    DrawCircle((int)(rTvGabineteTela.x + rTvGabineteTela.width - 40 * escala),
               (int)(rTvGabineteTela.y + rTvGabineteTela.height - 13 * escala),
               3.0f * escala, (Color){ 180, 40, 40, 255 }); // LED vermelho da TV ligada

    // Área interna da tela (vidro CRT)
    Rectangle rTvTelaVirt = { 106.0f, 26.0f, 428.0f, 222.0f };
    Rectangle rTvTela = ParaTela(rTvTelaVirt);

    // 1) Desenha a imagem da jornalista com o shader de Rig e Dithering na tela da TV
    if (g_texJornalista.id > 0) {
        if (g_shaderJornalista.id > 0 && g_jornFaseFala < 3) {
            BeginShaderMode(g_shaderJornalista);

            const FaceRigAnchors *anc = &ANCHORS_JORNALISTA;
            Vector2 browD[4]  = { g_poseJornCurr.d[0], g_poseJornCurr.d[1], g_poseJornCurr.d[2], g_poseJornCurr.d[3] };
            Vector2 lidD[2]   = { g_poseJornCurr.d[4], g_poseJornCurr.d[5] };
            Vector2 mouthD[2] = {
                { g_poseJornCurr.d[6].x, g_poseJornCurr.d[6].y + talkJawAdd * 0.45f },
                { g_poseJornCurr.d[7].x, g_poseJornCurr.d[7].y + talkJawAdd * 0.45f }
            };
            Vector2 jawD      = { g_poseJornCurr.d[8].x, g_poseJornCurr.d[8].y + talkJawAdd };

            SetShaderValueV(g_shaderJornalista, g_locBrowP,  anc->browP,  SHADER_UNIFORM_VEC2, 4);
            SetShaderValueV(g_shaderJornalista, g_locBrowD,  browD,       SHADER_UNIFORM_VEC2, 4);
            SetShaderValueV(g_shaderJornalista, g_locLidP,   anc->lidP,   SHADER_UNIFORM_VEC2, 2);
            SetShaderValueV(g_shaderJornalista, g_locLidD,   lidD,        SHADER_UNIFORM_VEC2, 2);
            SetShaderValueV(g_shaderJornalista, g_locMouthP, anc->mouthP, SHADER_UNIFORM_VEC2, 2);
            SetShaderValueV(g_shaderJornalista, g_locMouthD, mouthD,      SHADER_UNIFORM_VEC2, 2);
            SetShaderValue (g_shaderJornalista, g_locJawP,   &anc->jawP,  SHADER_UNIFORM_VEC2);
            SetShaderValue (g_shaderJornalista, g_locJawD,   &jawD,       SHADER_UNIFORM_VEC2);

            Vector3 palCol = (Vector3){ 90.0f / 255.0f, 83.0f / 255.0f, 83.0f / 255.0f };
            SetShaderValue(g_shaderJornalista, g_locBlink, &blinkT, SHADER_UNIFORM_FLOAT);
            SetShaderValue(g_shaderJornalista, g_locPalpebraCol, &palCol, SHADER_UNIFORM_VEC3);

            Rectangle src = { 0, 40, (float)g_texJornalista.width, (float)g_texJornalista.height - 80 };
            DrawTexturePro(g_texJornalista, src, rTvTela, (Vector2){ 0, 0 }, 0.0f, WHITE);

            EndShaderMode();
        } else if (g_jornFaseFala >= 3) {
            // Efeito estática de sinal cortado estilo transmissão de TV analógica
            DrawRectangleRec(rTvTela, TONS[1]);
            for (int sy = (int)rTvTela.y; sy < (int)(rTvTela.y + rTvTela.height); sy += 4) {
                int rnd = GetRandomValue(0, 100);
                Color cEst = (rnd > 50) ? TONS[4] : ((rnd > 25) ? TONS[2] : TONS[0]);
                DrawRectangle((int)rTvTela.x, sy, (int)rTvTela.width, 2, Fade(cEst, 0.45f));
            }
        } else {
            Rectangle src = { 0, 0, (float)g_texJornalista.width, (float)g_texJornalista.height };
            DrawTexturePro(g_texJornalista, src, rTvTela, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
    }

    // Linhas de scanline suaves da tela de TV
    for (int y = (int)rTvTela.y; y < (int)(rTvTela.y + rTvTela.height); y += 3) {
        DrawRectangle((int)rTvTela.x, y, (int)rTvTela.width, 1, Fade(BLACK, 0.28f));
    }

    // Moldura do vidro da tela
    DrawRectangleLinesEx(rTvTela, 1.5f * escala, (Color){ 30, 28, 30, 255 });

    // 2) Barra superior de telejornal na tela da TV
    Rectangle rBarraTopVirt = { 106.0f, 26.0f, 428.0f, 20.0f };
    Rectangle rBarraTop = ParaTela(rBarraTopVirt);
    DrawRectangleRec(rBarraTop, Fade(TONS[1], 0.90f));
    DrawRectangleLinesEx(rBarraTop, 1.0f * escala, TONS[3]);
    DrawText("XX/XX/XXXX — [PLANTAO DO DIA]", (int)(rBarraTop.x + 10 * escala), (int)(rBarraTop.y + 5 * escala), (int)(9 * escala), COR_CLARA);

    // Indicador "AO VIVO" piscando no canto
    float piscaAoVivo = 0.5f + 0.5f * sinf(tNow * 4.0f);
    DrawRectangle((int)(rBarraTop.x + rBarraTop.width - 55 * escala), (int)(rBarraTop.y + 6 * escala), (int)(6 * escala), (int)(6 * escala), Fade((Color){ 220, 50, 50, 255 }, piscaAoVivo));
    DrawText("AO VIVO", (int)(rBarraTop.x + rBarraTop.width - 44 * escala), (int)(rBarraTop.y + 5 * escala), (int)(8 * escala), COR_CLARA);

    // 3) Painel inferior de Noticiário / Legendas na TV
    Rectangle rPainelInfVirt = { 106.0f, 248.0f, 428.0f, 85.0f };
    Rectangle rPainelInf = ParaTela(rPainelInfVirt);
    DrawRectangleRec(rPainelInf, Fade(TONS[1], 0.96f));
    DrawRectangleLinesEx(rPainelInf, 1.5f * escala, TONS[3]);

    // Tarja de manchete "JORNALISTA:"
    Rectangle rTarjaVirt = { 116.0f, 253.0f, 80.0f, 14.0f };
    Rectangle rTarja = ParaTela(rTarjaVirt);
    DrawRectangleRec(rTarja, TONS[2]);
    DrawText("JORNALISTA", (int)(rTarja.x + 6 * escala), (int)(rTarja.y + 3 * escala), (int)(9 * escala), COR_CLARA);

    // Texto da fala
    if (g_jornFaseFala == 1) {
        Rectangle rPausaVirt = { 204.0f, 254.0f, 0, 0 };
        Rectangle rPausa = ParaTela(rPausaVirt);
        DrawText("...", (int)rPausa.x, (int)rPausa.y, (int)(12 * escala), TONS[3]);
    } else if (g_jornFaseFala == 3) {
        Rectangle rCortadoVirt = { 204.0f, 253.0f, 0, 0 };
        Rectangle rCortado = ParaTela(rCortadoVirt);
        DrawText("[SINAL CORTADO]", (int)rCortado.x, (int)rCortado.y, (int)(11 * escala), (Color){ 240, 80, 80, 255 });
        DesenharDicaClique("Clique para ver sua pontuacao");
        if (clicou) {
            g_estado = ESTADO_PONTUACAO;
            g_tempo = 0.0f;
            g_pontuacaoCompleta = false;
        }
    } else {
        char bufferVisivel[512];
        int nCopy = g_jornCharsRevealed;
        if (nCopy > (int)sizeof(bufferVisivel) - 1) nCopy = (int)sizeof(bufferVisivel) - 1;
        memcpy(bufferVisivel, textoAtual, nCopy);
        bufferVisivel[nCopy] = '\0';

        char linhas[8][128];
        int numLinhas = QuebrarLinhas(bufferVisivel, (int)(10 * escala), rPainelInf.width - 24 * escala, linhas, 8);
        for (int l = 0; l < numLinhas; l++) {
            Rectangle rLinhaVirt = { 116.0f, 270.0f + l * 14.0f, 0, 0 };
            Rectangle rLinha = ParaTela(rLinhaVirt);
            DrawText(linhas[l], (int)rLinha.x, (int)rLinha.y, (int)(10 * escala), COR_CLARA);
        }
    }
}

static void DesenharPontuacaoFinal(void) {
    const float durLinha = 0.45f;
    float durTotal = 6.0f * durLinha;
    bool clicou = !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (!g_pontuacaoCompleta && g_tempo < durTotal && clicou) {
        g_pontuacaoCompleta = true;
        clicou = false;
    }
    bool terminou = g_pontuacaoCompleta || (g_tempo >= durTotal);

    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;
    int tamTitulo = (int)(18 * escala);
    int tamTexto  = (int)(12 * escala);
    int tamPeq    = (int)(10 * escala);

    Rectangle painel = { 80.0f, 30.0f, 480.0f, 278.0f };
    Rectangle rPainel = ParaTela(painel);
    DrawRectangleRec(rPainel, Fade(COR_ESCURA, 0.96f));
    DrawRectangleLinesEx(rPainel, 2.0f * escala, TONS[3]);

    char cabecalho[64];
    snprintf(cabecalho, sizeof cabecalho, "RELATÓRIO DE PONTUAÇÃO - %s", GameStateGetPlayerName());
    Rectangle rCab = ParaTela((Rectangle){ painel.x + 16.0f, painel.y + 14.0f, 0, 0 });
    DrawText(cabecalho, (int)rCab.x, (int)rCab.y, tamTitulo, COR_CLARA);

    Rectangle rSub = ParaTela((Rectangle){ painel.x + 16.0f, painel.y + 38.0f, 0, 0 });
    char sub[96];
    snprintf(sub, sizeof sub, "Pranchetas: Maria (%d/%d) | Arthur (%d/%d) | Ana Lucia (%d/%d)",
             g_pranchetaMarcados[0], g_pranchetaTotais[0],
             g_pranchetaMarcados[1], g_pranchetaTotais[1],
             g_pranchetaMarcados[2], g_pranchetaTotais[2]);
    DrawText(sub, (int)rSub.x, (int)rSub.y, tamPeq, TONS[3]);

    int somaAnimada = 0;
    for (int i = 0; i < 6; i++) {
        float y = painel.y + 68.0f + i * 28.0f;
        float t0 = i * durLinha;
        float prog = terminou ? 1.0f : fmaxf(0.0f, fminf(1.0f, (g_tempo - t0) / durLinha));
        int valAtual = (int)(g_scoreLinhas[i].alvo * prog + 0.5f);
        somaAnimada += valAtual;

        Color corLinha = (prog > 0.0f) ? COR_CLARA : TONS[2];
        Rectangle rNome = ParaTela((Rectangle){ painel.x + 20.0f, y, 0, 0 });
        DrawText(g_scoreLinhas[i].rotulo, (int)rNome.x, (int)rNome.y, tamTexto, corLinha);

        char valTxt[24];
        snprintf(valTxt, sizeof valTxt, "+%d", valAtual);
        int wVal = MeasureText(valTxt, tamTexto);
        Rectangle rDir = ParaTela((Rectangle){ painel.x + painel.width - 20.0f, y, 0, 0 });
        DrawText(valTxt, (int)(rDir.x - wVal), (int)rDir.y, tamTexto,
                 (g_scoreLinhas[i].alvo > 0 && prog > 0.0f) ? TONS[4] : TONS[2]);
    }

    Rectangle rDiv = ParaTela((Rectangle){ painel.x + 16.0f, painel.y + 240.0f, painel.width - 32.0f, 1.5f });
    DrawRectangleRec(rDiv, TONS[3]);

    Rectangle rTotLbl = ParaTela((Rectangle){ painel.x + 20.0f, painel.y + 248.0f, 0, 0 });
    DrawText("PONTUAÇÃO TOTAL", (int)rTotLbl.x, (int)rTotLbl.y, tamTitulo, COR_CLARA);

    char totTxt[24];
    snprintf(totTxt, sizeof totTxt, "%d pts", terminou ? g_scoreTotal : somaAnimada);
    int wTot = MeasureText(totTxt, tamTitulo);
    Rectangle rTotDir = ParaTela((Rectangle){ painel.x + painel.width - 20.0f, painel.y + 248.0f, 0, 0 });
    DrawText(totTxt, (int)(rTotDir.x - wTot), (int)rTotDir.y, tamTitulo, TONS[4]);

    if (terminou) {
        DesenharDicaClique("Clique para voltar ao menu");
        if (clicou) g_tentarDeNovo = true;
    }
}

// Quebra "texto" em linhas que cabem em maxW (pixels de tela); devolve quantas linhas
static int QuebrarLinhas(const char *texto, int tam, float maxW, char linhas[][128], int maxLinhas) {
    int n = 0;
    char atual[128] = "";
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", texto);
    for (char *p = strtok(buf, " "); p; p = strtok(NULL, " ")) {
        char teste[128];
        snprintf(teste, sizeof teste, "%s%s%s", atual, atual[0] ? " " : "", p);
        if (atual[0] && MeasureText(teste, tam) > maxW) {
            if (n < maxLinhas) snprintf(linhas[n++], 128, "%s", atual);
            snprintf(atual, sizeof atual, "%s", p);
        } else {
            snprintf(atual, sizeof atual, "%s", teste);
        }
    }
    if (atual[0] && n < maxLinhas) snprintf(linhas[n++], 128, "%s", atual);
    return n;
}

// Painel neutro com todas as evidencias coletadas durante a investigacao (sem vincular a NPC)
static void DesenharEvidenciasColetadas(void) {
    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;
    int tam = (int)(9 * escala);
    const float painelW = 200.0f;
    const float linhaH  = 11.0f;
    const float padX    = 8.0f;
    const float padY    = 6.0f;

    // Coleta todas as evidencias de todos os suspeitos sem expor o dono
    char nomes[32][64];
    int total = 0;
    for (int s = 0; s < GAME_NUM_SUSPECTS && total < 32; s++) {
        int cnt = GetEvidenciaCount(s);
        for (int e = 0; e < cnt && total < 32; e++) {
            const char *id = GetEvidenciaIdByIndex(s, e);
            if (GameStateTemEvidencia(s, id)) {
                const char *nome = GetEvidenciaNome(s, id);
                snprintf(nomes[total], 64, "%s", nome ? nome : id);
                total++;
            }
        }
    }

    float painelH = padY * 2 + 14.0f + total * linhaH + (total > 0 ? 2.0f : 0.0f);
    float px = 8.0f;
    float py = 20.0f;
    Rectangle painel = { px, py, painelW, painelH };

    // Sombra + fundo
    DrawRectangleRec(ParaTela((Rectangle){ painel.x + 2, painel.y + 2, painelW, painelH }), Fade(BLACK, 0.6f));
    DrawRectangleRec(ParaTela(painel), Fade(COR_ESCURA, 0.95f));
    DrawRectangleLinesEx(ParaTela(painel), 1.5f * escala, TONS[3]);

    // Titulo neutro
    Rectangle t = ParaTela((Rectangle){ px + padX, py + padY, painelW, 10 });
    DrawText("EVIDENCIAS COLETADAS", (int)t.x, (int)t.y, tam, TONS[3]);

    if (total == 0) {
        Rectangle r = ParaTela((Rectangle){ px + padX, py + padY + 14.0f, 0, 0 });
        DrawText("Nenhuma evidencia.", (int)r.x, (int)r.y, tam, TONS[2]);
    } else {
        for (int k = 0; k < total; k++) {
            float cy = py + padY + 14.0f + k * linhaH;
            Rectangle r = ParaTela((Rectangle){ px + padX, cy, 0, 0 });
            DrawText(nomes[k], (int)r.x, (int)r.y, tam, COR_CLARA);
        }
    }
}

// Painel com todos os itens da prancheta do suspeito, destacando os marcados
static void DesenharAnotacoes(int i) {
    const char *itens[16];
    bool marcados[16];
    int n = GameplaySceneItensPrancheta(i, itens, marcados, 16);
    Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
    float escala = ref.height / 10.0f;
    int tam = (int)(9 * escala);
    const float w = 190.0f, linhaH = 10.0f, recuo = 18.0f;

    // Quebra todas as linhas antes, para saber a altura do painel
    char linhas[16][4][128];
    int nl[16], total = 0;
    for (int k = 0; k < n; k++) {
        nl[k] = QuebrarLinhas(itens[k], tam, (w - 12.0f - recuo) * escala, linhas[k], 4);
        total += nl[k];
    }
    float h = 20.0f + total * linhaH + n * 3.0f + 4.0f;
    Rectangle a = AreaSuspeito(i);
    float x = (i == 2) ? a.x - w - 6.0f : a.x + a.width + 6.0f;   // a direita (ou a esquerda, no ultimo)
    float y = fminf(a.y + 10.0f, VIRTUAL_H - h - 4.0f);
    Rectangle painel = { x, y, w, h };

    DrawRectangleRec(ParaTela((Rectangle){ painel.x + 2, painel.y + 2, w, h }), Fade(BLACK, 0.6f));
    DrawRectangleRec(ParaTela(painel), Fade(COR_ESCURA, 0.95f));
    DrawRectangleLinesEx(ParaTela(painel), 1.5f * escala, TONS[3]);
    Rectangle t = ParaTela((Rectangle){ painel.x + 6, painel.y + 5, w, 10 });
    DrawText("PRANCHETA", (int)t.x, (int)t.y, tam, TONS[3]);

    float cy = painel.y + 19.0f;
    for (int k = 0; k < n; k++) {
        Color corTexto = marcados[k] ? COR_CLARA : TONS[2];
        Rectangle caixa = ParaTela((Rectangle){ painel.x + 6, cy, 0, 0 });
        DrawText(marcados[k] ? "[X]" : "[ ]", (int)caixa.x, (int)caixa.y, tam,
                 marcados[k] ? (Color){ 196, 72, 72, 255 } : TONS[2]);
        for (int l = 0; l < nl[k]; l++) {
            Rectangle r = ParaTela((Rectangle){ painel.x + 6 + recuo, cy + l * linhaH, 0, 0 });
            DrawText(linhas[k][l], (int)r.x, (int)r.y, tam, corTexto);
        }
        cy += nl[k] * linhaH + 3.0f;
    }
}

// Lampada falhando: quase sempre acesa, com piscadas soltas e uma rajada de falhas a cada ~5 s
static float BrilhoLampada(float t) {
    float passo = floorf(t * 14.0f);
    float r = fabsf(fmodf(sinf(passo * 12.9898f) * 43758.5453f, 1.0f));
    float ciclo = fmodf(t, 5.0f);
    if (ciclo > 4.3f) return (r < 0.55f) ? 0.2f : 1.0f;
    if (r < 0.03f) return 0.45f;
    return 1.0f;
}

void UpdateDrawVerdictScene(void) {
    float dt = fminf(GetFrameTime(), 0.05f);
    g_tempo += dt;
    Vector2 mouse = MouseVirtual();

    // A arma e renderizada antes do frame (textura propria)
    bool cenaVisivel = (g_estado == ESTADO_MIRANDO) || (g_estado == ESTADO_TIRO && g_tempo < TEMPO_CLARAO);
    if (cenaVisivel) RenderizarArma(mouse);

    RayCanvasBegin();
    Rectangle tela = GetTelaRect();

    if (cenaVisivel) {
        bool mirando = (g_estado == ESTADO_MIRANDO);
        HideCursor();
        DrawTexturePro(g_sala, (Rectangle){ 0, 0, (float)g_sala.width, (float)g_sala.height }, tela, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // Suspeitos (quem esta na mira acende)
        if (mirando) {
            g_hover = -1;
            for (int i = 0; i < 3; i++)
                if (CheckCollisionPointRec(mouse, AreaSuspeito(i))) g_hover = i;
        }
        if (g_hover >= 0 && g_hover != g_hoverAnt) {
            SetSoundPitch(g_sndHover, 0.9f);
            Tocar(g_sndHover, 0.3f);
        }
        g_hoverAnt = g_hover;

        for (int i = 0; i < 3; i++) {
            Rectangle a = AreaSuspeito(i);
            a.y += sinf((float)GetTime() * 1.1f + i * 2.0f) * 0.8f;   // respiracao
            Rectangle src = { 0, 0, (float)g_sil[i].width, (float)g_sil[i].height };
            // sombra no chao
            DrawEllipse((int)ParaTela((Rectangle){ SUSPEITO_POS[i].x, SUSPEITO_POS[i].y, 0, 0 }).x,
                        (int)ParaTela((Rectangle){ SUSPEITO_POS[i].x, SUSPEITO_POS[i].y, 0, 0 }).y,
                        40.0f * tela.width / VIRTUAL_W, 7.0f * tela.height / VIRTUAL_H, Fade(BLACK, 0.5f));
            if (i == g_hover) {   // contorno claro de quem esta na mira
                const Vector2 offs[4] = { { -1.5f, 0 }, { 1.5f, 0 }, { 0, -1.5f }, { 0, 1.5f } };
                for (int d = 0; d < 4; d++) {
                    Rectangle o = { a.x + offs[d].x, a.y + offs[d].y, a.width, a.height };
                    DrawTexturePro(g_sil[i], src, ParaTela(o), (Vector2){ 0, 0 }, 0.0f, COR_CLARA);
                }
            }
            DrawTexturePro(g_sil[i], src, ParaTela(a), (Vector2){ 0, 0 }, 0.0f, WHITE);
        }

        // Pergunta e nome de quem esta na mira
        Rectangle ref = ParaTela((Rectangle){ 0, 0, 10, 10 });
        float escala = ref.height / 10.0f;
        const char *titulo = "Qual deles é a IA infiltrada?";
        int tt = (int)(14 * escala);
        Rectangle rt = ParaTela((Rectangle){ 0, 3, VIRTUAL_W, 16 });
        DrawText(titulo, (int)(rt.x + (rt.width - MeasureText(titulo, tt)) / 2.0f), (int)rt.y, tt, COR_CLARA);
        if (g_hover >= 0) {
            Rectangle a = AreaSuspeito(g_hover);
            int tn = (int)(10 * escala);
            float wv = MeasureText(NOMES[g_hover], tn) / escala + 12.0f;
            Rectangle placa = { SUSPEITO_POS[g_hover].x - wv / 2.0f, a.y - 18.0f, wv, 14.0f };
            Rectangle pt = ParaTela(placa);   // so o nome, sem caixa
            DrawText(NOMES[g_hover], (int)(pt.x + 6 * escala + 1), (int)(pt.y + (pt.height - tn) / 2.0f + 1), tn, BLACK);
            DrawText(NOMES[g_hover], (int)(pt.x + 6 * escala), (int)(pt.y + (pt.height - tn) / 2.0f), tn, COR_CLARA);
        }

        // Lista neutra de evidencias coletadas (sempre visivel, sem vincular a NPC)
        if (mirando) DesenharEvidenciasColetadas();

        // Lampada falhando (escurece a sala; a arma fica por cima, perto do jogador)
        float brilho = BrilhoLampada((float)GetTime());
        if (brilho < 1.0f) DrawRectangleRec(tela, Fade(BLACK, (1.0f - brilho) * 0.85f));

        // Arma 3D por cima de tudo
        DrawTexturePro(g_armaFinal.texture, (Rectangle){ 0, 0, VIRTUAL_W, -VIRTUAL_H }, tela, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // Mira no lugar do cursor
        Rectangle pm = ParaTela((Rectangle){ mouse.x, mouse.y, 0, 0 });
        float r = 6.0f * escala;
        Color corMira = g_hover >= 0 ? (Color){ 196, 72, 72, 255 } : COR_CLARA;
        DrawCircleLines((int)pm.x, (int)pm.y, r, corMira);
        DrawLineEx((Vector2){ pm.x - r * 1.8f, pm.y }, (Vector2){ pm.x - r * 0.6f, pm.y }, 1.5f * escala, corMira);
        DrawLineEx((Vector2){ pm.x + r * 0.6f, pm.y }, (Vector2){ pm.x + r * 1.8f, pm.y }, 1.5f * escala, corMira);
        DrawLineEx((Vector2){ pm.x, pm.y - r * 1.8f }, (Vector2){ pm.x, pm.y - r * 0.6f }, 1.5f * escala, corMira);
        DrawLineEx((Vector2){ pm.x, pm.y + r * 0.6f }, (Vector2){ pm.x, pm.y + r * 1.8f }, 1.5f * escala, corMira);

        // Clarao da bala saindo do cano
        if (!mirando) {
            float k = 1.0f - g_tempo / TEMPO_CLARAO;
            Rectangle pb = ParaTela((Rectangle){ g_bocaTela.x, g_bocaTela.y, 0, 0 });
            DrawRectangleRec(tela, Fade(COR_CLARA, 0.35f * k));
            for (int raio = 0; raio < 8; raio++) {   // estrela do disparo
                float a = raio * PI / 4.0f + 0.3f;
                float comp = (raio % 2 ? 22.0f : 40.0f) * escala * (0.6f + 0.4f * k);
                DrawLineEx((Vector2){ pb.x, pb.y }, (Vector2){ pb.x + cosf(a) * comp, pb.y + sinf(a) * comp }, 4.0f * escala, Fade(COR_CLARA, k));
            }
            DrawCircleV((Vector2){ pb.x, pb.y }, 16.0f * escala * (0.7f + 0.3f * k), Fade(COR_CLARA, k));
            DrawCircleV((Vector2){ pb.x, pb.y }, 8.0f * escala, Fade(WHITE, k));
        }

        if (mirando && g_hover >= 0 && !RayCanvasInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Atirar(g_hover);
    } else {
        ShowCursor();
        ClearBackground(BLACK);
        DrawRectangleRec(tela, BLACK);
        if (g_estado == ESTADO_INTRO) {
            // Explicacao antes de mirar (aparece aos poucos)
            float alfa = fminf(1.0f, g_tempo / 1.2f);
            float altura = ContarLinhas(TEXTO_INTRO) * 18.0f;
            DesenharLinhas(TEXTO_INTRO, (VIRTUAL_H - altura) / 2.0f - 10.0f, 14, Fade(COR_CLARA, alfa));
            if (g_tempo > 1.5f && DesenharDicaClique("Clique para entrar na sala")) { g_estado = ESTADO_MIRANDO; g_tempo = 0.0f; }
        } else if (g_estado == ESTADO_TIRO) {
            if (!g_gritou && g_tempo >= TEMPO_GRITO) { Tocar(g_sndGrito[g_alvo], 0.9f); g_gritou = true; }
            // Espera o grito acabar e mais um pouco de tela preta antes do texto
            // (no maximo GRITO_MAX segundos: o grito pode seguir tocando por baixo do texto)
            if (g_gritou && g_fimGrito < 0.0f &&
                (!IsSoundPlaying(g_sndGrito[g_alvo]) || g_tempo >= TEMPO_GRITO + GRITO_MAX)) g_fimGrito = g_tempo;
            if (g_fimGrito >= 0.0f && g_tempo >= g_fimGrito + PRETO_DEPOIS_GRITO) {
                g_estado = ESTADO_FINAL;
                g_tempo = 0.0f;
                g_textoCompleto = false;
            }
        } else if (g_estado == ESTADO_FINAL) {
            DesenharTextoFinal();
        } else if (g_estado == ESTADO_JORNALISTA) {
            DesenharCenaJornalista();
        } else {
            DesenharPontuacaoFinal();
        }
    }

    RayCanvasEnd();
}

void UnloadVerdictScene(void) {
    ShowCursor();
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    for (int i = 0; i < 3; i++) UnloadTexture(g_sil[i]);
    if (g_sndHover.frameCount > 0) UnloadSound(g_sndHover);
    if (g_sndTiro.frameCount > 0)  UnloadSound(g_sndTiro);
    for (int i = 0; i < 3; i++) {
        if (g_sndGrito[i].frameCount > 0) UnloadSound(g_sndGrito[i]);
        g_sndGrito[i] = (Sound){ 0 };
    }
    g_sndHover = g_sndTiro = (Sound){ 0 };
    UnloadTexture(g_sala);
    if (g_texJornalista.id > 0) UnloadTexture(g_texJornalista);
    if (g_shaderJornalista.id > 0) UnloadShader(g_shaderJornalista);
    if (g_sndBlipJornalista.frameCount > 0) UnloadSound(g_sndBlipJornalista);
    UnloadRenderTexture(g_armaRT);
    UnloadRenderTexture(g_armaFinal);
    UnloadShader(g_dither);
    RayCanvasClose();
}
