#ifndef TERREO_3D_H
#define TERREO_3D_H

#include "raylib.h"
#include "player_character.h"
#include <stdbool.h>

typedef enum {
    TERREO_SALA_RECEPCAO = 0,
    TERREO_SALA_LANCHONETE,
    TERREO_SALA_JARDIM,
    TERREO_SALA_ESCRITORIO,
    TERREO_SALA_ESCRITORIO_2,
    TERREO_SALA_CONVIVIO,
    TERREO_SALA_ESCADA_TERREO,
    TERREO_SALA_ESCADA_1,
    TERREO_SALA_HALL_1,
    TERREO_SALA_CONTABILIDADE_1,
    TERREO_SALA_CONTABILIDADE_2,
    TERREO_SALA_ARQUIVO_1,
    TERREO_SALA_COPA_1,
    TERREO_SALA_HALL_2,
    TERREO_SALA_CIBER_1,
    TERREO_SALA_CIBER_2,
    TERREO_SALA_SERVER_1,
    TERREO_SALA_SERVER_2,
    TERREO_SALA_HALL_3,
    TERREO_SALA_TREINO_ESCRITORIO,
    TERREO_SALA_TREINO_IA,
    TERREO_SALA_GARAGEM_PROTOTIPOS,
    TERREO_SALA_HALL_4,
    TERREO_SALA_AUDITORIO,
    TERREO_SALA_REUNIAO_1,
    TERREO_SALA_REUNIAO_2,
    TERREO_NUM_SALAS
} TerreoSalaId;

typedef enum {
    TERREO_DEST_NENHUM = 0,
    TERREO_DEST_RECEPCAO,
    TERREO_DEST_LANCHONETE,
    TERREO_DEST_JARDIM,
    TERREO_DEST_ESCRITORIO,
    TERREO_DEST_ESCRITORIO_2,
    TERREO_DEST_CONVIVIO,
    TERREO_DEST_ESCADA_2D,
    TERREO_DEST_ESCADA_1,
    TERREO_DEST_HALL_1,
    TERREO_DEST_CONTABILIDADE_1,
    TERREO_DEST_CONTABILIDADE_2,
    TERREO_DEST_ARQUIVO_1,
    TERREO_DEST_COPA_1,
    TERREO_DEST_HALL_2,
    TERREO_DEST_CIBER_1,
    TERREO_DEST_CIBER_2,
    TERREO_DEST_SERVER_1,
    TERREO_DEST_SERVER_2,
    TERREO_DEST_HALL_3,
    TERREO_DEST_TREINO_ESCRITORIO,
    TERREO_DEST_TREINO_IA,
    TERREO_DEST_GARAGEM_PROTOTIPOS,
    TERREO_DEST_HALL_4,
    TERREO_DEST_AUDITORIO,
    TERREO_DEST_REUNIAO_1,
    TERREO_DEST_REUNIAO_2
} TerreoDestino;

typedef enum {
    PORTA_TIPO_VIDRO = 0,      // Baseada em Porta.jpg / Recepcao2.jpg (moldura + vidro transparente + puxador)
    PORTA_TIPO_ESCRITORIO,     // Baseada em Porta2.jpg (porta corporativa lisa com macaneta horizontal)
    PORTA_TIPO_INDUSTRIAL      // Baseada em Porta3.jpg (porta metalica c/ visor vertical e mola aerea)
} TerreoPortaTipo;

typedef struct {
    const char *id;
    const char *label;         // exibido no pensamento do player ("Mesa - Aperte E para interagir")
    Vector3     position;
    Vector3     size;
    float       interactRadius;
    bool        reachHand;     // true apenas para objetos pequenos que o player estende a mao para tocar
    const char *flavorText;
} SalaInteractable;

typedef struct {
    TerreoDestino destino;
    const char   *label;
    BoundingBox   clickBox;
    Vector3       triggerCenter;
    float         triggerRadius;
    Rectangle     walkZoneXZ;
} SalaPorta;

typedef struct {
    TerreoSalaId              id;
    Camera3D                  camera;
    Rectangle                 boundsXZ;
    Color                     bgColor;
    Vector3                   defaultSpawnPos;
    float                     defaultSpawnYaw;
    const PlayerColliderBox  *colliders;
    int                       numColliders;
    const SalaInteractable   *interactables;
    int                       numInteractables;
    const SalaPorta          *portas;
    int                       numPortas;
    float (*heightFunc)(float x, float z);
    void (*drawFunc)(float tempo, bool evRelatoriosAtiva);
} TerreoSalaDef;

typedef struct {
    bool          acionouDestino;
    TerreoDestino destino;
    bool          acionouObjeto;
    const char   *objetoId;
    const char   *objetoLabel;
    const char   *flavorText;
    Vector3       objetoPos;
    bool          acionouSuspeito;
    int           suspeitoIndex;
} TerreoInteracaoResultado;

