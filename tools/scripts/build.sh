#!/usr/bin/env bash
set -euo pipefail

CMAKE_PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
HUYNITY_BUILD_DIR="${HUYNITY_BUILD_DIR:-$CMAKE_PROJECT_DIR/build}"
mkdir -p -- "$HUYNITY_BUILD_DIR"
HUYNITY_BUILD_DIR="$(cd -- "$HUYNITY_BUILD_DIR" && pwd -P)"

# Существующая сборка сохраняет generator, toolchain и выбранные опции.
if [[ -f "$HUYNITY_BUILD_DIR/CMakeCache.txt" ]]; then
    cmake -S "$CMAKE_PROJECT_DIR" -B "$HUYNITY_BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
    cmake --preset ninja -S "$CMAKE_PROJECT_DIR" -B "$HUYNITY_BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

cmake --build "$HUYNITY_BUILD_DIR" --parallel

if [[ ! -f "$HUYNITY_BUILD_DIR/compile_commands.json" ]]; then
    printf 'CMake не создал %s/compile_commands.json\n' "$HUYNITY_BUILD_DIR" >&2
    exit 1
fi

ln -sfn -- "$HUYNITY_BUILD_DIR/compile_commands.json" "$CMAKE_PROJECT_DIR/compile_commands.json"
