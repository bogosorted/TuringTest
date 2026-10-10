#ifndef TRANSITION_H
#define TRANSITION_H

#include <stdbool.h>

// Transicao: fade-out da cena atual -> tela preta com texto -> proxima cena.
// A troca de cena em si e feita pelo main.c quando recebe TRANSITION_FADE_OUT_DONE.

typedef enum {
    TRANSITION_NONE,
    TRANSITION_FADE_OUT_DONE,   // o fade-out acabou: hora de trocar a cena
    TRANSITION_FINISHED         // o texto acabou: a nova cena ja pode aparecer
} TransitionEvent;

void TransitionStart(const char *texto);   // ignora se ja houver uma transicao
TransitionEvent TransitionUpdate(void);    // chamar UMA vez por frame
bool TransitionActive(void);               // true durante o fade-out e o texto
bool TransitionShowingText(void);          // true na fase da tela preta com texto
void TransitionDrawTextScreen(void);       // desenha a tela preta com o texto

#endif
