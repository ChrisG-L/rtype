# ECS Evolution - Roadmap to Full ECS

> **Objectif** : Supprimer tout le code legacy pour un système 100% ECS.
> **État actuel** : Hybride ECS/Legacy (Phase 5 complète)
> **Branche** : `ECS_realImpl`

---

## Vue d'Ensemble

### Architecture Actuelle (Hybride)

```
┌─────────────────────────────────────────────────────────────────┐
│                        ECS (Actif)                              │
├─────────────────────────────────────────────────────────────────┤
│ Players      │ Position, Velocity, Input, Score, Weapon        │
│ Missiles     │ Position, Velocity, Lifetime, Damage            │
│ Enemies      │ Position, Velocity, AI, Health                  │
└─────────────────────────────────────────────────────────────────┘
                              ↕ sync
┌─────────────────────────────────────────────────────────────────┐
│                      Legacy (À supprimer)                       │
├─────────────────────────────────────────────────────────────────┤
│ _players     │ Map legacy, endpoints, health sync              │
│ _missiles    │ Map legacy, homing logic                        │
│ _enemies     │ Map legacy, shooting, OOB                       │
│ _enemyMissiles │ 100% legacy (pas d'entité ECS)                │
│ _boss        │ 100% legacy (pas d'entité ECS)                  │
│ _forcePods   │ 100% legacy                                     │
│ _bitDevices  │ 100% legacy                                     │
│ _powerUps    │ 100% legacy                                     │
│ _waveCannons │ 100% legacy                                     │
└─────────────────────────────────────────────────────────────────┘
```

### Architecture Cible (Full ECS)

```
┌─────────────────────────────────────────────────────────────────┐
│                        ECS (Tout)                               │
├─────────────────────────────────────────────────────────────────┤
│ Players      │ Toute la logique joueur                         │
│ Missiles     │ Player missiles + Enemy missiles                │
│ Enemies      │ Movement + Shooting + OOB                       │
│ Boss         │ Phases, attaques, hitbox                        │
│ Force Pods   │ Attach/detach, shooting, contact damage         │
│ Bit Devices  │ Orbit, shooting, contact damage                 │
│ Power-ups    │ Spawn, lifetime, collection                     │
│ Wave Cannons │ Charge, beam, damage                            │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    getSnapshot() from ECS                       │
│                    (Plus de maps legacy)                        │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phases d'Évolution

### Phase 6 - Suppression Maps Legacy

| Étape | Description | Dépendances | Risque |
|-------|-------------|-------------|--------|
| 6.1 | Supprimer `_players` map | getSnapshot() ECS | Moyen |
| 6.2 | Supprimer `_missiles` map | getSnapshot() ECS | Moyen |
| 6.3 | Supprimer `_enemies` map | Enemy shooting ECS | Moyen |
| 6.4 | Supprimer fonctions sync | 6.1-6.3 | Faible |

#### 6.1 - Supprimer `_players` map

**Fichiers impactés** :
- `GameWorld.hpp/cpp` - Supprimer `std::map<uint8_t, Player> _players`
- `UDPServer.cpp` - Adapter lookups par playerId

**Prérequis** :
- [ ] `getSnapshot()` lit players 100% depuis ECS
- [ ] `findPlayerByEndpoint()` utilise ECS query
- [ ] `getPlayerHealth()` lit depuis `HealthComp`
- [ ] `isPlayerAlive()` lit depuis `PlayerTag.isAlive`
- [ ] `updatePlayerActivity()` stocké dans composant ECS

**Nouveau composant requis** :
```cpp
struct PlayerNetworkComp {
    boost::asio::ip::udp::endpoint endpoint;
    std::chrono::steady_clock::time_point lastActivity;
    bool godMode;
};
```

#### 6.2 - Supprimer `_missiles` map

**Fichiers impactés** :
- `GameWorld.hpp/cpp` - Supprimer `std::map<uint16_t, Missile> _missiles`

**Prérequis** :
- [ ] `getSnapshot()` lit missiles depuis ECS
- [ ] Homing logic dans `MovementSystem` ou nouveau `HomingSystem`
- [ ] `_missileEntityIds` devient source de vérité (ou supprimé)

**Changements MovementSystem** :
```cpp
// Ajouter logique homing pour missiles avec MissileTag.isHoming
if (missile.isHoming && missile.targetId != 0) {
    // Calculer direction vers cible
    // Appliquer turn rate
}
```

#### 6.3 - Supprimer `_enemies` map

**Fichiers impactés** :
- `GameWorld.hpp/cpp` - Supprimer `std::map<uint16_t, Enemy> _enemies`

**Prérequis** :
- [ ] `getSnapshot()` lit enemies depuis ECS
- [ ] Enemy shooting dans `EnemyAISystem` (déjà partiellement fait)
- [ ] OOB check dans `CleanupSystem` (activer pour enemies)
- [ ] `_enemyEntityIds` devient source de vérité

**Changements EnemyAISystem** :
- Déjà produit `EnemyMissileRequest`
- GameWorld doit créer les missiles depuis ces requests

---

### Phase 7 - Enemy Missiles ECS

| Étape | Description | Complexité |
|-------|-------------|------------|
| 7.1 | Créer `EnemyMissileTag` component | Faible |
| 7.2 | `spawnEnemyMissile()` crée entité ECS | Moyenne |
| 7.3 | Adapter `DamageSystem` pour ENEMY_MISSILES | Moyenne |
| 7.4 | Supprimer `_enemyMissiles` map | Moyenne |

#### 7.1 - EnemyMissileTag Component

```cpp
struct EnemyMissileTag {
    uint8_t damage = 15;
    uint8_t sourceEnemyType = 0;
};
```

#### 7.2 - Spawn Enemy Missiles

```cpp
void GameWorld::spawnEnemyMissile(float x, float y, uint8_t enemyType) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMY_MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = x; pos.y = y;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = -300.0f;  // Vers la gauche
    vel.y = 0.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 16.0f; hitbox.height = 8.0f;

    auto& tag = _ecs.entityAddComponent<EnemyMissileTag>(entity);
    tag.damage = 15;
    tag.sourceEnemyType = enemyType;

    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
    lifetime.remaining = 10.0f;
}
```

#### 7.3 - DamageSystem Adaptation

Le code existe déjà (désactivé) :
```cpp
// PLAYERS + ENEMY_MISSILES → Player takes missile damage
if (hasGroup(groupA, groupB, ECS::EntityGroup::PLAYERS) &&
    hasGroup(groupA, groupB, ECS::EntityGroup::ENEMY_MISSILES)) {
    // Activer ce code
}
```

---

### Phase 8 - Boss ECS

| Étape | Description | Complexité |
|-------|-------------|------------|
| 8.1 | Créer `BossTag` + `BossAIComp` | Moyenne |
| 8.2 | `spawnBoss()` crée entité ECS | Moyenne |
| 8.3 | `BossAISystem` pour phases/attaques | Haute |
| 8.4 | Adapter `DamageSystem` pour boss | Moyenne |
| 8.5 | Supprimer `_boss` optionnal | Moyenne |

#### 8.1 - Boss Components

```cpp
struct BossTag {
    uint16_t bossId;
    uint8_t bossType = 0;  // Pour futurs boss différents
};

