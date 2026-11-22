#include "dialog_system.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int global_tokens = TOKENS_PER_DAY;
bool global_infinite_tokens = false;

bool HasTokens(void) {
    return global_infinite_tokens || SettingsGetDevMode() || global_tokens > 0;
}

void ResetTokens(void) {
    global_tokens = TOKENS_PER_DAY;
}

void AddDailyTokens(void) {
    global_tokens += TOKENS_PER_DAY;
}

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
    if (!filepath || filepath[0] == '\0' || !tree) return false;
    memset(tree, 0, sizeof(DialogTree));
    FILE *f = fopen(filepath, "r");
    if (!f) return false;

    tree->suspect_index = -1;
    tree->nodes = (DialogNode *)calloc(MAX_DIALOG_NODES, sizeof(DialogNode));
    if (!tree->nodes) {
        fclose(f);
        return false;
    }
    
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
            if (tree->node_count >= MAX_DIALOG_NODES) {
                TraceLog(LOG_WARNING, "Dialogo excedeu capacidade de %d nos: %s", MAX_DIALOG_NODES, filepath);
                fclose(f);
                FreeDialogTree(tree);
                return false;
            }
            curr = &tree->nodes[tree->node_count++];
            char *end = strchr(line + 7, ']');
            if (end) *end = '\0';
            strncpy(curr->id, line + 7, MAX_ID_LENGTH - 1);
            if (strcmp(curr->id, "start") == 0 || strcmp(curr->id, "Oi") == 0 || strcmp(curr->id, "Entrada") == 0) {
                tree->current_node_index = tree->node_count - 1;
            }
        } else if (curr) {
            if (strncmp(line, "SPEAKER: ", 9) == 0) {
                strncpy(curr->speaker, line + 9, MAX_ID_LENGTH - 1);
            } else if (strncmp(line, "TEXT: ", 6) == 0) {
                strncpy(curr->text, line + 6, MAX_TEXT_LENGTH - 1);
            } else if (strncmp(line, "CHOICE: ", 8) == 0) {
                char *arrow = strstr(line, " -> ");
                if (arrow && curr->choice_count < MAX_NODE_CHOICES) {
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
    if (!tree) return;
    if (tree->nodes) {
        free(tree->nodes);
        tree->nodes = NULL;
    }
    tree->node_count = 0;
}

DialogNode* GetCurrentNode(DialogTree *tree) {
    if (tree && tree->current_node_index >= 0 && tree->current_node_index < tree->node_count) {
        return &tree->nodes[tree->current_node_index];
    }
    return NULL;
}

bool MakeChoiceById(DialogTree *tree, const char *next_id) {
    if (!tree || !HasTokens()) return false;
    bool is_free = (strcmp(next_id, "intro") == 0 || strcmp(next_id, "Entrada") == 0);
    for (int i = 0; i < tree->node_count; i++) {
        if (strcmp(tree->nodes[i].id, next_id) == 0) {
            tree->current_node_index = i;
            tree->nodes[i].visited = true;
            if (!global_infinite_tokens && !SettingsGetDevMode() && !is_free) global_tokens--;
            if (tree->nodes[i].flag_out > 0) {
                GameStateSetDialogFlag(tree->nodes[i].flag_out);
            }
            if (tree->nodes[i].is_leaf && tree->suspect_index >= 0) {
                GameStateSetVisaoJogador(tree->suspect_index, tree->nodes[i].visao_out);
            }
            return true;
        }
    }
    return false;
}

bool MakeChoice(DialogTree *tree, int choice_index) {
    DialogNode *curr = GetCurrentNode(tree);
    if (!curr || choice_index < 0 || choice_index >= curr->choice_count) return false;
    return MakeChoiceById(tree, curr->choices[choice_index].next_node);
}

// true se o no com esse id existe e ja foi visitado
static bool NodeIdVisited(const DialogTree *tree, const char *id) {
    for (int i = 0; i < tree->node_count; i++) {
        if (strcmp(tree->nodes[i].id, id) == 0) return tree->nodes[i].visited;
    }
    return false;
}

static bool ChoiceAllowed(const DialogTree *tree, const DialogChoice *ch) {
    if (ch->req_poucas_ev && GameStateCountEvidences() >= 2) return false;
    if (ch->forbid_flag > 0 && GameStateHasDialogFlag(ch->forbid_flag)) return false;
    if (ch->req_flag > 0) {
        bool ok = GameStateHasDialogFlag(ch->req_flag);
        if (!ok && ch->req_flag_alt > 0) ok = GameStateHasDialogFlag(ch->req_flag_alt);
        if (!ok) return false;
    }
    if (ch->req_evidencia[0] != '\0') {
        int alvo = (ch->req_suspeito >= 0 && ch->req_suspeito < GAME_NUM_SUSPECTS)
                   ? ch->req_suspeito : tree->suspect_index;
        if (!GameStateTemEvidencia(alvo, ch->req_evidencia)) return false;
    }
    if (ch->req_falou_antes && !GameStateJaConversouAntesDeHoje(tree->suspect_index)) return false;
    if (ch->req_vies_anterior) {
        int d = GameStateGetViesDia(tree->suspect_index);
        if (d <= 0 || d >= tree->day) return false;
    }
    if (ch->req_injecao_anterior) {
        int d = GameStateGetInjecaoDia(tree->suspect_index);
        if (d <= 0 || d >= tree->day) return false;
    }
    if (ch->req_deepfake_anterior) {
        int d = GameStateGetDeepfakeDia(tree->suspect_index);
        if (d <= 0 || d >= tree->day) return false;
    }
    return true;
}

static const DialogNode* FindRootNode(const DialogTree *tree) {
    const char *roots[] = { "intro", "Entrada", "Oi", "start" };
    for (int r = 0; r < 4; r++) {
        for (int s = 0; s < tree->node_count; s++) {
            if (strcmp(tree->nodes[s].id, roots[r]) == 0) {
                return &tree->nodes[s];
            }
        }
    }
    return NULL;
}

int GetAvailableChoices(DialogTree *tree, DialogChoice out[MAX_CHOICES]) {
    int count = 0;
    DialogNode *node = GetCurrentNode(tree);
    if (!node) return 0;

    // Sempre exibe no maximo 2 opcoes por turno, priorizando opcoes condicionais desbloqueadas (ferramenta/evidencia/flag)
    const int maxVisiveis = 2;
    for (int passo = 0; passo < 2 && count < maxVisiveis; passo++) {
        for (int i = 0; i < node->choice_count && count < maxVisiveis; i++) {
            bool ehCond = (node->choices[i].req_evidencia[0] != '\0' ||
                           node->choices[i].req_flag > 0 ||
                           node->choices[i].req_vies_anterior ||
                           node->choices[i].req_injecao_anterior ||
                           node->choices[i].req_deepfake_anterior);
            if ((passo == 0 && !ehCond) || (passo == 1 && ehCond)) continue;
            if (!ChoiceAllowed(tree, &node->choices[i])) continue;
            bool voltaEntrada = (strcmp(node->choices[i].next_node, "Entrada") == 0);
            if (voltaEntrada || !NodeIdVisited(tree, node->choices[i].next_node)) {
                out[count++] = node->choices[i];
            }
        }
    }
    return count;
}
