#ifndef RANKING_H
#define RANKING_H

#include <stdbool.h>

#define RANKING_MAX_ENTRIES 10
#define RANKING_NAME_MAX    8
#define RANKING_PHOTO_MAX   96

typedef struct {
    char nome[RANKING_NAME_MAX + 1];
    int  pontos;
    bool acertou;
    char foto[RANKING_PHOTO_MAX]; // caminho local da foto ditherizada, ou "" quando escolheu nao tirar foto
} RankingEntry;

void RankingLoad(void);
void RankingSave(void);
void RankingAdd(const char *nome, int pontos, bool acertou);
// Insere com caminho opcional de foto (NULL ou "" = sem foto / mural quebrado) e devolve a posicao (0..9), ou -1 se fora do Top 10
int  RankingAddWithPhoto(const char *nome, int pontos, bool acertou, const char *fotoPath);
int  RankingGetCount(void);
const RankingEntry *RankingGetEntry(int index);

#endif
