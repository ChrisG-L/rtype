# API Reference - Domain Layer

**État:** ✅ Implémenté et Documenté
**Version:** 0.3.0
**Dernière mise à jour:** 17 janvier 2025

---

## 📋 Vue d'Ensemble

Le Domain Layer contient la logique métier pure du R-Type, sans dépendances externes. Il est composé de:

- **Entités** - Objets métier avec identité (Player, User)
- **Value Objects** - Objets immuables avec validation (Health, Position, PlayerId, etc.)
- **Exceptions** - Exceptions métier pour validation et erreurs de domaine
- **Services** - Règles métier complexes (GameRule) [Planifié]

**Architecture:**
```
domain/
├── entities/           # Entités avec identité
│   ├── Player          # Joueur en partie (gameplay)
│   └── User            # Utilisateur authentifié (auth)
├── value_objects/      # Value Objects immuables
│   ├── Health          # Points de vie
│   ├── Position        # Coordonnées 3D
│   ├── player/         # VOs spécifiques Player
│   │   └── PlayerId    # Identifiant unique Player
│   └── user/           # VOs spécifiques User
│       ├── UserId      # Identifiant unique User
│       ├── Username    # Nom d'utilisateur
│       ├── Email       # Adresse email validée
│       ├── Password    # Hash du mot de passe
│       └── utils/      # Utilitaires
│           └── PasswordUtils  # Fonctions de hachage
└── exceptions/         # Exceptions métier
    ├── DomainException # Exception de base
    ├── player/         # Exceptions Player
    └── user/           # Exceptions User
```

---

## 🎮 Entités

### Player

**Fichier:** `domain/entities/Player.hpp`
**Namespace:** `domain::entities`

**Description:** Représente un joueur en partie (gameplay). Contient les informations de jeu temps réel (position, santé).

#### Attributs

| Attribut | Type | Description |
|----------|------|-------------|
| `_health` | `value_objects::Health` | Points de vie du joueur |
| `_id` | `value_objects::player::PlayerId` | Identifiant unique (UUID) |
| `_position` | `value_objects::Position` | Position 3D dans le jeu |

#### Constructeur

```cpp
explicit Player(
    value_objects::Health health,
    value_objects::player::PlayerId id,
    value_objects::Position position = value_objects::Position()
);
```

**Paramètres:**
- `health` - Points de vie initiaux (doit être >= 0)
- `id` - Identifiant unique (UUID MongoDB valide)
- `position` - Position initiale (défaut: 0, 0, 0)

#### Méthodes

**Getters:**
```cpp
const value_objects::player::PlayerId& getId() const;
const value_objects::Position& getPosition() const;
```

**Actions:**
```cpp
void move(float dx, float dy, float dz);
void heal(float value);
void takeDamage(float value);
```

| Méthode | Paramètres | Description | Exceptions |
|---------|------------|-------------|------------|
| `move()` | `dx, dy, dz` | Déplace le joueur relativement | `PositionException` si invalide |
| `heal()` | `value` | Soigne le joueur (ajoute HP) | `HealthException` si value < 0 |
| `takeDamage()` | `value` | Inflige des dégâts (retire HP) | `HealthException` si value < 0 |

#### Exemple d'Utilisation

```cpp
#include "domain/entities/Player.hpp"
#include "domain/value_objects/Health.hpp"
#include "domain/value_objects/player/PlayerId.hpp"
#include "domain/value_objects/Position.hpp"

using namespace domain::entities;
using namespace domain::value_objects;

// Créer un joueur
Player player(
    Health(100.0f),                              // 100 HP
    player::PlayerId("507f1f77bcf86cd799439011"), // UUID MongoDB
    Position(0.0f, 0.0f, 0.0f)                   // Spawn au centre
);

// Déplacer le joueur
player.move(10.0f, 5.0f, 0.0f);  // Déplace de (10, 5, 0)

// Combat
player.takeDamage(25.0f);  // -25 HP
player.heal(10.0f);         // +10 HP
```

