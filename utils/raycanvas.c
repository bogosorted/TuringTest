#include "raycanvas.h"
#include <math.h>

static int g_refWidth = 640;
static int g_refHeight = 360;
static float g_uiScale = 1.0f;

void RayCanvasInit(int referenceWidth, int referenceHeight) {
    g_refWidth = referenceWidth;
    g_refHeight = referenceHeight;
}

void RayCanvasClose(void) {
}

void RayCanvasBegin(void) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Escala uniforme suave para os elementos de interface
    g_uiScale = fminf(screenW / (float)g_refWidth, screenH / (float)g_refHeight);
    if (g_uiScale < 0.1f) g_uiScale = 0.1f;

    BeginDrawing();
    ClearBackground((Color){ 31, 31, 31, 255 });
}

void RayCanvasEnd(void) {
    EndDrawing();
}

void RayCanvasDraw(void) {
}

void RayCanvasDrawTiledBackground(Texture2D texture) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // O background usa exclusivamente escala inteira para proteger o dithering
    float bgScale = floorf(screenH / (float)texture.height);
    if (bgScale < 1.0f) bgScale = 1.0f;

    float tileW = (float)texture.width * bgScale;
    float tileH = (float)texture.height * bgScale;

    // Alinhado pelo centro da tela
    float centerOffset = fmodf((screenW - tileW) * 0.5f, tileW);
    float startX = centerOffset;
    while (startX > 0.0f) {
        startX -= tileW;
    }

    // Sobreposição de 1px entre blocos (tileW + 1.0f) para eliminar a linha preta de subpixel na rasterização
    for (float x = startX; x < screenW; x += tileW) {
        Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
        Rectangle dst = { floorf(x), 0.0f, tileW + 1.0f, screenH };
        DrawTexturePro(texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }
}

void RayCanvasDrawTexture(Texture2D texture, Rectangle dest, Color tint) {
    Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    DrawTexturePro(texture, src, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

Rectangle RayCanvasGetRect(Vector2 anchor, Vector2 pivot, Vector2 offset, Vector2 baseSize) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float scaledWidth = baseSize.x * g_uiScale;
    float scaledHeight = baseSize.y * g_uiScale;
    float scaledOffsetX = offset.x * g_uiScale;
    float scaledOffsetY = offset.y * g_uiScale;

    float x = (screenW * anchor.x) + scaledOffsetX - (scaledWidth * pivot.x);
    float y = (screenH * anchor.y) + scaledOffsetY - (scaledHeight * pivot.y);

    return (Rectangle){ x, y, scaledWidth, scaledHeight };
}

Vector2 RayCanvasGetMousePosition(void) {
    return GetMousePosition();
}

float RayCanvasGetUIScale(void) {
    return g_uiScale;
}
