# ECS Integration - Result & Status

> **Document de suivi** de l'intégration ECS dans le projet R-Type.
> Référence : `ECS_INTEGRATION_PLAN.md`

---

## Table of Contents

1. [Current Status](#current-status)
2. [Phase 0 - Domain Services](#phase-0---domain-services)
3. [Phase 1 - ECS Core & Components](#phase-1---ecs-core--components)
4. [Phase 2 - Systems de Base](#phase-2---systems-de-base)
5. [Phase 3 - Entités Complexes & Systems Avancés](#phase-3---entités-complexes--systems-avancés)
6. [Implementation Choices & Deviations](#implementation-choices--deviations)
7. [File Mapping](#file-mapping)
8. [How to Enable ECS](#how-to-enable-ecs)
9. [Next Steps](#next-steps)
10. [Known Issues & TODOs](#known-issues--todos)

---

## Current Status

| Phase | Status | Description |
|-------|--------|-------------|
| **Phase 0** | ✅ Complete | Domain Services extraction |
| **Phase 1** | ✅ Complete | ECS Core + Components + DomainBridge |
| **Phase 2** | ✅ Complete | ECS Systems (Movement, Collision, Damage, etc.) |
| **Phase 3** | ✅ Complete | Player Components + Advanced Systems |
| **Phase 4** | ⏳ Pending | Full GameWorld migration |

**Last Updated:** 2026-01-18
**Current Branch:** `ECS_realImpl`

---

## Phase 0 - Domain Services

### Completed Items

| Service | File | Description |
|---------|------|-------------|
| GameRule | `domain/services/GameRule.hpp/.cpp` | Damage, score, combo, weapon stats |
| CollisionRule | `domain/services/CollisionRule.hpp/.cpp` | AABB collision detection |
| EnemyBehavior | `domain/services/EnemyBehavior.hpp/.cpp` | Enemy movement patterns |
| Constants | `domain/Constants.hpp` | Centralized game constants |

### Test Coverage

| Test File | Tests | Status |
|-----------|-------|--------|
| `GameRuleTest.cpp` | 30+ | ✅ Pass |
| `CollisionRuleTest.cpp` | 20+ | ✅ Pass |
| `EnemyBehaviorTest.cpp` | 30+ | ✅ Pass |

### Key Structures Created

```cpp
// CollisionRule.hpp
struct HitboxData {
    float x, y, width, height;
};

// EnemyBehavior.hpp
struct ZigzagState {
    float timer = 0.0f;
    bool movingUp = true;
};

struct EnemyMovementInput {
    uint8_t enemyType;
    float currentY, baseY, aliveTime, phaseOffset, deltaTime, targetY;
    ZigzagState zigzag;
};

struct EnemyMovementOutput {
    float dx, dy;
    ZigzagState updatedZigzag;
};
```

---

## Phase 1 - ECS Core & Components

### ECS Core Files

| File | Purpose |
|------|---------|
| `core/ECS.hpp` | Main ECS class, entity management |
| `core/Component.hpp` | ComponentPool with sparse sets |
| `core/Registry.hpp` | Component type registration |
| `core/System.hpp` | ISystem interface |
| `core/Includes.hpp` | Common includes, EntityGroup enum |
| `core/Errors.hpp` | ECS exceptions |

### Base Components (Phase 1)

| Component | Fields | Default Values |
|-----------|--------|----------------|
| `PositionComp` | `float x, y` | `0.0f, 0.0f` |
| `VelocityComp` | `float x, y` | `0.0f, 0.0f` |
| `HealthComp` | `uint16_t current, max; bool invulnerable` | `100, 100, false` |
| `HitboxComp` | `float width, height, offsetX, offsetY` | `0.0f, 0.0f, 0.0f, 0.0f` |
| `LifetimeComp` | `float remaining, total` | `0.0f, 0.0f` |
| `OwnerComp` | `uint32_t ownerId; bool isPlayerOwned` | `0, true` |

### DomainBridge

The DomainBridge encapsulates all three Domain services and provides a unified interface for ECS Systems.

```cpp
class DomainBridge {
public:
    DomainBridge(GameRule&, CollisionRule&, EnemyBehavior&);

    // Damage & Health
    DamageResult applyDamage(HealthComp& health, uint16_t damage);
    bool wouldDie(uint16_t currentHealth, uint16_t damage);

    // Score & Combo
    uint32_t calculateKillScore(uint16_t basePoints, float comboMultiplier);
    uint16_t getEnemyPoints(uint8_t enemyType);
    float incrementCombo(float currentCombo);
    float getComboGraceTime();

    // Weapon Stats
    uint8_t getMissileDamage(uint8_t weaponType, uint8_t level);
    float getMissileSpeed(uint8_t weaponType, uint8_t level);
    float getWeaponCooldown(uint8_t weaponType, uint8_t level);

    // Player Movement
    float getPlayerSpeed(uint8_t speedLevel);

    // Collision
    bool checkCollision(float x1, float y1, float w1, float h1,
                        float x2, float y2, float w2, float h2);
    bool isOutOfBounds(float x, float y, float w, float h);
    void clampToScreen(float& x, float& y, float w, float h);

    // Enemy Behavior
    bool canEnemyShoot(float cooldownRemaining);
    float getEnemyShootInterval(uint8_t enemyType);
    std::pair<float, float> getEnemyMovement(...);
};
```

### Test Coverage (Phase 1)

| Test File | Tests | Status |
|-----------|-------|--------|
| `ComponentPoolTest.cpp` | 20+ | ✅ Pass |
| `ECSIntegrationTest.cpp` | 25+ | ✅ Pass |
| `DomainBridgeTest.cpp` | 35+ | ✅ Pass |

---

## Phase 2 - Systems de Base

### New Components Created (Phase 2)

| Component | Fields | Purpose |
|-----------|--------|---------|
| `MissileTag` | `weaponType, baseDamage, isHoming, targetId` | Missile identification |
| `EnemyTag` | `type, points` | Enemy identification |
| `EnemyAIComp` | `shootCooldown, shootInterval, movementPattern, patternTimer, baseY, aliveTime, phaseOffset, targetY, zigzagTimer, zigzagUp` | Enemy AI state |
| `PowerUpTag` | `type` | Power-up identification |
| `WaveCannonTag` | `chargeLevel, width` | Wave Cannon projectile |

### Systems Created (Phase 2)

| System | Priority | Responsibility | Dependencies |
|--------|----------|----------------|--------------|
| `MovementSystem` | 300 | `pos += vel × dt` | None |
| `LifetimeSystem` | 600 | Decrement lifetime, delete expired | None |
| `CleanupSystem` | 700 | Remove OOB entities (not players) | DomainBridge |
| `CollisionSystem` | 400 | Detect AABB collisions (O(n²)) | DomainBridge |
| `DamageSystem` | 500 | Apply damage from collisions | DomainBridge, CollisionSystem |

### System Details

#### MovementSystem
```cpp
void Update(ECS& ecs, SystemID, uint32_t msecs) {
    float dt = msecs / 1000.0f;
    for (auto e : ecs.getEntitiesByComponentsAllOf<PositionComp, VelocityComp>()) {
        auto& pos = ecs.entityGetComponent<PositionComp>(e);
        auto& vel = ecs.entityGetComponent<VelocityComp>(e);
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
    }
}
```

#### CollisionSystem
- Uses O(n²) naive algorithm (acceptable for <100 entities)
- Stores `CollisionEvent { entityA, entityB, groupA, groupB }`
- Avoids duplicates with `i < j` check
- TODO: Implement spatial hashing if needed

#### DamageSystem
- Consumes CollisionEvents from CollisionSystem
- Collision rules:
  - `MISSILES + ENEMIES` → Enemy takes damage, missile consumed
  - `PLAYERS + ENEMIES` → Player takes 20 contact damage
  - `PLAYERS + ENEMY_MISSILES` → Player takes missile damage
  - `WAVE_CANNONS + ENEMIES` → Enemy takes damage (wave cannon persists)
  - `FORCE_PODS + ENEMIES` → Enemy takes 30 contact damage
  - `MISSILES + PLAYERS` → Ignored (no friendly fire)
- Generates `KillEvent` for ScoreSystem
- Respects `HealthComp::invulnerable` (GodMode)

### Test Coverage (Phase 2)

| Test File | Tests | Status |
|-----------|-------|--------|
| `ComponentTagsTest.cpp` | 20+ | ✅ Pass |
| `MovementSystemTest.cpp` | 12+ | ✅ Pass |
| `LifetimeSystemTest.cpp` | 8+ | ✅ Pass |
| `CleanupSystemTest.cpp` | 10+ | ✅ Pass |
| `CollisionSystemTest.cpp` | 12+ | ✅ Pass |
| `DamageSystemTest.cpp` | 10+ | ✅ Pass |
| `ECSPhase2IntegrationTest.cpp` | 7+ | ✅ Pass |

**Total Phase 2 Tests:** ~80 tests

### How to Test Phase 2

```bash
./scripts/compile.sh
./artifacts/tests/server_tests --gtest_filter="*ComponentTag*:*Movement*:*Lifetime*:*Cleanup*:*Collision*:*Damage*:*ECSPhase2*"
```

---

## Phase 3 - Entités Complexes & Systems Avancés

### Status: ✅ Complete

### New Components Created (Phase 3)

| Component | Fields | Purpose | Status |
|-----------|--------|---------|--------|
| `PlayerTag` | `playerId, shipSkin, isAlive` | Player identification | ✅ Complete |
| `ScoreComp` | `total, kills, comboMultiplier, comboTimer, maxCombo, deaths` | Score tracking | ✅ Complete |
| `WeaponComp` | `currentType, shootCooldown, isCharging, chargeTime, weaponLevels[4]` | Weapon state | ✅ Complete |
| `SpeedLevelComp` | `level` (0-3) | Speed upgrade level | ✅ Complete |

### Component Details

#### WeaponComp
```cpp
struct WeaponComp {
    uint8_t currentType = 0;                       // 0-3: Standard, Spread, Laser, Missile
    float shootCooldown = 0.0f;
    bool isCharging = false;
    float chargeTime = 0.0f;
    std::array<uint8_t, 4> weaponLevels = {0, 0, 0, 0};  // Independent per-weapon

    uint8_t getCurrentLevel() const;
    bool upgradeCurrentWeapon();  // Returns false if already max (3)
};
```

#### SpeedLevelComp
```cpp
struct SpeedLevelComp {
    uint8_t level = 0;  // 0-3

    bool upgrade();     // Returns false if already max (3)
    bool isMaxLevel() const;
};
```

### Systems Implemented (Phase 3)

| System | Priority | Status | Description |
|--------|----------|--------|-------------|
| `PlayerInputSystem` | 0 | ✅ Complete | Input → Velocity, screen clamp |
| `WeaponSystem` | 200 | ✅ Complete | Cooldowns, missile spawning |
| `ScoreSystem` | 800 | ✅ Complete | Combo decay, score calculation |
| `EnemyAISystem` | 100 | ✅ Complete | Movement patterns, shooting |

### Test Coverage (Phase 3)

| Test File | Tests | Status |
|-----------|-------|--------|
| `PlayerComponentsTest.cpp` | 20+ | ✅ Pass |
| `PlayerInputSystemTest.cpp` | 24+ | ✅ Pass |
| `WeaponSystemTest.cpp` | 12+ | ✅ Pass |
| `ScoreSystemTest.cpp` | 15+ | ✅ Pass |
| `EnemyAISystemTest.cpp` | 20+ | ✅ Pass |

**Total Phase 3 Tests:** ~90 tests

### How to Test Phase 3

```bash
./scripts/compile.sh
./artifacts/tests/server_tests --gtest_filter="*PlayerComponent*:*PlayerInput*:*Weapon*:*Score*:*EnemyAI*"
```

---

## Implementation Choices & Deviations

### Phase 2 Decisions

#### 1. O(n²) Collision Detection
**Decision:** Keep simple O(n²) algorithm
**Reason:** <100 entities in typical gameplay, optimization premature
**Future:** Spatial hashing if >500 entities or perf degradation

#### 2. CollisionEvent with Entity Groups
**Decision:** Store both groups in CollisionEvent
**Reason:** Allows DamageSystem to filter without re-querying
**Trade-off:** More memory per event but faster processing

#### 3. KillEvent for Score Decoupling
**Decision:** DamageSystem emits KillEvents for ScoreSystem
**Reason:** Separation of concerns, ScoreSystem can run independently
**Pattern:** Event-driven communication between systems

#### 4. Players Excluded from CleanupSystem
**Decision:** Never delete PLAYERS group entities
**Reason:** Player death/respawn managed differently (GameWorld)
**Consistency:** Aligns with existing game logic

#### 5. Deferred Entity Deletion
**Decision:** All deletions use deferred pattern
**Reason:** Avoids modifying collections during iteration
**Implementation:** Collect IDs to delete, then delete after loop

### Phase 3 Decisions

#### 1. Independent Weapon Levels
**Decision:** `weaponLevels[4]` array for per-weapon upgrades
**Reason:** R-Type authentic: each weapon upgrades separately
**Trade-off:** 4 bytes instead of 1, but correct gameplay

#### 2. SpeedLevelComp Separate from PlayerTag
**Decision:** Speed level as separate component
**Reason:** Can be queried/modified independently by different systems
**Alternative considered:** Embedding in PlayerTag (rejected: less flexible)

---

## File Mapping

### Domain Layer (Phase 0)

```
src/server/
├── include/domain/
│   ├── Constants.hpp
│   └── services/
│       ├── GameRule.hpp
│       ├── CollisionRule.hpp
│       └── EnemyBehavior.hpp
└── domain/services/
    ├── GameRule.cpp
    ├── CollisionRule.cpp
    └── EnemyBehavior.cpp
```

### Infrastructure Layer (Phases 1-3)

```
src/server/infrastructure/ecs/
├── core/
│   ├── ECS.hpp
│   ├── Component.hpp
│   ├── Registry.hpp
│   ├── System.hpp
│   ├── Includes.hpp
│   └── Errors.hpp
├── components/
│   ├── PositionComp.hpp       # Phase 1
│   ├── VelocityComp.hpp       # Phase 1
│   ├── HealthComp.hpp         # Phase 1
│   ├── HitboxComp.hpp         # Phase 1
│   ├── LifetimeComp.hpp       # Phase 1
│   ├── OwnerComp.hpp          # Phase 1
│   ├── MissileTag.hpp         # Phase 2
│   ├── EnemyTag.hpp           # Phase 2
│   ├── EnemyAIComp.hpp        # Phase 2
│   ├── PowerUpTag.hpp         # Phase 2
│   ├── WaveCannonTag.hpp      # Phase 2
│   ├── PlayerTag.hpp          # Phase 3
│   ├── ScoreComp.hpp          # Phase 3
│   ├── WeaponComp.hpp         # Phase 3
│   └── SpeedLevelComp.hpp     # Phase 3
├── systems/
│   ├── MovementSystem.hpp/cpp       # Phase 2
│   ├── LifetimeSystem.hpp/cpp       # Phase 2
│   ├── CleanupSystem.hpp/cpp        # Phase 2
│   ├── CollisionSystem.hpp/cpp      # Phase 2
│   ├── DamageSystem.hpp/cpp         # Phase 2
│   ├── PlayerInputSystem.hpp/cpp    # Phase 3
│   ├── WeaponSystem.hpp/cpp         # Phase 3
│   ├── ScoreSystem.hpp/cpp          # Phase 3
│   └── EnemyAISystem.hpp/cpp        # Phase 3
└── bridge/
    ├── DomainBridge.hpp
    └── DomainBridge.cpp
```

### Tests

```
tests/server/
├── domain/services/
│   ├── GameRuleTest.cpp
│   ├── CollisionRuleTest.cpp
│   └── EnemyBehaviorTest.cpp
└── ecs/
    ├── ComponentPoolTest.cpp        # Phase 1
    ├── ECSIntegrationTest.cpp       # Phase 1
    ├── DomainBridgeTest.cpp         # Phase 1
    ├── ComponentTagsTest.cpp        # Phase 2
    ├── ECSPhase2IntegrationTest.cpp # Phase 2
    ├── PlayerComponentsTest.cpp     # Phase 3
    └── systems/
        ├── MovementSystemTest.cpp       # Phase 2
        ├── LifetimeSystemTest.cpp       # Phase 2
        ├── CleanupSystemTest.cpp        # Phase 2
        ├── CollisionSystemTest.cpp      # Phase 2
        ├── DamageSystemTest.cpp         # Phase 2
        ├── PlayerInputSystemTest.cpp    # Phase 3
        ├── WeaponSystemTest.cpp         # Phase 3
        ├── ScoreSystemTest.cpp          # Phase 3
        └── EnemyAISystemTest.cpp        # Phase 3
```

---

## How to Enable ECS

### Build with ECS Backend

```bash
# Configure with ECS enabled
cmake -B build -DUSE_ECS_BACKEND=ON

# Or add to your build script
./scripts/build.sh -DUSE_ECS_BACKEND=ON
```

### Run All ECS Tests

```bash
./scripts/compile.sh

# All ECS tests (Phases 0-3) - 310+ tests
./artifacts/tests/server_tests --gtest_filter="*ECS*:*Component*:*DomainBridge*:*GameRule*:*Collision*:*Enemy*:*Movement*:*Lifetime*:*Cleanup*:*Damage*:*Player*:*Weapon*:*Score*"

# Phase 2 only (~80 tests)
./artifacts/tests/server_tests --gtest_filter="*ComponentTag*:*Movement*:*Lifetime*:*Cleanup*:*Collision*:*Damage*:*ECSPhase2*"

# Phase 3 only (~90 tests)
./artifacts/tests/server_tests --gtest_filter="*PlayerComponent*:*PlayerInput*:*Weapon*:*Score*:*EnemyAI*"
```

---

## Next Steps

### Phase 3 - ✅ Complete

All Phase 3 systems implemented and tested:
- ✅ PlayerInputSystem (24+ tests)
- ✅ WeaponSystem (12+ tests)
- ✅ ScoreSystem (15+ tests)
- ✅ EnemyAISystem (20+ tests)

**Total tests:** 310+ tests passing

### Phase 4 (GameWorld Migration) - ⏳ Next

1. Register all components in `GameWorld::initializeECS()`
2. Migrate player creation to ECS entities
3. Migrate enemy spawning to ECS
4. Migrate missile creation to ECS
5. Adapt `getSnapshot()` to read from ECS
6. Dual-run testing (legacy + ECS comparison)

---

## Known Issues & TODOs

### Current Issues

None identified.

### Resolved Issues (Phase 3)

| Issue | Resolution |
|-------|------------|
| `ECS::ECS` incomplete type in Systems | Added `#include "core/ECS.hpp"` to all .cpp files |
| Entity ID 0 treated as "not found" | Changed `findPlayerByID()` to return `std::optional<EntityID>` |
| Screen width hardcoded as 800 in tests | Updated to use actual 1920 from Constants.hpp |

### TODOs

| Priority | Item | Phase |
|----------|------|-------|
| High | Migrate GameWorld to use ECS Systems | 4 |
| High | Register all components in initializeECS() | 4 |
| Medium | Adapt getSnapshot() to read from ECS | 4 |
| Medium | Dual-run testing (legacy + ECS comparison) | 4 |
| Low | Spatial hashing for CollisionSystem | 4+ |
| Low | ForcePodSystem, BitDeviceSystem, BossSystem | 4+ |

### Performance Considerations

- **Sparse sets:** O(1) component operations ✅
- **O(n²) collision:** Acceptable for <100 entities ✅
- **Deferred deletion:** Avoids iteration bugs ✅
- **TODO:** Profile when entity count exceeds 200

---

## References

- **Plan Document:** `ECS_INTEGRATION_PLAN.md`
- **ECS Source:** `src/ECS/` (original implementation)
- **CLAUDE.md:** Project context and conventions
