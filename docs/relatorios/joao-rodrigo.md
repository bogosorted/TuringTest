# Relatório de Extreme Programming (XP) — João Gabriel e José Rodrigo Bispo

[← Voltar para o README principal](../../README.md)

**Dupla:** João Gabriel (*Dev / Scrum Master*) e José Rodrigo Bispo (*QA*)

---

## 1. Introdução
Este relatório descreve como nossa dupla aplicou as práticas de Extreme Programming (XP) durante as sprints do projeto de desenvolvimento do jogo. Unindo as funções de desenvolvimento/facilitação e garantia de qualidade, o trabalho em conjunto seguiu um ciclo contínuo de implementação, facilitação de fluxo, testes e correções: João Gabriel desenvolve o código, gerencia o backlog de tarefas e facilita os rituais, enquanto José Rodrigo Bispo executa os testes, identifica defeitos e valida o correto funcionamento do sistema.

## 2. Composição e papéis
* **João Gabriel:** Desenvolvedor e Scrum Master. Responsável pela implementação de funcionalidades e mecânicas do jogo, seguindo integração contínua. Como Scrum Master, gerencia o backlog no Jira, acompanha o tempo de ciclo (*cycle time*) das tarefas, remove impedimentos e organiza as entregas da equipe.
* **José Rodrigo Bispo:** QA. Responsável pelo planejamento e execução de testes de aceitação, elaboração e organização do relatório de defeitos (bugs), validação contínua das entregas e feedback de jogabilidade e estabilidade.

No contexto do XP, os papéis da dupla se complementam: João garante a evolução técnica da build e o fluxo de tarefas, enquanto José Rodrigo atua como o primeiro usuário e filtro de qualidade, testando incrementos e direcionando correções imediatas.

## 3. Atividades realizadas nas sprints

### 3.1 Desenvolvimento e Facilitação Scrum (João Gabriel)
* Implementação e refatoração de funcionalidades essenciais do jogo durante as sprints.
* Monitoramento e facilitação do fluxo de trabalho no Jira, acompanhando o progresso das histórias de usuário e subtarefas para manter a métrica de conclusão e vazão do projeto.
* Correção de bugs mais simples identificados pelo QA, garantindo a estabilidade das builds intermediárias.

### 3.2 Qualidade e testes (José Rodrigo, como QA)
* Elaboração de cenários de teste das funcionalidades desenvolvidas.
* Identificação, registro e categorização de falhas no relatório de bugs, fornecendo os passos para reprodução e os critérios de aceitação necessários.
* Validação e re-teste das correções efetuadas pelo desenvolvedor antes do fechamento definitivo das tarefas.

### 3.3 Gestão, Integração e Qualidade (João Gabriel e José Rodrigo)
* Alinhamento frequente entre desenvolvimento e QA para priorização do backlog de correções nas sprints.
* Triagem conjunta dos defeitos reportados para definir a gravidade e o impacto das falhas no funcionamento do jogo.
* Acompanhamento das subtarefas e histórias de usuário concluídas, garantindo que o ciclo de vida dos tickets do status "Fazendo" para "Feito" ocorresse com a devida validação de qualidade.

## 4. Como aplicamos as práticas do XP

| Prática do XP | Como aplicamos |
| :--- | :--- |
| **Trabalho em par (adaptado)** | A dupla atuou em ciclo iterativo: João desenvolveu e disponibilizou a build, Rodrigo testou e registrou inconsistências, e João aplicou os ajustes. |
| **Testes contínuos** | Entregas passaram pela validação do QA antes de finalizadas. As falhas encontradas geram registros no relatório de bugs e reorientam as prioridades de desenvolvimento. |
| **Integração contínua** | O código e os recursos do jogo foram integrados de maneira incremental ao longo das semanas, evitando o grande acúmulo de código não testado para o final da sprint. |
| **Releases pequenas** | Cada iteração gerou compilações jogáveis e funcionais com melhorias incrementais e correções graduais de bugs. |
| **Design simples** | Soluções de código diretas e focadas na funcionalidade requisitada, priorizando a estabilidade e a facilidade de manutenção. |
| **Feedback contínuo** | O retorno sobre novos bugs ocorre de forma imediata após os testes do QA, permitindo reajustes rápidos de escopo e correções no código. |

## 5. Decisões tomadas e justificativas
* **Priorizar correções de bugs sobre novas funcionalidades:** Correções que afetam a experiência de jogo foram priorizadas para evitar o acúmulo de débito técnico e garantir builds estáveis.
* **Acompanhamento do Cycle Time:** O Scrum Master monitorou os tempos de ciclo e espera das tarefas no Jira para identificar gargalos no fluxo de trabalho e evitar que tickets ficassem retidos no status "Fazendo".
* **Validação imediata pelo QA pós-build:** Testar as tarefas assim que eram integradas reduziu o tempo de resposta entre a ocorrência e a solução dos problemas.

## 6. Próximos passos da sprint
* Concluir a homologação das subtarefas e histórias atualmente em andamento.
* Realizar uma rodada completa de testes de regressão na build consolidada das semanas 10 a 12.
* Refinar o backlog e organizar a estimativa dos próximos tickets em conjunto com a equipe.
* Manter as métricas de tempo de ciclo e o relatório de bugs atualizados para o próximo ciclo.

## 7. Desafios e aprendizados
* **Desafio:** Conciliar o papel de Scrum Master (facilitação do processo e organização das tarefas) com a carga de desenvolvimento do jogo. Solução: Utilização rigorosa do quadro de tickets e métricas de fluxo para manter a transparência e focar no ciclo curto com o QA.
* **Desafio:** Manter a agilidade no fechamento de tickets durante a descoberta de falhas complexas. Solução: Decomposição das falhas em subtarefas menores e comunicação constante para isolar a origem do bug.
* **Aprendizado:** A atuação próxima entre a facilitação/desenvolvimento e a garantia de qualidade acelera a identificação de problemas e garante um produto final com maior nível de confiabilidade e menor tempo de retrabalho.

## 8. Conclusão
A dupla aplicou as práticas de XP de forma adaptada e eficiente ao longo das sprints. A combinação da facilitação e desenvolvimento de João com o controle de qualidade executado por José Rodrigo permitiu a entrega de versões incrementais do jogo com acompanhamento transparente dos tickets e controle de defeitos. O processo estabelecido assegura uma base sólida para a continuidade do projeto nas sprints seguintes.
