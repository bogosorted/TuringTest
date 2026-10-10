#ifndef RAYCANVAS_H
#define RAYCANVAS_H

#include "raylib.h"
#include <stdbool.h>

void RayCanvasInit(int referenceWidth, int referenceHeight);
void RayCanvasClose(void);
void RayCanvasBegin(void);
void RayCanvasEnd(void);
void RayCanvasDraw(void);
void RayCanvasDrawTiledBackground(Texture2D texture);
void RayCanvasDrawTexture(Texture2D texture, Rectangle dest, Color tint);

Rectangle RayCanvasGetRect(Vector2 anchor, Vector2 pivot, Vector2 offset, Vector2 size);
Vector2 RayCanvasGetMousePosition(void);
float RayCanvasGetUIScale(void);

// Fade: 0.0 = sem efeito, 1.0 = tela toda preta.
// Desenhado automaticamente no RayCanvasEnd, por cima de tudo.
void RayCanvasSetFade(float alpha);

// true enquanto ha fade em andamento. As cenas devem ignorar cliques nesse periodo.
bool RayCanvasInputBlocked(void);

#endif
