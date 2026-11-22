#include "gameplay_logic_hint.h"
#include "ui_common.h"
#include "raycanvas.h"
#include "settings.h"
#include "game_state.h"
#include "dialog_system.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *frase;      // proposicao em portugues
    const char *p;          // o que "p" significa
    const char *q;          // o que "q" significa
    const char *resposta;   // forma simbolica canonica (qualquer formula equivalente tambem vale)
} Proposicao;

#define NUM_PROPOSICOES 16
static const Proposicao PROPOSICOES[NUM_PROPOSICOES] = {
    { "Maria Bianca confere os relatórios e Arthur não entrega o relatório.",
      "Maria Bianca confere os relatórios", "Arthur entrega o relatório", "p ^ ~q" },
    { "Se Analu cobra os relatórios, então Maria Bianca não sai mais cedo.",
      "Analu cobra os relatórios", "Maria Bianca sai mais cedo", "p -> ~q" },
    { "Arthur não toca guitarra ou Analu devolve o relatório.",
      "Arthur toca guitarra", "Analu devolve o relatório", "~p v q" },
    { "Não é verdade que o ID sumiu e o cofre abriu.",
      "o ID sumiu", "o cofre abriu", "~(p ^ q)" },
    { "Arthur dá uma caneta preta se, e somente se, é próximo de Maria Bianca.",
      "Arthur dá uma caneta preta", "Arthur é próximo de Maria Bianca", "p <-> q" },
    { "Se Maria Bianca não assina o relatório, então o setor não é auditado.",
      "Maria Bianca assina o relatório", "o setor é auditado", "~p -> ~q" },
    { "Analu está no setor ou Maria Bianca está viajando.",
      "Analu está no setor", "Maria Bianca está viajando", "p v q" },
    { "Nem Arthur nem Analu estavam no setor.",
      "Arthur estava no setor", "Analu estava no setor", "~p ^ ~q" },
    { "Se Maria Bianca fala de viagens, então ela está à vontade.",
      "Maria Bianca fala de viagens", "Maria Bianca está à vontade", "p -> q" },
    { "Maria Bianca não está à vontade ou não fala de viagens.",
      "Maria Bianca está à vontade", "Maria Bianca fala de viagens", "~p v ~q" },
    { "Arthur puxa assunto e Analu não está por perto.",
      "Arthur puxa assunto", "Analu está por perto", "p ^ ~q" },
    { "Não é verdade que Maria Bianca ou Arthur acessaram o sistema.",
      "Maria Bianca acessou o sistema", "Arthur acessou o sistema", "~(p v q)" },
    { "É falso que, se Arthur chega atrasado, Analu o repreende.",
      "Arthur chega atrasado", "Analu repreende Arthur", "~(p -> q)" },
    { "Ou Maria Bianca ou Arthur sabe de quem é a caneta, mas não ambos.",
      "Maria Bianca sabe de quem é a caneta", "Arthur sabe de quem é a caneta", "(p v q) ^ ~(p ^ q)" },
    { "Analu devolve o relatório se, e somente se, ele está errado.",
      "Analu devolve o relatório", "o relatório está errado", "p <-> q" },
    { "Se Arthur presenteia a caneta, então Maria Bianca fica desconfiada.",
      "Arthur presenteia a caneta", "Maria Bianca fica desconfiada", "p -> q" },
};

enum { T_P, T_Q, T_NOT, T_AND, T_OR, T_IMP, T_IFF, T_LP, T_RP };
static const char *TOKEN_TXT[9] = { "p", "q", "~", "^", "v", "->", "<->", "(", ")" };
static const int BLOCO_TOKEN[9] = { T_P, T_Q, T_NOT, T_LP, T_RP, T_AND, T_OR, T_IMP, T_IFF };
#define DICA_MAX_TOKENS 14

static int TokenizarFormula(const char *txt, int *out, int max) {
    int n = 0;
    for (const char *c = txt; *c && n < max; c++) {
        switch (*c) {
            case 'p': out[n++] = T_P; break;
            case 'q': out[n++] = T_Q; break;
            case '~': out[n++] = T_NOT; break;
            case '^': out[n++] = T_AND; break;
            case 'v': out[n++] = T_OR; break;
            case '(': out[n++] = T_LP; break;
            case ')': out[n++] = T_RP; break;
            case '-': if (c[1] == '>') { out[n++] = T_IMP; c++; } break;
            case '<': if (c[1] == '-' && c[2] == '>') { out[n++] = T_IFF; c += 2; } break;
            default: break;
        }
    }
    return n;
}

typedef struct { const int *tok; int n; int pos; bool erro; bool p, q; } ParserFormula;

static bool PfIff(ParserFormula *ps);
static bool PfUnario(ParserFormula *ps) {
    if (ps->pos >= ps->n) { ps->erro = true; return false; }
    int t = ps->tok[ps->pos++];
    if (t == T_NOT) return !PfUnario(ps);
    if (t == T_P) return ps->p;
    if (t == T_Q) return ps->q;
    if (t == T_LP) {
        bool v = PfIff(ps);
        if (ps->pos < ps->n && ps->tok[ps->pos] == T_RP) ps->pos++; else ps->erro = true;
        return v;
    }
    ps->erro = true;
    return false;
}
static bool PfE(ParserFormula *ps) {
    bool v = PfUnario(ps);
    while (!ps->erro && ps->pos < ps->n && ps->tok[ps->pos] == T_AND) { ps->pos++; bool r = PfUnario(ps); v = v && r; }
    return v;
}
static bool PfOu(ParserFormula *ps) {
    bool v = PfE(ps);
    while (!ps->erro && ps->pos < ps->n && ps->tok[ps->pos] == T_OR) { ps->pos++; bool r = PfE(ps); v = v || r; }
    return v;
}
static bool PfImp(ParserFormula *ps) {
    bool v = PfOu(ps);
    if (!ps->erro && ps->pos < ps->n && ps->tok[ps->pos] == T_IMP) { ps->pos++; bool r = PfImp(ps); v = !v || r; }
    return v;
}
static bool PfIff(ParserFormula *ps) {
    bool v = PfImp(ps);
    while (!ps->erro && ps->pos < ps->n && ps->tok[ps->pos] == T_IFF) { ps->pos++; bool r = PfImp(ps); v = (v == r); }
    return v;
}

