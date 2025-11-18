# Installation

Ce guide vous explique comment installer et configurer l'environnement de développement pour R-Type.

!!! success "Processus d'installation en 7 étapes"
    1. **Installer les dépendances système** (GCC, CMake, Git, etc.)
    2. **Vérifier GCC** (version 11+)
    3. **Installer Docker et Docker Compose**
    4. **Cloner le repository**
    5. **Lancer l'infrastructure CI/CD** (Jenkins + Documentation) → `./scripts/launch_ci_cd.sh`
    6. **Installer vcpkg et compiler** → `./scripts/build.sh`
    7. **Vérifier l'installation** → `./scripts/compile.sh` et tests

    **Temps estimé :** 15-40 minutes (selon votre machine et connexion)

## Prérequis

### Configuration minimale requise

- **OS** : Linux (Ubuntu 22.04 LTS recommandé), ou Windows
- **Processeur** : Architecture x64 (x86_64)

### Outils nécessaires

| Outil | Version minimale | Commande de vérification |
| ----- | ---------------- | ------------------------ |
| GCC   | 11+              | `gcc --version`          |
| CMake | 3.30+            | `cmake --version`        |
| Git   | 2.0+             | `git --version`          |
| Ninja | 1.10+            | `ninja --version`        |

## Installation sur Linux (Ubuntu/Debian)

### Étape 1 : Installer les dépendances système

```bash
# Mettre à jour les paquets
sudo apt-get update && sudo apt-get upgrade -y

# Installer les outils de build
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libssl-dev \
    curl \
    wget \
    zip \
    unzip \
    tar
```

### Étape 2 : Vérifier GCC

```bash
# Vérifier la version de GCC
gcc --version

# Si GCC < 11, installer une version plus récente
sudo apt-get install -y gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

### Étape 3 : Installer Docker et Docker Compose

```bash
# Installer Docker
sudo apt-get install -y docker.io docker-compose

# Ajouter votre utilisateur au groupe docker (pour éviter sudo)
sudo usermod -aG docker $USER

# Déconnectez-vous et reconnectez-vous pour que les changements prennent effet
# Ou exécutez: newgrp docker

# Vérifier l'installation
docker --version
docker-compose --version
```

### Étape 4 : Cloner le repository

```bash
# Cloner le projet
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# Vérifier que vous êtes sur la branche main
git checkout main
git pull origin main
```

### Étape 5 : Lancer l'infrastructure CI/CD

Avant de compiler, lancez l'infrastructure Docker pour avoir accès à Jenkins (CI/CD) et à la documentation locale :

```bash
# Lancer Jenkins et la documentation
./scripts/launch_ci_cd.sh
```

Ce script va :

1. Lancer la documentation MkDocs sur **http://localhost:8000**
2. Lancer Jenkins sur **http://localhost:8081**

!!! tip "Documentation locale"
    Une fois lancée, vous pouvez accéder à la documentation complète du projet à http://localhost:8000. C'est très utile pour suivre les guides pendant le développement !

!!! info "Jenkins CI/CD"
    Jenkins est disponible à http://localhost:8081 et vous permet de lancer les pipelines de build et de tests automatiquement.

### Étape 6 : Installer vcpkg et les dépendances

Le projet utilise vcpkg pour gérer les dépendances. Le script d'installation s'en charge automatiquement :

```bash
# Lancer l'installation de vcpkg et configuration CMake
./scripts/build.sh
```

Ce script va :

1. Cloner vcpkg dans `third_party/vcpkg`
2. Compiler vcpkg (bootstrap)
3. Installer les dépendances définies dans `vcpkg.json` :
   - Boost.ASIO (communication réseau)
   - Google Test (tests unitaires)
   - MongoDB C++ Driver (base de données)
4. Configurer CMake avec vcpkg

!!! info "Durée d'installation"
    La première installation peut prendre 10-30 minutes selon votre connexion et votre machine, car vcpkg compile les dépendances depuis les sources.

### Étape 7 : Vérifier l'installation

```bash
# Vérifier que vcpkg est installé
./third_party/vcpkg/vcpkg version

# Vérifier que CMake est configuré
ls build/

