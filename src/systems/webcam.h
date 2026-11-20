#ifndef WEBCAM_H
#define WEBCAM_H

#include <stdbool.h>

// Inicializa a webcam padrao do notebook/PC (V4L2 no Linux, Media Foundation no Windows).
// Devolve true se encontrou e abriu uma camera valida.
bool WebcamOpen(void);

// Encerra a captura e libera o dispositivo da camera.
void WebcamClose(void);

// true se a camera estiver aberta e pronta para capturar quadros.
bool WebcamIsOpen(void);

// Se houver um quadro novo disponivel, preenche 'outRgb' (outW * outH * 3 bytes RGB, espelhado como espelho)
// com recorte centralizado na proporcao outW:outH e devolve true. Caso contrario, devolve false sem bloquear.
bool WebcamGrabFrameRGB(unsigned char *outRgb, int outW, int outH);

#endif
