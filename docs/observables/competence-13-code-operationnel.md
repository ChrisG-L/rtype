# Compétence 13 : Code Opérationnel

> **Rédiger le code à l'aide du langage informatique adapté au logiciel en implémentant les solutions techniques précédemment identifiées, afin de concrétiser la vision et la valeur du produit par le client.**

---

## Observable 13.1 : Code Opérationnel Répondant aux Exigences

### Exigences Fonctionnelles et Implémentation

| Exigence | Implémentation | Fichier Principal |
|----------|----------------|-------------------|
| Jeu multijoueur 4 joueurs | UDPServer + GameWorld | `UDPServer.cpp`, `GameWorld.cpp` |
| Authentification sécurisée | TCPAuthServer + TLS | `TCPAuthServer.cpp` |
| Classements persistants | MongoDB + Leaderboard | `MongoDBLeaderboardRepository.cpp` |
| Voice chat | Opus + PortAudio | `VoiceChatManager.cpp` |
| Système social | Friends + Messages | `MongoDBFriendshipRepository.cpp` |
| Cross-platform | SFML/SDL2 plugins | `IWindow.hpp`, `SFMLWindow.cpp` |

### Code du Serveur de Jeu

#### UDPServer - Boucle de Jeu

**Fichier** : `src/server/infrastructure/adapters/in/network/UDPServer.cpp`

```cpp
void UDPServer::startBroadcastLoop() {
    _broadcastTimer.expires_after(std::chrono::milliseconds(50));  // 20 Hz
    _broadcastTimer.async_wait([this](boost::system::error_code ec) {
        if (ec) return;

        // Broadcast snapshot pour chaque room active
        for (auto& [roomCode, instance] : _instanceManager.getInstances()) {
            auto gameWorld = instance->getGameWorld();
            if (gameWorld) {
                broadcastSnapshotForRoom(roomCode, gameWorld);
            }
        }

        startBroadcastLoop();  // Reschedule
    });
}

void UDPServer::broadcastSnapshotForRoom(
    const std::string& roomCode,
    const std::shared_ptr<game::GameWorld>& gameWorld
) {
    protocol::GameSnapshot snapshot = gameWorld->getSnapshot();

    // Sérialisation
    std::vector<uint8_t> payload(snapshot.serializedSize());
    snapshot.to_bytes(payload.data());

    // Compression si >= 128 bytes
    auto compressed = compression::compress(payload.data(), payload.size());
    // ...

    // Envoi à tous les joueurs de la room
    for (const auto& endpoint : roomEndpoints) {
        _socket.async_send_to(buffer, endpoint, ...);
    }
}
```

#### GameWorld - État de Jeu

**Fichier** : `src/server/infrastructure/game/GameWorld.cpp`

```cpp
void GameWorld::update(float deltaTime) {
    if (!_isRunning) return;

    float scaledDt = deltaTime * _gameSpeedMultiplier;

    // 1. Mettre à jour les positions des entités
    updatePlayerPositions(scaledDt);
    updateMissiles(scaledDt);
    updateEnemies(scaledDt);
    updateEnemyMissiles(scaledDt);

    // 2. Mettre à jour les systèmes
    updateForcePods(scaledDt);
    updateBitDevices(scaledDt);
    updatePowerUps(scaledDt);

    // 3. Vérifier les collisions
    checkCollisions();

    // 4. Mettre à jour le boss si présent
    if (_boss.has_value()) {
        updateBoss(scaledDt);
    }

    // 5. Gérer les vagues
    updateWaveSpawning(scaledDt);

    // 6. Mettre à jour les combos
    updateComboTimers(scaledDt);
}

protocol::GameSnapshot GameWorld::getSnapshot() const {
    protocol::GameSnapshot snap;

    // Joueurs
    snap.player_count = static_cast<uint8_t>(_players.size());
    for (const auto& [id, player] : _players) {
        snap.players[snap.player_count++] = player.toState();
    }

    // Missiles
    snap.missile_count = 0;
    for (const auto& [id, missile] : _missiles) {
        snap.missiles[snap.missile_count++] = missile.toState();
    }

    // Ennemis, boss, etc.
    // ...

    return snap;
}
```

### Code du Client

