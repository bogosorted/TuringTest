#ifndef CUTSCENE_3D_H
#define CUTSCENE_3D_H

#include <stdbool.h>

// Inicializa/abre a cutscene 3D em primeira pessoa (modo Dev).
void Cutscene3DStart(void);
void Cutscene3DStop(void);
bool Cutscene3DIsActive(void);

// Atualiza logica, audio e renderiza a cena 3D com dithering para textura interna.
// Chamar ANTES de RayCanvasBegin().
void Cutscene3DUpdateAndRender(float dt);

// Desenha a textura 3D na tela e o dialogo destacado por cima.
// Chamar DENTRO de RayCanvasBegin() ... RayCanvasEnd().
void Cutscene3DDrawOverlay(void);

// Libera texturas, shader e som sintetizado.
void Cutscene3DUnload(void);

#endif // CUTSCENE_3D_H
