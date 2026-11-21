#ifndef MAPA_HOLOGRAMA_H
#define MAPA_HOLOGRAMA_H

#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>

// Mapa 3D do predio da RASEC estilo holograma (low poly + dithering na rampa da paleta).
// Os andares ficam empilhados como no predio de verdade; cada sala e feita de 1 ou 2
// retangulos (para ter salas em L), os corredores sao so decoracao e a escadaria aparece
// em degraus, ligada por um poco entre os andares. O local atual ganha uma bolinha vermelha.
// A cena e desenhada numa textura em que 1 pixel = 1 pixel virtual da interface,
// entao todas as posicoes 2D abaixo estao em pixels dessa textura.

#define HOLO_NUM_ANDARES 5

typedef struct {
    float x, z, w, d;           // retangulo na planta do andar
} HoloRet;

typedef struct {
    int     andar;              // 0 = terreo, 1 = 1o andar, 2 = 2o andar
    int     numPartes;          // 1 (retangular) ou 2 (em L)
    HoloRet partes[2];
    bool    escada;             // desenha degraus em vez de paredes
} HoloSala;

typedef struct {
    int     andar;
    HoloRet r;
} HoloCorredor;

void      MapaHoloInit(const HoloSala *salas, int numSalas, const HoloCorredor *corredores, int numCorredores, bool modo3D);
void      MapaHoloUnload(void);

// Configura filtro de exibicao: apenas o andar atual (zoom amplo na planta) ou predio inteiro,
// alem da mascara de salas que possuem suspeitos para exibir marcadores amarelos.
void      MapaHoloSetFiltro(bool apenasAndarAtual, int andarFoco, uint64_t salasComSuspeitos);

// Desenha a cena numa textura do tamanho pedido (recriada se o tamanho mudar).
// "acessiveis" tem um bit por sala: salas fora dele aparecem apagadas.
// Chamar FORA de qualquer BeginTextureMode (ou seja, antes de RayCanvasBegin).
void      MapaHoloRender(int largura, int altura, int salaAtual, int salaHover, uint64_t acessiveis, float tempo);
Texture2D MapaHoloTextura(void);   // textura render (desenhar com altura negativa)
int       MapaHoloNumAndares(void);

int       MapaHoloSalaEm(Vector2 ponto);   // sala sob o ponto (pixels da textura) ou -1
Vector2   MapaHoloPosSala(int sala);       // centro do piso da sala, para o rotulo
Vector2   MapaHoloPosAndar(int andar);     // canto esquerdo da frente da placa (rotulo do andar logo abaixo)

#endif
