# R-Type - Document de Design : Intégration ECS

> **Version**: 1.0
> **Date**: Janvier 2026
> **Auteur**: Équipe R-Type
> **Statut**: Proposition de design

---

## Table des Matières

1. [Résumé Exécutif](#1-résumé-exécutif)
1bis. [**Principe Architectural : ECS dans une Architecture Hexagonale**](#1bis-principe-architectural--ecs-dans-une-architecture-hexagonale) ⚠️ IMPORTANT
2. [Analyse du Système Actuel](#2-analyse-du-système-actuel)
3. [L'ECS Blob de Guillaume](#3-lecs-blob-de-guillaume)
4. [Architecture Cible](#4-architecture-cible)
   - 4.1 [Components à Créer (DTOs)](#41-components-à-créer)
   - 4.2 [Systems à Implémenter (Orchestration)](#42-systems-à-implémenter)
   - 4.3 [Diagramme d'Architecture](#43-diagramme-darchitecture)
5. [Plan de Migration](#5-plan-de-migration)
   - 5.1 [Phase 1 - Fondations](#51-phase-1---fondations)
   - 5.2 [Phase 2 - Entités Simples](#52-phase-2---entités-simples)
   - 5.3 [Phase 3 - Entités Complexes](#53-phase-3---entités-complexes)
   - 5.4 [Phase 4 - Intégration Réseau](#54-phase-4---intégration-réseau)
6. [Compatibilité Réseau](#6-compatibilité-réseau)
7. [Stratégie de Tests](#7-stratégie-de-tests)
8. [Risques et Mitigations](#8-risques-et-mitigations)
9. [Références](#9-références)

---

## 1. Résumé Exécutif

### Contexte

Le jeu R-Type actuel utilise une architecture **monolithique** dans `GameWorld.cpp` où chaque type d'entité (joueurs, ennemis, missiles, boss, power-ups, Force Pod, Bit Devices) est géré par des `std::unordered_map` séparées avec des boucles de mise à jour distinctes.

L'équipe dispose d'un **ECS maison** (`src/ECS/`) développé par Guillaume LECOCQ (Blob ECS), actuellement inutilisé dans le projet.

### Objectif

Migrer l'architecture serveur vers l'ECS Blob pour :
- Améliorer les performances (cache-friendly, sparse sets)
- Réduire la complexité du code
- Faciliter l'ajout de nouveaux types d'entités
- Préparer le terrain pour l'IA améliorée (cf. ENEMY_AI_IMPROVEMENTS.md)

### Bénéfices Attendus

| Aspect | Avant (Monolithique) | Après (ECS) |
|--------|---------------------|-------------|
| Lignes de code GameWorld | ~2500 LOC | ~800 LOC |
| Ajout d'un type d'entité | ~200 LOC + 5 fichiers | ~50 LOC + 2 fichiers |
| Performance (10K entités) | ~8ms/frame | ~2ms/frame |
| Couplage | Fort (tout dans GameWorld) | Faible (Systems isolés) |
| Testabilité | Difficile | Unitaire par System |

### Effort Estimé

| Phase | Durée | Risque |
|-------|-------|--------|
| Phase 1 - Fondations | 1 semaine | Faible |
| Phase 2 - Entités Simples | 1.5 semaines | Moyen |
| Phase 3 - Entités Complexes | 1.5 semaines | Élevé |
| Phase 4 - Intégration Réseau | 1 semaine | Élevé |
| **Total** | **~5 semaines** | - |

---

## 1bis. Principe Architectural : ECS dans une Architecture Hexagonale

### Règle d'Or

> **Les Systems ECS orchestrent, le Domain décide.**
>
> L'ECS est un **détail d'implémentation de l'infrastructure**. Toute la logique métier reste dans le Domain Layer.

### Pourquoi cette règle ?

Le projet R-Type utilise une **Architecture Hexagonale** (Ports & Adapters) avec une séparation stricte :

| Layer | Responsabilité | Dépendances |
|-------|----------------|-------------|
| **Domain** | Règles métier, entités, value objects | Aucune (pur) |
| **Application** | Use cases, orchestration | Domain |
| **Infrastructure** | Adapters (réseau, DB, ECS) | Application, Domain |

L'ECS est un pattern de **performance pour la simulation de jeu**, pas un pattern architectural global. Il doit donc rester confiné dans l'Infrastructure Layer.

### Tension à Éviter

```
┌─────────────────────────────────────────────────────────────┐
│                    ❌ ANTI-PATTERN                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Si on laisse la logique métier dans les Systems ECS :     │
│                                                             │
│  ecs/systems/DamageSystem.hpp                              │
│    └─→ Calcule les dégâts (logique métier!)               │
│    └─→ Gère la mort (logique métier!)                     │
│    └─→ Applique les combos (logique métier!)              │
│                                                             │
│  Résultat : Le Domain Layer devient inutile, la logique   │
│  métier est éparpillée, impossible à tester unitairement. │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    ✅ PATTERN CORRECT                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  domain/services/GameRule.hpp                              │
│    └─→ calculateDamage(weapon, armorType) : uint16_t      │
│    └─→ shouldEntityDie(currentHP, damage) : bool          │
│    └─→ applyComboBonus(baseScore, comboMult) : uint32_t   │
│                                                             │
│  ecs/systems/DamageSystem.hpp                              │
│    └─→ Itère sur les entités avec Health                  │
│    └─→ Appelle gameRule.calculateDamage()                 │
│    └─→ Appelle gameRule.shouldEntityDie()                 │
│    └─→ Marque pour suppression si mort                    │
│                                                             │
│  Résultat : Logique métier centralisée, testable,         │
│  Systems ECS légers et focalisés sur l'orchestration.     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Convention de Nommage

Pour éviter la confusion entre les **Value Objects du Domain** et les **Components ECS** :

| Domain (value_objects/) | ECS (ecs/components/) | Raison |
|-------------------------|----------------------|--------|
| `Position` | `PositionComp` | Évite l'ambiguïté |
| `Health` | `HealthComp` | Suffixe `Comp` = Component |
| `Velocity` (nouveau) | `VelocityComp` | Cohérence |
| N/A | `PlayerTag` | Tags = pas d'équivalent domain |
| N/A | `EnemyTag` | Tags = pas d'équivalent domain |

### Adapter Pattern : DomainBridge

Un **adapter** fait le pont entre l'ECS et le Domain :

```cpp
// infrastructure/ecs/DomainBridge.hpp
class DomainBridge {
    GameRule& _gameRule;

public:
    explicit DomainBridge(GameRule& gameRule) : _gameRule(gameRule) {}

    // Délègue le calcul au Domain
    uint16_t applyDamage(HealthComp& health, uint16_t rawDamage) {
        uint16_t finalDamage = _gameRule.calculateDamage(rawDamage);

        if (_gameRule.shouldEntityDie(health.current, finalDamage)) {
            health.current = 0;
            return finalDamage;
        }

        health.current -= finalDamage;
        return finalDamage;
    }

    uint32_t calculateScore(uint16_t basePoints, uint8_t comboMultiplier) {
        return _gameRule.applyComboBonus(basePoints, comboMultiplier);
    }

    float getSpeedMultiplier(uint8_t speedLevel) {
        return _gameRule.getSpeedMultiplier(speedLevel);
    }
};
```

### Checklist de Validation

Avant chaque merge impliquant l'ECS, vérifier :

- [ ] **Aucun calcul de dégâts** dans les Systems (délégué à `GameRule`)
- [ ] **Aucun calcul de score** dans les Systems (délégué à `GameRule`)
- [ ] **Aucune règle de spawn** dans les Systems (délégué aux Use Cases)
- [ ] **Les Components sont des DTOs** (données brutes, pas de méthodes complexes)
- [ ] **Les Systems utilisent DomainBridge** pour toute logique métier

---

## 2. Analyse du Système Actuel

### 2.1 Structure de GameWorld

```
┌─────────────────────────────────────────────────────────────┐
│                    GameWorld.cpp (~2500 LOC)                 │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   Players   │  │   Enemies   │  │  Missiles   │         │
│  │ unordered_  │  │ unordered_  │  │ unordered_  │         │
│  │    map      │  │    map      │  │    map      │         │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │
│         │                │                │                 │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐         │
│  │ updatePlayer│  │updateEnemies│  │updateMissile│         │
│  │    Loop()   │  │    Loop()   │  │    Loop()   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│                                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │    Boss     │  │  PowerUps   │  │ WaveCannons │         │
│  │ (optionnel) │  │ unordered_  │  │ unordered_  │         │
│  │             │  │    map      │  │    map      │         │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │
│         │                │                │                 │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐         │
│  │ updateBoss  │  │updatePowerUp│  │ updateWave  │         │
│  │    Loop()   │  │    Loop()   │  │ CannonLoop()│         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│                                                              │
│  ┌─────────────┐  ┌─────────────┐                          │
│  │  ForcePods  │  │ BitDevices  │                          │
│  │ unordered_  │  │ unordered_  │                          │
│  │    map      │  │    map      │                          │
│  └──────┬──────┘  └──────┬──────┘                          │
│         │                │                                  │
│  ┌──────▼──────┐  ┌──────▼──────┐                          │
│  │ updateForce │  │ updateBits  │                          │
│  │    Loop()   │  │    Loop()   │                          │
│  └─────────────┘  └─────────────┘                          │
│                                                              │
│            checkCollisions() - Mega fonction                │
│              (~400 lignes de boucles imbriquées)            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Types d'Entités Actuels

| Type | Map | Struct | Champs Clés |
|------|-----|--------|-------------|
| **Player** | `_players` | `PlayerData` | id, x, y, health, score, weapon, force, bits |
| **Enemy** | `_enemies` | `EnemyData` | id, type, x, y, health, shootTimer |
| **Missile** | `_missiles` | `MissileData` | id, owner, x, y, weapon_type |
| **Boss** | `_boss` | `BossData` | id, x, y, health, phase, attackTimer |
| **PowerUp** | `_powerups` | `PowerUpData` | id, type, x, y, lifetime |
| **WaveCannon** | `_waveCannons` | `WaveCannonData` | id, owner, x, y, chargeLevel, width |
| **ForcePod** | `_forcePods` | `ForcePodData` | id, owner, x, y, level, attached |
| **BitDevice** | `_bitDevices` | `BitDeviceData` | id, owner, x, y, orbitAngle |
| **EnemyMissile** | `_enemyMissiles` | `MissileData` | id, x, y, damage |

### 2.3 Problèmes Identifiés

1. **Duplication de code** : Chaque type a sa boucle de mise à jour quasi-identique
2. **Collisions monolithiques** : `checkCollisions()` est une fonction géante avec N² comparaisons
3. **Pas de composition** : Un ennemi qui tire ≠ un missile, code dupliqué
4. **Ajout complexe** : Nouveau type = modifier 5+ endroits dans GameWorld
5. **Cache misses** : Les maps ne garantissent pas la localité mémoire

---

## 3. L'ECS Blob de Guillaume

### 3.1 Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────────┐
│                     Blob ECS Architecture                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                       ECS Class                       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │  Entities   │  │  Registry   │  │   Systems   │  │   │
│  │  │ vector<>    │  │ (Pools)     │  │  vector<>   │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│           ┌───────────────┼───────────────┐                │
│           ▼               ▼               ▼                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ComponentPool│  │ComponentPool│  │ComponentPool│        │
│  │  <Position> │  │  <Health>   │  │  <Velocity> │        │
│  │             │  │             │  │             │        │
│  │ SparseSet   │  │ SparseSet   │  │ SparseSet   │        │
│  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │        │
│  │ │ dense[] │ │  │ │ dense[] │ │  │ │ dense[] │ │        │
│  │ │ sparse[]│ │  │ │ sparse[]│ │  │ │ sparse[]│ │        │
│  │ └─────────┘ │  │ └─────────┘ │  │ └─────────┘ │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Fonctionnalités Disponibles

| Feature | Fichier | Description |
|---------|---------|-------------|
| Entity Management | `ECS.hpp` | Création/suppression avec recycling d'IDs |
| Component Pools | `Component.hpp` | Sparse sets pour stockage O(1) |
| System Tickrates | `ECS.hpp` | Support de fréquences différentes par système |
| Entity Groups | `Includes.hpp` | Groupes pour filtrage rapide |
| Type Safety | `Includes.hpp` | Concepts C++20 pour validation compile-time |
| Query AllOf/AnyOf | `ECS.hpp` | Requêtes sur composants multiples |

### 3.3 API Clé

```cpp
// Création d'entité
EntityID player = ecs.entityCreate(EntityGroup::PLAYERS);

// Ajout de composants
auto& pos = ecs.entityAddComponent<Position>(player);
pos.x = 100.0f;
pos.y = 200.0f;

auto& health = ecs.entityAddComponent<Health>(player);
health.current = 100;
health.max = 100;

// Query
auto entities = ecs.getEntitiesByComponentsAllOf<Position, Health, Velocity>();

// Dans un System
void Update(ECS& ecs, SystemID thisID, uint32_t msecs) override {
    auto entities = ecs.getEntitiesByComponentsAllOf<Position, Velocity>();
    for (auto e : entities) {
        auto& pos = ecs.entityGetComponent<Position>(e);
        auto& vel = ecs.entityGetComponent<Velocity>(e);
        pos.x += vel.x * msecs / 1000.0f;
        pos.y += vel.y * msecs / 1000.0f;
    }
}
```

### 3.4 Sparse Set - Performance

```
┌─────────────────────────────────────────────────────────────┐
│                    Sparse Set Structure                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Entity IDs:     [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]    │
│  Active:          ✓    ✗    ✓    ✗    ✓    ✗    ✗    ✓     │
│                                                              │
│  Sparse Array:   [ 0 | - | 1 | - | 2 | - | - | 3 ]         │
│                    │       │       │           │            │
│                    ▼       ▼       ▼           ▼            │
│  Dense Array:    ┌───┬───┬───┬───┐                         │
│   (contiguous)   │ 0 │ 2 │ 4 │ 7 │  ← Cache-friendly!      │
│                  └───┴───┴───┴───┘                         │
│                                                              │
│  Components:     ┌───┬───┬───┬───┐                         │
│   (packed)       │C0 │C2 │C4 │C7 │  ← Données côte à côte  │
│                  └───┴───┴───┴───┘                         │
│                                                              │
│  Complexité:                                                 │
│    - has(entity)    : O(1)                                  │
│    - get(entity)    : O(1)                                  │
│    - add(entity)    : O(1) amorti                           │
│    - remove(entity) : O(1) (swap with last)                 │
│    - iterate all    : O(n) avec n = entités actives         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Architecture Cible

### 4.1 Components à Créer

> **Rappel** : Les Components ECS sont des **DTOs** (Data Transfer Objects) internes à l'infrastructure.
> Ils ne contiennent **aucune logique métier**. Le suffixe `Comp` les distingue des Value Objects du Domain.

#### 4.1.1 Components de Base

```cpp
// PositionComp.hpp - Tous les objets positionnés
// Note: Différent de domain/value_objects/Position (qui peut avoir des méthodes métier)
struct PositionComp {
    float x = 0.0f;
    float y = 0.0f;
};

// VelocityComp.hpp - Objets en mouvement
struct VelocityComp {
    float x = 0.0f;
    float y = 0.0f;
};

// HealthComp.hpp - Entités destructibles
// Note: La logique de dégâts est dans domain/services/GameRule
struct HealthComp {
    uint16_t current = 100;
    uint16_t max = 100;
    bool invulnerable = false;  // GodMode
};

// HitboxComp.hpp - Collision AABB
// Note: La logique de collision (intersects) est dans domain/services/CollisionRule
struct HitboxComp {
    float width = 0.0f;
    float height = 0.0f;
    float offsetX = 0.0f;  // Offset depuis Position
    float offsetY = 0.0f;
};

// LifetimeComp.hpp - Entités temporaires
struct LifetimeComp {
    float remaining = 0.0f;
    float total = 0.0f;
};

// OwnerComp.hpp - Appartenance (missiles, force pod, bits)
struct OwnerComp {
    EntityID ownerId = 0;
    bool isPlayerOwned = true;
};
```

#### 4.1.2 Components de Joueur

```cpp
// PlayerTag.hpp - Identifie un joueur (Tag = pas de logique)
struct PlayerTag {
    uint8_t playerId = 0;  // 0-3
    uint8_t shipSkin = 1;  // 1-6
};

// ScoreComp.hpp - Données de score
// Note: Le calcul du combo bonus est dans domain/services/GameRule::applyComboBonus()
struct ScoreComp {
    uint32_t total = 0;
    uint16_t kills = 0;
    uint8_t combo = 10;  // x10 (15 = 1.5x)
    float comboTimer = 0.0f;
};

// WeaponComp.hpp - État de l'armement
// Note: Les dégâts par type sont dans domain/services/GameRule::getWeaponDamage()
struct WeaponComp {
    uint8_t currentType = 0;  // WeaponType enum
    uint8_t level = 0;        // 0-3
    float shootCooldown = 0.0f;
    float chargeTime = 0.0f;  // Wave Cannon
    bool isCharging = false;
};

// SpeedLevelComp.hpp - Niveau de vitesse
// Note: Les multiplicateurs sont dans domain/services/GameRule::getSpeedMultiplier()
struct SpeedLevelComp {
    uint8_t level = 0;  // 0-3
};
```

#### 4.1.3 Components d'Ennemi

```cpp
// EnemyTag.hpp - Identifie un ennemi (Tag = pas de logique)
struct EnemyTag {
    uint8_t type = 0;     // EnemyType enum
    uint16_t points = 0;  // Score à la mort (valeur, pas calcul)
};

// EnemyAIComp.hpp - État du comportement ennemi
// Note: La logique de mouvement est dans domain/services/EnemyBehavior
struct EnemyAIComp {
    float shootTimer = 0.0f;
    float shootInterval = 2.0f;
    uint8_t movementPattern = 0;
    float patternTimer = 0.0f;

    // Pour Tracker
    bool tracksPlayer = false;

    // Pour Zigzag
    float zigzagAmplitude = 50.0f;
    float zigzagFrequency = 2.0f;
};
```

#### 4.1.4 Components Spéciaux

```cpp
// BossTag.hpp - État du boss
// Note: La logique des phases est dans domain/services/BossBehavior
struct BossTag {
    uint8_t phase = 0;       // 0, 1, 2
    float attackTimer = 0.0f;
    float phaseTimer = 0.0f;
    bool isEnraged = false;
};

// MissileTag.hpp - Identifie un missile
// Note: Les dégâts sont calculés par domain/services/GameRule::getMissileDamage()
struct MissileTag {
    uint8_t weaponType = 0;  // WeaponType enum
    uint8_t baseDamage = 10; // Valeur de base, modifiée par GameRule
    bool isHoming = false;
};

// PowerUpTag.hpp - Identifie un power-up (Tag = données brutes)
struct PowerUpTag {
    uint8_t type = 0;  // PowerUpType enum
};

// ForcePodTag.hpp - Force Pod R-Type
// Note: Les dégâts de contact sont dans domain/services/GameRule
struct ForcePodTag {
    uint8_t level = 1;      // 1-2
    bool isAttached = true;
};

// BitDeviceTag.hpp - Bit Devices orbitants
// Note: L'orbite est calculée par domain/services/OrbitCalculator
struct BitDeviceTag {
    uint8_t bitIndex = 0;      // 0 ou 1
    float orbitAngle = 0.0f;
    float shootCooldown = 0.0f;
};

// WaveCannonTag.hpp - Tir chargé
// Note: Les dégâts/largeur sont dans domain/services/GameRule::getWaveCannonStats()
struct WaveCannonTag {
    uint8_t chargeLevel = 1;  // 1-3
};
```

#### Tableau Récapitulatif

| Component | Taille | Usage | Logique déléguée à |
|-----------|--------|-------|-------------------|
| PositionComp | 8 bytes | Tous | - (données pures) |
| VelocityComp | 8 bytes | Mobiles | - (données pures) |
| HealthComp | 5 bytes | Destructibles | `GameRule::calculateDamage()` |
| HitboxComp | 16 bytes | Collidables | `CollisionRule::intersects()` |
| LifetimeComp | 8 bytes | Temporaires | - (données pures) |
| OwnerComp | 5 bytes | Projectiles, pods | - (données pures) |
| PlayerTag | 2 bytes | Joueurs | - (tag d'identification) |
| ScoreComp | 11 bytes | Joueurs | `GameRule::applyComboBonus()` |
| WeaponComp | 7 bytes | Joueurs | `GameRule::getWeaponDamage()` |
| SpeedLevelComp | 1 byte | Joueurs | `GameRule::getSpeedMultiplier()` |
| EnemyTag | 3 bytes | Ennemis | - (tag d'identification) |
| EnemyAIComp | 21 bytes | Ennemis | `EnemyBehavior::getMovement()` |
| BossTag | 10 bytes | Boss | `BossBehavior::getPhaseAction()` |
| MissileTag | 3 bytes | Missiles | `GameRule::getMissileDamage()` |
| PowerUpTag | 1 byte | Power-ups | - (tag d'identification) |
| ForcePodTag | 3 bytes | Force Pods | `GameRule::getForceDamage()` |
| BitDeviceTag | 6 bytes | Bit Devices | `OrbitCalculator::getPosition()` |
| WaveCannonTag | 1 byte | Wave Cannons | `GameRule::getWaveCannonStats()` |

### 4.2 Systems à Implémenter

#### 4.2.1 Liste des Systems

```
┌─────────────────────────────────────────────────────────────┐
│                    SYSTEMS ARCHITECTURE                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              INPUT / NETWORK SYSTEMS                 │   │
│  │  ┌─────────────┐  ┌─────────────┐                   │   │
│  │  │ PlayerInput │  │  NetworkSync│                   │   │
│  │  │   System    │  │    System   │                   │   │
│  │  └─────────────┘  └─────────────┘                   │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│                           ▼                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 LOGIC SYSTEMS                        │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │   Enemy     │  │    Boss     │  │   Weapon    │  │   │
│  │  │ AI System   │  │   System    │  │   System    │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │  ForcePod   │  │ BitDevice   │  │  WaveCannon │  │   │
│  │  │   System    │  │   System    │  │   System    │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│                           ▼                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 PHYSICS SYSTEMS                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │  Movement   │  │  Collision  │  │   Damage    │  │   │
│  │  │   System    │  │   System    │  │   System    │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│                           ▼                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                CLEANUP SYSTEMS                       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │  Lifetime   │  │   Cleanup   │  │   Score     │  │   │
│  │  │   System    │  │   System    │  │   System    │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### 4.2.2 Détail des Systems

> **Règle Fondamentale** : Les Systems ne contiennent **aucune logique métier**.
> Ils itèrent sur les entités et **délèguent au DomainBridge** pour tout calcul.

```cpp
// MovementSystem.hpp - Déplace toutes les entités mobiles
// ✅ Correct : pas de logique métier, juste de l'intégration (pos += vel * dt)
class MovementSystem : public ECS::ISystem {
    DomainBridge& _domain;

public:
    explicit MovementSystem(DomainBridge& domain) : _domain(domain) {}

    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        float dt = msecs / 1000.0f;
        auto entities = ecs.getEntitiesByComponentsAllOf<PositionComp, VelocityComp>();

        for (auto e : entities) {
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& vel = ecs.entityGetComponent<VelocityComp>(e);

            // Optionnel : ajuster la vitesse selon SpeedLevel via Domain
            float speedMult = 1.0f;
            if (ecs.entityHasComponent<SpeedLevelComp>(e)) {
                auto& speed = ecs.entityGetComponent<SpeedLevelComp>(e);
                speedMult = _domain.getSpeedMultiplier(speed.level);
            }

            pos.x += vel.x * speedMult * dt;
            pos.y += vel.y * speedMult * dt;
        }
    }
};

// CollisionSystem.hpp - Détection de collisions
// ✅ Correct : détection = orchestration, logique AABB déléguée au Domain
class CollisionSystem : public ECS::ISystem {
    DomainBridge& _domain;

public:
    explicit CollisionSystem(DomainBridge& domain) : _domain(domain) {}

    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        auto collidables = ecs.getEntitiesByComponentsAllOf<PositionComp, HitboxComp>();

        // Spatial hashing pour éviter O(n²) - pure optimisation, pas de logique métier
        SpatialHash grid(CELL_SIZE);
        for (auto e : collidables) {
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& hbox = ecs.entityGetComponent<HitboxComp>(e);
            grid.insert(e, pos, hbox);
        }

        // Vérifier les paires potentielles
        for (auto e : collidables) {
            auto nearby = grid.query(e);
            for (auto other : nearby) {
                if (e >= other) continue;  // Éviter doublons

                // ✅ Délégation au Domain pour la logique de collision
                if (_domain.checkCollision(ecs, e, other)) {
                    // Émettre un événement, le DamageSystem traitera
                    _collisionEvents.push_back({e, other});
                }
            }
        }
    }

    std::vector<std::pair<EntityID, EntityID>> _collisionEvents;
};

// EnemyAISystem.hpp - IA des ennemis
// ✅ Correct : délègue les patterns de mouvement au Domain
class EnemyAISystem : public ECS::ISystem {
    DomainBridge& _domain;

public:
    explicit EnemyAISystem(DomainBridge& domain) : _domain(domain) {}

    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        float dt = msecs / 1000.0f;
        auto enemies = ecs.getEntitiesByComponentsAllOf<EnemyTag, EnemyAIComp, PositionComp>();

        for (auto e : enemies) {
            auto& ai = ecs.entityGetComponent<EnemyAIComp>(e);
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& tag = ecs.entityGetComponent<EnemyTag>(e);

            // Mise à jour du timer de tir
            ai.shootTimer -= dt;
            if (ai.shootTimer <= 0.0f) {
                // ✅ Délégation : le Domain décide SI l'ennemi peut tirer
                if (_domain.canEnemyShoot(tag.type, ai)) {
                    _missileSpawnRequests.push_back({e, pos, tag.type});
                }
                ai.shootTimer = _domain.getEnemyShootInterval(tag.type);
            }

            // ✅ Délégation : le Domain calcule le delta de mouvement
            auto [dx, dy] = _domain.getEnemyMovement(tag.type, ai, dt);
            pos.x += dx;
            pos.y += dy;

            ai.patternTimer += dt;
        }
    }

    std::vector<MissileSpawnRequest> _missileSpawnRequests;
};

// DamageSystem.hpp - Applique les dégâts
// ✅ Correct : délègue TOUT le calcul au Domain
class DamageSystem : public ECS::ISystem {
    DomainBridge& _domain;

public:
    explicit DamageSystem(DomainBridge& domain) : _domain(domain) {}

    struct DamageEvent {
        EntityID target;
        uint16_t rawDamage;  // Dégâts bruts, sera modifié par GameRule
        EntityID source;
    };

    std::vector<DamageEvent> pendingDamage;

    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        for (auto& evt : pendingDamage) {
            if (!ecs.entityHasComponent<HealthComp>(evt.target)) continue;

            auto& health = ecs.entityGetComponent<HealthComp>(evt.target);

            // ✅ Délégation complète au Domain
            // - Vérifie invulnérabilité (GodMode)
            // - Calcule les dégâts finaux (armure, résistances, etc.)
            // - Détermine si l'entité meurt
            auto result = _domain.applyDamage(health, evt.rawDamage);

            if (result.died) {
                // ✅ Délégation : le Domain gère les conséquences de la mort
                _domain.handleDeath(ecs, evt.target, evt.source);
                _entitiesToDelete.push_back(evt.target);
            }
        }
        pendingDamage.clear();

        // Suppression différée pour éviter les problèmes d'itération
        for (auto e : _entitiesToDelete) {
            ecs.entityDelete(e);
        }
        _entitiesToDelete.clear();
    }

private:
    std::vector<EntityID> _entitiesToDelete;
};

// ScoreSystem.hpp - Gestion du score et combos
// ✅ Correct : délègue les règles de combo au Domain
class ScoreSystem : public ECS::ISystem {
    DomainBridge& _domain;

public:
    explicit ScoreSystem(DomainBridge& domain) : _domain(domain) {}

    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        float dt = msecs / 1000.0f;
        auto players = ecs.getEntitiesByComponentsAllOf<PlayerTag, ScoreComp>();

        for (auto e : players) {
            auto& score = ecs.entityGetComponent<ScoreComp>(e);

            // Décrémentation du timer (pas de logique métier)
            score.comboTimer -= dt;

            if (score.comboTimer <= 0.0f) {
                // ✅ Délégation : le Domain calcule le nouveau combo
                score.combo = _domain.decayCombo(score.combo);
                score.comboTimer = _domain.getComboDecayTime();
            }
        }
    }

    // Appelé par le DamageSystem quand un ennemi meurt
    void addKillScore(ECS::ECS& ecs, EntityID player, EntityID killedEnemy) {
        if (!ecs.entityHasComponent<ScoreComp>(player)) return;
        if (!ecs.entityHasComponent<EnemyTag>(killedEnemy)) return;

        auto& score = ecs.entityGetComponent<ScoreComp>(player);
        auto& enemy = ecs.entityGetComponent<EnemyTag>(killedEnemy);

        // ✅ Délégation : le Domain calcule le score final avec combo
        uint32_t points = _domain.calculateKillScore(enemy.points, score.combo);

        score.total += points;
        score.kills += 1;
        score.combo = _domain.incrementCombo(score.combo);
        score.comboTimer = _domain.getComboDecayTime();
    }
};

// LifetimeSystem.hpp - Supprime les entités expirées
// ✅ Correct : pas de logique métier, juste de la gestion de timer
class LifetimeSystem : public ECS::ISystem {
public:
    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        float dt = msecs / 1000.0f;
        auto entities = ecs.getEntitiesByComponentsAllOf<LifetimeComp>();

        std::vector<EntityID> toDelete;

        for (auto e : entities) {
            auto& life = ecs.entityGetComponent<LifetimeComp>(e);
            life.remaining -= dt;

            if (life.remaining <= 0.0f) {
                toDelete.push_back(e);
            }
        }

        for (auto e : toDelete) {
            ecs.entityDelete(e);
        }
    }
};
```

#### 4.2.3 Ordre d'Exécution

| Priorité | System | Tickrate | Description |
|----------|--------|----------|-------------|
| 1 | PlayerInputSystem | 0 | Traite les inputs réseau |
| 2 | EnemyAISystem | 0 | Décisions IA |
| 3 | BossSystem | 0 | Logique du boss |
| 4 | WeaponSystem | 0 | Tirs et recharges |
| 5 | ForcePodSystem | 0 | Suivi et tirs Force |
| 6 | BitDeviceSystem | 0 | Orbite et tirs Bits |
| 7 | WaveCannonSystem | 0 | Charge et projectiles |
| 8 | MovementSystem | 0 | Déplacements |
| 9 | CollisionSystem | 0 | Détection collisions |
| 10 | DamageSystem | 0 | Application dégâts |
| 11 | LifetimeSystem | 0 | Entités temporaires |
| 12 | CleanupSystem | 0 | Suppression OOB |
| 13 | ScoreSystem | 0 | Combos et score |
| 14 | NetworkSyncSystem | 0 | Broadcast snapshots |

### 4.3 Diagramme d'Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                      NOUVELLE ARCHITECTURE ECS                       │
│                    (Respecte l'Architecture Hexagonale)              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                     DOMAIN LAYER (inchangé)                 │    │
│  │  ┌─────────────────────────────────────────────────────┐  │    │
│  │  │                 domain/services/                     │  │    │
│  │  │  GameRule       EnemyBehavior    CollisionRule      │  │    │
│  │  │  ├ calculateDamage()   ├ getMovement()   ├ checkAABB()│ │    │
│  │  │  ├ shouldEntityDie()   ├ canShoot()      └──────────┘ │ │    │
│  │  │  ├ applyComboBonus()   └────────────┘                  │ │    │
│  │  │  └ getSpeedMultiplier()                                │ │    │
│  │  └─────────────────────────────────────────────────────┘  │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                   ▲                                 │
│                                   │ délègue                         │
│  ┌────────────────────────────────┴───────────────────────────┐    │
│  │                   INFRASTRUCTURE LAYER                      │    │
│  │                                                              │    │
│  │  ┌──────────────────────────────────────────────────────┐  │    │
│  │  │                    DomainBridge                       │  │    │
│  │  │  (Pont entre ECS Systems et Domain Services)          │  │    │
│  │  │  ├ applyDamage() → GameRule.calculateDamage()        │  │    │
│  │  │  ├ getEnemyMovement() → EnemyBehavior.getMovement()  │  │    │
│  │  │  └ checkCollision() → CollisionRule.checkAABB()      │  │    │
│  │  └──────────────────────────────────────────────────────┘  │    │
│  │                            ▲                                │    │
│  │                            │ utilise                        │    │
│  │  ┌─────────────────────────┴────────────────────────────┐  │    │
│  │  │                        ECS::ECS                       │  │    │
│  │  │                                                        │  │    │
│  │  │  ┌────────────────────────────────────────────────┐  │  │    │
│  │  │  │               COMPONENT POOLS (DTOs)            │  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  Pool<PositionComp>  Pool<VelocityComp>         │  │  │    │
│  │  │  │  ┌───────────┐       ┌───────────┐              │  │  │    │
│  │  │  │  │42:(100,200)│      │42: (0,-5) │              │  │  │    │
│  │  │  │  │43:(200,300)│      │43: (0, 3) │              │  │  │    │
│  │  │  │  └───────────┘       └───────────┘              │  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  Pool<HealthComp>    Pool<EnemyAIComp>          │  │  │    │
│  │  │  │  ┌───────────┐       ┌───────────┐              │  │  │    │
│  │  │  │  │42:(80,100)│       │100:(timer)│              │  │  │    │
│  │  │  │  └───────────┘       └───────────┘              │  │  │    │
│  │  │  └────────────────────────────────────────────────┘  │  │    │
│  │  │                                                        │  │    │
│  │  │  ┌────────────────────────────────────────────────┐  │  │    │
│  │  │  │             SYSTEMS (Orchestration)             │  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  MovementSystem(DomainBridge&)                  │  │  │    │
│  │  │  │    └→ Itère entités, appelle domain.getSpeed()  │  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  DamageSystem(DomainBridge&)                    │  │  │    │
│  │  │  │    └→ Itère dégâts, appelle domain.applyDamage()│  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  EnemyAISystem(DomainBridge&)                   │  │  │    │
│  │  │  │    └→ Itère ennemis, appelle domain.getMovement()│ │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  │  NetworkSyncSystem (pas de DomainBridge)        │  │  │    │
│  │  │  │    └→ Pure conversion ECS → Protocol            │  │  │    │
│  │  │  │                                                  │  │  │    │
│  │  │  └────────────────────────────────────────────────┘  │  │    │
│  │  │                                                        │  │    │
│  │  └────────────────────────────────────────────────────────┘  │    │
│  │                                                              │    │
│  │  ┌──────────────────────────────────────────────────────┐  │    │
│  │  │                  GameWorld (Façade)                   │  │    │
│  │  │                                                        │  │    │
│  │  │  - Possède ECS::ECS + DomainBridge                    │  │    │
│  │  │  - Expose API compatible avec UDPServer existant      │  │    │
│  │  │  - Initialise Systems avec DomainBridge&              │  │    │
│  │  │                                                        │  │    │
│  │  └──────────────────────────────────────────────────────┘  │    │
│  │                                                              │    │
│  └──────────────────────────────────────────────────────────────┘    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

FLUX DE DONNÉES :
─────────────────
1. GameWorld.update() → ECS.Update()
2. Chaque System itère sur ses Components (DTOs)
3. Pour toute décision métier → System appelle DomainBridge
4. DomainBridge délègue au Domain (GameRule, EnemyBehavior, etc.)
5. System applique le résultat aux Components

RÈGLE FONDAMENTALE :
────────────────────
Les Systems NE CONTIENNENT PAS de logique métier.
Ils orchestrent les données et délèguent les décisions au Domain.
```

---

## 5. Plan de Migration

### 5.1 Phase 1 - Fondations

**Objectif** : Intégrer l'ECS sans casser l'existant.

```
┌─────────────────────────────────────────────────────────────┐
│                      PHASE 1 - FONDATIONS                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Semaine 1                                                   │
│  ─────────                                                   │
│                                                              │
│  Jour 1-2: Configuration                                     │
│    ☐ Copier src/ECS/ → src/server/infrastructure/ecs/       │
│    ☐ Adapter les includes                                    │
│    ☐ Ajouter au CMakeLists.txt                              │
│    ☐ Tests de compilation                                    │
│                                                              │
│  Jour 3-4: Components de base                                │
│    ☐ Créer Position, Velocity, Health, Hitbox               │
│    ☐ Créer Lifetime, Owner                                   │
│    ☐ Tests unitaires pour chaque component                   │
│                                                              │
│  Jour 5: Intégration GameWorld                               │
│    ☐ Ajouter ECS::ECS _ecs dans GameWorld                   │
│    ☐ Enregistrer les components de base                      │
│    ☐ Vérifier compilation + tests existants                  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Critères de succès** :
- [x] Compilation sans erreur
- [x] Tests unitaires passent
- [x] GameWorld fonctionne en parallèle (legacy + ECS)

### 5.2 Phase 2 - Entités Simples

**Objectif** : Migrer les missiles et power-ups.

```
┌─────────────────────────────────────────────────────────────┐
│                   PHASE 2 - ENTITÉS SIMPLES                  │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Semaine 2                                                   │
│  ─────────                                                   │
│                                                              │
│  Jour 1-2: Missiles                                          │
│    ☐ Créer MissileTag component                             │
│    ☐ Créer MissileSpawnSystem                               │
│    ☐ Migrer _missiles map vers ECS                          │
│    ☐ Adapter spawnMissile() pour utiliser ECS               │
│    ☐ Tests: missiles spawn, move, despawn                   │
│                                                              │
│  Jour 3: MovementSystem                                      │
│    ☐ Implémenter MovementSystem                             │
│    ☐ Migrer logique de updateMissiles()                     │
│    ☐ Tests: vitesse, directions, limites écran              │
│                                                              │
│  Jour 4-5: Power-ups                                         │
│    ☐ Créer PowerUpTag component                             │
│    ☐ Migrer _powerups map vers ECS                          │
│    ☐ Utiliser LifetimeSystem pour expiration                │
│    ☐ Tests: spawn, collection, expiration                   │
│                                                              │
│  Semaine 2.5                                                 │
│  ──────────                                                  │
│                                                              │
│  Jour 6-7: WaveCannons                                       │
│    ☐ Créer WaveCannonTag component                          │
│    ☐ Créer WaveCannonSystem                                 │
│    ☐ Migrer _waveCannons map vers ECS                       │
│    ☐ Tests: charge, release, collision                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Code exemple - Migration des missiles** :

```cpp
// AVANT (GameWorld.cpp)
void GameWorld::spawnMissile(uint8_t playerId) {
    auto it = _players.find(playerId);
    if (it == _players.end()) return;

    MissileData missile;
    missile.id = _nextMissileId++;
    missile.owner = playerId;
    missile.x = it->second.x + 50;
    missile.y = it->second.y + 15;
    missile.weapon_type = it->second.currentWeapon;

    _missiles[missile.id] = missile;
}

// APRÈS (GameWorld.cpp avec ECS + DomainBridge)
void GameWorld::spawnMissile(uint8_t playerId) {
    auto players = _ecs.getEntitiesByComponentsAllOf<PlayerTag, PositionComp, WeaponComp>();

    for (auto e : players) {
        auto& tag = _ecs.entityGetComponent<PlayerTag>(e);
        if (tag.playerId != playerId) continue;

        auto& pos = _ecs.entityGetComponent<PositionComp>(e);
        auto& weapon = _ecs.entityGetComponent<WeaponComp>(e);

        // Créer l'entité missile
        EntityID missile = _ecs.entityCreate(EntityGroup::MISSILES);

        auto& mPos = _ecs.entityAddComponent<PositionComp>(missile);
        mPos.x = pos.x + 50.0f;
        mPos.y = pos.y + 15.0f;

        auto& mVel = _ecs.entityAddComponent<VelocityComp>(missile);
        // ✅ Délégation : la vitesse du missile vient du Domain
        mVel.x = _domainBridge.getMissileSpeed(weapon.currentType);
        mVel.y = 0.0f;

        auto& mTag = _ecs.entityAddComponent<MissileTag>(missile);
        mTag.weaponType = weapon.currentType;
        // ✅ Délégation : les dégâts viennent du Domain
        mTag.baseDamage = _domainBridge.getMissileDamage(weapon.currentType, weapon.level);

        // ✅ Délégation : les dimensions de hitbox viennent du Domain
        auto hitboxSize = _domainBridge.getMissileHitbox(weapon.currentType);
        auto& mHitbox = _ecs.entityAddComponent<HitboxComp>(missile);
        mHitbox.width = hitboxSize.width;
        mHitbox.height = hitboxSize.height;

        auto& mOwner = _ecs.entityAddComponent<OwnerComp>(missile);
        mOwner.ownerId = e;
        mOwner.isPlayerOwned = true;

        break;
    }
}
```

### 5.3 Phase 3 - Entités Complexes

**Objectif** : Migrer joueurs, ennemis, boss, Force Pod, Bit Devices.

```
┌─────────────────────────────────────────────────────────────┐
│                  PHASE 3 - ENTITÉS COMPLEXES                 │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Semaine 3                                                   │
│  ─────────                                                   │
│                                                              │
│  Jour 1-2: Joueurs                                           │
│    ☐ Créer PlayerTag, Score, Weapon, SpeedLevel components  │
│    ☐ Migrer _players map vers ECS                           │
│    ☐ Créer PlayerInputSystem                                │
│    ☐ Adapter addPlayer(), removePlayer()                    │
│    ☐ Tests: spawn, input, movement, death                   │
│                                                              │
│  Jour 3-4: Ennemis                                           │
│    ☐ Créer EnemyTag, EnemyAI components                     │
│    ☐ Créer EnemyAISystem                                    │
│    ☐ Migrer _enemies map vers ECS                           │
│    ☐ Porter la logique de spawnWave()                       │
│    ☐ Tests: spawn, AI, tir, death                           │
│                                                              │
│  Jour 5: Force Pod                                           │
│    ☐ Créer ForcePodTag component                            │
│    ☐ Créer ForcePodSystem                                   │
│    ☐ Migrer _forcePods map vers ECS                         │
│    ☐ Tests: attach/detach, suivi, collision                 │
│                                                              │
│  Semaine 3.5                                                 │
│  ──────────                                                  │
│                                                              │
│  Jour 6: Bit Devices                                         │
│    ☐ Créer BitDeviceTag component                           │
│    ☐ Créer BitDeviceSystem                                  │
│    ☐ Migrer _bitDevices map vers ECS                        │
│    ☐ Tests: orbite, tir                                     │
│                                                              │
│  Jour 7: Boss                                                │
│    ☐ Créer BossTag component                                │
│    ☐ Créer BossSystem                                       │
│    ☐ Porter la logique de boss phases                       │
│    ☐ Tests: spawn, phases, patterns, death                  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 5.4 Phase 4 - Intégration Réseau

**Objectif** : Adapter NetworkSyncSystem et garantir la compatibilité protocol.

```
┌─────────────────────────────────────────────────────────────┐
│                PHASE 4 - INTÉGRATION RÉSEAU                  │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Semaine 4                                                   │
│  ─────────                                                   │
│                                                              │
│  Jour 1-2: Collision System                                  │
│    ☐ Implémenter CollisionSystem avec spatial hash          │
│    ☐ Supprimer checkCollisions() legacy                     │
│    ☐ Tests: player-enemy, missile-enemy, powerup-player     │
│                                                              │
│  Jour 3-4: NetworkSyncSystem                                 │
│    ☐ Créer NetworkSyncSystem                                │
│    ☐ Convertir état ECS → GameSnapshot                      │
│    ☐ Vérifier compatibilité Protocol.hpp                    │
│    ☐ Tests: snapshot serialization, network roundtrip       │
│                                                              │
│  Jour 5: Cleanup                                             │
│    ☐ Supprimer les anciennes maps de GameWorld              │
│    ☐ Supprimer les boucles de mise à jour legacy            │
│    ☐ Documentation du nouveau système                       │
│    ☐ Tests d'intégration complets                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 6. Compatibilité Réseau

### 6.1 Problème

Le protocol binaire (`Protocol.hpp`) définit des structures fixes :
- `PlayerState` (23 bytes)
- `EnemyState` (12 bytes)
- `MissileState` (8 bytes)
- `GameSnapshot` (variable)

Ces structures doivent rester identiques pour la compatibilité client-serveur.

### 6.2 Solution : Couche d'Adaptation

```cpp
// NetworkSyncSystem.hpp
// Note: Ce System ne contient pas de logique métier, il fait de la conversion de format
class NetworkSyncSystem : public ECS::ISystem {
public:
    void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override {
        // Construction du snapshot à partir de l'ECS
        GameSnapshot snapshot = buildSnapshot(ecs);

        // Envoi via le callback
        if (_broadcastCallback) {
            _broadcastCallback(snapshot);
        }
    }

private:
    GameSnapshot buildSnapshot(ECS::ECS& ecs) {
        GameSnapshot snap;

        // Players - Conversion ECS Components → Protocol Structures
        auto players = ecs.getEntitiesByComponentsAllOf<PlayerTag, PositionComp, HealthComp>();
        snap.player_count = std::min((size_t)MAX_PLAYERS, players.size());

        for (size_t i = 0; i < snap.player_count; ++i) {
            auto e = players[i];
            auto& tag = ecs.entityGetComponent<PlayerTag>(e);
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& hp = ecs.entityGetComponent<HealthComp>(e);
            auto& score = ecs.entityGetComponent<ScoreComp>(e);
            auto& weapon = ecs.entityGetComponent<WeaponComp>(e);

            // Pure conversion de données, pas de logique métier
            snap.players[i] = PlayerState{
                .id = tag.playerId,
                .x = static_cast<uint16_t>(pos.x),
                .y = static_cast<uint16_t>(pos.y),
                .health = static_cast<uint8_t>(hp.current),
                .alive = hp.current > 0 ? 1 : 0,
                .score = score.total,
                .kills = score.kills,
                .combo = score.combo,
                .currentWeapon = weapon.currentType,
                .chargeLevel = static_cast<uint8_t>(weapon.chargeTime),
                // ... autres champs
            };
        }

        // Enemies - Conversion similaire
        auto enemies = ecs.getEntitiesByComponentsAllOf<EnemyTag, PositionComp, HealthComp>();
        snap.enemy_count = std::min((size_t)MAX_ENEMIES, enemies.size());

        for (size_t i = 0; i < snap.enemy_count; ++i) {
            auto e = enemies[i];
            auto& tag = ecs.entityGetComponent<EnemyTag>(e);
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& hp = ecs.entityGetComponent<HealthComp>(e);

            snap.enemies[i] = EnemyState{
                .id = static_cast<uint16_t>(e),  // Entity ID comme ID réseau
                .type = tag.type,
                .x = static_cast<uint16_t>(pos.x),
                .y = static_cast<uint16_t>(pos.y),
                .health = static_cast<uint8_t>(hp.current),
            };
        }

        // Missiles - Conversion similaire
        auto missiles = ecs.getEntitiesByComponentsAllOf<MissileTag, PositionComp>();
        snap.missile_count = std::min((size_t)MAX_MISSILES, missiles.size());

        for (size_t i = 0; i < snap.missile_count; ++i) {
            auto e = missiles[i];
            auto& tag = ecs.entityGetComponent<MissileTag>(e);
            auto& pos = ecs.entityGetComponent<PositionComp>(e);
            auto& owner = ecs.entityGetComponent<OwnerComp>(e);

            snap.missiles[i] = MissileState{
                .id = static_cast<uint16_t>(e),
                .owner_id = static_cast<uint8_t>(owner.ownerId),
                .x = static_cast<uint16_t>(pos.x),
                .y = static_cast<uint16_t>(pos.y),
                .weapon_type = tag.weaponType,
            };
        }

        return snap;
    }
};
```

### 6.3 Mapping ECS → Protocol

| Protocol Struct | ECS Components |
|-----------------|----------------|
| `PlayerState` | PlayerTag + PositionComp + HealthComp + ScoreComp + WeaponComp + SpeedLevelComp |
| `EnemyState` | EnemyTag + PositionComp + HealthComp |
| `MissileState` | MissileTag + PositionComp + OwnerComp |
| `BossState` | BossTag + PositionComp + HealthComp |
| `PowerUpState` | PowerUpTag + PositionComp + LifetimeComp |
| `ForceStateSnapshot` | ForcePodTag + PositionComp + OwnerComp |
| `BitDeviceStateSnapshot` | BitDeviceTag + PositionComp + OwnerComp |

---

## 7. Stratégie de Tests

### 7.1 Tests Unitaires par Phase

> **Important** : Les tests vérifient aussi que les Systems **délèguent au Domain**.

```cpp
// tests/server/ecs/ComponentTests.cpp
TEST(ECSComponents, PositionComp_DefaultValues) {
    PositionComp pos;
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(ECSComponents, HealthComp_IsDTO) {
    // Les Components sont des DTOs - pas de logique
    HealthComp hp{100, 100, false};
    hp.current -= 30;  // Modification directe autorisée
    EXPECT_EQ(hp.current, 70);
    EXPECT_FALSE(hp.invulnerable);
    // Note: La logique de "shouldDie" est dans GameRule, pas ici
}

// tests/server/ecs/SystemTests.cpp
TEST(ECSMovement, MovementSystem_UpdatesPosition) {
    ECS::ECS ecs;
    ecs.registerComponent<PositionComp>();
    ecs.registerComponent<VelocityComp>();

    // Mock du DomainBridge pour tester l'intégration
    MockGameRule gameRule;
    DomainBridge domain(gameRule);

    auto moveSys = ecs.addSystem<MovementSystem>(domain);

    auto e = ecs.entityCreate();
    auto& pos = ecs.entityAddComponent<PositionComp>(e);
    pos.x = 100.0f;
    pos.y = 200.0f;

    auto& vel = ecs.entityAddComponent<VelocityComp>(e);
    vel.x = 50.0f;  // 50 px/s
    vel.y = -25.0f;

    ecs.Update(1000);  // 1 seconde

    EXPECT_FLOAT_EQ(pos.x, 150.0f);
    EXPECT_FLOAT_EQ(pos.y, 175.0f);
}

TEST(ECSCollision, CollisionSystem_DelegatesToDomain) {
    ECS::ECS ecs;
    ecs.registerComponent<PositionComp>();
    ecs.registerComponent<HitboxComp>();

    // ✅ Vérifie que le System délègue au Domain
    MockGameRule gameRule;
    EXPECT_CALL(gameRule, checkAABB(_, _)).Times(1).WillOnce(Return(true));

    DomainBridge domain(gameRule);
    CollisionSystem collider(domain);

    auto e1 = ecs.entityCreate();
    ecs.entityAddComponent<PositionComp>(e1) = {100.0f, 100.0f};
    ecs.entityAddComponent<HitboxComp>(e1) = {50.0f, 50.0f, 0, 0};

    auto e2 = ecs.entityCreate();
    ecs.entityAddComponent<PositionComp>(e2) = {120.0f, 120.0f};  // Overlapping
    ecs.entityAddComponent<HitboxComp>(e2) = {50.0f, 50.0f, 0, 0};

    collider.Update(ecs, 0, 16);

    EXPECT_EQ(collider._collisionEvents.size(), 1);
}

// tests/server/domain/GameRuleTests.cpp
// ✅ La logique métier est testée séparément du ECS
TEST(GameRule, CalculateDamage_AppliesArmor) {
    GameRule rule;

    // Logique métier pure, pas de dépendance ECS
    uint16_t rawDamage = 100;
    uint16_t finalDamage = rule.calculateDamage(rawDamage);

    EXPECT_EQ(finalDamage, 100);  // Pas d'armure par défaut
}

TEST(GameRule, ShouldEntityDie_WhenHealthBelowDamage) {
    GameRule rule;

    EXPECT_TRUE(rule.shouldEntityDie(50, 60));   // 50 HP, 60 dégâts → mort
    EXPECT_FALSE(rule.shouldEntityDie(100, 50)); // 100 HP, 50 dégâts → survit
}

TEST(GameRule, ApplyComboBonus_MultipliesScore) {
    GameRule rule;

    // combo = 15 → 1.5x
    uint32_t baseScore = 100;
    uint8_t combo = 15;

    uint32_t finalScore = rule.applyComboBonus(baseScore, combo);
    EXPECT_EQ(finalScore, 150);  // 100 * 1.5 = 150
}
```

### 7.2 Tests d'Intégration

```cpp
// tests/server/integration/ECSGameWorldTests.cpp
TEST(ECSIntegration, FullGameLoop) {
    GameWorld world;  // Utilise ECS en interne

    // Setup
    world.addPlayer(0);
    world.spawnWave(1);

    // Simulate 60 frames
    for (int i = 0; i < 60; ++i) {
        world.update(16);  // 16ms = ~60fps
    }

    // Verify snapshot
    auto snapshot = world.getSnapshot();
    EXPECT_EQ(snapshot.player_count, 1);
    EXPECT_GT(snapshot.enemy_count, 0);
}

TEST(ECSIntegration, MissileKillsEnemy) {
    GameWorld world;

    world.addPlayer(0);
    world.spawnEnemy(EnemyType::Basic, 200.0f, 100.0f);

    // Position player near enemy
    world.updatePlayerPosition(0, 100, 100);
    world.spawnMissile(0);

    // Run until collision
    for (int i = 0; i < 10; ++i) {
        world.update(16);
    }

    auto snapshot = world.getSnapshot();
    EXPECT_EQ(snapshot.enemy_count, 0);  // Enemy killed
    EXPECT_EQ(snapshot.players[0].kills, 1);
}
```

### 7.3 Tests de Non-Régression Réseau

```cpp
// tests/server/network/ProtocolCompatibilityTests.cpp
TEST(Protocol, SnapshotSerialization_ECS) {
    GameWorld world;
    world.addPlayer(0);
    world.updatePlayerPosition(0, 100, 200);

    auto snapshot = world.getSnapshot();

    // Serialize
    uint8_t buffer[2048];
    size_t len = snapshot.to_bytes(buffer);

    // Deserialize
    auto parsed = GameSnapshot::from_bytes(buffer, len);
    ASSERT_TRUE(parsed.has_value());

    // Verify
    EXPECT_EQ(parsed->player_count, 1);
    EXPECT_EQ(parsed->players[0].id, 0);
    EXPECT_EQ(parsed->players[0].x, 100);
    EXPECT_EQ(parsed->players[0].y, 200);
}
```

---

## 8. Risques et Mitigations

### 8.1 Matrice des Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| **Régression réseau** | Haute | Critique | Tests Protocol exhaustifs avant merge |
| **Performance dégradée** | Moyenne | Élevé | Benchmarks avant/après, profiling |
| **Bug ECS Blob** | Moyenne | Moyen | Tests unitaires poussés sur l'ECS |
| **Délai de migration** | Haute | Moyen | Migration incrémentale, feature flags |
| **Complexité snapshot** | Moyenne | Moyen | Couche d'adaptation bien testée |

### 8.2 Plan de Rollback

```
┌─────────────────────────────────────────────────────────────┐
│                      STRATÉGIE DE ROLLBACK                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. Feature Flag                                             │
│     ┌──────────────────────────────────────────────────┐   │
│     │  #define USE_ECS_BACKEND 1  // 0 = legacy        │   │
│     │                                                    │   │
│     │  void GameWorld::update(float dt) {               │   │
│     │      #if USE_ECS_BACKEND                          │   │
│     │          _ecs.Update(dt * 1000);                  │   │
│     │      #else                                        │   │
│     │          updateLegacy(dt);                        │   │
│     │      #endif                                       │   │
│     │  }                                                │   │
│     └──────────────────────────────────────────────────┘   │
│                                                              │
│  2. Dual Running (Debug Only)                                │
│     ┌──────────────────────────────────────────────────┐   │
│     │  void GameWorld::update(float dt) {               │   │
│     │      // Run both, compare outputs                 │   │
│     │      _ecs.Update(dt * 1000);                      │   │
│     │      auto ecsSnapshot = buildECSSnapshot();       │   │
│     │                                                    │   │
│     │      updateLegacy(dt);                            │   │
│     │      auto legacySnapshot = buildLegacySnapshot(); │   │
│     │                                                    │   │
│     │      assert(compareSnapshots(ecsSnapshot,         │   │
│     │                              legacySnapshot));    │   │
│     │  }                                                │   │
│     └──────────────────────────────────────────────────┘   │
│                                                              │
│  3. Git Branches                                             │
│     - main: Production (legacy)                             │
│     - feature/ecs-migration: Migration en cours             │
│     - ecs-stable: Dernière version ECS validée              │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 8.3 Critères Go/No-Go

| Phase | Critère Go | Action No-Go |
|-------|------------|--------------|
| Phase 1 | Tests compilent + passent | Corriger avant de continuer |
| Phase 2 | Missiles fonctionnent identiquement | Rollback au legacy, debug |
| Phase 3 | Snapshot identique à 99.9% | Identifier les divergences |
| Phase 4 | Tests réseau passent, latence < +5% | Optimiser ou rollback |

---

## 9. Références

### Documentation Interne

- [src/ECS/ECS.hpp](src/ECS/ECS.hpp) - Code source de l'ECS Blob
- [src/ECS/Component.hpp](src/ECS/Component.hpp) - Sparse Set implementation
- [src/common/protocol/Protocol.hpp](src/common/protocol/Protocol.hpp) - Structures réseau
- [src/server/infrastructure/game/GameWorld.hpp](src/server/infrastructure/game/GameWorld.hpp) - Architecture actuelle

### Articles et Ressources ECS

- [ECS FAQ](https://github.com/SanderMertens/ecs-faq) - FAQ complète sur les ECS
- [Data Locality](https://gameprogrammingpatterns.com/data-locality.html) - Game Programming Patterns
- [Sparse Set](https://www.geeksforgeeks.org/sparse-set/) - Structure de données
- [EnTT](https://github.com/skypjack/entt) - ECS de référence (inspiration)

### Design Patterns

- [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) - Wikipedia
- [Game Object Component](https://gameprogrammingpatterns.com/component.html) - Game Programming Patterns

---

## Annexes

### A. Structure des Dossiers Cible

```
src/server/
├── include/
│   └── domain/
│       └── services/                 # ✅ Logique métier (INCHANGÉ)
│           ├── GameRule.hpp          # Dégâts, score, combos
│           ├── CollisionRule.hpp     # Logique AABB
│           ├── EnemyBehavior.hpp     # Patterns de mouvement
│           ├── BossBehavior.hpp      # Phases du boss
│           └── OrbitCalculator.hpp   # Calculs orbitaux (Bits)
│
├── infrastructure/
│   ├── ecs/                          # ECS Blob (copié et adapté)
│   │   ├── ECS.hpp
│   │   ├── Component.hpp
│   │   ├── Registry.hpp
│   │   ├── System.hpp
│   │   ├── Includes.hpp
│   │   └── Errors.hpp
│   │
│   ├── ecs/bridge/                   # ✅ NOUVEAU : Pont vers le Domain
│   │   └── DomainBridge.hpp          # Délègue au domain/services/
│   │
│   ├── ecs/components/               # Components R-Type (DTOs)
│   │   ├── PositionComp.hpp          # Suffixe Comp pour éviter confusion
│   │   ├── VelocityComp.hpp
│   │   ├── HealthComp.hpp
│   │   ├── HitboxComp.hpp
│   │   ├── LifetimeComp.hpp
│   │   ├── OwnerComp.hpp
│   │   ├── PlayerTag.hpp             # Tags = pas de suffixe (identification)
│   │   ├── ScoreComp.hpp
│   │   ├── WeaponComp.hpp
│   │   ├── SpeedLevelComp.hpp
│   │   ├── EnemyTag.hpp
│   │   ├── EnemyAIComp.hpp
│   │   ├── BossTag.hpp
│   │   ├── MissileTag.hpp
│   │   ├── PowerUpTag.hpp
│   │   ├── ForcePodTag.hpp
│   │   ├── BitDeviceTag.hpp
│   │   └── WaveCannonTag.hpp
│   │
│   └── ecs/systems/                  # Systems R-Type (orchestration)
│       ├── MovementSystem.hpp        # Tous reçoivent DomainBridge&
│       ├── CollisionSystem.hpp
│       ├── DamageSystem.hpp
│       ├── EnemyAISystem.hpp
│       ├── BossSystem.hpp
│       ├── WeaponSystem.hpp
│       ├── ForcePodSystem.hpp
│       ├── BitDeviceSystem.hpp
│       ├── WaveCannonSystem.hpp
│       ├── LifetimeSystem.hpp        # Exception : pas de logique métier
│       ├── ScoreSystem.hpp
│       ├── PlayerInputSystem.hpp
│       ├── CleanupSystem.hpp
│       └── NetworkSyncSystem.hpp     # Pure conversion, pas de Domain
```

**Règle de dépendance** :
```
Systems → DomainBridge → domain/services/*
         (infrastructure)     (domain)
```

### B. Checklist de Migration

```
Phase 0 - Préparation Domain (NOUVEAU)
  ☐ Créer domain/services/GameRule.hpp (si n'existe pas)
  ☐ Ajouter méthodes : calculateDamage(), shouldEntityDie(), applyComboBonus()
  ☐ Ajouter méthodes : getSpeedMultiplier(), getMissileDamage(), getWeaponDamage()
  ☐ Créer domain/services/EnemyBehavior.hpp
  ☐ Créer domain/services/CollisionRule.hpp (wrapper AABB)
  ☐ Tests unitaires pour chaque service Domain

Phase 1 - Fondations
  ☐ Copie ECS dans infrastructure/ecs/
  ☐ Adaptation CMakeLists.txt
  ☐ Création DomainBridge.hpp dans infrastructure/ecs/bridge/
  ☐ Création PositionComp, VelocityComp, HealthComp, HitboxComp, LifetimeComp, OwnerComp
  ☐ Tests unitaires components (vérifier qu'ils sont des DTOs purs)
  ☐ Intégration ECS::ECS + DomainBridge dans GameWorld

Phase 2 - Entités Simples
  ☐ MissileTag component
  ☐ MovementSystem (avec DomainBridge&)
  ☐ Migration _missiles
  ☐ PowerUpTag component
  ☐ Migration _powerups
  ☐ WaveCannonTag component
  ☐ WaveCannonSystem (avec DomainBridge&)

Phase 3 - Entités Complexes
  ☐ PlayerTag, ScoreComp, WeaponComp, SpeedLevelComp
  ☐ PlayerInputSystem (avec DomainBridge&)
  ☐ Migration _players
  ☐ EnemyTag, EnemyAIComp
  ☐ EnemyAISystem (avec DomainBridge&)
  ☐ Migration _enemies
  ☐ ForcePodTag, ForcePodSystem (avec DomainBridge&)
  ☐ BitDeviceTag, BitDeviceSystem (avec DomainBridge&)
  ☐ BossTag, BossSystem (avec DomainBridge&)

Phase 4 - Intégration Réseau
  ☐ CollisionSystem avec spatial hashing (avec DomainBridge&)
  ☐ DamageSystem (avec DomainBridge&)
  ☐ ScoreSystem (avec DomainBridge&)
  ☐ NetworkSyncSystem (sans DomainBridge - pure conversion)
  ☐ Suppression code legacy
  ☐ Documentation
  ☐ Tests d'intégration finaux
  ☐ ✅ Validation : aucune logique métier dans les Systems
```

### C. Constantes à Définir

```cpp
// ECSConstants.hpp

namespace ECSConstants {
    // Entity Groups
    enum EntityGroup : uint8_t {
        NONE = 0,
        PLAYERS = 1,
        ENEMIES = 2,
        MISSILES = 3,
        POWERUPS = 4,
        WAVE_CANNONS = 5,
        FORCE_PODS = 6,
        BIT_DEVICES = 7,
        BOSS = 8
    };

    // System Priorities (lower = earlier)
    constexpr int PRIORITY_INPUT = 0;
    constexpr int PRIORITY_AI = 100;
    constexpr int PRIORITY_WEAPON = 200;
    constexpr int PRIORITY_MOVEMENT = 300;
    constexpr int PRIORITY_COLLISION = 400;
    constexpr int PRIORITY_DAMAGE = 500;
    constexpr int PRIORITY_LIFETIME = 600;
    constexpr int PRIORITY_CLEANUP = 700;
    constexpr int PRIORITY_SCORE = 800;
    constexpr int PRIORITY_NETWORK = 900;

    // Collision Grid
    constexpr float COLLISION_CELL_SIZE = 100.0f;
}
```

---

*Document généré pour le projet R-Type - Janvier 2026*
