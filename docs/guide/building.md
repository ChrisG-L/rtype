---
tags:
  - guide
  - compilation
---

# Compilation

Options avancées de build et configurations CMake.

## Build Rapide

```bash
# Configuration + Compilation
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Options CMake

### Options Principales

| Option | Description | Défaut |
|--------|-------------|--------|
| `CMAKE_BUILD_TYPE` | Type de build | `Release` |
| `BUILD_TESTS` | Compiler les tests | `ON` |
| `BUILD_DOCS` | Générer la documentation | `OFF` |
| `ENABLE_SANITIZERS` | Activer les sanitizers | `OFF` |

### Options Backend Graphique

| Option | Description | Défaut |
|--------|-------------|--------|
| `USE_SDL2` | Activer le backend SDL2 | `ON` |
| `USE_SFML` | Activer le backend SFML | `ON` |
| `DEFAULT_BACKEND` | Backend par défaut | `SDL2` |

### Exemple Complet

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DBUILD_TESTS=ON \
    -DUSE_SDL2=ON \
    -DUSE_SFML=ON \
    -DDEFAULT_BACKEND=SDL2
```

---

## Types de Build

=== "Release"

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    ```

    - Optimisations maximales (`-O3`)
    - Pas de symboles de debug
    - **Recommandé pour jouer**

=== "Debug"

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    ```

    - Symboles de debug complets
    - Pas d'optimisation (`-O0`)
    - **Recommandé pour développer**

=== "RelWithDebInfo"

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
    ```

    - Optimisations + symboles de debug
    - **Recommandé pour profiler**

---

## Build avec Ninja

Ninja est plus rapide que Make pour les gros projets :

```bash
# Installation
sudo apt install ninja-build

# Utilisation
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Cibles de Build

```bash
# Tout compiler
cmake --build build

# Seulement le serveur
cmake --build build --target r-type_server

# Seulement le client
cmake --build build --target r-type_client

# Les tests
cmake --build build --target tests

# Nettoyer
cmake --build build --target clean
```

---

## Tests

```bash
# Compiler les tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build --target tests

# Exécuter les tests
cd build && ctest --output-on-failure

# Tests avec couverture
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build --target coverage
```

---

## Sanitizers

Les sanitizers détectent les bugs mémoire et les data races :

=== "AddressSanitizer"

    ```bash
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_SANITIZERS=ON \
        -DSANITIZER_TYPE=address
    ```

    Détecte :
    - Buffer overflow
    - Use-after-free
    - Memory leaks

=== "ThreadSanitizer"

    ```bash
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_SANITIZERS=ON \
        -DSANITIZER_TYPE=thread
    ```

    Détecte :
    - Data races
    - Deadlocks

=== "UndefinedBehaviorSanitizer"

    ```bash
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_SANITIZERS=ON \
        -DSANITIZER_TYPE=undefined
    ```

    Détecte :
    - Integer overflow
    - Null pointer dereference
    - Type violations

---

## Cross-Compilation

### Linux vers Windows (MinGW)

```bash
cmake -B build-win \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-win
```

---

## Structure des Artefacts

```
build/
├── r-type_server          # Exécutable serveur
├── r-type_client          # Exécutable client
├── lib/
│   ├── libgraphics_sdl2.so
│   └── libgraphics_sfml.so
├── tests/
│   └── r-type_tests
└── _deps/                  # Dépendances vcpkg
```

---

## Résolution des Problèmes

??? question "Erreur : vcpkg toolchain not found"
    Vérifiez que `VCPKG_ROOT` est défini :
    ```bash
    export VCPKG_ROOT="$HOME/vcpkg"
    ```

??? question "Erreur : C++23 features not available"
    Mettez à jour votre compilateur :
    ```bash
    # Ubuntu
    sudo apt install gcc-13 g++-13

    # Puis reconfigurez
    cmake -B build -DCMAKE_CXX_COMPILER=g++-13
    ```

??? question "Build très lent"
    Utilisez Ninja et la compilation parallèle :
    ```bash
    cmake -B build -G Ninja
    cmake --build build -j$(nproc)
    ```

??? question "Erreurs de linkage SDL2/SFML"
    Réinstallez les dépendances vcpkg :
    ```bash
    rm -rf build
    vcpkg remove sdl2 sfml --recurse
    vcpkg install sdl2 sfml
    cmake -B build ...
    ```