static bool TabelaVerdade(const int *tok, int n, int *tabela) {
    *tabela = 0;
    if (n <= 0) return false;
    for (int i = 0; i < 4; i++) {
        ParserFormula ps = { tok, n, 0, false, (i & 2) != 0, (i & 1) != 0 };
        bool v = PfIff(&ps);
        if (ps.erro || ps.pos != n) return false;
        if (v) *tabela |= (1 << i);
    }
    return true;
}

static void FormulaParaTexto(const int *tok, int n, char *buf, size_t cap) {
    buf[0] = '\0';
    for (int i = 0; i < n; i++) {
        size_t len = strlen(buf);
        bool semEspaco = (i == 0) || tok[i - 1] == T_NOT || tok[i - 1] == T_LP || tok[i] == T_RP;
        snprintf(buf + len, cap - len, "%s%s", semEspaco ? "" : " ", TOKEN_TXT[tok[i]]);
    }
}

static const Rectangle DICA_PAINEL = { 170, 32, 300, 296 };
static const Rectangle DICA_OK     = { 170 + 100, 32 + 254, 100, 26 };

static Texture2D g_iconAceso = { 0 };
static bool g_dicaUsadaHoje      = false;
static bool g_entrevistaLiberada = false;
static bool g_infinitosAntes     = false;

static bool   g_dicaAberta = false;
static bool   g_dicaTutorialVisto = false;
static double g_dicaTAbertura = 0.0;
#define DICA_TEMPO_TOTAL 45.0f
static int    g_dicaEstado = 3;
static float  g_dicaTempo = DICA_TEMPO_TOTAL;
static bool   g_dicaTempoEsgotado = false;
static int    g_dicaPagina = 0;
#define DICA_NUM_PAGINAS 3
static int    g_dicaAtual  = 0;
static unsigned g_dicaSorteadas = 0;
static int    g_dicaFormula[DICA_MAX_TOKENS];
static int    g_dicaNFormula = 0;
static float  g_dicaInvalidaT = 0.0f;
static float  g_dicaShake = 0.0f;
static float  g_dicaShakeAmp = 0.0f;

static Sound g_dicaSndClique = { 0 }, g_dicaSndAcerto = { 0 }, g_dicaSndErro = { 0 };

typedef struct { Vector2 pos, vel; float vida, vidaMax; Color cor; } DicaParticula;
#define DICA_NUM_PART 48
static DicaParticula g_dicaPart[DICA_NUM_PART];

static Sound CriarSomNotas(const float *freqs, int nNotas, float durNota, bool quadrada, float vol) {
    const int taxa = 22050;
    int porNota = (int)(durNota * taxa);
    int total = porNota * nNotas;
    short *dados = (short *)MemAlloc((unsigned)total * sizeof(short));
    if (!dados) return (Sound){ 0 };
    float fase = 0.0f;
    for (int n = 0; n < nNotas; n++) {
        for (int i = 0; i < porNota; i++) {
            float t = (float)i / (float)porNota;
            float env = (1.0f - t) * (1.0f - t);
            if (i < 80) env *= (float)i / 80.0f;
            fase += 2.0f * PI * freqs[n] / (float)taxa;
            float v = quadrada ? ((sinf(fase) >= 0.0f) ? 1.0f : -1.0f) * 0.6f : sinf(fase);
            dados[n * porNota + i] = (short)(v * env * vol * 32000.0f);
        }
    }
    Wave w = { (unsigned)total, (unsigned)taxa, 16, 1, dados };
    Sound snd = LoadSoundFromWave(w);
    MemFree(dados);
    return snd;
}

static void GarantirSonsDica(void) {
    if (g_dicaSndClique.frameCount > 0) return;
    float fClique[1] = { 540.0f };
    float fAcerto[3] = { 660.0f, 880.0f, 1320.0f };
    float fErro[2]   = { 220.0f, 150.0f };
    g_dicaSndClique = CriarSomNotas(fClique, 1, 0.05f, false, 0.35f);
    g_dicaSndAcerto = CriarSomNotas(fAcerto, 3, 0.09f, false, 0.45f);
    g_dicaSndErro   = CriarSomNotas(fErro, 2, 0.16f, true, 0.35f);
}

static void TocarSomDica(Sound snd, float pitch) {
    if (snd.frameCount == 0) return;
    SetSoundVolume(snd, SettingsGetSfxVolume());
    SetSoundPitch(snd, pitch);
    PlaySound(snd);
}

static float Rand01(void) { return (float)GetRandomValue(0, 1000) / 1000.0f; }

static void SoltarParticulasDica(bool acerto) {
    Color verdes[3]    = { { 120, 190, 140, 255 }, { 189, 181, 168, 255 }, { 255, 242, 230, 255 } };
    Color vermelhos[3] = { { 224, 108, 104, 255 }, { 196,  72,  72, 255 }, { 138,  48,  50, 255 } };
    int quantas = acerto ? DICA_NUM_PART : DICA_NUM_PART / 3;
    for (int i = 0; i < DICA_NUM_PART; i++) {
        DicaParticula *pt = &g_dicaPart[i];
        if (i >= quantas) { pt->vida = 0.0f; continue; }
        float ang = (acerto ? -PI * (0.1f + 0.8f * Rand01()) : 2.0f * PI * Rand01());
        float vel = 50.0f + 110.0f * Rand01();
        pt->pos = (Vector2){ DICA_PAINEL.x + DICA_PAINEL.width * 0.5f, DICA_PAINEL.y + 190.0f };
        pt->vel = (Vector2){ cosf(ang) * vel, sinf(ang) * vel };
        pt->vidaMax = pt->vida = 0.7f + 0.8f * Rand01();
        pt->cor = (acerto ? verdes : vermelhos)[GetRandomValue(0, 2)];
    }
}

static void DesenharParticulasDica(void) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    for (int i = 0; i < DICA_NUM_PART; i++) {
        DicaParticula *pt = &g_dicaPart[i];
        if (pt->vida <= 0.0f) continue;
        pt->vida -= dt;
        pt->vel.y += 220.0f * dt;
        pt->pos.x += pt->vel.x * dt;
        pt->pos.y += pt->vel.y * dt;
        float a = pt->vida / pt->vidaMax;
        if (a < 0.0f) a = 0.0f;
        DrawRectangleRec(UIParaTela((Rectangle){ pt->pos.x, pt->pos.y, 3, 3 }), Fade(pt->cor, a));
    }
}

