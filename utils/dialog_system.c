#include "dialog_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int global_tokens = 6;

void InitDialogSystem(void) {
    // Tokens sao globais, nao resetar aqui
}

static void TrimRight(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' ')) {
        str[len-1] = '\0';
        len--;
    }
}

bool LoadDialogTree(const char *filepath, DialogTree *tree) {
    FILE *f = fopen(filepath, "r");
    if (!f) return false;

    memset(tree, 0, sizeof(DialogTree));
    tree->nodes = (DialogNode *)calloc(50, sizeof(DialogNode));
    
    char line[512];
    DialogNode *curr = NULL;
    
    while (fgets(line, sizeof(line), f)) {
        TrimRight(line);
        if (strlen(line) == 0) continue;
        
        if (strncmp(line, "NPC: ", 5) == 0) {
            strncpy(tree->npc_id, line + 5, MAX_ID_LENGTH - 1);
        } else if (strncmp(line, "DIA: ", 5) == 0) {
            tree->day = atoi(line + 5);
        } else if (strncmp(line, "[NODE: ", 7) == 0) {
            curr = &tree->nodes[tree->node_count++];
            char *end = strchr(line + 7, ']');
            if (end) *end = '\0';
            strncpy(curr->id, line + 7, MAX_ID_LENGTH - 1);
            if (strcmp(curr->id, "start") == 0) {
                tree->current_node_index = tree->node_count - 1;
            }
        } else if (curr) {
            if (strncmp(line, "SPEAKER: ", 9) == 0) {
                strncpy(curr->speaker, line + 9, MAX_ID_LENGTH - 1);
            } else if (strncmp(line, "TEXT: ", 6) == 0) {
                strncpy(curr->text, line + 6, MAX_TEXT_LENGTH - 1);
            } else if (strncmp(line, "CHOICE: ", 8) == 0) {
                char *arrow = strstr(line, " -> ");
                if (arrow && curr->choice_count < MAX_CHOICES) {
                    *arrow = '\0';
                    int idx = curr->choice_count;
                    strncpy(curr->choices[idx].text, line + 8, MAX_TEXT_LENGTH - 1);
                    strncpy(curr->choices[idx].next_node, arrow + 4, MAX_ID_LENGTH - 1);
                    curr->choice_count++;
                }
            }
        }
    }
    
    fclose(f);
    return true;
}

void FreeDialogTree(DialogTree *tree) {
    if (tree->nodes) {
        free(tree->nodes);
        tree->nodes = NULL;
    }
    tree->node_count = 0;
}

DialogNode* GetCurrentNode(DialogTree *tree) {
    if (tree->current_node_index >= 0 && tree->current_node_index < tree->node_count) {
        return &tree->nodes[tree->current_node_index];
    }
    return NULL;
}

bool MakeChoiceById(DialogTree *tree, const char *next_id) {
    if (global_tokens <= 0) return false;
    for (int i = 0; i < tree->node_count; i++) {
        if (strcmp(tree->nodes[i].id, next_id) == 0) {
            tree->current_node_index = i;
            tree->nodes[i].visited = true;
            global_tokens--; 
            return true;
        }
    }
    return false;
}

bool MakeChoice(DialogTree *tree, int choice_index) {
    if (global_tokens <= 0) return false;
    DialogNode *curr = GetCurrentNode(tree);
    if (!curr || choice_index < 0 || choice_index >= curr->choice_count) return false;
    return MakeChoiceById(tree, curr->choices[choice_index].next_node);
}
