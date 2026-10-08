#ifndef DIALOG_SYSTEM_H
#define DIALOG_SYSTEM_H

#include <stdbool.h>

#define MAX_CHOICES 3
#define MAX_TEXT_LENGTH 256
#define MAX_ID_LENGTH 32

typedef struct {
    char text[MAX_TEXT_LENGTH];
    char next_node[MAX_ID_LENGTH];
} DialogChoice;

typedef struct {
    char id[MAX_ID_LENGTH];
    char speaker[MAX_ID_LENGTH];
    char text[MAX_TEXT_LENGTH];
    int choice_count;
    DialogChoice choices[MAX_CHOICES];
    bool visited;
} DialogNode;

typedef struct {
    char npc_id[MAX_ID_LENGTH];
    int day;
    int node_count;
    DialogNode *nodes;
    int current_node_index;
} DialogTree;

// Economia de Tokens do Jogo (6 Diários)
extern int global_tokens;

// API
void InitDialogSystem(void);
bool LoadDialogTree(const char *filepath, DialogTree *tree);
void FreeDialogTree(DialogTree *tree);
DialogNode* GetCurrentNode(DialogTree *tree);
bool MakeChoice(DialogTree *tree, int choice_index);
bool MakeChoiceById(DialogTree *tree, const char *next_id);

#endif // DIALOG_SYSTEM_H
