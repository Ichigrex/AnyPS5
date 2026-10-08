#!/usr/bin/env python3
"""Diagnostic d'un jeu avant lancement : quelles fonctions système importées par le jeu
sont implémentées, ne sont que des bouchons (lèvent une erreur si appelées) ou manquent
(le jeu échoue au démarrage).

Usage : scripts/check-game.py <dossier_du_jeu | eboot.bin> [--build DIR] [--names] [--json] [--all]
"""
import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from nid_names import DEFAULT_CACHE, compute_nid, load_db  # noqa: E402
from progress import DEFINITION, PRX, body_end, stub_calls  # noqa: E402

NID_POSTFIX = "_nid_postfix"
PARTIAL = re.compile(r"\bif\s*\(")


def source_states():
    states = {}
    for source in PRX.rglob("*.cpp"):
        text = source.read_text(errors="ignore")
        calls = stub_calls(text)
        for match in DEFINITION.finditer(text):
            name = match.group(1)
            if name.endswith("_nid_no_patch"):
                continue
            body = text[match.end() - 1:body_end(text, match.end() - 1)]
            stub = next((call for call in calls if call in body), None)
            if stub is None:
                state = "ok"
            else:
                before = body[:body.index(stub)]
                state = "partial" if PARTIAL.search(before) else "stub"
            symbol = name[:-len(NID_POSTFIX)] if name.endswith(NID_POSTFIX) else name
            entry = {"name": symbol, "state": state, "file": str(source.relative_to(ROOT))}
            previous = states.get(compute_nid(symbol))
            if previous is None or previous["state"] != "ok":
                states[compute_nid(symbol)] = entry
    return states


def exported_nids(libs):
    nm = shutil.which("nm")
    if nm is None:
        sys.exit("nm introuvable (paquet binutils)")
    exports = {}
    for prx in sorted(libs.glob("*.prx")):
        output = subprocess.run([nm, "-D", "--defined-only", str(prx)], capture_output=True, text=True).stdout
        for line in output.splitlines():
            parts = line.split()
            if len(parts) >= 3:
                exports.setdefault(parts[2], prx.name)
    return exports


def game_imports(relinker, elf):
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "app.elf"
        result = subprocess.run([str(relinker), "--registry", str(elf), str(out)], capture_output=True, text=True)
        registry = Path(tmp) / "app.registry.json"
        if not registry.exists():
            sys.exit("Le relinker a échoué :\n" + result.stdout + result.stderr)
        entries = json.loads(registry.read_text())
    imports = {}
    for entry in entries:
        nid = entry["nid"].split("#")[0]
        imports.setdefault(nid, {"library": entry["library"], "calls": 0})
        imports[nid]["calls"] += len(entry["callSites"])
    return imports


def main():
    parser = argparse.ArgumentParser(description="Diagnostic des imports d'un jeu")
    parser.add_argument("game", help="dossier du jeu (contenant eboot.bin) ou chemin de l'ELF")
    parser.add_argument("--build", default=str(ROOT / "build" / "current"), help="dossier de build")
    parser.add_argument("--names", action="store_true", help="résout les NID inconnus via la base aerolib (téléchargée)")
    parser.add_argument("--json", action="store_true", help="sortie JSON")
    parser.add_argument("--all", action="store_true", help="liste aussi les fonctions implémentées")
    args = parser.parse_args()

    game = Path(args.game)
    elf = game / "eboot.bin" if game.is_dir() else game
    build = Path(args.build)
    relinker = build / "core" / "relinker" / "relinker"
    libs = build / "core" / "libs" / "libs"
    if not elf.is_file():
        sys.exit(f"Exécutable introuvable : {elf}")
    if not relinker.is_file() or not any(libs.glob("*.prx")):
        sys.exit("Build introuvable : lance d'abord scripts/build.sh")

    imports = game_imports(relinker, elf)
    exports = exported_nids(libs)
    states = source_states()
    db = load_db(DEFAULT_CACHE) if args.names else {}

    rows = []
    for nid, info in imports.items():
        source = states.get(nid)
        if nid not in exports:
            state = "missing"
        elif source is None:
            state = "ok"
        else:
            state = source["state"]
        name = source["name"] if source else db.get(nid, "")
        rows.append({"nid": nid, "name": name, "library": info["library"], "calls": info["calls"],
                     "state": state, "prx": exports.get(nid, ""), "file": source["file"] if source else ""})

    if args.json:
        print(json.dumps(rows, indent=2, ensure_ascii=False))
        return 1 if any(r["state"] == "missing" for r in rows) else 0

    counts = defaultdict(int)
    for row in rows:
        counts[row["state"]] += 1
    labels = {
        "missing": "ABSENTES : le jeu refusera de démarrer",
        "stub": "BOUCHONS : erreur dès que le jeu les appelle",
        "partial": "PARTIELLES : erreur dans certains cas seulement",
        "ok": "implémentées",
    }
    print(f"{elf}\n{len(rows)} fonctions système importées\n")
    for state in ("missing", "stub", "partial", "ok"):
        print(f"  {counts[state]:5d}  {labels[state]}")
    for state in ("missing", "stub", "partial") + (("ok",) if args.all else ()):
        selected = sorted((r for r in rows if r["state"] == state), key=lambda r: (r["library"], r["name"] or r["nid"]))
        if not selected:
            continue
        print(f"\n== {labels[state]} ==")
        by_library = defaultdict(list)
        for row in selected:
            by_library[row["library"] or row["prx"] or "?"].append(row)
        for library, items in sorted(by_library.items()):
            print(f"  [{library}]")
            for row in items:
                where = f"  ({row['file']})" if row["file"] and state != "missing" else ""
                print(f"    {row['nid']:<12} {row['name'] or '?':<48} appels:{row['calls']:<4}{where}")
    if counts["missing"] and not args.names:
        print("\nAstuce : --names affiche le nom des fonctions absentes (télécharge la base de NID).")
    return 1 if counts["missing"] else 0


if __name__ == "__main__":
    sys.exit(main())
