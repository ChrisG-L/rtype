# Guide de Migration API

**Version:** 0.3.0
**Dernière mise à jour:** 17 janvier 2025

---

## 📋 Vue d'Ensemble

Ce guide documente les changements d'API entre les versions et fournit des instructions de migration pour les développeurs.

---

## Migration v0.2.0 → v0.3.0

### 🔄 Changements Majeurs

#### 1. Entité User - Ajout du champ Email

**Avant (v0.2.0):**
```cpp
User user(
    UserId("507f1f77bcf86cd799439012"),
    Username("player123"),
    Password("hashed_password"),
    std::chrono::system_clock::now(),  // createdAt
    std::chrono::system_clock::now()   // lastLogin
);
```

**Après (v0.3.0):**
```cpp
User user(
    UserId("507f1f77bcf86cd799439012"),
    Username("player123"),
    Email("player@example.com"),       // ✨ NOUVEAU
    Password("hashed_password"),
    std::chrono::system_clock::now(),  // lastLogin (ordre changé)
    std::chrono::system_clock::now()   // createdAt
);
```

**⚠️ Breaking Changes:**
- Nouveau paramètre `Email` obligatoire
- Ordre des paramètres `lastLogin` et `createdAt` inversé

**Migration:**
```cpp
// Ancien code
User oldUser(id, username, password, createdAt, lastLogin);

// Nouveau code
#include "domain/value_objects/user/Email.hpp"
User newUser(id, username, Email("default@example.com"), password, lastLogin, createdAt);
```

---

#### 2. Password - Méthode hashPassword() déplacée

**Avant (v0.2.0):**
```cpp
std::string hash = Password::hashPassword("my_password");  // Méthode statique
Password pwd(hash);
```

**Après (v0.3.0):**
```cpp
#include "domain/value_objects/user/utils/PasswordUtils.hpp"

std::string hash = utils::hashPassword("my_password");  // Fonction libre
Password pwd(hash);
```

**⚠️ Breaking Change:**
- `Password::hashPassword()` n'existe plus
- Utiliser `domain::value_objects::user::utils::hashPassword()`

**Migration:**
```cpp
// Ancien code
#include "domain/value_objects/user/Password.hpp"
auto hash = Password::hashPassword("password");

// Nouveau code
#include "domain/value_objects/user/Password.hpp"
#include "domain/value_objects/user/utils/PasswordUtils.hpp"
using namespace domain::value_objects::user;
auto hash = utils::hashPassword("password");
```

---

#### 3. Use Case Login - Utilise l'email au lieu du username

**Avant (v0.2.0):**
```cpp
Login loginUseCase(userRepository);
loginUseCase.execute("player123", "password");  // username
```

**Après (v0.3.0):**
```cpp
Login loginUseCase(userRepository);
loginUseCase.execute("player@example.com", "password");  // email
```

**⚠️ Breaking Change:**
- Premier paramètre changé de `username` à `email`

**Migration:**
```cpp
// Ancien code
loginUseCase.execute(user.getUsername().value(), password);

// Nouveau code
loginUseCase.execute(user.getEmail().value(), password);
```

---

#### 4. IUserRespository - Nouvelle méthode findByEmail()

**Avant (v0.2.0):**
```cpp
class IUserRespository {
public:
    virtual std::optional<User> findByName(const std::string& username) = 0;
    // ...
};
```

**Après (v0.3.0):**
```cpp
class IUserRespository {
public:
    virtual std::optional<User> findByName(const std::string& username) = 0;
    virtual std::optional<User> findByEmail(const std::string& email) = 0;  // ✨ NOUVEAU
    // ...
};
```

**⚠️ Breaking Change pour les implémentations:**
- Toute implémentation de `IUserRespository` doit implémenter `findByEmail()`

**Migration:**
```cpp
// Dans votre implémentation de repository
class CustomUserRepository : public IUserRespository {
public:
    // Ajouter cette méthode
    std::optional<User> findByEmail(const std::string& email) override {
        // Votre implémentation
        // Exemple MongoDB:
        auto result = collection->find_one(make_document(kvp("email", email)));
        if (result) {
            return documentToUser(result->view());
        }
        return std::nullopt;
    }
};
```

---

### ✨ Nouveautés v0.3.0

#### Value Object Email

Nouveau type pour représenter et valider les adresses email.

```cpp
#include "domain/value_objects/user/Email.hpp"

Email email("user@example.com");
std::string value = email.value();

// Validation automatique RFC 5322
try {
    Email invalid("not-an-email");
} catch (const EmailException& e) {
    std::cerr << e.what() << std::endl;
}
```

