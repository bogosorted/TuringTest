#ifndef DIALOG_SYSTEM_H
#define DIALOG_SYSTEM_H

#include <stdbool.h>
#include "game_state.h"

#define MAX_CHOICES 4
#define MAX_NODE_CHOICES 8
#define MAX_TEXT_LENGTH 512
#define MAX_ID_LENGTH 32

typedef struct {
    char text[MAX_TEXT_LENGTH];
    char next_node[MAX_ID_LENGTH];
    char req_evidencia[MAX_ID_LENGTH]; // se nao vazio, exige esta evidencia registrada
} DialogChoice;

typedef struct {
    char id[MAX_ID_LENGTH];
    char speaker[MAX_ID_LENGTH];
    char text[MAX_TEXT_LENGTH];
    int choice_count;
    DialogChoice choices[MAX_NODE_CHOICES];
    bool visited;
    bool is_leaf;
    VisaoJogador visao_out;
    char ev_desbloqueada[MAX_ID_LENGTH]; // evidencia ganha ao visitar este no
} DialogNode;

typedef struct {
    char npc_id[MAX_ID_LENGTH];
    int suspect_index; // 0=Maria, 1=Arthur, 2=Ana Lucia
    int day;
    int node_count;
    DialogNode *nodes;
    int current_node_index;
} DialogTree;

// Economia de Tokens do Jogo (12 por dia)
#define TOKENS_PER_DAY 8
extern int global_tokens;

// Modo desenvolvedor: true = tokens ilimitados (nao bloqueia e nao gasta).
// Ligado/desligado pelo botao secreto do menu (digite "treloso").
extern bool global_infinite_tokens;

// true se ainda ha tokens para gastar (ou se o modo ilimitado esta ligado)
bool HasTokens(void);

// Devolve os tokens para o valor inicial (chamar no comeco de cada dia)
void ResetTokens(void);

// API
void InitDialogSystem(void);
bool LoadDialogTree(const char *filepath, DialogTree *tree);
void FreeDialogTree(DialogTree *tree);
DialogNode* GetCurrentNode(DialogTree *tree);
bool MakeChoice(DialogTree *tree, int choice_index);
bool MakeChoiceById(DialogTree *tree, const char *next_id);

// Preenche 'out' com as escolhas que o jogador pode fazer AGORA e devolve quantas sao.
//  1) escolhas do no atual cujo destino ainda nao foi visitado (e cuja evidencia requerida foi vista);
//  2) se nao sobrar nenhuma, as escolhas do "intro"/"Entrada"/"Oi"/"start" ainda nao visitadas.
// Devolver 0 significa que essa conversa acabou (nao ha mais para onde ir).
// Nao olha os tokens: quem chama decide se o jogador pode pagar.
int GetAvailableChoices(DialogTree *tree, DialogChoice out[MAX_CHOICES]);

#endif // DIALOG_SYSTEM_H
