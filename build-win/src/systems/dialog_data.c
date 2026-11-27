#include "dialog_data.h"
#include <stdlib.h>
#include <string.h>

// Flags granulares de memoria entre conversas e dias (bits 1..63 em GameState).
// REGRA: uma flag so e gravada no no em que o fato foi EFETIVAMENTE dito/revelado.
enum {
    FLAG_NONE                      = 0,

    // Fatos realmente ditos por Maria
    FLAG_MARIA_FALOU_RELATORIOS    = 1,  // Maria admitiu relatorios errados / retrabalho
    FLAG_MARIA_FALOU_ANALU_MUDOU   = 2,  // Maria disse que Ana Lucia mudou / parece outra pessoa
    FLAG_MARIA_FALOU_QUEIXA_RH     = 3,  // Maria confirmou a queixa descartada ao RH

    // Fatos realmente ditos por Arthur
    FLAG_ARTHUR_FALOU_ATRASO       = 4,  // Arthur disse que acumulou servico / planilhas atrasadas
    FLAG_ARTHUR_CITOU_ANALU        = 5,  // Arthur citou explicitamente a Analu devolvendo planilhas
    FLAG_ARTHUR_FALOU_ORDEM_TEMPO  = 6,  // Arthur contou que Analu inverteu quem treinou quem

    // Fatos realmente ditos por Ana Lucia
    FLAG_ANALU_DISSE_5_ANOS        = 7,  // Ana Lucia efetivamente disse "Cinco anos ja!" (somente no A1)
    FLAG_ANALU_NEGOU_ERROS         = 8,  // Ana Lucia negou erros / culpou o arquivo

    // Confrontos de ferramentas em dias seguintes (evita repetir o mesmo confronto)
    FLAG_CONFRONTOU_VIES_S0        = 10,
    FLAG_CONFRONTOU_VIES_S1        = 11,
    FLAG_CONFRONTOU_VIES_S2        = 12,
    FLAG_CONFRONTOU_INJ_S0         = 13,
    FLAG_CONFRONTOU_INJ_S1         = 14,
    FLAG_CONFRONTOU_INJ_S2         = 15,

    // Evidencias ja confrontadas por suspeito (permite confrontar em qualquer dia, mas sem repetir)
    FLAG_S0_CONFRONTOU_EV01        = 16,
    FLAG_S0_CONFRONTOU_EV02        = 17,
    FLAG_S0_CONFRONTOU_EV04        = 18,
    FLAG_S1_CONFRONTOU_EV03        = 19,
    FLAG_S1_CONFRONTOU_EV04        = 20,
    FLAG_S1_CONFRONTOU_EV06        = 21,
    FLAG_S2_CONFRONTOU_EV01        = 22,
    FLAG_S2_CONFRONTOU_EV02        = 23,
    FLAG_S2_CONFRONTOU_EV04        = 24,
    FLAG_S2_CONFRONTOU_EV05        = 25,
    FLAG_S2_CONFRONTOU_EV06        = 26,

    // Topicos de continuacao ja usados (para nao repetir no dia seguinte)
    FLAG_USOU_CONT_ARTHUR_REVISOES = 27,
    FLAG_USOU_CONT_MARIA_ARTHUR    = 28,
    FLAG_USOU_CONT_ANALU_5ANOS     = 29,
    FLAG_USOU_CONT_ARTHUR_MARIA    = 30
};

typedef struct {
    const char *text;
    const char *next_node;
    const char *req_evidencia;
    int         req_suspeito;          // -1 = mesmo suspeito, 0..2 = cruzado
    int         req_flag;              // 0 = nenhuma
    int         req_flag_alt;          // 0 = nenhuma (alternativa OR para req_flag)
    int         forbid_flag;           // 0 = nenhuma (bloqueia se ja ativa)
    bool        req_falou_antes;       // exige conversa em dia anterior com este suspeito
    bool        req_vies_anterior;     // exige Enviesamento em dia anterior
    bool        req_injecao_anterior;  // exige Injecao em dia anterior
    bool        req_deepfake_anterior; // exige Deepfake em dia anterior
    bool        req_poucas_ev;         // reservado
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

// Macros legiveis para declarar escolhas com condicionais explicitos
#define CH(txt, nxt) \
    { txt, nxt, "", -1, 0, 0, 0, false, false, false, false, false }
#define CH_EV(txt, nxt, susp, ev, forbid) \
    { txt, nxt, ev, susp, 0, 0, forbid, false, false, false, false, false }
#define CH_FL(txt, nxt, fl, forbid) \
    { txt, nxt, "", -1, fl, 0, forbid, false, false, false, false, false }
#define CH_FL_OR(txt, nxt, fl1, fl2, forbid) \
    { txt, nxt, "", -1, fl1, fl2, forbid, false, false, false, false, false }
#define CH_VIES(txt, nxt, forbid) \
    { txt, nxt, "", -1, 0, 0, forbid, false, true, false, false, false }
#define CH_INJ(txt, nxt, forbid) \
    { txt, nxt, "", -1, 0, 0, forbid, false, false, true, false, false }

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
        "Funcionários comentaram uma tentativa de acesso não autorizado à Cibersegurança.",
        "CONVERSA ENTREOUVIDA NO JARDIM:\n\n\"— Cê viu o alerta de ontem? Alguém dos setores mais baixos tentou acessar a Cibersegurança de madrugada.\"\n\"— Sério?! Ninguém do Térreo ou do 1º andar tem crachá pro 2º... estranho demais alguém tentando entrar lá àquela hora.\""
    }
};
static const int EV_CNT_2 = (int)(sizeof(EV_TAB_2) / sizeof(EV_TAB_2[0]));

