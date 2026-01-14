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
    ```

=== "Windows"

    1. **Visual Studio 2022** avec le workload "Développement Desktop C++"
    2. **Git for Windows** : [git-scm.com](https://git-scm.com/)
    3. **CMake 3.20+** : [cmake.org](https://cmake.org/download/)

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

!!! tip "Intégration CMake"
    vcpkg s'intègre automatiquement avec CMake via le toolchain file :
    ```bash
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
    ```

---

## Cloner le Projet

```bash
# Clone avec submodules
git clone --recursive https://github.com/votre-repo/r-type.git
cd r-type

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
    "boost-serialization",
    "spdlog",
    "nlohmann-json",
    "sdl2",
    "sdl2-image",
    "sdl2-ttf",
    "sfml"
  ]
}
```

L'installation est automatique lors du premier build CMake.

---

## Vérification

```bash
# Vérifier les versions
cmake --version   # >= 3.20
g++ --version     # >= 11 (ou clang++ >= 15)
vcpkg version
```

!!! success "Prêt pour la compilation"
    Passez à la section [Compilation](building.md) pour builder le projet.

---

## Résolution des Problèmes

??? question "vcpkg ne trouve pas les dépendances"
    Assurez-vous que `VCPKG_ROOT` est correctement défini :
    ```bash
    echo $VCPKG_ROOT
    # Doit afficher le chemin vers vcpkg
    ```

??? question "Erreurs de compilation avec GCC"
    R-Type requiert C++23. Vérifiez votre version de GCC :
    ```bash
    g++ --version
    # Doit être >= 11
    ```

    Pour installer GCC 13 sur Ubuntu :
    ```bash
    sudo apt install gcc-13 g++-13
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
    ```

??? question "Problèmes avec les libs graphiques sur Linux"
    Installez les dépendances de développement :
    ```bash
    sudo apt install libgl1-mesa-dev libglu1-mesa-dev
    sudo apt install libopenal-dev libvorbis-dev libflac-dev
    ```
