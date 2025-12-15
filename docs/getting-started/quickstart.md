# Démarrage Rapide

Ce guide vous permet de lancer rapidement le projet R-Type (serveur + client) en quelques commandes.

## Prérequis

Avant de commencer, assurez-vous d'avoir :

- **Système :** Linux (Ubuntu 22.04 recommandé) ou WSL2
- **Outils :** Git, CMake 3.30+, GCC 11+, Ninja
- **Dépendances :** SDL2, SDL2_image, SDL2_mixer, SDL2_ttf, SFML, Boost.ASIO
- **Espace disque :** Au moins 5 GB libre

!!! tip "Installation des prérequis"
    Les dépendances sont gérées automatiquement par vcpkg. Consultez le [Guide d'installation complet](installation.md) si besoin.

## Lancement en 4 commandes

### Méthode recommandée : Native + Docker

```bash
# 1. Cloner le projet
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# 2. Lancer l'infrastructure CI/CD (Jenkins + Documentation)
./scripts/launch_ci_cd.sh

# 3. Configuration et installation des dépendances (prend 10-30 min la première fois)
./scripts/build.sh

# 4. Compilation
./scripts/compile.sh
```

!!! success "C'est prêt!"
    - **Binaires** disponibles dans `artifacts/server/linux/`
    - **Documentation** accessible à http://localhost:8000
    - **Jenkins** accessible à http://localhost:8081

### Méthode alternative : Build complet avec Docker

```bash
# 1. Cloner le projet
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# 2. Build natif (recommandé pour développement)
./scripts/build.sh
./scripts/compile.sh
```

!!! note "Alternative CI/CD"
    Pour utiliser l'infrastructure CI/CD complète (Jenkins + Builder permanent), consultez le [Guide CI/CD](../development/ci-cd.md).

## Lancer le jeu

### 1. Lancer le serveur

```bash
# Depuis la racine du projet
./artifacts/server/linux/rtype_server
```

Le serveur démarre sur le port UDP **4124** et broadcast l'état de jeu à 20Hz.

### 2. Lancer le client

```bash
# Dans un autre terminal
./artifacts/client/linux/rtype_client
```

Le client se connecte automatiquement à `127.0.0.1:4124`.

!!! success "Gameplay"
    - **Flèches** : Déplacer le vaisseau
    - **Espace** : Tirer des missiles
    - **Échap** : Quitter

!!! info "Multijoueur"
    Jusqu'à 4 joueurs peuvent se connecter simultanément. Lancez plusieurs clients pour tester le multijoueur.

## Lancer les tests

Pour vérifier que tout fonctionne correctement :

```bash
# Lancer les tests unitaires
./artifacts/server/linux/server_tests
```

Sortie attendue :

```
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from CalculTests
[ RUN      ] CalculTests.AdditionSimple
[       OK ] CalculTests.AdditionSimple (0 ms)
[----------] 1 test from CalculTests (0 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (0 ms total)
[  PASSED  ] 1 test.
```

## Accéder à la documentation

La documentation est automatiquement lancée avec `./scripts/launch_ci_cd.sh` !

```bash
# Si ce n'est pas déjà fait
./scripts/launch_ci_cd.sh

# Ouvrez votre navigateur
# http://localhost:8000
```

!!! tip "Documentation en direct"
    La documentation est lancée avec MkDocs en mode live-reload. Toute modification des fichiers `.md` sera visible instantanément !

### Alternative : Documentation seule

Si vous voulez uniquement la documentation sans Jenkins :

```bash
cd ci_cd/docker
docker-compose -f docker-compose.docs.yml up
```

## Structure des artifacts

Après compilation, voici ce que vous trouverez :

```
artifacts/
├── server/
│   └── linux/
│       ├── rtype_server      # Binaire du serveur (UDP 4124)
│       └── server_tests      # Suite de tests serveur
└── client/
    └── linux/
        ├── rtype_client      # Binaire du client
        ├── librtype_sdl2.so  # Plugin SDL2 (défaut)
        └── librtype_sfml.so  # Plugin SFML (alternatif)
```

## Workflow de développement

### 1. Faire des modifications

Éditez les fichiers source :

```bash
# Éditer le serveur
nano src/server/main.cpp

# Éditer le client
nano src/client/src/scenes/GameScene.cpp
```

### 2. Recompiler

```bash
# Compilation complète (serveur + client)
./scripts/compile.sh

# Ou compilation spécifique
cmake --build build --target rtype_server  # Serveur uniquement
cmake --build build --target rtype_client  # Client uniquement
```

### 3. Tester

```bash
# Lancer les tests
./artifacts/server/linux/server_tests

# Lancer le serveur (terminal 1)
./artifacts/server/linux/rtype_server

# Lancer le client (terminal 2)
./artifacts/client/linux/rtype_client
```

### 4. Nettoyer si nécessaire

```bash
# Nettoyer les fichiers de build (Linux)
rm -rf buildLinux/

# Ou nettoyer tous les builds
rm -rf build*/

# Reconfigurer complètement
./scripts/build.sh                    # Linux
./scripts/build.sh --platform=windows # Windows
./scripts/compile.sh
```

## Commandes utiles

| Commande                                | Description                                        |
| --------------------------------------- | -------------------------------------------------- |
| `./scripts/build.sh`                    | Configure CMake et installe les dépendances        |
| `./scripts/compile.sh`                  | Compile le projet (serveur + client)               |
| `./artifacts/server/linux/rtype_server` | Lance le serveur (UDP 4124)                        |
| `./artifacts/client/linux/rtype_client` | Lance le client                                    |
| `./artifacts/server/linux/server_tests` | Lance les tests                                    |
| `./scripts/launch_ci_cd.sh`             | Lance Jenkins + Documentation (http://localhost)   |
| `./third_party/vcpkg/vcpkg list`        | Liste les dépendances installées                   |
| `cmake --build build --target clean`    | Nettoie les objets compilés                        |

## Modes de compilation

### Mode Debug (par défaut)

Le mode Debug est configuré par défaut et inclut :

- Symboles de debug complets (`-g3`)
- Pas d'optimisation (`-O0`)
- AddressSanitizer (détection de fuites mémoire)
- UndefinedBehaviorSanitizer
- Tous les warnings activés (`-Wall -Wextra -Wpedantic`)

```bash
# Déjà configuré par défaut
./scripts/build.sh
./scripts/compile.sh
```

### Mode Release

Pour compiler en mode Release avec optimisations :

```bash
# Reconfigurer en Release
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -G "Ninja"

# Compiler
cmake --build build --config Release
```

## Vérification rapide de l'environnement

Avant de commencer, vérifiez votre environnement :

```bash
# Vérifier les versions
gcc --version        # Doit être >= 11
cmake --version      # Doit être >= 3.30
ninja --version      # Doit être >= 1.10
git --version        # Doit être >= 2.0

# Vérifier l'espace disque
df -h .             # Au moins 5 GB disponibles
```

## Résolution rapide des problèmes

### Erreur de compilation

```bash
# Nettoyer et recommencer
rm -rf build*/ artifacts/
./scripts/build.sh
./scripts/compile.sh
```

### Erreur vcpkg

```bash
# Réinstaller vcpkg
rm -rf third_party/vcpkg
./scripts/build.sh
```

### Tests échouent

```bash
# Recompiler en mode Debug
rm -rf buildLinux/
./scripts/build.sh
./scripts/compile.sh
./artifacts/server/linux/server_tests --gtest_verbose
```

## Utiliser Jenkins (CI/CD)

Jenkins est automatiquement lancé avec `./scripts/launch_ci_cd.sh` !

```bash
# Si ce n'est pas déjà fait
./scripts/launch_ci_cd.sh

# Accéder à Jenkins
# http://localhost:8081
```

Le pipeline Jenkins exécute automatiquement à chaque push :

1. **Checkout** du code
2. **Installation** des dépendances (vcpkg)
3. **Compilation** du projet
4. **Tests** unitaires
5. **Archivage** des artifacts

!!! info "Configuration Jenkins"
    Le pipeline est défini dans `ci_cd/Jenkinsfile`. Vous pouvez le personnaliser selon vos besoins.

## Prochaines étapes

Maintenant que vous avez lancé le projet :

1. Explorez le [Guide de compilation détaillé](building.md)
2. Consultez l'[Architecture du projet](../guides/architecture.md)
3. Lisez le [Guide de contribution](../development/contributing.md)

## Commandes Docker utiles

### Documentation locale

```bash
# Lancer la documentation en mode développement
cd ci_cd/docker
docker-compose -f docker-compose.docs.yml up

# Accéder à http://localhost:8000
```

### Infrastructure CI/CD

```bash
# Lancer Jenkins + Builder permanent
cd ci_cd/docker
docker-compose up -d

# Voir les logs
docker-compose logs -f

# Arrêter l'infrastructure
docker-compose down
```

## Ressources additionnelles

- [Documentation complète de l'installation](installation.md)
- [Guide de compilation avancé](building.md)
- [FAQ](../reference/faq.md)
