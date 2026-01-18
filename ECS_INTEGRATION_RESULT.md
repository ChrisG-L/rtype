# ECS Integration - Result & Status

> **Référence technique** de l'intégration ECS dans R-Type.
> Plan: `ECS_INTEGRATION_PLAN.md`

---

## Current Status

| Phase | Status | Description |
|-------|--------|-------------|
| **Phase 0** | ✅ | Domain Services (96 tests) |
| **Phase 1** | ✅ | ECS Core + Components + DomainBridge (80 tests) |
| **Phase 2** | ✅ | Systems de base (80 tests) |
| **Phase 3** | ✅ | Systems avancés (90 tests) |
| **Phase 4** | ✅ | GameWorld migration (4.6 ✅, 4.8 ✅) |
| **Phase 5.1** | ✅ | LifetimeSystem + CleanupSystem (missiles) |
| **Phase 5.2** | ✅ | CollisionSystem (detection) |
| **Phase 5.3** | ✅ | DamageSystem (MISSILES ↔ ENEMIES) |

**Total**: 310+ tests | **Branch**: `ECS_realImpl` | **Updated**: 2026-01-18

---

## Phase 4 - GameWorld Migration

### Sub-Phase Status

| Step | Description | Status |
|------|-------------|--------|
| 4.1 | `initializeECS()` - 15 components | ✅ |
| 4.1 | `registerSystems()` - 9 Systems | ✅ |
| 4.2 | `addPlayer()` → ECS entities | ✅ |
| 4.3 | `spawnMissile()` → ECS entities | ✅ |
| 4.4 | `spawnEnemy()` → ECS entities | ✅ |
| 4.5 | CMakeLists.txt updated | ✅ |
| 4.6 | `getSnapshot()` from ECS | ✅ Complete |
| 4.7 | `runECSUpdate()` drives movement | ✅ |
| 4.8 | Integration tests | ✅ Complete |

### Current Architecture (Phase 5.3)

```
UDPServer.updateAndBroadcastRoom()
    │
    ├── runECSUpdate(deltaTime)           # ECS drives core gameplay
    │   ├── _ecs.Update(msecs)            # All active systems
    │   │   ├── PlayerInputSystem         # Input → Velocity
    │   │   ├── MovementSystem            # Position += Velocity × dt
    │   │   ├── CollisionSystem           # Detect AABB collisions
    │   │   ├── DamageSystem              # Apply damage, delete dead entities
    │   │   ├── LifetimeSystem            # Expire timed entities
    │   │   └── CleanupSystem             # Remove OOB entities
    │   ├── syncPlayersFromECS()          # ECS positions → legacy _players
    │   ├── syncDeletedMissilesFromECS()  # Remove destroyed missiles from legacy
    │   ├── processECSKillEvents()        # Award score, spawn power-ups
    │   └── syncDeletedEnemiesFromECS()   # Remove dead enemies from legacy
    │
    ├── updateShootCooldowns()            # Legacy
    ├── updateMissiles()                  # Homing velocity + sync positions from ECS
    ├── updateEnemies()                   # Legacy (complex patterns, OOB)
    ├── checkCollisions()                 # Legacy: enemy missiles→players, missiles→boss
    └── getSnapshot()                     # Players from ECS, rest from legacy
```

**Active Systems (Phase 5.3)**: 6/9
- PlayerInputSystem (0), MovementSystem (300), CollisionSystem (400)
- DamageSystem (500), LifetimeSystem (600), CleanupSystem (700)

**Disabled Systems**: EnemyAISystem (100), WeaponSystem (200), ScoreSystem (800)

**Data Flow (Phase 5.3)**:
1. `applyPlayerInput()` → queues to PlayerInputSystem
2. `runECSUpdate()` → Systems process:
   - CollisionSystem detects missile↔enemy collisions
   - DamageSystem applies damage, deletes entities, produces KillEvents
   - LifetimeSystem/CleanupSystem remove expired/OOB entities
3. `processECSKillEvents()` → awards score, spawns power-ups
4. `syncDeletedMissilesFromECS()` + `syncDeletedEnemiesFromECS()` → sync to legacy maps
5. Legacy `checkCollisions()` handles: enemy missiles→players, missiles→boss
6. `getSnapshot()` reads players from ECS, missiles/enemies from legacy

---

