# Fork personnel d'AnyPS5

Fork privé de [boykopovar/AnyPS5](https://github.com/boykopovar/AnyPS5). Le développement se fait sur la branche `dev`.

## Changements par rapport à l'upstream

| Bibliothèque | Fonction            | Description                                                                                                                         |
|--------------|---------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| libSceRtc    | `sceRtcCompareTick` | Compare deux ticks : renvoie `-1` (plus tôt), `0` (égal) ou `1` (plus tard), et `0x80B50002` (`INVALID_POINTER`) si un pointeur est nul. |
| libkernel    | `sceKernelAioPollRequests` | État de plusieurs requêtes AIO sans attendre (version groupée de `sceKernelAioPollRequest`). |
| libkernel    | `sceKernelGetEventError` | Renvoie 0 pour un événement sans `EV_ERROR` (seul cas produit par la file d'événements d'AnyPS5) ; un événement avec `EV_ERROR` lève toujours une erreur, son code n'étant pas documenté. |
| libSceSystemService | langue système | Variable `ANYPS5_LANGUAGE` (`fr`, `fr-CA`, `en-GB`, `ja`, `de`… ou l'identifiant 0-30). Défaut : anglais US, comme l'upstream. |
| libSceSystemService | fuseau horaire | `TIME_ZONE` et `SUMMERTIME` reprennent le fuseau du PC (variable `TZ` sous Linux), comme libSceRtc. L'upstream renvoie toujours UTC sans heure d'été. |

Le comportement de `sceRtcCompareTick` reprend la convention des bibliothèques RTC de la PSP et de la PS4 (et de fpPS4). Il n'a pas été vérifié sur une vraie console.

## Compiler rapidement

Première fois sur une machine Ubuntu/Debian (installe les dépendances, gdb et ccache) :

```sh
scripts/build.sh dev --deps
```

Ensuite :

```sh
scripts/build.sh            # build "dev" : optimisé + symboles de debug (recommandé)
scripts/build.sh release    # build pour jouer
scripts/build.sh debug      # -O0, pas à pas fiable mais jeux lents
scripts/build.sh dev --test # compile puis lance tous les tests
scripts/build.sh dev --clean
```

Chaque mode a son dossier (`build/release`, `build/dev`, `build/debug`) ; `build/current` pointe sur le dernier compilé. ccache est utilisé s'il est installé : les recompilations suivantes sont beaucoup plus rapides.

Les mêmes modes existent en presets CMake (`CMakePresets.json`), utilisables directement (`cmake --preset dev`) ou depuis VS Code / CLion.

## Tester un jeu

Le jeu doit être un dump avec `eboot.bin` **déchiffré** (ELF) et ses modules dans `sce_module/` :

```sh
scripts/run-game.sh /chemin/vers/PPSA01234
```

Le script :
1. convertit `eboot.bin` avec le relinker (ajoute `--to-intel` automatiquement sur un processeur Intel) ;
2. prépare `games/<nom_du_jeu>/` : `app.elf`, `libs/` (lien vers les `.prx` compilés), `app0/` (liens vers les fichiers du jeu, sans copie) ;
3. affiche le diagnostic des imports (voir ci-dessous) ;
4. lance le jeu et enregistre la sortie dans `games/<nom_du_jeu>/last-run.log`.

La conversion n'est refaite que si `eboot.bin` ou le relinker ont changé (`--relink` pour forcer). Les arguments après `--` sont passés au jeu. `--lang fr` lance le jeu en français (variable `ANYPS5_LANGUAGE`). Variables utiles : `ANYPS5_GPU=<nom>` pour choisir le GPU, `ANYPS5_SYSTEM_FONTS=<dossier>` pour les polices (voir [USAGE.md](user/USAGE.md)).

## Diagnostiquer un jeu avant de le lancer

```sh
scripts/check-game.py /chemin/vers/PPSA01234             # résumé + liste des problèmes
scripts/check-game.py /chemin/vers/PPSA01234 --names     # nom des fonctions absentes (télécharge la base de NID)
scripts/check-game.py /chemin/vers/PPSA01234 --json r.json
```

Raccourci vers `tools/import_audit.py` du projet d'origine (voir [USAGE.md](user/USAGE.md#import-audit)) : il convertit `eboot.bin` avec `relinker --registry` dans un dossier temporaire, puis classe chaque fonction système importée :

| Classe        | Conséquence                                                             |
|---------------|--------------------------------------------------------------------------|
| `absent`      | le jeu refuse de démarrer : il faut l'ajouter dans `core/libs/prx`        |
| `stub`        | erreur (`std::runtime_error`) dès que le jeu l'appelle                    |
| `module`      | fournie par un module du jeu (`sce_module/`), non vérifiée                 |
| `implemented` | OK                                                                       |

La liste des `stub` et `absent` est la liste de travail pour faire tourner le jeu. `run-game.sh` lance ce diagnostic automatiquement (résultat complet dans `games/<nom_du_jeu>/check.txt`, `--no-check` pour le désactiver). Limite : seuls les imports de `eboot.bin` sont analysés, pas ceux des modules du jeu.

## Déboguer

En ligne de commande :

```sh
scripts/run-game.sh /chemin/vers/PPSA01234 --gdb
scripts/run-game.sh /chemin/vers/PPSA01234 --gdb --catch-throw
```

`--catch-throw` arrête gdb à l'endroit exact où une `std::runtime_error` est levée (fonction non implémentée, état non supporté) : `bt` donne alors la pile d'appels complète. Sans cette option, gdb s'arrête sur les plantages (SIGSEGV, SIGABRT). Le build `dev` suffit le plus souvent ; prendre `debug` pour suivre les variables pas à pas.

Dans VS Code (extension C/C++ de Microsoft) :
- `Ctrl+Shift+B` : build dev ;
- onglet *Run and Debug* : « déboguer un jeu (gdb) », « déboguer un jeu + arrêt sur exception » ou « déboguer un test ». Le chemin du jeu est demandé au lancement ; il est préparé dans `games/debug/`.

Test rapide de libSceRtc seul, sans configurer tout le projet :

```sh
mkdir -p /tmp/rtc
g++ -std=c++20 -O2 -fPIC -shared -Icore/libs core/libs/prx/libSceRtc/Export.cpp -o /tmp/rtc/libSceRtc.so
g++ -std=c++20 -O2 -Icore/libs core/libs/tests/GuestRtc.cpp -L/tmp/rtc -lSceRtc -Wl,-rpath,/tmp/rtc -o /tmp/rtc/guest_rtc
/tmp/rtc/guest_rtc && echo OK
```

## Rester à jour avec l'upstream

Automatique : la GitHub Action `Sync upstream` (`.github/workflows/sync-upstream.yml`) fusionne chaque jour `boykopovar/AnyPS5` `main` dans `dev`. En cas de conflit elle échoue sans rien pousser : il faut alors fusionner en local. Pour qu'elle tourne, deux réglages sont nécessaires sur GitHub :
1. activer les Actions du fork (onglet *Actions*, bouton d'activation) ;
2. GitHub ne lance les tâches planifiées que depuis la branche par défaut : passer `dev` en branche par défaut (*Settings > General > Default branch*) ou copier ce fichier sur `main`.

En local :

```sh
scripts/sync-upstream.sh          # fusionne upstream/main dans dev
scripts/sync-upstream.sh --push   # et pousse sur le fork
scripts/build.sh                  # puis recompiler
```

## Problèmes connus

- `guest_sce_net` échoue sur une machine sans IPv6 (pas de `/proc/net/if_inet6`) : le test crée un socket IPv6. Ce n'est pas un bug du code. Les 461 autres tests passent (build `dev`, Ubuntu 24.04, GCC 13).
- Le relinker ne crée pas le dossier de sortie : il doit exister avant la conversion (`run-game.sh` s'en charge).

## Pistes pour la suite

Bibliothèques avec le plus de fonctions encore non implémentées (`NotImplemented_nid_no_patch`) :

- libSceFont (81)
- libSceAgc (34)
- libSceUsbd (25)
- libSceFontFt (19)
- libSceHmd2 (16)
- libkernel (13) : par exemple `sceKernelGetEventError`, `scePthreadCancel`, `sceKernelFcntl`