---

### User

**Fichier:** `domain/entities/User.hpp`
**Namespace:** `domain::entities`

**Description:** Représente un utilisateur authentifié. Contient les informations d'authentification et de profil.

#### Attributs

| Attribut | Type | Description |
|----------|------|-------------|
| `_id` | `value_objects::user::UserId` | Identifiant unique (ObjectId MongoDB) |
| `_username` | `value_objects::user::Username` | Nom d'utilisateur unique (6-21 chars) |
| `_email` | `value_objects::user::Email` | Adresse email unique validée RFC 5322 |
| `_passwordHash` | `value_objects::user::Password` | Hash du mot de passe (SHA-256) |
| `_lastLogin` | `std::chrono::system_clock::time_point` | Date de dernière connexion |
| `_createdAt` | `std::chrono::system_clock::time_point` | Date de création du compte |

#### Constructeur

```cpp
explicit User(
    value_objects::user::UserId id,
    value_objects::user::Username username,
    value_objects::user::Email email,
    value_objects::user::Password passwordHash,
    std::chrono::system_clock::time_point lastLogin = std::chrono::system_clock::now(),
    std::chrono::system_clock::time_point createdAt = std::chrono::system_clock::now()
);
```

**Paramètres:**
- `id` - Identifiant unique ObjectId MongoDB (ne peut pas être vide)
- `username` - Nom d'utilisateur unique (6-21 caractères)
- `email` - Adresse email unique validée (format RFC 5322, max 254 chars)
- `passwordHash` - Hash du mot de passe (SHA-256, min 6 chars)
- `lastLogin` - Dernière connexion (défaut: maintenant)
- `createdAt` - Date de création (défaut: maintenant)

#### Méthodes

**Getters:**
```cpp
const value_objects::user::UserId& getId() const;
const value_objects::user::Username& getUsername() const;
const value_objects::user::Email& getEmail() const;
const value_objects::user::Password& getPasswordHash() const;
const std::chrono::system_clock::time_point& getLastLogin() const;
const std::chrono::system_clock::time_point& getCreatedAt() const;
```

**Authentification:**
```cpp
bool verifyPassword(const std::string& password) const;
void updateLastLogin();
```

| Méthode | Paramètres | Description | Retour |
|---------|------------|-------------|--------|
| `verifyPassword()` | `password` | Vérifie si le mot de passe correspond au hash | `true` si valide |
| `updateLastLogin()` | - | Met à jour `_lastLogin` à maintenant | `void` |

**Note:** `verifyPassword()` utilise SHA-256 pour comparer les hashs. ⚠️ Préférez Argon2/bcrypt en production.

#### Exemple d'Utilisation

```cpp
#include "domain/entities/User.hpp"
#include "domain/value_objects/user/UserId.hpp"
#include "domain/value_objects/user/Username.hpp"
#include "domain/value_objects/user/Email.hpp"
#include "domain/value_objects/user/Password.hpp"
#include "domain/value_objects/user/utils/PasswordUtils.hpp"

using namespace domain::entities;
using namespace domain::value_objects::user;

// Créer un utilisateur
User user(
    UserId(bsoncxx::oid().to_string()),       // ObjectId MongoDB
    Username("player123"),                     // 6-21 chars
    Email("player@example.com"),               // Email validé RFC 5322
    Password(utils::hashPassword("secret123")) // Hash SHA-256
);

// Vérifier authentification
if (user.verifyPassword("secret123")) {
    user.updateLastLogin();  // Met à jour timestamp
    std::cout << "Login successful!" << std::endl;
}

// Récupérer informations
std::cout << "User: " << user.getUsername().value() << std::endl;
std::cout << "Email: " << user.getEmail().value() << std::endl;
std::cout << "ID: " << user.getId().value() << std::endl;
```

---

## 🔷 Value Objects

### Health

**Fichier:** `domain/value_objects/Health.hpp`
**Namespace:** `domain::value_objects`

