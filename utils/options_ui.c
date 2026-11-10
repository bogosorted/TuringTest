#include "options_ui.h"
#include "settings.h"
#include "raycanvas.h"
#include <math.h>
#include <stddef.h>

#define TELA_W 640
#define TELA_H 360

enum { HOVER_NENHUM = -1, HOVER_ANT = 0, HOVER_PROX = 1, HOVER_TELA_CHEIA = 2 };

static int g_sliderArrastando = -1;   // -1 = nenhum, 0 = musica, 1 = efeitos
static int g_hoverAnt = HOVER_NENHUM; // botao que estava com o mouse em cima no frame anterior

void OptionsUiReset(void) {
    g_sliderArrastando = -1;
    g_hoverAnt = HOVER_NENHUM;
}

bool OptionsUiArrastando(void) {
    return g_sliderArrastando >= 0;
}

// Mesma conversao das cenas: tela virtual 640x360 -> pixels reais da janela
static Rectangle TelaRect(void) {
    return RayCanvasGetRect((Vector2){ 0.5f, 0.5f }, (Vector2){ 0.5f, 0.5f },
                            (Vector2){ 0.0f, 0.0f }, (Vector2){ (float)TELA_W, (float)TELA_H });
}

static Rectangle ParaTela(Rectangle r) {
    Rectangle t = TelaRect();
    float ex = t.width / (float)TELA_W;
    float ey = t.height / (float)TELA_H;
    return (Rectangle){ t.x + r.x * ex, t.y + r.y * ey, r.width * ex, r.height * ey };
}

static Vector2 MouseVirtual(void) {
    Rectangle t = TelaRect();
    Vector2 m = RayCanvasGetMousePosition();
    return (Vector2){ (m.x - t.x) * (float)TELA_W / t.width, (m.y - t.y) * (float)TELA_H / t.height };
}

static Color ComAlpha(Color c, float a) {
    return Fade(c, (c.a / 255.0f) * a);
}

// Texto centralizado na vertical em 'yCentro'. alinhamento: 0 = esquerda, 1 = centro, 2 = direita
static void Texto(const OptionsUiEstilo *est, const char *txt, float x, float yCentro, Color cor, float alpha, int alinhamento) {
    Rectangle r = ParaTela((Rectangle){ x, yCentro - est->tamFonte / 2.0f, 0, est->tamFonte });
    float esc = r.height / est->tamFonte;
    float spacing = est->espacamento * esc;
    Vector2 med = MeasureTextEx(est->fonte, txt, r.height, spacing);
    float tx = r.x;
    if (alinhamento == 1) tx -= med.x / 2.0f;
    else if (alinhamento == 2) tx -= med.x;
    tx = roundf(tx);
    float ty = roundf(r.y);
    if (est->sombraTexto) {
        DrawTextEx(est->fonte, txt, (Vector2){ tx + esc, ty + esc }, r.height, spacing, ComAlpha(Fade(BLACK, 0.5f), alpha));
    }
    DrawTextEx(est->fonte, txt, (Vector2){ tx, ty }, r.height, spacing, ComAlpha(cor, alpha));
}

static bool Slider(int idx, const char *nome, float yCentro, float trilhoX, float trilhoW,
                   const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                   float a, bool interativo, void (*tocarHover)(void)) {
    float valor = (idx == 0) ? SettingsGetMusicVolume() : SettingsGetSfxVolume();

    Rectangle area = { trilhoX - 6.0f, yCentro - 10.0f, trilhoW + 12.0f, 20.0f };
    Vector2 mv = MouseVirtual();
    bool hover = interativo && CheckCollisionPointRec(mv, area);

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g_sliderArrastando = idx;

    if (g_sliderArrastando == idx) {
        if (interativo && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float v = roundf(((mv.x - trilhoX) / trilhoW) * 20.0f) / 20.0f;   // passos de 5%
            if (idx == 0) SettingsSetMusicVolume(v); else SettingsSetSfxVolume(v);
            valor = (idx == 0) ? SettingsGetMusicVolume() : SettingsGetSfxVolume();
        } else {
            g_sliderArrastando = -1;
            if (idx == 1 && tocarHover) tocarHover();   // amostra do novo volume dos efeitos
            SettingsSave();
        }
    }

    bool ativo = hover || g_sliderArrastando == idx;
    Color cheio = ativo ? est->preenchidoAtivo : est->preenchido;

    Texto(est, nome, lay->x, yCentro, est->nome, a, 0);

    // Trilho, parte preenchida e marcadores de 25%
    DrawRectangleRec(ParaTela((Rectangle){ trilhoX - 1, yCentro - 4, trilhoW + 2, 8 }), ComAlpha(est->trilhoBorda, a));
    DrawRectangleRec(ParaTela((Rectangle){ trilhoX, yCentro - 3, trilhoW, 6 }), ComAlpha(est->trilhoFundo, a));
    DrawRectangleRec(ParaTela((Rectangle){ trilhoX, yCentro - 3, trilhoW * valor, 6 }), ComAlpha(cheio, a));
    for (int m = 1; m < 4; m++) {
        DrawRectangleRec(ParaTela((Rectangle){ trilhoX + trilhoW * (m / 4.0f), yCentro + 5, 1, 3 }), ComAlpha(est->trilhoBorda, a));
    }

    // Puxador
    float kx = trilhoX + trilhoW * valor;
    DrawRectangleRec(ParaTela((Rectangle){ kx - 5, yCentro - 9, 10, 18 }), ComAlpha(est->puxadorBorda, a));
    DrawRectangleRec(ParaTela((Rectangle){ kx - 4, yCentro - 8, 8, 16 }), ComAlpha(cheio, a));

    Texto(est, TextFormat("%d%%", (int)roundf(valor * 100.0f)), lay->direita, yCentro,
          ativo ? est->valorAtivo : est->valor, a, 2);
    return hover;
}

