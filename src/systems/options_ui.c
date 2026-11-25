#include "options_ui.h"
#include "settings.h"
#include "game_state.h"
#include "raycanvas.h"
#include "cutscene_3d.h"
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define TELA_W 640
#define TELA_H 360

enum {
    HOVER_NENHUM = -1,
    HOVER_ANT = 0,
    HOVER_PROX = 1,
    HOVER_TELA_CHEIA = 2,
    HOVER_TUTORIAL = 3,
    HOVER_DEV_BTN = 4,
    HOVER_CRT = 5,
    HOVER_CAM3D = 6,
    HOVER_BALAO = 7,
    HOVER_DEV_AUTORESET = 8,
    HOVER_DEV_TECHNIGHT = 9,
    HOVER_DEV_RESTART = 10,
    HOVER_DEV_SUBSOLO = 12,
    HOVER_DEV_ANDARES = 13,
    HOVER_DEV_FP = 14,
    HOVER_DEV_BACK = 15,
    HOVER_DEV_DISABLE = 16,
    HOVER_DEV_NO2D = 17,
    HOVER_DEV_NO3D = 18,
    HOVER_SETA_GUIANDO = 19,
    HOVER_DEV_EXP_PAL = 20,
    HOVER_DEV_CUTSCENE = 21
};

static int   g_sliderArrastando = -1;   // -1 = nenhum, 0 = musica, 1 = efeitos
static int   g_hoverAnt = HOVER_NENHUM; // botao que estava com o mouse em cima no frame anterior
static int   g_devClicks = 0;           // cliques consecutivos no botaozinho escondido (3 abre a senha)
static bool  g_devModalAberto = false;  // modal pedindo a senha "RED"
static bool  g_devPaginaAberta = false; // pagina dedicada de configuracoes exclusivas do Modo Dev
static char  g_devSenhaInput[12] = "";
static float g_devErroTimer = 0.0f;