**Description:** Représente les points de vie d'une entité. Immuable avec validation.

#### Validation

- ✅ Valeur >= 0.0f
- ❌ Valeur < 0.0f → `HealthException`

#### Constructeur

```cpp
explicit Health(float value);
```

#### Méthodes

```cpp
float value() const;                    // Retourne la valeur
Health heal(float value) const;         // Retourne nouveau Health avec +value
Health damage(float value) const;       // Retourne nouveau Health avec -value
bool operator==(const Health& other);   // Comparaison égalité
bool operator<(const Health& other);    // Comparaison ordre
```

**Pattern Immuable:** Les méthodes `heal()` et `damage()` retournent un **nouveau** `Health`, l'original reste inchangé.

#### Exemple

```cpp
Health hp(100.0f);           // 100 HP
Health newHp = hp.heal(50);  // newHp = 150, hp = 100 (inchangé)
Health damaged = hp.damage(25); // damaged = 75, hp = 100

if (damaged < hp) {
    std::cout << "Player took damage!" << std::endl;
}
```

---

### Position

**Fichier:** `domain/value_objects/Position.hpp`
**Namespace:** `domain::value_objects`

**Description:** Coordonnées 3D dans l'espace de jeu. Immuable avec validation.

#### Validation

- ✅ x, y, z dans `[-1000.0f, 1000.0f]`
- ❌ x, y ou z hors limites → `PositionException`

#### Constructeur

```cpp
explicit Position(float x = 0.0f, float y = 0.0f, float z = 0.0f);
```

#### Méthodes

```cpp
float getX() const;
float getY() const;
float getZ() const;
Position move(float dx, float dy, float dz) const;  // Retourne nouvelle Position
bool operator==(const Position& other) const;
bool operator!=(const Position& other) const;
```

#### Exemple

```cpp
Position spawn(0.0f, 0.0f, 0.0f);         // Centre
Position moved = spawn.move(10.0f, 5.0f, 0.0f);  // (10, 5, 0)

std::cout << "X: " << moved.getX() << std::endl;  // X: 10
std::cout << "Y: " << moved.getY() << std::endl;  // Y: 5

if (moved != spawn) {
    std::cout << "Player moved!" << std::endl;
}
```

---

### PlayerId

**Fichier:** `domain/value_objects/player/PlayerId.hpp`
**Namespace:** `domain::value_objects::player`

**Description:** Identifiant unique d'un Player (UUID MongoDB). Immuable avec validation.

#### Validation

- ✅ Format UUID MongoDB valide (24 caractères hexadécimaux)
- ❌ Format invalide → `PlayerIdException`

**Implémentation:** Utilise `bsoncxx::oid::is_valid()` pour validation.

#### Constructeur

```cpp
explicit PlayerId(const std::string& id);
```

#### Méthodes

```cpp
std::string value() const;
bool operator==(const PlayerId& other);
bool operator!=(const PlayerId& other);
```

#### Exemple

```cpp
using namespace domain::value_objects::player;

// UUID MongoDB valide (24 hex chars)
PlayerId validId("507f1f77bcf86cd799439011");

// UUID invalide
try {
    PlayerId invalidId("invalid_uuid");
} catch (const exceptions::player::PlayerIdException& e) {
    std::cerr << e.what() << std::endl;
    // "Player ID must be a valid 24-character hexadecimal string: invalid_uuid."
}
```

---

### UserId

**Fichier:** `domain/value_objects/user/UserId.hpp`
**Namespace:** `domain::value_objects::user`

**Description:** Identifiant unique d'un User. Immuable avec validation.

#### Validation

- ✅ ID non vide
- ❌ ID vide → `UserIdException`

#### Constructeur

```cpp
explicit UserId(const std::string& id);
```

#### Méthodes

```cpp
std::string value() const;
bool operator==(const UserId& other);
bool operator!=(const UserId& other);
```

#### Exemple

