#!/usr/bin/env bash
# Crée une release du fork : vérifie le build et les tests, crée un tag daté et le pousse.
# Le tag déclenche .github/workflows/release.yml sur GitHub (build Linux + Windows, tests,
# publication du relinker, des bibliothèques .prx et de la doc). Les Actions doivent être activées.
# Usage : scripts/release.sh [--local] [--skip-tests] [--dry-run]
#   --local       construit aussi l'archive Linux dans dist/ (sans GitHub)
#   --skip-tests  ne relance pas les tests avant de taguer
#   --dry-run     affiche le tag sans le créer
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

LOCAL=0; TESTS=1; DRY=0
for arg in "$@"; do
    case "$arg" in
        --local) LOCAL=1 ;;
        --skip-tests) TESTS=0 ;;
        --dry-run) DRY=1 ;;
        -h|--help) sed -n 2,9p "$0"; exit 0 ;;
        *) echo "Argument inconnu : $arg" >&2; exit 1 ;;
    esac
done

[[ "$(git rev-parse --abbrev-ref HEAD)" == "dev" ]] || { echo "Passe sur la branche dev" >&2; exit 1; }
[[ -z "$(git status --porcelain)" ]] || { echo "Des modifications ne sont pas commitées" >&2; exit 1; }
git fetch -q origin dev
[[ "$(git rev-parse HEAD)" == "$(git rev-parse origin/dev)" ]] || { echo "dev n'est pas identique à origin/dev : pousse ou tire d'abord" >&2; exit 1; }

BASE="v$(date +%Y.%m.%d)-fork"
TAG="$BASE"
n=2
while git rev-parse -q --verify "refs/tags/$TAG" >/dev/null || git ls-remote --exit-code --tags origin "$TAG" >/dev/null 2>&1; do
    TAG="$BASE.$n"; n=$((n + 1))
done
echo "Tag : $TAG ($(git rev-parse --short HEAD))"
[[ $DRY -eq 1 ]] && exit 0

if [[ $TESTS -eq 1 ]]; then
    scripts/build.sh release --test
else
    scripts/build.sh release
fi

if [[ $LOCAL -eq 1 ]]; then
    rm -rf dist
    python3 tools/package_release.py --platform linux --build build/release --output dist --version "$TAG"
    python3 tools/package_release.py --docs docs/user --output dist
    cp docs/FORK.md dist/
    echo "Archives locales : $ROOT/dist"
fi

git tag -a "$TAG" -m "Release $TAG"
git push origin "$TAG"
echo "Tag poussé. Suivi : https://github.com/Ichigrex/AnyPS5/actions  —  release : https://github.com/Ichigrex/AnyPS5/releases/tag/$TAG"