struct BossAIComp {
    uint8_t phase = 0;           // 0, 1, 2
    float phaseTimer = 0.0f;
    float attackCooldown = 0.0f;
    float enrageTimer = 0.0f;
    bool isEnraged = false;

    // Attack patterns
    uint8_t currentPattern = 0;
    float patternTimer = 0.0f;
};
```

#### 8.3 - BossAISystem

```cpp
class BossAISystem : public ECS::ISystem {
public:
    void Update(ECS::ECS& ecs, ECS::SystemID id, uint32_t msecs) override {
        // Query boss entities
        // Update phase based on health %
        // Execute attack patterns
        // Produce BossAttackEvents
    }

    std::vector<BossAttackEvent> getAttackEvents();
};
```

---

### Phase 9 - Force Pods & Bit Devices ECS

| Étape | Description | Complexité |
|-------|-------------|------------|
| 9.1 | `ForcePodTag` + `ForcePodComp` | Moyenne |
| 9.2 | `ForcePodSystem` (attach/detach, shoot) | Haute |
| 9.3 | `BitDeviceTag` + `BitDeviceComp` | Moyenne |
| 9.4 | `BitDeviceSystem` (orbit, shoot) | Haute |
| 9.5 | Supprimer `_forcePods`, `_bitDevices` | Moyenne |

#### 9.1 - Force Pod Components

```cpp
struct ForcePodTag {
    uint8_t podId;
};

struct ForcePodComp {
    uint8_t ownerId = 0;
    uint8_t level = 1;           // 1-2
    bool isAttached = false;
    bool attachedFront = true;   // Front or back
    float shootCooldown = 0.0f;
    float detachTimer = 0.0f;    // Time since detach
};
```

#### 9.3 - Bit Device Components

```cpp
struct BitDeviceTag {
    uint8_t bitId;
    uint8_t bitIndex;  // 0 or 1 (2 bits per player)
};

