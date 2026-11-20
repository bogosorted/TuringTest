#ifndef SETAS_3D_H
#define SETAS_3D_H

#include "raylib.h"
#include <stdbool.h>

// Setas 3D low poly de navegacao desenhadas por cima da foto do local.
// A cena vai para uma textura com o tamanho da janela (1 pixel = 1 pixel virtual),
// com dithering na rampa da paleta. Posicoes 2D em pixels dessa textura.

typedef enum {
    SETA_CIMA = 0,
    SETA_BAIXO,
    SETA_ESQUERDA,
    SETA_DIREITA,
    NUM_SETAS
} DirecaoSeta;

void      Setas3DInit(int largura, int altura, const Vector2 posicoes[NUM_SETAS]);
void      Setas3DUnload(void);

// Chamar FORA de qualquer BeginTextureMode (antes de RayCanvasBegin).
void      Setas3DRender(const bool visiveis[NUM_SETAS], int hover, float tempo);
Texture2D Setas3DTextura(void);    // textura render (desenhar com altura negativa)

int       Setas3DEm(Vector2 ponto, const bool visiveis[NUM_SETAS]);   // seta sob o ponto ou -1
Vector2   Setas3DPos(int seta);

#endif
