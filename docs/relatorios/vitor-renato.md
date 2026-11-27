# Relatório de Extreme Programming (XP) — Vitor Cauã e Renato Veras

[← Voltar para o README principal](../../README.md)

**Dupla:** Vitor Cauã (*P.O. e Designer*) e Renato Veras (*Áudio e SFX*)

---

## 1. Introdução
Este relatório descreve como nossa dupla aplicou práticas de Extreme Programming (XP) durante a sprint atual do projeto do jogo. Como o time tem funções diferentes (desenvolvimento, QA, design, áudio e produto), adaptamos o trabalho em par para as nossas áreas: em vez de programarmos juntos, trabalhamos em par no alinhamento entre identidade visual, áudio e prioridades do produto.

## 2. Composição e papéis
* **Vitor Cauã:** Product Owner e Designer. Responsável pelo backlog, pelas prioridades e pelo design da interface (HUD, ícones e ferramentas).
* **Renato Veras:** Responsável por soundtrack e efeitos sonoros (SFX) e feedback contínuo do design.

No contexto do XP, o Vitor exerce também o papel de cliente: define as histórias de usuário e valida as entregas.

## 3. Atividades realizadas na sprint

### 3.1 Design e interface (Vitor)
* Atualização do HUD para um design mais polido e menos agressivo, melhorando a leitura e o conforto visual do jogador.
* Substituição de ícones e ferramentas por versões mais coerentes com a nova identidade visual.
* Implementação de funcionamentos dinâmicos na interface (elementos que respondem às ações do jogador).

### 3.2 Gestão do produto (Vitor, como P.O.)
* Repriorização do backlog para dar mais valor a diálogos melhores e mais curtos, evitando que o jogo se torne um "simulador de leitura de diálogos" e mantendo o foco na jogabilidade.

### 3.3 Áudio e SFX (Renato)
* Soundtrack e SFX já foram criados. Por decisão de prioridade, a integração ao jogo ficou para uma etapa posterior.
* Em paralelo, a dupla está avaliando quais áudios entrarão no jogo, para que a integração seja rápida quando chegar a vez deles no backlog.

## 4. Como aplicamos as práticas do XP

| Prática do XP | Como aplicamos |
| :--- | :--- |
| **Trabalho em par (adaptado)** | A dupla alinha design visual e áudio, revisando o trabalho do outro. Como nossa área não é código, o par acontece na definição da identidade do jogo, e não em programação conjunta. |
| **Cliente presente (Whole Team)** | O Vitor, como P.O., define histórias de usuário e prioridades, e está sempre disponível para tirar dúvidas do time. |
| **Jogo de planejamento** | A repriorização do backlog (diálogos curtos primeiro, áudio depois) foi feita com base no valor para o jogador e na dependência entre tarefas. |
| **Releases pequenas** | Cada sprint entrega uma versão jogável com melhorias incrementais de interface, sem esperar um "grande lançamento". |
| **Design simples** | O novo HUD reduz elementos agressivos e mantém só o necessário para o jogador. |
| **Feedback contínuo** | As mudanças de interface e diálogos são revisadas com o time e testadas pelos QAs para ajustar rapidamente. |
| **Ritmo sustentável** | A decisão de adiar o áudio evita sobrecarga e mantém o foco no que mais agrega valor agora. |

## 5. Decisões tomadas e justificativas
* **Diálogos curtos:** o jogo deve priorizar a ação e a experiência do jogador, e textos longos prejudicam o ritmo.
* **HUD menos agressivo:** uma interface mais limpa melhora a legibilidade e reduz a fadiga visual.
* **Áudio adiado, mas já planejado:** como o material já existe, o risco é baixo, e o time pode concentrar esforço em interface e conteúdo antes da integração.

## 6. Próximos passos da sprint
* Continuar a atualização de ícones e ferramentas.
* Criar novas telas do jogo.
* Acompanhar o backlog e ajustar as prioridades conforme o feedback.
* Definir a lista final de áudios a serem integrados e planejar sua entrada no backlog.

