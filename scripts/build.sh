#!/usr/bin/env bash
# Compilation rapide d'AnyPS5 (relinker + bibliothèques prx).
# Usage : scripts/build.sh [release|dev|debug] [--deps] [--test] [--clean]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET=dev
DEPS=0
TEST=0
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        release|dev|debug) PRESET="$arg" ;;
        --deps) DEPS=1 ;;
        --test) TEST=1 ;;
        --clean) CLEAN=1 ;;
        -h|--help) sed -n 2,3p "$0"; exit 0 ;;
        *) echo "Argument inconnu : $arg" >&2; exit 1 ;;
    esac
done

if [[ $DEPS -eq 1 ]]; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build git ccache gdb python3 \
        libx11-dev libxext-dev libvulkan1 mesa-vulkan-drivers vulkan-tools
fi

for tool in cmake ninja g++ git; do
    command -v "$tool" >/dev/null || { echo "$tool manquant : relance avec --deps" >&2; exit 1; }
done

cd "$ROOT"
git submodule update --init --recursive --jobs "$(nproc)"

BUILD_DIR="$ROOT/build/$PRESET"
[[ $CLEAN -eq 1 ]] && rm -rf "$BUILD_DIR"

EXTRA=()
if command -v ccache >/dev/null; then
    EXTRA+=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cmake --preset "$PRESET" "${EXTRA[@]}"
cmake --build --preset "$PRESET" --parallel "$(nproc)"

if [[ $TEST -eq 1 ]]; then
    ctest --preset "$PRESET" --parallel "$(nproc)" --timeout 120
fi

ln -sfn "$PRESET" "$ROOT/build/current"
echo
echo "OK : $BUILD_DIR"
echo "  relinker : $BUILD_DIR/core/relinker/relinker"
echo "  libs     : $BUILD_DIR/core/libs/libs/*.prx"
echo "  (build/current pointe sur ce build, utilisé par scripts/run-game.sh)"