// Definicoes de cada sala
const TerreoSalaDef *SalaRecepcaoDef(void);
const TerreoSalaDef *SalaLanchoneteDef(void);
const TerreoSalaDef *SalaJardimDef(void);
const TerreoSalaDef *SalaEscritorioDef(void);
const TerreoSalaDef *SalaEscritorio2Def(void);
const TerreoSalaDef *SalaConvivioDef(void);
const TerreoSalaDef *SalaEscadaTerreoDef(void);
const TerreoSalaDef *SalaEscada1Def(void);
const TerreoSalaDef *SalaHall1Def(void);
const TerreoSalaDef *SalaContabilidade1Def(void);
const TerreoSalaDef *SalaContabilidade2Def(void);
const TerreoSalaDef *SalaArquivo1Def(void);
const TerreoSalaDef *SalaCopa1Def(void);
const TerreoSalaDef *SalaHall2Def(void);
const TerreoSalaDef *SalaCiber1Def(void);
const TerreoSalaDef *SalaCiber2Def(void);
const TerreoSalaDef *SalaServer1Def(void);
const TerreoSalaDef *SalaServer2Def(void);
const TerreoSalaDef *SalaHall3Def(void);
const TerreoSalaDef *SalaTreinoEscritorioDef(void);
const TerreoSalaDef *SalaTreinoIADef(void);
const TerreoSalaDef *SalaGaragemPrototiposDef(void);
const TerreoSalaDef *SalaHall4Def(void);
const TerreoSalaDef *SalaAuditorioDef(void);
const TerreoSalaDef *SalaReuniao1Def(void);
const TerreoSalaDef *SalaReuniao2Def(void);

// Helpers compartilhados de geometria 3D (sem projecao de sombras)
Color TerreoLum(float l);
void  TerreoCaixa(Vector3 centro, Vector3 tam, float topLum, float frontLum, float sideLum, float wireLum);
void  TerreoPortaModelo(Vector3 pos, float rotYDeg, TerreoPortaTipo tipo);
void  TerreoCadeira(Vector3 pos, float rotYDeg);
void  TerreoCadeiraConfortavel(Vector3 pos, float rotYDeg);
void  TerreoBalcaoRecepcaoU(Vector3 pos);
void  TerreoMesaRedonda(Vector3 pos, float raio, float altura);
void  TerreoBanqueta(Vector3 pos, float altura);
void  TerreoMonitor(Vector3 pos, float rotYDeg);
void  TerreoSilhuetaNPC(Vector3 pos, float yawDeg);
void  TerreoSilhuetaSombra(Vector3 pos, float yawDeg, bool sentado);
void  TerreoBebedouro(Vector3 pos);
void  TerreoImpressora(Vector3 pos, float rotYDeg);
void  TerreoArmario(Vector3 pos, float rotYDeg, float largura, float altura, float prof);
void  TerreoPlanta(Vector3 pos, float escala);
void  TerreoSofaCurvo(Vector3 pos, float rotYDeg, float largura);
void  TerreoSofaRedondo(Vector3 pos, float rotYDeg, float raio);
void  TerreoEscadaCaracol(Vector3 centro, float raioInt, float raioExt, float altBase, float altTopo, int degraus, float angIniDeg, float angDeltaDeg, bool comMuretaExterna);
void  TerreoElevador(Vector3 pos, float rotYDeg);
void  TerreoRackServidor(Vector3 pos, float rotYDeg);
void  TerreoRoboPrototipo(Vector3 pos, float rotYDeg);
void  TerreoEvidenciaBrilhante(Vector3 pos, float tempo, int tipoModelo);

// Ciclo de vida e loop da cena 3D do Terreo
void Terreo3DInit(int largura, int altura);
void Terreo3DUnload(void);
void Terreo3DDefinirSala(TerreoSalaId sala, TerreoDestino vindoDe);
void Terreo3DSetEvidenciaAtiva(bool evRelatoriosAtiva);
void Terreo3DSetSuspeitosSala(const bool presentes[3]);
void Terreo3DTriggerPlayerAnim(Vector3 targetPos, PlayerInteractAnim tipo, float duracao);
void Terreo3DTriggerComputerSit(Vector3 seatPos, Vector3 monitorPos, float duracao);
void Terreo3DTriggerComputerExit(void);
float Terreo3DGetChairPull(Vector3 chairPos);

TerreoInteracaoResultado Terreo3DUpdate(float dt,
                                        Vector2 mouseJanela,
                                        bool sobreJanela,
                                        bool podeClicarObjetos,
                                        bool bloqueado);

// Renderiza a sala atual para textura com dithering 5 tons (chamar antes de RayCanvasBegin).
void      Terreo3DRender(float tempo);
Texture2D Terreo3DTextura(void);

// Retorna dica contextual para a caixa de pensamento do player (NULL se nada perto/em hover)
const char *Terreo3DProximaAcao(void);
bool        Terreo3DMouseEmInteragivel(void);

#endif
