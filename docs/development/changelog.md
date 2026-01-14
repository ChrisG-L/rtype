# Changelog

Toutes les modifications notables du projet R-Type sont documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

---

## [0.4.0] - 2025-11-30 {#040---2025-11-30}

### ✨ Ajouté

#### Protocole Binaire Client-Serveur
- **Protocol.hpp** - Nouveau module partagé (`src/common/protocol/`)
  - Énumération `MessageType` (HeartBeat, Login, Register, Ack)
  - Structure `Header` (6 bytes: type uint16 + payload_size uint32)
  - Sérialisation network byte order (bswap16/32)
  - Fichier : `src/common/protocol/Protocol.hpp`

- **Messages d'Authentification Binaires**
  - `LoginMessage` (287 bytes: username[32] + password[255])
  - `RegisterMessage` (542 bytes: username[32] + email[255] + password[255])
  - Méthodes `to_bytes()` et `from_bytes()` pour chaque message

#### Client TCP avec Protocole Binaire
- **TCPClient** refactorisé pour protocole binaire
  - Accumulator pour buffering des messages partiels
  - Méthodes `sendLoginData()` et `sendRegisterData()` avec paramètres
  - Setters `setLoginCredentials()` et `setRegisterCredentials()`
  - Parsing header et routing par `MessageType`
  - Fichiers : `src/client/include/network/TCPClient.hpp`, `src/client/src/network/TCPClient.cpp`

### 🐛 Corrigé

#### Sécurité (P0)
- **Register.cpp** - Correction logique détection doublons
  - `!playerOptByName.has_value()` → `playerOptByName.has_value()`
  - `!playerOptByEmail.has_value()` → `playerOptByEmail.has_value()`
  - Prévient les registrations dupliquées

### 🔄 Modifié

#### Serveur TCP Binaire
- **Command struct** refactorisée
  - `type` : `string` → `uint16_t`
  - `args` : `vector<string>` → `buf` : `vector<uint8_t>`
- **TCPServer** avec accumulator et sérialisation binaire
- **ExecuteAuth** désérialise `LoginMessage`/`RegisterMessage`

#### Use Case Login
- Paramètre `email` → `username` pour correspondre au protocole

### 🔧 Build & Environnement

- **CMakeLists.txt** : Include path `src/common/protocol` (client + serveur)
- **flake.nix** : Fix clang-tools (`llvmPackages.clang-tools`)
- **Environment X11/Wayland** : `XLIB_SKIP_ARGB_VISUALS=1`
- **OpenGL NVIDIA** : LD_LIBRARY_PATH dans `.envrc`
- **vcpkg** : Mise à jour sous-module

### 📚 Documentation

- **Architecture Réseau** (`docs/guides/network-architecture.md`)
  - Format messages TCP : JSON planifié → Binaire implémenté
  - Documentation Header/LoginMessage/RegisterMessage
  - Mise à jour statut et version (v0.3.0)

---

## [0.3.0] - 2025-01-17 {#030---2025-01-17}

### ✨ Ajouté

#### Système d'Authentification Complet
- **Value Object Email** avec validation RFC 5322 complète
  - Validation regex stricte des adresses email
  - Longueur maximale de 254 caractères
  - Exception `EmailException` pour emails invalides
  - Fichiers : `domain/value_objects/user/Email.{hpp,cpp}`, `domain/exceptions/user/EmailException.{hpp,cpp}`

- **Utilitaires Password (PasswordUtils)**
  - Extraction de `hashPassword()` vers un module utils réutilisable
  - Hachage SHA-256 avec OpenSSL
  - Fichiers : `domain/value_objects/user/utils/PasswordUtils.{hpp,cpp}`

- **Use Case Register**
  - Inscription de nouveaux utilisateurs
  - Validation de l'unicité username et email
  - Hachage automatique des mots de passe
  - Génération d'ObjectId MongoDB
  - Fichiers : `application/use_cases/auth/Register.{hpp,cpp}`

- **Gestionnaire de Commandes AUTH (ExecuteAuth)**
  - Routing des commandes LOGIN et REGISTER
  - Parsing des arguments réseau
  - Intégration avec les use cases Login/Register
  - Fichiers : `infrastructure/adapters/in/network/execute/auth/ExecuteAuth.{hpp,cpp}`

- **Repository MongoDB Complet**
  - Implémentation complète de `MongoDBUserRepository`
  - Méthodes : `save`, `update`, `findById`, `findByName`, `findByEmail`, `findAll`
  - Conversions BSON ↔ Domain entities
  - Helper `timePointToDate` pour conversion des timestamps
  - Vérification d'unicité username/email dans `save()`

### 🔄 Modifié

#### Entité User
- Ajout du champ `_email` de type `Email`
- Réorganisation des paramètres du constructeur (lastLogin avant createdAt)
- Ajout du getter `getEmail()`
- Utilisation de using declarations pour simplifier le code
- Fichiers : `domain/entities/User.{hpp,cpp}`

#### Value Object Password
- Suppression de la méthode statique `hashPassword()` (déplacée vers PasswordUtils)
- Ajout de la méthode `verify(password)` pour vérification
- Utilisation de `utils::hashPassword` dans `verify()`
- Fichier : `domain/value_objects/user/Password.cpp`

#### Use Case Login
- Remplacement du paramètre `name` par `email`
- Utilisation de `findByEmail()` au lieu de `findByName()`
- Passage de pointeur brut à `shared_ptr` pour IUserRespository
- Renommage `repository` → `_userRespository`
- Correction du formatage (accolades manquantes)
- Fichiers : `application/use_cases/auth/Login.{hpp,cpp}`