static Rectangle BlocoDica(int i) {
    float x0 = DICA_PAINEL.x + 6, w = DICA_PAINEL.width - 12, y0 = DICA_PAINEL.y;
    if (i < 5) {
        float bw = (w - 4 * 4.0f) / 5.0f;
        return (Rectangle){ x0 + i * (bw + 4.0f), y0 + 192, bw, 26 };
    }
    float bw = (w - 3 * 4.0f) / 4.0f;
    return (Rectangle){ x0 + (i - 5) * (bw + 4.0f), y0 + 222, bw, 26 };
}
static Rectangle BotaoApagarDica(void) {
    return (Rectangle){ DICA_PAINEL.x + 6, DICA_PAINEL.y + 258, (DICA_PAINEL.width - 12 - 8) / 2, 28 };
}
static Rectangle BotaoEnviarDica(void) {
    Rectangle a = BotaoApagarDica();
    return (Rectangle){ a.x + a.width + 8, a.y, a.width, a.height };
}
static Rectangle BotaoComecarDica(void) {
    return (Rectangle){ DICA_PAINEL.x + 6, DICA_PAINEL.y + 258, DICA_PAINEL.width - 12, 28 };
}

static const char *BLOCO_AJUDA[9] = {
    "= primeira afirmação", "= segunda afirmação", "= NÃO (nega o que vem depois)",
    "= abre um grupo", "= fecha o grupo", "= E (as duas valem)", "= OU (pelo menos uma vale)",
    "= SE ... ENTÃO", "= SE E SOMENTE SE"
};

static void LiberarEntrevista(void) {
    g_infinitosAntes = global_infinite_tokens;
    global_infinite_tokens = true;
    g_entrevistaLiberada = true;
}

static void FinalizarDica(bool acertou, bool tempoEsgotou) {
    if (!g_dicaUsadaHoje) GameStateRegisterHintUsed();
    g_dicaUsadaHoje = true;
    g_dicaTempoEsgotado = tempoEsgotou;
    if (acertou) {
        g_dicaEstado = 1;
        global_tokens += 1;
        g_dicaShake = 0.18f; g_dicaShakeAmp = 1.5f;
        TocarSomDica(g_dicaSndAcerto, 1.0f);
    } else {
        g_dicaEstado = 2;
        g_dicaShake = 0.32f; g_dicaShakeAmp = 4.0f;
        TocarSomDica(g_dicaSndErro, 1.0f);
    }
    SoltarParticulasDica(acertou);
}

static void EnviarDica(void) {
    int tabUser = 0, tabCerta = 0;
    if (!TabelaVerdade(g_dicaFormula, g_dicaNFormula, &tabUser)) {
        g_dicaInvalidaT = 1.3f;
        TocarSomDica(g_dicaSndErro, 1.4f);
        return;
    }
    int certa[DICA_MAX_TOKENS];
    int nCerta = TokenizarFormula(PROPOSICOES[g_dicaAtual].resposta, certa, DICA_MAX_TOKENS);
    TabelaVerdade(certa, nCerta, &tabCerta);
    FinalizarDica(tabUser == tabCerta, false);
}

#define GLIFO_LADO(tamPx)  ((tamPx) * 0.62f)

static int SimboloDica(const char *c, bool converteV, char *tipo) {
    if (c[0] == '<' && c[1] == '-' && c[2] == '>') { *tipo = '='; return 3; }
    if (c[0] == '-' && c[1] == '>')                { *tipo = '>'; return 2; }
    if (c[0] == '^' || c[0] == '~' || c[0] == '(' || c[0] == ')' || (converteV && c[0] == 'v')) { *tipo = c[0]; return 1; }
    return 0;
}

static float GlifoLargura(char tipo, int tamPx) {
    float lado = GLIFO_LADO(tamPx);
    switch (tipo) {
        case '>': return lado * 1.35f;
        case '=': return lado * 1.65f;
        case '(': case ')': return lado * 0.5f;
        default:  return lado;
    }
}

static float GlifoAvanco(char tipo, int tamPx) {
    float folga = (tipo == '(' || tipo == ')') ? tamPx * 0.14f : tamPx * 0.26f;
    return GlifoLargura(tipo, tamPx) + folga;
}

static float MedirFormulaDica(const char *txt, int tamPx, bool converteV) {
    float w = 0.0f;
    char run[96]; int n = 0;
    const char *c = txt;
    for (;;) {
        char tipo = 0;
        int len = (*c == '\0') ? 0 : SimboloDica(c, converteV, &tipo);
        if (*c == '\0' || len > 0) {
            run[n] = '\0';
            if (n > 0) w += (float)MeasureText(run, tamPx);
            n = 0;
            if (*c == '\0') break;
            w += GlifoAvanco(tipo, tamPx);
            c += len;
        } else if (n < 95) {
            run[n++] = *c++;
        } else {
            c++;
        }
    }
    return w;
}

static void TracoArredondadoDica(Vector2 a, Vector2 b, float esp, Color cor) {
    DrawLineEx(a, b, esp, cor);
    DrawCircleV(a, esp * 0.5f, cor);
    DrawCircleV(b, esp * 0.5f, cor);
}

static void DesenharGlifoDica(char tipo, float x, float ys, int tamPx, Color cor) {
    float lado = GLIFO_LADO(tamPx);
    float larg = GlifoLargura(tipo, tamPx);
    float esp = fmaxf(1.6f, tamPx * 0.12f);
    float cy = ys + tamPx * 0.46f;
    float l = x + esp * 0.5f, r = x + larg - esp * 0.5f;
    float t = cy - lado * 0.5f + esp * 0.5f, b = cy + lado * 0.5f - esp * 0.5f;
    float m = (l + r) * 0.5f;
    if (tipo == '^') {
        TracoArredondadoDica((Vector2){ l, b }, (Vector2){ m, t }, esp, cor);
        TracoArredondadoDica((Vector2){ m, t }, (Vector2){ r, b }, esp, cor);
    } else if (tipo == 'v') {
        TracoArredondadoDica((Vector2){ l, t }, (Vector2){ m, b }, esp, cor);
        TracoArredondadoDica((Vector2){ m, b }, (Vector2){ r, t }, esp, cor);
    } else if (tipo == '~') {
        float yBarra = t + (b - t) * 0.25f;
        TracoArredondadoDica((Vector2){ l, yBarra }, (Vector2){ r, yBarra }, esp, cor);
        TracoArredondadoDica((Vector2){ r, yBarra }, (Vector2){ r, b }, esp, cor);
    } else if (tipo == '>' || tipo == '=') {
        float h = lado * 0.3f;
        TracoArredondadoDica((Vector2){ l, cy }, (Vector2){ r, cy }, esp, cor);
        TracoArredondadoDica((Vector2){ r - h, cy - h }, (Vector2){ r, cy }, esp, cor);
        TracoArredondadoDica((Vector2){ r - h, cy + h }, (Vector2){ r, cy }, esp, cor);
        if (tipo == '=') {
            TracoArredondadoDica((Vector2){ l + h, cy - h }, (Vector2){ l, cy }, esp, cor);
            TracoArredondadoDica((Vector2){ l + h, cy + h }, (Vector2){ l, cy }, esp, cor);
        }
    } else {
        float alt = lado * 1.15f;
        float yt = cy - alt * 0.5f;
        Vector2 ant = { 0, 0 };
        for (int i = 0; i <= 8; i++) {
            float u = (float)i / 8.0f * 2.0f - 1.0f;
            float px = (tipo == '(') ? (l + (r - l) * u * u) : (r - (r - l) * u * u);
            Vector2 pt = { px, yt + (u + 1.0f) * 0.5f * alt };
            if (i > 0) TracoArredondadoDica(ant, pt, esp, cor);
            ant = pt;
        }
    }
}

