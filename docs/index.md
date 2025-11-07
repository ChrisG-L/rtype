# Documentation R-Type

Bienvenu!e dans la documentation du projet R-Type, un jeu multijoueur inspiré du classique R-Type développé en C++.

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

## Démarrage rapide

Pour commencer rapidement avec R-Type :

1. [**Installation**](getting-started/installation.md) - Installez les dépendances et préparez votre environnement
2. [**Démarrage Rapide**](getting-started/quickstart.md) - Lancez le projet en quelques commandes
3. [**Compilation**](getting-started/building.md) - Compilez le projet depuis les sources

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
| **MkDocs**      | Documentation                   | 1.5+      |

## Méthodes de lancement

Le projet peut être lancé de plusieurs façons selon vos besoins :

### 1. Compilation locale native

```bash
# Configuration et compilation
./scripts/build.sh
./scripts/compile.sh

# Lancement
./artifacts/server/linux/rtype_server
```

### 2. Docker pour le build

```bash
cd ci_cd/docker
docker-compose -f docker-compose.build.yml up
```

### 3. Docker pour la documentation

```bash
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

## Prérequis système

### Linux (Ubuntu 22.04 recommandé)

- GCC 11+ ou Clang 15+ (support C++23)
- CMake 3.30+
- Git
- Ninja build system
- Au moins 4 GB de RAM
- 5 GB d'espace disque libre

### Docker (optionnel)

- Docker Engine 20.10+
- Docker Compose 2.0+

## Structure de la documentation

Cette documentation est organisée en plusieurs sections :

- **Pour Commencer** : Installation, démarrage rapide, compilation
- **Guides Utilisateurs** : Architecture, tutoriels, bonnes pratiques
- **Référence API** : Documentation des APIs du domaine et des adapters
- **Développement** : Guide de contribution, architecture technique, tests
- **Référence** : Glossaire, FAQ

## Liens utiles

- [Repository GitHub](https://github.com/Pluenet-Killian/rtype)
- [Démarrage Rapide](getting-started/quickstart.md)
- [Guide de Contribution](development/contributing.md)
- [FAQ](reference/faq.md)

## Support

Pour toute question ou problème :

1. Consultez la [FAQ](reference/faq.md)
2. Vérifiez les [issues GitHub](https://github.com/Pluenet-Killian/rtype/issues)
3. Contactez l'équipe de développement

## Licence

Ce projet est développé dans le cadre d'un projet pédagogique EPITECH.