```cpp
using namespace domain::value_objects::user;

UserId userId("507f1f77bcf86cd799439012");
std::cout << "User ID: " << userId.value() << std::endl;

// ID vide invalide
try {
    UserId emptyId("");
} catch (const exceptions::user::UserIdException& e) {
    std::cerr << e.what() << std::endl;
    // "User ID cannot be empty or invalid: ."
}
```

---

### Username

**Fichier:** `domain/value_objects/user/Username.hpp`
**Namespace:** `domain::value_objects::user`

**Description:** Nom d'utilisateur. Immuable avec validation.

#### Validation

- ✅ Longueur entre 6 et 21 caractères
- ❌ Longueur invalide → `UsernameException`

#### Constructeur

```cpp
explicit Username(const std::string& username);
```

#### Méthodes

```cpp
std::string value() const;
bool operator==(const Username& other);
bool operator!=(const Username& other);
```

#### Exemple

```cpp
using namespace domain::value_objects::user;

Username validUser("player123");  // OK (8 chars)

// Trop court
try {
    Username tooShort("abc");  // 3 chars < 6
} catch (const exceptions::user::UsernameException& e) {
    std::cerr << e.what() << std::endl;
    // "Username must be between 6 and 21 characters: abc."
}

// Trop long
try {
    Username tooLong("this_is_way_too_long_username");  // 30 chars > 21
} catch (const exceptions::user::UsernameException& e) {
    std::cerr << e.what() << std::endl;
}
```

---

### Email

**Fichier:** `domain/value_objects/user/Email.hpp`
**Namespace:** `domain::value_objects::user`

**Description:** Adresse email validée selon RFC 5322. Immuable avec validation stricte.

#### Validation

- ✅ Format email valide selon regex RFC 5322 complète
- ✅ Longueur maximum 254 caractères (standard email)
- ❌ Format invalide → `EmailException`
- ❌ Longueur > 254 → `EmailException`

**Pattern Regex:**
```
^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$
```

#### Constructeur

```cpp
explicit Email(const std::string& email);
```

#### Méthodes

```cpp
std::string value() const;
bool operator==(const Email& other) const;
bool operator!=(const Email& other) const;
```

#### Exemple

```cpp
using namespace domain::value_objects::user;

// Email valide
Email valid("user@example.com");
std::cout << valid.value() << std::endl;  // "user@example.com"

// Email avec caractères spéciaux (valide)
Email complex("user.name+tag@sub.example.co.uk");

// Formats invalides
try {
    Email invalid1("not-an-email");  // Pas de @
} catch (const exceptions::user::EmailException& e) {
    std::cerr << e.what() << std::endl;
    // "Invalid email format: not-an-email."
}

try {
    Email invalid2("user@");  // Domaine manquant
} catch (const exceptions::user::EmailException& e) {
    std::cerr << e.what() << std::endl;
}

try {
    std::string tooLong(250, 'a');
    tooLong += "@example.com";  // > 254 chars
    Email invalid3(tooLong);
} catch (const exceptions::user::EmailException& e) {
    std::cerr << e.what() << std::endl;
}
```

---

### Password

**Fichier:** `domain/value_objects/user/Password.hpp`
**Namespace:** `domain::value_objects::user`

**Description:** Hash du mot de passe. Immuable avec validation.

#### Validation

- ✅ Longueur minimum 6 caractères (hash)
- ❌ Longueur < 6 → `PasswordException`

**Note:** Stocke le **hash** du mot de passe, pas le mot de passe en clair.

#### Constructeur

```cpp
explicit Password(const std::string& password);
```

#### Méthodes

```cpp
std::string value() const;                      // Retourne le hash stocké
bool verify(std::string password);              // Vérifie un mot de passe en clair
```

**Note:** `verify()` hache le `password` en clair avec SHA-256 et compare au hash stocké.

#### Utilitaires: PasswordUtils

**Fichier:** `domain/value_objects/user/utils/PasswordUtils.hpp`
**Namespace:** `domain::value_objects::user::utils`

