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
- **Outils:** Git, curl, zip/unzip, Docker & Docker Compose

### Installation et Build

```bash
# 1. Cloner le projet
git clone <repository-url>
cd rtype

# 2. Lancer l'infrastructure CI/CD (Jenkins + Documentation)
./scripts/launch_ci_cd.sh

# 3. Build automatique (installe vcpkg et dépendances)
./scripts/build.sh

# 4. Compilation
./scripts/compile.sh

# 5. Lancer les tests
./artifacts/server/linux/server_tests

# 6. Accéder à la documentation locale
# Ouvrez votre navigateur à http://localhost:8000
```

### Que fait chaque étape ?

| Étape | Commande | Durée | Description détaillée |
|-------|----------|-------|----------------------|
| **1** | `git clone` | ~1 min | Récupère le code source du projet depuis GitHub |
| **2** | `./scripts/launch_ci_cd.sh` | ~30 sec | Lance les conteneurs Docker pour Jenkins (http://localhost:8080) et la documentation MkDocs (http://localhost:8000) |
| **3** | `./scripts/build.sh` | 10-30 min | Clone et compile vcpkg, installe toutes les dépendances (Boost.ASIO, Google Test, MongoDB Driver), configure CMake |
| **4** | `./scripts/compile.sh` | ~2 min | Compile le serveur et les tests avec Ninja |
| **5** | `server_tests` | ~1 sec | Exécute la suite de tests unitaires avec Google Test |
| **6** | Documentation | - | Ouvrez http://localhost:8000 dans votre navigateur |

!!! note "Première installation"
    L'étape 3 (build) est longue uniquement la première fois car vcpkg compile les dépendances depuis les sources. Les builds suivants seront beaucoup plus rapides (~2 minutes).

### Services disponibles après installation

Une fois l'installation terminée, vous aurez accès à :

- **Documentation locale** : http://localhost:8000 (guides complets, API, architecture)
- **Jenkins CI/CD** : http://localhost:8080 (pipelines automatisés)
- **Serveur R-Type** : `./artifacts/server/linux/rtype_server`
- **Tests unitaires** : `./artifacts/server/linux/server_tests`

!!! tip "Analyse de qualité avec SonarCloud"
    Pour l'analyse de qualité de code (bugs, vulnérabilités, code smells), utilisez SonarCloud. Consultez le [guide SonarQube](docs/guides/sonarqube.md) pour configurer et utiliser SonarCloud avec le projet.

### Alternative Docker (build complet)

Pour un build complètement isolé via Docker :

```bash
# 1. Infrastructure d'abord
./scripts/launch_ci_cd.sh

# 2. Build via Docker
cd ci_cd/docker
docker-compose -f docker-compose.build.yml up
```

### Dépannage rapide

| Problème | Solution |
|----------|----------|
| Port 8000 ou 8080 déjà utilisé | `docker-compose -f ci_cd/docker/docker-compose.yml down` puis relancez |
| Erreur vcpkg | `rm -rf third_party/vcpkg && ./scripts/build.sh` |
| Erreur compilation | `rm -rf build && ./scripts/build.sh && ./scripts/compile.sh` |
| Docker non trouvé (WSL2) | Installez Docker Desktop et activez l'intégration WSL2 |

📖 Pour plus de détails, consultez la [documentation complète](docs/getting-started/installation.md)

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
