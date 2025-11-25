# Tests et Qualité de Code

**Dernière mise à jour:** 25 novembre 2025
**Version:** 1.1.0

Ce guide décrit le système de tests unitaires et les outils de qualité de code utilisés dans le projet R-Type.

---

## Vue d'Ensemble

Le projet R-Type dispose d'une suite de tests complète utilisant Google Test (GTest).

| Composant | Tests | Couverture |
|-----------|-------|------------|
| **Serveur - Value Objects** | 70+ tests | Health, Position, Email, Username |
| **Serveur - Entities** | 20+ tests | Player |
| **Serveur - Network Protobuf** | 35+ tests | User, Auth, Game messages |
| **Serveur - Network Protocol** | 30+ tests | CommandParser |
| **Serveur - TCP Integration** | 19 tests | Client/Server communication |
| **Serveur - UDP Integration** | 16 tests | Datagrammes, latence |
| **Client - Utils** | 40+ tests | Vecs, Signal |
| **Total** | 210+ tests | ~75% |

### Outils Utilisés

| Outil | Usage | Statut |
|-------|-------|--------|
| **Google Test** | Tests unitaires | ✅ Configuré |
| **Sanitizers** | Détection bugs runtime | ✅ Activé (Debug) |
| **CTest** | Orchestration tests | ✅ Intégré |
| **script test.sh** | Runner personnalisé | ✅ Disponible |

---

## Démarrage Rapide

### Exécuter Tous les Tests

```bash
# Méthode recommandée : utiliser le script
./scripts/test.sh

# Alternative : via CMake/CTest
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Options du Script test.sh

```bash
# Aide
./scripts/test.sh --help

# Tests serveur uniquement
./scripts/test.sh --server

# Tests client uniquement
./scripts/test.sh --client

# Mode verbose
./scripts/test.sh --verbose

# Filtrer les tests
./scripts/test.sh --filter "HealthTest.*"
./scripts/test.sh --filter "*Position*"

# Lister les tests disponibles
./scripts/test.sh --list

# Nettoyer et rebuilder
./scripts/test.sh --clean
```

---

## Structure des Tests

```
tests/
├── server/                              # Tests du serveur
│   ├── main.cpp                         # Point d'entrée GTest
│   ├── CMakeLists.txt                   # Configuration CMake
│   ├── domain/
│   │   ├── value_objects/
│   │   │   ├── HealthTest.cpp           # 25 tests Health
│   │   │   ├── PositionTest.cpp         # 25 tests Position
│   │   │   ├── EmailTest.cpp            # 20 tests Email
│   │   │   └── UsernameTest.cpp         # 20 tests Username
│   │   └── entities/
│   │       └── PlayerTest.cpp           # 25 tests Player
│   │
│   └── network/                         # Tests réseau
│       ├── ProtobufTest.cpp             # 35+ tests sérialisation
│       ├── protocol/
│       │   └── CommandParserTest.cpp    # 30+ tests parsing
│       ├── TCPIntegrationTest.cpp       # 19 tests TCP
│       └── UDPIntegrationTest.cpp       # 16 tests UDP
│
├── client/                              # Tests du client
│   ├── main.cpp                         # Point d'entrée GTest
│   ├── CMakeLists.txt                   # Configuration CMake
│   └── utils/
│       ├── VecsTest.cpp                 # 30 tests Vecs
│       └── SignalTest.cpp               # 25 tests Signal
│
└── artifacts/tests/                     # Binaires générés
    ├── server_tests
    └── client_tests

proto/                                   # Définitions Protobuf
├── user.proto                           # Messages User
├── auth.proto                           # Messages authentification
└── game.proto                           # Messages gameplay
```

---

## Écrire des Tests

### Structure de Base

```cpp
#include <gtest/gtest.h>
#include "domain/value_objects/Health.hpp"
#include "domain/exceptions/HealthException.hpp"

using namespace domain::value_objects;
using namespace domain::exceptions;

/**
 * @brief Suite de tests pour Health Value Object
 */
class HealthTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialisation avant chaque test
    }
    void TearDown() override {
        // Nettoyage après chaque test
    }
};

// Test de création valide
TEST_F(HealthTest, CreateWithValidValue) {
    Health health(3.0f);
    EXPECT_FLOAT_EQ(health.value(), 3.0f);
}