Fonctions pour hacher les mots de passe :

```cpp
namespace domain::value_objects::user::utils {
    std::string hashPassword(std::string password);  // Hache avec SHA-256
}
```

**Algorithme:** SHA-256 (OpenSSL)
⚠️ **Avertissement Sécurité:** SHA-256 n'est pas recommandé pour les mots de passe en production. Utilisez Argon2id, bcrypt ou scrypt.

#### Exemple

```cpp
using namespace domain::value_objects::user;

// Hacher un mot de passe
std::string hash = utils::hashPassword("my_secure_password");
Password passwordHash(hash);

// Vérifier un mot de passe
if (passwordHash.verify("my_secure_password")) {
    std::cout << "Password correct!" << std::endl;
} else {
    std::cout << "Wrong password!" << std::endl;
}

// Récupérer le hash pour stockage
std::string storedHash = passwordHash.value();

// Mot de passe trop court
try {
    Password weak("12345");  // 5 chars < 6
} catch (const exceptions::user::PasswordException& e) {
    std::cerr << e.what() << std::endl;
    // "Password must be at least 6 characters long."
}

// Exemple complet
std::string plaintextPassword = "user_password_123";
std::string hashedPassword = utils::hashPassword(plaintextPassword);
Password pwd(hashedPassword);

// Vérification
bool isValid = pwd.verify("user_password_123");  // true
bool isWrong = pwd.verify("wrong_password");     // false
```

---

## ⚠️ Exceptions

### Hiérarchie

```
std::exception
    └── DomainException (base)
        ├── HealthException
        ├── PositionException
        ├── PlayerIdException
        ├── UserIdException
        ├── UsernameException
        ├── EmailException
        └── PasswordException
```

### DomainException

**Fichier:** `domain/exceptions/DomainException.hpp`
**Namespace:** `domain::exceptions`

**Description:** Exception de base pour toutes les erreurs du Domain Layer.

```cpp
class DomainException: public std::exception {
    public:
        explicit DomainException(std::string message);
        const char* what() const noexcept override;
};
```

---

### HealthException

**Fichier:** `domain/exceptions/HealthException.hpp`
**Namespace:** `domain::exceptions`

**Lancée quand:** Valeur de santé invalide (< 0).

```cpp
// Exemple de déclenchement
Health hp(-10.0f);  // Lance HealthException
// Message: "Health point value cannot be negative: -10."
```

---

### PositionException

**Fichier:** `domain/exceptions/PositionException.hpp`
**Namespace:** `domain::exceptions`

**Lancée quand:** Coordonnées hors limites (< -1000 ou > 1000).

```cpp
// Exemple de déclenchement
Position pos(2000.0f, 0.0f, 0.0f);  // Lance PositionException
// Message: "Position coordinates must be between -1000 and 1000: x=2000, y=0, z=0."
```

---

### PlayerIdException

**Fichier:** `domain/exceptions/player/PlayerIdException.hpp`
**Namespace:** `domain::exceptions::player`

**Lancée quand:** UUID MongoDB invalide (pas 24 caractères hexadécimaux).

```cpp
// Exemple de déclenchement
PlayerId id("invalid");  // Lance PlayerIdException
// Message: "Player ID must be a valid 24-character hexadecimal string: invalid."
```

---

### UserIdException

**Fichier:** `domain/exceptions/user/UserIdException.hpp`
**Namespace:** `domain::exceptions::user`

**Lancée quand:** ID utilisateur vide ou invalide.

```cpp
// Exemple de déclenchement
UserId id("");  // Lance UserIdException
// Message: "User ID cannot be empty or invalid: ."
```

---

### UsernameException

**Fichier:** `domain/exceptions/user/UsernameException.hpp`
**Namespace:** `domain::exceptions::user`

**Lancée quand:** Nom d'utilisateur pas entre 6 et 21 caractères.

```cpp
// Exemple de déclenchement
Username name("abc");  // Lance UsernameException
// Message: "Username must be between 6 and 21 characters: abc."
```

