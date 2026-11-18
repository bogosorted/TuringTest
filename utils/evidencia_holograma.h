#ifndef EVIDENCIA_HOLOGRAMA_H
#define EVIDENCIA_HOLOGRAMA_H

#include "raylib.h"
#include <stdbool.h>

// Tipos de hologramas 3D interativos (estilo inspecao de artefatos do Resident Evil 4)
typedef enum {
    HOLO_EV_NENHUM = 0,
    HOLO_EV_CANETA,         // (1, EV-02) Caneta preta com emblema de ave/abutre
    HOLO_EV_RELATORIOS,     // (1, EV-03) Pilha de pastas e relatorios pendentes
    HOLO_EV_CRACHA,         // (1, EV-04) Cracha de acesso RASEC com cordao e mosquetao
    HOLO_EV_MEMORANDO,      // (1, EV-06 / EV-07) Prancheta de memorando + carimbo
    HOLO_EV_RELATORIO_ERRO, // (2, EV-02) Relatorio com dados inconsistentes / alerta
    HOLO_EV_RELOGIO_TEMPO,  // (2, EV-03) Calendario / relogio de tempo de casa contraditorio
    HOLO_EV_LEITOR_SETOR,   // (2, EV-04) Leitor biometrico/cartao de setor restrito
    HOLO_EV_GRAVADOR        // (0, EV-01 / 2, EV-01) Gravador de voz confidencial
} TipoHoloEvidencia;

typedef struct {
    float yaw;          // rotacao horizontal em radianos
    float pitch;        // inclinacao vertical em radianos
    float roll;         // inclinacao lateral sutil em radianos
    float zoom;         // fator de zoom (1.0 = padrao, ate ~2.6 = close-up macro)
    float velYaw;       // inercia suave ao soltar o arraste
    float velPitch;
    Vector2 panOffset;  // deslocamento fino do alvo da camera (botao direito)
    bool  interagiu;    // true assim que o jogador girou/deu zoom manualmente
} EstadoCameraHoloEv;

void              EvidenciaHoloInit(void);
void              EvidenciaHoloUnload(void);

// Mapeia (suspeito, ev_id) para o modelo 3D correspondente
TipoHoloEvidencia EvidenciaHoloTipoPorId(int suspeito, const char *ev_id);

// Reseta a camera 3D para o angulo heroico inicial de cada artefato
void              EvidenciaHoloResetarCamera(EstadoCameraHoloEv *cam, TipoHoloEvidencia tipo);

// Renderiza o artefato 3D com iluminacao direcional + rim light + shader de holograma dithered.
// IMPORTANTE: chamar FORA de RayCanvasBegin() (antes de iniciar o render target principal).
void              EvidenciaHoloRender(int largura, int altura, TipoHoloEvidencia tipo,
                                      const EstadoCameraHoloEv *cam, float tempo);

// Textura final pos-shader (desenhar com altura negativa no source rect)
Texture2D         EvidenciaHoloTextura(void);

#endif
