# État du Projet R-Type

**Dernière mise à jour:** 15 décembre 2025
**Version:** 0.5.1 (Phase: Gameplay Foundation)

---

## 📊 Vue d'Ensemble

Le projet R-Type est en phase de **gameplay actif** avec un client multi-backend (SDL2/SFML) complet, un serveur avec GameWorld fonctionnel (missiles, ennemis, collisions), et une librairie ECS prête à intégrer.

### Phase Actuelle

✅ **Phase 4: Gameplay Foundation** (95% complété)

**Réalisations:**
- ✅ Architecture hexagonale serveur complète (Domain/Application/Infrastructure)
- ✅ Client multi-backend (SDL2 par défaut, SFML via plugin dynamique)
- ✅ UDPClient temps réel (Boost.ASIO, thread-safe)
- ✅ GameScene complet avec HUD, missiles, ennemis, parallax stars
- ✅ AudioManager (SDL2_mixer) - musique + effets sonores
- ✅ AccessibilityConfig - remapping clavier, modes daltonien, vitesse de jeu
- ✅ GameWorld serveur - joueurs, missiles, 5 types d'ennemis avec IA
- ✅ Système de collision AABB avec damage events
- ✅ Wave spawning - vagues d'ennemis automatiques
- ✅ Protocol binaire - 62 types de messages (network byte order)
- ✅ Broadcast à 20Hz avec état de jeu complet
- ✅ **Blob-ECS** - Librairie ECS complète (51.3M ops/s, non intégrée)

**Prochaine phase:** Intégration ECS et Polish

---

## 📈 Progression par Composant

| Composant | État | Avancement | Notes |
|-----------|------|------------|-------|
| **Infrastructure CI/CD** | ✅ Terminé | 100% | Pipeline complet |
| **Documentation** | 🚧 En cours | 90% | 81 pages, mise à jour en cours |
| **Architecture Hexagonale** | ✅ Terminé | 100% | 3 couches complètes |
| ├─ Séparation .hpp/.cpp | ✅ Terminé | 100% | 110+ fichiers |
| └─ Serveurs Réseau | ✅ Terminé | 100% | UDP 4124 + TCP/TLS 4125 |
| **Domain Layer** | ✅ Terminé | 100% | Entités et Value Objects |
| ├─ Entité Player | ✅ Terminé | 100% | Position, Health, PlayerId |
| ├─ Entité User | ✅ Terminé | 100% | Auth (Username, Email, Password) |
| ├─ Value Objects | ✅ Terminé | 100% | 10 Value Objects (incl. Email) |
| └─ Exceptions | ✅ Terminé | 100% | 15 exceptions métier |
| **Application Layer** | ✅ Terminé | 100% | Use Cases |
| ├─ PlayerInput | ✅ Terminé | 100% | Commandes joueur (via GameWorld) |
| ├─ LoginUseCase | ✅ Terminé | 100% | Authentification par email |
| └─ RegisterUseCase | ✅ Terminé | 100% | Inscription utilisateur |
| **Infrastructure Layer** | ✅ Terminé | 95% | Adapters |
| ├─ UDPServer | ✅ Terminé | 100% | Port 4124, broadcast 20Hz |
| ├─ GameWorld | ✅ Terminé | 100% | Joueurs, missiles, ennemis, collisions |
| ├─ Protocol | ✅ Terminé | 100% | 62 types de messages |
| ├─ Collision System | ✅ Terminé | 100% | AABB hitboxes |
| └─ VoiceUDPServer | ✅ Terminé | 100% | Port 4126, relay Opus |
| **Client Graphique** | ✅ Terminé | 90% | Multi-backend SDL2/SFML |
| ├─ Boot/Engine/GameLoop | ✅ Terminé | 100% | Architecture complète, 60 FPS |
| ├─ SceneManager | ✅ Terminé | 100% | GameScene complet |
| ├─ UDPClient | ✅ Terminé | 100% | Thread-safe, async |
| ├─ SDL2/SFML Backends | ✅ Terminé | 100% | Plugins dynamiques |
| ├─ AudioManager | ✅ Terminé | 100% | SDL2_mixer, musique + SFX |
| ├─ VoiceChatManager | ✅ Terminé | 100% | Opus + PortAudio, PTT/VAD |
| └─ AccessibilityConfig | ✅ Terminé | 100% | Remapping, daltonisme |
| **Module Gameplay** | ✅ Terminé | 95% | Complet côté serveur et client |
| ├─ GameScene | ✅ Terminé | 100% | HUD, missiles, ennemis, stars |
| ├─ 5 Types d'Ennemis | ✅ Terminé | 100% | Basic, Tracker, Zigzag, Fast, Bomber |
| ├─ Wave Spawning | ✅ Terminé | 100% | 6-12s, 2-6 ennemis |
| └─ EntityManager | ✅ Terminé | 100% | Template spawn<T>() |
| **Blob-ECS Library** | ✅ Terminé | 100% | 51.3M ops/s, non intégré |

### Légende des États

- ✅ **Terminé** - Implémenté, compile et testé
- 🚧 **En cours** - Développement actif
- 📋 **Planifié** - Conception faite, développement à venir
- 💡 **Idée** - Concept en exploration

---

## 💻 Statistiques Code

### Lignes de Code (Mise à jour 15/12/2025)

| Catégorie | Fichiers | Lignes | Pourcentage |
|-----------|----------|--------|-------------|
| **Code Source Serveur** | 106 | ~7,500 | 38% |
| ├─ Headers (.hpp) | 62 | ~3,500 | 18% |
| └─ Implémentations (.cpp) | 44 | ~4,000 | 20% |
| **Code Source Client** | 93 | ~6,500 | 32% |
| ├─ Headers (.hpp) | 56 | ~3,000 | 15% |
| └─ Implémentations (.cpp) | 37 | ~3,500 | 17% |
| **Code Common** | 2 | ~600 | 4% |
| **Blob-ECS Library** | 6 | ~800 | 5% |
| **Documentation** | 81 | ~8,000 | 28% |
| **Total Projet** | 207+ | ~20,000 | 100% |