void OptionsUiReset(void) {
    g_sliderArrastando = -1;
    g_hoverAnt = HOVER_NENHUM;
    g_devClicks = 0;
    g_devModalAberto = false;
    g_devPaginaAberta = false;
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

// Checkbox com hitbox exata na propria area (usado na pagina dedicada do Modo Dev)
static bool DevCheckbox(bool marcada, float x, float yCentro, float larguraArea, const char *rotulo,
                        int idHover, const OptionsUiEstilo *est, float a, bool interativo,
                        bool *algumHover, void (*tocarHover)(void)) {
    Rectangle area = { x, yCentro - 9.0f, larguraArea, 18.0f };
    bool hover = interativo && CheckCollisionPointRec(MouseVirtual(), area);
    if (hover) {
        *algumHover = true;
        if (g_hoverAnt != idHover && tocarHover) tocarHover();
        g_hoverAnt = idHover;
    }
    Rectangle box = { x, yCentro - 7.0f, 14.0f, 14.0f };
    DrawRectangleRec(ParaTela(box), ComAlpha(hover ? est->setaBordaHover : est->setaBorda, a));
    DrawRectangleRec(ParaTela((Rectangle){ box.x + 1, box.y + 1, box.width - 2, box.height - 2 }),
                     ComAlpha(hover ? est->setaFundoHover : est->setaFundo, a));
    if (marcada) {
        DrawRectangleRec(ParaTela((Rectangle){ box.x + 3.5f, box.y + 3.5f, box.width - 7.0f, box.height - 7.0f }),
                         ComAlpha(hover ? est->preenchidoAtivo : est->preenchido, a));
    }
    Texto(est, rotulo, x + 20.0f, yCentro, hover ? est->valorAtivo : est->valor, a, 0);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Botao retangular simples na pagina dedicada do Modo Dev
static bool DevActionButton(Rectangle r, const char *lbl, Color fundo, Color fundoHov, Color borda,
                            int idHover, float a, bool interativo, bool *algumHover, void (*tocarHover)(void)) {
    bool hover = interativo && CheckCollisionPointRec(MouseVirtual(), r);
    if (hover) {
        *algumHover = true;
        if (g_hoverAnt != idHover && tocarHover) tocarHover();
        g_hoverAnt = idHover;
    }
    Rectangle rt = ParaTela(r);
    float esc = rt.height / r.height;
    DrawRectangleRec(rt, ComAlpha(hover ? fundoHov : fundo, a));
    DrawRectangleLinesEx(rt, (hover ? 1.5f : 1.1f) * esc, ComAlpha(borda, a));
    int fz = (int)(8.0f * esc);
    if (fz < 1) fz = 1;
    int tw = MeasureText(lbl, fz);
    DrawText(lbl, (int)(rt.x + (rt.width - tw) * 0.5f),
             (int)(rt.y + (rt.height - fz) * 0.5f), fz, ComAlpha((Color){ 255, 242, 230, 255 }, a));
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool DesenharPaginaDev(const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                              float a, bool interativo, void (*tocarHover)(void)) {
    bool algumHover = false;
    float xL = lay->x;
    float wTot = lay->direita - lay->x;
    float colW = (wTot - 12.0f) * 0.5f;
    float xR = xL + colW + 12.0f;

    float yHeader = lay->yPrimeira - 4.0f;
    Texto(est, "MODO DEV — CONFIGURAÇÕES", xL, yHeader, est->nomePaleta, a, 0);

    Rectangle rVoltar = { lay->direita - 64.0f, yHeader - 8.0f, 64.0f, 16.0f };
    if (DevActionButton(rVoltar, "< VOLTAR", (Color){ 64, 58, 60, 255 }, (Color){ 96, 86, 88, 255 },
                        (Color){ 255, 242, 230, 255 }, HOVER_DEV_BACK, a, interativo, &algumHover, tocarHover)) {
        g_devPaginaAberta = false;
    }

    // Linha divisoria sob o cabecalho
    DrawRectangleRec(ParaTela((Rectangle){ xL, yHeader + 12.0f, wTot, 1.0f }), ComAlpha(est->trilhoBorda, a));

    float y1 = lay->yPrimeira + lay->passoY * 0.78f;
    float y2 = y1 + lay->passoY * 0.72f;
    float y3 = y2 + lay->passoY * 0.72f;
    float y4 = y3 + lay->passoY * 0.72f;
    float y5 = y4 + lay->passoY * 0.72f;
    float y6 = y5 + lay->passoY * 0.95f;

    if (DevCheckbox(SettingsGetAutoReset(), xL, y1, colW, "Auto-Reset",
                    HOVER_DEV_AUTORESET, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetAutoReset(!SettingsGetAutoReset());
        SettingsSave();
    }
    if (DevCheckbox(SettingsGetTechNight(), xR, y1, colW, "Tech Night (08h-17h)",
                    HOVER_DEV_TECHNIGHT, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetTechNight(!SettingsGetTechNight());
        SettingsSave();
    }

    if (DevCheckbox(SettingsGetDevUnlockSubsolo(), xL, y2, colW, "Subsolo liberado",
                    HOVER_DEV_SUBSOLO, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevUnlockSubsolo(!SettingsGetDevUnlockSubsolo());
    }
    if (DevCheckbox(SettingsGetDevUnlockUpperFloors(), xR, y2, colW, "Andares 2-4 liberados",
                    HOVER_DEV_ANDARES, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevUnlockUpperFloors(!SettingsGetDevUnlockUpperFloors());
    }

    if (DevCheckbox(SettingsGetDevFirstPerson(), xL, y3, colW, "1ª Pessoa (F1)",
                    HOVER_DEV_FP, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevFirstPerson(!SettingsGetDevFirstPerson());
    }
    Rectangle rCutscene = { xR, y3 - 8.0f, colW, 17.0f };
    if (DevActionButton(rCutscene, "CUTSCENE 3D", (Color){ 64, 58, 60, 255 }, (Color){ 96, 86, 88, 255 },
                        (Color){ 255, 242, 230, 255 }, HOVER_DEV_CUTSCENE, a, interativo, &algumHover, tocarHover)) {
        Cutscene3DStart();
    }

    if (DevCheckbox(SettingsGetDevDisable2D(), xL, y4, colW, "Desativar Modo 2D (Menu)",
                    HOVER_DEV_NO2D, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevDisable2D(!SettingsGetDevDisable2D());
        SettingsSave();
    }
    if (DevCheckbox(SettingsGetDevDisable3D(), xR, y4, colW, "Desativar Modo 3D (Menu)",
                    HOVER_DEV_NO3D, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevDisable3D(!SettingsGetDevDisable3D());
        SettingsSave();
    }

    if (DevCheckbox(SettingsGetDevExperimentalPalettes(), xL, y5, wTot, "Paletas Experimentais (Indexadas)",
                    HOVER_DEV_EXP_PAL, est, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevExperimentalPalettes(!SettingsGetDevExperimentalPalettes());
        SettingsSave();
    }

    Rectangle rDesligar = { xL, y6 - 8.0f, 114.0f, 17.0f };
    if (DevActionButton(rDesligar, "DESATIVAR MODO DEV", (Color){ 90, 46, 48, 255 }, (Color){ 142, 56, 58, 255 },
                        (Color){ 255, 242, 230, 255 }, HOVER_DEV_DISABLE, a, interativo, &algumHover, tocarHover)) {
        SettingsSetDevMode(false);
        SettingsSave();
        g_devPaginaAberta = false;
        g_devClicks = 0;
    }

    Rectangle rRestart = { lay->direita - 98.0f, y6 - 8.0f, 98.0f, 17.0f };
    if (DevActionButton(rRestart, "REINICIAR APP", (Color){ 112, 42, 44, 255 }, (Color){ 165, 62, 64, 255 },
                        (Color){ 255, 242, 230, 255 }, HOVER_DEV_RESTART, a, interativo, &algumHover, tocarHover)) {
        SettingsSave();
        SettingsRequestAppRestart();
    }

    if (!algumHover) g_hoverAnt = HOVER_NENHUM;
    return algumHover;
}

bool OptionsUiUpdateDraw(const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                         float a, bool interativo, void (*tocarHover)(void)) {
    if (g_devPaginaAberta && SettingsGetDevMode()) {
        return DesenharPaginaDev(lay, est, a, interativo, tocarHover);
    }
    if (!SettingsGetDevMode()) g_devPaginaAberta = false;
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
    float yBalao     = yExtras + lay->passoY;
    float yPaleta    = yBalao + lay->passoY;

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

    // Balao [E/Enter] perto do player ao aproximar de interagiveis no modo 3D
    {
        const char *lblBalao = "Balão [E/Enter]";
        Texto(est, lblBalao, lay->x, yBalao, est->nome, a, 0);
        bool balao = SettingsGetBalaoInteracao();
        float wBalao = MeasureTextEx(est->fonte, lblBalao, est->tamFonte, est->espacamento).x;
        float balaoCaixaX = roundf(lay->x + wBalao + lay->gapNome);
        if (balaoCaixaX < trilhoX) balaoCaixaX = trilhoX;
        if (Caixinha(balao, balaoCaixaX, yBalao, wBalao, HOVER_BALAO, lay, est, a, interativoControles, &hoverSeta, tocarHover)) {
            SettingsSetBalaoInteracao(!balao);
            SettingsSave();
        }
        Texto(est, SettingsGetBalaoInteracao() ? "Ligado" : "Desligado", balaoCaixaX + 24.0f, yBalao, est->valor, a, 0);

        const char *lblGuia = "Seta Guiando";
        float guiaCaixaX = lay->direita - 16.0f;
        float wGuia = MeasureTextEx(est->fonte, lblGuia, est->tamFonte, est->espacamento).x;
        float guiaAreaX = guiaCaixaX - 8.0f - wGuia;
        Rectangle guiaCaixa = { guiaCaixaX, yBalao - 8.0f, 16.0f, 16.0f };
        Rectangle guiaArea  = { guiaAreaX, yBalao - 10.0f, lay->direita - guiaAreaX, 20.0f };
        bool guiaHov = interativoControles && CheckCollisionPointRec(MouseVirtual(), guiaArea);
        if (guiaHov) {
            hoverSeta = true;
            if (g_hoverAnt != HOVER_SETA_GUIANDO && tocarHover) tocarHover();
            g_hoverAnt = HOVER_SETA_GUIANDO;
        }
        Texto(est, lblGuia, guiaCaixaX - 8.0f, yBalao, est->nome, a, 2);
        DrawRectangleRec(ParaTela(guiaCaixa), ComAlpha(guiaHov ? est->setaBordaHover : est->setaBorda, a));
        DrawRectangleRec(ParaTela((Rectangle){ guiaCaixa.x + 1, guiaCaixa.y + 1, guiaCaixa.width - 2, guiaCaixa.height - 2 }),
                         ComAlpha(guiaHov ? est->setaFundoHover : est->setaFundo, a));
        if (SettingsGetSetaGuiando()) {
            DrawRectangleRec(ParaTela((Rectangle){ guiaCaixa.x + 4, guiaCaixa.y + 4, guiaCaixa.width - 8, guiaCaixa.height - 8 }),
                             ComAlpha(guiaHov ? est->preenchidoAtivo : est->preenchido, a));
        }
        if (guiaHov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SettingsSetSetaGuiando(!SettingsGetSetaGuiando());
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
    Texto(est, TextFormat("%d/%d", SettingsUnlockedPaletteOrder(SettingsGetPalette()) + 1, SettingsUnlockedPaletteCount()), lay->direita, yPaleta, est->valor, a, 2);

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

    // Botao de acesso ao Modo Dev no canto inferior direito das configuracoes:
    // - Quando desligado: um pequeno botao discreto que ao ser clicado 3 vezes pede a senha "RED".
    // - Quando ligado: mostra "MODO DEV >>" para abrir a pagina dedicada de configuracoes Dev.
    {
        bool devLigado = SettingsGetDevMode();
        Rectangle rDevBtn = devLigado
            ? (Rectangle){ lay->direita - 84.0f, yAmostra - 3.0f, 84.0f, 15.0f }
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
            const char *lbl = "MODO DEV >>";
            int tw = MeasureText(lbl, fz);
            DrawText(lbl, (int)(rDevTela.x + (rDevTela.width - tw) * 0.5f),
                     (int)(rDevTela.y + (rDevTela.height - fz) * 0.5f), fz, ComAlpha((Color){ 255, 242, 230, 255 }, a));
            if (hovDev && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g_devPaginaAberta = true;
                g_hoverAnt = HOVER_NENHUM;
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
