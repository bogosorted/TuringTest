# Relatório de Extreme Programming (XP) — Gabriel Aragão e Andressa

[← Voltar para o README principal](../../README.md)

**Dupla:** Gabriel Aragão (*Desenvolvedor*) e Andressa (*UX / Testes*)

---

## 1. Introdução
Este relatório conta como nossa dupla trabalhou junta aplicando práticas de Extreme Programming (XP) na sprint atual do jogo. Nosso foco principal foi criar o tutorial interativo forçado do Dia 1 e ajustar o fluxo de diálogos e tokens, garantindo que quem joga pela primeira vez entenda a ideia do jogo e as mecânicas na prática, sem ficar preso em textos longos e cansativos.

## 2. Composição e papéis
* **Gabriel Aragão:** Desenvolvimento e implementação no código. Responsável por programar a lógica dos passos do tutorial no Dia 1, o bloqueio de cliques fora de ordem, os destaques visuais na tela e a integração com o sistema de diálogos e tokens.
* **Andressa:** Design de experiência, concepção do fluxo e validação. Responsável por pensar na sequência didática do tutorial, revisar os textos curtos de cada etapa e testar se a transição entre os passos estava fluida e intuitiva para o jogador.

Na prática, colaboramos trocando ideias o tempo todo: Andressa ajudava a definir como o jogador deveria ser guiado sem se entediar e testava o resultado, enquanto Aragão transformava essas ideias em código e ajustava a interface na hora.

## 3. Atividades realizadas na sprint

### 3.1 Planejamento e fluxo do tutorial (Andressa e Aragão)
* Definição dos 6 passos essenciais do Dia 1: escolher um suspeito, abrir o menu de ferramentas, consultar a ficha pessoal, cruzar os dados com a fala, fazer uma pergunta gastando token e usar a prancheta/dica lógica.
* Criação de textos curtos e diretos para os balões de instrução, evitando blocos grandes de texto.

### 3.2 Implementação do tutorial interativo (Aragão)
* Programação do sistema de passos guiados na cena de interrogatório e na seleção de cenário, com molduras pulsantes e setas animadas apontando para cada botão.
* Bloqueio de cliques fora de ordem durante o treinamento e devolução automática do token de teste ao concluir o tutorial.

### 3.3 Testes de usabilidade e ajustes (Andressa)
* Testes práticos do fluxo completo do Dia 1 para verificar se os cards de explicação não cobriam os personagens, o chat ou a ficha aberta.
* Validação da fluidez dos cliques, garantindo que cada passo avançasse automaticamente assim que o jogador realizasse a ação pedida.

Fazer esse alinhamento juntos evitou criar um tutorial travado e deixou a introdução do jogo bem mais dinâmica.

## 4. Como aplicamos as práticas do XP

| Prática do XP | Como aplicamos |
| :--- | :--- |
| **Programação em par** | Trabalhamos juntos definindo cada passo do tutorial, implementando no código, rodando o jogo na hora e já ajustando o que precisava melhorar. |
| **Testes** | Cada passo do tutorial foi testado diretamente no jogo (Dia 1) para garantir que nenhum clique quebrasse a sequência ou travasse a tela. |
| **Integração contínua** | O tutorial foi integrado direto nas cenas reais de seleção e interrogatório, aproveitando o sistema de diálogos, fichas e prancheta já existentes. |
| **Releases pequenas** | Primeiro estruturamos o estado do tutorial no Dia 1, depois adicionamos os destaques visuais e, por fim, refinamos o posicionamento e os textos. |
| **Design simples** | Em vez de criar uma cena separada cheia de regras, fizemos o jogador aprender usando a própria tela do jogo com avisos curtos. |
| **Feedback contínuo** | Testamos o build a cada mudança para ver na prática se a posição das caixas e o tamanho dos textos estavam bons de ler. |
| **Propriedade coletiva** | Tanto a lógica do fluxo quanto o visual e os textos do tutorial foram discutidos, testados e ajustados pelos dois. |

## 5. Decisões tomadas e justificativas
* **Aprender fazendo em vez de só ler texto:** decidimos que cada passo só avança quando a pessoa clica na ferramenta certa, porque assim ela aprende jogando e não fica entediada.
* **Bloquear cliques fora de hora no tutorial:** travamos os outros botões durante cada passo para evitar que o jogador saia pelo mapa sem querer ou gaste tokens antes de entender como eles funcionam.
* **Devolver o token usado no teste:** ao finalizar o tutorial, restauramos os 12 tokens do Dia 1 para que o jogador não seja prejudicado pela pergunta de treinamento.

## 6. Próximos passos da sprint
* Coletar feedback do resto da equipe jogando o tutorial do Dia 1.
* Ajustar pequenos detalhes visuais ou de texto caso alguém tenha dúvida em algum passo.
* Integrar o tutorial com os novos elementos sonoros e visuais das próximas entregas.
* Continuar refinando a experiência dos diálogos nos dias seguintes.

## 7. Desafios e aprendizados
* **Desafio:** explicar várias mecânicas (suspeitos, ficha, tokens, prancheta, dica lógica e mapa) sem poluir a tela ou cansar o jogador. A solução foi dividir em 6 ações rápidas e diretas.
* **Desafio:** posicionar os balões do tutorial na resolução 640x360 sem tampar a ficha, a prancheta ou o chat. Resolvemos testando e ajustando as coordenadas em conjunto.
* **Aprendizado:** testar na hora enquanto desenvolvemos em dupla economiza muito tempo e deixa a experiência do jogo muito mais fluida.

## 8. Conclusão
O trabalho em dupla entre Andressa e Aragão permitiu criar um tutorial de Dia 1 leve, fluido e direto ao ponto. Seguindo as práticas do XP com testes rápidos e conversa constante, entregamos uma introdução que ensina as mecânicas principais na prática e melhora bastante a primeira impressão de quem vai jogar.
