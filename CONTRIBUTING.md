# Guia de Contribuição — Teste de Turing

Obrigado pelo interesse em contribuir com o **Teste de Turing**! Este documento reúne tudo o que você precisa para configurar o ambiente de desenvolvimento, compilar o jogo, entender a estrutura do projeto e enviar suas alterações.

---

## 1. Pré-requisitos

O projeto é escrito em **C (padrão C99)** e usa **CMake (>= 3.14)** para baixar e compilar a **Raylib** automaticamente via `FetchContent`.

### Geral (todos os sistemas)
- **Git**
- **CMake** (versão 3.14 ou superior)
- **Compilador C compatível com C99**
- Conexão com a internet na **primeira configuração** (o CMake baixa a Raylib em `build/_deps/`; nas compilações seguintes ela fica em cache local com `FETCHCONTENT_UPDATES_DISCONNECTED ON`).

### Linux (Ubuntu/Debian)
```bash
sudo apt update && sudo apt install -y \
  cmake build-essential git \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
  libxinerama-dev libwayland-dev libxkbcommon-dev
```

### Linux (Arch Linux)
```bash
sudo pacman -S --needed \
  cmake base-devel git \
  alsa-lib libx11 libxrandr libxi \
  mesa glu libxcursor libxinerama wayland libxkbcommon
```

> **Dica de performance (Linux):** se `ccache` ou o linker `mold` estiverem instalados no `PATH` (ou em `~/.local/bin`), o [`CMakeLists.txt`](CMakeLists.txt) os detecta e ativa automaticamente.

### Windows (nativo)
1. Instale o **Git** e o **CMake** (ou use o CMake que vem com o Visual Studio).
2. Instale o **Visual Studio Community** marcando a carga **Desenvolvimento para Desktop com C++** (para ter o compilador MSVC e o Windows SDK), ou utilize **MinGW-w64**.
3. O projeto já vincula automaticamente as bibliotecas de mídia do Windows (`mf`, `mfplat`, `mfreadwrite`, `mfuuid`, `ole32`) exigidas para captura de webcam no Windows.

---

## 2. Configurando e Rodando Localmente

### Clone o repositório
```bash
git clone https://github.com/bogosorted/TuringTest.git
cd TuringTest
```

### Linux / macOS
Você pode usar o script de conveniência na raiz (configura na primeira execução, recompila incrementalmente e abre o jogo):
```bash
./run_turing_test.sh
```

Ou rodar os passos manualmente:
```bash
# 1. Configurar o CMake (primeira vez ou ao alterar CMakeLists.txt)
cmake -B build

# 2. Compilar
cmake --build build -j

# 3. Executar
./build/turing_test
```

### Windows (PowerShell / Terminal do VS Code)
```powershell
# 1. Configurar o CMake
cmake -B build

# 2. Compilar
cmake --build build

# 3. Executar (o caminho varia conforme o gerador: MSVC usa pasta Debug/Release)
.\build\Debug\turing_test.exe
# ou, com MinGW/Ninja:
.\build\turing_test.exe
```

> **Sobre a pasta `assets/`:** no Linux/Unix, o CMake cria um link simbólico `build/assets -> assets`. No Windows, ele copia `assets/` para dentro de `build/`. Em execução, o `main.c` ajusta o diretório de trabalho automaticamente para localizar `assets/`.

### Atalhos úteis durante testes
- **`F11`**: alterna entre modo janela e tela cheia (salvo em `settings.cfg`).
- **`F10` ou `Ctrl+Q`**: encerra o jogo imediatamente de qualquer cena.
- **`ESC`**: abre o menu de pausa durante o gameplay.

---

## 3. Estrutura do Projeto

