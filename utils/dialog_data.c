#include "dialog_data.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *text;
    const char *next_node;
    const char *req_evidencia;
} StaticChoice;

typedef struct {
    const char *id;
    const char *speaker;
    const char *text;
    bool is_leaf;
    VisaoJogador visao_out;
    const char *ev_desbloqueada;
    int choice_count;
    StaticChoice choices[MAX_NODE_CHOICES];
} StaticNode;

typedef struct {
    const char *id;
    const char *nome;
} StaticEvidencia;

static const StaticNode NODES_S0_D1[] = {
    { "Oi", "Maria", "Olá, nos conhecemos?", false, VISAO_DUVIDA, "", 2, { { "Ainda não, sou o novo funcionário Inspetor, e você?", "A", "" }, { "Não, sou novo por aqui. Você é de que setor?", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Maria", "Me chamo Maria, sou auxiliar administrativa.", false, VISAO_DUVIDA, "", 2, { { "Como andam as coisas por aqui?", "A.1", "" }, { "O que você acha dos outros funcionarios? não levo muito jeito para falar com os outros.", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Maria", "Sou do administrativo, e você?", false, VISAO_DUVIDA, "", 2, { { "Sou estágiario auxiliar geral e me designaram primeiro para ver o setor administrativo.", "B.1", "" }, { "Sou estágiario, me designaram aqui por reclamações de desempenho.", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Maria", "Está indo tudo bem, apenas alguns relatórios estão errados...", false, VISAO_DUVIDA, "", 2, { { "Como assim? desculpa a curiosidade.", "A.1.1", "" }, { "Ouvi falar sobre, é loucura né? mas todos cometem erros", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Maria", "Ah, a maioria é bem legal, todo mundo se da bem.", false, VISAO_DUVIDA, "", 2, { { "Eu ouvi por ai que não gostam tanto de uma pessoa no setor administrativo.", "A.2.1", "" }, { "Me falaram o contrario, então fiquei com medo.", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Maria", "Ah entendi, mas por quê? a equipe está indo bem.", false, VISAO_DUVIDA, "", 2, { { "Só estou fazendo meu trabalho, pode ser precaução.", "B.1.1", "" }, { "Você sabe muito bem que tem relatórios errados vindo.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Maria", "Problemas no desempenho? Como assim?", false, VISAO_DUVIDA, "", 2, { { "Aparentemente em uma reunião, tinham dados completamente incorretos em um relatório.", "B.2.3", "" }, { "Você está responsavel pelo arquivamento dos relatórios? Ultimamente há diversos errados.", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Maria", "Alguns relatórios do setor administrativo estão sendo enviados de forma errada, tenho que revisar todos antes de arquivar.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Maria", "Pois é, talvez com sua ajuda a gente consiga mitigar mais esses erros.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Maria", "Eu não sei, talvez tenha alguém que não se de bem com alguém daqui. tenho que voltar ao trabalho.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Maria", "Eu não sei, tenho que voltar ao trabalho. Mas o pessoal é amigável, não se preocupa.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Maria", "Talvez, talvez. Mas qualquer coisa estou por aqui, tenho que voltar ao trabalho.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Maria", "Isso faz parte, não entendi pra que ter um auxiliar aqui. Já vou indo.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Maria", "Sou sim, mas não sou eu quem faço eles. tenho que voltar ao trabalho.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.3", "Maria", "Ah, isso... acontece, as vezes. eu acho. tenho que voltar ao trabalho.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S0_D2[] = {
    { "Entrada", "Maria", "Ler o apoio geral.", false, VISAO_DUVIDA, "", 2, { { "Está tudo bem, só vim pedir um conselho.", "A", "" }, { "Desculpe por ontem, me sinto deslocado ainda.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Maria", "Pode falar rapaz. Estamos aqui pra isso.", false, VISAO_DUVIDA, "", 2, { { "Estou passando por problemas pessoais, e desde que comecei na empresa uma pessoa vem me tratando mal.", "A.1", "" }, { "Os relatórios sempre vêm incorretos, queria descobrir quem está preenchendo eles.", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Maria", "Tudo bem, sou toda ouvidos.", false, VISAO_DUVIDA, "", 2, { { "Os números nos relatóios parecem muito fora da curva.", "B.1", "" }, { "Você gosta de viajar?", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Maria", "Ah serio? que tipo de problemas?", false, VISAO_DUVIDA, "", 2, { { "Problemas financeiros. Desabafei isso com uma pessoa da empresa e ela agiu de forma rude.", "A.1.1", "" }, { "Nada importante, mas essa pessoa está me deixando desconfortável. desculpa estar incomodando.", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Maria", "Ah, isso é facil de descobrir. mas pra que você quer saber?", false, VISAO_DUVIDA, "", 2, { { "Quero falar com a pessoa, mas não tem o nome de quem fez o relatório.", "A.2.1", "" }, { "Porque eu fui contratado para isso, procurar gargalos, principalmente.", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Maria", "Bom, quando chegam relatórios com erros crassos assim, eu devolvo para avaliação da supervisora.", false, VISAO_DUVIDA, "", 2, { { "Compreendo, obrigado por explicar essa parte.", "B.1.1", "" }, { "Vou ter de relatar caso aconteçam inconsistências demais.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Maria", "Sim, já fui a Lisboa! Ainda lembro dos bondinhos e daquele sol lindo.", false, VISAO_DUVIDA, "", 2, { { "Em que hotel ficou?", "B.2.1", "" }, { "Você achou difícil entender o português deles?", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Maria", "Eu acho que sei quem é... depois a gente conversa, mas fica bem ok?", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Maria", "Se for quem estou pensando, ela não foi sempre assim. Outro dia a gente conversa mais sobre.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Maria", "É contra o procedimento, mas poderia achar facilmente quem fez.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Maria", "Ah, entendi. Boa sorte, talvez consiga descobrir conversando com outros.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Maria", "De nada, em geral, pode contatar a supervisora.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Maria", "Pode falar para a supervisora. Estou fazendo minha parte.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1", "Maria", "Um hotel no... centro, era ótimo aliás.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Maria", "No começo é um pouco difícil, o pessoal fala rápido, mas você acostuma.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S0_D3[] = {
    { "Entrada", "Maria", "Ler apoio geral.", false, VISAO_DUVIDA, "", 2, { { "Finalmente, final de semana.", "B.2", "" }, { "Essa semana está sendo horrivel, desculpe.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Maria", "Todos nós temos semanas ruins, relaxa.", false, VISAO_DUVIDA, "", 2, { { "Para ser sincero, eu acho que você é a causa desses relatórios errados.", "B.1", "" }, { "Quem estava na sua mesa?", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Maria", "De novo isso? Eu não faço os relatórios, eu apenas os arquivo.", false, VISAO_DUVIDA, "", 2, { { "Mas você sabe quem está cometendo os erros?", "B.1.1", "" }, { "mas não acha que deveria relatar os erros antes de arquivar?", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Maria", "Sobre aquela pessoa, ela não era assim sempre. Aconteceu a algumas semanas, como se ela tivesse se tornado outra pessoa.", false, VISAO_DUVIDA, "", 2, { { "Você diz da supervisora do setor administrativo?", "B.2.1", "" }, { "Está falando sobre os relatórios ou o comportamento dela?", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Maria", "Isso não importa, faça seu trabalho... não é como se fossem tantos relatórios errados assim.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Maria", "Eu relato eles, você acha que simplesmente chegam lá os relátorios de erro?", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1", "Maria", "Sim... Ela fazia o trabalho perfeitamente, mas ela alucina informações para por no relatório.", false, VISAO_DUVIDA, "", 2, { { "Você pode me falar mais? só se estiver confortavel, claro.", "Entrada", "" }, { "Você pode me falar mais sobre a supervisora?", "B.2.1#EV-01", "EV-01" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Maria", "Eu acho que já falei demais. Tenho que voltar ao trabalho, esses relatórios não param de vir.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1#EV-01", "Maria", "Ela começou a ir para setores que não deveria ir, a agir de forma muito arrogante. O Arthur faz de tudo para agradar ela e acaba errando relatórios, mas ela faz pior.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S1_D1[] = {
    { "Oi", "Arthur", "Oi brother! Prazer, meu nome é Arthur. Como vai?", false, VISAO_DUVIDA, "", 2, { { "Prazer, meu nome é Inspetor. Fala sobre você.", "A", "" }, { "Não te conheço.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Arthur", "Tudo ótimo, sou assistente do administrativo.", false, VISAO_DUVIDA, "", 2, { { "Qual sua vibe, no pessoal?", "A.1", "" }, { "Como funciona seu setor?", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Arthur", "Ah... Tudo bem, se precisar de algo, pode falar comigo", false, VISAO_DUVIDA, "", 2, { { "Onde você trabalha?", "B.1", "" }, { "Seu serviço é difícil?", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Arthur", "Ah, sou músico por hobby, guitarrista.", false, VISAO_DUVIDA, "EV-08", 2, { { "Show! Qual tua banda favorita?", "A.1.1", "" }, { "Qual seu gênero musical favorito?", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Arthur", "Eu preencho os relatórios e faço projeções com os números do pessoal, mas gosto de ajudar no que posso.", false, VISAO_DUVIDA, "", 2, { { "Tem muita demanda hoje?", "A.2.1", "" }, { "Você gosta da função?", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Arthur", "Eu sou assistente do administrativo, preencho os relatórios. Nada demais.", false, VISAO_DUVIDA, "", 2, { { "Você revisa os relatórios?", "B.1.1", "" }, { "Você preferia trabalhar em outra coisa?", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Arthur", "Eu preencho os relatórios, as vezes acumula trabalho de um dia para o outro, mas da pra lidar", false, VISAO_DUVIDA, "", 2, { { "Você não consegue terminar no dia?", "B.2.3", "" }, { "Por que você não fala com seu chefe?", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Arthur", "Metal Wyrms, eles são sinistros!", true, VISAO_CONFIANCA, "EV-05", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Arthur", "Em geral, todos os tipos de rock, curto eletrônica também.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Arthur", "Tem, mas eu dou conta. Geralmente...", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Arthur", "Não é meu sonho, mas estou confortável. Sou mais da música.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Arthur", "É, isso ai.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Arthur", "Meu sonho era música, mas estou bem aqui", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Arthur", "Eu já tentei...", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.3", "Arthur", "Não... É complicado", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S1_D2[] = {
    { "Entrada", "Arthur", "Ler o apoio geral.", false, VISAO_DUVIDA, "", 2, { { "Oi! Tudo certo administrativo?", "A", "" }, { "Bom dia. A supervisora Ana Lúcia é bem exigente, não é?", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Arthur", "Tudo, a supervisora é exigente, mas tudo ok. Se adaptou bem ontem?", false, VISAO_DUVIDA, "", 3, { { "Sim, falei com todos.", "A.1", "" }, { "Falei, mas estou passando por problemas pessoais, acabei desabafando e uma pessoa vem me tratando mal.", "A.2", "" }, { "Consegui falar com galera. Vem cá, você disse que gosta dos Metal Wyrms, você conhece os Psicóticos?", "A#EV-05", "EV-05" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Arthur", "Ela prefere Analu, é exigente, pois quer manter um padrão de qualidade, mas ela deve estar tendo problemas ultimamente.", false, VISAO_DUVIDA, "", 2, { { "Todos tem problemas, mas se for o caso, ela deveria separar mais do profissional.", "B.1", "" }, { "Você sabe que tipo de problemas?", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Arthur", "Supimpa! Gosto de passar um tempo no refeitório com o pessoal, cê devia aparecer.", false, VISAO_DUVIDA, "", 2, { { "Legal, você parece muito simpático.", "A.1.1", "" }, { "Com o pessoal do administrativo? ou de outros setores também?", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Arthur", "Oh... Sinto muito, não se preocupa com isso, não vou permitir que tratem os outros assim.", false, VISAO_DUVIDA, "", 2, { { "Tá traquilo, agradeço a força.", "A.2.1", "" }, { "Tem conseguido lidar com as demandas?", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Arthur", "Isso é verdade, mas sabemos que não é assim, cê tá sendo extremamente lógico.", false, VISAO_DUVIDA, "EV-07", 2, { { "Eu... acho que você está certo", "B.1.1", "" }, { "Acho que sim... Maria é bem profissional, mesmo ela está tendo dificuldades.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Arthur", "Olha, não sei, mas não gosto de falar dos outros.", false, VISAO_DUVIDA, "", 2, { { "Mesmo assim, não acha que ela está pondo muita pressão?", "B.2.1", "" }, { "A Maria está reclamando do acúmulo de relatórios.", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Arthur", "Hihihi... Valeu, eu gosto de socializar com todo mundo.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Arthur", "De vários setores, inclusive a supervisora... Mas ela tem estado distante.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Arthur", "Olha, eu deixei um energético na geladeira do refeitório, pode pegar se fizer te sentir melhor.", true, VISAO_CONFIANCA, "EV-04", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Arthur", "A supervisora vem implicando um pouco nas entregas, mas estou tentando meu melhor.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Arthur", "Eu vou tentar conversar com ela, não esquenta.", true, VISAO_CONFIANCA, "EV-06", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Arthur", "Ela está com dificuldades? Vou falar com ela.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1", "Arthur", "Talvez, mas vou tentar um diálogo.", true, VISAO_DUVIDA, "EV-06", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Arthur", "Eu vou conseguir resolver, a Analu está dificultando as coisas.", true, VISAO_PRESSAO, "EV-06", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A#EV-05", "Arthur", "É minha banda indie favorita irmão! Cê tem bom gosto.", false, VISAO_DUVIDA, "", 2, { { "Você tem notado algo diferente no comportamento de Ana Lúcia?", "A#EV-05.1", "" }, { "Você tem notado algo diferente no comportamento de Maria Bianca?", "A#EV-05.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A#EV-05.1", "Arthur", "Ela prefere Analu, mas sim, de um tempo pra cá nada que faço parece correto. Parece que ela segue um roteiro.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A#EV-05.2", "Arthur", "Particularmente não, ela parece mais na dela, mas acho que é algo temporário.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S1_D3[] = {
    { "Entrada", "Arthur", "(a fala de entrada depende da visão herdada do Dia 2)", false, VISAO_DUVIDA, "", 4, { { "Oi Arthur, tudo bem?", "A", "" }, { "Oi, estava te procurando.", "B", "" }, { "Bom dia, falou com a Ana Lúcia?", "Entrada#EV-06", "EV-06" }, { "Achei seu ID, por quê estava na geladeira?", "Entrada#EV-04", "EV-04" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Arthur", "Tudo, vou visitar a família amanhã.", false, VISAO_DUVIDA, "", 2, { { "Que ótimo, sempre legal ficar perto dos pais.", "A.1", "" }, { "Filho da mamãe né?", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Arthur", "Pode dizer, manda ai.", false, VISAO_DUVIDA, "", 2, { { "Revisou muitos relatórios hoje?", "B.1", "" }, { "Precisamos falar sobre esses relatórios", "B#EV-03", "EV-03" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Arthur", "É, passo muito tempo com meu pai. Cresci apenas com ele.", false, VISAO_DUVIDA, "", 2, { { "Revisou muitos relatórios hoje?", "A.1.1", "" }, { "A Maria está distante ultimamente? Vocês pareciam chegados.", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Arthur", "Eu.. Eu cresci sem minha mãe.", false, VISAO_DUVIDA, "", 2, { { "Desculpe Arthur, não quis ser rude.", "A.2.1", "" }, { "Ah... Perdão, como vão os relatórios?", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Arthur", "Revisei alguns hoje sim, por quê a pergunta?", false, VISAO_DUVIDA, "", 2, { { "Sua função é revisar os relatórios?", "B.1.1", "" }, { "Conseguiu terminar o trabalho de ontem?", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Arthur", "É, tive que revisar, mesmo não sendo minha função, se eu não fizesse não ia ter como atender as demandas doidas da Analu.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Arthur", "Estava, juntou um pouco de burnout com outros problemas, mas tirando a Analu, acho que está tudo normal agora.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Arthur", "Tudo bem, não tinha como você saber. Vivi com meu pai desde sempre, ele que me ensinou guitarra.", true, VISAO_CONFIANCA, "EV-08", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Arthur", "Revisei os relatórios, tá complicado.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Arthur", "Não, falei pra cê, eu os preencho, mas tive que revisar por conta de alguns comentários de Analu.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Arthur", "Parecia que iria conseguir progresso, mas estou estagnado.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "Entrada#EV-06", "Arthur", "Falei, ela simplesmente não entende", false, VISAO_DUVIDA, "", 1, { { "Oque aconteceu?", "Entrada#EV-06.1", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "Entrada#EV-06.1", "Arthur", "Ela parece teimosa até demais, como se estivesse empenhada em me contradizer, isso não é normal.", false, VISAO_DUVIDA, "", 1, { { "Hmm... Entendo", "Entrada", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "Entrada#EV-04", "Arthur", "Eita! Valeu por devolver. Cê sabe, as vezes sou meio desastrado.", false, VISAO_DUVIDA, "", 1, { { "Sei...", "Entrada", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B#EV-03", "Arthur", "Olha, eu vou resolver, tá?", false, VISAO_DUVIDA, "", 2, { { "Certo, tem algum motivo pra isso?", "B#EV-03.1", "" }, { "Você não pode simplesmente o trabalho se amontoar.", "B#EV-03.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B#EV-03.1", "Arthur", "Analu tem basicamente inventado exigências que não fazem sentido, no mais é isso.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B#EV-03.2", "Arthur", "Veja só, eu conseguiria lidar com tudo como fazia antes, se Analu não tivesse dando umas justificativas insanas pra mim.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S2_D1[] = {
    { "Oi", "Ana Lúcia", "Opa você deve ser o novo estágiario, né? Sou a supervisora do setor de administração.", false, VISAO_DUVIDA, "", 2, { { "Boa tarde, Lucia! Tudo joia?", "A", "" }, { "Sim, vim resolver os problemas do setor.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Ana Lúcia", "Está tudo bem sim. Soube que você entrou por indicação, vamos ver se é bom mesmo.", false, VISAO_DUVIDA, "", 2, { { "Vou tentar não decepcionar. Algum problema no setor?", "A.1", "" }, { "Como funciona seu setor?", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Ana Lúcia", "Problemas? Como assim?", false, VISAO_DUVIDA, "", 2, { { "Não sei, acho que pode ser só rotina. não quis ofender.", "B.1", "" }, { "Só faço o que mandam.", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Ana Lúcia", "Problema nenhum, o meu setor é organizado, por que?", false, VISAO_DUVIDA, "", 2, { { "Da pra ver! Todos me falam que esse setor é o que menos dá problema.", "A.1.1", "" }, { "Eu estou muito animado para trabalhar com você!", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Ana Lúcia", "Funciona perfeitamente, por que te enviaram aqui?", false, VISAO_DUVIDA, "", 2, { { "Me enviaram aqui porque eu precisava de experiência.", "A.2.1", "" }, { "Me enviaram para esse setor para ver se tem algum tipo de erro acontecendo.", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Ana Lúcia", "Deve ser um erro. Meu setor não comete erro. Vou ter que falar com eles.", false, VISAO_DUVIDA, "", 2, { { "Você está muito agressiva, pode ter sido algo recente.", "B.1.1", "" }, { "Deve ser mesmo, da pra ver que vocè é uma pessoa bem calma.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Ana Lúcia", "Você está fazendo muitas perguntas, não acha?", false, VISAO_DUVIDA, "", 2, { { "Só estou tentando fazer meu trabalho, não quis incomodar.", "B.2.3", "" }, { "Desculpa, só estou tentando me enturmar.", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Ana Lúcia", "Ah, Querido, você é muito gentil.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Ana Lúcia", "Pois vai aprender muito comigo, Nunca cometi nenhum tipo de erro durante meus 4 anos no RASEC.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Ana Lúcia", "Bem, não vai ter muito o que fazer. Talvez falando com os outros você possa ajudar com algo.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Ana Lúcia", "Não faz sentido, meus relatórios são perfeitos. Tenho que voltar ao trabalho.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Ana Lúcia", "Você é um garoto muito insolente, acha que sua indicação vai te salvar?", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Ana Lúcia", "Vou procurar mais sobre, tenho que ir agora. Tchau.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Ana Lúcia", "Seu trabalho não é se enturmar. Ver se aprende.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.3", "Ana Lúcia", "Não acha que está passando muito tempo fazendo perguntas inuteis? Volte ao trabalho, querido.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S2_D2[] = {
    { "Entrada", "Ana Lúcia", "Ler o apoio geral.", false, VISAO_DUVIDA, "", 2, { { "Boa tarde. Dormiu bem, lucia?", "A", "" }, { "Boa tarde. Desculpe por ontem.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Ana Lúcia", "Lucia... amo que me chamem pelo sobrenome, como soube?", false, VISAO_DUVIDA, "", 2, { { "Ah, eu nem percebi. Desculpa, não queria forçar intimidade.", "A.1", "" }, { "Ah, desculpa, eu nem percebi. Você gosta mesmo de ser chamada de lúcia?", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Ana Lúcia", "Não há problema, querido. apenas se concentre mais no seu emprego.", false, VISAO_DUVIDA, "", 2, { { "Me pediram para eu refazer alguns relatórios.", "B.1", "" }, { "Você pode me contar mais sobre os outros funcionarios?", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Ana Lúcia", "Ah serio? que tipo de problemas?", false, VISAO_DUVIDA, "", 2, { { "Problemas financeiros. Desabafei isso com uma pessoa da empresa e ela agiu de forma rude.", "A.1.1", "" }, { "Nada importante, mas essa pessoa está me deixando desconfortável. desculpa estar incomodando.", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Ana Lúcia", "Sim, sim! Pode me chamar de lúcia, sinto que você é uma pessoa de confiança.", false, VISAO_DUVIDA, "", 2, { { "Falando nisso, você sabe quem fez os relatórios dessas semanas?", "A.2.1", "" }, { "Enfim, Lucia, Tenho que lhe perguntar sobre os relatórios, me falaram sobre alguns erros.", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Ana Lúcia", "Bem, eu falei com alguns outros setores, aparentemente alguns relatórios estão incorretos, mas nenhum meu.", false, VISAO_DUVIDA, "", 2, { { "Nossa, isso deve ser um alívio. Como se sente?", "B.1.1", "" }, { "Mas eu achava que os relatórios não tinham nomes e fossem arquivados por terceiros.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Ana Lúcia", "Os funcionarios do meu setor são bem treinados e prestativos, só não tanto quanto eu. Você já conheceu a Maria?", false, VISAO_DUVIDA, "", 2, { { "Acho que já devo ter falado com ela, sim. Não lembro.", "B.2.1", "" }, { "Ja, sim. Ela é bem galera, parece experiente.", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Ana Lúcia", "Eu acho que sei quem é... depois a gente conversa, mas fica bem ok?", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Ana Lúcia", "Se for quem estou pensando, ela não foi sempre assim. Outro dia a gente conversa mais sobre.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Ana Lúcia", "Sim, mas essa informação é confidencial, querido.", false, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Ana Lúcia", "Deve ter sido algum novo funcionario, meus relatórios estão corretos.", true, VISAO_DUVIDA, "EV-02", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Ana Lúcia", "Estou bem, queridinho. Você deveria focar mais em corrigir o relatório da pessoa incorreta, sim?", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Ana Lúcia", "Tenho privilégios como supervisora e não estou gostando do seu tom. Tenho que voltar ao trabalho.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1", "Ana Lúcia", "Como assim, você não lembra? A Maria é uma pessoa boa. Se você não consegue lembrar nem o nome dos seus funcionarios...", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Ana Lúcia", "NO começo você se sente deslocado mesmo, mas como eu já trabalho faz 3 anos, posso te falar mais sobre como se dar bem com eles.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticNode NODES_S2_D3[] = {
    { "Entrada", "Ana Lúcia", "Ler apoio geral", false, VISAO_DUVIDA, "", 2, { { "Só vim finalizar a semana.", "A", "" }, { "Que semana cansativa.", "B", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A", "Ana Lúcia", "O que houve agora, queridinho?", false, VISAO_DUVIDA, "", 2, { { "Você é tão confiante, sempre foi assim?", "A.1", "" }, { "Fiz tanto trabalho essa semana!", "A.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B", "Ana Lúcia", "Mal começou e já está reclamando? Não vai durar tanto tempo.", false, VISAO_DUVIDA, "", 2, { { "Você trabalha aqui faz quantos anos mesmo?", "B.1", "" }, { "Você é muito grossa, mas snito que você só está fingindo.", "B.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1", "Ana Lúcia", "Sim, desde que sou criança! Nós capricornianos somos assim. e você?", false, VISAO_DUVIDA, "", 2, { { "Não sou tão confiante, Lúcia. Parece que não sei o que estou fazendo.", "A.1.1", "" }, { "Sou sim, só não quero incomodar os outros.", "A.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2", "Ana Lúcia", "Entregou tudo? Tenho certeza que precisamos atingir uma cota essa semana?", false, VISAO_DUVIDA, "", 2, { { "Você tem certeza? não lembro disso.", "A.2.1", "" }, { "Você que me diz, ué. A supervisora é você.", "A.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1", "Ana Lúcia", "Você faz perguntas demais para um estágiario. Trabalho faz 5 anos na RASEC e nunca passei por essa insubordinação!", false, VISAO_DUVIDA, "", 2, { { "Alguns funcionarios me falaram que você não trabalha aqui faz tanto tempo (MENTIR)", "B.1.1", "" }, { "Só estou fazendo perguntas, refiz varios relatorios pela semana e quero lhe conhecer melhor.", "B.1.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2", "Ana Lúcia", "O que você quer dizer com isso? Quem lhe indicou? Que grosseria!", false, VISAO_DUVIDA, "", 2, { { "Ah, digo, você no fundo parece gentil. Não quis ofender.", "B.2.1", "" }, { "Você não precisa saber disso, o seu setor está cheia de relatórios incorretos!", "B.2.2", "" }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.1", "Ana Lúcia", "Vou lhe mostrar como ser mais confiante, querido.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.1.2", "Ana Lúcia", "Haha, não transparece. Mas é porque você é novato.", true, VISAO_CONFIANCA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.1", "Ana Lúcia", "Sim, querido. A cota veio do setor de contabéis, preciso ir no setor de cibersegurança agora. Se me dá licença...", true, VISAO_DUVIDA, "EV-04", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "A.2.2", "Ana Lúcia", "Ah, querido. Você é muito grosso as vezes, qual o seu problema? Tenho que ir. Volte ao trabalho.", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.1", "Ana Lúcia", "Pois quem saberia melhor quanto tempo eu trabalho na empresa além de mim mesma? que absurdo.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.1.2", "Ana Lúcia", "Não está aqui para fazer amizades e sim para trabalhar. Volte ao trabalho, estou ocupada.", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.1", "Ana Lúcia", "Ah, entendi. Bem, eu sou mesmo, caso contrario, ja estaria demitido!", true, VISAO_DUVIDA, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
    { "B.2.2", "Ana Lúcia", "Talvez eu deva começar mesmo a cuidar melhor do meu setor, parece que um rato entrou nele!", true, VISAO_PRESSAO, "", 0, { { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL } } },
};

static const StaticEvidencia EV_TAB_0[] = { { "EV-01", "Dialogo de confiança" } };
static const int EV_CNT_0 = 1;
static const StaticEvidencia EV_TAB_1[] = { { "EV-01", "Ficha de RH consta apenas pai" }, { "EV-02", "Sempre entrega uma caneta preta com uma imagem de abutre para os amigos" }, { "EV-03", "Pilha de relatórios à fazer" }, { "EV-04", "ID perdido" }, { "EV-05", "Metal Wyrms" }, { "EV-06", "Ana Lúcia" }, { "EV-07", "Acúmulo de desconfiança" }, { "EV-08", "Guitarrista" } };
static const int EV_CNT_1 = 8;
static const StaticEvidencia EV_TAB_2[] = { { "EV-01", "Nome Preferido" }, { "EV-02", "Relatórios Errados" }, { "EV-03", "Tempo de experiência" }, { "EV-04", "Indo a setores incorretos" } };
static const int EV_CNT_2 = 4;

const char* GetEvidenciaNome(int suspeito, const char *ev_id) {
    if (!ev_id) return "";
    const StaticEvidencia *tab = NULL; int cnt = 0;
    if (suspeito == 0) { tab = EV_TAB_0; cnt = EV_CNT_0; }
    else if (suspeito == 1) { tab = EV_TAB_1; cnt = EV_CNT_1; }
    else if (suspeito == 2) { tab = EV_TAB_2; cnt = EV_CNT_2; }
    for (int i = 0; i < cnt; i++) {
        if (strcmp(tab[i].id, ev_id) == 0) return tab[i].nome;
    }
    return ev_id;
}

int GetEvidenciaCount(int suspeito) {
    if (suspeito == 0) return EV_CNT_0;
    if (suspeito == 1) return EV_CNT_1;
    if (suspeito == 2) return EV_CNT_2;
    return 0;
}

const char* GetEvidenciaIdByIndex(int suspeito, int index) {
    const StaticEvidencia *tab = NULL; int cnt = 0;
    if (suspeito == 0) { tab = EV_TAB_0; cnt = EV_CNT_0; }
    else if (suspeito == 1) { tab = EV_TAB_1; cnt = EV_CNT_1; }
    else if (suspeito == 2) { tab = EV_TAB_2; cnt = EV_CNT_2; }
    if (!tab || index < 0 || index >= cnt) return "";
    return tab[index].id;
}

static const char* GetAberturaFala(int suspeito, int day, VisaoJogador v) {
    if (suspeito == 0 && day == 2) {
        if (v == VISAO_CONFIANCA) return "Oi! Tudo bem, Inspetor?";
        if (v == VISAO_PRESSAO)   return "Olá, boa tarde. Não preciso de auxílio por hoje.";
        return "Oi, você por aqui.";
    }
    if (suspeito == 0 && day == 3) {
        if (v == VISAO_CONFIANCA) return "Ah, a escala 3x4 da RASEC é incrível.";
        if (v == VISAO_PRESSAO)   return "Olá.";
        return "Ah, oi. Tudo bem?";
    }
    if (suspeito == 1 && day == 2) {
        if (v == VISAO_CONFIANCA) return "Eai Inspetor! Tudo em cima?";
        if (v == VISAO_PRESSAO)   return "Bom dia.";
        return "Oi, tudo bom?";
    }
    if (suspeito == 1 && day == 3) {
        if (v == VISAO_CONFIANCA) return "Firmeza? Como tem passado?";
        if (v == VISAO_PRESSAO)   return "Oi, eai?";
        return "Bom dia, bom trabalho.";
    }
    if (suspeito == 2 && day == 2) {
        if (v == VISAO_CONFIANCA) return "O novato, como está indo?";
        if (v == VISAO_PRESSAO)   return "O que você quer agora? Já disse que você deveria focar em seu emprego.";
        return "Estagiário.";
    }
    if (suspeito == 2 && day == 3) {
        if (v == VISAO_CONFIANCA) return "Último dia da semana. Que saudade já!";
        if (v == VISAO_PRESSAO)   return "Tem algo para me contar?";
        return "Como andam os relatórios, novato?";
    }
    return NULL;
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
    if (!src || count <= 0) return false;
    memset(tree, 0, sizeof(DialogTree));
    const char *ids[] = { "maria", "arthur", "ana_lucia" };
    strncpy(tree->npc_id, ids[suspeito], MAX_ID_LENGTH - 1);
    tree->suspect_index = suspeito;
    tree->day = day;
    tree->node_count = count;
    tree->nodes = (DialogNode *)calloc((size_t)count + 4, sizeof(DialogNode));
    VisaoJogador visao = GameStateGetVisaoJogador(suspeito);
    for (int i = 0; i < count; i++) {
        DialogNode *dst = &tree->nodes[i];
        strncpy(dst->id, src[i].id, MAX_ID_LENGTH - 1);
        strncpy(dst->speaker, src[i].speaker, MAX_ID_LENGTH - 1);
        const char *fala = src[i].text;
        if (day >= 2 && strcmp(dst->id, "Entrada") == 0) {
            const char *ab = GetAberturaFala(suspeito, day, visao);
            if (ab) fala = ab;
        }
        strncpy(dst->text, fala, MAX_TEXT_LENGTH - 1);
        dst->is_leaf = src[i].is_leaf;
        dst->visao_out = src[i].visao_out;
        if (src[i].ev_desbloqueada && src[i].ev_desbloqueada[0]) {
            strncpy(dst->ev_desbloqueada, src[i].ev_desbloqueada, MAX_ID_LENGTH - 1);
        }
        dst->choice_count = src[i].choice_count;
        for (int c = 0; c < src[i].choice_count; c++) {
            strncpy(dst->choices[c].text, src[i].choices[c].text, MAX_TEXT_LENGTH - 1);
            strncpy(dst->choices[c].next_node, src[i].choices[c].next_node, MAX_ID_LENGTH - 1);
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