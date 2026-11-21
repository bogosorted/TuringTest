#include "options_ui.h"
#include "settings.h"
#include "game_state.h"
#include "raycanvas.h"
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define TELA_W 640
#define TELA_H 360

enum { HOVER_NENHUM = -1, HOVER_ANT = 0, HOVER_PROX = 1, HOVER_TELA_CHEIA = 2, HOVER_TUTORIAL = 3, HOVER_DEV_BTN = 4, HOVER_CRT = 5, HOVER_CAM3D = 6, HOVER_DEV_AUTORESET = 7, HOVER_DEV_TECHNIGHT = 8, HOVER_DEV_RESTART = 9 };

static int   g_sliderArrastando = -1;   // -1 = nenhum, 0 = musica, 1 = efeitos
static int   g_hoverAnt = HOVER_NENHUM; // botao que estava com o mouse em cima no frame anterior
static int   g_devClicks = 0;           // cliques consecutivos no botaozinho escondido (3 abre a senha)
static bool  g_devModalAberto = false;  // modal pedindo a senha "RED"
static char  g_devSenhaInput[12] = "";
static float g_devErroTimer = 0.0f;

void OptionsUiReset(void) {
    g_sliderArrastando = -1;
    g_hoverAnt = HOVER_NENHUM;
    g_devClicks = 0;
    g_devModalAberto = false;
    g_devSenhaInput[0] = '\0';
    g_devErroTimer = 0.0f;
}

bool OptionsUiArrastando(void) {
    return g_sliderArrastando >= 0;
}

bool OptionsUiModalAberto(void) {
    return g_devModalAberto;
}