# Devrait afficher des fichiers comme CMakeCache.txt, build.ninja, etc.
```

## Alternative : Build complet avec Docker

Si vous préférez utiliser Docker pour isoler complètement l'environnement de build :

### Prérequis Docker

Docker doit déjà être installé (voir Étape 3 ci-dessus).

### Build avec Docker

```bash
# Lancer l'infrastructure CI/CD complète (Jenkins + Builder permanent)
cd ci_cd/docker
docker-compose up
```

!!! note "Build natif vs CI/CD"
    Pour le développement local quotidien, le build natif (via `./scripts/build.sh`) est recommandé car il est plus rapide. L'infrastructure Docker CI/CD (Jenkins + Builder permanent) est principalement utilisée pour l'intégration continue et les builds automatisés.

L'image Docker contient tous les outils nécessaires :

- Ubuntu 22.04
- GCC/G++ 11
- CMake 3.22+
- Ninja
- Git
- vcpkg (installé automatiquement)

## Installation sur Windows (WSL2)

Windows nécessite WSL2 (Windows Subsystem for Linux) pour un environnement de développement optimal.

### Installer WSL2

```powershell
# Dans PowerShell en tant qu'administrateur
wsl --install -d Ubuntu-22.04

# Redémarrer Windows
# Configurer votre nom d'utilisateur et mot de passe Ubuntu
```

### Suivre les instructions Linux

Une fois WSL2 configuré, suivez les instructions d'installation pour Linux ci-dessus.

```bash
# Dans le terminal WSL2
cd ~
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# Installer Docker dans WSL2
sudo apt-get update
sudo apt-get install -y docker.io docker-compose
sudo service docker start

# Lancer l'infrastructure CI/CD
./scripts/launch_ci_cd.sh

# Build et compilation
./scripts/build.sh
./scripts/compile.sh
```

!!! warning "Docker dans WSL2"
    Assurez-vous que Docker Desktop pour Windows est installé ET que l'intégration WSL2 est activée dans les paramètres Docker Desktop.

## Dépendances du projet

Le fichier `vcpkg.json` définit les dépendances :

```json
{
  "dependencies": [
    "boost-asio", // Communication réseau asynchrone
    "gtest", // Framework de tests
    "mongo-cxx-driver" // Client MongoDB
  ]
}
```

Ces dépendances sont installées automatiquement par `./scripts/build.sh`.

## Variables d'environnement

Le projet utilise les variables d'environnement suivantes :

| Variable               | Description       | Valeur par défaut   |
| ---------------------- | ----------------- | ------------------- |
| `VCPKG_ROOT`           | Chemin vers vcpkg | `third_party/vcpkg` |
| `CMAKE_BUILD_TYPE`     | Type de build     | `Debug`             |
| `VCPKG_TARGET_TRIPLET` | Triplet cible     | `x64-linux`         |

Ces variables sont configurées automatiquement par les scripts.

## Résolution des problèmes

### Erreur : "CMake version too old"

```bash
# Installer CMake depuis le site officiel
wget https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-linux-x86_64.sh
chmod +x cmake-3.28.1-linux-x86_64.sh
sudo ./cmake-3.28.1-linux-x86_64.sh --prefix=/usr/local --skip-license
```

### Erreur : "GCC version < 11"

```bash
sudo apt-get install -y gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
```

### Erreur : "vcpkg: command not found"

```bash
# Relancer le script d'installation
./scripts/build.sh
```

### Erreur de compilation Boost ou MongoDB

```bash
# Nettoyer et réinstaller les dépendances
rm -rf third_party/vcpkg/buildtrees
rm -rf third_party/vcpkg/packages
./third_party/vcpkg/vcpkg install --clean-after-build
```

## Vérification de l'installation

Pour vérifier que tout est correctement installé :

```bash
# Compiler le projet
./scripts/compile.sh

# Vérifier les binaires
ls -lh artifacts/server/linux/

# Devrait afficher :
# - rtype_server       (serveur principal)
# - server_tests       (tests unitaires)

# Lancer les tests
./artifacts/server/linux/server_tests

# Devrait afficher :
# [==========] Running X tests...
# [  PASSED  ] X tests.
```

## Prochaines étapes

Une fois l'installation terminée :

1. Consultez le [Guide de démarrage rapide](quickstart.md) pour lancer le projet
2. Lisez le [Guide de compilation](building.md) pour plus de détails sur le système de build
3. Explorez l'[Architecture](../guides/architecture.md) du projet

## Mise à jour du projet

Pour mettre à jour le projet et ses dépendances :

```bash
# Mettre à jour les sources
git pull origin main

# Mettre à jour vcpkg
cd third_party/vcpkg
git pull
./bootstrap-vcpkg.sh
cd ../..

# Reconfigurer le projet
./scripts/build.sh

# Recompiler
./scripts/compile.sh
```