## Components (15 total)

### Phase 1 - Base (6)

| Component | Fields | Purpose |
|-----------|--------|---------|
| `PositionComp` | `float x, y` | All positioned entities |
| `VelocityComp` | `float x, y` | Moving entities |
| `HealthComp` | `uint16_t current, max; bool invulnerable` | Destructible entities |
| `HitboxComp` | `float width, height, offsetX, offsetY` | Collision AABB |
| `LifetimeComp` | `float remaining, total` | Temporary entities |
| `OwnerComp` | `uint32_t ownerId; bool isPlayerOwned` | Projectiles, pods |

### Phase 2 - Tags (5)

| Component | Fields | Purpose |
|-----------|--------|---------|
| `MissileTag` | `weaponType, baseDamage, isHoming, targetId` | Missile ID |
| `EnemyTag` | `type, points` | Enemy ID |
| `EnemyAIComp` | `shootCooldown, shootInterval, movementPattern, patternTimer, baseY, aliveTime, phaseOffset, targetY, zigzagTimer, zigzagUp` | AI state |
| `PowerUpTag` | `type` | Power-up ID |
| `WaveCannonTag` | `chargeLevel, width` | Wave Cannon |

### Phase 3 - Player (4)

| Component | Fields | Purpose |
|-----------|--------|---------|
| `PlayerTag` | `playerId, shipSkin, isAlive` | Player ID |
| `ScoreComp` | `total, kills, comboMultiplier, comboTimer, maxCombo, deaths` | Score |
| `WeaponComp` | `currentType, shootCooldown, isCharging, chargeTime, weaponLevels[4]` | Weapons |
| `SpeedLevelComp` | `level` (0-3) | Speed upgrade |

---

## Systems (9 total)

| Priority | System | Query | Responsibility |
|----------|--------|-------|----------------|
| 0 | `PlayerInputSystem` | `<PlayerTag, Position, Velocity, SpeedLevel>` | Input → Velocity, screen clamp |
| 100 | `EnemyAISystem` | `<EnemyTag, EnemyAIComp, Position, Velocity>` | Movement patterns, shooting |
| 200 | `WeaponSystem` | `<PlayerTag, WeaponComp, Position>` | Cooldowns, missile spawn |
| 300 | `MovementSystem` | `<Position, Velocity>` | `pos += vel × dt` |
| 400 | `CollisionSystem` | `<Position, Hitbox>` | O(n²) AABB detection |
| 500 | `DamageSystem` | Consumes `CollisionEvent` | Apply damage via DomainBridge |
| 600 | `LifetimeSystem` | `<Lifetime>` | Decrement, mark expired |
| 700 | `CleanupSystem` | `<Position, Hitbox>` | Remove OOB (not players) |
| 800 | `ScoreSystem` | `<PlayerTag, ScoreComp>` | Combo decay, score calc |

---

## DomainBridge API

