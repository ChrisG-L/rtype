# R-Type - ECS Integration Plan

> **Version**: 2.0 (Compact)
> **Branche**: `ECS_realImpl`
> **Status**: Phase 4 en cours

---

## Current Status

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 0 | ✅ | Domain Services (GameRule, CollisionRule, EnemyBehavior) |
| Phase 1 | ✅ | ECS Core + 6 Components + DomainBridge |
| Phase 2 | ✅ | 5 Systems (Movement, Collision, Damage, Lifetime, Cleanup) |
| Phase 3 | ✅ | Player/Enemy Systems (PlayerInput, Weapon, Score, EnemyAI) |
| **Phase 4** | 🔄 | GameWorld Migration |

**Tests**: 310+ tests passent

---

## Architecture Principle

> **Les Systems ECS orchestrent, le Domain décide.**

```
Domain Layer (pure)          Infrastructure Layer
├── GameRule.hpp         ←── DomainBridge ←── ECS Systems
├── CollisionRule.hpp        (Adapter)
└── EnemyBehavior.hpp
```

Les Systems ne contiennent **aucune logique métier** - ils délèguent au DomainBridge.

---

## Components Summary (15 total)

| Phase | Components |
|-------|------------|
| 1 | PositionComp, VelocityComp, HealthComp, HitboxComp, LifetimeComp, OwnerComp |
| 2 | MissileTag, EnemyTag, EnemyAIComp, PowerUpTag, WaveCannonTag |
| 3 | PlayerTag, ScoreComp, WeaponComp, SpeedLevelComp |

---

## Systems Summary (9 total)

| Priority | System | Responsibility |
|----------|--------|----------------|
| 0 | PlayerInputSystem | Input → Velocity, screen clamp |
| 100 | EnemyAISystem | Movement patterns, shooting |
| 200 | WeaponSystem | Cooldowns, missile spawning |
| 300 | MovementSystem | `pos += vel × dt` |
| 400 | CollisionSystem | AABB collision detection |
| 500 | DamageSystem | Apply damage from collisions |
| 600 | LifetimeSystem | Decrement, delete expired |
| 700 | CleanupSystem | Remove OOB entities |
| 800 | ScoreSystem | Combo decay, score calculation |

---

## Phase 4 - GameWorld Migration

### Completed ✅

| Step | Description |
|------|-------------|
| 4.1 | `initializeECS()` - 15 components registered |
| 4.1 | `registerSystems()` - 9 Systems with priorities |
| 4.2 | `addPlayer()` creates ECS entities |
| 4.3 | `spawnMissile()` creates ECS entities |
| 4.4 | `spawnEnemy()` creates ECS entities |
| 4.5 | CMakeLists.txt includes all System .cpp |
| 4.7 | `runECSUpdate()` drives player movement |

### Current Architecture (Phase 4.7)

```
UDPServer.updateAndBroadcastRoom()
    │
    ├── runECSUpdate(dt)           # ECS drives player movement
    │   ├── _ecs.Update(msecs)     # PlayerInputSystem + MovementSystem
    │   └── syncPlayersFromECS()   # ECS → legacy _players map
    │
    ├── updateMissiles()           # Legacy
    ├── updateEnemies()            # Legacy
    ├── checkCollisions()          # Legacy
    └── getSnapshot()              # Reads from legacy
```

**Active ECS Systems**: PlayerInputSystem, MovementSystem
**Disabled Systems**: EnemyAI, Weapon, Collision, Damage, Lifetime, Cleanup, Score

### Remaining ❌

| Step | Description | Status |
|------|-------------|--------|
| 4.6 | `getSnapshot()` reads from ECS | ✅ Complete |
| 4.8 | Integration tests | ❌ Pending |

### Phase 4.6 - getSnapshot() from ECS

```cpp
// Query ECS for player positions (source of truth)
auto playerEntities = _ecs.getEntitiesByComponentsAllOf<PlayerTag, PositionComp, HealthComp>();
for (auto entityId : playerEntities) {
    const auto& pos = _ecs.entityGetComponent<PositionComp>(entityId);
    // Build PlayerState from ECS data
}

// Still use legacy for missiles/enemies until Phase 5
```

---

## Phase 5+ (Future)

| Item | Description |
|------|-------------|
| Enable more Systems | Progressively enable EnemyAI, Weapon, Collision, etc. |
| Remove legacy maps | Delete `_players`, `_missiles`, `_enemies` maps |
| ForcePod/BitDevice/Boss | Implement auxiliary systems |
| Spatial hashing | Optimize CollisionSystem if >500 entities |

---

## File Structure

```
src/server/infrastructure/ecs/
├── core/           # ECS.hpp, Component.hpp, Registry.hpp, System.hpp
├── components/     # 15 component headers
├── systems/        # 9 system .hpp/.cpp files
└── bridge/         # DomainBridge.hpp/.cpp

tests/server/ecs/
├── ComponentPoolTest.cpp
├── ECSIntegrationTest.cpp
├── DomainBridgeTest.cpp
├── ComponentTagsTest.cpp
├── PlayerComponentsTest.cpp
├── ECSPhase2IntegrationTest.cpp
└── systems/        # 9 system test files
```

---

## Build & Test

```bash
# Build with ECS enabled
cmake -B build -DUSE_ECS_BACKEND=ON
./scripts/compile.sh

# All ECS tests (310+)
./artifacts/tests/server_tests --gtest_filter="*ECS*:*Component*:*DomainBridge*:*Movement*:*Collision*:*Player*:*Weapon*:*Score*:*EnemyAI*"
```

---

## Reference

- **Detailed Result**: `ECS_INTEGRATION_RESULT.md`
- **Original ECS**: `src/ECS/` (Guillaume's Blob ECS)
- **Project Context**: `CLAUDE.md`

*Updated: 2026-01-18*