void OptionsUiFecharModal(void) {
    g_devModalAberto = false;
    g_devClicks = 0;
    g_devSenhaInput[0] = '\0';
    g_devErroTimer = 0.0f;
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
    float yExtras    = yTelaCheia + lay->passoY;
    float yPaleta    = yExtras + lay->passoY;

    bool interativoControles = interativo && !g_devModalAberto;
    bool algumHover = false;
    if (Slider(0, nomes[0], yMusica,  trilhoX, trilhoW, lay, est, a, interativoControles, tocarHover)) algumHover = true;
    if (Slider(1, nomes[1], yEfeitos, trilhoX, trilhoW, lay, est, a, interativoControles, tocarHover)) algumHover = true;

    bool hoverSeta = false;

    // Tela cheia: [x] marcada = tela cheia, desmarcada = modo janela
    Texto(est, nomes[2], lay->x, yTelaCheia, est->nome, a, 0);
    bool telaCheia = SettingsGetFullscreen();
    float larguraTelaCheia = MeasureTextEx(est->fonte, nomes[2], est->tamFonte, est->espacamento).x;
    float caixaX = roundf(lay->x + larguraTelaCheia + lay->gapNome);
    if (caixaX < trilhoX) caixaX = trilhoX;
    if (Caixinha(telaCheia, caixaX, yTelaCheia, larguraTelaCheia, HOVER_TELA_CHEIA, lay, est, a, interativoControles, &hoverSeta, tocarHover)) {
        SettingsSetFullscreen(!telaCheia);
        SettingsApplyWindowMode();
        SettingsSave();
    }
    Texto(est, SettingsGetFullscreen() ? "Ligada" : "Desligada", caixaX + 24.0f, yTelaCheia,
          est->valor, a, 0);

    // Tutorial: [x] no canto direito da mesma linha (padrao ligado)
    {
        const char *lblTut = "Tutorial";
        float tutCaixaX = lay->direita - 16.0f;
        float wTut = MeasureTextEx(est->fonte, lblTut, est->tamFonte, est->espacamento).x;
        float tutAreaX = tutCaixaX - 8.0f - wTut;
        Rectangle tutCaixa = { tutCaixaX, yTelaCheia - 8.0f, 16.0f, 16.0f };
        Rectangle tutArea  = { tutAreaX, yTelaCheia - 10.0f, lay->direita - tutAreaX, 20.0f };
        bool tutHov = interativoControles && CheckCollisionPointRec(MouseVirtual(), tutArea);
        if (tutHov) {
            hoverSeta = true;
            if (g_hoverAnt != HOVER_TUTORIAL && tocarHover) tocarHover();
            g_hoverAnt = HOVER_TUTORIAL;
        }
        Texto(est, lblTut, tutCaixaX - 8.0f, yTelaCheia, est->nome, a, 2);
        DrawRectangleRec(ParaTela(tutCaixa), ComAlpha(tutHov ? est->setaBordaHover : est->setaBorda, a));
        DrawRectangleRec(ParaTela((Rectangle){ tutCaixa.x + 1, tutCaixa.y + 1, tutCaixa.width - 2, tutCaixa.height - 2 }),
                         ComAlpha(tutHov ? est->setaFundoHover : est->setaFundo, a));
        if (SettingsGetTutorial()) {
            DrawRectangleRec(ParaTela((Rectangle){ tutCaixa.x + 4, tutCaixa.y + 4, tutCaixa.width - 8, tutCaixa.height - 8 }),
                             ComAlpha(tutHov ? est->preenchidoAtivo : est->preenchido, a));
        }
        if (tutHov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            bool novo = !SettingsGetTutorial();
            SettingsSetTutorial(novo);
            GameStateSetTutorialConfig(novo);
            SettingsSave();
        }
    }

    // CRT (TV de tubo) [x] na esquerda e Camera 3D Fixa [x] na direita
    {
        const char *lblCrt = "CRT (TV)";
        Texto(est, lblCrt, lay->x, yExtras, est->nome, a, 0);
        bool crt = SettingsGetCRT();
        float wCrt = MeasureTextEx(est->fonte, lblCrt, est->tamFonte, est->espacamento).x;
        float crtCaixaX = roundf(lay->x + wCrt + lay->gapNome);
        if (crtCaixaX < trilhoX) crtCaixaX = trilhoX;
        if (Caixinha(crt, crtCaixaX, yExtras, wCrt, HOVER_CRT, lay, est, a, interativoControles, &hoverSeta, tocarHover)) {
            SettingsSetCRT(!crt);
            SettingsSave();
        }
        Texto(est, SettingsGetCRT() ? "ON" : "OFF", crtCaixaX + 22.0f, yExtras, est->valor, a, 0);

        const char *lblCam = "Câm.3D Fixa";
        float camCaixaX = lay->direita - 16.0f;
        float wCam = MeasureTextEx(est->fonte, lblCam, est->tamFonte, est->espacamento).x;
        float camAreaX = camCaixaX - 8.0f - wCam;
        Rectangle camCaixa = { camCaixaX, yExtras - 8.0f, 16.0f, 16.0f };
        Rectangle camArea  = { camAreaX, yExtras - 10.0f, lay->direita - camAreaX, 20.0f };
        bool camHov = interativoControles && CheckCollisionPointRec(MouseVirtual(), camArea);
        if (camHov) {
            hoverSeta = true;
            if (g_hoverAnt != HOVER_CAM3D && tocarHover) tocarHover();
            g_hoverAnt = HOVER_CAM3D;
        }
        Texto(est, lblCam, camCaixaX - 8.0f, yExtras, est->nome, a, 2);
        DrawRectangleRec(ParaTela(camCaixa), ComAlpha(camHov ? est->setaBordaHover : est->setaBorda, a));
        DrawRectangleRec(ParaTela((Rectangle){ camCaixa.x + 1, camCaixa.y + 1, camCaixa.width - 2, camCaixa.height - 2 }),
                         ComAlpha(camHov ? est->setaFundoHover : est->setaFundo, a));
        if (SettingsGetCamera3DFixa()) {
            DrawRectangleRec(ParaTela((Rectangle){ camCaixa.x + 4, camCaixa.y + 4, camCaixa.width - 8, camCaixa.height - 8 }),
                             ComAlpha(camHov ? est->preenchidoAtivo : est->preenchido, a));
        }
        if (camHov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SettingsSetCamera3DFixa(!SettingsGetCamera3DFixa());
            SettingsSave();
        }
    }

    // Paleta: [<]  Nome  [>]  3/8
    Texto(est, nomes[3], lay->x, yPaleta, est->nome, a, 0);
    if (Seta(true, trilhoX, yPaleta, HOVER_ANT, est, a, interativoControles, &hoverSeta, tocarHover)) {
        SettingsSetPalette(SettingsGetPalette() - 1);
        SettingsSave();
    }
    if (Seta(false, trilhoX + trilhoW - 18.0f, yPaleta, HOVER_PROX, est, a, interativoControles, &hoverSeta, tocarHover)) {
        SettingsSetPalette(SettingsGetPalette() + 1);
        SettingsSave();
    }

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

    // Opcoes exclusivas do Modo Dev (abaixo das amostras de paleta)
    if (SettingsGetDevMode()) {
        float yDevRow = yAmostra + 13.0f;
        float xDev = lay->x;
        bool autoR = SettingsGetAutoReset();
        if (Caixinha(autoR, xDev, yDevRow, 98.0f, HOVER_DEV_AUTORESET, lay, est, a, interativoControles, &hoverSeta, tocarHover)) {
            SettingsSetAutoReset(!autoR);
            SettingsSave();
        }
        Texto(est, "Auto-Reset", xDev + 20.0f, yDevRow, est->valor, a, 0);

        float xTech = xDev + 104.0f;
        bool techN = SettingsGetTechNight();
        if (Caixinha(techN, xTech, yDevRow, 96.0f, HOVER_DEV_TECHNIGHT, lay, est, a, interativoControles, &hoverSeta, tocarHover)) {
            SettingsSetTechNight(!techN);
            SettingsSave();
        }
        Texto(est, "Tech Night", xTech + 20.0f, yDevRow, est->valor, a, 0);

        Rectangle rAppRestart = { lay->direita - 86.0f, yDevRow - 1.0f, 86.0f, 15.0f };
        bool hovRestart = interativoControles && CheckCollisionPointRec(MouseVirtual(), rAppRestart);
        if (hovRestart) {
            hoverSeta = true;
            if (g_hoverAnt != HOVER_DEV_RESTART && tocarHover) tocarHover();
            g_hoverAnt = HOVER_DEV_RESTART;
        }
        Rectangle rAppTela = ParaTela(rAppRestart);
        float escR = rAppTela.height / rAppRestart.height;
        DrawRectangleRec(rAppTela, ComAlpha(hovRestart ? (Color){ 165, 62, 64, 255 } : (Color){ 112, 42, 44, 255 }, a));
        DrawRectangleLinesEx(rAppTela, 1.1f * escR, ComAlpha((Color){ 255, 242, 230, 255 }, a));
        int fzR = (int)(8.0f * escR);
        if (fzR < 1) fzR = 1;
        const char *lblR = "REINICIAR APP";
        int twR = MeasureText(lblR, fzR);
        DrawText(lblR, (int)(rAppTela.x + (rAppTela.width - twR) * 0.5f),
                 (int)(rAppTela.y + (rAppTela.height - fzR) * 0.5f), fzR, ComAlpha((Color){ 255, 242, 230, 255 }, a));
        if (hovRestart && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SettingsSave();
            SettingsRequestAppRestart();
        }
    }

    // Botaozinho escondido de Dev Tools no canto inferior direito das configuracoes:
    // - Quando desligado: um pequeno botao discreto que ao ser clicado 3 vezes pede a senha "RED".
    // - Quando ligado: mostra "[DEV: ON]" para desativar em 1 clique.
    {
        bool devLigado = SettingsGetDevMode();
        Rectangle rDevBtn = devLigado
            ? (Rectangle){ lay->direita - 56.0f, yAmostra - 2.0f, 56.0f, 13.0f }
            : (Rectangle){ lay->direita - 13.0f, yAmostra - 1.0f, 13.0f, 11.0f };
        bool hovDev = interativoControles && CheckCollisionPointRec(MouseVirtual(), rDevBtn);
        if (hovDev) {
            hoverSeta = true;
            if (g_hoverAnt != HOVER_DEV_BTN && tocarHover) tocarHover();
            g_hoverAnt = HOVER_DEV_BTN;
        }

        Rectangle rDevTela = ParaTela(rDevBtn);
        float esc = rDevTela.height / rDevBtn.height;
        if (devLigado) {
            Color cFundo = hovDev ? (Color){ 196, 72, 72, 255 } : (Color){ 138, 48, 50, 255 };
            DrawRectangleRec(rDevTela, ComAlpha(cFundo, a));
            DrawRectangleLinesEx(rDevTela, 1.2f * esc, ComAlpha((Color){ 255, 242, 230, 255 }, a));
            int fz = (int)(8.0f * esc);
            if (fz < 1) fz = 1;
            const char *lbl = "DEV: ON";
            int tw = MeasureText(lbl, fz);
            DrawText(lbl, (int)(rDevTela.x + (rDevTela.width - tw) * 0.5f),
                     (int)(rDevTela.y + (rDevTela.height - fz) * 0.5f), fz, ComAlpha((Color){ 255, 242, 230, 255 }, a));
            if (hovDev && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                SettingsSetDevMode(false);
                SettingsSave();
                g_devClicks = 0;
            }
        } else {
            // Discreto nas configuracoes (parece um pequeno parafuso/terminal de manutencao)
            Color cBorda = hovDev ? est->setaBordaHover : Fade(est->trilhoBorda, 0.55f);
            Color cMiolo = (g_devClicks > 0) ? Fade((Color){ 196, 72, 72, 255 }, 0.35f + 0.25f * g_devClicks)
                                             : Fade(est->setaFundo, 0.55f);
            DrawRectangleRec(rDevTela, ComAlpha(cMiolo, a));
            DrawRectangleLinesEx(rDevTela, 1.0f * esc, ComAlpha(cBorda, a));
            Rectangle rPonto = ParaTela((Rectangle){ rDevBtn.x + 5.0f, rDevBtn.y + 4.0f, 3.0f, 3.0f });
            DrawRectangleRec(rPonto, ComAlpha(cBorda, a));

            if (interativoControles && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hovDev) {
                    g_devClicks++;
                    if (g_devClicks >= 3) {
                        g_devClicks = 0;
                        g_devModalAberto = true;
                        g_devSenhaInput[0] = '\0';
                        g_devErroTimer = 0.0f;
                        while (GetCharPressed() != 0) { } // limpa buffer de teclado
                    }
                } else {
                    g_devClicks = 0; // exige 3 cliques especificos nele
                }
            }
        }
    }

    if (hoverSeta) algumHover = true;
    else g_hoverAnt = HOVER_NENHUM;

    // Modal de senha "RED" para ativar Dev Tools
    if (g_devModalAberto) {
        if (g_devErroTimer > 0.0f) {
            g_devErroTimer -= GetFrameTime();
            if (g_devErroTimer < 0.0f) g_devErroTimer = 0.0f;
        }

        int ch;
        while ((ch = GetCharPressed()) != 0) {
            size_t len = strlen(g_devSenhaInput);
            if (ch >= 33 && ch <= 126 && len < 8) {
                g_devSenhaInput[len] = (char)toupper((unsigned char)ch);
                g_devSenhaInput[len + 1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            size_t len = strlen(g_devSenhaInput);
            if (len > 0) g_devSenhaInput[len - 1] = '\0';
        }

        float mw = 216.0f, mh = 96.0f;
        float mx = lay->x + ((lay->direita - lay->x) - mw) * 0.5f;
        float my = yMusica + 8.0f;
        Rectangle box = { mx, my, mw, mh };
        Rectangle btnCancelar = { mx + 12.0f, my + mh - 24.0f, 90.0f, 18.0f };
        Rectangle btnOk       = { mx + mw - 102.0f, my + mh - 24.0f, 90.0f, 18.0f };

        Vector2 mv = MouseVirtual();
        bool hovCanc = interativo && CheckCollisionPointRec(mv, btnCancelar);
        bool hovOk   = interativo && CheckCollisionPointRec(mv, btnOk);
        if (hovCanc || hovOk) algumHover = true;

        bool confirmar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                         (hovOk && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        bool cancelar  = IsKeyPressed(KEY_ESCAPE) ||
                         (hovCanc && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

        if (cancelar) {
            OptionsUiFecharModal();
        } else if (confirmar) {
            if (strcmp(g_devSenhaInput, "RED") == 0) {
                SettingsSetDevMode(true);
                SettingsSave();
                OptionsUiFecharModal();
            } else {
                g_devErroTimer = 1.4f;
                g_devSenhaInput[0] = '\0';
            }
        }

        if (g_devModalAberto) {
            Rectangle rSombra = ParaTela((Rectangle){ box.x + 3.0f, box.y + 3.0f, box.width, box.height });
            Rectangle rBox    = ParaTela(box);
            float esc = rBox.height / box.height;
            DrawRectangleRec(rSombra, Fade(BLACK, 0.65f));
            DrawRectangleRec(rBox, (Color){ 32, 29, 31, 252 });
            DrawRectangleLinesEx(rBox, 2.0f * esc, (Color){ 255, 242, 230, 255 });

            Rectangle rHeader = ParaTela((Rectangle){ box.x, box.y, box.width, 18.0f });
            DrawRectangleRec(rHeader, (Color){ 138, 48, 50, 255 });
            int fTitle = (int)(9.0f * esc);
            if (fTitle < 1) fTitle = 1;
            const char *tit = "DEV TOOLS - SENHA";
            DrawText(tit, (int)(rBox.x + (rBox.width - MeasureText(tit, fTitle)) * 0.5f),
                     (int)(rHeader.y + (rHeader.height - fTitle) * 0.5f), fTitle, (Color){ 255, 242, 230, 255 });

            int fSub = (int)(8.0f * esc);
            if (fSub < 1) fSub = 1;
            const char *msg = (g_devErroTimer > 0.0f) ? "Senha incorreta!" : "Digite a senha de acesso:";
            Color corMsg = (g_devErroTimer > 0.0f) ? (Color){ 224, 108, 104, 255 } : (Color){ 189, 181, 168, 255 };
            DrawText(msg, (int)(rBox.x + (rBox.width - MeasureText(msg, fSub)) * 0.5f),
                     (int)(rBox.y + 23.0f * esc), fSub, corMsg);

            Rectangle campo = { box.x + 38.0f, box.y + 38.0f, box.width - 76.0f, 22.0f };
            Rectangle rCampo = ParaTela(campo);
            DrawRectangleRec(rCampo, (Color){ 18, 16, 18, 255 });
            DrawRectangleLinesEx(rCampo, 1.5f * esc, (g_devErroTimer > 0.0f) ? (Color){ 224, 108, 104, 255 } : (Color){ 189, 181, 168, 255 });
            const char *cursor = (((int)(GetTime() * 2.5f) % 2) == 0) ? "_" : "";
            const char *mostra = TextFormat("%s%s", g_devSenhaInput, cursor);
            int fInp = (int)(11.0f * esc);
            if (fInp < 1) fInp = 1;
            DrawText(mostra, (int)(rCampo.x + (rCampo.width - MeasureText(mostra, fInp)) * 0.5f),
                     (int)(rCampo.y + (rCampo.height - fInp) * 0.5f), fInp, (Color){ 255, 242, 230, 255 });

            Rectangle rCanc = ParaTela(btnCancelar);
            Rectangle rOk   = ParaTela(btnOk);
            DrawRectangleRec(rCanc, hovCanc ? (Color){ 160, 147, 142, 255 } : (Color){ 125, 112, 113, 255 });
            DrawRectangleLinesEx(rCanc, 1.2f * esc, (Color){ 255, 242, 230, 255 });
            DrawText("Cancelar", (int)(rCanc.x + (rCanc.width - MeasureText("Cancelar", fSub)) * 0.5f),
                     (int)(rCanc.y + (rCanc.height - fSub) * 0.5f), fSub, (Color){ 32, 29, 31, 255 });

            DrawRectangleRec(rOk, hovOk ? (Color){ 196, 72, 72, 255 } : (Color){ 138, 48, 50, 255 });
            DrawRectangleLinesEx(rOk, 1.2f * esc, (Color){ 255, 242, 230, 255 });
            DrawText("Ativar", (int)(rOk.x + (rOk.width - MeasureText("Ativar", fSub)) * 0.5f),
                     (int)(rOk.y + (rOk.height - fSub) * 0.5f), fSub, (Color){ 255, 242, 230, 255 });
        }
    }

    return algumHover || g_sliderArrastando >= 0 || g_devModalAberto;
}
