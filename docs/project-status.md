# État du Projet R-Type

**Dernière mise à jour:** 3 décembre 2025
**Version:** 0.5.0 (Phase: Gameplay Foundation)

---

## 📊 Vue d'Ensemble

Le projet R-Type est en phase de **développement actif** avec un client graphique fonctionnel, un serveur avec architecture hexagonale complète, et un module gameplay opérationnel.

### Phase Actuelle

🚧 **Phase 4: Gameplay Foundation** (85% complété)

**Réalisations:**
- ✅ Architecture hexagonale serveur complète (Domain/Application/Infrastructure)
- ✅ Client graphique SFML fonctionnel (Boot, Engine, GameLoop, Scenes)
- ✅ Système de scènes (LoginScene, GameScene)
- ✅ Communication TCP client-serveur (Boost.Asio)
- ✅ Authentification complète (Login, Register, MongoDBUserRepository)
- ✅ Système de logging professionnel (spdlog) - 12 loggers
- ✅ Gestionnaire d'assets avec Z-index (AssetManager)
- ✅ **Module Gameplay** (GameObject, Missile, EntityManager)
- ✅ **Système d'événements abstrait** (découplage SFML)
- ✅ **GameLoop avec deltaTime correct** (std::chrono)
- ✅ **Contrôles joueur** (flèches + tir)
- 🚧 ECS complet en cours

**Prochaine phase:** ECS complet et Multijoueur

---

## 📈 Progression par Composant

