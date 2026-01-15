# R-Type - Document de Design : Améliorations IA Ennemie

> **Version**: 1.0
> **Date**: Janvier 2026
> **Auteur**: Équipe R-Type
> **Statut**: Proposition de design

---

## Table des Matières

1. [Résumé Exécutif](#1-résumé-exécutif)
2. [Analyse du Système Actuel](#2-analyse-du-système-actuel)
3. [Problème Identifié](#3-problème-identifié)
4. [Solutions Proposées](#4-solutions-proposées)
   - 4.1 [Système de Difficulté Progressive](#41-système-de-difficulté-progressive)
   - 4.2 [Tir Prédictif (Lead Shots)](#42-tir-prédictif-lead-shots)
   - 4.3 [IA d'Esquive Ennemie](#43-ia-desquive-ennemie)
   - 4.4 [Adaptation au Joueur](#44-adaptation-au-joueur)
   - 4.5 [Formations Coordonnées](#45-formations-coordonnées)
   - 4.6 [Nouveaux Types d'Ennemis](#46-nouveaux-types-dennemis)
5. [Implémentation Technique](#5-implémentation-technique)
6. [Équilibrage et Métriques](#6-équilibrage-et-métriques)
7. [Priorités et Roadmap](#7-priorités-et-roadmap)
8. [Références et Sources](#8-références-et-sources)

---

## 1. Résumé Exécutif

### Contexte

Le jeu R-Type actuel dispose d'un système d'ennemis fonctionnel avec 6 types distincts (Basic, Tracker, Zigzag, Fast, Bomber, POWArmor). Cependant, une fois le joueur **full max** (Weapon LV3 + Force Pod + 2 Bits + Speed 3), le jeu devient trivial car les ennemis ne s'adaptent pas à la puissance du joueur.

### Objectif

Créer un système d'IA ennemie évolutif qui maintient le challenge tout au long de la partie, inspiré des classiques du genre (R-Type, Gradius, Ikaruga) et des techniques modernes d'ajustement dynamique de difficulté.

### Bénéfices Attendus

| Aspect | Avant | Après |
|--------|-------|-------|
| Durée de vie | ~30 min avant ennui | Sessions longues engageantes |
| Courbe de difficulté | Plate après wave 15 | Progressive constante |
| Rejouabilité | Faible | Haute (patterns variés) |
| Satisfaction joueur | Décroissante | Maintenue |

---

## 2. Analyse du Système Actuel

### 2.1 Types d'Ennemis Actuels

| Type | HP | Vitesse | Comportement | Pattern de Tir |
|------|-----|---------|--------------|----------------|
| **Basic** | 40 | -120 | Sinusoïdal vertical | Tir droit toutes les 2.5s |
| **Tracker** | 35 | -100 | Suit la position Y du joueur | Tir droit toutes les 2.0s |
| **Zigzag** | 30 | -140 | Alternance haut/bas | Tir droit toutes les 3.0s |
| **Fast** | 25 | -220 | Ligne droite rapide | Tir droit toutes les 1.5s |
| **Bomber** | 80 | -80 | Lent, tank | Tir droit toutes les 1.0s |
| **POWArmor** | 60 | -90 | Lent, récompense | Tir droit toutes les 4.0s |

### 2.2 Limitations Identifiées

1. **Patterns fixes** : Les ennemis suivent toujours le même mouvement
2. **Tir non-intelligent** : Tous tirent droit vers la gauche, jamais vers le joueur
3. **Pas de scaling** : Les stats restent identiques wave 1 vs wave 50
4. **Pas de coordination** : Les ennemis agissent indépendamment
5. **Pas de réaction** : Les ennemis ignorent les missiles du joueur

---

## 3. Problème Identifié

### Le "Power Ceiling Problem"

```
Puissance Joueur          Difficulté Perçue
     │                         │
     │    ████████████         │    ████
     │   █            █        │   █    ██
     │  █              █       │  █       ████████████
     │ █                █      │ █
     │█                  █████ │█
     └─────────────────────────└────────────────────────
       Wave 1    Wave 15   Wave 30    Wave 1   Wave 15   Wave 30

       ÉTAT ACTUEL              OBJECTIF
```

**Problème** : À partir de la wave 15-20, le joueur full max détruit tout instantanément.

**Solution** : Faire évoluer les ennemis proportionnellement à la progression du joueur.

---

## 4. Solutions Proposées

### 4.1 Système de Difficulté Progressive

#### Concept

Augmenter progressivement les statistiques et comportements des ennemis en fonction de la wave actuelle.

#### Formules de Scaling

```cpp
// Multiplicateur de difficulté basé sur la wave
float getDifficultyMultiplier(uint16_t wave) {
    // Scaling logarithmique pour éviter l'explosion
    // Wave 1: 1.0x, Wave 10: 1.3x, Wave 20: 1.5x, Wave 30: 1.65x, Wave 50: 1.85x
    return 1.0f + 0.15f * std::log2(1.0f + wave * 0.5f);
}

// Application aux stats
float getScaledHealth(uint8_t baseHP, uint16_t wave) {
    return baseHP * getDifficultyMultiplier(wave);
}

float getScaledSpeed(float baseSpeed, uint16_t wave) {
    // Vitesse scale moins vite (cap à +50%)
    float mult = 1.0f + 0.1f * std::log2(1.0f + wave * 0.3f);
    return baseSpeed * std::min(mult, 1.5f);
}

float getScaledFireRate(float baseInterval, uint16_t wave) {
    // Fire rate scale inversement (intervalle diminue)
    float mult = 1.0f - 0.05f * std::log2(1.0f + wave * 0.2f);
    return baseInterval * std::max(mult, 0.5f);  // Min 50% du cooldown original
}
```

#### Table de Scaling par Wave

| Wave | HP Mult | Speed Mult | Fire Rate Mult | Comportement |
|------|---------|------------|----------------|--------------|
| 1-5 | ×1.00 | ×1.00 | ×1.00 | Basique |
| 6-10 | ×1.15 | ×1.05 | ×0.95 | + Tir visé occasionnel |
| 11-15 | ×1.25 | ×1.10 | ×0.90 | + Esquive basique |
| 16-20 | ×1.35 | ×1.15 | ×0.85 | + Formations |
| 21-30 | ×1.45 | ×1.20 | ×0.80 | + Tir prédictif |
| 31-40 | ×1.55 | ×1.25 | ×0.75 | + Coordination |
| 41-50 | ×1.65 | ×1.30 | ×0.70 | + Tout activé |
| 51+ | ×1.75+ | ×1.35+ | ×0.65+ | Mode Hardcore |

---

### 4.2 Tir Prédictif (Lead Shots)

#### Concept

Les ennemis calculent où le joueur **sera** dans X secondes et tirent en avance pour intercepter.

#### Algorithme

```cpp
struct PredictiveShot {
    // Calcule la direction de tir pour intercepter le joueur
    static Vec2f calculateLeadDirection(
        Vec2f enemyPos,
        Vec2f playerPos,
        Vec2f playerVelocity,
        float bulletSpeed,
        float leadFactor  // 0.0 = pas de prédiction, 1.0 = prédiction parfaite
    ) {
        // Temps estimé pour que le bullet atteigne le joueur
        float distance = (playerPos - enemyPos).length();
        float timeToHit = distance / bulletSpeed;

        // Position prédite du joueur
        Vec2f predictedPos = playerPos + playerVelocity * timeToHit * leadFactor;

        // Direction vers la position prédite
        Vec2f direction = (predictedPos - enemyPos).normalized();
        return direction;
    }
};
```

#### Lead Factor par Wave et Type d'Ennemi

| Type | Wave 1-10 | Wave 11-20 | Wave 21-30 | Wave 31+ |
|------|-----------|------------|------------|----------|
| **Basic** | 0.0 | 0.2 | 0.4 | 0.6 |
| **Tracker** | 0.3 | 0.5 | 0.7 | 0.9 |
| **Zigzag** | 0.0 | 0.1 | 0.2 | 0.3 |
| **Fast** | 0.2 | 0.4 | 0.6 | 0.8 |
| **Bomber** | 0.1 | 0.3 | 0.5 | 0.7 |
| **POWArmor** | 0.0 | 0.0 | 0.1 | 0.2 |

#### Variantes de Tir

```cpp
enum class ShotPattern : uint8_t {
    Straight,       // Tir droit (actuel)
    Aimed,          // Vers position actuelle joueur
    Predictive,     // Vers position prédite
    Spread3,        // 3 tirs en éventail
    Spread5,        // 5 tirs en éventail
    Burst,          // Rafale de 3 tirs rapides
    Homing,         // Missile guidé (lent)
    Spiral          // Tir en spirale
};

// Patterns disponibles par type et wave
ShotPattern getAvailablePattern(EnemyType type, uint16_t wave) {
    if (wave < 10) return ShotPattern::Straight;

    switch (type) {
        case EnemyType::Tracker:
            if (wave >= 25) return ShotPattern::Predictive;
            if (wave >= 15) return ShotPattern::Aimed;
            return ShotPattern::Straight;

        case EnemyType::Bomber:
            if (wave >= 30) return ShotPattern::Spread5;
            if (wave >= 20) return ShotPattern::Spread3;
            return ShotPattern::Aimed;

        // ... autres types
    }
}
```

---

### 4.3 IA d'Esquive Ennemie

#### Concept

Certains ennemis détectent les missiles du joueur et tentent de les éviter.

#### Système de Détection

```cpp
struct DodgeAI {
    float detectionRadius;    // Rayon de détection des missiles
    float dodgeSpeed;         // Vitesse d'esquive
    float reactionTime;       // Délai avant esquive
    float dodgeChance;        // Probabilité d'esquive (0-1)

    // Vérifie si un missile menace cet ennemi
    bool isThreatened(const Enemy& enemy, const std::vector<Missile>& missiles) {
        for (const auto& missile : missiles) {
            // Seulement les missiles du joueur
            if (missile.owner_id == ENEMY_OWNER_ID) continue;

            // Vérifier si le missile se dirige vers nous
            Vec2f missileDir = Vec2f(missile.velocityX, missile.velocityY).normalized();
            Vec2f toEnemy = Vec2f(enemy.x - missile.x, enemy.y - missile.y);

            // Projection sur la trajectoire du missile
            float projection = toEnemy.dot(missileDir);
            if (projection < 0) continue;  // Missile s'éloigne

            // Distance perpendiculaire à la trajectoire
            Vec2f closestPoint = Vec2f(missile.x, missile.y) + missileDir * projection;
            float perpDistance = (Vec2f(enemy.x, enemy.y) - closestPoint).length();

            // Menace si le missile passera près de nous
            if (perpDistance < detectionRadius && projection < detectionRadius * 2) {
                return true;
            }
        }
        return false;
    }

    // Calcule la direction d'esquive optimale
    Vec2f calculateDodgeDirection(const Enemy& enemy, const Missile& threat) {
        // Perpendiculaire à la trajectoire du missile
        Vec2f missileDir = Vec2f(threat.velocityX, threat.velocityY).normalized();
        Vec2f perpendicular(-missileDir.y, missileDir.x);

        // Choisir la direction qui nous éloigne le plus du joueur
        // (éviter de foncer vers le joueur en esquivant)
        if (enemy.y > SCREEN_HEIGHT / 2) {
            return -perpendicular;  // Esquiver vers le haut
        }
        return perpendicular;  // Esquiver vers le bas
    }
};
```

#### Capacité d'Esquive par Type

| Type | Détection | Chance | Vitesse | Notes |
|------|-----------|--------|---------|-------|
| **Fast** | 150px | 70% | 300 | Très agile |
| **Zigzag** | 100px | 50% | 250 | Esquive naturelle |
| **Tracker** | 120px | 40% | 200 | Esquive modérée |
| **Basic** | 80px | 20% | 150 | Esquive rare |
| **Bomber** | 0px | 0% | 0 | Tank, n'esquive pas |
| **POWArmor** | 50px | 10% | 100 | Esquive minimale |

#### Activation Progressive

| Wave | Types avec Esquive |
|------|-------------------|
| 1-10 | Aucun |
| 11-15 | Fast uniquement |
| 16-20 | Fast, Zigzag |
| 21-25 | Fast, Zigzag, Tracker |
| 26-30 | Tous sauf Bomber |
| 31+ | Tous sauf Bomber (chance augmentée) |

---

### 4.4 Adaptation au Joueur

#### Concept

Le jeu analyse l'équipement et les performances du joueur pour ajuster les comportements ennemis.

#### Détection de l'État Joueur

```cpp
struct PlayerThreatLevel {
    uint8_t weaponLevel;      // 0-3
    bool hasForce;            // Force Pod?
    bool hasBits;             // Bit Devices?
    uint8_t speedLevel;       // 0-3
    float recentKillRate;     // Kills/seconde (moyenné sur 30s)
    float survivalTime;       // Temps depuis dernier dégât

    // Score de menace global (0-100)
    uint8_t calculateThreatScore() const {
        uint8_t score = 0;

        score += weaponLevel * 10;         // 0-30
        score += hasForce ? 20 : 0;        // +20
        score += hasBits ? 15 : 0;         // +15
        score += speedLevel * 5;           // 0-15
        score += std::min(recentKillRate * 10, 10.0f);  // 0-10
        score += std::min(survivalTime / 30.0f * 10, 10.0f);  // 0-10

        return std::min(score, (uint8_t)100);
    }
};
```

#### Réactions Adaptatives

| Threat Score | Réaction IA |
|--------------|-------------|
| **0-20** (Débutant) | Comportement standard, tirs lents |
| **21-40** (Intermédiaire) | Tirs visés, patterns basiques |
| **41-60** (Avancé) | Esquive activée, formations |
| **61-80** (Expert) | Tir prédictif, coordination |
| **81-100** (Maître) | Tout activé, spawn bonus |

#### Contre-Mesures Spécifiques

```cpp
void applyCounterMeasures(Enemy& enemy, const PlayerThreatLevel& threat) {
    // Contre Force Pod: éviter le centre (zone de contact)
    if (threat.hasForce) {
        // Les ennemis préfèrent les bords de l'écran
        enemy.preferredY = (enemy.y < SCREEN_HEIGHT / 2)
                          ? SCREEN_HEIGHT * 0.2f
                          : SCREEN_HEIGHT * 0.8f;
    }

    // Contre Bits: tir en spread (contrer l'orbite)
    if (threat.hasBits) {
        enemy.preferredPattern = ShotPattern::Spread3;
    }

    // Contre Weapon LV3: dispersion (éviter AoE Spread)
    if (threat.weaponLevel >= 3) {
        enemy.formationSpacing *= 1.5f;
    }

    // Contre Speed 3: plus de missiles homing
    if (threat.speedLevel >= 3) {
        enemy.homingChance += 0.2f;
    }
}
```

---

### 4.5 Formations Coordonnées

#### Concept

Les ennemis apparaissent et agissent en formations coordonnées plutôt qu'individuellement.

#### Types de Formations

```
┌─────────────────────────────────────────────────────────────┐
│                     FORMATIONS ENNEMIES                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  V-SHAPE (5 ennemis)         WALL (6 ennemis)               │
│                                                              │
│        ●                     ●  ●  ●  ●  ●  ●               │
│       ● ●                    ────────────────►              │
│      ●   ●                                                   │
│     ────────►                                                │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  PINCER (4+4 ennemis)        HELIX (8 ennemis)              │
│                                                              │
│  ●●●●─────┐                      ●                          │
│           │                     ● ●                         │
│  JOUEUR ◄─┼─────────            ●   ●                       │
│           │                    ●     ●                      │
│  ●●●●─────┘                   ●       ●                     │
│                              ──────────►                    │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  DIAMOND (9 ennemis)         SNAKE (6 ennemis)              │
│                                                              │
│       ●                      ●─●─●─●─●─●                    │
│      ● ●                      \_/\_/\_/                     │
│     ● ● ●                    Mouvement sinusoïdal           │
│      ● ●                     coordonné                      │
│       ●                                                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### Structure de Données

```cpp
struct Formation {
    enum class Type : uint8_t {
        VShape,
        Wall,
        Pincer,
        Helix,
        Diamond,
        Snake,
        Circle,
        Cross
    };

    Type type;
    std::vector<Vec2f> relativePositions;  // Positions relatives au leader
    EnemyType enemyType;

    // Comportement coordonné
    bool synchronizedFiring = false;    // Tous tirent en même temps
    float formationSpeed = 1.0f;        // Multiplicateur de vitesse
    bool maintainFormation = true;      // Garde la formation ou se disperse

    // Patterns de mouvement
    enum class MovementPattern : uint8_t {
        Linear,          // Ligne droite
        Sinusoidal,      // Vague synchronisée
        Expanding,       // S'écarte progressivement
        Contracting,     // Se rapproche du centre
        Rotating         // Rotation autour du leader
    };
    MovementPattern movement = MovementPattern::Linear;
};

// Exemple: Formation V-Shape
Formation createVShape() {
    Formation f;
    f.type = Formation::Type::VShape;
    f.relativePositions = {
        {0, 0},         // Leader
        {-40, -30},     // Gauche haut
        {-40, 30},      // Gauche bas
        {-80, -60},     // Loin gauche haut
        {-80, 60}       // Loin gauche bas
    };
    f.synchronizedFiring = true;
    f.enemyType = EnemyType::Fast;
    return f;
}
```

#### Disponibilité par Wave

| Wave | Formations Disponibles |
|------|------------------------|
| 1-10 | Aucune (ennemis solo) |
| 11-15 | VShape, Wall |
| 16-20 | + Snake, Diamond |
| 21-25 | + Pincer |
| 26-30 | + Helix, Circle |
| 31+ | Toutes, formations mixtes |

---

### 4.6 Nouveaux Types d'Ennemis

#### 4.6.1 Sniper (Wave 15+)

```cpp
struct EnemySniper {
    static constexpr uint8_t HEALTH = 30;
    static constexpr float SPEED_X = -60.0f;  // Très lent
    static constexpr float CHARGE_TIME = 2.0f;
    static constexpr uint8_t DAMAGE = 35;     // Dégâts élevés

    // Comportement
    // 1. Se positionne
    // 2. Charge (indicateur visuel - laser de visée)
    // 3. Tire un projectile rapide et précis
    // 4. Cooldown puis répète

    bool isCharging = false;
    float chargeProgress = 0.0f;
    Vec2f aimDirection;

    void update(float dt, const Vec2f& playerPos) {
        if (isCharging) {
            chargeProgress += dt;
            // Met à jour la visée pendant la charge
            aimDirection = (playerPos - position).normalized();

            if (chargeProgress >= CHARGE_TIME) {
                fireSnipeShot();
                isCharging = false;
                chargeProgress = 0.0f;
            }
        }
    }
};
```

**Indicateurs Visuels**:
- Laser rouge de visée pendant la charge
- Clignotement avant le tir
- Son de charge distinctif

#### 4.6.2 Swarm (Wave 20+)

```cpp
struct EnemySwarm {
    // Mini-ennemis en essaim (5 unités)
    static constexpr uint8_t UNIT_HEALTH = 10;
    static constexpr uint8_t UNIT_COUNT = 5;
    static constexpr float SWARM_RADIUS = 40.0f;
    static constexpr float ORBIT_SPEED = 4.0f;

    // Comportement de groupe
    // - Orbite autour d'un centre mobile
    // - Si une unité meurt, les autres accélèrent
    // - Dégâts de contact uniquement (pas de tir)

    Vec2f centerPos;
    std::array<SwarmUnit, 5> units;

    void updateSwarm(float dt) {
        for (size_t i = 0; i < units.size(); ++i) {
            if (!units[i].alive) continue;

            float angle = units[i].orbitAngle + ORBIT_SPEED * dt;
            units[i].orbitAngle = angle;
            units[i].x = centerPos.x + std::cos(angle) * SWARM_RADIUS;
            units[i].y = centerPos.y + std::sin(angle) * SWARM_RADIUS;
        }

        // Vitesse augmente avec les pertes
        int alive = countAlive();
        centerVelocity *= 1.0f + (UNIT_COUNT - alive) * 0.1f;
    }
};
```

#### 4.6.3 Mirror (Wave 25+)

```cpp
struct EnemyMirror {
    static constexpr uint8_t HEALTH = 45;
    static constexpr float MIRROR_DELAY = 0.3f;  // Délai avant de copier

    // Copie les mouvements du joueur avec un miroir vertical
    // Si joueur monte → Mirror monte
    // Si joueur va à droite → Mirror va à gauche

    std::deque<Vec2f> playerMovementHistory;

    void recordPlayerMovement(const Vec2f& playerVel) {
        playerMovementHistory.push_back(playerVel);
        if (playerMovementHistory.size() > 30) {  // ~0.5s d'historique
            playerMovementHistory.pop_front();
        }
    }

    Vec2f getMirroredVelocity() {
        if (playerMovementHistory.size() < 6) return {0, 0};

        // Utilise le mouvement d'il y a ~0.1s
        Vec2f delayed = playerMovementHistory[playerMovementHistory.size() - 6];
        return Vec2f(-delayed.x, delayed.y);  // Miroir horizontal
    }
};
```

#### 4.6.4 Shield (Wave 30+)

```cpp
struct EnemyShield {
    static constexpr uint8_t HEALTH = 100;
    static constexpr float SHIELD_RADIUS = 80.0f;
    static constexpr float SHIELD_ARC = 120.0f;  // Degrés (devant)

    // Génère un bouclier qui protège les ennemis derrière lui
    // Le bouclier absorbe les missiles venant de devant
    // Vulnérable par les côtés et l'arrière

    float shieldHealth = 200.0f;
    float shieldAngle = 180.0f;  // Face au joueur

    bool isProjectileBlocked(const Vec2f& projectilePos, const Vec2f& projectileVel) {
        Vec2f toProjectile = projectilePos - position;
        float angle = std::atan2(toProjectile.y, toProjectile.x) * 180.0f / M_PI;

        // Vérifier si dans l'arc du bouclier
        float angleDiff = std::abs(angle - shieldAngle);
        if (angleDiff > 180.0f) angleDiff = 360.0f - angleDiff;

        return angleDiff < SHIELD_ARC / 2.0f && toProjectile.length() < SHIELD_RADIUS;
    }
};
```

#### 4.6.5 Splitter (Wave 35+)

```cpp
struct EnemySplitter {
    static constexpr uint8_t HEALTH_LARGE = 60;
    static constexpr uint8_t HEALTH_MEDIUM = 30;
    static constexpr uint8_t HEALTH_SMALL = 15;

    enum class Size { Large, Medium, Small };
    Size currentSize = Size::Large;

    // Quand détruit, se divise en 2 ennemis plus petits
    // Large → 2 Medium → 4 Small → mort définitive

    std::vector<EnemySplitter> split() {
        std::vector<EnemySplitter> children;

        switch (currentSize) {
            case Size::Large:
                children.push_back(createChild(Size::Medium, {-20, -20}));
                children.push_back(createChild(Size::Medium, {-20, 20}));
                break;
            case Size::Medium:
                children.push_back(createChild(Size::Small, {-15, -15}));
                children.push_back(createChild(Size::Small, {-15, 15}));
                break;
            case Size::Small:
                // Meurt définitivement
                break;
        }

        return children;
    }
};
```

#### Tableau Récapitulatif des Nouveaux Ennemis

| Type | Wave | HP | Vitesse | Spécialité | Menace |
|------|------|-----|---------|------------|--------|
| **Sniper** | 15+ | 30 | -60 | Tir chargé précis | ⭐⭐⭐ |
| **Swarm** | 20+ | 10×5 | -150 | Essaim contact | ⭐⭐ |
| **Mirror** | 25+ | 45 | Variable | Copie mouvements | ⭐⭐⭐⭐ |
| **Shield** | 30+ | 100 | -80 | Protège alliés | ⭐⭐⭐ |
| **Splitter** | 35+ | 60→30→15 | -100 | Se divise | ⭐⭐⭐⭐ |

---

## 5. Implémentation Technique

### 5.1 Architecture Proposée

```cpp
// Nouvelle structure Enemy étendue
struct Enemy {
    // === Existant ===
    uint16_t id;
    float x, y;
    uint8_t health;
    uint8_t enemy_type;
    // ... autres champs existants

    // === Nouveau: Système d'IA ===
    struct AIState {
        // Comportement de tir
        ShotPattern currentPattern = ShotPattern::Straight;
        float leadFactor = 0.0f;

        // Comportement d'esquive
        bool canDodge = false;
        float dodgeChance = 0.0f;
        float dodgeCooldown = 0.0f;
        bool isDodging = false;
        Vec2f dodgeTarget;

        // Formation
        uint16_t formationId = 0;       // 0 = pas de formation
        uint8_t formationIndex = 0;     // Position dans la formation

        // Adaptation
        uint8_t threatResponse = 0;     // Bitfield des contre-mesures actives
    } ai;

    // === Nouveau: Statistiques scalées ===
    struct ScaledStats {
        float healthMult = 1.0f;
        float speedMult = 1.0f;
        float fireRateMult = 1.0f;
        float damageMult = 1.0f;
    } scaled;
};
```

### 5.2 Système de Difficulté

```cpp
class DifficultyManager {
public:
    void setWave(uint16_t wave) {
        _currentWave = wave;
        _difficultyMult = calculateDifficultyMultiplier(wave);
        updateActiveFeatures();
    }

    void setPlayerThreat(const PlayerThreatLevel& threat) {
        _playerThreat = threat;
        updateCounterMeasures();
    }

    // Applique le scaling à un ennemi nouvellement spawné
    void applyDifficultyToEnemy(Enemy& enemy) {
        // Scaling des stats
        enemy.scaled.healthMult = _difficultyMult;
        enemy.scaled.speedMult = 1.0f + (_difficultyMult - 1.0f) * 0.5f;
        enemy.scaled.fireRateMult = 2.0f - _difficultyMult;  // Inverse

        // Activation des features
        if (_features.predictiveAiming) {
            enemy.ai.leadFactor = getLeadFactorForType(enemy.enemy_type);
        }
        if (_features.dodging) {
            enemy.ai.canDodge = canTypeDodge(enemy.enemy_type);
            enemy.ai.dodgeChance = getDodgeChanceForType(enemy.enemy_type);
        }

        // Contre-mesures
        applyCounterMeasures(enemy, _playerThreat);
    }

private:
    uint16_t _currentWave = 1;
    float _difficultyMult = 1.0f;
    PlayerThreatLevel _playerThreat;

    struct ActiveFeatures {
        bool predictiveAiming = false;
        bool dodging = false;
        bool formations = false;
        bool coordination = false;
        bool newEnemyTypes = false;
    } _features;

    void updateActiveFeatures() {
        _features.predictiveAiming = _currentWave >= 10;
        _features.dodging = _currentWave >= 15;
        _features.formations = _currentWave >= 15;
        _features.coordination = _currentWave >= 25;
        _features.newEnemyTypes = _currentWave >= 15;
    }
};
```

### 5.3 Formation Manager

```cpp
class FormationManager {
public:
    uint16_t createFormation(Formation::Type type, EnemyType enemyType, Vec2f spawnPos) {
        uint16_t formationId = _nextFormationId++;

        Formation formation = buildFormation(type, enemyType);
        formation.leaderPosition = spawnPos;

        _activeFormations[formationId] = formation;
        return formationId;
    }

    void updateFormations(float dt, std::unordered_map<uint16_t, Enemy>& enemies) {
        for (auto& [formationId, formation] : _activeFormations) {
            updateFormationMovement(formation, dt);

            // Synchroniser les positions des membres
            for (size_t i = 0; i < formation.memberIds.size(); ++i) {
                auto enemyIt = enemies.find(formation.memberIds[i]);
                if (enemyIt == enemies.end()) continue;

                Vec2f targetPos = formation.leaderPosition + formation.relativePositions[i];
                enemyIt->second.x = targetPos.x;
                enemyIt->second.y = targetPos.y;
            }

            // Tir synchronisé
            if (formation.synchronizedFiring && formation.fireTimer <= 0) {
                for (uint16_t memberId : formation.memberIds) {
                    queueEnemyShot(memberId);
                }
                formation.fireTimer = formation.fireInterval;
            }
            formation.fireTimer -= dt;
        }
    }

private:
    std::unordered_map<uint16_t, Formation> _activeFormations;
    uint16_t _nextFormationId = 1;
};
```

---

## 6. Équilibrage et Métriques

### 6.1 Métriques Clés à Surveiller

| Métrique | Cible | Méthode de Mesure |
|----------|-------|-------------------|
| **Temps moyen de survie** | 5-15 min | Moyenne sur sessions |
| **Kills/minute** | 8-15 (variable par wave) | Compteur |
| **Dégâts reçus/wave** | 10-30 HP | Moyenne |
| **Taux de mort** | 1 mort / 8-12 waves | Stats session |
| **Score médian Wave 30** | ~50,000 | Tracking |
| **Taux d'abandon** | < 20% | Analytics |

### 6.2 Courbe de Difficulté Cible

```
Difficulté
Perçue
   │
 10├                                    ████
   │                               █████
  8├                          █████
   │                     █████
  6├                █████           ← Zone de Flow
   │           █████
  4├      █████
   │  ████
  2├██
   │
   └──────────────────────────────────────────
     1    5    10   15   20   25   30   35   Wave

     │ Introduction │ Montée │ Challenge │ Hardcore │
```

### 6.3 Tests d'Équilibrage

```cpp
// Système de logging pour équilibrage
struct BalanceMetrics {
    // Par wave
    std::map<uint16_t, WaveMetrics> waveData;

    struct WaveMetrics {
        uint32_t enemiesSpawned = 0;
        uint32_t enemiesKilled = 0;
        uint32_t playerDamageReceived = 0;
        uint32_t playerDamageDealt = 0;
        float averageTimeToKill = 0.0f;
        float playerDeathCount = 0;

        // Par type d'ennemi
        std::map<EnemyType, EnemyTypeMetrics> byType;
    };

    struct EnemyTypeMetrics {
        uint32_t spawned = 0;
        uint32_t killed = 0;
        uint32_t killedPlayer = 0;  // Combien de fois a tué un joueur
        float avgLifetime = 0.0f;
    };

    void exportToCSV(const std::string& filename);
};
```

---

## 7. Priorités et Roadmap

### Phase 1 - Fondations (Priorité: HAUTE)

| Tâche | Complexité | Impact | Durée Est. |
|-------|------------|--------|------------|
| Scaling HP/Speed/FireRate par wave | Faible | Élevé | - |
| Tir visé basique (vers joueur) | Faible | Moyen | - |
| Logging métriques d'équilibrage | Faible | Élevé | - |

### Phase 2 - Comportements Avancés (Priorité: MOYENNE)

| Tâche | Complexité | Impact | Durée Est. |
|-------|------------|--------|------------|
| Tir prédictif (lead shots) | Moyenne | Élevé | - |
| Système d'esquive | Moyenne | Élevé | - |
| Patterns de tir variés | Moyenne | Moyen | - |

### Phase 3 - Intelligence (Priorité: MOYENNE)

| Tâche | Complexité | Impact | Durée Est. |
|-------|------------|--------|------------|
| Adaptation au joueur (threat level) | Moyenne | Élevé | - |
| Contre-mesures équipement | Moyenne | Moyen | - |
| Formations de base (V, Wall) | Moyenne | Élevé | - |

### Phase 4 - Contenu (Priorité: BASSE)

| Tâche | Complexité | Impact | Durée Est. |
|-------|------------|--------|------------|
| Nouveaux ennemis (Sniper, Swarm) | Haute | Moyen | - |
| Formations avancées (Helix, Pincer) | Haute | Moyen | - |
| Ennemis complexes (Mirror, Shield) | Haute | Élevé | - |

### Dépendances

```
┌─────────────────────────────────────────────────────────────┐
│                    GRAPHE DE DÉPENDANCES                     │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  [Scaling Stats] ──────┬──────────────────────────────────┐ │
│         │              │                                  │ │
│         ▼              ▼                                  │ │
│  [Tir Visé] ────► [Tir Prédictif] ────► [Adaptation]     │ │
│         │              │                      │          │ │
│         │              ▼                      ▼          │ │
│         └────────► [Esquive] ◄───────── [Contre-mesures] │ │
│                        │                                  │ │
│                        ▼                                  │ │
│                  [Formations]                            │ │
│                        │                                  │ │
│                        ▼                                  │ │
│               [Nouveaux Ennemis]                         │ │
│                                                          │ │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. Références et Sources

### Articles et Ressources Game Design

- [Balancing the sh#& out of our shmup](https://www.gamedeveloper.com/design/balancing-the-sh-out-of-our-shmup) - Game Developer
- [Combining enemy types and enemy behaviors](https://www.gamedeveloper.com/programming/combining-enemy-types-and-enemy-behaviors) - Game Developer
- [Enemy design | The Level Design Book](https://book.leveldesignbook.com/process/combat/enemy)
- [How Tough Is Your Game? Creating Difficulty Graphs](https://www.gamedeveloper.com/design/how-tough-is-your-game-creating-difficulty-graphs) - Game Developer
- [Unleashing Chaos: Mastering Enemy Waves](https://medium.com/@victormct/unleashing-chaos-mastering-enemy-waves-9be16f92e673) - Medium

### Shmup Design Specifique

- [Enemy movement patterns, scripting, etc..](https://shmups.system11.org/viewtopic.php?t=4551) - Shmups Forum
- [Theory Behind Common Bullet Hell Patterns](https://www.gamedev.net/forums/topic/605497-theory-behind-common-bullet-hell-patterns/) - GameDev.net
- [Boghog's bullet hell shmup 101](https://shmups.wiki/library/Boghog's_bullet_hell_shmup_101) - Shmups Wiki
- [Sparen's Danmaku Design Studio - Guide A2](https://sparen.github.io/ph3tutorials/ddsga2.html)
- [Giest118's Guide to Making Good Bullet Hell Bosses](https://shmups.system11.org/viewtopic.php?t=44816) - Shmups Forum

### Difficulté Adaptative

- [Dynamic game difficulty balancing](https://en.wikipedia.org/wiki/Dynamic_game_difficulty_balancing) - Wikipedia
- [Exploring Dynamic Difficulty Adjustment Methods for Video Games](https://www.mdpi.com/2813-2084/3/2/12) - MDPI
- [AI for dynamic difficulty adjustment in games](https://www.researchgate.net/publication/228889029_AI_for_dynamic_difficulty_adjustment_in_games) - ResearchGate
- [Rubber-Band A.I.](https://tvtropes.org/pmwiki/pmwiki.php/Main/RubberBandAI) - TV Tropes
- [Explaining "Rubber-Banding AI" in Game Design](https://game-wisdom.com/critical/rubber-banding-ai-game-design) - Game Wisdom

### Jeux de Référence

- [Gradius/Enemies](https://strategywiki.org/wiki/Gradius/Enemies) - StrategyWiki
- [Ikaruga](https://shmups.wiki/library/Ikaruga) - Shmups Wiki
- [Radiant Silvergun](https://en.wikipedia.org/wiki/Radiant_Silvergun) - Wikipedia
- [Building the brutal bullet hell systems of Luna Abyss](https://www.gamedeveloper.com/design/building-the-bullet-hell-systems-of-luna-abyss) - Game Developer

### AI et Comportements

- [Brainstorming for shmup AI](https://gamedev.net/forums/topic/579281-brainstorming-for-shmup-ai/4690401/) - GameDev.net
- [How enemy AI decide to dodge or block](https://www.gamedev.net/forums/topic/708540-how-enemy-ai-decide-to-dodge-or-block-from-players-melee-attack/) - GameDev.net
- [Scaling Enemy Difficulty per wave](https://www.construct.net/en/forum/construct-2/how-do-i-18/scaling-enemy-difficulty-per-67572) - Construct Forum

---

## Annexes

### A. Constantes Suggérées

```cpp
// DifficultyConstants.hpp

namespace DifficultyConstants {
    // Scaling par wave
    constexpr float HEALTH_SCALE_BASE = 0.15f;
    constexpr float SPEED_SCALE_BASE = 0.10f;
    constexpr float FIRE_RATE_SCALE_BASE = 0.05f;
    constexpr float SPEED_SCALE_CAP = 1.5f;
    constexpr float FIRE_RATE_MIN = 0.5f;

    // Activation des features
    constexpr uint16_t WAVE_AIMED_SHOTS = 10;
    constexpr uint16_t WAVE_PREDICTIVE = 20;
    constexpr uint16_t WAVE_DODGE = 15;
    constexpr uint16_t WAVE_FORMATIONS = 15;
    constexpr uint16_t WAVE_NEW_ENEMIES = 15;
    constexpr uint16_t WAVE_COORDINATION = 25;

    // Tir prédictif
    constexpr float LEAD_FACTOR_BASE = 0.2f;
    constexpr float LEAD_FACTOR_PER_WAVE = 0.02f;
    constexpr float LEAD_FACTOR_MAX = 0.9f;

    // Esquive
    constexpr float DODGE_DETECTION_BASE = 80.0f;
    constexpr float DODGE_CHANCE_BASE = 0.2f;
    constexpr float DODGE_COOLDOWN = 1.5f;
}
```

### B. Enums et Types

```cpp
// EnemyAI.hpp

enum class ShotPattern : uint8_t {
    Straight = 0,
    Aimed = 1,
    Predictive = 2,
    Spread3 = 3,
    Spread5 = 4,
    Burst = 5,
    Homing = 6,
    Spiral = 7,
    COUNT
};

enum class FormationType : uint8_t {
    None = 0,
    VShape = 1,
    Wall = 2,
    Pincer = 3,
    Helix = 4,
    Diamond = 5,
    Snake = 6,
    Circle = 7,
    Cross = 8,
    COUNT
};

enum class EnemyBehavior : uint8_t {
    Standard = 0,
    Aggressive = 1,
    Defensive = 2,
    Evasive = 3,
    Support = 4,
    COUNT
};

// Bitfield pour les features actives
namespace AIFeatures {
    constexpr uint8_t NONE = 0x00;
    constexpr uint8_t AIMED_SHOTS = 0x01;
    constexpr uint8_t PREDICTIVE = 0x02;
    constexpr uint8_t DODGE = 0x04;
    constexpr uint8_t FORMATION = 0x08;
    constexpr uint8_t COORDINATION = 0x10;
    constexpr uint8_t COUNTER_FORCE = 0x20;
    constexpr uint8_t COUNTER_BITS = 0x40;
    constexpr uint8_t COUNTER_WEAPON = 0x80;
}
```

---

*Document généré pour le projet R-Type - Janvier 2026*
