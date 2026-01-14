---
tags:
  - technique
  - gameplay
  - collisions
---

# Système de Collisions

Détection AABB (Axis-Aligned Bounding Box).

## AABB

```cpp
struct AABB {
    float x, y;       // Position (top-left)
    float w, h;       // Dimensions

    bool intersects(const AABB& other) const {
        return x < other.x + other.w &&
               x + w > other.x &&
               y < other.y + other.h &&
               y + h > other.y;
    }

    bool contains(float px, float py) const {
        return px >= x && px < x + w &&
               py >= y && py < y + h;
    }

    AABB expanded(float margin) const {
        return {x - margin, y - margin, w + margin*2, h + margin*2};
    }
};
```

---

## Visualisation

```
    x,y ─────────────────┐
     │                   │
     │      AABB         │ h
     │                   │
     └───────────────────┘
              w
```

Intersection :
```
  ┌─────────┐
  │    A    │
  │    ┌────┼────┐
  └────┼────┘    │
       │    B    │
       └─────────┘
     Intersection!
```

---

## Détection dans GameWorld

```cpp
void GameWorld::checkCollisions() {
    // Missiles joueurs vs Ennemis
    for (auto& missile : playerMissiles_) {
        for (auto& enemy : enemies_) {
            if (missile.getAABB().intersects(enemy.getAABB())) {
                enemy.takeDamage(missile.damage_);
                missile.destroy();

                if (!enemy.isAlive()) {
                    addScore(enemy.getScoreValue());
                    broadcastDestroy(enemy.id_);
                }
                break;
            }
        }
    }

    // Missiles ennemis vs Joueurs
    for (auto& missile : enemyMissiles_) {
        for (auto& player : players_) {
            if (!player.isAlive()) continue;

            if (missile.getAABB().intersects(player.getAABB())) {
                player.takeDamage(missile.damage_);
                missile.destroy();
                broadcastPlayerDamaged(player.id_, missile.damage_);

                if (!player.isAlive()) {
                    broadcastPlayerDied(player.id_);
                }
                break;
            }
        }
    }

    // Ennemis vs Joueurs (contact)
    for (auto& enemy : enemies_) {
        for (auto& player : players_) {
            if (!player.isAlive()) continue;

            if (enemy.getAABB().intersects(player.getAABB())) {
                player.takeDamage(CONTACT_DAMAGE);
                broadcastPlayerDamaged(player.id_, CONTACT_DAMAGE);

                if (!player.isAlive()) {
                    broadcastPlayerDied(player.id_);
                }
            }
        }
    }
}
```

---

## Optimisation : Spatial Partitioning

Pour beaucoup d'entités, utiliser une grille :

```cpp
class SpatialGrid {
    static constexpr int CELL_SIZE = 128;
    static constexpr int GRID_WIDTH = WORLD_WIDTH / CELL_SIZE;
    static constexpr int GRID_HEIGHT = WORLD_HEIGHT / CELL_SIZE;

    std::vector<Entity*> cells_[GRID_WIDTH][GRID_HEIGHT];

public:
    void clear() {
        for (int x = 0; x < GRID_WIDTH; x++)
            for (int y = 0; y < GRID_HEIGHT; y++)
                cells_[x][y].clear();
    }

    void insert(Entity* entity) {
        auto aabb = entity->getAABB();
        int x1 = std::max(0, (int)(aabb.x / CELL_SIZE));
        int y1 = std::max(0, (int)(aabb.y / CELL_SIZE));
        int x2 = std::min(GRID_WIDTH-1, (int)((aabb.x + aabb.w) / CELL_SIZE));
        int y2 = std::min(GRID_HEIGHT-1, (int)((aabb.y + aabb.h) / CELL_SIZE));

        for (int x = x1; x <= x2; x++)
            for (int y = y1; y <= y2; y++)
                cells_[x][y].push_back(entity);
    }

    std::vector<Entity*> query(const AABB& area) {
        std::set<Entity*> result;
        int x1 = std::max(0, (int)(area.x / CELL_SIZE));
        int y1 = std::max(0, (int)(area.y / CELL_SIZE));
        int x2 = std::min(GRID_WIDTH-1, (int)((area.x + area.w) / CELL_SIZE));
        int y2 = std::min(GRID_HEIGHT-1, (int)((area.y + area.h) / CELL_SIZE));

        for (int x = x1; x <= x2; x++)
            for (int y = y1; y <= y2; y++)
                for (auto* e : cells_[x][y])
                    result.insert(e);

        return {result.begin(), result.end()};
    }
};
```

---

## Complexité

| Approche | Complexité | Entités |
|----------|------------|---------|
| Brute force | O(n²) | < 100 |
| Spatial grid | O(n) | 100-1000 |
| Quadtree | O(n log n) | 1000+ |

Pour R-Type (~50 entités max), **brute force** suffit.

---

## Hitboxes

| Entité | Largeur | Hauteur |
|--------|---------|---------|
| Player | 64 | 32 |
| Basic Enemy | 48 | 32 |
| Zigzag Enemy | 48 | 32 |
| Follower Enemy | 40 | 40 |
| Shooter Enemy | 56 | 40 |
| Boss | 256 | 128 |
| Player Missile | 16 | 8 |
| Enemy Missile | 12 | 6 |
