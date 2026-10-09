#!/usr/bin/env bash
# Récupère les derniers commits de boykopovar/AnyPS5 et les fusionne dans dev.
# Usage : scripts/sync-upstream.sh [--push]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

git remote get-url upstream >/dev/null 2>&1 || git remote add upstream https://github.com/boykopovar/AnyPS5.git
git fetch upstream main
git checkout dev
git pull --ff-only origin dev

NEW=$(git rev-list --count dev..upstream/main)
if [[ $NEW -eq 0 ]]; then
    echo "Déjà à jour."
    exit 0
fi
echo "$NEW nouveaux commits :"
git log --oneline --merges -n 30 dev..upstream/main
git merge --no-edit upstream/main
git submodule update --init --recursive

[[ "${1:-}" == "--push" ]] && git push origin dev
echo "Fusion faite. Recompile avec scripts/build.sh."
