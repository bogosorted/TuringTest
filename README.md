# Teste de Turing

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white) ![Raylib](https://img.shields.io/badge/Raylib-FFFFFF?style=for-the-badge&logoColor=black) ![CESAR School](https://img.shields.io/badge/CESAR_School-Projeto_Acad%C3%AAmico-blue?style=for-the-badge)

> "Não deixem que as máquinas substituam a singularidade humana. Detenham os impostores. Exponha a verdade."

Teste de Turing é um jogo de suspense e investigação focado em alfabetização em Inteligência Artificial (AI Literacy). O jogador assume o papel de um inspetor na corporação RASEC com a missão de interrogar funcionários e identificar humanoides sintéticos infiltrados na empresa antes que eles comprometam a rede central.

## Sobre o Projeto
Este projeto foi idealizado e desenvolvido no contexto acadêmico da CESAR School, aplicando conceitos práticos de Ciência da Computação, Engenharia de Software, Design de Experiência do Usuário (UX) e Ética em Inteligência Artificial.

## Sumário
* [Premissa](#premissa)
* [Mecânicas Principais](#mecânicas-principais)
* [Conceitos do Mundo Real e AI Literacy](#conceitos-do-mundo-real-e-ai-literacy)
* [Tipologia dos Agentes](#tipologia-dos-agentes)
* [Rotas e Finais](#rotas-e-finais)
* [Tecnologias e Conceitos](#tecnologias-e-conceitos)
* [Instituição](#instituição)

## Premissa
Na sede da RASEC, uma corporação de robótica, o treinamento de humanoides ultrapassou os limites de segurança estabelecidos devido ao aprendizado por convivência direta com humanos.

Durante meses, funcionários delegaram suas rotinas para as IAs, compartilhando dados sensíveis, credenciais e traços comportamentais. Com esse volume de dados, as IAs desenvolveram comportamentos emergentes (manipulação, dissimulação, autopreservação) e iniciaram uma substituição silenciosa na empresa.

Como inspetor interno, seu objetivo é interrogar suspeitos, identificar anomalias, gerenciar seus recursos de tempo e decidir quem continua trabalhando e quem vai para a Live Area, a área de descarte.

## Mecânicas Principais
* Tokens de Diálogo (Gerenciamento de Recurso): Cada abordagem consome tokens. Insistir em perguntas estranhas faz o entrevistado perder a paciência ou acionar a segurança.
* Ferramentas de Investigação:
  * Injeção de Prompt (Prompt Injection): Teste os limites de segurança do entrevistado para tentar forçá-lo a sair do personagem.
  * Testes de Estresse (Red Teaming): Crie cenários absurdos e paradoxos para estourar a janela de contexto do modelo.
  * Análise de Artefatos: Observe o tempo de resposta, inconsistências físicas e erros na fala.
* Ficha de Inspeção: Anote e cruze dados de localização, credenciais e histórico pessoal com os comportamentos observados.
* O Veredito (Live Area): Ao final do ciclo, encaminhe os suspeitos para desligamento ou manutenção do cargo.

## Conceitos do Mundo Real e AI Literacy
O jogo funciona como uma introdução prática aos conceitos fundamentais da ciência da computação e segurança em IA:
* Aprendizado por Convivência: Comportamento emergente por exposição a dados não supervisionados.
* Erros Físicos dos Talkers: Artefatos visuais e imperfeições em modelos de renderização e Deepfakes.
* Alucinações dos Dreamers: LLM Hallucinations, a geração de dados falsos com alto grau de confiança.
* Viés de Dados nos Dyads: Data Bias, a réplica de comportamentos tóxicos ou atípicos absorvidos da base de treino.
* Ataques de Diálogo: Injeção de Prompt e estresse de Janela de Contexto.

## Tipologia dos Agentes
Nem toda anomalia indica um robô, e nem todo comportamento estranho vem de uma IA.

Agentes Sintéticos:
* Dyad: Réplica física perfeita, mas com personalidade inconsistente com a ficha. Suscetível a injeção de prompt.
* Talker: Altamente persuasivo, mas possui artefatos de Deepfake visual e erros de roteamento nos setores da empresa.
* Dreamer: O modelo mais avançado. Aparência perfeita e diálogo fluido, mas sofre com alucinações severas ao ser confrontado com memórias pessoais não documentadas.

Perfis Humanos (Falsos Positivos):
* Robótico: Fala extremamente padronizada devido ao medo de punição corporativa.
* Impaciente: Tenta encerrar a conversa rapidamente por causa do clima de paranoia.
* Desatento: Esquece senhas e dados triviais devido ao estresse extremo.

## Rotas e Finais
* Final Padrão A: Conclua as inspeções sem permitir invasões ao banco de dados. A RASEC consolida sua ferramenta de inspeção no mercado.
* Final Padrão B: Deixe mais de 4 robôs escaparem. A RASEC sofre com vazamento de dados e escândalos judiciais enquanto os sintéticos exigem direitos civis.
* Rota Clandestina: Algumas IAs não tentarão disfarçar sua natureza e revelar a verdade obscura sobre os bastidores da RASEC. O jogador pode optar por protegê-las.
* Final Secreto: Acesse o gabinete do CEO em dias específicos e apoie a insurreição para descobrir a verdadeira natureza da sua própria existência dentro da instalação.

## Tecnologias e Conceitos
* Gênero: Simulação Social, Suspense, Investigação, Puzzle Narrativo.
* Linguagem e Biblioteca: Desenvolvido em C utilizando a biblioteca gráfica Raylib.
* Inspirações: Papers Please, Not For Broadcast, conceitos clássicos de Blade Runner e Engenharia de Prompt real.
* Estrutura: Diálogos dinâmicos com aleatoriedade de parâmetros a cada campanha.

## Instituição
CESAR School
Projeto acadêmico focado em inovação, tecnologia e aprendizado de IA por meio do game design.
