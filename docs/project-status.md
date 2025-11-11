# État du Projet R-Type

**Dernière mise à jour:** 11 janvier 2025
**Version:** 0.2.0 (Phase: Core Engine - Architecture)

---

## 📊 Vue d'Ensemble

Le projet R-Type est actuellement en phase de **développement du Core Engine**. L'architecture hexagonale est complète avec séparation propre .hpp/.cpp, et les fondations réseau sont implémentées.

### Phase Actuelle

🚧 **Phase 2: Core Engine - Architecture** (60% complété)

Nous avons construit une architecture solide avec:
- Architecture hexagonale complète (Domain/Application/Infrastructure)
- Séparation .hpp/.cpp (18 fichiers)
- Serveurs réseau UDPServer et TCPServer (Boost.Asio)
- Entités et Value Objects du domaine
- Système d'authentification (entité User)

**Prochaine phase:** Gameplay et ECS

---

## 📈 Progression par Composant

| Composant | État | Avancement | Ticket Linear | Notes |
|-----------|------|------------|---------------|-------|
| **Infrastructure CI/CD** | ✅ Terminé | 100% | [EPI-9](https://linear.app/epitech-study/issue/EPI-9) | Pipeline complet |
| **Documentation** | 🚧 En cours | 75% | [EPI-15](https://linear.app/epitech-study/issue/EPI-15) | Mise à jour en cours |
| **Architecture Hexagonale** | ✅ Terminé | 100% | [EPI-22](https://linear.app/epitech-study/issue/EPI-22) | 3 couches complètes |
| ├─ Séparation .hpp/.cpp | ✅ Terminé | 100% | [EPI-23](https://linear.app/epitech-study/issue/EPI-23) | 18 fichiers .cpp |
| └─ Serveurs Réseau | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | UDP + TCP |
| **Domain Layer** | 🚧 En cours | 70% | - | Entités et Value Objects |
| ├─ Entité Player | ✅ Terminé | 100% | - | Position, Health, PlayerId |
| ├─ Entité User | ✅ Terminé | 100% | - | Auth (Username, Password) |
| ├─ Value Objects | ✅ Terminé | 100% | - | 8 Value Objects |
| └─ Exceptions | ✅ Terminé | 100% | - | 10 Exceptions métier |
| **Application Layer** | 🚧 En cours | 50% | - | Use Cases |
| ├─ MovePlayerUseCase | ✅ Terminé | 100% | - | Déplacement joueur |
| ├─ LoginUseCase | 🚧 En cours | 60% | - | Authentification utilisateur |
| └─ RegisterUseCase | 📋 Planifié | 0% | - | Inscription utilisateur |
| **Infrastructure Layer** | 🚧 En cours | 50% | - | Adapters |
| ├─ UDPServer | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | Port 4123 |
| ├─ TCPServer | ✅ Terminé | 100% | [EPI-24](https://linear.app/epitech-study/issue/EPI-24) | Port 4123 |
| ├─ MongoDBConfiguration | ✅ Terminé | 100% | - | Connexion DB |
| └─ Repositories | 🚧 En cours | 40% | - | IPlayerRepository, IUserRepository |
| **Architecture ECS** | 📋 Planifié | 0% | [EPI-17](https://linear.app/epitech-study/issue/EPI-17) | À démarrer |
| **Client graphique** | 📋 Planifié | 0% | [EPI-11](https://linear.app/epitech-study/issue/EPI-11) | À démarrer |

### Légende des États

- ✅ **Terminé** - Implémenté, compile et testé
- 🚧 **En cours** - Développement actif
- 📋 **Planifié** - Conception faite, développement à venir
- 💡 **Idée** - Concept en exploration

---

## 💻 Statistiques Code

### Lignes de Code (Mise à jour 11/01/2025)

| Catégorie | Fichiers | Lignes | Pourcentage |
|-----------|----------|--------|-------------|
| **Code Source Serveur** | 46 | ~2,500 | 45% |
| ├─ Headers (.hpp) | 23 | ~1,200 | 22% |
| └─ Implémentations (.cpp) | 23 | ~1,300 | 23% |
| **Documentation** | 12 | ~3,200 | 55% |
| **Tests** | 2 | 50 | <1% |
| **Total Projet** | 60+ | ~5,750 | 100% |

**Ratio Documentation/Code:** 1.3:1 (normalisé)

### Architecture du Code Source

```
src/server/
├── domain/
│   ├── entities/
│   │   ├── Player.hpp/.cpp ✅
│   │   └── User.hpp/.cpp ✅ (nouveau)
│   ├── value_objects/
│   │   ├── Health.hpp/.cpp ✅
│   │   ├── Position.hpp/.cpp ✅
│   │   ├── player/
│   │   │   └── PlayerId.hpp/.cpp ✅
│   │   └── user/ ✅ (nouveau)
│   │       ├── UserId.hpp/.cpp
│   │       ├── Username.hpp/.cpp
│   │       └── Password.hpp/.cpp
│   ├── exceptions/
│   │   ├── DomainException.hpp/.cpp ✅
│   │   ├── HealthException.hpp/.cpp ✅
│   │   ├── PositionException.hpp/.cpp ✅
│   │   ├── player/
│   │   │   └── PlayerIdException.hpp/.cpp ✅
│   │   └── user/ ✅ (nouveau)
│   │       ├── UserIdException.hpp/.cpp
│   │       ├── UsernameException.hpp/.cpp
│   │       └── PasswordException.hpp/.cpp
│   └── services/
│       └── GameRule.hpp (placeholder)
├── application/
│   ├── use_cases/
│   │   └── MovePlayerUseCase.hpp/.cpp ✅
│   ├── ports/
│   │   ├── in/
│   │   │   └── IGameCommands.hpp ✅
│   │   └── out/
│   │       └── persistence/
│   │           └── IPlayerRepository.hpp ✅
│   └── dto/
│       └── PlayerDTO.hpp ✅
├── infrastructure/
│   ├── adapters/
│   │   ├── in/
│   │   │   ├── cli/
│   │   │   │   └── CLIGameController.hpp/.cpp ✅
│   │   │   └── network/ ✅ (nouveau)
│   │   │       ├── UDPServer.hpp/.cpp
│   │   │       └── TCPServer.hpp/.cpp
│   │   └── out/
│   │       └── persistence/
│   │           ├── MongoDBConfiguration.hpp/.cpp ✅
│   │           └── MongoDBPlayerRepository.hpp 🚧
│   └── configuration/
│       └── DBConfig.hpp ✅
└── main.cpp ✅ (intégré avec UDPServer)
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

### 🚧 Jalon 3: Core Engine (En cours - Jan-Fév 2025)

**Objectif:** Développer le moteur de jeu de base
**Date estimée:** Janvier - Février 2025
**Statut:** 🚧 **35% complété**

**Tâches principales:**
- 🚧 Système d'authentification User (75%)
  - ✅ Entité User complète
  - ✅ IUserRepository (interface)
  - 🚧 LoginUseCase (en cours)
  - 📋 RegisterUseCase (à faire)
  - 📋 MongoDBUserRepository (à faire)
- 📋 Architecture ECS (Entity Component System) (0%)
- 📋 Protocole réseau R-Type (0%)
- 📋 Game loop serveur (0%)
- 📋 Gestion des sessions de jeu (0%)

**Tickets Linear:**
- [EPI-17: Architecture ECS](https://linear.app/epitech-study/issue/EPI-17) - Priorité Haute
- À créer: Ticket pour finaliser LoginUseCase
- À créer: Protocole réseau R-Type

### 📋 Jalon 4: Gameplay Basique (Planifié - Mars 2025)

**Objectif:** Gameplay R-Type fonctionnel
**Date estimée:** Mars 2025
**Statut:** 📋 **Planifié (0%)**

**Tâches principales:**
- Vaisseau joueur contrôlable
- Système de tir
- Ennemis basiques avec patterns
- Système de collisions
- Power-ups

### 📋 Jalon 5: Client Graphique (Planifié - Avril 2025)

**Objectif:** Interface graphique jouable
**Date estimée:** Avril 2025
**Statut:** 📋 **Planifié (0%)**

**Tâches principales:**
- Rendu graphique SFML
- Gestion des inputs
- UI et menus
- Système audio
- Assets graphiques

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

1. **Documentation réseau** - Créer `docs/guides/network-architecture.md`
   - Documenter UDPServer et TCPServer
   - Diagrammes de flux
   - Exemples d'utilisation

2. **Documentation API** - Créer `docs/api/domain.md` et `docs/api/adapters.md`
   - Référence complète des entités
   - Value Objects et Exceptions
   - Adapters réseau

3. **Use Cases Auth** - Implémenter LoginUserUseCase et RegisterUserUseCase
   - Utiliser l'entité User
   - Intégrer avec TCPServer

### Semaines Suivantes (Priorité Haute)

4. **[EPI-17](https://linear.app/epitech-study/issue/EPI-17)** - Architecture ECS
   - Entity Manager
   - Components (Position, Velocity, Sprite, Collider)
   - Systems (Movement, Rendering, Collision)
   - Estimation: 2 semaines

5. **Protocole Réseau R-Type**
   - Définir format messages UDP (positions, actions)
   - Définir format messages TCP (auth, chat)
   - Sérialisation/désérialisation
   - Estimation: 1 semaine

### Plus Tard

6. **Game Loop** - Boucle principale du serveur
7. **Gameplay** - Ennemis, tirs, collisions
8. **Client** - Interface graphique SFML
9. **Tests** - Couverture complète
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
- ✅ Build automatisé avec CMake
- ✅ Pipeline Jenkins fonctionnel
- ✅ Serveur UDP écoute sur port 4123
- ✅ Serveur TCP accepte connexions
- ✅ Entités Player et User complètes
- ✅ Value Objects avec validation
- ✅ Exceptions métier
- ✅ MongoDB configuré et prêt
- ✅ Architecture hexagonale respectée

**Ce qui reste à faire:**
- 🚧 Documentation réseau à créer
- 🚧 Use Cases d'authentification
- 📋 Architecture ECS
- 📋 Protocole réseau R-Type
- 📋 Game loop serveur
- 📋 Client graphique
- 📋 Gameplay complet

---

**Dernière révision:** 11/01/2025 par la Reine des Abeilles 👑
**Prochaine mise à jour:** Après création documentation réseau et API