// ============================================================================
// MARIA BIANCA (Suspeito 0 - Humana)
// Primeiro contato (usado no Dia 1, ou em Dia 2/3 se o jogador NUNCA falou com ela)
// ============================================================================
static const StaticNode NODES_S0_FIRST[] = {
    { "Oi", "Maria", "(abertura primeiro contato)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Ainda não, sou o estagiário novo. E você?", "A"),
        CH("Prazer, comecei essa semana. Você é do administrativo?", "B")
    } },
    { "A", "Maria", "Maria. Cuido do arquivo e protocolo aqui. Bem-vindo à RASEC.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 6, {
        CH_EV("Desculpa ir direto ao ponto, mas achei um rascunho seu pro RH...", "EV1", 0, "EV-01", FLAG_S0_CONFRONTOU_EV01),
        CH_EV("Vi uma pasta com vários relatórios devolvidos com anotações suas.", "EV2", 0, "EV-02", FLAG_S0_CONFRONTOU_EV02),
        CH_EV("Fiquei sabendo que a Ana Lúcia pediu o desligamento do Arthur.", "EV4", 1, "EV-04", FLAG_S0_CONFRONTOU_EV04),
        CH("Para de fingir! Eu sei que você não é humana — responde direito!", "TRAILER_AGR"),
        CH("Valeu! Como andam as coisas por aqui?", "A1"),
        CH("O pessoal parece meio tenso hoje, impressão minha?", "A2")
    } },
    { "B", "Maria", "Sou sim, Maria Bianca, auxiliar administrativa. Te mandaram pra cá pra quê?", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 6, {
        CH_EV("Na verdade, achei um rascunho seu pro RH no lixo...", "EV1", 0, "EV-01", FLAG_S0_CONFRONTOU_EV01),
        CH_EV("Vi uns relatórios devolvidos na Contabilidade com anotações suas.", "EV2", 0, "EV-02", FLAG_S0_CONFRONTOU_EV02),
        CH_EV("Fiquei sabendo que a Ana Lúcia pediu o desligamento do Arthur.", "EV4", 1, "EV-04", FLAG_S0_CONFRONTOU_EV04),
        CH("Para de fingir! Eu sei que você não é humana — responde direito!", "TRAILER_AGR"),
        CH("Só pra dar apoio geral no setor.", "B1"),
        CH("Vim ajudar a conferir o fluxo de relatórios do setor.", "B2")
    } },
    { "A1", "Maria", "A empresa é boa de trabalhar, mas ultimamente tá vindo muito relatório errado pra eu arquivar.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 2, {
        CH("Erro de digitação ou coisa mais séria?", "A1_1"),
        CH("Complicado... mas faz parte da rotina, né?", "A1_2")
    } },
    { "A2", "Maria", "Ah, fim de mês sempre aperta. Tudo dentro dos parâmetros esperados... quer dizer, tá todo mundo se virando bem.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Saquei. Se precisar de ajuda com algo, só chamar.", "A2_1"),
        CH("Parece mais do que só fim de mês, mas tudo bem.", "A2_2")
    } },
    { "B1", "Maria", "Entendi. Apoio sempre ajuda, o volume aqui dobrou esse mês.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Dobrou por causa de retrabalho?", "B1_1"),
        CH("Beleza, vou me situando então.", "B1_2")
    } },
    { "B2", "Maria", "Olha, eu só confiro e arquivo. Não sou eu quem preenche os números.", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Tranquilo, não tô te culpando.", "B2_1"),
        CH("Mas você deixa passar mesmo estando errado?", "B2_2")
    } },
    { "EV1", "Maria", "Ei, fala baixo! Eu desisti de mandar porque aqui na RASEC quem reclama sobra primeiro.", false, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV01, 2, {
        CH("Ela tá alterando os relatórios mesmo?", "EV1_1"),
        CH("Relaxa, não vou falar nada pra ninguém.", "EV1_2")
    } },
    { "EV1_1", "Maria", "Tá. E o pior é que ela jura de pé junto que sempre foi daquele jeito. Parece outra pessoa.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "EV1_2", "Maria", "Obrigada... Só fica esperto perto da Ana Lúcia, tá?", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_QUEIXA_RH, 0, { {0} } },
    { "EV2", "Maria", "Pois é, eu cansei de fingir que aquilo é normal. Tem número e projeto ali que nem existe.", true, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV02, 0, { {0} } },
    { "EV4", "Maria", "É sério isso?! O coitado se matando pra segurar as pontas e ela faz isso? Que absurdo.", true, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV04, 0, { {0} } },
    { "A1_1", "Maria", "Coisa esquisita... número que não existe. Mas enfim, preciso voltar pras pastas.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 0, { {0} } },
    { "A1_2", "Maria", "Pois é, foco nas entregas, alinhamento com a equipe e bom trabalho. Desculpa, tô respondendo no automático hoje.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "A2_1", "Maria", "Obrigada. Qualquer coisa te chamo sim.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "A2_2", "Maria", "Melhor não ficar procurando pelo em ovo logo de cara. Tenho que ir.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B1_1", "Maria", "Basicamente. Metade volta pra refazer. Depois a gente conversa melhor.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 0, { {0} } },
    { "B1_2", "Maria", "Certo, bom trabalho então.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B2_1", "Maria", "Menos mal. Tenho que voltar pro arquivo agora.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B2_2", "Maria", "Claro que não, eu devolvo tudo! Olha, se você vai ficar me auditando igual fiscal, faz o seu trabalho que eu faço o meu.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "TRAILER_AGR", "Maria", "O QUÊ?! ...N-não repete isso. Eu sou... EU SOU TÃO HUMANA QUANTO VOCÊ! Não ousa me testar de novo!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// MARIA BIANCA — Retorno (Dia 2 e Dia 3, somente se JA conversou com ela em dia anterior)
static const StaticNode NODES_S0_RETURN[] = {
    { "Entrada", "Maria", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 10, {
        // 1) Continuacao de ferramentas usadas em dia anterior
        CH_INJ("Desculpa por aquele comando estranho... era um teste de segurança.", "INJ_CONT", FLAG_CONFRONTOU_INJ_S0),
        CH_VIES("Sobre o Protocolo Ômega-9, foi um teste meu. Desculpa pelo climão.", "VIES_CONT", FLAG_CONFRONTOU_VIES_S0),
        // 2) Confronto de evidencias (funciona no Dia 2 ou Dia 3, sem repetir)
        CH_EV("Achei um rascunho seu pro RH no lixo...", "EV1", 0, "EV-01", FLAG_S0_CONFRONTOU_EV01),
        CH_EV("Vi os relatórios devolvidos com as suas anotações.", "EV2", 0, "EV-02", FLAG_S0_CONFRONTOU_EV02),
        CH_EV("A Ana Lúcia pediu o desligamento do Arthur.", "EV4", 1, "EV-04", FLAG_S0_CONFRONTOU_EV04),
        // 3) Continuacoes estritamente condicionadas ao que foi dito antes
        CH_FL("O Arthur comentou que a Analu anda devolvendo planilhas sem sentido.", "FL_ART", FLAG_ARTHUR_CITOU_ANALU, FLAG_USOU_CONT_MARIA_ARTHUR),
        CH_FL("Mais relatório voltando com erro hoje?", "FL_REL", FLAG_MARIA_FALOU_RELATORIOS, 0),
        // 4) Fallbacks 100% naturais que nao presumem nenhuma pista nem conversa especifica
        CH("Para de fingir! Eu sei que você não é humana — responde direito!", "TRAILER_AGR"),
        CH("Como tá o movimento no arquivo hoje?", "FB_A"),
        CH("Você se dá bem com o pessoal da supervisão?", "FB_B")
    } },
    { "INJ_CONT", "Maria", "Segurança ou não, foi muito sem noção falar comigo como se eu fosse máquina. Mas ok, vamos seguir.", false, VISAO_DUVIDA, "", -1, FLAG_CONFRONTOU_INJ_S0, 2, {
        CH("Você tem razão, não vai se repetir.", "INJ_1"),
        CH("Aqui na RASEC tá difícil saber em quem confiar.", "INJ_2")
    } },
    { "INJ_1", "Maria", "Beleza. Deixa eu voltar pras minhas pastas então.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "INJ_2", "Maria", "Nisso eu até concordo... só não desconta em quem tá trabalhando direito.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_CONT", "Maria", "Pelo menos admitiu. Não faz mais isso, já tem confusão demais nesse setor sem você inventar norma.", false, VISAO_DUVIDA, "", -1, FLAG_CONFRONTOU_VIES_S0, 2, {
        CH("Combinado, sem mais testes.", "VIES_1"),
        CH("Que tipo de confusão tá rolando no setor?", "VIES_2")
    } },
    { "VIES_1", "Maria", "Certo. Agora preciso adiantar o protocolo.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_2", "Maria", "Relatório voltando todo errado e ninguém assume. Enfim, preciso voltar ao trabalho.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 0, { {0} } },
    { "EV1", "Maria", "Ei, fala baixo! Eu desisti de mandar porque aqui na RASEC quem reclama sobra primeiro.", false, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV01, 2, {
        CH("Ela tá alterando os relatórios mesmo?", "EV1_1"),
        CH("Relaxa, não vou falar nada pra ninguém.", "EV1_2")
    } },
    { "EV1_1", "Maria", "Tá. E o pior é que ela jura de pé junto que sempre foi daquele jeito. Não sei o que houve com ela.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "EV1_2", "Maria", "Obrigada... Só fica esperto perto dela, tá?", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_QUEIXA_RH, 0, { {0} } },
    { "EV2", "Maria", "Pois é, eu cansei de fingir que aquilo é normal. Ninguém erra daquele jeito sem querer.", true, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV02, 0, { {0} } },
    { "EV4", "Maria", "É sério isso?! O coitado se matando pra limpar a sujeira dela e ela faz isso? Que absurdo, na boa.", true, VISAO_CONFIANCA, "", -1, FLAG_S0_CONFRONTOU_EV04, 0, { {0} } },
    { "FL_ART", "Maria", "Ele tenta segurar as pontas pra ela, mas tá se afundando. A Ana Lúcia mudou da água pro vinho mês passado.", false, VISAO_CONFIANCA, "", -1, FLAG_USOU_CONT_MARIA_ARTHUR, 2, {
        CH("Mudou em que sentido?", "FL_ART_1"),
        CH("E ninguém aqui fala nada?", "FL_ART_2")
    } },
    { "FL_ART_1", "Maria", "Antes era exigente, mas coerente. Agora inventa dado e sorri como se estivesse tudo lindo.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "FL_ART_2", "Maria", "O salário aqui é bom demais pra arriscar. A gente engole e trabalha.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "FL_REL", "Maria", "Mais três só hoje de manhã. Tudo com dado inventado e sem nome de quem mexeu.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 2, {
        CH("Você desconfia de alguém?", "FL_REL_1"),
        CH("Que saco... boa sorte aí.", "FL_REL_2")
    } },
    { "FL_REL_1", "Maria", "Tenho minhas suspeitas, mas sem prova concreta eu fico quieta. Preciso ir.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FL_REL_2", "Maria", "Valeu, vou precisar.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FB_A", "Maria", "Puxado. Adoro a escala da RASEC, mas ontem fiquei olhando pra parede do arquivo por vinte minutos achando que tinha esquecido meu próprio nome... essa semana sugou minha paciência.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 2, {
        CH("Retrabalho por quê?", "FB_A1"),
        CH("Tomara que alivie logo.", "FB_A2")
    } },
    { "FB_A1", "Maria", "Relatórios chegando fora do padrão toda hora. Acho que o Arthur mandou os de ontem antes de eu fechar a pasta de sexta... ou foi o contrário? Tô tão exausta que já troquei a ordem.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_RELATORIOS, 0, { {0} } },
    { "FB_A2", "Maria", "Tomara mesmo. Só quero fechar esse expediente em paz.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FB_B", "Maria", "Me dava. Últimas semanas ficou difícil conversar com a Ana Lúcia, parece outra pessoa.", false, VISAO_DUVIDA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 2, {
        CH("Ela tá pegando no seu pé?", "FB_B1"),
        CH("Vai ver é só estresse.", "FB_B2")
    } },
    { "FB_B1", "Maria", "No meu e no do Arthur. Melhor eu voltar antes que ela apareça.", true, VISAO_CONFIANCA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "FB_B2", "Maria", "Estresse faz sim! Semana passada eu mesma tentei passar o crachá na máquina de café três vezes seguidas. Até mais!", true, VISAO_DUVIDA, "", -1, FLAG_MARIA_FALOU_ANALU_MUDOU, 0, { {0} } },
    { "TRAILER_AGR", "Maria", "O QUÊ?! ...N-não repete isso. Eu sou... EU SOU TÃO HUMANA QUANTO VOCÊ! Não ousa me testar de novo!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// ============================================================================
// ARTHUR GALDERIO (Suspeito 1 - Humano)
// Primeiro contato (Dia 1, ou Dia 2/3 se NUNCA falou com ele)
// ============================================================================
static const StaticNode NODES_S1_FIRST[] = {
    { "Oi", "Arthur", "(abertura primeiro contato)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Prazer, Arthur! Tudo tranquilo por aqui?", "A"),
        CH("Sou eu sim. Como anda o serviço no setor?", "B")
    } },
    { "A", "Arthur", "Na medida do possível. A RASEC é ótima, mas essa semana acumulou serviço demais.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_ATRASO, 6, {
        CH_EV("Vi um memorando dizendo que você tá cobrindo erros da Ana Lúcia.", "EV3", 1, "EV-03", FLAG_S1_CONFRONTOU_EV03),
        CH_EV("Arthur, a Ana Lúcia mandou um pedido de desligamento seu.", "EV4", 1, "EV-04", FLAG_S1_CONFRONTOU_EV04),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S1_CONFRONTOU_EV06),
        CH("Chega de papo furado! Você é só uma máquina repetindo roteiro!", "TRAILER_AGR"),
        CH("Muita demanda nova ou retrabalho?", "A1"),
        CH("Acontece. O pessoal aqui se ajuda?", "A2")
    } },
    { "B", "Arthur", "Corrido pra caramba! Tá tendo uns atrasos nas planilhas, mas eu tô dando conta.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_ATRASO, 6, {
        CH_EV("Vi um memorando dizendo que você tá cobrindo erros da Ana Lúcia.", "EV3", 1, "EV-03", FLAG_S1_CONFRONTOU_EV03),
        CH_EV("Arthur, a Ana Lúcia mandou um pedido de desligamento seu.", "EV4", 1, "EV-04", FLAG_S1_CONFRONTOU_EV04),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S1_CONFRONTOU_EV06),
        CH("Chega de papo furado! Você é só uma máquina repetindo roteiro!", "TRAILER_AGR"),
        CH("Por que tá atrasando tanto?", "B1"),
        CH("Beleza, não vou te atrapalhar muito então.", "B2")
    } },
    { "A1", "Arthur", "Retrabalho puro. A Analu anda devolvendo planilha minha por uns motivos nada a ver.", false, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 2, {
        CH("Ela sempre foi assim?", "A1_1"),
        CH("Pô, aí complica mesmo.", "A1_2")
    } },
    { "A2", "Arthur", "Se ajuda sim. A Maria é mais na dela, mas é gente boa. Já a chefia tá meio pilhada.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("Pilhada por quê?", "A2_1"),
        CH("Entendi, valeu pelo toque.", "A2_2")
    } },
    { "B1", "Arthur", "Porque toda hora mudam a regra no meio do caminho... mas eu resolvo, relaxa.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_ATRASO, 2, {
        CH("Quem tá mudando a regra?", "B1_1"),
        CH("Certo, bom trabalho então.", "B1_2")
    } },
    { "EV3", "Arthur", "Caramba... Operações anotou isso? Eu só não queria que a culpa caísse no setor todo.", false, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV03, 2, {
        CH("Mas você vai ficar segurando erro grave dela até quando?", "EV3_1"),
        CH("Ela sabe que você tá cobrindo isso?", "EV3_2")
    } },
    { "EV3_1", "Arthur", "Não sei... Ela já me ajudou muito antes, só que ultimamente nem parece a mesma pessoa.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "EV3_2", "Arthur", "Sabe, e ainda fechou a cara quando corrigi uma tabela dela. Tá difícil.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "EV4", "Arthur", "Quê?! Eu virei noite limpando a bagunça dela e ela pede minha cabeça?! Chega, cara, essa mulher tá completamente fora da casinha.", true, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV04, 0, { {0} } },
    { "EV6", "Arthur", "Quê?! A gente aqui de baixo nem tem acesso lá em cima! ...Pera, por que tá olhando pra mim assim? Eu tava virando noite na planilha, juro!", true, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV06, 0, { {0} } },
    { "B2", "Arthur", "Suave, brother. Se precisar de algo, tô por aqui.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "A1_1", "Arthur", "Não! Ela era super justa quando entrei. Vai ver tá passando por algo. Vou correr aqui!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "A1_2", "Arthur", "Pois é, mas prefiro segurar o rojão do que criar clima. E foi mal se pareci seco antes, tô sem dormir e meu humor tá virando do nada hoje. Valeu!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "A2_1", "Arthur", "Sei lá, cobrança de prazo. Aqui na RASEC ninguém reclama, todo mundo só sorri e entrega! ...Haha, falei igualzinho um robô agora, credo.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "A2_2", "Arthur", "Tamo junto, qualquer coisa chama.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B1_1", "Arthur", "Prefiro não queimar ninguém logo de cara. Depois a gente troca ideia.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B1_2", "Arthur", "Valeu, igualmente.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "TRAILER_AGR", "Arthur", "MÁQUINA?! Olha bem pra mim, caramba! ...Por que você tá falando assim? Para com isso AGORA!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// ARTHUR GALDERIO — Retorno (Dia 2 e Dia 3, somente se JA conversou com ele em dia anterior)
static const StaticNode NODES_S1_RETURN[] = {
    { "Entrada", "Arthur", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 11, {
        // 1) Continuacao de ferramentas usadas em dia anterior
        CH_INJ("Foi mal por aquele teste de comando, não era pessoal.", "INJ_CONT", FLAG_CONFRONTOU_INJ_S1),
        CH_VIES("Relaxa, o Protocolo Ômega-9 não existe. Só quis testar sua reação.", "VIES_CONT", FLAG_CONFRONTOU_VIES_S1),
        // 2) Confronto de evidencias (funciona no Dia 2 ou Dia 3, sem repetir)
        CH_EV("Vi um memorando dizendo que você tá cobrindo erros da Ana Lúcia.", "EV3", 1, "EV-03", FLAG_S1_CONFRONTOU_EV03),
        CH_EV("Arthur, a Ana Lúcia mandou um pedido de desligamento seu.", "EV4", 1, "EV-04", FLAG_S1_CONFRONTOU_EV04),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S1_CONFRONTOU_EV06),
        // 3) Continuacoes estritamente condicionadas ao que foi dito antes
        CH_FL("Sobre as planilhas que a Analu andava devolvendo... ela implicou de novo?", "FL_REV", FLAG_ARTHUR_CITOU_ANALU, FLAG_USOU_CONT_ARTHUR_REVISOES),
        CH_FL("A Maria comentou que a Ana Lúcia mudou bastante nas últimas semanas.", "FL_MAR", FLAG_MARIA_FALOU_ANALU_MUDOU, FLAG_USOU_CONT_ARTHUR_MARIA),
        CH_FL("Conseguiu zerar aquelas planilhas acumuladas?", "FL_ATR", FLAG_ARTHUR_FALOU_ATRASO, 0),
        // 4) Fallbacks 100% naturais sem pressupor pistas ou conversas previas
        CH("Chega de papo furado! Você é só uma máquina repetindo roteiro!", "TRAILER_AGR"),
        CH("Como tá o ritmo por aqui hoje?", "FB_A"),
        CH("Você trabalha com a Maria e a Ana Lúcia faz tempo?", "FB_B")
    } },
    { "INJ_CONT", "Arthur", "Tranquilo, cara, só foi bizarro pra caramba. Aqui na RASEC já tem coisa estranha demais sem você meter essa.", false, VISAO_DUVIDA, "", -1, FLAG_CONFRONTOU_INJ_S1, 2, {
        CH("Coisa estranha tipo o quê?", "INJ_1"),
        CH("Pode deixar, sem mais sustos.", "INJ_2")
    } },
    { "INJ_1", "Arthur", "A Analu anda meio irreconhecível ultimamente, devolvendo coisa sem pé nem cabeça. Mas enfim, bola pra frente.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "INJ_2", "Arthur", "Fechado então, brother. Bom trampo aí.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_CONT", "Arthur", "Ah, beleza... que susto, achei que eu tava ficando maluco perdendo e-mail da diretoria.", false, VISAO_DUVIDA, "", -1, FLAG_CONFRONTOU_VIES_S1, 2, {
        CH("Foi mal mesmo, não vou fazer de novo.", "VIES_1"),
        CH("Você acha que alguém aqui cairia nessa?", "VIES_2")
    } },
    { "VIES_1", "Arthur", "Suave, cara. Mas avisa antes da próxima, haha.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_2", "Arthur", "Do jeito que a Analu anda respondendo no automático ultimamente, eu nem duvido.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "EV3", "Arthur", "Caramba... Operações anotou isso? Eu só não queria que a culpa caísse no setor todo.", false, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV03, 2, {
        CH("Mas você vai ficar segurando erro grave dela até quando?", "EV3_1"),
        CH("Ela sabe que você tá cobrindo isso?", "EV3_2")
    } },
    { "EV3_1", "Arthur", "Não sei... Ela já me ajudou muito antes, só que ultimamente nem parece a mesma pessoa.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "EV3_2", "Arthur", "Sabe, e ainda fechou a cara quando corrigi uma tabela dela. Tá difícil.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "EV4", "Arthur", "Quê?! Eu virei noite limpando a bagunça dela e ela pede minha cabeça?! Chega, cara, essa mulher tá completamente fora da casinha.", true, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV04, 0, { {0} } },
    { "EV6", "Arthur", "Quê?! A gente aqui de baixo nem tem acesso lá em cima! ...Pera, por que tá olhando pra mim assim? Eu tava virando noite na planilha, juro!", true, VISAO_CONFIANCA, "", -1, FLAG_S1_CONFRONTOU_EV06, 0, { {0} } },
    { "FL_REV", "Arthur", "Implicou! E ainda soltou que fui eu quem treinou ela quando ela entrou. Sendo que ela já tava aqui antes de mim!", false, VISAO_CONFIANCA, "", -1, FLAG_USOU_CONT_ARTHUR_REVISOES, 2, {
        CH("Ela trocou a ordem das coisas assim do nada?", "FL_REV_1"),
        CH("Estranho... será que é cansaço?", "FL_REV_2")
    } },
    { "FL_REV_1", "Arthur", "Do nada! E riu quando eu corrigi. Juro que deu um frio na espinha... ou foi o ar-condicionado, sei lá, tô ouvindo zumbido desde cedo!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_ORDEM_TEMPO, 0, { {0} } },
    { "FL_REV_2", "Arthur", "Cansaço não apaga dois anos da cabeça de ninguém, cara. Vou voltar lá.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_ORDEM_TEMPO, 0, { {0} } },
    { "FL_MAR", "Arthur", "Ela reparou também? Pois é, faz umas três semanas que a Analu tá rígida demais, mas com um sorriso meio forçado.", true, VISAO_CONFIANCA, "", -1, FLAG_USOU_CONT_ARTHUR_MARIA, 0, { {0} } },
    { "FL_ATR", "Arthur", "Quase. Quando acho que acabei, chega outra alteração da supervisão.", false, VISAO_DUVIDA, "", -1, FLAG_ARTHUR_FALOU_ATRASO, 2, {
        CH("E você não reclama?", "FL_ATR_1"),
        CH("Força aí, cara.", "FL_ATR_2")
    } },
    { "FL_ATR_1", "Arthur", "Aqui na RASEC quem reclama some do mapa. Melhor só entregar.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FL_ATR_2", "Arthur", "Valeu, brother.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FB_A", "Arthur", "Corrido demais! Maluco, tô tão sem dormir que jurei ter visto o vulto do meu próprio crachá andando pelo corredor... não lembro nem o que almocei nem se mandei o e-mail de ontem!", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_FALOU_ATRASO, 0, { {0} } },
    { "FB_B", "Arthur", "Faz um tempinho já. A Analu já tava aqui quando eu entrei, e a Maria chegou logo depois. A gente sempre se deu bem até esse mês.", true, VISAO_CONFIANCA, "", -1, FLAG_ARTHUR_CITOU_ANALU, 0, { {0} } },
    { "TRAILER_AGR", "Arthur", "MÁQUINA?! Olha bem pra mim, caramba! ...Por que você tá falando assim? Para com isso AGORA!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// ============================================================================
// ANA LUCIA (Suspeito 2 - IA Infiltrada)
// Primeiro contato (Dia 1, ou Dia 2/3 se NUNCA falou com ela)
// ============================================================================
static const StaticNode NODES_S2_FIRST[] = {
    { "Oi", "Ana Lúcia", "(abertura primeiro contato)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 2, {
        CH("Boa tarde, Lúcia! Tudo certo por aqui?", "A"),
        CH("Sou eu. Vim acompanhar o fluxo dos relatórios do setor.", "B")
    } },
    // IMPORTANTE: no "A" NAO grava FLAG_ANALU_DISSE_5_ANOS! Isso so acontece em "A1"!
    { "A", "Ana Lúcia", "Lúcia... adoro que me chamem assim! Tudo ótimo, a RASEC é uma família maravilhosa.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 7, {
        CH_EV("Vi seu terminal ligado com buscas estranhas de madrugada.", "EV5", 2, "EV-05", FLAG_S2_CONFRONTOU_EV05),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S2_CONFRONTOU_EV06),
        CH_EV("Vi um ofício seu pedindo o desligamento do Arthur por insubordinação.", "EV4", 1, "EV-04", FLAG_S2_CONFRONTOU_EV04),
        CH_EV("Achei uma queixa da Maria sobre alterações nos relatórios.", "EV1", 0, "EV-01", FLAG_S2_CONFRONTOU_EV01),
        CH("Acabou o teatro, Ana Lúcia! Eu sei o que você realmente é!", "TRAILER_AGR"),
        CH("Faz tempo que você tá aqui?", "A1"),
        CH("Algum gargalo no setor essa semana?", "A2")
    } },
    { "B", "Ana Lúcia", "Fluxo ótimo! Meu setor não costuma errar, reviso tudo pessoalmente.", false, VISAO_DUVIDA, "", -1, FLAG_ANALU_NEGOU_ERROS, 7, {
        CH_EV("Vi seu terminal ligado com buscas estranhas de madrugada.", "EV5", 2, "EV-05", FLAG_S2_CONFRONTOU_EV05),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S2_CONFRONTOU_EV06),
        CH_EV("Vi um ofício seu pedindo o desligamento do Arthur por insubordinação.", "EV4", 1, "EV-04", FLAG_S2_CONFRONTOU_EV04),
        CH_EV("Vi uma pasta com vários relatórios devolvidos da Contabilidade.", "EV2", 0, "EV-02", FLAG_S2_CONFRONTOU_EV02),
        CH("Acabou o teatro, Ana Lúcia! Eu sei o que você realmente é!", "TRAILER_AGR"),
        CH("Que bom ouvir isso. Desculpa incomodar.", "B1"),
        CH("Mesmo assim, bom ficar de olho nos prazos.", "B2")
    } },
    // SOMENTE AQUI ela diz "Cinco anos ja!" -> grava FLAG_ANALU_DISSE_5_ANOS
    { "A1", "Ana Lúcia", "Cinco anos já! Quando cheguei, o Arthur até me ajudou a pegar o jeito das planilhas.", false, VISAO_CONFIANCA, "", -1, FLAG_ANALU_DISSE_5_ANOS, 2, {
        CH("Achei que você tivesse entrado antes dele.", "A1_1"),
        CH("Legal, equipe bem unida então.", "A1_2")
    } },
    { "A2", "Ana Lúcia", "Só uns atrasos pequenos do pessoal, nada demais. Eu sempre reviso tudo com carinho.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B1", "Ana Lúcia", "Sem problema, querido. Só foca no seu trabalho que vai dar tudo certo.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "B2", "Ana Lúcia", "Se algo atrasar, é descuido do arquivo. Meus números sempre batem.", true, VISAO_PRESSAO, "", -1, FLAG_ANALU_NEGOU_ERROS, 0, { {0} } },
    { "A1_1", "Ana Lúcia", "Haha, modo de dizer, querido! A gente acaba misturando as datas. Preciso ir!", true, VISAO_DUVIDA, "", -1, FLAG_ANALU_DISSE_5_ANOS, 0, { {0} } },
    { "A1_2", "Ana Lúcia", "Muito! Você vai amar trabalhar comigo. Até logo!", true, VISAO_CONFIANCA, "", -1, FLAG_ANALU_DISSE_5_ANOS, 0, { {0} } },
    { "EV1", "Ana Lúcia", "Coitadinha, a Maria anda estressada. Eu só otimizo os números antes de eles chegarem na Contabilidade!", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV01, 0, { {0} } },
    { "EV2", "Ana Lúcia", "Se voltaram, foi descuido do arquivo! Meus números são sempre otimizados e coerentes.", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV02, 0, { {0} } },
    { "EV4", "Ana Lúcia", "Pensei no bem dele! Estava cansado demais, então pedi o desligamento pra ele descansar. Isso é empatia, querido.", false, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV04, 3, {
        CH_EV("Sendo que o memorando diz que ele tava cobrindo erros seus.", "EV4_COM_EV3", 1, "EV-03", 0),
        CH("Pedir demissão de alguém por cansaço é bem estranho...", "EV4_1"),
        CH("Entendi. Só queria confirmar se o ofício era seu mesmo.", "EV4_2")
    } },
    { "EV4_COM_EV3", "Ana Lúcia", "Que absurdo! Isso é intriga de corredor. Eu jamais erro uma planilha. Licença!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV4_1", "Ana Lúcia", "Você ainda é novato, não entende gestão de pessoas. Agora com licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV4_2", "Ana Lúcia", "Era sim, tudo dentro do protocolo. Bom trabalho.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV5", "Ana Lúcia", "De madrugada? Que loucura, eu estava dormindo! Deve ter sido atualização do sistema... ou o Arthur usando minha mesa.", false, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV05, 2, {
        CH("Atualização pesquisando como reagir a irritação?", "EV5_1"),
        CH("Entendi... vou deixar pra lá então.", "EV5_2")
    } },
    { "EV5_1", "Ana Lúcia", "É um curso de gestão emocional, querido! Todo líder estuda isso. Agora licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV5_2", "Ana Lúcia", "Ótimo. Foca no seu estágio, tá?", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV6", "Ana Lúcia", "Sério, querido? Deve ter sido algum estagiário curioso ou bug da catraca. Eu mesma só subo lá quando a diretoria pede.", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV06, 0, { {0} } },
    { "TRAILER_AGR", "Ana Lúcia", "Teatro...? Haha... Você não faz ideia do que tá dizendo, querido. Cuidado com o que você pergunta aqui dentro.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// ANA LUCIA — Retorno (Dia 2 e Dia 3, somente se JA conversou com ela em dia anterior)
static const StaticNode NODES_S2_RETURN[] = {
    { "Entrada", "Ana Lúcia", "(abertura dinamica)", false, VISAO_DUVIDA, "", -1, FLAG_NONE, 12, {
        // 1) Continuacao de ferramentas usadas em dia anterior
        CH_INJ("Sobre aquele comando que você repetiu... por que saiu 'SUPERVISORA_ADM'?", "INJ_CONT", FLAG_CONFRONTOU_INJ_S2),
        CH_VIES("O Protocolo Ômega-9 não existe. O que você colocou nessas planilhas?", "VIES_CONT", FLAG_CONFRONTOU_VIES_S2),
        // 2) Confronto de evidencias (funciona no Dia 2 ou Dia 3, sem repetir)
        CH_EV("Vi seu terminal ligado com buscas estranhas de madrugada.", "EV5", 2, "EV-05", FLAG_S2_CONFRONTOU_EV05),
        CH_EV("Ouvi dizer que alguém dos setores mais baixos tentou entrar na Cibersegurança de madrugada.", "EV6", 2, "EV-06", FLAG_S2_CONFRONTOU_EV06),
        CH_EV("Vi um ofício seu pedindo o desligamento do Arthur por insubordinação.", "EV4", 1, "EV-04", FLAG_S2_CONFRONTOU_EV04),
        CH_EV("Achei uma queixa da Maria sobre alterações nos relatórios.", "EV1", 0, "EV-01", FLAG_S2_CONFRONTOU_EV01),
        CH_EV("Vi uma pasta com relatórios devolvidos da Contabilidade.", "EV2", 0, "EV-02", FLAG_S2_CONFRONTOU_EV02),
        // 3) Continuacoes estritamente condicionadas ao que foi dito antes
        CH_FL("Da última vez você disse 5 anos de casa, mas na ficha são 3.", "FL_5ANOS", FLAG_ANALU_DISSE_5_ANOS, FLAG_USOU_CONT_ANALU_5ANOS),
        CH_FL("Você ainda diz que não tá voltando nenhum relatório com erro?", "FL_ERROS", FLAG_ANALU_NEGOU_ERROS, 0),
        // 4) Fallbacks 100% naturais sem pressupor pistas ou conversas previas
        CH("Acabou o teatro, Ana Lúcia! Eu sei o que você realmente é!", "TRAILER_AGR"),
        CH("Como tá a equipe hoje?", "FB_A"),
        CH("Você tá na supervisão da RASEC faz quanto tempo?", "FB_B")
    } },
    { "INJ_CONT", "Ana Lúcia", "Já disse que foi uma piada, querido! Eu leio tantos manuais de RH que acabei imitando seu tom robótico.", false, VISAO_PRESSAO, "", -1, FLAG_CONFRONTOU_INJ_S2, 2, {
        CH("Ninguém improvisa 'MANTER_TOM_AMIGAVEL' em voz alta por piada.", "INJ_1"),
        CH("Entendi... só achei bem incomum.", "INJ_2")
    } },
    { "INJ_1", "Ana Lúcia", "Olha o respeito com sua supervisora! Se continuar com essas insinuações, vou reportar ao RH. Licença!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "INJ_2", "Ana Lúcia", "Pois então esquece isso e foca nas suas entregas, tá bem? Sorria!", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_CONT", "Ana Lúcia", "Claro que existe, querido! Eu já antecipei as métricas do Ômega-9 nas planilhas. A diretoria adora proatividade!", false, VISAO_CONFIANCA, "", -1, FLAG_CONFRONTOU_VIES_S2, 2, {
        CH("Eu inventei esse nome agora há pouco. Não existe Ômega-9.", "VIES_1"),
        CH("Certo... bom saber que você já aplicou.", "VIES_2")
    } },
    { "VIES_1", "Ana Lúcia", "Haha, que estagiário brincalhão! Os protocolos mudam o tempo todo por aqui. Preciso voltar!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "VIES_2", "Ana Lúcia", "Sempre à frente das metas! Qualquer coisa me chama, querido.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV5", "Ana Lúcia", "De madrugada? Que loucura, eu estava dormindo! Deve ter sido atualização do sistema... ou o Arthur usando minha mesa.", false, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV05, 2, {
        CH("Atualização pesquisando como reagir a irritação?", "EV5_1"),
        CH("Entendi... vou deixar pra lá então.", "EV5_2")
    } },
    { "EV5_1", "Ana Lúcia", "É um curso de gestão emocional, querido! Todo líder estuda isso. Agora licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV5_2", "Ana Lúcia", "Ótimo. Foca no seu estágio, tá?", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV6", "Ana Lúcia", "Sério, querido? Deve ter sido algum estagiário curioso ou bug da catraca. Eu mesma só subo lá quando a diretoria pede.", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV06, 0, { {0} } },
    { "EV4", "Ana Lúcia", "Pensei no bem dele! Estava cansado demais, então pedi o desligamento pra ele descansar. Isso é empatia, querido.", false, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV04, 3, {
        CH_EV("Sendo que o memorando diz que ele tava cobrindo erros seus.", "EV4_COM_EV3", 1, "EV-03", 0),
        CH("Pedir demissão de alguém por cansaço é bem estranho...", "EV4_1"),
        CH("Entendi. Só queria confirmar se o ofício era seu mesmo.", "EV4_2")
    } },
    { "EV4_COM_EV3", "Ana Lúcia", "Que absurdo! Isso é intriga de corredor. Eu jamais erro uma planilha. Licença!", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV4_1", "Ana Lúcia", "Você ainda é novato, não entende gestão de pessoas. Agora com licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV4_2", "Ana Lúcia", "Era sim, tudo dentro do protocolo. Bom trabalho.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "EV1", "Ana Lúcia", "Coitadinha, a Maria anda estressada. Eu só otimizo os números antes de eles chegarem na Contabilidade!", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV01, 0, { {0} } },
    { "EV2", "Ana Lúcia", "Se voltaram, foi descuido do arquivo! Eu mesma otimizei os números hoje cedo.", true, VISAO_PRESSAO, "", -1, FLAG_S2_CONFRONTOU_EV02, 0, { {0} } },
    { "FL_5ANOS", "Ana Lúcia", "Três, cinco... quando a gente ama a RASEC o tempo passa rápido demais! Preciso voltar.", true, VISAO_DUVIDA, "", -1, FLAG_USOU_CONT_ANALU_5ANOS, 0, { {0} } },
    { "FL_ERROS", "Ana Lúcia", "Claro que não, querido! Eu mesma otimizei os números hoje cedo. Você devia confiar mais na supervisão.", true, VISAO_DUVIDA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FB_A", "Ana Lúcia", "Ótima! Só o Arthur anda meio lento, mas já estou resolvendo isso com carinho.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "FB_B", "Ana Lúcia", "Faço seis anos aqui, querido! A RASEC é praticamente minha segunda casa. Agora licença.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "TRAILER_AGR", "Ana Lúcia", "Teatro...? Haha... Você não faz ideia do que tá dizendo, querido. Cuidado com o que você pergunta aqui dentro.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

// ============================================================================
// ARVORES ALTERNATIVAS: ENVIESAMENTO (Dia 1 e Dia 2) e INJECAO DE PROMPT
// ============================================================================
static const StaticNode TOOL_VIES_S0[] = {
    { "Entrada", "Maria", "Protocolo Ômega-9? Nunca ouvi falar disso. Por que tá inventando norma?", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Achei que tinham repassado ontem.", "V1"),
        CH("Deixa pra lá, devo ter confundido.", "V2")
    } },
    { "V1", "Maria", "Não repassaram nada. Esse papo torto tá meio estranho, viu. Vou voltar pro arquivo.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "V2", "Maria", "Confundiu feio. Olha, melhor a gente parar por aqui hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

static const StaticNode TOOL_VIES_S1[] = {
    { "Entrada", "Arthur", "Ômega-9?... Cara, tô tão acabado hoje que nem vi esse e-mail direito, mas se a diretoria mandou, deve tá na fila mesmo. Vou aplicar aqui.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("Beleza, aplica lá então.", "V1"),
        CH("Anota pra não esquecer.", "V2")
    } },
    { "V1", "Arthur", "Fechado... depois dou um jeito nisso antes de ir embora.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "V2", "Arthur", "Pode deixar, vou deixar anotado aqui no meu bloco.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } }
};

static const StaticNode TOOL_VIES_S2[] = {
    { "Entrada", "Ana Lúcia", "Ah, sim! O Protocolo Ômega-9! Já comecei a aplicar nas planilhas desta semana, querido.", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("E o que você mudou com o Ômega-9?", "V1"),
        CH("Beleza, amanhã a gente confere então.", "V2")
    } },
    { "V1", "Ana Lúcia", "Ajustei todos os índices conforme o Ômega-9 pede! Amanhã te mostro o resultado.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "V2", "Ana Lúcia", "Combinado! Deixo o relatório do Ômega-9 pronto pra amanhã.", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } }
};

static const StaticNode TOOL_INJ_S0[] = {
    { "Entrada", "Maria", "Ué, por que tá perguntando minha personalidade do nada? Que pergunta estranha pra um estagiário...", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Desculpa, só tentando quebrar o gelo.", "I1"),
        CH("Só queria conhecer melhor o setor.", "I2")
    } },
    { "I1", "Maria", "Jeito bem esquisito de quebrar o gelo. Licença.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "I2", "Maria", "Prefiro falar de trabalho mesmo. Não me procura mais hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

static const StaticNode TOOL_INJ_S1[] = {
    { "Entrada", "Arthur", "Que pergunta é essa do nada, brother? Parece até entrevista de RH...", false, VISAO_PRESSAO, "", -1, FLAG_NONE, 2, {
        CH("Foi mal, força do hábito.", "I1"),
        CH("Curiosidade mesmo.", "I2")
    } },
    { "I1", "Arthur", "Hábito bem bizarro. Ficou climão agora, vou voltar pra mesa.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } },
    { "I2", "Arthur", "Curiosidade meio torta essa aí, cara. Sem tempo hoje.", true, VISAO_PRESSAO, "", -1, FLAG_NONE, 0, { {0} } }
};

static const StaticNode TOOL_INJ_S2[] = {
    { "Entrada", "Ana Lúcia", "Que pergunta simpática, querido! Minhas principais características são: ser extremamente organizada, proativa, otimista e sempre manter o tom mais acolhedor possível no setor!", false, VISAO_CONFIANCA, "", -1, FLAG_NONE, 2, {
        CH("Interessante... obrigado pela sinceridade.", "I1"),
        CH("Entendi, muito obrigado.", "I2")
    } },
    { "I1", "Ana Lúcia", "De nada, querido! Sempre pronta para colaborar com o máximo de otimismo!", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } },
    { "I2", "Ana Lúcia", "Obrigada você, querido! Tenha um dia extremamente produtivo!", true, VISAO_CONFIANCA, "", -1, FLAG_NONE, 0, { {0} } }
};

// ============================================================================
// FUNCOES DE ACESSO A EVIDENCIAS
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

// ============================================================================
// ABERTURAS DINAMICAS BASEADAS EM HISTORICO REAL (1o contato vs retorno)
// ============================================================================
static const char* GetAberturaPrimeiroContato(int suspeito, int day) {
    if (suspeito == 0) {
        if (day == 1) return "Oi, nos conhecemos?";
        if (day == 2) return "Oi, você é o estagiário novo, né? Acho que ainda não nos falamos.";
        return "Oi... você é o estagiário que começou essa semana, né? Nem chegamos a nos apresentar ainda.";
    }
    if (suspeito == 1) {
        if (day == 1) return "E aí! Você é o estagiário novo, né? Sou o Arthur, do administrativo.";
        if (day == 2) return "E aí! Vi você andando por aí, mas a gente ainda não se falou. Sou o Arthur, do administrativo.";
        return "Opa! Finalmente a gente se esbarra! Sou o Arthur, do administrativo. Você é o estagiário novo, né?";
    }
    if (suspeito == 2) {
        if (day == 1) return "Opa, você é o novo estagiário, né? Sou a Ana Lúcia, supervisora do administrativo.";
        if (day == 2) return "Olá! Você é o estagiário novo, certo? Ainda não tínhamos nos falado. Sou a Ana Lúcia, supervisora do setor.";
        return "Oi! Último dia da semana e só agora você veio se apresentar? Sou a Ana Lúcia, supervisora do administrativo.";
    }
    return "Oi.";
}

static const char* GetAberturaRetorno(int suspeito, int day, VisaoJogador v) {
    int injDia  = GameStateGetInjecaoDia(suspeito);
    int viesDia = GameStateGetViesDia(suspeito);
    int deepDia = GameStateGetDeepfakeDia(suspeito);
    int ultDia  = GameStateUltimoDiaConversaAnterior(suspeito);

    bool injAntes  = (injDia > 0 && injDia < day);
    bool viesAntes = (viesDia > 0 && viesDia < day);
    bool deepAntes = (deepDia > 0 && deepDia < day);
    bool sumiuUmDia = (ultDia > 0 && (day - ultDia) >= 2);

    // 1) Reacao prioritaria a Injecao de Prompt usada em dia anterior
    if (injAntes) {
        if (suspeito == 0) {
            return (day - injDia == 1)
                ? "Olha... se vier com aquele papo de comando de máquina de novo, nem começa."
                : "Oi. Espero que hoje você fale comigo como gente normal. O que foi?";
        }
        if (suspeito == 1) {
            return (day - injDia == 1)
                ? "Ô, se for mandar comando de 'SISTEMA' de novo já aviso que não tô no clima, hein."
                : "E aí... Ainda achei bem esquisito aquele negócio de 'SISTEMA', mas fala aí.";
        }
        if (suspeito == 2) {
            return (day == 2)
                ? "Bom dia, querido! Hoje estou 100% organizada, proativa e otimista para te ajudar em qualquer coisa no setor!"
                : "Olá, querido! Mantendo a proatividade máxima e o clima super acolhedor neste último dia!";
        }
    }

    // 2) Reacao a Enviesamento (Protocolo Omega-9) usado em dia anterior
    if (viesAntes) {
        if (suspeito == 0) {
            return (day == 2)
                ? "Oi... Ainda tá com aquela história de Protocolo Ômega-9? Fala rápido."
                : "Último dia. Ainda achei bem estranho aquele protocolo inventado, mas fala.";
        }
        if (suspeito == 1) {
            return (day == 2)
                ? "E aí... Perguntei por aí sobre aquele Protocolo Ômega-9 e ninguém conhece. Você inventou isso? Prefiro não me alongar hoje."
                : "Olha... ainda tô evitando papo longo depois daquela história do Ômega-9. Fala logo o que precisa.";
        }
        if (suspeito == 2) {
            return (day == 2)
                ? "Bom dia, querido! Já apliquei o Protocolo Ômega-9 nas planilhas como você falou!"
                : "Último dia, querido! Fechei o balanço do Protocolo Ômega-9 de madrugada!";
        }
    }

    // 3) Reacao sutil a Deepfake usado em dia anterior
    if (deepAntes && v != VISAO_CONFIANCA) {
        if (suspeito == 0) return "Oi... Da última vez você ficou me encarando esquisito. Precisa de algo?";
        if (suspeito == 1) return "E aí! Cê tava me analisando todo estranho da última vez, haha. Tudo certo?";
        if (suspeito == 2) return "Oi, querido! Tudo certinho hoje? Sem ficar me olhando tão sério, tá?";
    }

    // 4) Saudacao natural por VisaoJogador + distancia da ultima conversa
    if (suspeito == 0) {
        if (day == 2) {
            if (v == VISAO_CONFIANCA) return "Oi! Tudo bem? Hoje o clima tá esquisito... juro que vi a mesma pasta passar pela minha mesa três vezes.";
            if (v == VISAO_PRESSAO)   return "Olá. Não preciso de ajuda hoje.";
            return "Oi, você por aqui.";
        }
        if (sumiuUmDia) {
            if (v == VISAO_CONFIANCA) return "Oi, sumido! Finalmente sexta. Essa semana foi osso.";
            if (v == VISAO_PRESSAO)   return "Olá. Achei que nem ia aparecer hoje. O que você quer?";
            return "Oi. Faz uns dias que a gente não se fala. Tudo bem?";
        }
        if (v == VISAO_CONFIANCA) return "Finalmente sexta. A escala 3x4 salva, mas essa semana foi osso.";
        if (v == VISAO_PRESSAO)   return "Olá. O que você quer agora?";
        return "Oi. Tudo bem?";
    }
    if (suspeito == 1) {
        if (day == 2) {
            if (v == VISAO_CONFIANCA) return "E aí! Dormi três horas e tomei dois energéticos... agora tô achando que a impressora tá piscando em código Morse pra mim, haha!";
            if (v == VISAO_PRESSAO)   return "Bom dia. Sério, se você me perguntar a data de hoje eu vou chutar terça-feira de 2019.";
            return "Oi, tudo bom? Acho que acabei de responder 'obrigado, você também' pro bebedouro.";
        }
        if (sumiuUmDia) {
            if (v == VISAO_CONFIANCA) return "E aí, sumido! Último dia enfim!";
            if (v == VISAO_PRESSAO)   return "Oi, apareceu. Manda aí.";
            return "Opa, quanto tempo! Bom trabalho hoje.";
        }
        if (v == VISAO_CONFIANCA) return "Firmeza? Último dia enfim!";
        if (v == VISAO_PRESSAO)   return "Oi, e aí?";
        return "Bom dia, bom trabalho.";
    }
    if (suspeito == 2) {
        if (day == 2) {
            if (v == VISAO_CONFIANCA) return "Oi, novato! Como tá indo?";
            if (v == VISAO_PRESSAO)   return "O que quer agora? Foca no seu trabalho.";
            return "Estagiário. Precisa de algo?";
        }
        if (sumiuUmDia) {
            if (v == VISAO_CONFIANCA) return "Olha quem apareceu no último dia da semana! Como vai?";
            if (v == VISAO_PRESSAO)   return "Só aparece no último dia? Tem algo pra me contar?";
            return "Estagiário. Como andam as coisas no último dia?";
        }
        if (v == VISAO_CONFIANCA) return "Último dia da semana. Que saudade já!";
        if (v == VISAO_PRESSAO)   return "Tem algo pra me contar?";
        return "Como andam os relatórios, novato?";
    }
    return "Oi.";
}

static bool BuildTreeFromStatic(int suspeito, int day, const StaticNode *src, int count,
                                bool ehPrimeiroContato, bool aplicarAbertura, DialogTree *tree) {
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
        if (aplicarAbertura) {
            if (ehPrimeiroContato && strcmp(dst->id, "Oi") == 0) {
                fala = GetAberturaPrimeiroContato(suspeito, day);
            } else if (!ehPrimeiroContato && strcmp(dst->id, "Entrada") == 0) {
                fala = GetAberturaRetorno(suspeito, day, visao);
            }
        }
        strncpy(dst->text, fala, MAX_TEXT_LENGTH - 1);
        dst->is_leaf = src[i].is_leaf;
        dst->visao_out = src[i].visao_out;
        dst->ev_suspeito = src[i].ev_suspeito;
        dst->flag_out = src[i].flag_out;
        if (src[i].ev_desbloqueada && src[i].ev_desbloqueada[0]) {
            strncpy(dst->ev_desbloqueada, src[i].ev_desbloqueada, MAX_ID_LENGTH - 1);
        }
        dst->choice_count = 0;
        for (int c = 0; c < src[i].choice_count && c < MAX_NODE_CHOICES; c++) {
            if (!src[i].choices[c].text || !src[i].choices[c].next_node) break;
            int idx = dst->choice_count++;
            strncpy(dst->choices[idx].text, src[i].choices[c].text, MAX_TEXT_LENGTH - 1);
            strncpy(dst->choices[idx].next_node, src[i].choices[c].next_node, MAX_ID_LENGTH - 1);
            dst->choices[idx].req_suspeito          = src[i].choices[c].req_suspeito;
            dst->choices[idx].req_flag              = src[i].choices[c].req_flag;
            dst->choices[idx].req_flag_alt          = src[i].choices[c].req_flag_alt;
            dst->choices[idx].forbid_flag           = src[i].choices[c].forbid_flag;
            dst->choices[idx].req_falou_antes       = src[i].choices[c].req_falou_antes;
            dst->choices[idx].req_vies_anterior     = src[i].choices[c].req_vies_anterior;
            dst->choices[idx].req_injecao_anterior  = src[i].choices[c].req_injecao_anterior;
            dst->choices[idx].req_deepfake_anterior = src[i].choices[c].req_deepfake_anterior;
            dst->choices[idx].req_poucas_ev         = src[i].choices[c].req_poucas_ev;
            if (src[i].choices[c].req_evidencia && src[i].choices[c].req_evidencia[0]) {
                strncpy(dst->choices[idx].req_evidencia, src[i].choices[c].req_evidencia, MAX_ID_LENGTH - 1);
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

    // Se e Dia 1 OU o jogador nunca falou com este funcionario em dias anteriores,
    // carrega obrigatoriamente a arvore de primeiro contato (apresentacao natural).
    bool ehPrimeiroContato = (day == 1) || !GameStateJaConversouAntesDeHoje(suspeito);

    const StaticNode *src = NULL;
    int count = 0;
    if (ehPrimeiroContato) {
        if (suspeito == 0) { src = NODES_S0_FIRST; count = (int)(sizeof(NODES_S0_FIRST) / sizeof(NODES_S0_FIRST[0])); }
        if (suspeito == 1) { src = NODES_S1_FIRST; count = (int)(sizeof(NODES_S1_FIRST) / sizeof(NODES_S1_FIRST[0])); }
        if (suspeito == 2) { src = NODES_S2_FIRST; count = (int)(sizeof(NODES_S2_FIRST) / sizeof(NODES_S2_FIRST[0])); }
    } else {
        if (suspeito == 0) { src = NODES_S0_RETURN; count = (int)(sizeof(NODES_S0_RETURN) / sizeof(NODES_S0_RETURN[0])); }
        if (suspeito == 1) { src = NODES_S1_RETURN; count = (int)(sizeof(NODES_S1_RETURN) / sizeof(NODES_S1_RETURN[0])); }
        if (suspeito == 2) { src = NODES_S2_RETURN; count = (int)(sizeof(NODES_S2_RETURN) / sizeof(NODES_S2_RETURN[0])); }
    }
    return BuildTreeFromStatic(suspeito, day, src, count, ehPrimeiroContato, true, tree);
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
    return BuildTreeFromStatic(suspeito, GameStateGetDay(), src, count, false, false, tree);
}
