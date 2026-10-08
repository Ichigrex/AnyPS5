#!/usr/bin/env python3
"""Diagnostic d'un jeu avant lancement, en une commande.

Raccourci vers tools/import_audit.py (outil du projet d'origine) : convertit eboot.bin avec
`relinker --registry` dans un dossier temporaire, puis audite ses imports contre les
bibliothèques compilées et les modules du jeu.

Usage : scripts/check-game.py <dossier_du_jeu | eboot.bin> [--build DIR] [--names] [--json FICHIER]
"""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from nid_names import DEFAULT_CACHE, load_db  # noqa: E402

MODULE_DIRS = ("sce_module", "sce_modules", "prx")


def main():
    parser = argparse.ArgumentParser(description="Diagnostic des imports d'un jeu")
    parser.add_argument("game", help="dossier du jeu (contenant eboot.bin) ou chemin de l'ELF")
    parser.add_argument("--build", default=str(ROOT / "build" / "current"), help="dossier de build")
    parser.add_argument("--names", action="store_true", help="nomme les fonctions absentes (télécharge la base de NID la 1re fois)")
    parser.add_argument("--json", help="écrit le résultat complet dans ce fichier")
    args = parser.parse_args()

    game = Path(args.game).resolve()
    elf = game / "eboot.bin" if game.is_dir() else game
    build = Path(args.build)
    relinker = build / "core" / "relinker" / "relinker"
    libs = build / "core" / "libs" / "libs"
    if not elf.is_file():
        sys.exit(f"Exécutable introuvable : {elf}")
    if not relinker.is_file() or not any(libs.glob("*.prx")):
        sys.exit("Build introuvable : lance d'abord scripts/build.sh")

    audit = [sys.executable, str(ROOT / "tools" / "import_audit.py"), "--libs", str(libs)]
    for name in MODULE_DIRS:
        if (elf.parent / name).is_dir():
            audit += ["--modules", str(elf.parent / name)]
    if args.names:
        load_db(DEFAULT_CACHE)
        audit += ["--names", str(DEFAULT_CACHE)]
    if args.json:
        audit += ["--json", str(Path(args.json).resolve())]

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "app.elf"
        result = subprocess.run([str(relinker), "--registry", str(elf), str(out)], capture_output=True, text=True)
        registry = Path(tmp) / "app.registry.json"
        if not registry.exists():
            sys.exit("Le relinker a échoué :\n" + result.stdout + result.stderr)
        code = subprocess.run(audit + [str(registry)]).returncode

    if code == 1:
        print("\n=> Des imports sont « absent » : le jeu ne démarrera pas tant qu'ils ne sont pas ajoutés dans core/libs/prx.")
    elif code == 0:
        print("\n=> Rien ne bloque le chargement. Les « stub » lèveront une erreur si le jeu les appelle.")
    return code


if __name__ == "__main__":
    sys.exit(main())