## 7. Desafios e aprendizados
* **Desafio:** aplicar o XP em funções que não programam. A solução foi adaptar as práticas (par, cliente presente, releases pequenas) à realidade de design, produto e áudio.
* **Desafio:** equilibrar a vontade de entregar tudo (como o áudio) com a necessidade de priorizar. O backlog serviu como ferramenta de decisão.
* **Aprendizado:** o XP funciona além do código quando o foco está em comunicação, feedback rápido e entregas pequenas.

## 8. Conclusão (Sprint 3)
A dupla aplicou XP de forma adaptada, valorizando comunicação, simplicidade e entregas incrementais. Os resultados desta sprint foram um HUD mais limpo, ícones e ferramentas renovados, backlog melhor priorizado e áudio preparado para a integração.

---

## 9. Atualização Final — Sprint 4 (Entrega 4)

Na reta final do projeto (a partir da introdução do modo 3D em 20/11 até o fechamento da Entrega 4), a dinâmica de trabalho em par evoluiu para **programação em par e design conjunto direto no código**, integrando todas as frentes pendentes (3D, 2D, áudio, tutorial, cutscenes e balanceamento).

### 9.1 Parcerias de trabalho na Sprint 4
* **Vitor Cauã e Renato Veras (par principal):** atuaram juntos em toda a evolução a partir do modo 3D — incluindo mapa e protagonista 3D, setores e elevador, NPCs e evidências no 3D, rotação de suspeitos, seta-guia para jogadores iniciantes, modo difícil, easter-eggs, cutscenes, animações, correção de compilação pós-modularização e integração final de áudio/SFX.
* **Vitor Cauã e Andressa Souza:** trabalharam em conjunto a partir do modo 3D na refatoração do tutorial (movimentação 3D, diálogos e ferramentas), melhorias de UI das ferramentas, correções no modo 2D, paletas secretas com reset por run, preview dos modos no menu e ajustes do Modo Dev.
* **Vitor Cauã e João Gabriel:** trabalharam juntos nos diálogos novos, rodadas de feedback e acompanhamento de issues (*issue tracking*). Como Scrum Master, João Gabriel conduziu reuniões periódicas a cada 4 dias para destravar problemas de comunicação da equipe, nas quais Vitor Cauã validava e aprovava as mudanças como Product Owner (P.O.).

> **Nota sobre o histórico Git:** Nem todos os commits feitos em par a partir do modo 3D possuem o trailer `Co-authored-by` registrado no Git, pois a prática de incluir coautoria explícita nos commits passou a ser adotada posteriormente pela dupla.

### 9.2 Práticas de XP consolidadas na Sprint 4
| Prática do XP | Como aplicamos na Sprint 4 |
| :--- | :--- |
| **Programação em par** | Sessões conjuntas entre Vitor, Renato e Andressa na implementação e ajuste fino dos modos 3D/2D, cutscenes, áudio e tutorial. |
| **Cliente presente (P.O.) e Scrum** | Reuniões de alinhamento a cada 4 dias conduzidas por João Gabriel (SM) com aprovação direta de escopo e mudanças por Vitor Cauã (P.O.). |
| **Integração contínua e releases pequenas** | Entregas incrementais diárias entre 19/11 e 25/11 (modularização, novos andares 2D/3D, áudio, modo difícil, preview e cutscenes). |
| **Feedback contínuo** | Ciclos rápidos de teste e ajuste em cima de usabilidade (tutorial, seta-guia, rotação de suspeitos e balanceamento de tokens). |

### 9.3 Resultado final
A Sprint 4 consolidou a visão completa do produto: os áudios e SFX planejados na Sprint 3 foram integrados, os modos 2D e 3D passaram a coexistir com preview e tutorial dedicados, e o fluxo de comunicação entre P.O., Scrum Master e desenvolvedores garantiu a entrega final dentro do prazo.
