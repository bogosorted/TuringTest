#ifndef MURAL_UI_H
#define MURAL_UI_H

#include "raylib.h"
#include <stdbool.h>

#define MURAL_FOTO_W 128
#define MURAL_FOTO_H 144

// Inicializa recursos visuais do Mural (logo Turing Test, fontes/sons opcionais) e carrega fotos do ranking
void MuralInit(void);

// Recarrega o ranking.cfg e as texturas das 10 fotos do disco
void MuralReload(int highlightRankIndex);

// Libera texturas carregadas pelo Mural e garante que a webcam esteja fechada
void MuralUnload(void);

// Desenha e atualiza o Mural de 10 Retratos em tela cheia (640x360).
// Devolve true quando o jogador clica em "Menu Principal" (ou aperta ESC).
bool MuralUpdateDraw(float alpha, Sound sndHover);

// --- Cabine de Foto pos-pontuacao (antes de abrir o Mural no fim da run) ---

// Abre a cabine de foto (liga a webcam se disponivel)
void MuralFotoOpen(void);

// Fecha a webcam caso esteja aberta
void MuralFotoClose(void);

// Atualiza e desenha a tela de captura de foto do agente (com dither leve ao vivo + opcao de nao tirar foto).
// Quando o jogador confirma (com ou sem foto), salva automaticamente no Ranking via RankingAddWithPhoto,
// preenche *outRankIndex (0..9 ou -1) e devolve true para avancar ao Mural!
bool MuralFotoUpdateDraw(const char *nomeJogador, int pontuacao, bool acertouVeredito,
                         Sound sndHover, int *outRankIndex);

#endif