#### Infrastructure Réseau
- **TCPServer et Session** : Ajout de `MongoDBUserRepository` comme dépendance
- **Execute** : Instanciation des use cases Login/Register et délégation à ExecuteAuth
- **MongoDBConfiguration** : Ajout de l'include `<bsoncxx/oid.hpp>`
- Fichiers : `infrastructure/adapters/in/network/{TCPServer,Execute}.{hpp,cpp}`

#### Interface Repository
- Ajout de la signature `findByEmail(const std::string& email)`
- Fichier : `application/ports/out/persistence/IUserRespository.hpp`

### 🔧 Build & CI/CD

- **CMakeLists.txt** : Ajout de tous les nouveaux fichiers sources
  - `EmailException.cpp`, `Email.cpp`
  - `PasswordUtils.cpp`
  - `Login.cpp`, `Register.cpp`
  - `Execute.cpp`, `ExecuteAuth.cpp`
  - `MongoDBUserRepository.cpp`

- **vcpkg** : Mise à jour du sous-module vers version plus récente

### 📚 Documentation

- **Guide Authentification** (`docs/guides/authentication.md`)
  - Documentation complète du système d'authentification
  - Flux d'inscription et de connexion avec diagrammes Mermaid
  - Exemples d'utilisation détaillés
  - Analyse de sécurité et recommandations
  - Tests unitaires et d'intégration recommandés

- **API Domain** (`docs/api/domain.md`)
  - Mise à jour de l'entité User avec champ Email
  - Documentation du value object Email
  - Documentation de PasswordUtils
  - Mise à jour de la hiérarchie d'exceptions

- **Changelog** (`docs/development/changelog.md`)
  - Création du fichier changelog suivant Keep a Changelog

### ⚠️ Notes de Sécurité

**Vulnérabilités Connues** (à corriger en priorité) :

1. **SHA-256 pour mots de passe** - Algorithme rapide, vulnérable au brute-force
   - ⚠️ Recommandé : Migrer vers Argon2id ou bcrypt

2. **Pas de rate limiting** - Attaques par force brute possibles
   - ⚠️ Recommandé : Limiter tentatives login (ex: 5/minute)

3. **Pas de gestion de sessions** - Aucun token après authentification
   - ⚠️ Recommandé : Implémenter JWT ou sessions Redis

4. **Messages d'erreur verbeux** - Permet énumération d'emails
   - ⚠️ Recommandé : Message générique "Invalid credentials"

5. **Communication TCP non chiffrée** - Mots de passe en clair sur réseau
   - ⚠️ Recommandé : TLS/SSL obligatoire

6. **Pas de politique de mots de passe** - Accepte mots de passe faibles
   - ⚠️ Recommandé : Longueur min + complexité requise

### 🔗 Commits Git

```
5018963 BUILD: mise à jour du sous-module vcpkg
1115463 BUILD: ajout des nouveaux fichiers d'authentification au CMake
37719d0 REFACTOR: intégration du repository dans le serveur TCP
ecb8e8a FEAT: ajout du gestionnaire de commandes d'authentification
fd1fcb5 REFACTOR: modification du use case Login pour utiliser l'email
f52d4b5 FEAT: ajout du use case Register pour l'inscription utilisateur
45b8312 FEAT: implémentation complète du MongoDBUserRepository
09ded81 REFACTOR: ajout du champ email à l'entité User
5f0094c FEAT: ajout du value object Email avec validation
3d35cb7 REFACTOR: extraction de la fonction de hachage vers PasswordUtils
```

---

## [0.2.0] - 2025-01-11 {#020---2025-01-11}

### ✨ Ajouté

#### Architecture Hexagonale
- Séparation claire Domain / Application / Infrastructure
- Domain Layer avec entités Player et User
- Value Objects : Health, Position, PlayerId, UserId, Username, Password
- Exceptions métier personnalisées

#### Infrastructure Réseau
- **UDPServer** : Serveur UDP asynchrone (port 4124)
- **TCPAuthServer** : Serveur TCP/TLS asynchrone (port 4125)
- **Session** : Gestion des connexions TCP
- Boost.Asio avec event loop io_context

#### Base de Données
- **MongoDBConfiguration** : Connexion MongoDB (bsoncxx/mongocxx)
- **DBConfig** : Configuration centralisée

#### Build System
- Séparation .hpp/.cpp (46 fichiers)
- Compilation incrémentale (~15s vs ~45s)
- vcpkg pour gestion des dépendances

#### Documentation
- MkDocs Material avec thème personnalisé
- Guides : Architecture, Hexagonal Architecture, C++ Header/Implementation
- API Reference : Domain Layer, Adapters Layer
- CI/CD : Jenkins avec Docker

### 🔧 Build & CI/CD

- Configuration CMake modulaire
- Dockerfile.build pour environnement de build
- Dockerfile.docs pour génération documentation
- Docker Compose pour Jenkins CI/CD

---

## [0.1.0] - 2024-12-XX

### ✨ Ajouté

- Projet initial R-Type
- Structure de base C++
- Intégration vcpkg

---

## Format des Entrées

### Types de Changements

- **✨ Ajouté** : Nouvelles fonctionnalités
- **🔄 Modifié** : Changements dans des fonctionnalités existantes
- **❌ Supprimé** : Fonctionnalités retirées
- **🐛 Corrigé** : Corrections de bugs
- **🔒 Sécurité** : Corrections de vulnérabilités
- **⚡ Performance** : Améliorations de performance
- **📚 Documentation** : Modifications de la documentation
- **🔧 Build & CI/CD** : Changements dans le build ou CI/CD
- **♻️ Refactoring** : Refactoring de code sans changement fonctionnel

### Liens Utiles

- [Guide de Contribution](contributing.md)
- [Politique de Commits](COMMIT_POLICES.md)
- [Architecture du Projet](../guides/architecture.md)
