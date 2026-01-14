---
tags:
  - api
  - serveur
  - gameplay
---

# GameWorld

Simulation du monde de jeu côté serveur.

## Synopsis

```cpp
#include "server/GameWorld.hpp"

GameWorld world;
world.addPlayer(playerId, spawnPosition);

// Game loop
while (running) {
    world.processInput(playerId, input);
    world.tick();
    auto snapshot = world.getSnapshot();
    broadcast(snapshot);
}
```

---

## Déclaration

```cpp
namespace rtype::server {

class GameWorld {
public:
    static constexpr float TICK_RATE = 60.0f;
    static constexpr float TICK_DURATION = 1.0f / TICK_RATE;
    static constexpr float WORLD_WIDTH = 1920.0f;
    static constexpr float WORLD_HEIGHT = 1080.0f;

    GameWorld();
    ~GameWorld();

    // Player management
    void addPlayer(PlayerId id, Vector2f position);
    void removePlayer(PlayerId id);
    Player* getPlayer(PlayerId id);
    const std::vector<Player>& players() const;

    // Input processing
    void queueInput(PlayerId playerId, const Input& input);
    void processInput(PlayerId playerId, const Input& input);

    // Simulation
    void tick();
    Tick currentTick() const;

    // State
    GameSnapshot getSnapshot() const;
    GameState state() const;
    void setState(GameState state);

    // Queries
    Player* findNearestPlayer(float x, float y);
    std::vector<Entity*> getEntitiesInRadius(Vector2f center, float r);

    // Spawning
    void spawnEnemy(EnemyType type);
    void spawnMissile(PlayerId owner, Vector2f pos, Vector2f vel);
    void spawnEnemyMissile(Vector2f pos, Vector2f vel);

    // Events
    void setVictory();
    void setGameOver();

private:
    void processInputs();
    void updatePhysics();
    void updateEnemies();
    void checkCollisions();
    void cleanup();

    Tick currentTick_ = 0;
    GameState state_ = GameState::Playing;

    std::vector<Player> players_;
    std::vector<Enemy> enemies_;
    std::vector<Missile> playerMissiles_;
    std::vector<Missile> enemyMissiles_;

    WaveManager waveManager_;
    EntityIdGenerator idGenerator_;
};

} // namespace rtype::server
```

---

## Types

### GameState

```cpp
enum class GameState {
    Playing,    // Partie en cours
    Victory,    // Victoire (boss vaincu)
    GameOver    // Défaite (tous morts)
};
```

### GameSnapshot

```cpp
struct GameSnapshot {
    Tick tick;

    // Players (max 4)
    uint8_t playerCount;
    PlayerState players[4];

    // Enemies (max 50)
    uint8_t enemyCount;
    EnemyState enemies[50];

    // Missiles (max 100)
    uint8_t missileCount;
    MissileState missiles[100];
};
```

---

## Méthodes

### `tick()`

```cpp
void tick();
```

Avance la simulation d'un tick (1/60 seconde).

**Ordre d'exécution:**

1. `processInputs()` - Traite les inputs en attente
2. `updatePhysics()` - Met à jour positions
3. `updateEnemies()` - IA des ennemis
4. `checkCollisions()` - Détection collisions
5. `cleanup()` - Supprime entités mortes

```cpp
void GameWorld::tick() {
    processInputs();
    updatePhysics();
    updateEnemies();
    checkCollisions();
    cleanup();
    currentTick_++;
}
```

---

### `processInput()`

```cpp
void processInput(PlayerId playerId, const Input& input);
```

Applique un input joueur immédiatement.

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `playerId` | `PlayerId` | ID du joueur |
| `input` | `Input` | Input à appliquer |

**Exemple:**

```cpp
void GameWorld::processInput(PlayerId id, const Input& input) {
    auto* player = getPlayer(id);
    if (!player || !player->isAlive()) return;

    const float SPEED = 300.0f;

    if (input.keys & KEY_UP)
        player->y -= SPEED * TICK_DURATION;
    if (input.keys & KEY_DOWN)
        player->y += SPEED * TICK_DURATION;
    if (input.keys & KEY_LEFT)
        player->x -= SPEED * TICK_DURATION;
    if (input.keys & KEY_RIGHT)
        player->x += SPEED * TICK_DURATION;

    // Clamp bounds
    player->x = std::clamp(player->x, 0.0f, WORLD_WIDTH);
    player->y = std::clamp(player->y, 0.0f, WORLD_HEIGHT);

    // Shooting
    if ((input.keys & KEY_SHOOT) && player->canShoot()) {
        spawnMissile(id, {player->x + 64, player->y + 16},
                    {600.0f, 0.0f});
        player->shoot();
    }

    player->lastAckedInput = input.sequence;
}
```

---

### `getSnapshot()`

```cpp
GameSnapshot getSnapshot() const;
```

Génère un snapshot de l'état actuel.

**Retour:** `GameSnapshot` contenant l'état complet du jeu

**Note:** Le snapshot est envoyé aux clients à chaque tick.

---

### `checkCollisions()`

```cpp
void checkCollisions();
```

Vérifie toutes les collisions.

**Types de collisions:**

- Missiles joueurs vs Ennemis
- Missiles ennemis vs Joueurs
- Ennemis vs Joueurs (contact)

```cpp
void GameWorld::checkCollisions() {
    // Player missiles vs enemies
    for (auto& missile : playerMissiles_) {
        if (!missile.isAlive()) continue;

        for (auto& enemy : enemies_) {
            if (!enemy.isAlive()) continue;

            if (missile.getAABB().intersects(enemy.getAABB())) {
                enemy.takeDamage(missile.damage());
                missile.destroy();

                if (!enemy.isAlive()) {
                    // Score event
                    onEnemyKilled(enemy);
                }
                break;
            }
        }
    }

    // Enemy missiles vs players
    // Enemies vs players (contact)
    // ...
}
```

---

## Diagramme de Tick

```mermaid
flowchart TB
    Start[tick()] --> Inputs[processInputs]
    Inputs --> Physics[updatePhysics]
    Physics --> AI[updateEnemies]
    AI --> Collision[checkCollisions]
    Collision --> Cleanup[cleanup]
    Cleanup --> Increment[currentTick_++]
    Increment --> End[return]

    style Start fill:#7c3aed,color:#fff
    style End fill:#7c3aed,color:#fff
```

---

## Wave Manager Integration

```cpp
class GameWorld {
    WaveManager waveManager_;

    void updateEnemies() {
        // Update AI
        for (auto& enemy : enemies_) {
            enemy.updateAI(*this);
        }

        // Wave progression
        waveManager_.update(*this, TICK_DURATION);
    }
};
```

---

## Constantes

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `TICK_RATE` | 60.0 | Ticks par seconde |
| `TICK_DURATION` | 0.0167 | Durée d'un tick (s) |
| `WORLD_WIDTH` | 1920.0 | Largeur monde (px) |
| `WORLD_HEIGHT` | 1080.0 | Hauteur monde (px) |
| `MAX_PLAYERS` | 4 | Joueurs maximum |
| `MAX_ENEMIES` | 50 | Ennemis maximum |

---

## Thread Safety

`GameWorld` n'est **PAS** thread-safe. Il est conçu pour être utilisé depuis un seul thread (le thread de simulation de la room).
