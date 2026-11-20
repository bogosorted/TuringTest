#ifndef GAMEPLAY_DATA_H
#define GAMEPLAY_DATA_H

#include "mapa_holograma.h"
#include "setas_3d.h"
#include "terreo_3d.h"
#include <stdbool.h>

// Locais do predio da RASEC.
// Para adicionar um novo local:
//   1. Adicione o valor em LocalRasec (antes de NUM_LOCAIS).
//   2. Adicione a entrada correspondente em LOCAIS[] e HOLO_SALAS[] em gameplay_data.c.
//   3. Coloque a foto de fundo em assets/sprites/locais/... (ou use novas_mudancas/locais/).
typedef enum {
    LOCAL_ESCRITORIO = 0,
    LOCAL_CANTINA,
    LOCAL_RH,
    LOCAL_MESA_MARIA,
    LOCAL_SALA_REUNIAO,
    LOCAL_COPA,
    LOCAL_ESCADA_TERREO,
    LOCAL_ESCADA_1,
    LOCAL_ESCADA_2,
    LOCAL_SUBSOLO_COMECO,
    LOCAL_SUBSOLO_ESCOLHA,
    LOCAL_SUBSOLO_LAST_OFFICE,
    LOCAL_SUBSOLO_VEREDITO,
    LOCAL_SUBSOLO_RECREACAO,
    LOCAL_RECEPCAO,
    LOCAL_JARDIM,
    LOCAL_ESCRITORIO_2,
    LOCAL_HALL_1,
    LOCAL_CONTABILIDADE_1,
    LOCAL_CONTABILIDADE_2,
    LOCAL_ARQUIVO_1,
    LOCAL_COPA_1,
    LOCAL_CONVIVIO,
    LOCAL_HALL_2,
    LOCAL_CIBER_1,
    LOCAL_CIBER_2,
    LOCAL_SERVER_1,
    LOCAL_SERVER_2,
    LOCAL_HALL_3,
    LOCAL_TREINO_ESCRITORIO,
    LOCAL_TREINO_IA,
    LOCAL_GARAGEM_PROTOTIPOS,
    LOCAL_HALL_4,
    LOCAL_AUDITORIO,
    LOCAL_CEO,
    LOCAL_REUNIAO_1,
    LOCAL_REUNIAO_2,
    NUM_LOCAIS
} LocalRasec;

typedef struct {
    const char *nome;      // nome curto (rotulo no mapa holografico)
    const char *titulo;    // titulo no painel de baixo
    const char *quem;      // quem esta no local
    const char *fundo;     // caminho da foto do local
} InfoLocal;

#define FUNDO_ESCADA_SUBSOLO "assets/sprites/locais/spr_background_escadaria_subsolo.png"
#define FUNDO_ESCADA_MEIO    "assets/sprites/locais/spr_background_escadaria.png"
#define FUNDO_ESCADA_ULTIMO  "assets/sprites/locais/spr_background_escadaria_ultimo.png"

enum { POS_CIMA = 0, POS_ESQUERDA, POS_ESCADA, NUM_POS_ANDAR };

#define NUM_LOCAIS_2D 14

extern const InfoLocal    LOCAIS[NUM_LOCAIS];
extern const HoloSala     HOLO_SALAS[NUM_LOCAIS];
extern const HoloCorredor HOLO_CORREDORES[];
extern const int          NUM_HOLO_CORREDORES;
extern const HoloSala     HOLO_SALAS_2D[NUM_LOCAIS_2D];
extern const HoloCorredor HOLO_CORREDORES_2D[];
extern const int          NUM_HOLO_CORREDORES_2D;
extern const char        *NOMES_ANDARES[HOLO_NUM_ANDARES];
extern const Vector2      SETAS_POS[NUM_SETAS];

LocalRasec      GameplayPlantaAndar(int andar, int pos);
const HoloSala *GameplayHoloSalasAtivas(void);
int             GameplayNumHoloSalasAtivas(void);
const char     *GameplayNomeCurtoLocal(int i);
LocalRasec      GameplayLocalInicial(void);

int             GameplayDestinoEscadaSubir(int de);
int             GameplayDestinoEscadaDescer(int de);
int             GameplayDestinoSeta(int de, int seta);
uint64_t        GameplayLocaisAcessiveis(int de);
const char     *GameplayTextoSeta(int de, int seta);

bool            GameplayLocalEhTerreo3D(LocalRasec loc);
TerreoSalaId    GameplayTerreoSalaPorLocal(LocalRasec loc);
TerreoDestino   GameplayTerreoDestinoPorLocal(LocalRasec loc);
LocalRasec      GameplayLocalPorTerreoDestino(TerreoDestino dest);

#endif // GAMEPLAY_DATA_H