static float DesenharFormulaDicaPx(const char *txt, float xs, float ys, int tamPx, Color cor, bool converteV) {
    float x = xs;
    char run[96]; int n = 0;
    const char *c = txt;
    for (;;) {
        char tipo = 0;
        int len = (*c == '\0') ? 0 : SimboloDica(c, converteV, &tipo);
        if (*c == '\0' || len > 0) {
            run[n] = '\0';
            if (n > 0) { DrawText(run, (int)x, (int)ys, tamPx, cor); x += (float)MeasureText(run, tamPx); }
            n = 0;
            if (*c == '\0') break;
            DesenharGlifoDica(tipo, x + tamPx * 0.06f, ys, tamPx, cor);
            x += GlifoAvanco(tipo, tamPx);
            c += len;
        } else if (n < 95) {
            run[n++] = *c++;
        } else {
            c++;
        }
    }
    return x - xs;
}

static float DesenharFormulaDicaVirt(const char *txt, Vector2 pos, int tam, Color cor, bool converteV) {
    Rectangle d = UIParaTela((Rectangle){ pos.x, pos.y, 10, 10 });
    float esc = d.height / 10.0f;
    int tamPx = (int)(tam * esc); if (tamPx < 1) tamPx = 1;
    return DesenharFormulaDicaPx(txt, d.x, d.y, tamPx, cor, converteV) / esc;
}

static void DesenharFormulaDicaCentro(const char *txt, Rectangle area, int tam, Color cor, bool converteV) {
    Rectangle d = UIParaTela(area);
    float esc = d.height / area.height;
    int tamPx = (int)(tam * esc); if (tamPx < 1) tamPx = 1;
    float w = MedirFormulaDica(txt, tamPx, converteV);
    DesenharFormulaDicaPx(txt, d.x + (d.width - w) * 0.5f, d.y + (d.height - tamPx) * 0.5f, tamPx, cor, converteV);
}

static void DesenharBotaoDicaT(Rectangle rVirt, const char *rotulo, int tam, bool hover,
                               Color fundo, Color fundoHover, Color borda, Color bordaHover, Color texto) {
    DrawRectangleRec(UIParaTela((Rectangle){ rVirt.x + 2, rVirt.y + 2, rVirt.width, rVirt.height }), Fade(BLACK, 0.45f));
    DrawRectangleRec(UIParaTela(rVirt), hover ? fundoHover : fundo);
    DrawRectangleLinesEx(UIParaTela(rVirt), hover ? 2.0f : 1.5f, hover ? bordaHover : borda);
    UIDrawTextInArea(rotulo, rVirt, tam, texto);
}

static void DesenharChipProposicao(Rectangle rVirt, const char *simbolo, const char *significado) {
    Color corEscura   = {  32,  29,  31, 255 };
    Color corClaro    = { 160, 147, 142, 255 };
    Color corDestaque = { 189, 181, 168, 255 };
    Color corBranco   = { 255, 242, 230, 255 };
    DrawRectangleRec(UIParaTela(rVirt), corClaro);
    DrawRectangleLinesEx(UIParaTela(rVirt), 1.0f, corDestaque);
    Rectangle selo = { rVirt.x, rVirt.y, 28, rVirt.height };
    DrawRectangleRec(UIParaTela(selo), corEscura);
    UIDrawTextInArea(simbolo, selo, 14, corBranco);
    UIDrawTextWrappedLeftAligned(significado, (Vector2){ rVirt.x + 34, rVirt.y + (rVirt.height - 10) * 0.5f - 1 },
                                 rVirt.width - 38, 10, corEscura);
}