```cpp
class DomainBridge {
    // Damage & Health
    DamageResult applyDamage(HealthComp& health, uint16_t damage);
    bool wouldDie(uint16_t currentHealth, uint16_t damage);

    // Score & Combo
    uint32_t calculateKillScore(uint16_t basePoints, float comboMultiplier);
    uint16_t getEnemyPoints(uint8_t enemyType);
    float incrementCombo(float currentCombo);
    float getComboGraceTime();

    // Weapon
    uint8_t getMissileDamage(uint8_t weaponType, uint8_t level);
    float getMissileSpeed(uint8_t weaponType, uint8_t level);
    float getWeaponCooldown(uint8_t weaponType, uint8_t level);

    // Player
    float getPlayerSpeed(uint8_t speedLevel);

    // Collision
    bool checkCollision(float x1, y1, w1, h1, float x2, y2, w2, h2);
    bool isOutOfBounds(float x, y, w, h);
    void clampToScreen(float& x, float& y, float w, h);

    // Enemy
    bool canEnemyShoot(float cooldownRemaining);
    float getEnemyShootInterval(uint8_t enemyType);
    std::pair<float, float> getEnemyMovement(...);
};
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| O(n²) collision | <100 entities typical, spatial hash if >500 |
| CollisionEvent with groups | DamageSystem filters without re-query |
| KillEvent for score | Decouples DamageSystem from ScoreSystem |
| Players excluded from Cleanup | Death/respawn managed by GameWorld |
| Deferred entity deletion | Avoids collection modification during iteration |
| Independent weapon levels | R-Type authentic: each weapon upgrades separately |
| SpeedLevelComp separate | Can be queried/modified independently |

---

## File Structure

```
src/server/infrastructure/ecs/
├── core/
│   ├── ECS.hpp, Component.hpp, Registry.hpp
│   ├── System.hpp, Includes.hpp, Errors.hpp
├── components/
│   ├── PositionComp.hpp, VelocityComp.hpp, HealthComp.hpp
│   ├── HitboxComp.hpp, LifetimeComp.hpp, OwnerComp.hpp
│   ├── MissileTag.hpp, EnemyTag.hpp, EnemyAIComp.hpp
│   ├── PowerUpTag.hpp, WaveCannonTag.hpp
│   ├── PlayerTag.hpp, ScoreComp.hpp, WeaponComp.hpp
│   └── SpeedLevelComp.hpp
├── systems/
│   ├── MovementSystem.hpp/.cpp
│   ├── LifetimeSystem.hpp/.cpp, CleanupSystem.hpp/.cpp
│   ├── CollisionSystem.hpp/.cpp, DamageSystem.hpp/.cpp
│   ├── PlayerInputSystem.hpp/.cpp, WeaponSystem.hpp/.cpp
│   ├── ScoreSystem.hpp/.cpp, EnemyAISystem.hpp/.cpp
└── bridge/
    ├── DomainBridge.hpp/.cpp

tests/server/ecs/
├── ComponentPoolTest.cpp, ECSIntegrationTest.cpp
├── DomainBridgeTest.cpp, ComponentTagsTest.cpp
├── PlayerComponentsTest.cpp, ECSPhase2IntegrationTest.cpp
└── systems/
    ├── MovementSystemTest.cpp, LifetimeSystemTest.cpp
    ├── CleanupSystemTest.cpp, CollisionSystemTest.cpp
    ├── DamageSystemTest.cpp, PlayerInputSystemTest.cpp
    ├── WeaponSystemTest.cpp, ScoreSystemTest.cpp
    └── EnemyAISystemTest.cpp
```

---

## How to Enable ECS

```bash
# Configure with ECS
cmake -B build -DUSE_ECS_BACKEND=ON

# Compile
./scripts/compile.sh

# Run all ECS tests (310+)
./artifacts/tests/server_tests --gtest_filter="*ECS*:*Component*:*DomainBridge*:*Movement*:*Collision*:*Player*:*Weapon*:*Score*:*EnemyAI*"

# Phase-specific tests
./artifacts/tests/server_tests --gtest_filter="*ECSPhase2*"  # Phase 2
./artifacts/tests/server_tests --gtest_filter="*PlayerInput*:*Weapon*:*Score*:*EnemyAI*"  # Phase 3
```

---

## Next Steps

### Phase 4 Complete

| Task | Status |
|------|--------|
| Phase 4.6: `getSnapshot()` reads from ECS | ✅ Complete |
| Phase 4.8: Integration tests | ✅ Complete |

### Next: Phase 5+

| Priority | Task |
|----------|------|
| Medium | Enable remaining Systems progressively |
| Medium | Remove legacy `_players`, `_missiles`, `_enemies` maps |
| Low | Spatial hashing for CollisionSystem |
| Low | ForcePodSystem, BitDeviceSystem, BossSystem |

---

## Resolved Issues

| Issue | Resolution |
|-------|------------|
| `ECS::ECS` incomplete type | Added `#include "core/ECS.hpp"` to .cpp files |
| Entity ID 0 as "not found" | `findPlayerByID()` returns `std::optional<EntityID>` |
| Screen width 800 in tests | Updated to 1920 from Constants.hpp |
| MongoDB socket crash on reconnect | Migrated from single `mongocxx::client` to `mongocxx::pool` for thread-safety |
| Session use-after-free on shutdown | Removed `_onClose` from Session destructor, only call on real errors |
| ECS memory leak on shutdown | Fixed `ECS::shutdown()` to properly delete heap-allocated components |

---

## References

- **Plan**: `ECS_INTEGRATION_PLAN.md`
- **Original ECS**: `src/ECS/` (Guillaume's Blob ECS)
- **Project Context**: `CLAUDE.md`
