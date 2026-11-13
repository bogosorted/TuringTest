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

typedef enum { ESTADO_INTRO = 0, ESTADO_MIRANDO, ESTADO_TIRO, ESTADO_FINAL, ESTADO_PONTUACAO } EstadoVeredito;

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
        DesenharDicaClique("Clique para ver sua pontuação");
        if (clicou) {
            g_estado = ESTADO_PONTUACAO;
            g_tempo = 0.0f;
            g_pontuacaoCompleta = false;
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
static int QuebrarLinhas(const char *texto, int tam, float maxW, char linhas[][96], int maxLinhas) {
    int n = 0;
    char atual[96] = "";
    char buf[256];
    snprintf(buf, sizeof buf, "%s", texto);
    for (char *p = strtok(buf, " "); p; p = strtok(NULL, " ")) {
        char teste[96];
        snprintf(teste, sizeof teste, "%s%s%s", atual, atual[0] ? " " : "", p);
        if (atual[0] && MeasureText(teste, tam) > maxW) {
            if (n < maxLinhas) snprintf(linhas[n++], 96, "%s", atual);
            snprintf(atual, sizeof atual, "%s", p);
        } else {
            snprintf(atual, sizeof atual, "%s", teste);
        }
    }
    if (atual[0] && n < maxLinhas) snprintf(linhas[n++], 96, "%s", atual);
    return n;
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
    char linhas[16][4][96];
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

        // Anotacoes da prancheta de quem esta na mira
        if (mirando && g_hover >= 0) DesenharAnotacoes(g_hover);

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
    UnloadRenderTexture(g_armaRT);
    UnloadRenderTexture(g_armaFinal);
    UnloadShader(g_dither);
    RayCanvasClose();
}
