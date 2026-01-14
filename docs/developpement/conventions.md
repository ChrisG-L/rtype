---
tags:
  - developpement
  - conventions
  - style
---

# Conventions de Code

Standards de codage pour le projet R-Type.

## Nommage

### Classes et Structs

```cpp
// PascalCase
class GameWorld { };
struct PlayerState { };
```

### Fonctions et Méthodes

```cpp
// camelCase
void processInput();
float calculateDistance();
```

### Variables

```cpp
// camelCase
int playerCount;
float deltaTime;

// Membres de classe: trailing underscore
class Player {
    int health_;
    float speed_;
};
```

### Constantes

```cpp
// UPPER_SNAKE_CASE
constexpr int MAX_PLAYERS = 4;
constexpr float TICK_DURATION = 1.0f / 60.0f;
```

### Namespaces

```cpp
// snake_case
namespace rtype::server { }
namespace rtype::client { }
namespace rtype::network { }
```

---

## Formatage

### Indentation

```cpp
// 4 espaces, pas de tabs
void function() {
    if (condition) {
        doSomething();
    }
}
```

### Accolades

```cpp
// Style Allman pour les fonctions
void function()
{
    // ...
}

// Style K&R pour le reste
if (condition) {
    // ...
} else {
    // ...
}

for (int i = 0; i < n; i++) {
    // ...
}
```

### Longueur de Ligne

Maximum **100 caractères**.

```cpp
// Mauvais
void function(int veryLongParameterName, float anotherVeryLongParameterName, std::string yetAnotherLongName);

// Bon
void function(int veryLongParameterName,
              float anotherVeryLongParameterName,
              std::string yetAnotherLongName);
```

---

## C++ Moderne

### Utiliser `auto` avec parcimonie

```cpp
// Bon: type évident
auto it = map.find(key);
auto ptr = std::make_unique<Player>();

// Mauvais: type non évident
auto result = calculate();  // Quel type?
```

### Préférer `const` et `constexpr`

```cpp
// Constantes compile-time
constexpr int MAX_PLAYERS = 4;

// Paramètres non modifiés
void process(const Player& player);

// Méthodes qui ne modifient pas l'objet
bool isAlive() const;
```

### Smart Pointers

```cpp
// Ownership unique
std::unique_ptr<Player> player;

// Ownership partagé
std::shared_ptr<Resource> resource;

// Référence non-owning
Player* rawPtr;  // N'utiliser que si ownership clair
```

### Range-based For

```cpp
// Préférer
for (const auto& player : players) {
    process(player);
}

// Éviter
for (size_t i = 0; i < players.size(); i++) {
    process(players[i]);
}
```

---

## Headers

### Include Guards

```cpp
#pragma once  // Préféré

// Ou garde traditionnelle
#ifndef RTYPE_PLAYER_HPP
#define RTYPE_PLAYER_HPP
// ...
#endif
```

### Ordre des Includes

```cpp
// 1. Header associé (pour .cpp)
#include "Player.hpp"

// 2. Headers du projet
#include "network/Protocol.hpp"
#include "game/Entity.hpp"

// 3. Headers de bibliothèques tierces
#include <SFML/Graphics.hpp>
#include <mongocxx/client.hpp>

// 4. Headers standard
#include <vector>
#include <string>
#include <memory>
```

### Forward Declarations

```cpp
// Préférer forward declaration quand possible
class Player;  // Au lieu de #include "Player.hpp"

class GameWorld {
    std::vector<std::unique_ptr<Player>> players_;
};
```

---

## Commentaires

### Documentation Doxygen

```cpp
/**
 * @brief Traite un input joueur
 *
 * @param playerId ID du joueur
 * @param input Input à traiter
 * @return true si l'input a été traité
 */
bool processInput(PlayerId playerId, const Input& input);
```

### Commentaires Inline

```cpp
// Bon: explique le POURQUOI
// Clamp pour éviter les positions invalides hors écran
position.x = std::clamp(position.x, 0.0f, WORLD_WIDTH);

// Mauvais: explique le QUOI (évident)
// Incrémenter i
i++;
```

### TODO et FIXME

```cpp
// TODO: Implémenter la validation des inputs
// FIXME: Memory leak quand player déconnecté
// HACK: Workaround pour bug SFML #1234
```

---

## Structure des Classes

```cpp
class GameWorld {
public:
    // Types publics
    using PlayerList = std::vector<Player>;

    // Constantes
    static constexpr int MAX_PLAYERS = 4;

    // Constructeurs / Destructeur
    GameWorld();
    ~GameWorld();

    // Pas de copie
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;

    // Move OK
    GameWorld(GameWorld&&) = default;
    GameWorld& operator=(GameWorld&&) = default;

    // Méthodes publiques
    void tick();
    void addPlayer(PlayerId id);

private:
    // Méthodes privées
    void processInputs();
    void checkCollisions();

    // Membres (ordre: dépendances d'abord)
    PlayerList players_;
    uint32_t currentTick_ = 0;
};
```

---

## .clang-format

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
BreakBeforeBraces: Custom
BraceWrapping:
  AfterFunction: true
  AfterClass: false
  AfterControlStatement: false
PointerAlignment: Left
SpaceAfterCStyleCast: false
SpacesInParentheses: false
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
SortIncludes: true
```

---

## Checklist PR

- [ ] Code formaté avec clang-format
- [ ] Pas de warnings de compilation
- [ ] Tests unitaires passent
- [ ] Documentation mise à jour
- [ ] Commit messages suivent conventional commits
