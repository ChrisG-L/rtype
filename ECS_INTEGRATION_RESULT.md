# ECS Integration - Result & Status

> **Document de suivi** de l'intégration ECS dans le projet R-Type.
> Référence : `ECS_INTEGRATION_PLAN.md`

---

## Table of Contents

1. [Current Status](#current-status)
2. [Phase 0 - Domain Services](#phase-0---domain-services)
3. [Phase 1 - ECS Core & Components](#phase-1---ecs-core--components)
4. [Implementation Choices & Deviations](#implementation-choices--deviations)
5. [File Mapping](#file-mapping)
6. [How to Enable ECS](#how-to-enable-ecs)
7. [Next Steps (Phase 2+)](#next-steps-phase-2)
8. [Known Issues & TODOs](#known-issues--todos)

---

## Current Status

| Phase | Status | Description |
|-------|--------|-------------|
| **Phase 0** | ✅ Complete | Domain Services extraction |
| **Phase 1** | ✅ Complete | ECS Core + Components + DomainBridge |
| **Phase 2** | ⏳ Pending | ECS Systems (Movement, Collision, etc.) |
| **Phase 3** | ⏳ Pending | GameWorld migration |
| **Phase 4** | ⏳ Pending | Full ECS backend |

**Last Updated:** 2025-01-17
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

### Components Created

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

### GameWorld Integration

```cpp
// GameWorld.hpp
#ifdef USE_ECS_BACKEND
    ECS::ECS _ecs;
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
    std::unique_ptr<ecs::bridge::DomainBridge> _domainBridge;

    void initializeECS();
#endif
```

### Test Coverage

| Test File | Tests | Status |
|-----------|-------|--------|
| `ComponentPoolTest.cpp` | 20+ | ✅ Pass |
| `ECSIntegrationTest.cpp` | 25+ | ✅ Pass |
| `DomainBridgeTest.cpp` | 35+ | ✅ Pass |

---

## Implementation Choices & Deviations

### 1. HealthComp: `uint16_t` instead of `int16_t`

**Plan:** `int16_t current, max`
**Implementation:** `uint16_t current, max`

**Reason:** Health points are never negative. Using unsigned prevents invalid states and is more semantically correct.

---

### 2. Constants Organization

**Plan:** Not explicitly specified
**Implementation:** Organized in nested namespaces

```cpp
namespace domain::constants {
    namespace weapon { ... }
    namespace enemy { ... }
    namespace player { ... }
    namespace score { ... }
    namespace world { ... }
    namespace boss { ... }
}
```

**Reason:** Better organization and prevents name collisions.

---

### 3. ZigzagState as Separate Struct

**Plan:** Part of EnemyMovementInput
**Implementation:** Separate struct, passed by reference

```cpp
struct ZigzagState {
    float timer = 0.0f;
    bool movingUp = true;
};
```

**Reason:** The zigzag state needs to persist between frames and be updated. Having it as a separate struct allows the caller to maintain state.

---

### 4. DomainBridge Returns `std::pair` for Movement

**Plan:** Not specified
**Implementation:** Returns `std::pair<float, float>` for dx/dy

```cpp
std::pair<float, float> getEnemyMovement(...);
```

**Reason:** Simple and efficient for returning two floats. Avoids heap allocation of EnemyMovementOutput struct.

---

### 5. ECS Namespace Structure

**Plan:** `infrastructure::ecs`
**Implementation:** Exact match

```
infrastructure::ecs::components::PositionComp
infrastructure::ecs::bridge::DomainBridge
ECS::ECS (core class - kept original namespace from src/ECS)
```

**Note:** The ECS core classes retain their original `ECS::` namespace from the copied source, while new infrastructure code uses `infrastructure::ecs::`.

---

### 6. Component Default Values

**Plan:** "Valeurs par défaut saines"
**Implementation:** All components have explicit defaults

| Component | Defaults |
|-----------|----------|
| PositionComp | `x=0, y=0` |
| VelocityComp | `x=0, y=0` |
| HealthComp | `current=100, max=100, invulnerable=false` |
| HitboxComp | `width=0, height=0, offsetX=0, offsetY=0` |
| LifetimeComp | `remaining=0, total=0` |
| OwnerComp | `ownerId=0, isPlayerOwned=true` |

---

### 7. EntityGroups Defined

**Implementation:** 6 groups as per plan

```cpp
enum class EntityGroup : uint8_t {
    NONE = 0,
    PLAYERS,
    ENEMIES,
    MISSILES,
    POWERUPS,
    WAVES,
    BOSSES
};
```

---

## File Mapping

### Domain Layer (Phase 0)

```
src/server/
├── include/domain/
│   ├── Constants.hpp                    # Centralized constants
│   └── services/
│       ├── GameRule.hpp                 # Damage, score, combo
│       ├── CollisionRule.hpp            # AABB collision
│       └── EnemyBehavior.hpp            # Movement patterns
└── domain/services/
    ├── GameRule.cpp
    ├── CollisionRule.cpp
    └── EnemyBehavior.cpp
```

### Infrastructure Layer (Phase 1)

```
src/server/infrastructure/ecs/
├── core/
│   ├── ECS.hpp                          # Main ECS class
│   ├── Component.hpp                    # ComponentPool
│   ├── Registry.hpp                     # Type registration
│   ├── System.hpp                       # ISystem interface
│   ├── Includes.hpp                     # Common includes
│   └── Errors.hpp                       # Exceptions
├── components/
│   ├── PositionComp.hpp
│   ├── VelocityComp.hpp
│   ├── HealthComp.hpp
│   ├── HitboxComp.hpp
│   ├── LifetimeComp.hpp
│   └── OwnerComp.hpp
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
    ├── ComponentPoolTest.cpp
    ├── ECSIntegrationTest.cpp
    └── DomainBridgeTest.cpp
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

### Verify ECS is Active

When enabled, you'll see in CMake output:
```
-- ECS Backend: ENABLED
```

When disabled (default):
```
-- ECS Backend: DISABLED (default)
```

### Run Tests

```bash
./scripts/compile.sh
./artifacts/tests/server_tests --gtest_filter="*ECS*:*Component*:*DomainBridge*:*GameRule*:*Collision*:*Enemy*"
```

---

## Next Steps (Phase 2)

### Systems to Create

| System | Priority | Responsibility |
|--------|----------|----------------|
| `MovementSystem` | High | Update positions from velocities |
| `CollisionSystem` | High | Detect collisions, delegate to DomainBridge |
| `LifetimeSystem` | Medium | Decrement lifetime, mark expired entities |
| `EnemyAISystem` | Medium | Enemy movement patterns via DomainBridge |
| `WeaponSystem` | Medium | Missile spawning, cooldowns |
| `DamageSystem` | High | Apply damage via DomainBridge |
| `ScoreSystem` | Low | Score calculation via DomainBridge |

### System Template

```cpp
// infrastructure/ecs/systems/MovementSystem.hpp
#pragma once
#include "../core/System.hpp"
#include "../bridge/DomainBridge.hpp"

namespace infrastructure::ecs::systems {

class MovementSystem : public ECS::ISystem {
public:
    explicit MovementSystem(bridge::DomainBridge& bridge);

    void update(ECS::ECS& ecs, float deltaTime) override;

private:
    bridge::DomainBridge& _bridge;
};

} // namespace infrastructure::ecs::systems
```

### Key Principle for Phase 2

> **"Systems ECS orchestrent, le Domain décide"**

Systems should:
1. Query entities with required components
2. Extract data from components
3. Call DomainBridge methods for business logic
4. Update components with results

Systems should NOT:
- Contain business logic
- Make game rule decisions
- Hardcode constants (use DomainBridge/Constants)

---

## Known Issues & TODOs

### Current Issues

None identified.

### TODOs for Future Phases

1. **Phase 2:** Create Systems as described above
2. **Phase 2:** Add `EnemyComp`, `MissileComp`, `PlayerComp` for type-specific data
3. **Phase 3:** Migrate GameWorld to use ECS for entity management
4. **Phase 4:** Remove legacy entity storage, full ECS backend

### Performance Considerations

- Sparse sets provide O(1) component operations
- Consider component ordering for cache efficiency in hot loops
- Profile collision detection when many entities exist

---

## References

- **Plan Document:** `ECS_INTEGRATION_PLAN.md`
- **ECS Source:** `src/ECS/` (original implementation)
- **CLAUDE.md:** Project context and conventions
