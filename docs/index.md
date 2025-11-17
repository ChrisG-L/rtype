# Documentation R-Type

Bienvenue dans la documentation du projet R-Type, un jeu multijoueur inspiré du classique R-Type développé en C++.

## 🚧 État du Projet

!!! info "Phase Actuelle: Infrastructure et Fondations"
Le projet est en **phase de construction des fondations**. L'infrastructure est complète,
le développement du code métier (ECS, Networking, Gameplay) est en cours.

    **✅ Infrastructure:** 100% complété
    **🚧 Code métier:** 5% complété

    📊 [Voir l'état détaillé du projet](project-status.md)

## Vue d'ensemble

R-Type est un projet de jeu multijoueur client-serveur développé avec des technologies modernes C++23. Le projet utilise une architecture modulaire avec:

- **Serveur** : Gestion de la logique de jeu et communication réseau asynchrone via Boost.ASIO
- **Client** : Interface utilisateur et rendu graphique (en développement)
- **Base de données** : MongoDB pour la persistance des données
- **Tests** : Suite de tests unitaires avec Google Test
- **CI/CD** : Pipeline automatisé avec Jenkins et Docker
- **Qualité de code** : Analyse continue avec SonarCloud

## Démarrage rapide

Pour commencer rapidement avec R-Type :

1. [**Installation**](getting-started/installation.md) - Installez les dépendances et préparez votre environnement
2. [**Démarrage Rapide**](getting-started/quickstart.md) - Lancez le projet en quelques commandes
3. [**Compilation**](getting-started/building.md) - Compilez le projet depuis les sources

### Procédure ultra-rapide

```bash
# 1. Cloner
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# 2. Infrastructure (Jenkins + Documentation sur http://localhost:8000)
./scripts/launch_ci_cd.sh

# 3. Build et compilation
./scripts/build.sh
./scripts/compile.sh

# 4. Tests
./artifacts/server/linux/server_tests
```

## Architecture du projet

```
rtype/
├── ci_cd/              # Infrastructure CI/CD (Docker, Jenkins)
├── docs/               # Documentation (vous êtes ici!)
├── scripts/            # Scripts de build et déploiement
├── src/
│   ├── server/        # Code source du serveur
│   └── client/        # Code source du client (à venir)
├── tests/             # Tests unitaires et d'intégration
├── artifacts/         # Binaires compilés
└── third_party/       # Dépendances externes (vcpkg)
```

## Technologies utilisées

| Technologie     | Utilisation                     | Version   |
| --------------- | ------------------------------- | --------- |
| **C++**         | Langage principal               | C++23     |
| **CMake**       | Système de build                | 3.30+     |
| **vcpkg**       | Gestionnaire de dépendances     | Latest    |
| **Boost.ASIO**  | Communication réseau asynchrone | Via vcpkg |
| **Google Test** | Framework de tests              | Via vcpkg |
| **MongoDB**     | Base de données                 | Via vcpkg |
| **Docker**      | Conteneurisation                | Latest    |
| **Jenkins**     | CI/CD                           | LTS       |
| **SonarCloud**  | Analyse de qualité de code      | Cloud     |
| **MkDocs**      | Documentation                   | 1.5+      |

## Méthodes de lancement

Le projet peut être lancé de plusieurs façons selon vos besoins :

### 1. Méthode recommandée (Native + Docker)

```bash
# Infrastructure CI/CD (Jenkins + Documentation)
./scripts/launch_ci_cd.sh

# Build et compilation native
./scripts/build.sh
./scripts/compile.sh

# Lancement du serveur
./artifacts/server/linux/rtype_server
```

!!! tip "Meilleure approche"
    Cette méthode combine les avantages de Docker pour l'infrastructure (Jenkins, docs) et la compilation native pour un développement rapide.

### 2. Build complet avec Docker

```bash
# Infrastructure d'abord
./scripts/launch_ci_cd.sh

# Build via Docker
cd ci_cd/docker
docker-compose -f docker-compose.build.yml up
```

### 3. Accès à la documentation uniquement

```bash
# Lancer uniquement la documentation
cd ci_cd/docker
docker-compose -f docker-compose.docs.yml up
# Accédez à http://localhost:8000
```

### 4. Jenkins CI/CD

Le pipeline Jenkins s'exécute automatiquement à chaque push et effectue :

- Installation des dépendances
- Compilation du projet
- Exécution des tests
- Archivage des artifacts

**Accès :** http://localhost:8081 (après `./scripts/launch_ci_cd.sh`)

### 5. SonarCloud - Analyse de qualité

Pour analyser la qualité du code avec SonarCloud :

```bash
# Configurer les variables d'environnement
export SONAR_TOKEN="votre-token"
export SONAR_ORG="votre-org"
export SONAR_PROJECT="votre-org_rtype"

# Analyser le code
./scripts/sonar-analyze.sh

# Accédez aux résultats
# https://sonarcloud.io/project/overview?id=votre-org_rtype
```

!!! tip "SonarCloud"
    Consultez le [guide complet SonarCloud](guides/sonarqube.md) pour créer un compte, configurer votre projet et utiliser l'analyse de qualité de code.

## Prérequis système

### Linux (Ubuntu 22.04 recommandé)

- **Compilateur :** GCC 11+ ou Clang 15+ (support C++23)
- **Build :** CMake 3.30+, Ninja
- **Outils :** Git, curl, zip/unzip
- **Docker :** Docker Engine 20.10+, Docker Compose 2.0+ (requis pour Jenkins et docs)
- **Matériel :** 4 GB de RAM minimum, 5 GB d'espace disque libre

!!! warning "Docker requis"
    Docker n'est plus optionnel. Il est **nécessaire** pour lancer l'infrastructure CI/CD et la documentation locale via `./scripts/launch_ci_cd.sh`.

## Structure de la documentation

Cette documentation est organisée en plusieurs sections :

- **Pour Commencer** : Installation, démarrage rapide, compilation
- **Guides Utilisateurs** : Architecture, tutoriels, bonnes pratiques, SonarCloud
- **Référence API** : Documentation des APIs du domaine et des adapters
- **Développement** : Guide de contribution, architecture technique, tests
- **Référence** : Glossaire, FAQ

## Liens utiles

- [Repository GitHub](https://github.com/Pluenet-Killian/rtype)
- [Démarrage Rapide](getting-started/quickstart.md)
- [Guide SonarCloud](guides/sonarqube.md)
- [Guide de Contribution](development/contributing.md)
- [FAQ](reference/faq.md)

## Support

Pour toute question ou problème :

1. Consultez la [FAQ](reference/faq.md)
2. Vérifiez les [issues GitHub](https://github.com/Pluenet-Killian/rtype/issues)
3. Contactez l'équipe de développement

## Licence

Ce projet est développé dans le cadre d'un projet pédagogique EPITECH.
