# Démarrage Rapide

Ce guide vous permet de lancer rapidement le projet R-Type en quelques commandes.

## Prérequis

Avant de commencer, assurez-vous d'avoir :

- Un système Linux (Ubuntu 22.04 recommandé) ou WSL2
- Git installé
- Au moins 5 GB d'espace disque libre

## Lancement en 3 commandes

### Méthode 1 : Compilation native (recommandée)

```bash
# 1. Cloner le projet
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# 2. Configuration et installation des dépendances (prend 10-30 min la première fois)
./scripts/build.sh

# 3. Compilation
./scripts/compile.sh
```

!!! success "C'est prêt!"
Les binaires sont maintenant disponibles dans `artifacts/server/linux/`

### Méthode 2 : Avec Docker

```bash
# 1. Cloner le projet
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype/ci_cd/docker

# 2. Build et compilation avec Docker
docker-compose -f docker-compose.build.yml up
```

## Lancer le serveur

```bash
# Depuis la racine du projet
./artifacts/server/linux/rtype_server
```

Sortie attendue :

```
Hello world
```

!!! info "Note"
Le serveur est actuellement en développement initial. Les fonctionnalités complètes seront disponibles prochainement.

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

## Lancer la documentation

Pour consulter cette documentation localement avec live-reload :

```bash
# Méthode 1 : Avec Docker (recommandé)
cd ci_cd/docker
docker-compose -f docker-compose.docs.yml up

# Méthode 2 : Installation locale de MkDocs
pip install mkdocs-material
mkdocs serve
```

Puis ouvrez votre navigateur à l'adresse : [http://localhost:8000](http://localhost:8000)

## Structure des artifacts

Après compilation, voici ce que vous trouverez :

```
artifacts/
└── server/
    └── linux/
        ├── rtype_server      # Binaire du serveur
        └── server_tests      # Suite de tests
```

## Workflow de développement

### 1. Faire des modifications

Éditez les fichiers source dans `src/server/` :

```bash
# Exemple : éditer le serveur
nano src/server/main.cpp
```

### 2. Recompiler

```bash
# Compilation rapide (ne reconfigure pas CMake)
./scripts/compile.sh
```

### 3. Tester

```bash
# Lancer les tests
./artifacts/server/linux/server_tests

# Lancer le serveur
./artifacts/server/linux/rtype_server
```

### 4. Nettoyer si nécessaire

```bash
# Nettoyer les fichiers de build
rm -rf build/

# Reconfigurer complètement
./scripts/build.sh
./scripts/compile.sh
```

## Commandes utiles

| Commande                                | Description                                 |
| --------------------------------------- | ------------------------------------------- |
| `./scripts/build.sh`                    | Configure CMake et installe les dépendances |
| `./scripts/compile.sh`                  | Compile le projet                           |
| `./artifacts/server/linux/rtype_server` | Lance le serveur                            |
| `./artifacts/server/linux/server_tests` | Lance les tests                             |
| `./third_party/vcpkg/vcpkg list`        | Liste les dépendances installées            |
| `cmake --build build --target clean`    | Nettoie les objets compilés                 |

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
rm -rf build/ artifacts/
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
rm -rf build/
./scripts/build.sh
./scripts/compile.sh
./artifacts/server/linux/server_tests --gtest_verbose
```

## Lancement avec Jenkins (CI/CD)

Le projet inclut un pipeline Jenkins automatisé :

```bash
# Lancer Jenkins
cd ci_cd/docker
docker-compose up -d

# Accéder à Jenkins
# http://localhost:8080
```

Le pipeline exécute automatiquement :

1. Checkout du code
2. Installation des dépendances
3. Compilation
4. Tests

## Prochaines étapes

Maintenant que vous avez lancé le projet :

1. Explorez le [Guide de compilation détaillé](building.md)
2. Consultez l'[Architecture du projet](../guides/architecture.md)
3. Lisez le [Guide de contribution](../development/contributing.md)

## Commandes Docker utiles

Si vous utilisez Docker :

```bash
# Construire l'image de build
docker-compose -f ci_cd/docker/docker-compose.build.yml build

# Lancer la compilation
docker-compose -f ci_cd/docker/docker-compose.build.yml up

# Arrêter les containers
docker-compose -f ci_cd/docker/docker-compose.build.yml down

# Voir les logs
docker-compose -f ci_cd/docker/docker-compose.build.yml logs -f

# Nettoyer les images
docker-compose -f ci_cd/docker/docker-compose.build.yml down --rmi all
```

## Ressources additionnelles

- [Documentation complète de l'installation](installation.md)
- [Guide de compilation avancé](building.md)
- [FAQ](../reference/faq.md)