// Test d'exception
TEST_F(HealthTest, CreateWithNegativeThrows) {
    EXPECT_THROW({
        Health health(-1.0f);
    }, HealthException);
}

// Test d'immutabilité
TEST_F(HealthTest, HealIsImmutable) {
    Health original(2.0f);
    Health healed = original.heal(1.0f);

    EXPECT_FLOAT_EQ(original.value(), 2.0f);  // Inchangé
    EXPECT_FLOAT_EQ(healed.value(), 3.0f);    // Nouveau
}
```

### Assertions GTest

```cpp
// Assertions fatales (arrêtent le test en cas d'échec)
ASSERT_EQ(a, b);          // a == b
ASSERT_NE(a, b);          // a != b
ASSERT_TRUE(condition);   // condition == true
ASSERT_FALSE(condition);  // condition == false
ASSERT_NO_THROW({ ... }); // Pas d'exception levée
ASSERT_THROW({ ... }, ExceptionType);  // Exception levée

// Assertions non-fatales (continuent le test)
EXPECT_EQ(a, b);
EXPECT_FLOAT_EQ(a, b);    // Comparaison float avec tolérance
EXPECT_NEAR(a, b, delta); // |a - b| < delta
EXPECT_STREQ(s1, s2);     // Strings égales
```

### Patterns de Tests

#### Test de Constructeur

```cpp
TEST_F(PositionTest, CreateWithDefaultValues) {
    Position pos;
    EXPECT_FLOAT_EQ(pos.getX(), 0.0f);
    EXPECT_FLOAT_EQ(pos.getY(), 0.0f);
    EXPECT_FLOAT_EQ(pos.getZ(), 0.0f);
}
```

#### Test de Validation

```cpp
TEST_F(EmailTest, InvalidEmailWithoutAtThrows) {
    EXPECT_THROW({
        Email email("invalid-email.com");
    }, EmailException);
}
```

#### Test d'Opérateurs

```cpp
TEST_F(HealthTest, EqualityOperator) {
    Health h1(3.0f);
    Health h2(3.0f);
    Health h3(2.0f);

    EXPECT_TRUE(h1 == h2);
    EXPECT_FALSE(h1 == h3);
}
```

#### Test de Signal/Slot

```cpp
TEST_F(SignalTest, SignalEmitsToMultipleSlots) {
    Signal<int> signal;
    int sum = 0;

    signal.connect([&sum](int v) { sum += v; });
    signal.connect([&sum](int v) { sum += v * 2; });

    signal.emit(10);

    EXPECT_EQ(sum, 30);  // 10 + 20
}
```

#### Test Protobuf (Sérialisation)

```cpp
TEST_F(GameProtoTest, SerializeWorldSnapshot) {
    rtype::game::WorldSnapshot original;
    original.set_server_tick(999);
    original.set_state(rtype::game::GAME_RUNNING);

    for (int i = 0; i < 10; i++) {
        auto* entity = original.add_entities();
        entity->set_network_id(i);
        entity->set_type(rtype::game::ENTITY_PLAYER);
        entity->mutable_position()->set_x(static_cast<float>(i));
        entity->set_health(100);
    }

    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));

    rtype::game::WorldSnapshot deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.server_tick(), 999u);
    EXPECT_EQ(deserialized.entities_size(), 10);
}
```

#### Test TCP/UDP Integration

```cpp
TEST_F(TCPCommunicationTest, SendReceiveSimpleMessage) {
    std::string message = "Hello, Server!";

    ASSERT_TRUE(_client->send(message));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, message);  // Echo server
}

TEST_F(UDPProtobufTest, SendClientInput) {
    rtype::game::ClientInput input;
    input.set_sequence_number(42);
    input.mutable_input()->set_shoot(true);

    std::string serialized;
    ASSERT_TRUE(input.SerializeToString(&serialized));

    ASSERT_TRUE(_client->sendTo("127.0.0.1", 19892, serialized));

    std::string response = _client->receive(1000);
    rtype::game::ClientInput received;
    ASSERT_TRUE(received.ParseFromString(response));
    EXPECT_EQ(received.sequence_number(), 42u);
}
```

---

## Conventions de Nommage

### Fichiers de Test

```
{ComponentName}Test.cpp

