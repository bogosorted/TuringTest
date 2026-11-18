#include "ranking.h"
#include <stdio.h>
#include <string.h>

#define RANKING_FILE "ranking.cfg"

static RankingEntry g_entries[RANKING_MAX_ENTRIES];
static int g_count = 0;

static void SanitizarNome(char *dst, const char *src) {
    int j = 0;
    if (src) {
        for (int i = 0; src[i] != '\0' && j < RANKING_NAME_MAX; i++) {
            unsigned char c = (unsigned char)src[i];
            if (c >= 33 && c <= 126 && c != '=') {
                dst[j++] = (char)c;
            }
        }
    }
    if (j == 0) {
        snprintf(dst, RANKING_NAME_MAX + 1, "AGENTE");
    } else {
        dst[j] = '\0';
    }
}

static void SanitizarFoto(char *dst, const char *src) {
    if (!src || src[0] == '\0' || strcmp(src, "-") == 0) {
        dst[0] = '\0';
        return;
    }
    int j = 0;
    for (int i = 0; src[i] != '\0' && j < RANKING_PHOTO_MAX - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c > 32 && c <= 126) {
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
}

void RankingLoad(void) {
    g_count = 0;
    FILE *f = fopen(RANKING_FILE, "r");
    if (!f) return;

    char linha[256];
    while (fgets(linha, sizeof linha, f) && g_count < RANKING_MAX_ENTRIES) {
        char nome[32] = { 0 };
        char foto[RANKING_PHOTO_MAX] = { 0 };
        int pontos = 0;
        int acertou = 0;
        int lidos = sscanf(linha, "%31s %d %d %95s", nome, &pontos, &acertou, foto);
        if (lidos >= 3) {
            SanitizarNome(g_entries[g_count].nome, nome);
            g_entries[g_count].pontos = pontos;
            g_entries[g_count].acertou = (acertou != 0);
            SanitizarFoto(g_entries[g_count].foto, (lidos >= 4) ? foto : NULL);
            g_count++;
        }
    }
    fclose(f);
}

void RankingSave(void) {
    FILE *f = fopen(RANKING_FILE, "w");
    if (!f) return;
    for (int i = 0; i < g_count; i++) {
        const char *fotoOut = (g_entries[i].foto[0] != '\0') ? g_entries[i].foto : "-";
        fprintf(f, "%s %d %d %s\n",
                g_entries[i].nome,
                g_entries[i].pontos,
                g_entries[i].acertou ? 1 : 0,
                fotoOut);
    }
    fclose(f);
}

int RankingAddWithPhoto(const char *nome, int pontos, bool acertou, const char *fotoPath) {
    RankingLoad(); // sincroniza caso tenha sido alterado

    RankingEntry novo;
    memset(&novo, 0, sizeof novo);
    SanitizarNome(novo.nome, nome);
    novo.pontos = pontos;
    novo.acertou = acertou;
    SanitizarFoto(novo.foto, fotoPath);

    int pos = g_count;
    for (int i = 0; i < g_count; i++) {
        if (novo.pontos > g_entries[i].pontos) {
            pos = i;
            break;
        }
    }

    if (pos >= RANKING_MAX_ENTRIES) {
        if (novo.foto[0] != '\0') remove(novo.foto);
        return -1;
    }

    // Se a lista ja estava cheia (10), remove do disco a foto do 10o colocado que caiu fora
    if (g_count == RANKING_MAX_ENTRIES && g_entries[RANKING_MAX_ENTRIES - 1].foto[0] != '\0') {
        remove(g_entries[RANKING_MAX_ENTRIES - 1].foto);
    }

    int limit = (g_count < RANKING_MAX_ENTRIES) ? g_count : (RANKING_MAX_ENTRIES - 1);
    for (int i = limit; i > pos; i--) {
        g_entries[i] = g_entries[i - 1];
    }
    g_entries[pos] = novo;
    if (g_count < RANKING_MAX_ENTRIES) g_count++;

    RankingSave();
    return pos;
}

void RankingAdd(const char *nome, int pontos, bool acertou) {
    (void)RankingAddWithPhoto(nome, pontos, acertou, NULL);
}

int RankingGetCount(void) {
    return g_count;
}

const RankingEntry *RankingGetEntry(int index) {
    if (index < 0 || index >= g_count) return NULL;
    return &g_entries[index];
}
