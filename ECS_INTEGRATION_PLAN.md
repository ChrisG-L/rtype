# R-Type - ECS Integration Plan

> **Version**: 3.0
> **Branche**: `ECS_realImpl`
> **Status**: Phase 5 en cours (6/9 systèmes actifs)

---

## Current Status

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 0 | ✅ | Domain Services (GameRule, CollisionRule, EnemyBehavior) |
| Phase 1 | ✅ | ECS Core + 6 Components + DomainBridge |
| Phase 2 | ✅ | 5 Systems (Movement, Collision, Damage, Lifetime, Cleanup) |
| Phase 3 | ✅ | Player/Enemy Systems (PlayerInput, Weapon, Score, EnemyAI) |
| Phase 4 | ✅ | GameWorld Migration (entities, runECSUpdate, getSnapshot) |
| **Phase 5** | 🔄 | Activation progressive des Systems |

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

| Priority | System | Status | Responsibility |
|----------|--------|--------|----------------|
| 0 | PlayerInputSystem | ✅ Active | Input → Velocity, screen clamp |
| 100 | EnemyAISystem | ❌ Disabled | Movement patterns, shooting |
| 200 | WeaponSystem | ✅ Active | Cooldowns (Phase 5.6) |
| 300 | MovementSystem | ✅ Active | `pos += vel × dt` |
| 400 | CollisionSystem | ✅ Active | AABB collision detection |
| 500 | DamageSystem | ✅ Active | Apply damage from collisions |
| 600 | LifetimeSystem | ✅ Active | Decrement, delete expired |
| 700 | CleanupSystem | ✅ Active | Remove OOB entities |
| 800 | ScoreSystem | ✅ Active | Combo decay (Phase 5.4) |

---

## Phase 4 - GameWorld Migration ✅ COMPLETE

| Step | Description | Status |
|------|-------------|--------|
| 4.1 | `initializeECS()` - 15 components registered | ✅ |
| 4.1 | `registerSystems()` - 9 Systems with priorities | ✅ |
| 4.2 | `addPlayer()` creates ECS entities | ✅ |
| 4.3 | `spawnMissile()` creates ECS entities | ✅ |
| 4.4 | `spawnEnemy()` creates ECS entities | ✅ |
| 4.5 | CMakeLists.txt includes all System .cpp | ✅ |
| 4.6 | `getSnapshot()` reads players from ECS | ✅ |
| 4.7 | `runECSUpdate()` drives movement | ✅ |
| 4.8 | Integration tests | ✅ |

---

## Phase 5 - System Activation 🔄 IN PROGRESS

### Completed

| Step | Description | Status |
|------|-------------|--------|
| 5.1 | LifetimeSystem + CleanupSystem | ✅ |
| 5.2 | CollisionSystem (detection) | ✅ |
| 5.3 | DamageSystem (missiles ↔ enemies) | ✅ |

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
    ├── updateShootCooldowns()            # Legacy (→ WeaponSystem)
    ├── updateMissiles()                  # Homing velocity + sync from ECS
    ├── updateEnemies()                   # Legacy patterns (→ EnemyAISystem)
    ├── checkCollisions()                 # Legacy: enemy missiles→players, missiles→boss
    └── getSnapshot()                     # Players from ECS, rest from legacy
```

### Remaining

| Step | Description | Complexity | Status |
|------|-------------|------------|--------|
| 5.4 | ScoreSystem (combo decay) | Faible | ✅ Complete |
| 5.5 | EnemyAISystem (patterns) | Haute | ❌ Pending |
| 5.6 | WeaponSystem (cooldowns) | Moyenne | ✅ Complete |
| 5.7 | Integration finale | Moyenne | ❌ Pending |

### Phase 5.4 - ScoreSystem

**Objectif**: Migrer combo decay de `updateComboTimers()` vers ScoreSystem.

```cpp
// Activer le système
// Dans registerSystems(): ne pas désactiver ScoreSystem

// ScoreSystem gère:
// - Combo decay (grace time + decay rate)
// - Score calculation via DomainBridge
```

**Legacy à supprimer**: `updateComboTimers()`

### Phase 5.5 - EnemyAISystem

**Objectif**: Migrer mouvement ennemis de `updateEnemyMovement()` vers EnemyAISystem.

**Patterns à migrer**:
- Basic: mouvement linéaire
- Tracker: suit la position Y du joueur
- Zigzag: oscillation verticale
- Fast: mouvement rapide
- Bomber: lent mais résistant
- POWArmor: drop power-up garanti

**Legacy à supprimer**: `updateEnemyMovement()`, partie de `updateEnemies()`

### Phase 5.6 - WeaponSystem

**Objectif**: Migrer cooldowns et tir de `spawnMissile()` vers WeaponSystem.

```cpp
// WeaponSystem gère:
// - Cooldown per-player (WeaponComp.shootCooldown)
// - Validation tir (canShoot)
// - Création entités missiles via callback

weaponSystem->setMissileSpawnCallback([this](auto& req) {
    return createMissileEntity(req);
});
```

**Legacy à supprimer**: `updateShootCooldowns()`, `canPlayerShoot()`

---

## Phase 6 - Legacy Removal (Future)

| Step | Description | Risk |
|------|-------------|------|
| 6.1 | Remove `_players` map | Medium |
| 6.2 | Remove `_missiles` map | Medium |
| 6.3 | Remove `_enemies` map | Medium |
| 6.4 | Remove sync functions | Low |
| 6.5 | getSnapshot() fully from ECS | Low |

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
