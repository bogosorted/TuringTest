#include "transition.h"
#include "raycanvas.h"
#include "raylib.h"
#include <stdio.h>

#define FADE_OUT_TIME  0.6f   // segundos escurecendo a cena
#define TEXT_FADE_TIME 0.5f   // segundos para o texto aparecer
#define TEXT_HOLD_TIME 3.0f   // segundos com o texto totalmente visivel

typedef enum { PHASE_NONE, PHASE_FADE_OUT, PHASE_TEXT } Phase;

static Phase g_phase = PHASE_NONE;
static float g_timer = 0.0f;
static char  g_text[64] = "";

void TransitionStart(const char *texto) {
    if (g_phase != PHASE_NONE) return;
    snprintf(g_text, sizeof g_text, "%s", texto);
    g_phase = PHASE_FADE_OUT;
    g_timer = 0.0f;
}

bool TransitionActive(void) {
    return g_phase != PHASE_NONE;
}

bool TransitionShowingText(void) {
    return g_phase == PHASE_TEXT;
}

TransitionEvent TransitionUpdate(void) {
    if (g_phase == PHASE_NONE) return TRANSITION_NONE;

    g_timer += GetFrameTime();

    if (g_phase == PHASE_FADE_OUT) {
        float alpha = g_timer / FADE_OUT_TIME;
        if (alpha > 1.0f) alpha = 1.0f;
        RayCanvasSetFade(alpha);   // o RayCanvasEnd desenha o preto por cima da cena

        if (g_timer >= FADE_OUT_TIME) {
            RayCanvasSetFade(0.0f);
            g_phase = PHASE_TEXT;
            g_timer = 0.0f;
            return TRANSITION_FADE_OUT_DONE;
        }
        return TRANSITION_NONE;
    }

    // PHASE_TEXT
    if (g_timer >= TEXT_FADE_TIME + TEXT_HOLD_TIME) {
        g_phase = PHASE_NONE;
        g_timer = 0.0f;
        return TRANSITION_FINISHED;
    }
    return TRANSITION_NONE;
}

void TransitionDrawTextScreen(void) {
    float alpha = g_timer / TEXT_FADE_TIME;
    if (alpha > 1.0f) alpha = 1.0f;

    int tamanho = GetScreenHeight() / 10;
    int largura = MeasureText(g_text, tamanho);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawText(g_text,
             (GetScreenWidth() - largura) / 2,
             (GetScreenHeight() - tamanho) / 2,
             tamanho, Fade(WHITE, alpha));
    EndDrawing();
}