// Botaozinho quadrado "<" ou ">". Devolve true se foi clicado.
static bool Seta(bool esquerda, float x, float yCentro, int idHover, const OptionsUiEstilo *est,
                 float a, bool interativo, bool *algumHover, void (*tocarHover)(void)) {
    Rectangle r = { x, yCentro - 9.0f, 18.0f, 18.0f };
    bool hover = interativo && CheckCollisionPointRec(MouseVirtual(), r);
    if (hover) {
        *algumHover = true;
        if (g_hoverAnt != idHover && tocarHover) tocarHover();
        g_hoverAnt = idHover;
    }
    DrawRectangleRec(ParaTela(r), ComAlpha(hover ? est->setaBordaHover : est->setaBorda, a));
    DrawRectangleRec(ParaTela((Rectangle){ r.x + 1, r.y + 1, r.width - 2, r.height - 2 }),
                     ComAlpha(hover ? est->setaFundoHover : est->setaFundo, a));

    // Setinha desenhada como triangulo (nao depende da fonte ter os glifos < >)
    float cx = x + 9.0f, cy = yCentro;
    Vector2 p1, p2, p3;
    if (esquerda) { p1 = (Vector2){ cx - 3, cy }; p2 = (Vector2){ cx + 2, cy + 4 }; p3 = (Vector2){ cx + 2, cy - 4 }; }
    else          { p1 = (Vector2){ cx + 3, cy }; p2 = (Vector2){ cx - 2, cy - 4 }; p3 = (Vector2){ cx - 2, cy + 4 }; }
    Rectangle t = TelaRect();
    float ex = t.width / (float)TELA_W, ey = t.height / (float)TELA_H;
    p1 = (Vector2){ t.x + p1.x * ex, t.y + p1.y * ey };
    p2 = (Vector2){ t.x + p2.x * ex, t.y + p2.y * ey };
    p3 = (Vector2){ t.x + p3.x * ex, t.y + p3.y * ey };
    DrawTriangle(p1, p2, p3, ComAlpha(est->setaTexto, a));

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Caixinha de marcar. Devolve true se foi clicada (o nome ao lado tambem conta).
static bool Caixinha(bool marcada, float x, float yCentro, float larguraNome, int idHover,
                     const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                     float a, bool interativo, bool *algumHover, void (*tocarHover)(void)) {
    Rectangle caixa = { x, yCentro - 8.0f, 16.0f, 16.0f };
    Rectangle area  = { lay->x, yCentro - 10.0f, (x + 16.0f) - lay->x, 20.0f };   // nome + caixa
    (void)larguraNome;
    bool hover = interativo && CheckCollisionPointRec(MouseVirtual(), area);
    if (hover) {
        *algumHover = true;
        if (g_hoverAnt != idHover && tocarHover) tocarHover();
        g_hoverAnt = idHover;
    }

    DrawRectangleRec(ParaTela(caixa), ComAlpha(hover ? est->setaBordaHover : est->setaBorda, a));
    DrawRectangleRec(ParaTela((Rectangle){ caixa.x + 1, caixa.y + 1, caixa.width - 2, caixa.height - 2 }),
                     ComAlpha(hover ? est->setaFundoHover : est->setaFundo, a));
    if (marcada) {
        DrawRectangleRec(ParaTela((Rectangle){ caixa.x + 4, caixa.y + 4, caixa.width - 8, caixa.height - 8 }),
                         ComAlpha(hover ? est->preenchidoAtivo : est->preenchido, a));
    }
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool OptionsUiUpdateDraw(const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                         float a, bool interativo, void (*tocarHover)(void)) {
    const char *nomes[4] = { "Música", "Efeitos", "Tela cheia", "Paleta" };

    // A coluna dos controles comeca logo depois do nome mais largo.
    // "Tela cheia" fica de fora: a caixinha vai logo depois do proprio nome,
    // para nao afastar as barrinhas de "Musica" e "Efeitos".
    float maiorNome = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (i == 2) continue;
        float w = MeasureTextEx(est->fonte, nomes[i], est->tamFonte, est->espacamento).x;
        if (w > maiorNome) maiorNome = w;
    }
    float trilhoX = roundf(lay->x + maiorNome + lay->gapNome);
    float trilhoW = lay->direita - lay->larguraValor - trilhoX;

    float yMusica    = lay->yPrimeira;
    float yEfeitos   = yMusica + lay->passoY;
    float yTelaCheia = yEfeitos + lay->passoY;
    float yPaleta    = yTelaCheia + lay->passoY;

    bool algumHover = false;
    if (Slider(0, nomes[0], yMusica,  trilhoX, trilhoW, lay, est, a, interativo, tocarHover)) algumHover = true;
    if (Slider(1, nomes[1], yEfeitos, trilhoX, trilhoW, lay, est, a, interativo, tocarHover)) algumHover = true;

    bool hoverSeta = false;

    // Tela cheia: [x] marcada = tela cheia, desmarcada = modo janela
    Texto(est, nomes[2], lay->x, yTelaCheia, est->nome, a, 0);
    bool telaCheia = SettingsGetFullscreen();
    float larguraTelaCheia = MeasureTextEx(est->fonte, nomes[2], est->tamFonte, est->espacamento).x;
    float caixaX = roundf(lay->x + larguraTelaCheia + lay->gapNome);
    if (caixaX < trilhoX) caixaX = trilhoX;
    if (Caixinha(telaCheia, caixaX, yTelaCheia, larguraTelaCheia, HOVER_TELA_CHEIA, lay, est, a, interativo, &hoverSeta, tocarHover)) {
        SettingsSetFullscreen(!telaCheia);
        SettingsApplyWindowMode();
        SettingsSave();
    }
    Texto(est, SettingsGetFullscreen() ? "Ligada" : "Desligada", caixaX + 24.0f, yTelaCheia,
          est->valor, a, 0);

    // Paleta: [<]  Nome  [>]  3/8
    Texto(est, nomes[3], lay->x, yPaleta, est->nome, a, 0);
    if (Seta(true, trilhoX, yPaleta, HOVER_ANT, est, a, interativo, &hoverSeta, tocarHover)) {
        SettingsSetPalette(SettingsGetPalette() - 1);
        SettingsSave();
    }
    if (Seta(false, trilhoX + trilhoW - 18.0f, yPaleta, HOVER_PROX, est, a, interativo, &hoverSeta, tocarHover)) {
        SettingsSetPalette(SettingsGetPalette() + 1);
        SettingsSave();
    }
    if (hoverSeta) algumHover = true;
    else g_hoverAnt = HOVER_NENHUM;

    Texto(est, SettingsPaletteName(SettingsGetPalette()), trilhoX + trilhoW / 2.0f, yPaleta, est->nomePaleta, a, 1);
    Texto(est, TextFormat("%d/%d", SettingsGetPalette() + 1, SettingsPaletteCount()), lay->direita, yPaleta, est->valor, a, 2);

    // Amostras: desenhamos os tons ORIGINAIS de proposito. O pos-processamento
    // (raycanvas.c) troca cada um pelo tom da paleta escolhida, entao o que
    // aparece na tela e exatamente a paleta atual.
    const Color *tons = SettingsOriginalStops();
    float larguraAmostra = 16.0f, gap = 4.0f;
    float total = PALETTE_NUM_STOPS * larguraAmostra + (PALETTE_NUM_STOPS - 1) * gap;
    float x0 = trilhoX + (trilhoW - total) / 2.0f;
    float yAmostra = yPaleta + 16.0f;
    for (int i = 0; i < PALETTE_NUM_STOPS; i++) {
        Rectangle q = { x0 + i * (larguraAmostra + gap), yAmostra, larguraAmostra, 9.0f };
        DrawRectangleRec(ParaTela((Rectangle){ q.x - 1, q.y - 1, q.width + 2, q.height + 2 }), ComAlpha(est->amostraBorda, a));
        DrawRectangleRec(ParaTela(q), ComAlpha(tons[i], a));
    }

    return algumHover || g_sliderArrastando >= 0;
}