#### GameScene - Rendu et Input

**Fichier** : `src/client/src/scenes/GameScene.cpp`

```cpp
void GameScene::handleEvent(const events::Event& event) {
    auto& config = AccessibilityConfig::getInstance();

    // Gestion inputs avec remapping
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (config.isActionKey(GameAction::Shoot, keyPressed->key)) {
            shootMissile();
        }
        if (config.isActionKey(GameAction::Pause, keyPressed->key)) {
            togglePause();
        }
        if (config.isActionKey(GameAction::OpenChat, keyPressed->key)) {
            openChatInput();
        }
        // Mouvement
        if (config.isActionKey(GameAction::MoveUp, keyPressed->key)) {
            _inputState.up = true;
        }
        // ...
    }
}

void GameScene::update(float deltaTime) {
    // Appliquer prédiction client
    if (_inputState.up) _localY -= MOVE_SPEED * deltaTime;
    if (_inputState.down) _localY += MOVE_SPEED * deltaTime;
    if (_inputState.left) _localX -= MOVE_SPEED * deltaTime;
    if (_inputState.right) _localX += MOVE_SPEED * deltaTime;

    // Envoyer inputs au serveur
    sendInputToServer();

    // Appliquer snapshot serveur (réconciliation)
    applyServerSnapshot();

    // Mettre à jour animations
    updateAnimations(deltaTime);

    // Mettre à jour chat
    updateChatMessages(deltaTime);
}

void GameScene::render() {
    renderBackground();
    renderEnemies();
    renderMissiles();
    renderPlayers();
    renderPowerUps();
    renderForcePods();

    if (_boss.has_value()) {
        renderBoss();
        renderBossHealthBar();
    }

    renderHUD();
    renderChatOverlay();

    if (_paused) {
        renderPauseOverlay();
    }
}
```

### Environnement d'Exécution

#### Langages Utilisés

| Composant | Langage | Standard | Justification |
|-----------|---------|----------|---------------|
| Serveur | C++ | C++23 | Performance, contrôle mémoire |
| Client | C++ | C++23 | Performance graphique |
| Discord Bots | TypeScript | ES2022 | Écosystème Discord.js |
| Scripts | Bash | POSIX | Automatisation build/deploy |

#### Configuration Build (CMake)

```cmake
cmake_minimum_required(VERSION 3.30)
project(rtype CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dépendances vcpkg
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(mongocxx REQUIRED)
find_package(SFML REQUIRED COMPONENTS graphics window system audio)
```

---

## Observable 13.2 : Conventions de Formatage et de Nommage

### Conventions de Nommage

| Type | Convention | Exemples |
|------|------------|----------|
| **Classes** | PascalCase | `GameWorld`, `UDPServer`, `Button` |
| **Interfaces** | I + PascalCase | `IWindow`, `IScene`, `IUIElement` |
| **Méthodes** | camelCase | `handleEvent()`, `getSnapshot()` |
| **Membres privés** | _camelCase | `_players`, `_missiles` |
| **Constantes** | SCREAMING_SNAKE_CASE | `MAX_PLAYERS`, `MOVE_SPEED` |
| **Namespaces** | lowercase | `domain::entities`, `events` |
| **Enums** | PascalCase | `enum class State { Waiting, InGame }` |
| **Fichiers** | PascalCase.hpp/.cpp | `GameWorld.hpp`, `UDPServer.cpp` |

### Exemples de Code Conforme

#### Structure des Headers

```cpp
/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameWorld
*/

#ifndef GAMEWORLD_HPP_
#define GAMEWORLD_HPP_

#include <unordered_map>
#include <vector>
#include <optional>
#include "../../common/protocol/Protocol.hpp"
#include "../../common/collision/AABB.hpp"

namespace infrastructure::game {

class GameWorld {
public:
    void update(float deltaTime);
    protocol::GameSnapshot getSnapshot() const;

private:
    std::unordered_map<uint8_t, ConnectedPlayer> _players;
    std::optional<Boss> _boss;

    static constexpr float MOVE_SPEED = 200.0f;
    static constexpr uint8_t MAX_PLAYERS = 4;
};

}  // namespace infrastructure::game

#endif /* !GAMEWORLD_HPP_ */
```