---

### PasswordException

**Fichier:** `domain/exceptions/user/PasswordException.hpp`
**Namespace:** `domain::exceptions::user`

**Lancée quand:** Mot de passe (ou hash) moins de 6 caractères.

```cpp
// Exemple de déclenchement
Password pwd("12345");  // Lance PasswordException
// Message: "Password must be at least 6 characters long."
```

---

## 📊 Récapitulatif API

### Entités

| Classe | Namespace | Fichier | Attributs | Méthodes |
|--------|-----------|---------|-----------|----------|
| **Player** | `domain::entities` | `entities/Player.hpp` | `_health`, `_id`, `_position` | `move()`, `heal()`, `takeDamage()`, getters |
| **User** | `domain::entities` | `entities/User.hpp` | `_id`, `_username`, `_passwordHash`, `_createdAt`, `_lastLogin` | `verifyPassword()`, `updateLastLogin()`, getters |

### Value Objects

| Classe | Namespace | Fichier | Validation | Méthodes Clés |
|--------|-----------|---------|------------|---------------|
| **Health** | `domain::value_objects` | `value_objects/Health.hpp` | >= 0 | `heal()`, `damage()`, `value()` |
| **Position** | `domain::value_objects` | `value_objects/Position.hpp` | [-1000, 1000] | `move()`, `getX()`, `getY()`, `getZ()` |
| **PlayerId** | `domain::value_objects::player` | `value_objects/player/PlayerId.hpp` | UUID 24 hex | `value()` |
| **UserId** | `domain::value_objects::user` | `value_objects/user/UserId.hpp` | Non vide | `value()` |
| **Username** | `domain::value_objects::user` | `value_objects/user/Username.hpp` | 6-21 chars | `value()` |
| **Password** | `domain::value_objects::user` | `value_objects/user/Password.hpp` | >= 6 chars | `value()` |

### Exceptions

| Exception | Namespace | Condition de Déclenchement |
|-----------|-----------|----------------------------|
| **DomainException** | `domain::exceptions` | Base pour toutes les exceptions |
| **HealthException** | `domain::exceptions` | Health < 0 |
| **PositionException** | `domain::exceptions` | Position hors [-1000, 1000] |
| **PlayerIdException** | `domain::exceptions::player` | UUID MongoDB invalide |
| **UserIdException** | `domain::exceptions::user` | ID vide |
| **UsernameException** | `domain::exceptions::user` | Username pas 6-21 chars |
| **PasswordException** | `domain::exceptions::user` | Password < 6 chars |

---

## 🎯 Patterns et Conventions

### Pattern Value Object

Tous les Value Objects suivent ces principes:

1. **Immuabilité** - Aucune méthode ne modifie l'objet, elles retournent de nouveaux objets
2. **Validation** - Le constructeur valide toujours les données
3. **Comparaison** - Opérateurs `==` et `!=` basés sur la valeur
4. **Pas d'identité** - Deux Value Objects avec même valeur sont interchangeables

```cpp
// Exemple: Health est immuable
Health hp(100.0f);
Health newHp = hp.heal(50);  // hp reste 100, newHp = 150
```

### Pattern Exception

Toutes les exceptions suivent ces conventions:

1. **Message Builder** - Méthode `static buildMessage()` pour formater le message
2. **Héritage** - Toutes héritent de `DomainException`
3. **Const Correctness** - `what()` est `const noexcept`

```cpp
class CustomException: public DomainException {
    public:
        explicit CustomException(std::string value);
    private:
        static std::string buildMessage(std::string value);
};
```

### Séparation .hpp/.cpp

Tous les fichiers du Domain suivent cette séparation:

- **`.hpp`** - Déclarations (interfaces publiques)
- **`.cpp`** - Implémentations (définitions)

**Avantages:**
- Compilation incrémentale plus rapide
- Séparation claire interface/implémentation
- Facilite les tests et le mocking

---

## 🔗 Intégration

