---
tags:
  - guide
  - installation
---

# Installation

Guide complet pour installer R-Type et ses dépendances sur votre système.

## Prérequis Système

=== "Linux (Ubuntu/Debian)"

    ```bash
    # Mise à jour du système
    sudo apt update && sudo apt upgrade -y

    # Installation des outils de base
    sudo apt install -y \
        build-essential \
        git \
        cmake \
        ninja-build \
        pkg-config \
        curl \
        zip \
        unzip \
        tar

    # Installation des dépendances graphiques
    sudo apt install -y \
        libx11-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libgl1-mesa-dev \
        libudev-dev \
        libfreetype6-dev

    # Dépendances additionnelles (TLS, compilation de libs)
    sudo apt install -y \
        libssl-dev \
        autoconf \
        autoconf-archive \
        automake \
        libtool
    ```

=== "Windows"

    1. **Visual Studio 2022** avec le workload "Développement Desktop C++"
    2. **Git for Windows** : [git-scm.com](https://git-scm.com/)
    3. **CMake 3.30+** : [cmake.org](https://cmake.org/download/)

=== "macOS"

    ```bash
    # Installation de Homebrew si nécessaire
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Installation des outils
    brew install cmake ninja git
    ```

---

## Installation de vcpkg

vcpkg est notre gestionnaire de dépendances C++.

```bash
# Cloner vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg

# Bootstrap
cd ~/vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# ou bootstrap-vcpkg.bat sous Windows

# Configurer la variable d'environnement
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
echo 'export PATH="$VCPKG_ROOT:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

!!! tip "Intégration automatique"
    Le script `./scripts/build.sh` gère automatiquement l'intégration de vcpkg avec CMake.

---

## Cloner le Projet

```bash
# Clone avec submodules
git clone --recursive https://github.com/Pluenet-Killian/rtype.git
cd rtype

# Si vous avez déjà cloné sans --recursive
git submodule update --init --recursive
```

---

## Installation des Dépendances

Le projet utilise vcpkg en mode manifest. Les dépendances sont définies dans `vcpkg.json` :

```json
{
  "dependencies": [
    "boost-asio",
    "gtest",
    "mongo-cxx-driver",
    "openssl",
    "protobuf",
    "sdl2",
    "sdl2-image",
    "sdl2-ttf",
    "sdl2-mixer",
    "sfml",
    "spdlog",
    "opus",
    "portaudio"
  ]
}
```

| Dépendance | Usage |
|------------|-------|
| `boost-asio` | Networking asynchrone (serveur) |
| `gtest` | Tests unitaires |
| `mongo-cxx-driver` | Base de données (authentification, rooms) |
| `openssl` | Chiffrement (authentification) |
| `protobuf` | Sérialisation (non utilisé actuellement) |
| `sdl2`, `sdl2-image`, `sdl2-ttf`, `sdl2-mixer` | Backend graphique SDL2 |
| `sfml` | Backend graphique SFML (par défaut) |
| `spdlog` | Logging |
| `opus`, `portaudio` | Voice chat (codec audio + I/O) |

L'installation est automatique lors du premier build CMake.

---

## Vérification

```bash
# Vérifier les versions
cmake --version      # >= 3.30
clang++ --version    # >= 15
vcpkg version
```

!!! success "Prêt pour la compilation"
    Vous pouvez maintenant compiler le projet :
    ```bash
    ./scripts/build.sh    # Configuration
    ./scripts/compile.sh  # Compilation
    ```
    Pour plus de détails, consultez la section [Compilation](compilation.md).

---

## Résolution des Problèmes

??? question "CMake version insuffisante (< 3.30)"
    Les dépôts Ubuntu/Debian ont souvent une version de CMake trop ancienne.
    Installez CMake depuis le dépôt officiel Kitware :

    ```bash
    # Ajouter la clé GPG Kitware
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
        gpg --dearmor - | \
        sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

    # Ajouter le dépôt (adapter 'noble' selon votre version Ubuntu)
    # noble = 24.04, jammy = 22.04, focal = 20.04
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | \
        sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null

    # Installer CMake
    sudo apt update
    sudo apt install kitware-archive-keyring  # Remplace la clé temporaire
    sudo apt install cmake

    # Vérifier
    cmake --version  # Doit afficher >= 3.30
    ```

??? question "vcpkg ne trouve pas les dépendances"
    Assurez-vous que `VCPKG_ROOT` est correctement défini :
    ```bash
    echo $VCPKG_ROOT
    # Doit afficher le chemin vers vcpkg
    ```

??? question "Erreurs de compilation avec Clang"
    R-Type requiert C++23. Vérifiez votre version de Clang :
    ```bash
    clang++ --version
    # Doit être >= 15
    ```

    Pour installer Clang sur Ubuntu :
    ```bash
    sudo apt install clang
    ```

??? question "Problèmes avec les libs graphiques sur Linux"
    Installez les dépendances de développement :
    ```bash
    sudo apt install libgl1-mesa-dev libglu1-mesa-dev
    sudo apt install libopenal-dev libvorbis-dev libflac-dev
    ```