# Exemples
HealthTest.cpp
PositionTest.cpp
PlayerTest.cpp
SignalTest.cpp
```

### Classes de Test

```cpp
class {ComponentName}Test : public ::testing::Test { };

// Exemples
class HealthTest : public ::testing::Test { };
class PlayerTest : public ::testing::Test { };
```

### Méthodes de Test

Format : `{Action}{Condition}{ExpectedResult}`

```cpp
// Exemples
TEST_F(HealthTest, CreateWithMinValue)
TEST_F(HealthTest, CreateWithNegativeValueThrows)
TEST_F(HealthTest, HealIncreasesHealth)
TEST_F(HealthTest, HealIsImmutable)
TEST_F(PositionTest, MoveOutOfBoundsThrows)
TEST_F(EmailTest, CreateWithValidEmail)
```

---

## Filtrage des Tests

### Patterns GTest

```bash
# Tous les tests d'une classe
./scripts/test.sh --filter "HealthTest.*"

# Tests commençant par un préfixe
./scripts/test.sh --filter "HealthTest.Create*"

# Tests contenant un mot
./scripts/test.sh --filter "*Position*"

# Exclure des tests
./scripts/test.sh --filter "-HealthTest.*"

# Combinaison
./scripts/test.sh --filter "HealthTest.*:PositionTest.*"
```

### Exemples Pratiques

```bash
# Tests de création uniquement
./scripts/test.sh --filter "*Create*"

# Tests d'exceptions
./scripts/test.sh --filter "*Throws*"

# Tests d'opérateurs
./scripts/test.sh --filter "*Operator*"

# Un test spécifique
./scripts/test.sh --filter "HealthTest.CreateWithMaxValue"
```

---

## Sanitizers

Les sanitizers sont automatiquement activés en mode Debug.

### Types

| Sanitizer | Détecte |
|-----------|---------|
| **AddressSanitizer** | Buffer overflow, use-after-free |
| **UndefinedBehaviorSan** | Division par zéro, integer overflow |
| **LeakSanitizer** | Fuites mémoire |

### Configuration CMake

```cmake
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(target PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
    )
    target_link_options(target PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
    )
endif()
```

---

## CI/CD

### Pipeline Jenkins

Le pipeline exécute automatiquement :

1. **Compilation** Debug
2. **Tests unitaires** via test.sh
3. **Analyse Sanitizers**
4. **Rapport de couverture**

### Vérification Locale

```bash
# Avant chaque commit
./scripts/test.sh

# Ou avec plus de détails
./scripts/test.sh --verbose
```

---

## Bonnes Pratiques

### Principes FIRST

- **F**ast : Tests rapides (<100ms chacun)
- **I**ndependent : Tests indépendants
- **R**epeatable : Résultats identiques
- **S**elf-validating : Pass ou Fail clair
- **T**imely : Écrits avec le code

### Couverture Cible

| Couche | Objectif |
|--------|----------|
| Value Objects | 95%+ |
| Entities | 90%+ |
| Use Cases | 80%+ |
| Utils | 85%+ |

### Documentation

Chaque fichier de test devrait contenir :

```cpp
/**
 * @brief Suite de tests pour [Component]
 *
 * [Description du composant testé]
 * Contraintes : [Liste des règles de validation]
 */
```

---

## Dépannage

### Erreur de Compilation

```bash
# Nettoyer et reconfigurer
./scripts/test.sh --clean

# Vérifier les dépendances
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### Tests qui Échouent

```bash
# Mode verbose pour plus de détails
./scripts/test.sh --verbose

# Exécuter un test spécifique
./scripts/test.sh --filter "FailingTest.*"
```

### Sanitizer Errors

```bash
# Les erreurs sont affichées automatiquement
# Chercher les lignes avec ERROR: AddressSanitizer
```

---

## Ressources

- [Google Test Documentation](https://google.github.io/googletest/)
- [GTest Primer](https://google.github.io/googletest/primer.html)
- [Sanitizers Documentation](https://clang.llvm.org/docs/AddressSanitizer.html)

---

**Dernière révision:** 25/11/2025 par Agent Documentation
