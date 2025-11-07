# R-Type - Jeu Multijoueur en C++

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Documentation](https://img.shields.io/badge/docs-mkdocs-blue)]()
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)]()
[![License](https://img.shields.io/badge/license-Epitech-orange)]()

> Recréation du classique R-Type avec architecture client-serveur moderne en C++23

## 📋 État du Projet

🚧 **En développement actif** - Phase: Infrastructure et fondations

### ✅ Actuellement implémenté

- **Infrastructure CI/CD complète** (Jenkins, Docker)
- **Système de build moderne** (CMake 3.26+, vcpkg, C++23)
- **Documentation technique détaillée** (MkDocs Material)
- **Pipeline de tests automatisés** (Google Test)
- **Scripts de build automatisés**

### 🔨 En cours de développement

- Architecture ECS (Entity Component System)
- Système réseau asynchrone (Boost.ASIO)
- Serveur de jeu multijoueur

### 📋 Planifié

- Client graphique (rendu, UI, input)
- Gameplay complet (ennemis, power-ups, niveaux)
- Système audio
- Base de données persistante (MongoDB)

## 🚀 Quick Start

### Prérequis

- **OS:** Linux (Ubuntu 22.04+ recommandé) ou WSL2
- **Compilateur:** GCC 11+ ou Clang 15+ (support C++23)
- **Build:** CMake 3.26+, Ninja
- **Outils:** Git, curl, zip/unzip

### Installation et Build

```bash
# Cloner le projet
git clone <repository-url>
cd rtype

# Build automatique (installe vcpkg et dépendances)
./scripts/build.sh

# Compilation
./scripts/compile.sh

# Lancer les tests
./artifacts/server/linux/server_tests
```

### Alternative Docker

```bash
# Build via Docker
docker-compose -f ci_cd/docker/docker-compose.build.yml up

# Documentation locale (http://localhost:8000)
docker-compose -f ci_cd/docker/docker-compose.docs.yml up
```

## 📁 Structure du Projet

```
rtype/
├── ci_cd/              # Infrastructure CI/CD
│   ├── docker/         # Dockerfiles et compose
│   └── Jenkinsfile     # Pipeline Jenkins
├── docs/               # Documentation MkDocs Material
│   ├── guides/         # Guides techniques
│   ├── getting-started/# Installation et quickstart
│   └── reference/      # Référence API
├── src/                # Code source
│   ├── server/         # Serveur de jeu
│   └── client/         # Client (en développement)
├── tests/              # Tests unitaires
│   ├── server/         # Tests serveur
│   └── client/         # Tests client (à venir)
├── scripts/            # Scripts de build
│   ├── build.sh        # Configuration et build
│   ├── compile.sh      # Compilation
│   └── vcpkg/          # Scripts vcpkg
├── third_party/        # Dépendances externes
│   └── vcpkg/          # Gestionnaire vcpkg
├── artifacts/          # Binaires compilés
└── CMakeLists.txt      # Configuration CMake
```

## 🛠️ Stack Technique

| Composant | Technologie | Version |
|-----------|-------------|---------|
| **Langage** | C++ | 23 |
| **Build System** | CMake + Ninja | 3.26+ |
| **Gestionnaire de dépendances** | vcpkg | Latest |
| **Networking** | Boost.ASIO | Via vcpkg |
| **Tests** | Google Test | Via vcpkg |
| **Base de données** | MongoDB C++ Driver | Via vcpkg |
| **CI/CD** | Jenkins | LTS |
| **Conteneurisation** | Docker + Docker Compose | Latest |
| **Documentation** | MkDocs Material | 1.5+ |

## 📚 Documentation

La documentation complète est disponible:

- **Localement:**
  ```bash
  docker-compose -f ci_cd/docker/docker-compose.docs.yml up
  # Accéder à http://localhost:8000
  ```

### Sections principales

- [Architecture du projet](docs/guides/architecture.md) - Architecture technique détaillée
- [Installation](docs/getting-started/installation.md) - Guide d'installation complet
- [Quick Start](docs/getting-started/quickstart.md) - Démarrage rapide
- [Guide de build](docs/getting-started/building.md) - Compilation du projet
- [État du projet](docs/project-status.md) - Avancement et statistiques
- [Guide de contribution](docs/development/contributing.md) - Contribuer au projet
- [FAQ](docs/reference/faq.md) - Questions fréquentes

## 🧪 Tests

```bash
# Compiler et exécuter les tests
./scripts/compile.sh
./artifacts/server/linux/server_tests

# Via CTest
cd build
ctest --output-on-failure
```

## 🤝 Contribution

Les contributions sont les bienvenues! Consultez:

- [Guide de contribution](docs/development/contributing.md) pour:
  - Conventions de code
  - Workflow Git
  - Standards de tests
  - Processus de review

## 📊 Gestion de Projet

Le projet est géré sur **Linear**:
- [Tableau de projet R-Type](https://linear.app/epitech-study/project/r-type)
- Tickets, epics et roadmap
- Suivi de l'avancement

## 🎯 Objectifs du Projet

Recréer le jeu classique **R-Type** avec:

1. **Architecture moderne C++23**
   - Entity Component System (ECS)
   - Design patterns modernes
   - Code maintenable et testable

2. **Multijoueur robuste**
   - Architecture client-serveur
   - Networking asynchrone (Boost.ASIO)
   - Synchronisation temps réel

3. **Qualité professionnelle**
   - Tests automatisés
   - CI/CD complet
   - Documentation extensive

## 📝 Licence

**Epitech Project - 2025**

Ce projet est développé dans le cadre du cursus Epitech.

---

**Note:** Ce projet est en développement actif. L'infrastructure et les fondations sont en place, le développement du code métier (ECS, networking, gameplay) est en cours.

Pour l'état d'avancement détaillé, consultez la [page d'état du projet](docs/project-status.md).
