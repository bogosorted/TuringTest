#ifndef RANKING_H
#define RANKING_H

#include <stdbool.h>

#define RANKING_MAX_ENTRIES 10
#define RANKING_NAME_MAX    8

typedef struct {
    char nome[RANKING_NAME_MAX + 1];
    int  pontos;
    bool acertou;
} RankingEntry;

void RankingLoad(void);
void RankingSave(void);
void RankingAdd(const char *nome, int pontos, bool acertou);
int  RankingGetCount(void);
const RankingEntry *RankingGetEntry(int index);

#endif