#### Constructeur avec Initializer List

**Fichier** : `src/client/src/ui/Button.cpp`

```cpp
Button::Button(const Vec2f& pos, const Vec2f& size,
               const std::string& text, const std::string& fontKey)
    : _pos(pos)          // Alignement vertical
    , _size(size)        // Virgule en début de ligne
    , _text(text)
    , _fontKey(fontKey)
    , _state(State::Normal)
    , _focused(false)
{
}
```

#### Gestion des Événements avec variant

```cpp
void Button::handleEvent(const events::Event& event) {
    if (!_enabled) {
        _state = State::Disabled;
        return;
    }

    // Pattern matching avec std::get_if
    if (auto* moved = std::get_if<events::MouseMoved>(&event)) {
        bool isHovered = contains(static_cast<float>(moved->x),
                                  static_cast<float>(moved->y));
        _state = isHovered ? State::Hovered : State::Normal;
    }

    if (auto* pressed = std::get_if<events::MouseButtonPressed>(&event)) {
        if (pressed->button == events::MouseButton::Left &&
            contains(static_cast<float>(pressed->x),
                     static_cast<float>(pressed->y))) {
            _state = State::Pressed;
            if (_onClick) {
                _onClick();
            }
        }
    }
}
```

#### Constantes Nommées

**Fichier** : `src/client/include/scenes/GameScene.hpp`

```cpp
// Constantes de gameplay
static constexpr float MOVE_SPEED = 200.0f;
static constexpr float SHIP_WIDTH = 64.0f;
static constexpr float SHIP_HEIGHT = 30.0f;
static constexpr float SHOOT_COOLDOWN_TIME = 0.3f;
static constexpr uint8_t MAX_HEALTH = 100;

// Constantes UI
static constexpr float HUD_HEALTH_BAR_WIDTH = 200.0f;
static constexpr float HUD_MARGIN = 20.0f;
static constexpr unsigned int LABEL_FONT_SIZE = 14;

// Constantes écran
static constexpr float SCREEN_WIDTH = 1920.0f;
static constexpr float SCREEN_HEIGHT = 1080.0f;

// Clés de ressources
static constexpr const char* SHIP_TEXTURE_KEY = "ship";
static constexpr const char* FONT_KEY = "main";
```

### Formatage du Code

#### Indentation et Espacement

```cpp
// Accolades sur ligne suivante pour classes/fonctions
class GameWorld
{
public:
    void update(float deltaTime);

private:
    std::unordered_map<uint8_t, Player> _players;
};

// Espaces autour des opérateurs
float scaledDt = deltaTime * _gameSpeedMultiplier;

// Pas d'espace après nom de fonction
void handleEvent(const events::Event& event);

// Espace après keywords
if (condition) {
    // ...
}

for (const auto& item : collection) {
    // ...
}
```

#### Commentaires

```cpp
// Commentaire sur une ligne pour explications courtes

/*
 * Commentaire multi-lignes pour
 * explications plus longues
 */

/// Documentation Doxygen pour API publique
/// @param deltaTime Temps écoulé depuis la dernière frame
/// @return Snapshot de l'état de jeu
protocol::GameSnapshot getSnapshot() const;
```

### Tableau Récapitulatif

| Aspect | Convention | Exemple |
|--------|------------|---------|
| Indentation | 4 espaces | Standard projet |
| Accolades | Nouvelle ligne (classes) | `class Foo\n{` |
| Pointeurs/Refs | Collés au type | `const std::string& name` |
| Include guards | FILENAME_HPP_ | `#ifndef GAMEWORLD_HPP_` |
| Namespaces | Pas d'indentation | `namespace foo {\nclass Bar;` |
| Lignes max | ~100 caractères | Lisibilité |

### Conclusion

Le code R-Type respecte :

1. **Exigences fonctionnelles** : Multijoueur, auth, classements, voice, social
2. **Langage adapté** : C++23 pour performance et modernité
3. **Conventions strictes** : PascalCase classes, _camelCase membres, SCREAMING_SNAKE constantes
4. **Lisibilité** : Indentation cohérente, commentaires pertinents, noms explicites

Ces pratiques garantissent un code **maintenable**, **lisible** et **professionnel**.
