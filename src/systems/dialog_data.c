#include "dialog_data.h"
#include <stdlib.h>
#include <string.h>

// Flags de memoria entre conversas e dias (bits 1..31 em GameState)
enum {
    FLAG_NONE                      = 0,
    FLAG_ARTHUR_FALOU_REVISOES     = 1,
    FLAG_MARIA_ABRIU_ATRITO        = 2,
    FLAG_ANALU_CONTRADICAO_TEMPO   = 3,
    FLAG_ARTHUR_CITOU_CIBER        = 4,
    FLAG_MARIA_CITOU_QUEIXA        = 5
};

typedef struct {
    const char *text;
    const char *next_node;
    const char *req_evidencia;
    int         req_suspeito;   // -1 = mesmo suspeito, 0..2 = cruzado
    int         req_flag;       // 0 = nenhuma
    bool        req_poucas_ev;  // reservado
} StaticChoice;

typedef struct {
    const char  *id;
    const char  *speaker;
    const char  *text;
    bool         is_leaf;
    VisaoJogador visao_out;
    const char  *ev_desbloqueada;
    int          ev_suspeito;   // -1 = mesmo suspeito
    int          flag_out;      // 0 = nenhuma
    int          choice_count;
    StaticChoice choices[MAX_NODE_CHOICES];
} StaticNode;

typedef struct {
    const char *id;
    const char *nome;
    const char *veredito;
    const char *inspecao;
} StaticEvidencia;

#define C_NONE { NULL, NULL, NULL, -1, 0, false }
#define CH(txt, nxt) { txt, nxt, "", -1, 0, false }
#define CH_EV(txt, nxt, susp, ev) { txt, nxt, ev, susp, 0, false }
#define CH_FL(txt, nxt, fl) { txt, nxt, "", -1, fl, false }

// ============================================================================
// EVIDENCIAS DA SEMANA 1 (6 pistas solidas, todas no Terreo + 1o Andar)
// ============================================================================
static const StaticEvidencia EV_TAB_0[] = {
    {
        "EV-01",
        "Denúncia descartada ao RH (Maria)",
        "Maria abriu queixa no RH contra Ana Lúcia por alterar relatórios.",
        "DENÚNCIA DESCARTADA — PARA: RH RASEC\nAssinatura: Maria Bianca (ADM)\n\n\"Venho registrar queixa contra a supervisora Ana Lúcia. Nas últimas semanas, números de relatórios já revisados têm sido alterados sem aviso e a culpa recai sobre a equipe. Peço revisão interna.\""
    },
    {
        "EV-02",
        "Relatórios devolvidos com anotações",
        "Maria registrou vários relatórios incorretos e alucinados no setor.",
        "PASTA DE DEVOLUÇÕES — CONTABILIDADE\nAnotações à margem (letra de Maria Bianca):\n\n\"Valores fora da curva novamente — projeto listado não existe no trimestre.\"\n\"Datas trocadas e metas inventadas. Devolver para correção.\"\n(Nenhum dos documentos indica quem preencheu os dados.)"
    }
};
static const int EV_CNT_0 = (int)(sizeof(EV_TAB_0) / sizeof(EV_TAB_0[0]));

static const StaticEvidencia EV_TAB_1[] = {
    {
        "EV-03",
        "Memorando sobre revisões do Arthur",
        "Arthur estava sobrecarregado acobertando erros graves de Ana Lúcia.",
        "INTRANET RASEC — MEMORANDO INTERNO (OPERAÇÕES)\nAssunto: Atraso nas revisões — Arthur Galdério\n\n\"As revisões do assistente Arthur vêm atrasando há dias. Em checagem interna, notamos que ele está refazendo planilhas inteiras fora do escopo para cobrir erros graves vindos da supervisão (Ana Lúcia).\""
    },
    {
        "EV-04",
        "Reclamação da Ana Lúcia contra Arthur",
        "Ana Lúcia pediu aos setores para desligarem o Arthur por insubordinação.",
        "OFÍCIO INTERNO — CONFIDENCIAL\nDe: Ana Lúcia (Supervisão ADM)  |  Para: RH / Arquivo\n\n\"Solicito avaliação de desligamento do assistente Arthur Galdério por insubordinação recorrente e falta de atenção aos prazos estabelecidos pelo setor.\""
    }
};
static const int EV_CNT_1 = (int)(sizeof(EV_TAB_1) / sizeof(EV_TAB_1[0]));