**Documentation:** [API Domain - Email](domain.md#email)

---

#### Use Case Register

Nouveau use case pour l'inscription d'utilisateurs.

```cpp
#include "application/use_cases/auth/Register.hpp"

Register registerUseCase(userRepository);
registerUseCase.execute("username", "user@example.com", "password123");
```

**Documentation:** [Guide Authentification - Register](../guides/authentication.md#use-case-register)

---

#### ExecuteAuth Command Handler

Nouveau gestionnaire de commandes d'authentification pour le réseau.

```cpp
#include "infrastructure/adapters/in/network/execute/auth/ExecuteAuth.hpp"

Command cmd = parser.parse("REGISTER alice alice@example.com pass123");
ExecuteAuth handler(cmd, loginUseCase, registerUseCase);
```

**Documentation:** [Guide Authentification - ExecuteAuth](../guides/authentication.md#command-handler-executeauth)

---

#### MongoDBUserRepository - Méthodes complètes

Le repository MongoDB est maintenant complètement implémenté.

**Nouvelles méthodes fonctionnelles:**
- `save(user)` - Avec validation d'unicité
- `findByEmail(email)` - Recherche par email
- `findById(id)` - Recherche par ID
- `findByName(username)` - Recherche par username
- `documentToUser(doc)` - Conversion BSON → User
- `timePointToDate(tp)` - Helper pour dates

**Documentation:** [Guide Authentification - MongoDBUserRepository](../guides/authentication.md#repository-mongodbuserrepository)

---

## Migration v0.1.0 → v0.2.0

### 🔄 Changements Majeurs

#### Architecture Hexagonale

Refactorisation complète vers une architecture hexagonale.

**Avant (v0.1.0):** Code monolithique

**Après (v0.2.0):** Séparation claire en couches
- Domain Layer (logique métier)
- Application Layer (use cases)
- Infrastructure Layer (adapters)

**Migration:** Réorganiser le code selon les couches appropriées.

---

#### Séparation .hpp/.cpp

**Avant (v0.1.0):** Tout dans les headers

**Après (v0.2.0):** Séparation déclaration/implémentation

```cpp
// Avant: Player.hpp contenait tout
class Player {
    void move(float dx, float dy, float dz) {
        // Implémentation dans le header
    }
};

// Après: Player.hpp + Player.cpp
// Player.hpp
class Player {
    void move(float dx, float dy, float dz);
};

// Player.cpp
void Player::move(float dx, float dy, float dz) {
    // Implémentation dans le .cpp
}
```

---

#### Value Objects avec Validation

Introduction de Value Objects immuables avec validation.

**Avant (v0.1.0):**
```cpp
struct Player {
    float health;  // Peut être négatif
    std::string id;  // Peut être invalide
};
```

**Après (v0.2.0):**
```cpp
#include "domain/value_objects/Health.hpp"
#include "domain/value_objects/player/PlayerId.hpp"

Player player(
    Health(100.0f),  // Validation >= 0
    PlayerId("507f1f77bcf86cd799439011")  // Validation format UUID
);
```

---

## 📚 Ressources de Migration

### Documentation API

- [API Domain Layer](domain.md) - Référence complète du domaine
- [API Adapters Layer](adapters.md) - Référence des adaptateurs

### Guides

- [Architecture Hexagonale](../guides/hexagonal-architecture.md) - Principes et patterns
- [Guide Authentification](../guides/authentication.md) - Système d'authentification complet

### Changelog

- [Changelog v0.3.0](../development/changelog.md#030---2025-01-17) - Détails complets des changements
- [Changelog v0.2.0](../development/changelog.md#020---2025-01-11) - Architecture hexagonale

---

## ⚠️ Checklist de Migration

### Migration v0.2.0 → v0.3.0

- [ ] Mettre à jour tous les constructeurs de `User` avec `Email`
- [ ] Inverser l'ordre `lastLogin`/`createdAt` dans les constructeurs `User`
- [ ] Remplacer `Password::hashPassword()` par `utils::hashPassword()`
- [ ] Changer `Login::execute()` pour utiliser email au lieu de username
- [ ] Implémenter `findByEmail()` dans tous les repositories custom
- [ ] Ajouter l'include `<domain/value_objects/user/Email.hpp>` où nécessaire
- [ ] Ajouter l'include `<domain/value_objects/user/utils/PasswordUtils.hpp>` où nécessaire
- [ ] Mettre à jour les tests unitaires pour les nouvelles signatures
- [ ] Mettre à jour la documentation projet si nécessaire

### Tests de Régression

```bash
# Compiler le projet
cd build
cmake --build .

# Lancer les tests
ctest --output-on-failure

# Vérifier les warnings de compilation
cmake --build . 2>&1 | grep -i "warning\|error"
```

---

## 🆘 Support

### Problèmes Courants

#### Erreur: "Email.hpp: No such file or directory"

```cpp
// Solution: Ajouter l'include
#include "domain/value_objects/user/Email.hpp"
```

#### Erreur: "no matching function for call to 'User::User'"

```cpp
// Problème: Ancien constructeur
User user(id, username, password, createdAt, lastLogin);

// Solution: Nouveau constructeur avec Email
User user(id, username, Email("email@example.com"), password, lastLogin, createdAt);
```

#### Erreur: "hashPassword is not a member of Password"

```cpp
// Problème: Ancienne méthode statique
auto hash = Password::hashPassword("pass");

// Solution: Utiliser utils
#include "domain/value_objects/user/utils/PasswordUtils.hpp"
auto hash = utils::hashPassword("pass");
```

---

## 📞 Contact

Pour toute question sur la migration :

- **Issues GitHub:** https://github.com/Pluenet-Killian/rtype/issues
- **Documentation:** https://rtype.example/docs
- **Email:** support@rtype.example

---

*Dernière mise à jour: 17 janvier 2025 | Version 0.3.0*
