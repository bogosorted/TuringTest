#include "transition.h"
#include "raycanvas.h"
#include "raylib.h"
#include <stdio.h>

#define FADE_OUT_TIME      0.55f  // segundos escurecendo a cena
#define TEXT_FADE_IN_TIME  0.25f  // segundos para o texto aparecer
#define TEXT_HOLD_TIME     0.80f  // segundos com o texto visivel (total da tela de texto = 1.3s)
#define TEXT_FADE_OUT_TIME 0.25f  // segundos para o texto sumir suavemente
#define FADE_IN_TIME       0.55f  // segundos clareando a nova cena

typedef enum { PHASE_NONE, PHASE_FADE_OUT, PHASE_TEXT, PHASE_FADE_IN } Phase;

static Phase g_phase = PHASE_NONE;
static float g_timer = 0.0f;
static char  g_text[64] = "";
static bool  g_showText = true;   // false = transicao so com fade-out -> fade-in

static float SmoothStep(float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void TransitionStart(const char *texto) {
    if (g_phase != PHASE_NONE) return;
    snprintf(g_text, sizeof g_text, "%s", texto ? texto : "");
    g_showText = (g_text[0] != '\0');
    g_phase = PHASE_FADE_OUT;
    g_timer = 0.0f;
}

void TransitionStartFadeIn(void) {
    g_phase = PHASE_FADE_IN;
    g_timer = 0.0f;
    RayCanvasSetFade(1.0f);
}

bool TransitionActive(void) {
    return g_phase != PHASE_NONE;
}

bool TransitionShowingText(void) {
    return g_phase == PHASE_TEXT;
}

TransitionEvent TransitionUpdate(void) {
    if (g_phase == PHASE_NONE) return TRANSITION_NONE;

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;   // evita pular o fade caso o carregamento de texturas demore num frame
    g_timer += dt;

    if (g_phase == PHASE_FADE_OUT) {
        float alpha = SmoothStep(g_timer / FADE_OUT_TIME);
        RayCanvasSetFade(alpha);   // o RayCanvasEnd desenha o preto por cima da cena

        if (g_timer >= FADE_OUT_TIME) {
            g_timer = 0.0f;
            if (g_showText) {
                RayCanvasSetFade(1.0f);
                g_phase = PHASE_TEXT;
            } else {
                RayCanvasSetFade(1.0f);
                g_phase = PHASE_FADE_IN;
            }
            return TRANSITION_FADE_OUT_DONE;
        }
        return TRANSITION_NONE;
    }

    if (g_phase == PHASE_TEXT) {
        float totalTextTime = TEXT_FADE_IN_TIME + TEXT_HOLD_TIME + TEXT_FADE_OUT_TIME; // 1.3s
        if (g_timer >= totalTextTime) {
            g_phase = PHASE_FADE_IN;
            g_timer = 0.0f;
            RayCanvasSetFade(1.0f);
        }
        return TRANSITION_NONE;
    }

    // PHASE_FADE_IN: clareia suavemente a cena nova
    if (g_phase == PHASE_FADE_IN) {
        float alpha = 1.0f - SmoothStep(g_timer / FADE_IN_TIME);
        RayCanvasSetFade(alpha);

        if (g_timer >= FADE_IN_TIME) {
            RayCanvasSetFade(0.0f);
            g_phase = PHASE_NONE;
            g_timer = 0.0f;
            return TRANSITION_FINISHED;
        }
        return TRANSITION_NONE;
    }

    return TRANSITION_NONE;
}

void TransitionDrawTextScreen(void) {
    float alpha = 1.0f;
    if (g_timer < TEXT_FADE_IN_TIME) {
        alpha = SmoothStep(g_timer / TEXT_FADE_IN_TIME);
    } else if (g_timer > TEXT_FADE_IN_TIME + TEXT_HOLD_TIME) {
        float outElapsed = g_timer - (TEXT_FADE_IN_TIME + TEXT_HOLD_TIME);
        alpha = 1.0f - SmoothStep(outElapsed / TEXT_FADE_OUT_TIME);
    }

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int tamanho = screenH / 10;
    int largura = MeasureText(g_text, tamanho);
    int tx = (screenW - largura) / 2;
    int ty = (screenH - tamanho) / 2;

    Color corFundo    = (Color){  32,  29,  31, 255 };
    Color corLinha    = Fade((Color){ 125, 112, 113, 255 }, alpha);
    Color corDetalhe  = Fade((Color){ 160, 147, 142, 255 }, alpha);
    Color corTexto    = Fade((Color){ 189, 181, 168, 255 }, alpha);

    RayCanvasBeginFrame();
    ClearBackground(corFundo);

    // Linhas horizontais decorativas da paleta acima e abaixo do titulo do dia
    int lineW = largura + screenW / 8;
    int lineX = (screenW - lineW) / 2;
    DrawRectangle(lineX, ty - 18, lineW, 2, corLinha);
    DrawRectangle(lineX, ty + tamanho + 16, lineW, 2, corLinha);
    DrawRectangle(lineX + lineW / 2 - 24, ty + tamanho + 15, 48, 4, corDetalhe);

    DrawText(g_text, tx + 2, ty + 3, tamanho, Fade(BLACK, 0.45f * alpha));
    DrawText(g_text, tx, ty, tamanho, corTexto);
    RayCanvasEndFrame();
}
