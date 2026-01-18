# R-Type - ECS Integration Plan

> **Version**: 3.1
> **Branche**: `ECS_realImpl`
> **Status**: Phase 5.5 complète (9/9 systèmes actifs)

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
| 100 | EnemyAISystem | ✅ Active | Movement patterns (Phase 5.5) |
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

## Phase 5 - System Activation ✅ COMPLETE

### Status

| Step | Description | Status |
|------|-------------|--------|
| 5.1 | LifetimeSystem + CleanupSystem | ✅ |
| 5.2 | CollisionSystem (detection) | ✅ |
| 5.3 | DamageSystem (missiles ↔ enemies) | ✅ |
| 5.4 | ScoreSystem (combo decay) | ✅ |
| 5.5 | EnemyAISystem (movement patterns) | ✅ |
| 5.6 | WeaponSystem (cooldowns) | ✅ |
| 5.7 | Integration finale | ❌ Pending |

### Current Architecture (Phase 5.5)

```
UDPServer.updateAndBroadcastRoom()
    │
    ├── runECSUpdate(deltaTime × gameSpeedMultiplier)  # ECS drives core gameplay
    │   ├── _ecs.Update(msecs)              # All 9 systems active
    │   │   ├── PlayerInputSystem           # Input → Velocity
    │   │   ├── EnemyAISystem               # Movement patterns (Basic, Tracker, Zigzag, etc.)
    │   │   ├── WeaponSystem                # Cooldown decay
    │   │   ├── MovementSystem              # Position += Velocity × dt
    │   │   ├── CollisionSystem             # Detect AABB collisions
    │   │   ├── DamageSystem                # Apply damage, delete dead entities
    │   │   ├── LifetimeSystem              # Expire timed entities
    │   │   ├── CleanupSystem               # Remove OOB entities
    │   │   └── ScoreSystem                 # Combo decay
    │   ├── syncPlayersFromECS()            # ECS positions → legacy _players
    │   ├── syncDeletedMissilesFromECS()    # Remove destroyed missiles from legacy
    │   ├── processECSKillEvents()          # Award score, spawn power-ups
    │   ├── syncDeletedEnemiesFromECS()     # Remove dead enemies from legacy
    │   ├── syncComboFromECS()              # ECS combo → legacy _playerScores
    │   └── syncCooldownsFromECS()          # ECS cooldowns → legacy _players
    │
    ├── updateShootCooldowns()              # No-op (ECS handles)
    ├── updateComboTimers()                 # No-op (ECS handles)
    ├── updateMissiles()                    # Homing velocity + sync positions from ECS
    ├── updateEnemies()                     # Sync FROM ECS, legacy shooting + OOB check
    ├── checkCollisions()                   # Legacy: enemy missiles→players, missiles→boss
    └── getSnapshot()                       # Players from ECS, rest from legacy
```

### Remaining

| Step | Description | Complexity | Status |
|------|-------------|------------|--------|
| 5.7 | Integration finale | Moyenne | ❌ Pending |

### Phase 5.7 - Integration Finale

**Objectif**: Nettoyage final et optimisations.

**Tâches restantes**:
- Migrer enemy missiles vers ECS (optionnel)
- Migrer Boss vers ECS (optionnel)
- Supprimer les fonctions sync redondantes
- Optimiser les requêtes ECS (batch queries)

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
