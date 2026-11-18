#!/usr/bin/env bash
set -euo pipefail

# Sempre vai para a pasta raiz do projeto (mesmo clicando pelo atalho da Area de Trabalho)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

export PATH="$HOME/.local/bin:$PATH"

# Se o projeto ainda nao foi configurado ou se algum arquivo .c/.h/CMakeLists.txt mudou,
# recompila incrementalmente de forma ultra-rapida antes de abrir o jogo!
if [ ! -f "build/CMakeCache.txt" ]; then
    cmake -B build >/dev/null
fi

cmake --build build >/dev/null

exec ./build/turing_test "$@"