### Avec Application Layer

Les entités et Value Objects sont utilisés par les Use Cases:

```cpp
// application/use_cases/MovePlayerUseCase.cpp
#include "domain/entities/Player.hpp"

void MovePlayerUseCase::execute(
    const std::string& playerId,
    float dx, float dy, float dz
) {
    // 1. Charger Player depuis repository
    auto player = _playerRepo->findById(PlayerId(playerId));

    // 2. Appliquer logique métier (Domain)
    player.move(dx, dy, dz);

    // 3. Persister
    _playerRepo->save(player);
}
```

### Avec Infrastructure Layer

Les repositories convertissent entre Domain et base de données:

```cpp
// infrastructure/adapters/out/persistence/MongoDBPlayerRepository.cpp
Player MongoDBPlayerRepository::findById(const PlayerId& id) {
    // 1. Requête MongoDB
    bsoncxx::document::value query =
        bsoncxx::builder::stream::document{} << "_id" << id.value() << finalize;

    // 2. Convertir BSON -> Domain
    auto doc = _collection.find_one(query.view());

    return Player(
        Health(doc["health"].get_double()),
        PlayerId(doc["_id"].get_string()),
        Position(doc["x"].get_double(), doc["y"].get_double(), doc["z"].get_double())
    );
}
```

---

## 🧪 Tests

### Exemple de Test Unitaire

```cpp
// tests/domain/value_objects/HealthTest.cpp
#include <gtest/gtest.h>
#include "domain/value_objects/Health.hpp"
#include "domain/exceptions/HealthException.hpp"

TEST(HealthTest, ValidHealth) {
    Health hp(100.0f);
    EXPECT_EQ(hp.value(), 100.0f);
}

TEST(HealthTest, NegativeHealthThrows) {
    EXPECT_THROW(Health(-10.0f), HealthException);
}

TEST(HealthTest, HealReturnsNewHealth) {
    Health hp(100.0f);
    Health healed = hp.heal(50.0f);

    EXPECT_EQ(hp.value(), 100.0f);      // Original inchangé
    EXPECT_EQ(healed.value(), 150.0f);  // Nouveau avec +50
}
```

---

## 📚 Références

### Documentation Connexe

- **Architecture Hexagonale:** [guides/hexagonal-architecture.md](../guides/hexagonal-architecture.md)
- **Architecture Globale:** [guides/architecture.md](../guides/architecture.md)
- **Use Cases:** [api/application.md](application.md) (À créer)
- **Adapters:** [api/adapters.md](adapters.md) (En cours)

### Fichiers Source

**Entités:**
- `src/server/domain/entities/Player.hpp` / `.cpp`
- `src/server/domain/entities/User.hpp` / `.cpp`

**Value Objects:**
- `src/server/domain/value_objects/Health.hpp` / `.cpp`
- `src/server/domain/value_objects/Position.hpp` / `.cpp`
- `src/server/domain/value_objects/player/PlayerId.hpp` / `.cpp`
- `src/server/domain/value_objects/user/UserId.hpp` / `.cpp`
- `src/server/domain/value_objects/user/Username.hpp` / `.cpp`
- `src/server/domain/value_objects/user/Password.hpp` / `.cpp`

**Exceptions:**
- `src/server/domain/exceptions/DomainException.hpp` / `.cpp`
- `src/server/domain/exceptions/HealthException.hpp` / `.cpp`
- `src/server/domain/exceptions/PositionException.hpp` / `.cpp`
- `src/server/domain/exceptions/player/PlayerIdException.hpp` / `.cpp`
- `src/server/domain/exceptions/user/UserIdException.hpp` / `.cpp`
- `src/server/domain/exceptions/user/UsernameException.hpp` / `.cpp`
- `src/server/domain/exceptions/user/PasswordException.hpp` / `.cpp`

---

**Dernière révision:** 11/01/2025 par Agent Documentation
**Statut:** ✅ Complet et à jour avec le code (v0.2.0)
