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
// camelCase pour les variables locales
int playerCount;
float deltaTime;

// Membres de classe: underscore prefix
class Player {
    int _health;
    float _speed;
    std::string _displayName;
};
```

### Constantes

```cpp
// UPPER_SNAKE_CASE
constexpr int MAX_PLAYERS = 4;
constexpr float TICK_DURATION = 1.0f / 60.0f;
```

### Namespaces

Le projet utilise des namespaces basés sur l'architecture hexagonale :

```cpp
// Serveur - Architecture hexagonale
namespace domain::entities { }
namespace domain::value_objects { }
namespace domain::exceptions { }
namespace application::use_cases::auth { }
namespace application::ports::out { }
namespace infrastructure::game { }
namespace infrastructure::room { }
namespace infrastructure::session { }
namespace infrastructure::bootstrap { }
namespace infrastructure::adapters::in::network { }
namespace infrastructure::adapters::out::persistence { }

// Client
namespace core { }                    // Engine, GameLoop
namespace client::network { }         // UDPClient, TCPClient
namespace audio { }                   // AudioManager

// Common
namespace protocol { }                // MessageType, structures
namespace collision { }               // AABB
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
// Style K&R partout
void function() {
    if (condition) {
        // ...
    } else {
        // ...
    }

    for (int i = 0; i < n; i++) {
        // ...
    }
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
for (const auto& player : _players) {
    process(player);
}

// Éviter
for (size_t i = 0; i < _players.size(); i++) {
    process(_players[i]);
}
```

---

## Headers

### Include Guards

```cpp
// Préférer ifndef/define (style Epitech)
#ifndef GAMEWORLD_HPP_
#define GAMEWORLD_HPP_

// ...

#endif /* !GAMEWORLD_HPP_ */
```

### Ordre des Includes

```cpp
// 1. Header associé (pour .cpp)
#include "GameWorld.hpp"

// 2. Headers du projet
#include "Protocol.hpp"
#include "domain/entities/Player.hpp"

// 3. Headers de bibliothèques tierces
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

// 4. Headers standard
#include <vector>
#include <string>
#include <memory>
```

### Forward Declarations

```cpp
// Préférer forward declaration quand possible
namespace domain::entities {
    class Player;
}

class GameWorld {
    std::vector<std::unique_ptr<domain::entities::Player>> _players;
};
```

---

## Commentaires

### En-tête de Fichier (Style Epitech)

```cpp
/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameWorld - Manages game state and players
*/
```

### Documentation

```cpp
/**
 * @brief Suite de tests pour l'entité Player
 *
 * Player est une entité de domaine représentant un joueur.
 */
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
    using PlayerMap = std::unordered_map<uint8_t, ConnectedPlayer>;

    // Constantes
    static constexpr int MAX_PLAYERS = 4;

    // Constructeurs / Destructeur
    explicit GameWorld(boost::asio::io_context& io_ctx);
    ~GameWorld() = default;

    // Pas de copie
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;

    // Move OK
    GameWorld(GameWorld&&) = default;
    GameWorld& operator=(GameWorld&&) = default;

    // Méthodes publiques
    void tick();
    std::optional<uint8_t> addPlayer(const udp::endpoint& endpoint);

private:
    // Méthodes privées
    void processInputs();
    void checkCollisions();

    // Membres (underscore prefix)
    PlayerMap _players;
    uint32_t _currentTick = 0;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
};
```

---

## Patterns du Projet

### Sérialisation Réseau

```cpp
// Pattern to_bytes / from_bytes avec big-endian
struct PlayerState {
    void to_bytes(uint8_t* buf) const {
        buf[0] = id;
        uint16_t net_x = swap16(x);
        std::memcpy(buf + 1, &net_x, 2);
        // ...
    }

    static std::optional<PlayerState> from_bytes(const void* buf, size_t len) {
        if (len < WIRE_SIZE) return std::nullopt;
        // Parse avec swap16/swap32 pour network to host
    }
};
```

### Value Objects Immutables

```cpp
// Domain value objects retournent de nouvelles instances
class Position {
public:
    Position withX(float newX) const {
        return Position(newX, _y, _z);
    }

private:
    float _x, _y, _z;
};
```

---

## Checklist PR

- [ ] Code formaté correctement
- [ ] Pas de warnings de compilation
- [ ] Tests unitaires passent
- [ ] Documentation mise à jour si nécessaire
- [ ] Commit messages suivent conventional commits
- [ ] Namespaces corrects (`infrastructure::`, `domain::`, etc.)
- [ ] Membres avec underscore prefix (`_member`)