struct BitDeviceComp {
    uint8_t ownerId = 0;
    float orbitAngle = 0.0f;
    float orbitRadius = 50.0f;
    float orbitSpeed = 3.0f;     // rad/s
    float shootCooldown = 0.0f;
};
```

---

### Phase 10 - Power-ups & Wave Cannons ECS

| Étape | Description | Complexité |
|-------|-------------|------------|
| 10.1 | Power-ups déjà ont `PowerUpTag` | Faible |
| 10.2 | `PowerUpSystem` (collection) | Moyenne |
| 10.3 | Wave Cannons ont `WaveCannonTag` | Faible |
| 10.4 | `WaveCannonSystem` (beam logic) | Moyenne |
| 10.5 | Supprimer `_powerUps`, `_waveCannons` | Faible |

---

### Phase 11 - getSnapshot() Full ECS

| Étape | Description | Complexité |
|-------|-------------|------------|
| 11.1 | `getPlayersSnapshot()` from ECS | Faible |
| 11.2 | `getMissilesSnapshot()` from ECS | Faible |
| 11.3 | `getEnemiesSnapshot()` from ECS | Faible |
| 11.4 | `getBossSnapshot()` from ECS | Moyenne |
| 11.5 | `getForcesSnapshot()` from ECS | Faible |
| 11.6 | `getBitsSnapshot()` from ECS | Faible |
| 11.7 | `getPowerUpsSnapshot()` from ECS | Faible |

---

### Phase 12 - Cleanup Final

| Étape | Description |
|-------|-------------|
| 12.1 | Supprimer toutes les fonctions `sync*FromECS()` |
| 12.2 | Supprimer `_*EntityIds` maps (plus nécessaires) |
| 12.3 | Supprimer structs legacy (`Player`, `Missile`, `Enemy`, etc.) |
| 12.4 | Simplifier `GameWorld` (déléguer aux Systems) |
| 12.5 | Mettre à jour documentation |

---

## Nouveaux Systems Requis

| System | Priority | Responsabilité |
|--------|----------|----------------|
| `HomingSystem` | 250 | Missiles homing (après WeaponSystem) |
| `BossAISystem` | 150 | Boss phases et attaques |
| `ForcePodSystem` | 350 | Force pod attach/detach/shoot |
| `BitDeviceSystem` | 360 | Bit orbit et shoot |
| `PowerUpSystem` | 450 | Power-up collection |
| `WaveCannonSystem` | 350 | Wave cannon beam |

### Ordre d'Exécution Final (Full ECS)

```
Priority 0:   PlayerInputSystem      # Input → Velocity
Priority 100: EnemyAISystem          # Enemy movement + shoot requests
Priority 150: BossAISystem           # Boss AI + attack requests
Priority 200: WeaponSystem           # Player weapon cooldowns
Priority 250: HomingSystem           # Homing missile tracking
Priority 300: MovementSystem         # Position += Velocity × dt
Priority 350: ForcePodSystem         # Force pod logic
Priority 360: BitDeviceSystem        # Bit device orbit
Priority 400: CollisionSystem        # Pair-based AABB detection (optimized)
Priority 450: PowerUpSystem          # Power-up collection
Priority 500: DamageSystem           # Apply damage
Priority 600: LifetimeSystem         # Expire entities
Priority 700: CleanupSystem          # Remove OOB
Priority 800: ScoreSystem            # Combo decay
```

---

## Nouveaux Components Requis

| Component | Phase | Champs |
|-----------|-------|--------|
| `PlayerNetworkComp` | 6.1 | endpoint, lastActivity, godMode |
| `EnemyMissileTag` | 7.1 | damage, sourceEnemyType |
| `BossTag` | 8.1 | bossId, bossType |
| `BossAIComp` | 8.1 | phase, attackCooldown, patterns |
| `ForcePodComp` | 9.1 | level, isAttached, attachedFront |
| `BitDeviceComp` | 9.3 | orbitAngle, orbitRadius, orbitSpeed |

---

## Estimation Effort

| Phase | Effort | Tests |
|-------|--------|-------|
| Phase 6 (Legacy maps) | 2-3 jours | ~20 tests |
| Phase 7 (Enemy missiles) | 1-2 jours | ~10 tests |
| Phase 8 (Boss) | 3-4 jours | ~15 tests |
| Phase 9 (Force/Bits) | 3-4 jours | ~20 tests |
| Phase 10 (PowerUps/WaveCannon) | 2 jours | ~10 tests |
| Phase 11 (getSnapshot) | 1 jour | ~5 tests |
| Phase 12 (Cleanup) | 1 jour | - |

**Total estimé** : 13-17 jours de développement

---

## Risques et Mitigations

| Risque | Impact | Mitigation | Status |
|--------|--------|------------|--------|
| Régression gameplay | Haut | Tests exhaustifs à chaque phase | Futur |
| Performance queries ECS | Moyen | Pair-based collision detection | ✅ Résolu |
| Synchronisation réseau | Moyen | Garder même format snapshot | Futur |
| Complexité Boss AI | Moyen | Implémenter pattern par pattern | Futur |

---

## Critères de Succès

- [ ] Aucune map legacy (`_players`, `_missiles`, `_enemies`, etc.)
- [ ] Aucune fonction `sync*FromECS()`
- [ ] `getSnapshot()` 100% depuis ECS queries
- [ ] Tous les 643+ tests passent
- [ ] Performance équivalente ou meilleure
- [ ] Gameplay identique (vérifié manuellement)

---

## Références

- `ECS_INTEGRATION_PLAN.md` - Plan original
- `ECS_INTEGRATION_RESULT.md` - État actuel détaillé
- `CLAUDE.md` - Contexte projet

*Créé: 2026-01-18*