| Composant | État | Avancement | Ticket Linear | Notes |
|-----------|------|------------|---------------|-------|
| **Infrastructure CI/CD** | ✅ Terminé | 100% | [EPI-9](https://linear.app/epitech-study/issue/EPI-9) | Pipeline complet |
| **Documentation** | 🚧 En cours | 85% | [EPI-15](https://linear.app/epitech-study/issue/EPI-15) | Mise à jour en cours |
| **Architecture Hexagonale** | ✅ Terminé | 100% | [EPI-22](https://linear.app/epitech-study/issue/EPI-22) | 3 couches complètes |
| ├─ Séparation .hpp/.cpp | ✅ Terminé | 100% | [EPI-23](https://linear.app/epitech-study/issue/EPI-23) | 40+ fichiers .cpp |
| └─ Serveurs Réseau | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | UDP + TCP |
| **Domain Layer** | ✅ Terminé | 95% | - | Entités et Value Objects |
| ├─ Entité Player | ✅ Terminé | 100% | - | Position, Health, PlayerId |
| ├─ Entité User | ✅ Terminé | 100% | - | Auth (Username, Email, Password) |
| ├─ Value Objects | ✅ Terminé | 100% | - | 9 Value Objects (incl. Email) |
| └─ Exceptions | ✅ Terminé | 100% | - | 10 Exceptions métier |
| **Application Layer** | ✅ Terminé | 90% | - | Use Cases |
| ├─ MovePlayerUseCase | ✅ Terminé | 100% | - | Déplacement joueur |
| ├─ LoginUseCase | ✅ Terminé | 100% | - | Authentification par email |
| └─ RegisterUseCase | ✅ Terminé | 100% | - | Inscription utilisateur |
| **Infrastructure Layer** | 🚧 En cours | 80% | - | Adapters |
| ├─ UDPServer | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | Port 4123 |
| ├─ TCPServer | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | Port 3000 |
| ├─ MongoDBConfiguration | ✅ Terminé | 100% | - | Connexion DB |
| ├─ MongoDBUserRepository | ✅ Terminé | 100% | - | CRUD utilisateurs |
| └─ MongoDBPlayerRepository | 🚧 En cours | 30% | - | Stub implémentation |
| **Client Graphique** | 🚧 En cours | 65% | [EPI-11](https://linear.app/epitech-study/issue/EPI-11) | SFML fonctionnel |
| ├─ Boot/Engine/GameLoop | ✅ Terminé | 100% | - | Architecture complète |
| ├─ SceneManager | ✅ Terminé | 100% | - | LoginScene, GameScene |
| ├─ SFMLRenderer/Window | ✅ Terminé | 90% | - | Rendu fonctionnel |
| ├─ AssetManager | ✅ Terminé | 95% | - | Cache textures/sprites |
| ├─ TCPClient | ✅ Terminé | 85% | - | Communication async |
| └─ UI Components | 🚧 En cours | 20% | - | TextField en cours |
| **Module Gameplay** | 🚧 En cours | 25% | [KAN-65](https://epitech-team-w5qkn5hj.atlassian.net) | GameObject, Missile, EntityManager |
| **Architecture ECS** | 🚧 En cours | 20% | [KAN-12](https://epitech-team-w5qkn5hj.atlassian.net) | Foundation via EntityManager |

### Légende des États

- ✅ **Terminé** - Implémenté, compile et testé
- 🚧 **En cours** - Développement actif
- 📋 **Planifié** - Conception faite, développement à venir
- 💡 **Idée** - Concept en exploration

---

## 💻 Statistiques Code

### Lignes de Code (Mise à jour 25/11/2025)

| Catégorie | Fichiers | Lignes | Pourcentage |
|-----------|----------|--------|-------------|
| **Code Source Serveur** | 50+ | ~3,500 | 35% |
| ├─ Headers (.hpp) | 25+ | ~1,500 | 15% |
| └─ Implémentations (.cpp) | 25+ | ~2,000 | 20% |
| **Code Source Client** | 30+ | ~2,500 | 25% |
| ├─ Headers (.hpp) | 20+ | ~1,200 | 12% |
| └─ Implémentations (.cpp) | 14 | ~1,300 | 13% |
| **Documentation** | 42 | ~7,500 | 35% |
| **Tests** | 5+ | 200+ | 2% |
| **Total Projet** | 120+ | ~13,700 | 100% |

**Ratio Documentation/Code:** 1.25:1

### Architecture du Code Source

```
src/
├── server/                          # Serveur de jeu (Architecture Hexagonale)
│   ├── domain/
│   │   ├── entities/
│   │   │   ├── Player.hpp/.cpp ✅
│   │   │   └── User.hpp/.cpp ✅
│   │   ├── value_objects/
│   │   │   ├── Health.hpp/.cpp ✅
│   │   │   ├── Position.hpp/.cpp ✅
│   │   │   ├── player/PlayerId.hpp/.cpp ✅
│   │   │   └── user/ ✅
│   │   │       ├── UserId, Username, Password.hpp/.cpp
│   │   │       ├── Email.hpp/.cpp ✅ (nouveau)
│   │   │       └── utils/PasswordUtils.hpp/.cpp ✅
│   │   └── exceptions/ ✅ (10 exceptions métier)
│   ├── application/
│   │   ├── use_cases/
│   │   │   ├── MovePlayerUseCase.hpp/.cpp ✅
│   │   │   ├── Login.hpp/.cpp ✅
│   │   │   └── Register.hpp/.cpp ✅
│   │   └── ports/ (IGameCommands, IUserRepository, IPlayerRepository) ✅
│   ├── infrastructure/
│   │   ├── adapters/
│   │   │   ├── in/network/
│   │   │   │   ├── TCPServer.hpp/.cpp ✅
│   │   │   │   ├── UDPServer.hpp/.cpp ✅
│   │   │   │   ├── Execute.hpp/.cpp ✅
│   │   │   │   └── ExecuteAuth.hpp/.cpp ✅
│   │   │   └── out/persistence/
│   │   │       ├── MongoDBConfiguration.hpp/.cpp ✅
│   │   │       ├── MongoDBUserRepository.hpp/.cpp ✅
│   │   │       └── MongoDBPlayerRepository.hpp 🚧
│   │   ├── logging/Logger.hpp/.cpp ✅
│   │   └── boostrap/GameBootstrap.hpp ✅
│   └── main.cpp ✅
│
└── client/                          # Client de jeu (SFML)
    ├── boot/Boot.hpp/.cpp ✅
    ├── core/
    │   ├── Engine.hpp/.cpp ✅
    │   ├── GameLoop.hpp/.cpp ✅
    │   └── Logger.hpp/.cpp ✅
    ├── scenes/
    │   ├── SceneManager.hpp/.cpp ✅
    │   ├── LoginScene.hpp/.cpp ✅
    │   └── GameScene.hpp/.cpp ✅
    ├── graphics/Graphics.hpp/.cpp ✅
    ├── network/TCPClient.hpp/.cpp ✅
    ├── implementations/sfml/
    │   ├── SFMLWindow.hpp/.cpp ✅
    │   ├── SFMLRenderer.hpp/.cpp ✅
    │   ├── SFMLTexture.hpp/.cpp ✅
    │   └── utils/AssetManager.hpp/.cpp ✅
    ├── include/
    │   ├── core/ (IEngine, IGameLoop, IRenderer)
    │   ├── graphics/ (IWindow, ITexture, IDrawable, Asset, GraphicTexture)
    │   ├── scenes/ (IScene)
    │   ├── ui/ (IUIElement, Button, TextInput) 🚧
    │   └── utils/ (Vecs, TextField)
    └── main.cpp ✅
```

### Dépendances Utilisées

| Dépendance | Version | Statut | Utilisation |
|------------|---------|--------|-------------|
| Boost.ASIO | Via vcpkg | ✅ Utilisé | UDPServer, TCPServer (actif) |
| MongoDB C++ Driver | Via vcpkg | ✅ Utilisé | MongoDBConfiguration (actif) |
| Google Test | Via vcpkg | ✅ Installé | Tests unitaires (à développer) |
| bsoncxx | Via vcpkg | ✅ Utilisé | Validation UUID, serialization |

---

## 🎯 Jalons et Roadmap

### ✅ Jalon 1: Infrastructure (Complété - Nov 2025)

**Objectif:** Mettre en place l'infrastructure de développement
**Statut:** ✅ **100% Terminé**

**Réalisations:**
- ✅ Configuration CMake + vcpkg
- ✅ Pipeline CI/CD Jenkins
- ✅ Conteneurisation Docker
- ✅ Documentation MkDocs
- ✅ Tests automatisés configurés

### ✅ Jalon 2: Architecture Hexagonale (Complété - Jan 2025)

**Objectif:** Implémenter l'architecture hexagonale complète
**Statut:** ✅ **100% Terminé**

**Réalisations:**
- ✅ Domain Layer complet (Player, User, Value Objects, Exceptions)
- ✅ Application Layer (Ports, Use Cases)
- ✅ Infrastructure Layer (Adapters, MongoDB, Network)
- ✅ Séparation .hpp/.cpp (18 fichiers)
- ✅ Serveurs réseau UDP/TCP Boost.Asio

**Tickets:**
- [EPI-22: Epic Refactoring Architecture C++](https://linear.app/epitech-study/issue/EPI-22)
- [EPI-23: Séparation .hpp/.cpp](https://linear.app/epitech-study/issue/EPI-23)
- [EPI-24: Serveurs réseau UDP/TCP](https://linear.app/epitech-study/issue/EPI-24)

### ✅ Jalon 3: Core Engine (Complété - Nov 2025)

**Objectif:** Développer le moteur de jeu de base
**Date complétée:** Novembre 2025
**Statut:** ✅ **95% complété**

**Réalisations:**
- ✅ Système d'authentification User complet
  - ✅ Entité User avec Email
  - ✅ IUserRepository + MongoDBUserRepository
  - ✅ LoginUseCase (authentification par email)
  - ✅ RegisterUseCase (inscription)
  - ✅ ExecuteAuth (routage commandes auth)
- ✅ Client graphique fonctionnel
  - ✅ Boot, Engine, GameLoop
  - ✅ SceneManager avec LoginScene/GameScene
  - ✅ SFMLRenderer, SFMLWindow
  - ✅ AssetManager avec cache
  - ✅ TCPClient asynchrone
- ✅ Système de logging (12 loggers)
- 🚧 UI Components (TextField en cours)

**Tickets Linear:**
- [EPI-17: Architecture ECS](https://linear.app/epitech-study/issue/EPI-17) - Prochaine priorité

### 🚧 Jalon 4: Gameplay et ECS (En cours - Déc 2025)

**Objectif:** Gameplay R-Type fonctionnel avec ECS
**Date estimée:** Décembre 2025
**Statut:** 🚧 **35% complété**

**Tâches principales:**
- ✅ **Module Gameplay** (GameObject, Missile, EntityManager)
- ✅ **Vaisseau joueur contrôlable** (flèches directionnelles)
- ✅ **Système de tir** (Space + cooldown)
- ✅ **GameLoop deltaTime** (std::chrono)
- ✅ **Système événements abstrait** (découplage SFML)
- 🚧 Architecture ECS complète (composants/systèmes)
- 📋 Ennemis basiques avec patterns
- 📋 Système de collisions
- 📋 Power-ups
- 📋 Synchronisation réseau état de jeu

### 📋 Jalon 5: Polish et Multijoueur (Planifié - 2026)

**Objectif:** Expérience multijoueur complète
**Date estimée:** 2026
**Statut:** 📋 **Planifié (0%)**

**Tâches principales:**
- UI et menus complets
- Système audio (musique, effets)
- Matchmaking
- Lobby et chat
- Optimisations réseau

---

## 📊 Métriques de Qualité

### Build & Tests

| Métrique | Valeur | Statut |
|----------|--------|--------|
| **Build Status** | ✅ Passing | Excellent |
| **Temps de Build** | ~15s (incrémental) | Excellent |
| **Temps de Build** | ~3 min (complet) | Bon |
| **Tests Status** | ✅ Passing | Bon |
| **Couverture Tests** | ~5% | À améliorer |
| **Pipeline CI/CD** | ✅ Automatisé | Excellent |
| **Sanitizers** | ✅ Activés (Debug) | Excellent |

### Code Quality

| Métrique | Valeur | Statut |
|----------|--------|--------|
| **Warnings de Compilation** | 3 mineurs | Excellent |
| **Architecture** | Hexagonale | Excellent |
| **Séparation .hpp/.cpp** | 100% | Excellent |
| **Standards C++23** | ✅ Respectés | Excellent |
| **Const Correctness** | ✅ Respectée | Excellent |

### Documentation

| Métrique | Valeur | Statut |
|----------|--------|--------|
| **Pages Documentation** | 12 | Très bon |
| **Lignes Documentation** | 3,200+ | Excellent |
| **Diagrammes** | 8+ | Bon |
| **Guides Complets** | ✅ Oui | Excellent |
| **Documentation API** | 🚧 En cours | À compléter |
| **Cohérence avec Code** | 75% | En amélioration |

---

## 🚀 Prochaines Étapes

### Cette Semaine (Priorité Urgente)

1. **Architecture ECS** - [EPI-17](https://linear.app/epitech-study/issue/EPI-17)
   - Entity Manager
   - Components (Position, Velocity, Sprite, Collider)
   - Systems (Movement, Rendering, Collision)

2. **UI Components Client**
   - Finaliser TextField pour input utilisateur
   - Implémenter Button interactif
   - Menu de connexion fonctionnel

3. **Intégration Auth Client-Serveur**
   - Connecter LoginScene au serveur via TCPClient
   - Implémenter RegisterScene

### Semaines Suivantes (Priorité Haute)

4. **Protocole Réseau Gameplay**
   - Définir format messages UDP (positions, actions)
   - Définir format messages TCP (auth, chat)
   - Sérialisation/désérialisation Protocol Buffers

5. **Gameplay de Base**
   - Vaisseau joueur contrôlable
   - Système de tir basique
   - Premier ennemi

### Plus Tard

6. **Ennemis et Patterns** - IA basique
7. **Collisions** - Détection et réponse
8. **Power-ups** - Système d'améliorations
9. **Audio** - Musique et effets sonores
10. **Polish** - Optimisations et finitions

---

## 📞 Contact et Contribution

### Gestion de Projet

- **Linear:** [Projet R-Type](https://linear.app/epitech-study/project/r-type)
- **Git:** Commits atomiques avec convention AREA
- **Issues:** Créer un ticket sur Linear

### Documentation

- **Architecture:** [guides/architecture.md](guides/architecture.md)
- **Hexagonal:** [guides/hexagonal-architecture.md](guides/hexagonal-architecture.md)
- **Réseau:** [guides/network-architecture.md](guides/network-architecture.md) 🚧
- **Contribution:** [development/contributing.md](development/contributing.md)

### Ressources

- **Build:** `cmake --build build`
- **Tests:** `ctest --test-dir build`
- **Docs:** `docker-compose up mkdocs`
- **Jenkins:** Pipeline CI/CD automatisé

---

## 📝 Notes de Version

### v0.2.0 (11 janvier 2025) - Architecture Complète

**Ajouts majeurs:**
- ✅ Architecture hexagonale complète (Domain/Application/Infrastructure)
- ✅ Séparation .hpp/.cpp (18 fichiers .cpp créés)
- ✅ UDPServer et TCPServer (Boost.Asio)
- ✅ Entité User pour l'authentification
- ✅ Value Objects User (UserId, Username, Password)
- ✅ Exceptions User (UserIdException, UsernameException, PasswordException)
- ✅ MongoDBConfiguration
- ✅ Main.cpp intégré avec UDPServer

**Améliorations:**
- Compilation incrémentale rapide (séparation .hpp/.cpp)
- Code maintenable et testable
- Architecture évolutive
- Standards C++23 respectés

### v0.1.0 (7 novembre 2025) - Infrastructure

**Première version:**
- CI/CD complet
- Documentation extensive
- Configuration CMake/vcpkg
- Tests automatisés

---

## 🏆 Accomplissements

**Ce qui fonctionne aujourd'hui:**

### Serveur
- ✅ Build automatisé avec CMake
- ✅ Pipeline Jenkins fonctionnel
- ✅ Serveur UDP écoute sur port 4123
- ✅ Serveur TCP accepte connexions (port 3000)
- ✅ Entités Player et User complètes
- ✅ Value Objects avec validation (9 VOs dont Email)
- ✅ Exceptions métier (10 exceptions)
- ✅ MongoDB configuré avec MongoDBUserRepository
- ✅ Architecture hexagonale respectée
- ✅ Use Cases complets (Login, Register, MovePlayer)
- ✅ Système de logging (6 loggers serveur)

### Client
- ✅ Client SFML fonctionnel
- ✅ Architecture modulaire (Boot, Engine, GameLoop)
- ✅ Système de scènes (LoginScene, GameScene)
- ✅ SFMLRenderer et SFMLWindow
- ✅ AssetManager avec cache textures/sprites
- ✅ TCPClient asynchrone
- ✅ Système de logging (6 loggers client)

**Ce qui reste à faire:**
- 🚧 UI Components (TextField, Button)
- 🚧 Intégration complète auth client-serveur
- 📋 Architecture ECS
- 📋 Protocole réseau gameplay (UDP)
- 📋 Gameplay (vaisseau, tir, ennemis)
- 📋 Audio et effets
- 📋 Multijoueur complet

---

**Dernière révision:** 03/12/2025 par le Général Army2077 ⭐
**Prochaine mise à jour:** Après implémentation ECS complet