static void DesenharDicaConteudo(const char *suspectName) {
    Color corEscura   = {  32,  29,  31, 255 };
    Color corFundo    = { 125, 112, 113, 252 };
    Color corClaro    = { 160, 147, 142, 255 };
    Color corDestaque = { 189, 181, 168, 255 };
    Color corBranco   = { 255, 242, 230, 255 };
    Color verde       = {  52, 100,  66, 255 };
    Color verdeHover  = {  72, 130,  90, 255 };
    Color verdeClaro  = { 120, 190, 140, 255 };
    Vector2 mouse = RayCanvasGetMousePosition();

    const Proposicao *p = &PROPOSICOES[g_dicaAtual];
    float x = DICA_PAINEL.x + 6;
    float y = DICA_PAINEL.y;
    float largura = DICA_PAINEL.width - 12;

    float t = (float)(GetTime() - g_dicaTAbertura) / 0.22f;
    if (t > 1.0f) t = 1.0f;
    float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    DrawRectangleRec(UIGetTelaRect(), Fade(BLACK, 0.55f * ease));

    if (t < 1.0f) {
        float h = DICA_PAINEL.height * ease;
        Rectangle abrindo = { DICA_PAINEL.x, DICA_PAINEL.y + (DICA_PAINEL.height - h) * 0.5f, DICA_PAINEL.width, h };
        DrawRectangleRec(UIParaTela(abrindo), corFundo);
        DrawRectangleLinesEx(UIParaTela(abrindo), 2.0f, corDestaque);
        return;
    }

    DrawRectangleRec(UIParaTela((Rectangle){ DICA_PAINEL.x + 4, DICA_PAINEL.y + 4, DICA_PAINEL.width, DICA_PAINEL.height }),
                     Fade(BLACK, 0.6f));

    DrawRectangleRec(UIParaTela(DICA_PAINEL), corFundo);
    DrawRectangleLinesEx(UIParaTela(DICA_PAINEL), 2.0f, corDestaque);
    Rectangle barraTit = { DICA_PAINEL.x, DICA_PAINEL.y, DICA_PAINEL.width, 24 };
    DrawRectangleRec(UIParaTela(barraTit), corEscura);
    if (g_iconAceso.id > 0) {
        DrawTexturePro(g_iconAceso, (Rectangle){ 0, 0, (float)g_iconAceso.width, (float)g_iconAceso.height },
                       UIParaTela((Rectangle){ DICA_PAINEL.x + 4, DICA_PAINEL.y + 4, 16, 16 }),
                       (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
    UIDrawTextInArea("DICA - LÓGICA", (Rectangle){ DICA_PAINEL.x + 22, DICA_PAINEL.y, 110, 24 }, 11, corBranco);
    UIDrawCloseButton((Rectangle){ DICA_PAINEL.x + DICA_PAINEL.width - 20, DICA_PAINEL.y + 4, 16, 16 });

    if (g_dicaEstado == 0) {
        bool urgente = g_dicaTempo <= 10.0f;
        bool piscaOn = !urgente || fmodf((float)GetTime(), 0.5f) < 0.3f;
        Color corTempo = urgente ? UI_COR_NEGACAO_CLARO : verdeClaro;
        char tempoTxt[16];
        snprintf(tempoTxt, sizeof tempoTxt, "%d s", (int)ceilf(g_dicaTempo));
        if (piscaOn) UIDrawTextInArea(tempoTxt, (Rectangle){ DICA_PAINEL.x + DICA_PAINEL.width - 76, DICA_PAINEL.y, 52, 24 }, 12, corTempo);
        float frac = g_dicaTempo / DICA_TEMPO_TOTAL;
        DrawRectangleRec(UIParaTela((Rectangle){ DICA_PAINEL.x + 2, DICA_PAINEL.y + 24, DICA_PAINEL.width - 4, 4 }), corEscura);
        DrawRectangleRec(UIParaTela((Rectangle){ DICA_PAINEL.x + 2, DICA_PAINEL.y + 24, (DICA_PAINEL.width - 4) * frac, 4 }),
                         urgente ? UI_COR_NEGACAO : verdeClaro);
    }

    if (g_dicaEstado == 3) {
        if (g_dicaPagina == 0) {
            UIDrawTextLeftAligned("1/3   Entenda a frase", (Vector2){ x, y + 32 }, 11, corEscura);
            UIDrawTextWrappedLeftAligned("Toda frase tem duas partes, chamadas de p e q. Veja um exemplo:",
                                         (Vector2){ x, y + 50 }, largura, 10, corEscura);
            Rectangle ex = { x, y + 80, largura, 26 };
            DrawRectangleRec(UIParaTela(ex), corEscura);
            DrawRectangleLinesEx(UIParaTela(ex), 1.0f, corDestaque);
            UIDrawTextLeftAligned(">", (Vector2){ ex.x + 6, ex.y + 7 }, 12, UI_COR_NEGACAO_CLARO);
            UIDrawTextLeftAligned("Chove e faz frio.", (Vector2){ ex.x + 20, ex.y + 7 }, 12, corBranco);
            DesenharChipProposicao((Rectangle){ x, y + 114, largura, 24 }, "p", "chove");
            DesenharChipProposicao((Rectangle){ x, y + 142, largura, 24 }, "q", "faz frio");
            UIDrawTextLeftAligned("Escreva a frase só com símbolos:", (Vector2){ x, y + 172 }, 10, corEscura);
            {
                float esc = UIGetEscala(), xx = x, yy = y + 187;
                UIDrawTextLeftAligned("O", (Vector2){ xx, yy + 4 }, 11, corEscura);
                xx += (float)MeasureText("O", (int)(11 * esc)) / esc + 6;
                Rectangle chipE = { xx, yy, 18, 18 };
                DrawRectangleRec(UIParaTela(chipE), corEscura);
                UIDrawTextInArea("e", chipE, 12, corBranco);
                xx += 18 + 6;
                UIDrawTextLeftAligned("vira", (Vector2){ xx, yy + 4 }, 11, corEscura);
                xx += (float)MeasureText("vira", (int)(11 * esc)) / esc + 6;
                Rectangle chipS = { xx, yy, 28, 18 };
                DrawRectangleRec(UIParaTela(chipS), corEscura);
                DesenharFormulaDicaCentro("^", chipS, 14, corBranco, true);
            }
            Rectangle resp = { x, y + 210, largura, 28 };
            DrawRectangleRec(UIParaTela(resp), corEscura);
            DrawRectangleLinesEx(UIParaTela(resp), 1.0f, verdeClaro);
            DesenharFormulaDicaCentro("p ^ q", resp, 16, corBranco, true);
        } else if (g_dicaPagina == 1) {
            UIDrawTextLeftAligned("2/3   O que cada símbolo diz", (Vector2){ x, y + 32 }, 11, corEscura);
            static const char *simb[6]  = { "~", "^", "v", "->", "<->", "( )" };
            static const char *nome[6]  = { "NÃO", "E", "OU", "SE ... ENTÃO", "SE E SOMENTE SE", "AGRUPA" };
            static const char *exF[6] = { "~p", "p ^ q", "p v q", "p -> q", "p <-> q", "~(p ^ q)" };
            static const char *exM[6] = {
                "= não p",
                "= p e q",
                "= p ou q",
                "= se p então q",
                "= os dois valem ou nenhum vale",
                "= não (p e q)"
            };
            for (int i = 0; i < 6; i++) {
                Rectangle lin = { x, y + 48 + i * 31, largura, 28 };
                DrawRectangleRec(UIParaTela(lin), corClaro);
                DrawRectangleLinesEx(UIParaTela(lin), 1.0f, corDestaque);
                Rectangle selo = { lin.x, lin.y, 48, lin.height };
                DrawRectangleRec(UIParaTela(selo), corEscura);
                DesenharFormulaDicaCentro(simb[i], selo, 16, corBranco, true);
                UIDrawTextLeftAligned(nome[i], (Vector2){ lin.x + 54, lin.y + 2 }, 9, corEscura);
                float wF = DesenharFormulaDicaVirt(exF[i], (Vector2){ lin.x + 54, lin.y + 14 }, 10, corEscura, true);
                UIDrawTextLeftAligned(exM[i], (Vector2){ lin.x + 54 + wF + 5, lin.y + 15 }, 9, corEscura);
            }
        } else {
            UIDrawTextLeftAligned("3/3   O desafio", (Vector2){ x, y + 32 }, 11, corEscura);
            Rectangle relogio = { x, y + 52, largura, 46 };
            DrawRectangleRec(UIParaTela(relogio), corEscura);
            DrawRectangleLinesEx(UIParaTela(relogio), 1.0f, verdeClaro);
            UIDrawTextInArea("45 SEGUNDOS", (Rectangle){ relogio.x, relogio.y + 3, relogio.width, 24 }, 16, verdeClaro);
            UIDrawTextInArea("o tempo só começa no botão abaixo", (Rectangle){ relogio.x, relogio.y + 27, relogio.width, 16 }, 9, corBranco);
            UIDrawTextWrappedLeftAligned("- Clique nos blocos para montar a fórmula e aperte Enviar.",
                                         (Vector2){ x, y + 112 }, largura, 10, corEscura);
            UIDrawTextWrappedLeftAligned("- Passe o mouse num bloco para ver o que ele significa.",
                                         (Vector2){ x, y + 142 }, largura, 10, corEscura);
            UIDrawTextWrappedLeftAligned("- Acertou: ganha +1 Token extra.",
                                         (Vector2){ x, y + 172 }, largura, 10, verde);
            UIDrawTextWrappedLeftAligned("- Errou ou o tempo acabou: a dica só volta amanhã.",
                                         (Vector2){ x, y + 196 }, largura, 10, UI_COR_NEGACAO_FUNDO);
        }

        for (int i = 0; i < DICA_NUM_PAGINAS; i++) {
            Rectangle d = { DICA_PAINEL.x + DICA_PAINEL.width * 0.5f - 17.0f + i * 12.0f, y + 245, 9, 6 };
            DrawRectangleRec(UIParaTela(d), (i == g_dicaPagina) ? corEscura : corDestaque);
        }

        bool ultima = (g_dicaPagina == DICA_NUM_PAGINAS - 1);
        Rectangle rProx = (g_dicaPagina == 0) ? BotaoComecarDica() : BotaoEnviarDica();
        if (g_dicaPagina > 0) {
            Rectangle rV = BotaoApagarDica();
            DesenharBotaoDicaT(rV, "< Voltar", 11, CheckCollisionPointRec(mouse, UIParaTela(rV)),
                               corClaro, corDestaque, corEscura, corBranco, corEscura);
        }
        DesenharBotaoDicaT(rProx, ultima ? "Começar!" : "Próximo >", 12, CheckCollisionPointRec(mouse, UIParaTela(rProx)),
                           verde, verdeHover, verdeClaro, corBranco, corBranco);
        return;
    }

    if (g_dicaEstado == 4) {
        UIDrawTextLeftAligned("Dica Lógica", (Vector2){ x, y + 38 }, 13, corEscura);
        Rectangle card = { x, y + 66, largura, 130 };
        DrawRectangleRec(UIParaTela(card), corClaro);
        DrawRectangleLinesEx(UIParaTela(card), 1.5f, corDestaque);
        UIDrawTextInArea("Consumir dica lógica?", (Rectangle){ card.x + 8, card.y + 18, card.width - 16, 26 }, 15, corEscura);
        UIDrawTextWrappedLeftAligned(
            "Você só pode usar a Dica Lógica 1 vez por dia e terá 45 segundos para resolver o desafio.",
            (Vector2){ card.x + 12, card.y + 56 }, card.width - 24, 10, corEscura
        );
        Rectangle rN = BotaoApagarDica(), rS = BotaoEnviarDica();
        DesenharBotaoDicaT(rN, "Não", 12, CheckCollisionPointRec(mouse, UIParaTela(rN)),
                           UI_COR_NEGACAO_FUNDO, UI_COR_NEGACAO, UI_COR_NEGACAO_CLARO, corBranco, corBranco);
        DesenharBotaoDicaT(rS, "Sim, consumir", 11, CheckCollisionPointRec(mouse, UIParaTela(rS)),
                           verde, verdeHover, verdeClaro, corBranco, corBranco);
        return;
    }

    Rectangle tela = { x, y + 32, largura, 48 };
    DrawRectangleRec(UIParaTela(tela), corEscura);
    for (float ly = tela.y + 1; ly < tela.y + tela.height; ly += 2.0f) {
        DrawRectangleRec(UIParaTela((Rectangle){ tela.x, ly, tela.width, 1 }), Fade(BLACK, 0.35f));
    }
    DrawRectangleLinesEx(UIParaTela(tela), 1.0f, corDestaque);
    UIDrawTextLeftAligned(">", (Vector2){ tela.x + 6, tela.y + 6 }, 11, UI_COR_NEGACAO_CLARO);
    UIDrawTextWrappedLeftAligned(p->frase, (Vector2){ tela.x + 20, tela.y + 6 }, tela.width - 26, 11, corBranco);

    DesenharChipProposicao((Rectangle){ x, y + 86,  largura, 26 }, "p", p->p);
    DesenharChipProposicao((Rectangle){ x, y + 116, largura, 26 }, "q", p->q);

    if (g_dicaEstado == 0) {
        int blocoSob = -1;
        for (int i = 0; i < 9; i++) if (CheckCollisionPointRec(mouse, UIParaTela(BlocoDica(i)))) blocoSob = i;
        if (blocoSob < 0) {
            UIDrawTextLeftAligned("Monte a forma simbólica:", (Vector2){ x, y + 148 }, 10, corEscura);
        } else {
            float wSim = DesenharFormulaDicaVirt(TOKEN_TXT[BLOCO_TOKEN[blocoSob]], (Vector2){ x, y + 147 }, 11, corEscura, true);
            UIDrawTextLeftAligned(BLOCO_AJUDA[blocoSob], (Vector2){ x + wSim + 5, y + 148 }, 10, corEscura);
        }
        Rectangle visor = { x, y + 162, largura, 26 };
        bool invalida = g_dicaInvalidaT > 0.0f;
        DrawRectangleRec(UIParaTela(visor), corEscura);
        DrawRectangleLinesEx(UIParaTela(visor), 1.0f, invalida ? UI_COR_NEGACAO_CLARO : corDestaque);
        char txt[96];
        if (invalida) {
            snprintf(txt, sizeof txt, "Fórmula inválida!");
        } else {
            FormulaParaTexto(g_dicaFormula, g_dicaNFormula, txt, sizeof txt);
            if (fmodf((float)GetTime(), 1.0f) < 0.5f) strncat(txt, "_", sizeof txt - strlen(txt) - 1);
        }
        DesenharFormulaDicaVirt(txt, (Vector2){ visor.x + 8, visor.y + 6 }, 14, invalida ? UI_COR_NEGACAO_CLARO : corBranco, !invalida);

        for (int i = 0; i < 9; i++) {
            Rectangle r = BlocoDica(i);
            bool hover = CheckCollisionPointRec(mouse, UIParaTela(r));
            bool variavel = (BLOCO_TOKEN[i] == T_P || BLOCO_TOKEN[i] == T_Q);
            DesenharBotaoDicaT(r, "", 16, hover,
                               variavel ? corDestaque : corClaro, corBranco, corEscura, corBranco, corEscura);
            DesenharFormulaDicaCentro(TOKEN_TXT[BLOCO_TOKEN[i]], r, 16, corEscura, true);
        }
        Rectangle rA = BotaoApagarDica(), rE = BotaoEnviarDica();
        DesenharBotaoDicaT(rA, "Apagar", 12, CheckCollisionPointRec(mouse, UIParaTela(rA)),
                           UI_COR_NEGACAO_FUNDO, UI_COR_NEGACAO, UI_COR_NEGACAO_CLARO, corBranco, corBranco);
        DesenharBotaoDicaT(rE, "Enviar", 12, CheckCollisionPointRec(mouse, UIParaTela(rE)),
                           verde, verdeHover, verdeClaro, corBranco, corBranco);
    } else {
        const char *nome = (suspectName && suspectName[0]) ? suspectName : "este funcionário";
        bool acertou = (g_dicaEstado == 1);
        Rectangle badge = { x, y + 152, largura, 90 };
        Color badgeFundo = acertou ? verde : UI_COR_NEGACAO_FUNDO;
        Color badgeBorda = acertou ? verdeClaro : UI_COR_NEGACAO_CLARO;
        DrawRectangleRec(UIParaTela(badge), badgeFundo);
        DrawRectangleLinesEx(UIParaTela(badge), 1.5f, badgeBorda);

        Rectangle rIco = UIParaTela((Rectangle){ badge.x + 8, badge.y + 8, 26, 26 });
        DrawRectangleRec(rIco, corEscura);
        float cx = rIco.x + rIco.width * 0.5f, cy = rIco.y + rIco.height * 0.5f, k = rIco.width * 0.25f;
        if (acertou) {
            DrawLineEx((Vector2){ cx - k, cy }, (Vector2){ cx - k * 0.3f, cy + k * 0.8f }, 2.5f, badgeBorda);
            DrawLineEx((Vector2){ cx - k * 0.3f, cy + k * 0.8f }, (Vector2){ cx + k, cy - k * 0.8f }, 2.5f, badgeBorda);
        } else {
            DrawLineEx((Vector2){ cx - k, cy - k }, (Vector2){ cx + k, cy + k }, 2.5f, badgeBorda);
            DrawLineEx((Vector2){ cx + k, cy - k }, (Vector2){ cx - k, cy + k }, 2.5f, badgeBorda);
        }
        float tx = badge.x + 42, tw = badge.width - 50, esc = UIGetEscala();
        if (acertou) {
            char msg[160];
            snprintf(msg, sizeof msg, "Correto! Você ganhou +1 Token extra.");
            UIDrawTextWrappedLeftAligned(msg, (Vector2){ tx, badge.y + 8 }, tw, 11, corBranco);
        } else {
            float yy = badge.y + 8;
            UIDrawTextLeftAligned(g_dicaTempoEsgotado ? "Tempo esgotado!" : "Errado!", (Vector2){ tx, yy }, 12, corBranco);
            yy += 20;
            if (!g_dicaTempoEsgotado) {
                char sua[96];
                FormulaParaTexto(g_dicaFormula, g_dicaNFormula, sua, sizeof sua);
                UIDrawTextLeftAligned("Você montou:", (Vector2){ tx, yy + 2 }, 10, corBranco);
                float wl = (float)MeasureText("Você montou:", (int)(10 * esc)) / esc;
                DesenharFormulaDicaVirt(sua, (Vector2){ tx + wl + 6, yy }, 12, corBranco, true);
                yy += 20;
            }
            UIDrawTextLeftAligned("Resposta certa:", (Vector2){ tx, yy + 2 }, 10, corBranco);
            float wl = (float)MeasureText("Resposta certa:", (int)(10 * esc)) / esc;
            DesenharFormulaDicaVirt(p->resposta, (Vector2){ tx + wl + 6, yy }, 12, corBranco, true);
            UIDrawTextLeftAligned("A dica só volta amanhã.", (Vector2){ badge.x + 8, badge.y + 68 }, 10, corBranco);
        }

        DesenharBotaoDicaT(DICA_OK, "OK", 12, CheckCollisionPointRec(mouse, UIParaTela(DICA_OK)),
                           corClaro, corDestaque, corEscura, corBranco, corEscura);
    }
}

void LogicHintInit(Texture2D iconAceso) {
    g_iconAceso = iconAceso;
    g_dicaAberta = false;
}

void LogicHintResetDay(void) {
    g_dicaUsadaHoje = false;
    g_dicaAberta = false;
}

void LogicHintResetRun(void) {
    g_dicaSorteadas = 0;
    g_dicaTutorialVisto = false;
    LogicHintResetDay();
}

void LogicHintUnload(void) {
    UISafeUnloadSound(&g_dicaSndClique);
    UISafeUnloadSound(&g_dicaSndAcerto);
    UISafeUnloadSound(&g_dicaSndErro);
    g_iconAceso = (Texture2D){ 0 };
    g_dicaAberta = false;
}

void LogicHintEndInterview(void) {
    if (g_entrevistaLiberada) global_infinite_tokens = g_infinitosAntes || SettingsGetDevMode();
    g_entrevistaLiberada = false;
}

bool LogicHintIsAvailable(bool dialogLoaded) {
    return dialogLoaded && (!g_dicaUsadaHoje || SettingsGetDevMode());
}

bool LogicHintIsOpen(void) {
    return g_dicaAberta;
}

void LogicHintOpen(bool dialogLoaded, bool *tutDicaVisto) {
    if (!LogicHintIsAvailable(dialogLoaded) || g_dicaAberta) return;
    GarantirSonsDica();
    unsigned todas = (1u << NUM_PROPOSICOES) - 1u;
    if ((g_dicaSorteadas & todas) == todas) g_dicaSorteadas = 0;
    int livres[NUM_PROPOSICOES], nLivres = 0;
    for (int i = 0; i < NUM_PROPOSICOES; i++) if (!(g_dicaSorteadas & (1u << i))) livres[nLivres++] = i;
    g_dicaAtual = livres[GetRandomValue(0, nLivres - 1)];
    g_dicaSorteadas |= (1u << g_dicaAtual);
    g_dicaPagina = 0;
    g_dicaEstado = g_dicaTutorialVisto ? 4 : 3;
    g_dicaTempo = DICA_TEMPO_TOTAL;
    g_dicaTempoEsgotado = false;
    g_dicaNFormula = 0;
    g_dicaInvalidaT = 0.0f;
    g_dicaShake = 0.0f;
    memset(g_dicaPart, 0, sizeof g_dicaPart);
    g_dicaAberta = true;
    if (tutDicaVisto) *tutDicaVisto = true;
    g_dicaTAbertura = GetTime();
}

void LogicHintClose(void) {
    if (g_dicaAberta && g_dicaEstado == 0) {
        if (!g_dicaUsadaHoje) GameStateRegisterHintUsed();
        g_dicaUsadaHoje = true;
    }
    g_dicaAberta = false;
}

bool LogicHintUpdate(void) {
    if (!g_dicaAberta) return false;
    Vector2 mouse = RayCanvasGetMousePosition();
    bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool hover = false;
    float dt = GetFrameTime();
    if (g_dicaInvalidaT > 0.0f) g_dicaInvalidaT -= dt;
    if (g_dicaShake > 0.0f) g_dicaShake -= dt;
    if (g_dicaEstado == 0) {
        g_dicaTempo -= dt;
        if (g_dicaTempo <= 0.0f) { g_dicaTempo = 0.0f; FinalizarDica(false, true); }
    }

    Rectangle btnFechar = { DICA_PAINEL.x + DICA_PAINEL.width - 19, DICA_PAINEL.y + 3, 16, 16 };
    if (CheckCollisionPointRec(mouse, UIParaTela(btnFechar))) {
        hover = true;
        if (clicou) {
            LogicHintClose();
            return true;
        }
    }

    if (g_dicaEstado == 3) {
        bool ultima = (g_dicaPagina == DICA_NUM_PAGINAS - 1);
        Rectangle rProx = (g_dicaPagina == 0) ? BotaoComecarDica() : BotaoEnviarDica();
        Rectangle rVolta = BotaoApagarDica();
        bool proximo = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        bool volta = false;
        if (CheckCollisionPointRec(mouse, UIParaTela(rProx))) { hover = true; if (clicou) proximo = true; }
        if (g_dicaPagina > 0 && CheckCollisionPointRec(mouse, UIParaTela(rVolta))) { hover = true; if (clicou) volta = true; }
        if (GetTime() - g_dicaTAbertura > 0.25) {
            if (volta) {
                g_dicaPagina--;
                TocarSomDica(g_dicaSndClique, 0.8f);
            } else if (proximo && !ultima) {
                g_dicaPagina++;
                TocarSomDica(g_dicaSndClique, 1.1f);
            } else if (proximo && ultima) {
                g_dicaTutorialVisto = true;
                g_dicaEstado = 4;
                g_dicaTAbertura = GetTime();
                TocarSomDica(g_dicaSndClique, 1.2f);
            }
        }
    } else if (g_dicaEstado == 0) {
        for (int i = 0; i < 9; i++) {
            if (!CheckCollisionPointRec(mouse, UIParaTela(BlocoDica(i)))) continue;
            hover = true;
            if (clicou && g_dicaNFormula < DICA_MAX_TOKENS) {
                g_dicaFormula[g_dicaNFormula++] = BLOCO_TOKEN[i];
                g_dicaInvalidaT = 0.0f;
                TocarSomDica(g_dicaSndClique, 0.9f + 0.05f * (float)(i % 5));
            }
        }
        bool apagar = IsKeyPressed(KEY_BACKSPACE);
        bool enviar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        if (CheckCollisionPointRec(mouse, UIParaTela(BotaoApagarDica()))) { hover = true; if (clicou) apagar = true; }
        if (CheckCollisionPointRec(mouse, UIParaTela(BotaoEnviarDica()))) { hover = true; if (clicou) enviar = true; }
        if (apagar && g_dicaNFormula > 0) {
            g_dicaNFormula--;
            g_dicaInvalidaT = 0.0f;
            TocarSomDica(g_dicaSndClique, 0.7f);
        }
        if (enviar) EnviarDica();
    } else if (g_dicaEstado == 4) {
        Rectangle rSim = BotaoEnviarDica();
        Rectangle rNao = BotaoApagarDica();
        bool sim = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        bool nao = false;
        if (CheckCollisionPointRec(mouse, UIParaTela(rSim))) { hover = true; if (clicou) sim = true; }
        if (CheckCollisionPointRec(mouse, UIParaTela(rNao))) { hover = true; if (clicou) nao = true; }
        if (GetTime() - g_dicaTAbertura > 0.25) {
            if (sim) {
                g_dicaEstado = 0;
                g_dicaTempo = DICA_TEMPO_TOTAL;
                TocarSomDica(g_dicaSndClique, 1.2f);
            } else if (nao) {
                LogicHintClose();
            }
        }
    } else if (CheckCollisionPointRec(mouse, UIParaTela(DICA_OK))) {
        hover = true;
        if (clicou) LogicHintClose();
    }
    SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
    return true;
}

void LogicHintDraw(const char *suspectName) {
    if (!g_dicaAberta) return;
    bool tremendo = g_dicaShake > 0.0f;
    if (tremendo) {
        float a = g_dicaShakeAmp * (g_dicaShake / 0.3f > 1.0f ? 1.0f : g_dicaShake / 0.3f);
        float tt = (float)GetTime() * 70.0f;
        rlPushMatrix();
        rlTranslatef(sinf(tt) * a, cosf(tt * 1.3f) * a * 0.5f, 0.0f);
    }
    DesenharDicaConteudo(suspectName);
    if (tremendo) rlPopMatrix();
    DesenharParticulasDica();
}
