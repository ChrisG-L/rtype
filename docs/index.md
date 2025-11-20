# Documentation R-Type

Bienvenue dans la documentation officielle du projet **R-Type**, un jeu multijoueur client-serveur moderne développé en C++23 avec une architecture hexagonale et des technologies de pointe.

## 🚀 Démarrage Ultra-Rapide

```bash
git clone https://github.com/Pluenet-Killian/rtype.git && cd rtype
./scripts/launch_ci_cd.sh  # Infrastructure (Jenkins + Docs)
./scripts/build.sh          # Installation dépendances
./scripts/compile.sh        # Compilation
./artifacts/server/linux/rtype_server  # Lancement serveur
```

**Accès:**
- 📚 Documentation: http://localhost:8000
- 🔧 Jenkins CI/CD: http://localhost:8081
- 🎮 Serveur: localhost:4123 (TCP)

[→ Guide complet de démarrage](getting-started/index.md){ .md-button .md-button--primary }

---

## 🚧 État du Projet

!!! success "Phase Actuelle: Développement Actif"
    Le projet est en **phase de construction** avec infrastructure complète et implémentation progressive des fonctionnalités.

    **Infrastructure:** ✅ 100% complété
    **Système de Logging:** ✅ 100% complété 🆕
    **Client (Core, Graphics, Network):** 🚧 50% complété
    **Serveur (Domain, Network):** 🚧 60% complété
    **Gameplay:** 📋 Planifié

