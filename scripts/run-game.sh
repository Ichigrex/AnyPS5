#!/usr/bin/env bash
# Convertit un jeu avec le relinker puis le lance (optionnellement sous gdb).
# Usage : scripts/run-game.sh <dossier_du_jeu | eboot.bin> [options] [-- arguments du jeu]
#   --gdb          lance le jeu sous gdb
#   --catch-throw  avec --gdb : s'arrête sur chaque std::runtime_error levée
#   --relink       force la reconversion
#   --prepare-only convertit et prépare le dossier sans lancer le jeu
#   --lang <code>  langue système du jeu (fr, fr-CA, en-GB, ja...) ; défaut : anglais US
#   --no-check     ne lance pas le diagnostic des imports (scripts/check-game.py)
#   --out <dir>    dossier de sortie (défaut : games/<nom_du_jeu>)
#   --build <dir>  dossier de build (défaut : build/current)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build/current"
GDB=0
CATCH=0
RELINK=0
PREPARE=0
CHECK=1
OUT=""
INPUT=""
GAME_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --gdb) GDB=1 ;;
        --catch-throw) CATCH=1 ;;
        --relink) RELINK=1 ;;
        --prepare-only) PREPARE=1 ;;
        --no-check) CHECK=0 ;;
        --lang) export ANYPS5_LANGUAGE="$2"; shift ;;
        --out) OUT="$2"; shift ;;
        --build) BUILD="$2"; shift ;;
        -h|--help) sed -n 2,11p "$0"; exit 0 ;;
        --) shift; GAME_ARGS=("$@"); break ;;
        *) INPUT="$1" ;;
    esac
    shift
done

[[ -n "$INPUT" ]] || { sed -n 2,11p "$0"; exit 1; }
if [[ -d "$INPUT" ]]; then
    GAME_DIR="$(cd "$INPUT" && pwd)"
    ELF="$GAME_DIR/eboot.bin"
else
    ELF="$(cd "$(dirname "$INPUT")" && pwd)/$(basename "$INPUT")"
    GAME_DIR="$(dirname "$ELF")"
fi
[[ -f "$ELF" ]] || { echo "Exécutable introuvable : $ELF" >&2; exit 1; }
head -c 4 "$ELF" | grep -q $'\x7fELF' || { echo "$ELF n'est pas un ELF déchiffré" >&2; exit 1; }

BUILD="$(cd "$BUILD" && pwd)"
RELINKER="$BUILD/core/relinker/relinker"
LIBS="$BUILD/core/libs/libs"
[[ -x "$RELINKER" ]] || { echo "Relinker absent : lance d'abord scripts/build.sh" >&2; exit 1; }
compgen -G "$LIBS/*.prx" >/dev/null || { echo "Aucune lib dans $LIBS : lance scripts/build.sh" >&2; exit 1; }

OUT="${OUT:-$ROOT/games/$(basename "$GAME_DIR")}"
mkdir -p "$OUT/app0"
OUT="$(cd "$OUT" && pwd)"
ln -sfn "$LIBS" "$OUT/libs"

for entry in "$GAME_DIR"/* "$GAME_DIR"/.[!.]*; do
    [[ -e "$entry" ]] || continue
    name="$(basename "$entry")"
    case "$name" in
        sce_module|sce_modules|prx) mkdir -p "$OUT/app0/$name" ;;
        *) [[ -e "$OUT/app0/$name" ]] || ln -s "$entry" "$OUT/app0/$name" ;;
    esac
done

APP="$OUT/app.elf"
if [[ $RELINK -eq 1 || ! -f "$APP" || "$ELF" -nt "$APP" || "$RELINKER" -nt "$APP" ]]; then
    FLAGS=()
    grep -q GenuineIntel /proc/cpuinfo && FLAGS+=(--to-intel)
    echo "Conversion : $ELF -> $APP ${FLAGS[*]:-}"
    "$RELINKER" "${FLAGS[@]}" "$ELF" "$APP"
    chmod +x "$APP"
fi

if [[ $CHECK -eq 1 ]] && command -v python3 >/dev/null; then
    python3 "$ROOT/scripts/check-game.py" "$ELF" --build "$BUILD" > "$OUT/check.txt" 2>&1 || true
    sed -n '1,/^$/p' "$OUT/check.txt"
    tail -n 1 "$OUT/check.txt"
    echo "  (détail : $OUT/check.txt)"
fi

[[ $PREPARE -eq 1 ]] && { echo "Prêt : $APP"; exit 0; }

cd "$OUT"
LOG="$OUT/last-run.log"
if [[ $GDB -eq 1 ]]; then
    GDB_ARGS=(-q -x "$ROOT/scripts/gdbinit")
    [[ $CATCH -eq 1 ]] && GDB_ARGS+=(-ex "catch throw std::runtime_error")
    exec gdb "${GDB_ARGS[@]}" -ex run --args "$APP" "${GAME_ARGS[@]}"
fi
echo "Lancement (journal : $LOG)"
set +e
"$APP" "${GAME_ARGS[@]}" 2>&1 | tee "$LOG"
code=${PIPESTATUS[0]}
echo "Code de sortie : $code"
exit "$code"