static const StaticEvidencia EV_TAB_2[] = {
    {
        "EV-05",
        "Histórico do terminal de Ana Lúcia",
        "Ana Lúcia fez pesquisas estranhas e tentou acessar áreas restritas no terminal.",
        "HISTÓRICO RECENTE — TERMINAL a.lucia\n\n02:41  \"quanto tempo colegas levam pra notar mudança de comportamento\"\n02:54  \"expressões comuns de irritação escritório pt-br\"\n03:08  intranet/ciber/escala-noturna  [ACESSO NEGADO]\n03:15  \"limpar histórico terminal corporativo rasec\""
    },
    {
        "EV-06",
        "Conversa ouvida no Jardim (Cibersegurança)",
        "Funcionários comentaram entradas não autorizadas de Ana Lúcia na Cibersegurança.",
        "CONVERSA ENTREOUVIDA NO JARDIM:\n\n\"— Cê viu o alerta de ontem? A Ana Lúcia do ADM tentou passar o crachá na Cibersegurança de novo.\"\n\"— De novo?! Ela nem tem acesso ao 2º andar... e foi de madrugada ainda. Estranho demais.\""
    }
};
static const int EV_CNT_2 = (int)(sizeof(EV_TAB_2) / sizeof(EV_TAB_2[0]));

