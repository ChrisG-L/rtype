---
tags:
  - guide
  - compilation
---

# Compilation

Guide de compilation du projet R-Type avec les scripts fournis.

## Build Rapide

```bash
# Configuration (première fois ou après changement CMakeLists)
./scripts/build.sh

# Compilation
./scripts/compile.sh
```

---

## Prérequis

Avant de compiler, assurez-vous d'avoir les outils suivants installés :

| Outil | Version Minimum | Vérification |
|-------|-----------------|--------------|
| CMake | 3.30+ | `cmake --version` |
| Ninja | - | `ninja --version` |
| Clang/GCC | Clang 15+ / GCC 13+ | `clang++ --version` |
| Git | - | `git --version` |

---

## Script de Configuration : `build.sh`

Le script `build.sh` configure l'environnement de build complet.

### Usage

```bash
./scripts/build.sh [OPTIONS]
```

### Options

| Option | Description | Défaut |
|--------|-------------|--------|
| `--platform=linux` | Cible Linux | ✓ |
| `--platform=windows` | Cross-compilation Windows (MinGW) | |
| `--platform=macos` | Cible macOS | |

### Ce que fait le script

1. Clone ou met à jour **vcpkg** dans `third_party/vcpkg/`
2. Bootstrap vcpkg
3. Configure CMake avec les bons paramètres selon la plateforme
4. Crée le dossier de build (`buildLinux/`, `buildWin/`, `buildMac/`)

### Exemples

```bash
# Build Linux (par défaut)
./scripts/build.sh

# Cross-compilation Windows
./scripts/build.sh --platform=windows

# macOS
./scripts/build.sh --platform=macos
```

---

## Script de Compilation : `compile.sh`

Le script `compile.sh` compile le projet et peut lancer les exécutables.

### Usage

```bash
./scripts/compile.sh [OPTIONS]
```

### Options

| Option | Description | Défaut |
|--------|-------------|--------|
| `--platform=PLATFORM` | linux, windows, macos | `linux` |
| `--server` | Lancer le serveur après compilation | ✓ |
| `--client` | Lancer le client après compilation | |
| `--both` | Lancer serveur + client | |
| `--no-launch` | Compiler sans lancer | |
| `--verbose`, `-v` | Afficher la sortie complète | |
| `--help`, `-h` | Afficher l'aide | |

### Exemples

```bash
# Compiler et lancer le serveur (par défaut)
./scripts/compile.sh

# Compiler et lancer le client
./scripts/compile.sh --client

# Compiler et lancer les deux
./scripts/compile.sh --both

# Compiler uniquement (pas de lancement)
./scripts/compile.sh --no-launch

# Voir toute la sortie de compilation
./scripts/compile.sh --verbose
```

---

## Workflow Complet

### Première Installation

```bash
# 1. Cloner le projet
git clone --recursive https://github.com/votre-repo/r-type.git
cd r-type

# 2. Configurer (installe vcpkg, configure CMake)
./scripts/build.sh

# 3. Compiler et lancer
./scripts/compile.sh
```

### Développement Quotidien

```bash
# Recompiler après modifications
./scripts/compile.sh

# Ou compiler sans lancer
./scripts/compile.sh --no-launch
```

### Après Modification de CMakeLists.txt

```bash
# Reconfigurer
./scripts/build.sh

# Puis recompiler
./scripts/compile.sh
```

---

## Plateformes Supportées

=== "Linux"

    ```bash
    # Configuration
    ./scripts/build.sh --platform=linux

    # Compilation
    ./scripts/compile.sh --platform=linux
    ```

    - Compilateur : Clang++
    - Dossier de build : `buildLinux/`
    - Artefacts : `./artifacts/server/linux/`, `./artifacts/client/linux/`

=== "Windows (Cross-compilation)"

    ```bash
    # Configuration
    ./scripts/build.sh --platform=windows

    # Compilation
    ./scripts/compile.sh --platform=windows --no-launch
    ```

    - Compilateur : MinGW (x86_64-w64-mingw32-g++)
    - Dossier de build : `buildWin/`
    - Artefacts : `./artifacts/server/windows/`, `./artifacts/client/windows/`

    !!! warning "Lancement automatique"
        Le lancement automatique n'est pas disponible pour Windows lors de la cross-compilation.

=== "macOS"

    ```bash
    # Configuration
    ./scripts/build.sh --platform=macos

    # Compilation
    ./scripts/compile.sh --platform=macos
    ```

    - Compilateur : Clang++
    - Dossier de build : `buildMac/`
    - Artefacts : `./artifacts/server/macos/`, `./artifacts/client/macos/`

---

## Structure des Artefacts

```
artifacts/
├── server/
│   ├── linux/
│   │   └── rtype_server
│   ├── windows/
│   │   └── rtype_server.exe
│   └── macos/
│       └── rtype_server
└── client/
    ├── linux/
    │   └── rtype_client
    ├── windows/
    │   └── rtype_client.exe
    └── macos/
        └── rtype_client
```

---

## Build Clean

Pour nettoyer complètement et reconfigurer :

```bash
# Supprimer les dossiers de build
rm -rf buildLinux/ buildWin/ buildMac/ artifacts/

# Reconfigurer
./scripts/build.sh

# Recompiler
./scripts/compile.sh
```

---

## Résolution des Problèmes

??? question "Erreur : Le dossier buildLinux n'existe pas"
    Vous devez d'abord exécuter le script de configuration :
    ```bash
    ./scripts/build.sh
    ```

??? question "Erreur : vcpkg n'a pas été compilé correctement"
    Le bootstrap de vcpkg a échoué. Vérifiez votre connexion internet et réessayez :
    ```bash
    rm -rf third_party/vcpkg
    ./scripts/build.sh
    ```

??? question "Erreur : C++23 features not available"
    Mettez à jour votre compilateur :
    ```bash
    # Ubuntu
    sudo apt install gcc-13 g++-13 clang
    ```
    Puis relancez la configuration :
    ```bash
    rm -rf buildLinux
    ./scripts/build.sh
    ```

??? question "Build très lent"
    La première compilation peut prendre du temps car vcpkg compile les dépendances.
    Les compilations suivantes seront beaucoup plus rapides.

??? question "Erreurs de linkage SDL2/SFML"
    Nettoyez et reconfigurez :
    ```bash
    rm -rf buildLinux third_party/vcpkg
    ./scripts/build.sh
    ./scripts/compile.sh
    ```

??? question "Les sanitizers ne fonctionnent pas"
    Les sanitizers sont configurés dans le script de build. Vérifiez que vous êtes en mode Debug (par défaut).