📊 [Voir l'état détaillé du projet](project-status.md)

---

## ✨ Fonctionnalités Récentes

### 🆕 Système de Logging Professionnel (spdlog)

Le projet intègre maintenant un système de logging centralisé de niveau production:

=== "Client (6 loggers)"
    - **NetworkLogger** - Connexions, paquets (DEBUG)
    - **EngineLogger** - Lifecycle du moteur (INFO)
    - **GraphicsLogger** - Rendu, textures (INFO)
    - **SceneLogger** - Transitions de scènes (INFO)
    - **UILogger** - Interactions utilisateur (INFO)
    - **BootLogger** - Initialisation (INFO)

=== "Serveur (6 loggers)"
    - **NetworkLogger** - TCP/UDP, connexions (DEBUG)
    - **AuthLogger** - Authentification, sécurité (INFO)
    - **DomainLogger** - Logique métier (INFO)
    - **PersistenceLogger** - MongoDB, requêtes (INFO)
    - **GameLogger** - Sessions de jeu (INFO)
    - **MainLogger** - Démarrage, arrêt (INFO)

**Caractéristiques:**
- ✅ Multi-sink: Console couleur + Fichiers rotatifs (10MB)
- ✅ Thread-safe par défaut
- ✅ Formatage type-safe (syntaxe fmt)
- ✅ Niveaux configurables (trace, debug, info, warn, error, critical)
- ✅ Performance optimisée (overhead <1%)

```cpp
// Exemple d'utilisation
auto logger = client::logging::Logger::getNetworkLogger();
logger->info("Connecting to {}:{}...", host, port);
logger->error("Connection failed: {}", error.message());
```

[→ Documentation complète du logging](development/logging.md){ .md-button }

---

## 🏗️ Architecture du Projet

```mermaid
graph TB
    subgraph "Client (C++23)"
        Boot[Boot] --> Engine[Engine]
        Engine --> GameLoop[GameLoop]
        GameLoop --> Scenes[Scenes]
        GameLoop --> Renderer[Renderer]
        GameLoop --> TCPClient[TCPClient]
    end

    subgraph "Infrastructure"
        Jenkins[Jenkins CI/CD]
        Docs[MkDocs]
        SonarCloud[SonarCloud]
    end

    subgraph "Serveur (C++23)"
        TCP[TCPServer]
        UDP[UDPServer]
        Domain[Domain Layer]
        Persistence[MongoDB]
    end

    TCPClient -.TCP.-> TCP
    GameLoop -.UDP.-> UDP
    TCP --> Domain
    UDP --> Domain
    Domain --> Persistence

    style Boot fill:#4CAF50
    style TCPClient fill:#2196F3
    style TCP fill:#2196F3
    style Domain fill:#FF9800
```

### Organisation du Code

```
rtype/
├── src/
│   ├── client/              # Client de jeu
│   │   ├── core/           # Engine, GameLoop, Boot
│   │   ├── graphics/       # Rendu, AssetManager
│   │   ├── network/        # TCPClient
│   │   └── scenes/         # LoginScene, GameScene
│   │
│   └── server/              # Serveur de jeu
│       ├── domain/         # Entités, Value Objects
│       ├── application/    # Use Cases, Services
│       └── infrastructure/ # Repositories, Network, Logging
│
├── tests/                   # Tests unitaires (Google Test)
├── docs/                    # Documentation (vous êtes ici!)
├── ci_cd/                   # Infrastructure Jenkins/Docker
└── scripts/                 # Scripts de build/déploiement
```

[→ Architecture détaillée](guides/architecture.md)

---

## 🎯 Guides par Profil

### Pour les Nouveaux

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Installation**

    ---

    Installez les prérequis et configurez votre environnement en 15 minutes.

    [:octicons-arrow-right-24: Guide d'installation](getting-started/installation.md)

-   :material-flash:{ .lg .middle } **Quickstart**

    ---

    Lancez le projet en 5 minutes avec notre guide de démarrage rapide.

    [:octicons-arrow-right-24: Démarrage rapide](getting-started/quickstart.md)

-   :material-hammer-wrench:{ .lg .middle } **Compilation**

    ---

    Compilez le projet avec CMake, sanitizers, et options avancées.

    [:octicons-arrow-right-24: Guide de compilation](getting-started/building.md)

-   :material-help-circle:{ .lg .middle } **FAQ**

    ---

    Réponses aux questions fréquentes et résolution de problèmes.

    [:octicons-arrow-right-24: Voir la FAQ](reference/faq.md)

</div>

### Pour les Développeurs

<div class="grid cards" markdown>

-   :material-code-braces:{ .lg .middle } **Architecture Client**

    ---

    Explorez l'architecture du client: Core, Graphics, Network.

    [:octicons-arrow-right-24: Documentation client](client/index.md)

-   :material-server:{ .lg .middle } **Architecture Hexagonale**

    ---

    Comprenez l'architecture hexagonale du serveur.

    [:octicons-arrow-right-24: Guide hexagonal](guides/hexagonal-architecture.md)

-   :material-console:{ .lg .middle } **Système de Logging**

    ---

    Utilisez spdlog pour un logging professionnel.

    [:octicons-arrow-right-24: Documentation logging](development/logging.md)

-   :material-test-tube:{ .lg .middle } **Tests & Qualité**

    ---

    Google Test, sanitizers, et bonnes pratiques de testing.

    [:octicons-arrow-right-24: Guide des tests](development/testing.md)

</div>

### Pour les Contributeurs

<div class="grid cards" markdown>

-   :material-git:{ .lg .middle } **Contribuer**

    ---

    Workflow Git, standards de code, et processus de contribution.

    [:octicons-arrow-right-24: Guide de contribution](development/contributing.md)

-   :material-source-commit:{ .lg .middle } **Politiques de Commit**

    ---

    Conventions AREA et format des messages de commit.

    [:octicons-arrow-right-24: Politiques](development/COMMIT_POLICES.md)

-   :material-robot-industrial:{ .lg .middle } **CI/CD Jenkins**

    ---

    Pipeline automatisé et builder permanent.

    [:octicons-arrow-right-24: Documentation CI/CD](development/ci-cd.md)

-   :material-chart-line:{ .lg .middle } **SonarCloud**

    ---

    Analyse de qualité de code et métriques.

    [:octicons-arrow-right-24: Guide SonarCloud](guides/sonarqube.md)

</div>

---

## 🛠️ Technologies Utilisées

<div class="grid" markdown>

| Catégorie | Technologies | Version |
|-----------|--------------|---------|
| **Langage** | C++23 | GCC 11+ / Clang 15+ |
| **Build** | CMake, Ninja, vcpkg | 3.30+ |
| **Réseau** | Boost.ASIO | Latest |
| **Graphics** | SFML 3.0+ | 3.0.1 |
| **Logging** | spdlog 🆕 | Latest |
| **Database** | MongoDB C++ Driver | Latest |
| **Tests** | Google Test, Sanitizers | Latest |
| **CI/CD** | Jenkins, Docker | LTS |
| **Qualité** | SonarCloud | Cloud |
| **Docs** | MkDocs Material | 1.5+ |

</div>

[→ Stack technique complète](guides/architecture.md#technologies)

---

## 📊 Métriques du Projet

<div class="grid" markdown>

| Métrique | Valeur |
|----------|--------|
| **Lignes de code** | ~10,000+ |
| **Fichiers sources** | 50+ |
| **Tests unitaires** | 30+ tests |
| **Couverture tests** | 60%+ |
| **Loggers implémentés** | 12 (6 client + 6 serveur) |
| **Commits** | 100+ |
| **Documentation** | 35+ pages |

</div>

---

## 🎮 Fonctionnalités Implémentées

### Client

- [x] **Boot System** - Initialisation et connexion serveur
- [x] **Engine** - Orchestration générale
- [x] **GameLoop** - Boucle de jeu update/render
- [x] **SceneManager** - Gestion des scènes (Login, Game)
- [x] **TCPClient** - Communication réseau asynchrone
- [x] **AssetManager** - Gestion textures et sprites
- [x] **Logging** - Système centralisé spdlog 🆕
- [x] **SFML Renderer** - Backend graphique
- [ ] **UDP Client** - Gameplay temps réel (planifié)
- [ ] **ECS** - Entity Component System (planifié)
- [ ] **UI System** - Interface utilisateur (en cours)

### Serveur

- [x] **TCPServer** - Serveur TCP asynchrone
- [x] **UDPServer** - Serveur UDP asynchrone
- [x] **Architecture Hexagonale** - Domain, Application, Infrastructure
- [x] **User Entity** - Authentification et utilisateurs
- [x] **MongoDB Integration** - Persistance des données
- [x] **Logging** - Système centralisé spdlog 🆕
- [ ] **Game Logic** - Logique de jeu (en cours)
- [ ] **Matchmaking** - Système de matchmaking (planifié)
- [ ] **ECS Serveur** - Synchronisation d'état (planifié)

### Infrastructure

- [x] **Jenkins CI/CD** - Pipeline automatisé
- [x] **Builder Permanent** - Workspaces isolés, cache vcpkg
- [x] **Docker** - Conteneurisation complète
- [x] **MkDocs** - Documentation exhaustive
- [x] **SonarCloud** - Analyse qualité de code
- [x] **Google Test** - Framework de tests
- [x] **Sanitizers** - ASan, TSan, LSan, UBSan
- [x] **vcpkg** - Gestion dépendances

---

## 🔗 Liens Rapides

<div class="grid cards" markdown>

-   **Repository**

    [:fontawesome-brands-github: GitHub](https://github.com/Pluenet-Killian/rtype)

-   **CI/CD**

    [:material-wrench: Jenkins Local](http://localhost:8081)

-   **Documentation**

    [:material-book-open-page-variant: Docs Local](http://localhost:8000)

-   **Qualité**

    [:material-chart-box: SonarCloud](https://sonarcloud.io)

</div>

---

## 📚 Structure de la Documentation

Cette documentation est organisée en plusieurs sections pour faciliter la navigation:

=== "Essentiels"
    - **[Pour Commencer](getting-started/index.md)** - Installation, quickstart, compilation
    - **[État du Projet](project-status.md)** - Avancement et roadmap
    - **[FAQ](reference/faq.md)** - Questions fréquentes

=== "Client"
    - **[Architecture Client](client/index.md)** - Vue d'ensemble
    - **[Core](client/core/index.md)** - Engine, GameLoop, Scenes
    - **[Graphics](client/graphics/index.md)** - Rendu, AssetManager
    - **[Network](client/network/index.md)** - TCPClient, protocoles

=== "Guides"
    - **[Architecture Générale](guides/architecture.md)** - Vue d'ensemble du système
    - **[Architecture Hexagonale](guides/hexagonal-architecture.md)** - Serveur DDD
    - **[Architecture Réseau](guides/network-architecture.md)** - TCP/UDP, Boost.ASIO
    - **[Authentification](guides/authentication.md)** - User entity, sécurité

=== "Développement"
    - **[Contribuer](development/contributing.md)** - Workflow et standards
    - **[Logging](development/logging.md)** - Système spdlog 🆕
    - **[Tests](development/testing.md)** - Google Test, sanitizers
    - **[CI/CD](development/ci-cd.md)** - Jenkins, Docker
    - **[Changelog](development/changelog.md)** - Historique des versions

---

## 💬 Support et Contribution

### Besoin d'Aide?

1. 📖 Consultez la [FAQ](reference/faq.md)
2. 🔍 Utilisez la recherche (en haut à droite)
3. 🐛 Vérifiez les [Issues GitHub](https://github.com/Pluenet-Killian/rtype/issues)
4. 💬 Contactez l'équipe de développement

### Contribuer au Projet

Nous accueillons chaleureusement les contributions! Pour commencer:

1. Lisez le [Guide de Contribution](development/contributing.md)
2. Comprenez les [Politiques de Commit](development/COMMIT_POLICES.md)
3. Explorez les [Issues "Good First Issue"](https://github.com/Pluenet-Killian/rtype/labels/good%20first%20issue)
4. Créez votre Pull Request!

[→ Guide complet de contribution](development/contributing.md){ .md-button .md-button--primary }

---

## 🎓 Projet Pédagogique EPITECH

Ce projet est développé dans le cadre d'un cursus EPITECH par l'équipe AREA.

**Objectifs pédagogiques:**
- Architecture logicielle moderne (hexagonale, SOLID)
- C++23 et programmation avancée
- Réseau asynchrone et temps réel
- Tests automatisés et qualité de code
- CI/CD et DevOps
- Documentation technique professionnelle

---

## 📝 Licence

Ce projet est développé dans un contexte pédagogique EPITECH.

---

<div class="grid" markdown style="text-align: center; padding: 2rem 0;">

**Prêt à commencer?**

[Installation](getting-started/installation.md){ .md-button .md-button--primary }
[Quickstart](getting-started/quickstart.md){ .md-button }
[Architecture](guides/architecture.md){ .md-button }

</div>