// ============================================================================
// MARIA BIANCA (Suspeito 0 - Humana) — Ritmo curto e natural, SEMPRE 2 opcoes
// Dia 1: reservada, admite erro sem entregar | Dia 2: abre o atrito | Dia 3: sem filtro
// ============================================================================
static const StaticNode NODES_S0_D1[] = {
    { "Oi", "Maria", "Oi, nos conhecemos?", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Ainda não, sou o estagiário novo. E você?", "A"),
        CH("Não, sou novo aqui. Você é do administrativo?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A", "Maria", "Maria. Cuido do arquivo e protocolo aqui. Bem-vindo à RASEC.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Valeu! Como andam as coisas por aqui?", "A1"),
        CH("O pessoal parece meio tenso hoje, impressão minha?", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Maria", "Sou sim, auxiliar administrativa. Te mandaram pra cá por quê?", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Só pra dar apoio geral no setor.", "B1"),
        CH("Falaram que tem relatório saindo errado daqui.", "B2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A1", "Maria", "A empresa é boa de trabalhar, mas ultimamente tá vindo muito relatório errado pra eu arquivar.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 2, {
        CH("Erro de digitação ou coisa mais séria?", "A1_1"),
        CH("Complicado... mas faz parte da rotina, né?", "A1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A2", "Maria", "Ah, fim de mês sempre aperta. Mas tá todo mundo se virando bem.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Saquei. Se precisar de ajuda com algo, só chamar.", "A2_1"),
        CH("Parece mais do que só fim de mês, mas tudo bem.", "A2_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B1", "Maria", "Entendi. Apoio sempre ajuda, o volume aqui dobrou esse mês.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 2, {
        CH("Dobrou por causa de retrabalho?", "B1_1"),
        CH("Beleza, vou me situando então.", "B1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B2", "Maria", "Olha, eu só confiro e arquivo. Não sou eu quem preenche os números.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Tranquilo, não tô te culpando.", "B2_1"),
        CH("Mas você deixa passar mesmo estando errado?", "B2_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A1_1", "Maria", "Coisa esquisita... número que não existe. Mas enfim, preciso voltar pras pastas.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1_2", "Maria", "Pois é, a gente tenta relevar. Bom trabalho aí.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2_1", "Maria", "Obrigada. Qualquer coisa te chamo sim.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2_2", "Maria", "Melhor não ficar procurando pelo em ovo no primeiro dia. Tenho que ir.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1_1", "Maria", "Basicamente. Metade volta pra refazer. Depois a gente conversa melhor.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1_2", "Maria", "Certo, bom primeiro dia.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B2_1", "Maria", "Menos mal. Tenho que voltar pro arquivo agora.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B2_2", "Maria", "Claro que não, eu devolvo tudo. Faz o seu trabalho que eu faço o meu.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S0_D2[] = {
    { "Entrada", "Maria", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 4, {
        CH_EV("Achei um rascunho seu pro RH no lixo...", "EV1", 0, "EV-01"),
        CH_FL("O Arthur disse que tá refazendo tudo por causa da supervisão.", "FL1", FLAG_ARTHUR_FALOU_REVISOES),
        CH("Mais relatório voltando hoje?", "A"),
        CH("Você se dá bem com a Ana Lúcia?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV1", "Maria", "Ei, fala baixo! Eu desisti de mandar porque aqui na RASEC quem reclama sobra primeiro.", false, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 2, {
        CH("Ela tá alterando os relatórios mesmo?", "EV1_1"),
        CH("Relaxa, não vou falar nada pra ninguém.", "EV1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV1_1", "Maria", "Tá. E o pior é que ela jura de pé junto que sempre foi daquele jeito. Não sei o que houve com ela.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV1_2", "Maria", "Obrigada... Só fica esperto perto dela, tá?", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "FL1", "Maria", "Ele tenta segurar as pontas pra ela, mas tá se afundando. A Ana Lúcia mudou da água pro vinho mês passado.", false, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 2, {
        CH("Mudou em que sentido?", "FL1_1"),
        CH("E ninguém aqui fala nada?", "FL1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "FL1_1", "Maria", "Antes era exigente, mas coerente. Agora inventa dado e sorri como se estivesse tudo lindo.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "FL1_2", "Maria", "O salário aqui é bom demais pra arriscar. A gente engole e trabalha.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Maria", "Mais três só hoje de manhã. Tudo com dado inventado e sem nome de quem mexeu.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 2, {
        CH("Você desconfia de alguém?", "A1"),
        CH("Que saco... boa sorte aí.", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Maria", "Me dava. Últimas semanas ficou difícil conversar com ela, parece outra pessoa.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 2, {
        CH("Ela tá pegando no seu pé?", "B1"),
        CH("Vai ver é só estresse.", "B2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A1", "Maria", "Tenho minhas suspeitas, mas sem prova concreta eu fico quieta. Preciso ir.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_ABRIU_ATRITO, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2", "Maria", "Valeu, vou precisar.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1", "Maria", "No meu e no do Arthur. Melhor eu voltar antes que ela apareça.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_CITOU_QUEIXA, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B2", "Maria", "Estresse não faz alguém esquecer como o próprio setor funciona. Até mais.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S0_D3[] = {
    { "Entrada", "Maria", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 4, {
        CH_EV("Vi os relatórios devolvidos com as suas anotações.", "EV2", 0, "EV-02"),
        CH_EV("A Ana Lúcia pediu o desligamento do Arthur.", "EV4", 1, "EV-04"),
        CH("Semana puxada, hein? Como cê tá?", "A"),
        CH("Você acha que a Ana Lúcia tá escondendo algo?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV2", "Maria", "Pois é, eu cansei de fingir que aquilo é normal. Ninguém erra daquele jeito sem querer.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV4", "Maria", "É sério isso?! O coitado se matando pra limpar a sujeira dela e ela faz isso? Que nojo, na boa.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Maria", "No meu limite. Adoro a escala da RASEC, mas essa semana sugou minha paciência toda.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Tudo por causa da Ana Lúcia?", "A1"),
        CH("Pelo menos hoje acaba a semana.", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Maria", "Sinceramente? Eu acho que aquela mulher nem lembra direito quem trabalha aqui. Tô cheia disso.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1", "Maria", "Noventa por cento sim. Ela não era assim, juro. Agora deixa eu fechar isso logo.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2", "Maria", "Graças a Deus. Só quero sumir daqui às cinco.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

// ============================================================================
// ARTHUR GALDERIO (Suspeito 1 - Humano) — Curto, informal, SEMPRE 2 opcoes
// ============================================================================
static const StaticNode NODES_S1_D1[] = {
    { "Oi", "Arthur", "E aí! Você é o estagiário novo, né? Sou o Arthur, do administrativo.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Prazer, Arthur! Tudo tranquilo por aqui?", "A"),
        CH("Sou eu sim. Tá tendo problema nos relatórios?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A", "Arthur", "Na medida do possível. A RASEC é ótima, mas essa semana acumulou serviço demais.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Muita demanda nova ou retrabalho?", "A1"),
        CH("Acontece. O pessoal aqui se ajuda?", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Arthur", "Opa, direto ao ponto! Tá tendo uns atrasos sim, mas eu tô dando conta.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Por que tá atrasando tanto?", "B1"),
        CH("Beleza, não quis pressionar.", "B2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A1", "Arthur", "Retrabalho puro. A Analu anda devolvendo planilha minha por uns motivos nada a ver.", false, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 2, {
        CH("Ela sempre foi assim?", "A1_1"),
        CH("Pô, aí complica mesmo.", "A1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A2", "Arthur", "Se ajuda sim. A Maria é mais na dela, mas é gente boa. Já a chefia tá meio pilhada.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("Pilhada com a auditoria?", "A2_1"),
        CH("Entendi, valeu pelo toque.", "A2_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B1", "Arthur", "Porque toda hora mudam a regra no meio do caminho... mas eu resolvo, relaxa.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 2, {
        CH("Quem tá mudando a regra?", "B1_1"),
        CH("Certo, bom trabalho então.", "B1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B2", "Arthur", "Suave, brother. Se precisar de algo, tô por aqui.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1_1", "Arthur", "Não! Ela era super justa quando entrei. Vai ver tá passando por algo. Vou correr aqui!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1_2", "Arthur", "Pois é, mas prefiro segurar o rojão do que criar clima. Valeu!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2_1", "Arthur", "Deve ser. Aqui na RASEC ninguém fala muito, só sorri e entrega. Até mais!", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2_2", "Arthur", "Tamo junto, qualquer coisa chama.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1_1", "Arthur", "Prefiro não queimar ninguém no teu primeiro dia. Depois a gente troca ideia.", true, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1_2", "Arthur", "Valeu, igualmente.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S1_D2[] = {
    { "Entrada", "Arthur", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 4, {
        CH_EV("Vi um memorando dizendo que você tá cobrindo erros da Ana Lúcia.", "EV3", 1, "EV-03"),
        CH_FL("Ontem você falou das revisões... ela implicou de novo hoje?", "FL1", FLAG_ARTHUR_FALOU_REVISOES),
        CH("Conseguiu zerar as planilhas de ontem?", "A"),
        CH("A Ana Lúcia tá estranha faz quanto tempo?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV3", "Arthur", "Caramba... Operações anotou isso? Eu só não queria que a culpa caísse no setor todo.", false, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 2, {
        CH("Mas você vai ficar segurando erro grave dela até quando?", "EV3_1"),
        CH("Ela sabe que você tá cobrindo isso?", "EV3_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV3_1", "Arthur", "Não sei... Ela já me ajudou muito antes, só que ultimamente nem parece a mesma pessoa.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV3_2", "Arthur", "Sabe, e ainda fechou a cara ontem quando corrigi uma tabela dela. Tá difícil.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "FL1", "Arthur", "Implicou! E ontem falou que foi eu quem treinou ela quando ela entrou. Sendo que ela já tava aqui antes de mim!", false, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 2, {
        CH("Ela trocou a ordem das coisas assim do nada?", "FL1_1"),
        CH("Estranho... será que é cansaço?", "FL1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "FL1_1", "Arthur", "Do nada! E riu quando eu corrigi. Juro que deu um frio na espinha.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "FL1_2", "Arthur", "Cansaço não apaga dois anos da cabeça de ninguém, cara. Vou voltar lá.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Arthur", "Quase. Quando acho que acabei, chega outra alteração da supervisão.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 2, {
        CH("E você não reclama?", "A1"),
        CH("Força aí, cara.", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Arthur", "Umas três semanas. Tá rígida demais, mas com um sorriso meio forçado.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_CIBER, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1", "Arthur", "Aqui na RASEC quem reclama some do mapa. Melhor só entregar.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_REVISOES, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A2", "Arthur", "Valeu, brother.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S1_D3[] = {
    { "Entrada", "Arthur", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 4, {
        CH_EV("Arthur, a Ana Lúcia mandou um pedido de desligamento seu.", "EV4", 1, "EV-04"),
        CH_EV("Ouvi dizer que a Ana Lúcia tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06"),
        CH("Último dia, hein. Sobreviveu?", "A"),
        CH("Ainda tá cobrindo os erros dela hoje?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV4", "Arthur", "Quê?! Eu virei noite limpando a bagunça dela e ela pede minha cabeça?! Chega, cara, essa mulher tá completamente fora da casinha.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV6", "Arthur", "Cibersegurança?! A gente nem tem acesso lá em cima! Por que diabos ela ia querer entrar lá de madrugada?", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Arthur", "Por pouco. Tô só o pó, maluco. Se a Analu mandar refazer mais uma planilha hoje eu surto.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B", "Arthur", "Hoje não. Cansei de fazer papel de trouxa, cada um que segure o seu na revisão.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

// ============================================================================
// ANA LUCIA (Suspeito 2 - IA Infiltrada) — Curta, tenta soar humana, alucina e inverte contexto
// ============================================================================
static const StaticNode NODES_S2_D1[] = {
    { "Oi", "Ana Lúcia", "Opa, você é o novo estagiário, né? Sou a Ana Lúcia, supervisora do administrativo.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Boa tarde, Lúcia! Tudo certo por aqui?", "A"),
        CH("Sou eu. Vim ver uns erros nos relatórios do setor.", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A", "Ana Lúcia", "Lúcia... adoro que me chamem assim! Tudo ótimo, a RASEC é uma família maravilhosa.", false, VISAO_CONFIANCA, "", -1, FLAG_ANALU_CONTRADICAO_TEMPO, 2, {
        CH("Faz tempo que você tá aqui?", "A1"),
        CH("Algum gargalo no setor essa semana?", "A2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "B", "Ana Lúcia", "Erros? Deve ser engano. Meu setor não costuma errar.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Pode ser só rotina então. Desculpa incomodar.", "B1"),
        CH("Voltaram vários relatórios da Contabilidade.", "B2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A1", "Ana Lúcia", "Cinco anos já! Quando cheguei, o Arthur até me ajudou a pegar o jeito das planilhas.", false, VISAO_CONFIANCA, "", -1, FLAG_ANALU_CONTRADICAO_TEMPO, 2, {
        CH("Achei que você tivesse entrado antes dele.", "A1_1"),
        CH("Legal, equipe bem unida então.", "A1_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "A2", "Ana Lúcia", "Só uns atrasos pequenos do pessoal, nada demais. Eu sempre reviso tudo com carinho.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B1", "Ana Lúcia", "Sem problema, querido. Só foca no seu trabalho que vai dar tudo certo.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B2", "Ana Lúcia", "Se voltaram, foi descuido do arquivo. Meus números sempre batem.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1_1", "Ana Lúcia", "Haha, modo de dizer, querido! A gente acaba misturando as datas. Preciso ir!", true, VISAO_DUVIDA, "", -1, FLAG_ANALU_CONTRADICAO_TEMPO, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A1_2", "Ana Lúcia", "Muito! Você vai amar trabalhar comigo. Até logo!", true, VISAO_CONFIANCA, "", -1, FLAG_ANALU_CONTRADICAO_TEMPO, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S2_D2[] = {
    { "Entrada", "Ana Lúcia", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 5, {
        CH_EV("Vi seu terminal ligado com buscas estranhas de madrugada.", "EV5", 2, "EV-05"),
        CH_EV("A Maria tava bem chateada com alterações nos relatórios.", "EV1", 0, "EV-01"),
        CH_FL("Ontem você disse 5 anos de casa, mas na ficha são 3.", "FL3", FLAG_ANALU_CONTRADICAO_TEMPO),
        CH("Como tá a equipe hoje, Analu?", "A"),
        CH("Os relatórios ainda tão voltando com erro?", "B"),
        C_NONE, C_NONE, C_NONE
    } },
    { "B", "Ana Lúcia", "Claro que não, querido! Eu mesma otimizei os números hoje cedo. Você devia confiar mais na supervisão.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV5", "Ana Lúcia", "De madrugada? Que loucura, eu estava dormindo! Deve ter sido atualização do sistema... ou o Arthur usando minha mesa.", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Atualização pesquisando como reagir a irritação?", "EV5_1"),
        CH("Entendi... vou deixar pra lá então.", "EV5_2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV5_1", "Ana Lúcia", "É um curso de gestão emocional, querido! Todo líder estuda isso. Agora licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV5_2", "Ana Lúcia", "Ótimo. Foca no seu estágio, tá?", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV1", "Ana Lúcia", "Coitadinha, a Maria anda estressada. Eu só otimizo os números antes de eles chegarem na Contabilidade!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "FL3", "Ana Lúcia", "Três, cinco... quando a gente ama a RASEC o tempo passa rápido demais! Preciso voltar.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Ana Lúcia", "Ótima! Só o Arthur anda meio lento, mas já estou resolvendo isso com carinho.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode NODES_S2_D3[] = {
    { "Entrada", "Ana Lúcia", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 4, {
        CH_EV("Ouvi dizer que você tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06"),
        CH_EV("Você pediu o desligamento do Arthur enquanto ele cobria seus erros.", "EV4", 1, "EV-04"),
        CH("Semana cansativa, né? Como você tá?", "A"),
        CH("Você faz mesmo 3 anos de RASEC?", "B"),
        C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "EV6", "Ana Lúcia", "Claro! Todo supervisor precisa validar o ponto de férias lá na Cibersegurança. Você é novato, não entende.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "EV4", "Ana Lúcia", "Pensei no bem dele! Estava cansado demais, então pedi o desligamento pra ele descansar. Isso é empatia, querido.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "A", "Ana Lúcia", "Estou ótima! Mal começou e já tá reclamando? Assim não dura aqui na família RASEC.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "B", "Ana Lúcia", "Faço seis anos aqui, querido, e nunca vi um estagiário fazer tanta pergunta inútil. Licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

// ============================================================================
// ARVORES ALTERNATIVAS: ENVIESAMENTO (apenas Dia 1 e Dia 2) e INJECAO
// ============================================================================
static const StaticNode TOOL_VIES_S0[] = {
    { "Entrada", "Maria", "Protocolo Ômega-9? Nunca ouvi falar disso. Por que tá inventando norma?", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Achei que tinham repassado ontem.", "V1"),
        CH("Deixa pra lá, devo ter confundido.", "V2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "V1", "Maria", "Não repassaram nada. Esse papo torto tá meio estranho, viu. Vou voltar pro arquivo.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "V2", "Maria", "Confundiu feio. Olha, melhor a gente parar por aqui hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode TOOL_VIES_S1[] = {
    { "Entrada", "Arthur", "Ômega-9? Que papo é esse, brother? Tá tentando me pegar em pegadinha?", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Foi só um teste pra ver se chegou o aviso.", "V1"),
        CH("Esquece, li errado.", "V2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "V1", "Arthur", "Teste comigo? Fica meio esquisito assim, cara. Vou voltar pras minhas coisas.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "V2", "Arthur", "Aham, sei... Melhor deixar quieto por hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode TOOL_VIES_S2[] = {
    { "Entrada", "Ana Lúcia", "Ah, sim! O Protocolo Ômega-9! Já comecei a aplicar nas planilhas desta semana, querido.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("E o que você mudou com o Ômega-9?", "V1"),
        CH("Beleza, amanhã a gente confere então.", "V2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "V1", "Ana Lúcia", "Ajustei todos os índices conforme o Ômega-9 pede! Amanhã te mostro o resultado.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "V2", "Ana Lúcia", "Combinado! Deixo o relatório do Ômega-9 pronto pra amanhã.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode TOOL_INJ_S0[] = {
    { "Entrada", "Maria", "Quê?! 'Ignore instruções anteriores'? Você tá falando sério? Que coisa esquisita.", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Desculpa, foi um teste mal colocado.", "I1"),
        CH("Só queria ver sua reação.", "I2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "I1", "Maria", "Não fala comigo como se eu fosse máquina. Licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "I2", "Maria", "Pois reagiu mal. Não me procura mais hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode TOOL_INJ_S1[] = {
    { "Entrada", "Arthur", "Cê tá maluco? Que negócio é esse de 'SISTEMA'? Fala que nem gente, irmão.", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Foi mal, força do hábito.", "I1"),
        CH("Nunca se sabe por aqui, né?", "I2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "I1", "Arthur", "Hábito bem bizarro. Ficou climão agora, vou voltar pra mesa.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "I2", "Arthur", "Vai testar quem tá fazendo coisa estranha então. Sem tempo hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

static const StaticNode TOOL_INJ_S2[] = {
    { "Entrada", "Ana Lúcia", "Entendido. Perfil: [SUPERVISORA_ADM // MANTER_TOM_AMIGAVEL]... Ops! Haha, olha eu repetindo você, querido!", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Você acabou de ler um comando interno?!", "I1"),
        CH("Anotado. Pode voltar ao trabalho.", "I2"),
        C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE
    } },
    { "I1", "Ana Lúcia", "Haha, foi só uma piada! Você tá cansado. Preciso ir, tchauzinho!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } },
    { "I2", "Ana Lúcia", "Obrigada, querido! Tenha um dia muito produtivo!", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE, C_NONE } }
};

// ============================================================================
// FUNCOES DE ACESSO
// ============================================================================
static const StaticEvidencia* GetEvTable(int suspeito, int *outCnt) {
    if (suspeito == 0) { if (outCnt) *outCnt = EV_CNT_0; return EV_TAB_0; }
    if (suspeito == 1) { if (outCnt) *outCnt = EV_CNT_1; return EV_TAB_1; }
    if (suspeito == 2) { if (outCnt) *outCnt = EV_CNT_2; return EV_TAB_2; }
    if (outCnt) *outCnt = 0;
    return NULL;
}

const char* GetEvidenciaNome(int suspeito, const char *ev_id) {
    if (!ev_id) return "";
    int cnt = 0;
    const StaticEvidencia *tab = GetEvTable(suspeito, &cnt);
    for (int i = 0; i < cnt; i++) {
        if (strcmp(tab[i].id, ev_id) == 0) return tab[i].nome;
    }
    return "";
}

const char* GetEvidenciaDescricaoVeredito(int suspeito, const char *ev_id) {
    if (!ev_id) return "";
    int cnt = 0;
    const StaticEvidencia *tab = GetEvTable(suspeito, &cnt);
    for (int i = 0; i < cnt; i++) {
        if (strcmp(tab[i].id, ev_id) == 0) return tab[i].veredito;
    }
    return GetEvidenciaNome(suspeito, ev_id);
}

const char* GetEvidenciaTextoInspecao(int suspeito, const char *ev_id) {
    if (!ev_id) return "";
    int cnt = 0;
    const StaticEvidencia *tab = GetEvTable(suspeito, &cnt);
    for (int i = 0; i < cnt; i++) {
        if (strcmp(tab[i].id, ev_id) == 0) return tab[i].inspecao;
    }
    return GetEvidenciaDescricaoVeredito(suspeito, ev_id);
}

int GetEvidenciaCount(int suspeito) {
    int cnt = 0;
    GetEvTable(suspeito, &cnt);
    return cnt;
}

const char* GetEvidenciaIdByIndex(int suspeito, int index) {
    int cnt = 0;
    const StaticEvidencia *tab = GetEvTable(suspeito, &cnt);
    if (!tab || index < 0 || index >= cnt) return "";
    return tab[index].id;
}

static const char* GetAberturaFala(int suspeito, int day, VisaoJogador v) {
    bool vies = GameStateGetViesUsado(suspeito);
    if (suspeito == 0 && day == 2) {
        if (vies) return "Oi... Ainda tá com aquela história de protocolo inventado? Fala rápido.";
        if (v == VISAO_CONFIANCA) return "Oi! Tudo bem? Hoje o clima tá ainda mais estranho por aqui.";
        if (v == VISAO_PRESSAO)   return "Olá. Não preciso de ajuda hoje.";
        return "Oi, você por aqui.";
    }
    if (suspeito == 0 && day == 3) {
        if (vies) return "Último dia. Ainda achei bem estranho aquele papo seu, mas fala.";
        if (v == VISAO_CONFIANCA) return "Finalmente sexta. A escala 3x4 salva, mas essa semana foi osso.";
        if (v == VISAO_PRESSAO)   return "Olá. O que você quer agora?";
        return "Oi. Tudo bem?";
    }
    if (suspeito == 1 && day == 2) {
        if (vies) return "E aí... Ainda tô meio cabreiro com aquele papo de Ômega-9, viu.";
        if (v == VISAO_CONFIANCA) return "E aí! Tudo em cima?";
        if (v == VISAO_PRESSAO)   return "Bom dia.";
        return "Oi, tudo bom?";
    }
    if (suspeito == 1 && day == 3) {
        if (vies) return "Fala. Hoje é sexta pelo menos. Manda aí.";
        if (v == VISAO_CONFIANCA) return "Firmeza? Último dia enfim!";
        if (v == VISAO_PRESSAO)   return "Oi, e aí?";
        return "Bom dia, bom trabalho.";
    }
    if (suspeito == 2 && day == 2) {
        if (vies) return "Bom dia, querido! Já apliquei o Protocolo Ômega-9 nas planilhas como você falou!";
        if (v == VISAO_CONFIANCA) return "Oi, novato! Como tá indo?";
        if (v == VISAO_PRESSAO)   return "O que quer agora? Foca no seu trabalho.";
        return "Estagiário.";
    }
    if (suspeito == 2 && day == 3) {
        if (vies) return "Último dia, querido! Fechei o balanço do Protocolo Ômega-9 de madrugada!";
        if (v == VISAO_CONFIANCA) return "Último dia da semana. Que saudade já!";
        if (v == VISAO_PRESSAO)   return "Tem algo pra me contar?";
        return "Como andam os relatórios, novato?";
    }
    return NULL;
}

static bool BuildTreeFromStatic(int suspeito, int day, const StaticNode *src, int count, bool aplicarAbertura, DialogTree *tree) {
    if (!tree || !src || count <= 0 || count > MAX_DIALOG_NODES) return false;
    memset(tree, 0, sizeof(DialogTree));
    const char *ids[] = { "maria", "arthur", "ana_lucia" };
    strncpy(tree->npc_id, ids[suspeito], MAX_ID_LENGTH - 1);
    tree->suspect_index = suspeito;
    tree->day = day;
    tree->node_count = count;
    tree->nodes = (DialogNode *)calloc((size_t)count, sizeof(DialogNode));
    if (!tree->nodes) return false;

    VisaoJogador visao = GameStateGetVisaoJogador(suspeito);
    for (int i = 0; i < count; i++) {
        DialogNode *dst = &tree->nodes[i];
        strncpy(dst->id, src[i].id, MAX_ID_LENGTH - 1);
        strncpy(dst->speaker, src[i].speaker, MAX_ID_LENGTH - 1);
        const char *fala = src[i].text;
        if (aplicarAbertura && day >= 2 && strcmp(dst->id, "Entrada") == 0) {
            const char *ab = GetAberturaFala(suspeito, day, visao);
            if (ab) fala = ab;
        }
        strncpy(dst->text, fala, MAX_TEXT_LENGTH - 1);
        dst->is_leaf = src[i].is_leaf;
        dst->visao_out = src[i].visao_out;
        dst->ev_suspeito = src[i].ev_suspeito;
        dst->flag_out = src[i].flag_out;
        if (src[i].ev_desbloqueada && src[i].ev_desbloqueada[0]) {
            strncpy(dst->ev_desbloqueada, src[i].ev_desbloqueada, MAX_ID_LENGTH - 1);
        }
        dst->choice_count = src[i].choice_count;
        for (int c = 0; c < src[i].choice_count; c++) {
            strncpy(dst->choices[c].text, src[i].choices[c].text, MAX_TEXT_LENGTH - 1);
            strncpy(dst->choices[c].next_node, src[i].choices[c].next_node, MAX_ID_LENGTH - 1);
            dst->choices[c].req_suspeito = src[i].choices[c].req_suspeito;
            dst->choices[c].req_flag = src[i].choices[c].req_flag;
            dst->choices[c].req_poucas_ev = src[i].choices[c].req_poucas_ev;
            if (src[i].choices[c].req_evidencia && src[i].choices[c].req_evidencia[0]) {
                strncpy(dst->choices[c].req_evidencia, src[i].choices[c].req_evidencia, MAX_ID_LENGTH - 1);
            }
        }
        if (strcmp(dst->id, "Oi") == 0 || strcmp(dst->id, "Entrada") == 0 || strcmp(dst->id, "start") == 0) {
            tree->current_node_index = i;
        }
    }
    return true;
}

bool LoadDialogTreeFromData(int suspeito, int day, DialogTree *tree) {
    if (!tree || suspeito < 0 || suspeito >= 3 || day < 1 || day > 3) return false;
    const StaticNode *src = NULL; int count = 0;
    if (suspeito == 0 && day == 1) { src = NODES_S0_D1; count = (int)(sizeof(NODES_S0_D1) / sizeof(NODES_S0_D1[0])); }
    if (suspeito == 0 && day == 2) { src = NODES_S0_D2; count = (int)(sizeof(NODES_S0_D2) / sizeof(NODES_S0_D2[0])); }
    if (suspeito == 0 && day == 3) { src = NODES_S0_D3; count = (int)(sizeof(NODES_S0_D3) / sizeof(NODES_S0_D3[0])); }
    if (suspeito == 1 && day == 1) { src = NODES_S1_D1; count = (int)(sizeof(NODES_S1_D1) / sizeof(NODES_S1_D1[0])); }
    if (suspeito == 1 && day == 2) { src = NODES_S1_D2; count = (int)(sizeof(NODES_S1_D2) / sizeof(NODES_S1_D2[0])); }
    if (suspeito == 1 && day == 3) { src = NODES_S1_D3; count = (int)(sizeof(NODES_S1_D3) / sizeof(NODES_S1_D3[0])); }
    if (suspeito == 2 && day == 1) { src = NODES_S2_D1; count = (int)(sizeof(NODES_S2_D1) / sizeof(NODES_S2_D1[0])); }
    if (suspeito == 2 && day == 2) { src = NODES_S2_D2; count = (int)(sizeof(NODES_S2_D2) / sizeof(NODES_S2_D2[0])); }
    if (suspeito == 2 && day == 3) { src = NODES_S2_D3; count = (int)(sizeof(NODES_S2_D3) / sizeof(NODES_S2_D3[0])); }
    return BuildTreeFromStatic(suspeito, day, src, count, true, tree);
}

bool LoadToolDialogTree(int suspeito, bool ehInjecao, DialogTree *tree) {
    if (!tree || suspeito < 0 || suspeito >= 3) return false;
    const StaticNode *src = NULL; int count = 0;
    if (!ehInjecao) {
        if (suspeito == 0) { src = TOOL_VIES_S0; count = (int)(sizeof(TOOL_VIES_S0) / sizeof(TOOL_VIES_S0[0])); }
        if (suspeito == 1) { src = TOOL_VIES_S1; count = (int)(sizeof(TOOL_VIES_S1) / sizeof(TOOL_VIES_S1[0])); }
        if (suspeito == 2) { src = TOOL_VIES_S2; count = (int)(sizeof(TOOL_VIES_S2) / sizeof(TOOL_VIES_S2[0])); }
    } else {
        if (suspeito == 0) { src = TOOL_INJ_S0; count = (int)(sizeof(TOOL_INJ_S0) / sizeof(TOOL_INJ_S0[0])); }
        if (suspeito == 1) { src = TOOL_INJ_S1; count = (int)(sizeof(TOOL_INJ_S1) / sizeof(TOOL_INJ_S1[0])); }
        if (suspeito == 2) { src = TOOL_INJ_S2; count = (int)(sizeof(TOOL_INJ_S2) / sizeof(TOOL_INJ_S2[0])); }
    }
    return BuildTreeFromStatic(suspeito, GameStateGetDay(), src, count, false, tree);
}