```text
TuringTest/
├── main.c                  # Ponto de entrada, loop principal, BGM global e troca de cenas
├── CMakeLists.txt          # Configuração de build (C99 + Raylib via FetchContent)
├── run_turing_test.sh      # Script de build incremental + execução no Linux
├── scenes/                 # Cenas (telas) do jogo
│   ├── menu_scene.*        # Menu principal e acesso às opções
│   ├── intro_scene.*       # Introdução narrativa antes do Dia 0 / Dia 1
│   ├── selection_scene.*   # Seleção de local / funcionário
│   ├── gameplay_scene.*    # Interrogatório, ferramentas, câmera, ficha e evidências
│   └── verdict_scene.*     # Tela de veredito final, pontuação e ranking
├── utils/                  # Módulos e sistemas reutilizáveis
│   ├── raycanvas.*         # Canvas virtual (640x360), âncoras, escala, fade e paletas
│   ├── game_state.*        # Estado da run (dias, tokens, suspeitos, visão do jogador)
│   ├── dialog_system.*     # Motor da árvore de diálogos e escolhas
│   ├── dialog_data.*       # Árvores de diálogo em C e catálogo de evidências
│   ├── transition.*        # Transições com fade-out e tela de texto entre dias
│   ├── settings.*          # Leitura/escrita de configurações locais (settings.cfg)
│   ├── options_ui.*        # Interface compartilhada de Opções (volumes, tela cheia, paleta)
│   ├── ranking.*           # Persistência do ranking local (ranking.cfg)
│   ├── webcam.*            # Captura de câmera para foto do jogador (V4L2/Media Foundation)
│   ├── mural_ui.*          # Interface do mural de investigação
│   ├── mapa_holograma.*    # Mapa interativo / holograma de navegação
│   ├── evidencia_holograma.* # Visualização holográfica de evidências
│   └── setas_3d.*          # Indicadores/setas 3D da interface
├── assets/                 # Sprites, áudio, fontes, paletas, diálogos e dados
└── docs/                   # Diagramas de User Stories, storyboards e relatórios XP
```

---

## 4. Convenções de Código e Boas Práticas

- **Padrão C99 e portabilidade:** mantenha compatibilidade tanto com **GCC/Clang** (Linux) quanto com **MSVC** (Windows). Evite extensões exclusivas de um único compilador.
- **Novos arquivos `.c`:** sempre que criar um novo módulo em `scenes/` ou `utils/`, adicione o `.c` à lista do `add_executable(turing_test ...)` no [`CMakeLists.txt`](CMakeLists.txt).
- **Ciclo de vida das cenas:** toda cena possui `Init*Scene()`, `UpdateDraw*Scene()` e `Unload*Scene()`. Qualquer textura, imagem, fonte, som ou shader carregado no `Init` **deve** ser liberado no `Unload` para evitar vazamento de memória/VRAM.
- **Caminhos de arquivos:** referencie recursos usando sempre caminhos relativos com `/` (ex.: `"assets/sprites/..."`), para funcionar corretamente tanto no Linux quanto no Windows.
- **Não versione arquivos locais gerados:** `build/`, `settings.cfg`, `ranking.cfg`, executáveis (`*.exe`, `turing_test`) e arquivos de IDE (`.vscode/`) já estão no [`.gitignore`](.gitignore) e não devem ser enviados.

---

## 5. Fluxo de Contribuição (Git e Pull Requests)

1. **Sincronize com a `main` antes de começar:**
   ```bash
   git checkout main
   git pull origin main
   ```
2. **Crie uma branch descritiva para sua tarefa:**
   ```bash
   git checkout -b feat/nome-da-funcionalidade
   # ou: fix/descricao-do-bug, docs/atualizacao-readme
   ```
3. **Teste o build e jogue o fluxo alterado** antes de commitar (verifique também se não há warnings novos ou regressões nas trocas de cena).
4. **Escreva mensagens de commit claras e objetivas**, explicando *o que* e *por que* mudou:
   - Bom: `Corrige carregamento de paleta ao retornar ao menu principal`
   - Bom: `feat: adiciona indicador visual de tokens restantes no interrogatorio`
   - Evite: `WIP`, `ajustes`, `mudancas`, `fix`.
5. **Envie sua branch e abra um Pull Request** para `main` descrevendo:
   - O que foi implementado ou corrigido;
   - Como testar a mudança;
   - Se envolve novos assets ou dependências no `CMakeLists.txt`.
