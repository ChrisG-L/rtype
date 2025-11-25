# Tests et Qualité de Code

**Dernière mise à jour:** 25 novembre 2025

Ce guide décrit les pratiques de test et les outils de qualité de code utilisés dans le projet R-Type.

---

## Vue d'Ensemble

Le projet R-Type utilise plusieurs niveaux de test et outils de qualité :

| Outil | Usage | Statut |
|-------|-------|--------|
| **Google Test** | Tests unitaires | ✅ Configuré |
| **Sanitizers** | Détection bugs runtime | ✅ Activé (Debug) |
| **SonarCloud** | Analyse statique | ✅ Intégré CI |
| **Valgrind** | Analyse mémoire | 📋 Optionnel |

---

## Google Test

### Installation

Google Test est installé via vcpkg :

```bash
./vcpkg install gtest
```

### Structure des Tests

```
tests/
├── domain/
│   ├── entities/
│   │   ├── PlayerTest.cpp
│   │   └── UserTest.cpp
│   └── value_objects/
│       ├── HealthTest.cpp
│       ├── PositionTest.cpp
│       └── EmailTest.cpp
├── application/
│   └── use_cases/
│       └── LoginTest.cpp
└── CMakeLists.txt
```

### Écrire un Test

```cpp
#include <gtest/gtest.h>
#include "domain/value_objects/Health.hpp"
#include "domain/exceptions/HealthException.hpp"

using namespace domain::value_objects;
using namespace domain::exceptions;

// Test basique
TEST(HealthTest, ValidHealth) {
    Health hp(100.0f);
    EXPECT_EQ(hp.value(), 100.0f);
}

// Test d'exception
TEST(HealthTest, NegativeHealthThrows) {
    EXPECT_THROW(Health(-10.0f), HealthException);
}

// Test immuabilité
TEST(HealthTest, HealReturnsNewHealth) {
    Health hp(100.0f);
    Health healed = hp.heal(50.0f);

    EXPECT_EQ(hp.value(), 100.0f);      // Original inchangé
    EXPECT_EQ(healed.value(), 150.0f);  // Nouveau avec +50
}

// Fixture pour tests complexes
class UserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup avant chaque test
    }

    void TearDown() override {
        // Cleanup après chaque test
    }
};

TEST_F(UserTest, CreateValidUser) {
    // Test avec fixture
}
```

### Exécuter les Tests

```bash
# Compiler avec tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Exécuter tous les tests
ctest --test-dir build --output-on-failure

# Exécuter un test spécifique
ctest --test-dir build -R HealthTest

# Avec verbose
ctest --test-dir build -V
```

---

## Sanitizers

### Types de Sanitizers

| Sanitizer | Flag | Détecte |
|-----------|------|---------|
| **AddressSanitizer (ASan)** | `-fsanitize=address` | Buffer overflow, use-after-free |
| **UndefinedBehaviorSan (UBSan)** | `-fsanitize=undefined` | Division par zéro, integer overflow |
| **LeakSanitizer (LSan)** | `-fsanitize=leak` | Fuites mémoire |
| **ThreadSanitizer (TSan)** | `-fsanitize=thread` | Data races |

### Configuration CMake

Les sanitizers sont activés automatiquement en mode Debug :

```cmake
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT CMAKE_CROSSCOMPILING AND NOT MINGW)
        target_compile_options(rtype_server PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
        )
        target_link_options(rtype_server PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
        )
    endif()
endif()
```

### Exécuter avec Sanitizers

```bash
# Compiler en Debug (sanitizers activés)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Exécuter
./build/src/server/rtype_server

# Les erreurs sont reportées automatiquement
```

### Exemple de Sortie ASan

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010
    #0 0x55555555abcd in MyClass::doSomething() src/MyClass.cpp:42
    #1 0x55555555dcba in main src/main.cpp:15
=================================================================
```

---

## SonarCloud

### Intégration

SonarCloud est intégré au pipeline Jenkins pour l'analyse statique du code.

### Métriques Analysées

- **Bugs** : Problèmes de code potentiels
- **Vulnerabilities** : Failles de sécurité
- **Code Smells** : Problèmes de maintenabilité
- **Coverage** : Couverture de tests
- **Duplications** : Code dupliqué

### Configuration

Le fichier `sonar-project.properties` à la racine configure l'analyse :

```properties
sonar.projectKey=rtype
sonar.organization=epitech-study
sonar.sources=src
sonar.tests=tests
sonar.language=cpp
sonar.cpp.file.suffixes=.cpp,.hpp
```

### Voir les Résultats

1. Ouvrir [SonarCloud](https://sonarcloud.io)
2. Naviguer vers le projet R-Type
3. Consulter les métriques et issues

---

## Bonnes Pratiques de Test

### Principes FIRST

- **F**ast : Tests rapides (<1s chacun)
- **I**ndependent : Tests indépendants les uns des autres
- **R**epeatable : Résultats identiques à chaque exécution
- **S**elf-validating : Pass ou Fail, pas d'interprétation
- **T**imely : Écrits en même temps que le code

### Couverture de Code

Viser une couverture de :

| Couche | Couverture Cible |
|--------|------------------|
| Domain | 90%+ |
| Application | 80%+ |
| Infrastructure | 60%+ |

### Nommage des Tests

```cpp
// Format: MethodName_StateUnderTest_ExpectedBehavior
TEST(Health, Constructor_NegativeValue_ThrowsException)
TEST(User, VerifyPassword_CorrectPassword_ReturnsTrue)
TEST(TCPClient, Connect_ServerOffline_CallsErrorCallback)
```

---

## Tests d'Intégration

### Base de Données

Pour les tests avec MongoDB :

```cpp
class MongoDBIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Utiliser une base de test
        config.dbName = "rtype_test";
    }

    void TearDown() override {
        // Nettoyer après test
        dropTestDatabase();
    }
};
```

### Réseau

Pour les tests réseau :

```cpp
class TCPClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Démarrer un serveur mock
        mockServer.start(12345);
    }

    void TearDown() override {
        mockServer.stop();
    }
};
```

---

## CI/CD et Tests

### Pipeline Jenkins

Le pipeline exécute automatiquement :

1. **Compilation** Debug et Release
2. **Tests unitaires** via ctest
3. **Analyse Sanitizers** en Debug
4. **Analyse SonarCloud**
5. **Build artifacts**

### Exécution Locale avant Push

```bash
# Script de vérification pré-commit
#!/bin/bash
set -e

echo "Building..."
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

echo "Running tests..."
ctest --test-dir build --output-on-failure

echo "All checks passed!"
```

---

## Ressources

- [Google Test Documentation](https://google.github.io/googletest/)
- [Sanitizers Documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
- [SonarCloud Documentation](https://sonarcloud.io/documentation)
- [Guide SonarQube](../guides/sonarqube.md)

---

**Dernière révision:** 25/11/2025
