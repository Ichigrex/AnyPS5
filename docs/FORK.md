# Fork personnel d'AnyPS5

Fork privé de [boykopovar/AnyPS5](https://github.com/boykopovar/AnyPS5). Le développement se fait sur la branche `dev`.

## Changements par rapport à l'upstream

| Bibliothèque | Fonction            | Description                                                                                                                         |
|--------------|---------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| libSceRtc    | `sceRtcCompareTick` | Compare deux ticks : renvoie `-1` (plus tôt), `0` (égal) ou `1` (plus tard), et `0x80B50002` (`INVALID_POINTER`) si un pointeur est nul. |

Le comportement de `sceRtcCompareTick` reprend la convention des bibliothèques RTC de la PSP et de la PS4 (et de fpPS4). Il n'a pas été vérifié sur une vraie console.

## Compiler et tester

Build complet (voir [BUILD.md](dev/BUILD.md)) :

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build
cmake --build build --target libs
ctest --test-dir build --output-on-failure
```

Test rapide de libSceRtc seul, sans configurer tout le projet :

```sh
mkdir -p /tmp/rtc
g++ -std=c++20 -O2 -fPIC -shared -Icore/libs core/libs/prx/libSceRtc/Export.cpp -o /tmp/rtc/libSceRtc.so
g++ -std=c++20 -O2 -Icore/libs core/libs/tests/GuestRtc.cpp -L/tmp/rtc -lSceRtc -Wl,-rpath,/tmp/rtc -o /tmp/rtc/guest_rtc
/tmp/rtc/guest_rtc && echo OK
```

## Rester à jour avec l'upstream

```sh
git remote add upstream https://github.com/boykopovar/AnyPS5.git
git fetch upstream main
git checkout dev
git merge upstream/main
git push origin dev
```

## Pistes pour la suite

Bibliothèques avec le plus de fonctions encore non implémentées (`NotImplemented_nid_no_patch`) :

- libSceFont (81)
- libSceAgc (34)
- libSceUsbd (25)
- libSceFontFt (19)
- libSceHmd2 (16)
- libkernel (13) : par exemple `sceKernelGetEventError`, `scePthreadCancel`, `sceKernelFcntl`