**Ratio Documentation/Code:** 0.8:1

### Architecture du Code Source

```
src/
├── server/                          # Serveur de jeu (106 fichiers)
│   ├── domain/
│   │   ├── entities/Player.hpp/.cpp ✅
│   │   ├── value_objects/ ✅ (Health, Position, PlayerId, etc.)
│   │   ├── services/GameRule.hpp ✅
│   │   └── exceptions/ ✅ (15 exceptions métier)
│   ├── application/
│   │   ├── use_cases/ (Login, Register) ✅
│   │   └── ports/out/ ✅ (IUserRepository, IUserSettingsRepository, IChatMessageRepository, IIdGenerator, ILogger)
│   └── infrastructure/
│       ├── game/
│       │   └── GameWorld.hpp/.cpp ✅  # Joueurs, missiles, ennemis, collisions
│       ├── adapters/in/network/
│       │   ├── UDPServer.hpp/.cpp ✅  # Port 4124, broadcast 20Hz
│       │   └── protocol/CommandParser.hpp ✅
│       ├── logging/Logger.hpp/.cpp ✅
│       └── bootstrap/GameBootstrap.hpp ✅
│
├── client/                          # Client de jeu (93 fichiers)
│   ├── include/
│   │   ├── core/ (Engine, GameLoop, Logger, DynamicLib)
│   │   ├── graphics/ (IWindow, IDrawable, Graphics, Asset)
│   │   ├── scenes/ (IScene, GameScene, SceneManager)
│   │   ├── network/ (UDPClient) ✅
│   │   ├── gameplay/ (EntityManager, GameObject, Missile)
│   │   ├── audio/ (AudioManager - SDL2_mixer) ✅
│   │   ├── accessibility/ (AccessibilityConfig) ✅
│   │   └── events/ (Event, Signal)
│   ├── lib/
│   │   ├── sfml/ (SFMLWindow, SFMLRenderer, SFMLPlugin)
│   │   └── sdl2/ (SDL2Window, SDL2Renderer, SDL2Plugin) ✅ défaut
│   └── main.cpp ✅
│
├── common/                          # Code partagé (2 fichiers)
│   ├── protocol/Protocol.hpp ✅     # 62 types de messages
│   └── collision/AABB.hpp ✅        # Hitboxes
│
└── ECS/                             # Blob-ECS (6 fichiers, non intégré)
    ├── ECS.hpp ✅
    ├── Registry.hpp ✅
    ├── Component.hpp ✅
    ├── System.hpp ✅
    ├── Errors.hpp ✅
    └── Includes.hpp ✅
```

### Dépendances Utilisées

| Dépendance | Version | Statut | Utilisation |
|------------|---------|--------|-------------|
| Boost.ASIO | Via vcpkg | ✅ Utilisé | UDPServer, UDPClient (async I/O) |
| SDL2 | Via vcpkg | ✅ Utilisé | Backend graphique par défaut |
| SDL2_image | Via vcpkg | ✅ Utilisé | Chargement textures (PNG, etc.) |
| SDL2_mixer | Via vcpkg | ✅ Utilisé | AudioManager (musique, SFX) |
| SDL2_ttf | Via vcpkg | ✅ Utilisé | Rendu de texte (fonts) |
| SFML 3.0 | Via vcpkg | ✅ Utilisé | Backend graphique alternatif |
| spdlog | Via vcpkg | ✅ Utilisé | Logging (12 loggers) |
| MongoDB C++ Driver | Via vcpkg | ✅ Utilisé | MongoDBConfiguration (auth) |
| Google Test | Via vcpkg | ✅ Installé | Tests unitaires |

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
- ✅ UDPServer sur port 4124 avec broadcast 20Hz
- ✅ GameWorld complet (joueurs, missiles, ennemis, collisions)
- ✅ 5 types d'ennemis avec IA unique (Basic, Tracker, Zigzag, Fast, Bomber)
- ✅ Wave spawning automatique (6-12s, 2-6 ennemis)
- ✅ Système de collision AABB avec damage events
- ✅ Protocol binaire - 62 types de messages
- ✅ Architecture hexagonale respectée
- ✅ Système de logging (6 loggers serveur)

### Client
- ✅ Client multi-backend (SDL2 par défaut, SFML alternatif)
- ✅ Architecture modulaire (Boot, Engine, GameLoop)
- ✅ UDPClient thread-safe (Boost.ASIO)
- ✅ GameScene complet (HUD, missiles, ennemis, parallax stars)
- ✅ AudioManager (SDL2_mixer) - musique + effets sonores
- ✅ AccessibilityConfig (remapping clavier, modes daltonien)
- ✅ EntityManager avec template spawn<T>()
- ✅ Event system (std::variant)
- ✅ Death screen et health bar HUD
- ✅ Système de logging (6 loggers client)

### Blob-ECS Library
- ✅ Sparse set architecture (51.3M ops/s)
- ✅ Entity/Component/System framework complet
- ✅ Type-safe avec C++20 concepts
- ✅ Documentation complète

**Ce qui reste à faire:**
- 🚧 Intégration Blob-ECS dans gameplay
- 📋 UI Components avancés (menus, lobby)
- 📋 Power-ups et bonus
- 📋 Niveaux et progression
- 📋 Matchmaking et lobby
- 📋 Polish et optimisations

---

**Dernière révision:** 15/12/2025
**Prochaine mise à jour:** Après intégration ECS
