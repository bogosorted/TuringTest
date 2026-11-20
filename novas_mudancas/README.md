# Guia de Novas Mudanças (Staging de Conteúdo)

Esta pasta foi criada para você adicionar **novas fotos, locais, funcionários, evidências e diálogos** de forma organizada, sem misturar rascunhos com os arquivos ativos do jogo em `assets/`.

## Estrutura das Pastas

- `funcionarios/`
  - `fotos/`: Retratos oficiais dos funcionários (ex: `char_nome_sobrenome.png`).
  - `silhuetas/`: Silhuetas dos funcionários nos cenários e no veredito (ex: `sil_nome.png`, `sil_full_nome.png`).
  - `biometria/`: Imagens de retina e digitais para a ferramenta Deepfake (ex: `df_nome_retina.png`, `df_nome_digital.png`).
  - `modelo_ficha.txt`: Modelo pronto para preencher os dados de um novo funcionário.
- `locais/`
  - `2d_backgrounds/`: Fundos 2D de novas salas (`spr_background_*.png`).
  - `3d_texturas/`: Texturas e referências para salas 3D.
  - `mascaras/`: Máscaras de clique de portas, escadas e objetos interagíveis (`mask_*.png`).
- `evidencias/`: Sprites de close-up e ícones de novas pistas (`spr_*.png`).
- `dialogos/`: Rascunhos de diálogos (`.txt` / `.json`) antes de irem para `assets/dialogues/`.
- `rascunhos/`: Assets experimentais e referências antigas (incluindo os antigos arquivos de `PossibleAssets/`).

## Como integrar uma nova mudança ao jogo

1. **Novo Funcionário**:
   - Copie a foto para `assets/sprites/personagens/char_<nome>.png` e a silhueta para `sil_<nome>.png`.
   - Adicione os dados seguindo `modelo_ficha.txt` em `assets/data/fichas_funcionarios.txt`.
   - Adicione os arquivos de diálogo em `assets/dialogues/` e registre em `src/systems/dialog_data.c`.
   - Se necessário, atualize o carregamento de texturas de suspeitos em `src/scenes/gameplay_scene.c` (`InitGameplayScene`).

2. **Novo Local / Sala**:
   - Coloque o fundo em `assets/sprites/locais/<andar>/`.
   - Adicione o `enum` em `LocalRasec` (`src/gameplay/gameplay_data.h`).
   - Adicione a linha correspondente em `LOCAIS[]` e `HOLO_SALAS[]` (`src/gameplay/gameplay_data.c`), além das conexões em `GameplayDestinoSeta()` / `GameplayLocaisAcessiveis()`.
   - Se for uma sala 3D nova, crie um módulo em `src/world/` seguindo o padrão de `sala_recepcao.c`.
