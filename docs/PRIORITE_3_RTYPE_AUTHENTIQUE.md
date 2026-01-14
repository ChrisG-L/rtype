# PRIORITE 3 - Mécaniques R-Type Authentiques

## Contexte

Ce document fait suite à **PRIORITE_2_GAMEPLAY.md** (Boss, Score, Armes), dont les fonctionnalités ont été implémentées dans la PR #7. Cette phase vise à ajouter les **mécaniques signature** du R-Type original pour une expérience plus fidèle au jeu arcade de 1987.

### Déjà Réalisé (PR #6 + PR #7)

| PR | Contenu | Statut |
|----|---------|--------|
| **PR #6** | TLS 1.2+, CSPRNG, Documentation RNCP | ✅ Merged |
| **PR #7** | Boss 3 phases, Score/Combo, 4 Armes, HUD | ⏳ En review |

### État Actuel vs R-Type Original

| Mécanique | R-Type 1987 | Notre Version | Écart |
|-----------|-------------|---------------|-------|
| **Force Pod** | Orbe détachable, bouclier + arme | ❌ Absent | 🔴 CRITIQUE |
| **Charge Shot (Wave Cannon)** | Maintenir tir → beam puissant | ❌ Absent | 🔴 CRITIQUE |
| **Power-up Drops** | POW Armor → items | ❌ Absent | 🔴 MAJEUR |
| **Speed-Up** | Item "S" | ❌ Absent | 🟠 MOYEN |
| **Bits (Options)** | 2 orbes défensifs | ❌ Absent | 🟡 MINEUR |
| **3 Lasers colorés** | Rouge/Bleu/Jaune | ✅ 4 armes (adapté) | ✅ OK |
| **Boss mémorables** | Dobkeratops, Gomander | ✅ Nemesis 3 phases | ✅ OK |
| **Missiles homing** | Item "M" | ✅ Arme Missile | ✅ OK |

---

## Résumé Exécutif

| Priorité | Feature | Effort | Fidélité R-Type | Impact Demo |
|----------|---------|--------|-----------------|-------------|
| 🔴 **P0** | Charge Shot (Wave Cannon) | 3-4h | ⭐⭐⭐⭐⭐ Signature | Très élevé |
| 🔴 **P0** | Power-up Drops | 2-3h | ⭐⭐⭐⭐⭐ Core loop | Élevé |
| 🟠 **P1** | Force Pod (simplifié) | 5-6h | ⭐⭐⭐⭐⭐ Iconique | Très élevé |
| 🟠 **P1** | Speed-Up Item | 1h | ⭐⭐⭐⭐ Important | Moyen |
| 🟡 **P2** | Bits (Options) | 3-4h | ⭐⭐⭐ Nice-to-have | Moyen |
| 🟡 **P2** | Effets Visuels (polish) | 2-3h | ⭐⭐ Polish | Élevé |
| ⚪ **P3** | Sons/Musique | 1-2h | ⭐⭐ Polish | Moyen |

**Objectif** : Transformer notre R-Type en une expérience authentique fidèle à l'arcade original.

---

# PARTIE 1 : ANALYSE DU R-TYPE ORIGINAL

## 1.1 Mécaniques Clés du R-Type (1987)

Sources : [R-Type Wikipedia](https://en.wikipedia.org/wiki/R-Type), [StrategyWiki R-Type](https://strategywiki.org/wiki/R-Type/Gameplay), [Shmups Wiki](https://shmups.wiki/library/R-Type)

### Le Vaisseau R-9 "Arrowhead"

Le joueur contrôle le **R-9A Arrowhead**, un chasseur spatial équipé de :

1. **Tir standard** : Projectiles rapides, faibles dégâts
2. **Wave Cannon** : Tir chargé dévastateur (signature du jeu)
3. **Force Pod** : Module détachable offrant protection et puissance de feu
4. **Missiles** : Projectiles homing (via power-up)
5. **Bits** : Orbes défensifs au-dessus/dessous du vaisseau

### Le Wave Cannon (Charge Shot)

```
┌────────────────────────────────────────────────────────────────┐
│                     WAVE CANNON MECHANICS                       │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  MAINTENIR TIR:    [░░░░░░░░░░░░░░░░░░░░] BEAM GAUGE           │
│                     ▲                                           │
│                     │ Charge progressive                        │
│                     │ (ne peut pas tirer normal pendant)        │
│                                                                 │
│  NIVEAU 1 (0.5s):  ●───────►  Petit beam, 1 ennemi             │
│  NIVEAU 2 (1.0s):  ●●●═════►  Beam moyen, traverse faibles     │
│  NIVEAU 3 (1.5s):  ●●●●●════════►  MEGA BEAM, traverse tout    │
│                                                                 │
│  RELÂCHER TIR:     Tire le beam accumulé                        │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

**Caractéristiques** :
- Maintenir le bouton de tir charge la jauge "BEAM"
- **Impossible de tirer normal** pendant la charge
- 3 niveaux de puissance selon le temps de charge
- Le beam traverse les ennemis faibles (piercing)
- Stratégique : quand charger vs tirer rapidement ?

### Le Force Pod

```
┌────────────────────────────────────────────────────────────────┐
│                      FORCE POD MECHANICS                        │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  MODE ATTACHÉ AVANT:        MODE ATTACHÉ ARRIÈRE:               │
│                                                                 │
│      [FORCE]◄══►[SHIP]          [SHIP]◄══►[FORCE]              │
│         │                                    │                  │
│         ▼                                    ▼                  │
│    - Bouclier frontal              - Bouclier arrière          │
│    - Tir laser amplifié            - Tir laser vers l'arrière  │
│    - Bloque projectiles            - Bloque projectiles        │
│                                                                 │
│  MODE DÉTACHÉ:                                                  │
│                                                                 │
│      [SHIP]          [FORCE]                                    │
│                         │                                       │
│                         ▼                                       │
│                  - Flotte librement                             │
│                  - Tire indépendamment                          │
│                  - Peut être rappelé                            │
│                                                                 │
│  NIVEAUX DU FORCE:                                              │
│    Lvl 1: 1 tir forward                                         │
│    Lvl 2: 2 tirs en V + laser faible                           │
│    Lvl 3: 4 tirs + laser puissant                              │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

**Caractéristiques** :
- **Invincible** : le Force bloque TOUS les projectiles
- **Attachable** : avant ou arrière du vaisseau
- **Détachable** : vole indépendamment, tire seul
- **Upgradable** : 3 niveaux via cristaux laser
- **Damage on contact** : détruit les ennemis au toucher

### Système de Power-Ups

Les power-ups apparaissent quand on détruit un **POW Armor** (ennemi spécial) :

| Item | Symbole | Effet | Persistance |
|------|---------|-------|-------------|
| **Force Pod** | Orbe | Crée ou upgrade le Force | Permanent |
| **Laser Crystal (Rouge)** | 🔴 | Counter-Air Laser (DNA helix) | Permanent |
| **Laser Crystal (Bleu)** | 🔵 | Rebound Laser (rebondit) | Permanent |
| **Laser Crystal (Jaune)** | 🟡 | Counter-Ground Laser (longe murs) | Permanent |
| **Speed-Up** | S | +1 niveau de vitesse | Permanent |
| **Missile** | M | Active missiles homing | Permanent |
| **Bit** | Orbe gris | Ajoute un Bit (max 2) | Permanent |

### Les Bits (Options)

```
                    [BIT]
                      │
                      │
    [SHIP] ◄═════════►
                      │
                      │
                    [BIT]
```

- 2 Bits maximum (haut et bas du vaisseau)
- **Invincibles** : bloquent les projectiles
- Tirent avec le laser rouge uniquement
- Se déplacent avec un léger délai (trailing)

---

## 1.2 Comparaison avec Notre Implémentation

### Ce qu'on a bien fait ✅

| Aspect | R-Type Original | Notre Version |
|--------|-----------------|---------------|
| Armes variées | 3 lasers colorés | 4 armes (Standard, Spread, Laser, Missile) |
| Boss épique | Dobkeratops multi-phases | Nemesis 3 phases, 8 attaques |
| Score système | Points par ennemi | Score + Combo multiplier |
| Difficulté progressive | 8 stages | Waves infinies + boss wave 10 |
| Multiplayer | ❌ Solo uniquement | ✅ 4 joueurs coop |

### Ce qui manque crucialement ❌

| Aspect | Importance | Raison |
|--------|------------|--------|
| **Wave Cannon** | 🔴 CRITIQUE | Mécanique SIGNATURE du jeu |
| **Force Pod** | 🔴 CRITIQUE | Innovation qui a défini le genre |
| **Power-up drops** | 🔴 MAJEUR | Core gameplay loop (risk/reward) |
| **Speed-Up** | 🟠 MOYEN | Progression du joueur |
| **Bits** | 🟡 MINEUR | Polish, pas essentiel |

---

# PARTIE 2 : SPÉCIFICATIONS TECHNIQUES

## 2.1 Wave Cannon (Charge Shot) - 3-4h

### Design

| Niveau | Temps Charge | Dégâts | Taille | Comportement |
|--------|--------------|--------|--------|--------------|
| **Niveau 0** | 0s | 15 | Normal | Tir standard (pas de charge) |
| **Niveau 1** | 0.5s | 40 | Petit | Petit beam, 1 hit |
| **Niveau 2** | 1.0s | 80 | Moyen | Beam moyen, traverse faibles |
| **Niveau 3** | 1.5s | 150 | Grand | MEGA BEAM, traverse tout |

### Contraintes Gameplay

- **Impossible de tirer normal** pendant la charge
- Le joueur doit **choisir** : tir rapide ou charge puissante
- Jauge visuelle "BEAM" dans le HUD
- Feedback audio : son de charge croissant

### Modifications Protocol (Protocol.hpp)

```cpp
// Nouveau champ dans PlayerInput
struct PlayerInput {
    uint8_t keys;              // Existant
    uint32_t sequence;         // Existant
    uint8_t charge_level;      // NOUVEAU: 0-3
    static constexpr size_t WIRE_SIZE = 6;  // était 5
};

// Nouveau message
enum class MessageType : uint16_t {
    // ... existants ...
    ChargeStart = 0x00D5,      // C→S : Début charge
    ChargeRelease = 0x00D6,    // C→S : Relâche charge (fire beam)
};

// Structure ChargeRelease
struct ChargeRelease {
    uint8_t charge_level;      // 1-3
    static constexpr size_t WIRE_SIZE = 1;
};

// Extension PlayerState pour affichage HUD autres joueurs
struct PlayerState {
    // ... existants ...
    uint8_t charge_level;      // NOUVEAU: 0-3 (pour afficher jauge)
    static constexpr size_t WIRE_SIZE = 19;  // était 18
};
```

### Modifications Serveur (GameWorld)

```cpp
// GameWorld.hpp
struct ConnectedPlayer {
    // ... existants ...
    float chargeTimer = 0.0f;      // Temps de charge accumulé
    uint8_t chargeLevel = 0;       // 0-3
    bool isCharging = false;       // En train de charger ?
};

// Constantes Wave Cannon
namespace WaveCannon {
    constexpr float CHARGE_TIME_LV1 = 0.5f;
    constexpr float CHARGE_TIME_LV2 = 1.0f;
    constexpr float CHARGE_TIME_LV3 = 1.5f;

    constexpr uint8_t DAMAGE_LV1 = 40;
    constexpr uint8_t DAMAGE_LV2 = 80;
    constexpr uint8_t DAMAGE_LV3 = 150;

    constexpr float SPEED = 900.0f;
    constexpr float WIDTH_LV1 = 20.0f;
    constexpr float WIDTH_LV2 = 35.0f;
    constexpr float WIDTH_LV3 = 50.0f;
}

// GameWorld.cpp
void GameWorld::updatePlayerCharge(uint8_t playerId, float deltaTime, bool fireHeld) {
    auto& player = _connectedPlayers[playerId];

    if (fireHeld && !player.isCharging) {
        // Début de charge
        player.isCharging = true;
        player.chargeTimer = 0.0f;
    }

    if (player.isCharging && fireHeld) {
        player.chargeTimer += deltaTime;

        // Calculer niveau de charge
        if (player.chargeTimer >= WaveCannon::CHARGE_TIME_LV3) {
            player.chargeLevel = 3;
        } else if (player.chargeTimer >= WaveCannon::CHARGE_TIME_LV2) {
            player.chargeLevel = 2;
        } else if (player.chargeTimer >= WaveCannon::CHARGE_TIME_LV1) {
            player.chargeLevel = 1;
        } else {
            player.chargeLevel = 0;
        }
    }
}

void GameWorld::releaseCharge(uint8_t playerId) {
    auto& player = _connectedPlayers[playerId];

    if (player.chargeLevel > 0) {
        // Tirer le Wave Cannon
        spawnWaveCannon(playerId, player.chargeLevel);
    } else {
        // Tir normal si pas assez chargé
        spawnMissileWithWeapon(playerId, player.currentWeapon);
    }

    player.isCharging = false;
    player.chargeTimer = 0.0f;
    player.chargeLevel = 0;
}

void GameWorld::spawnWaveCannon(uint8_t playerId, uint8_t level) {
    auto it = _players.find(playerId);
    if (it == _players.end()) return;

    float damage, width, speed;
    switch (level) {
        case 1:
            damage = WaveCannon::DAMAGE_LV1;
            width = WaveCannon::WIDTH_LV1;
            break;
        case 2:
            damage = WaveCannon::DAMAGE_LV2;
            width = WaveCannon::WIDTH_LV2;
            break;
        case 3:
            damage = WaveCannon::DAMAGE_LV3;
            width = WaveCannon::WIDTH_LV3;
            break;
        default:
            return;
    }

    // Créer missile spécial "Wave Cannon"
    Missile beam{
        .id = _nextMissileId++,
        .owner_id = playerId,
        .weapon_type = WeaponType::WaveCannon,  // Nouveau type
        .x = it->second.x + 64.0f,
        .y = it->second.y,
        .velocityX = WaveCannon::SPEED,
        .velocityY = 0.0f,
        .damage = static_cast<uint8_t>(damage),
        .piercing = (level >= 2),  // Niveau 2+ traverse
        .width = width,
        .chargeLevel = level
    };

    _missiles[beam.id] = beam;
    broadcastMissileSpawned(beam);
}
```

### Modifications Client (GameScene)

```cpp
// GameScene.hpp
class GameScene {
    // ... existants ...
    float _chargeTimer = 0.0f;
    uint8_t _chargeLevel = 0;
    bool _isCharging = false;
    bool _fireHeld = false;
};

// GameScene.cpp - Input handling
void GameScene::handleEvent(const events::Event& event) {
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Space) {
            _fireHeld = true;
            _isCharging = true;
            _chargeTimer = 0.0f;
        }
    }

    if (auto* keyReleased = std::get_if<events::KeyReleased>(&event)) {
        if (keyReleased->key == events::Key::Space) {
            _fireHeld = false;

            if (_chargeLevel > 0) {
                // Envoyer ChargeRelease au serveur
                _context.udpClient->releaseCharge(_chargeLevel);
            } else {
                // Tir normal
                _context.udpClient->shootMissile(_selectedWeapon);
            }

            _isCharging = false;
            _chargeTimer = 0.0f;
            _chargeLevel = 0;
        }
    }
}

// GameScene.cpp - Update
void GameScene::update(float deltaTime) {
    // ... existant ...

    // Mettre à jour charge
    if (_isCharging && _fireHeld) {
        _chargeTimer += deltaTime;

        if (_chargeTimer >= 1.5f) _chargeLevel = 3;
        else if (_chargeTimer >= 1.0f) _chargeLevel = 2;
        else if (_chargeTimer >= 0.5f) _chargeLevel = 1;
        else _chargeLevel = 0;
    }
}

// GameScene.cpp - Rendu jauge BEAM
void GameScene::renderChargeGauge() {
    if (!_isCharging && _chargeLevel == 0) return;

    float gaugeWidth = 150.0f;
    float gaugeHeight = 12.0f;
    float gaugeX = 20.0f;
    float gaugeY = SCREEN_HEIGHT - 120.0f;

    // Background
    _context.window->drawRect(gaugeX, gaugeY, gaugeWidth, gaugeHeight, {40, 40, 40, 200});

    // Charge progress
    float progress = std::min(_chargeTimer / 1.5f, 1.0f);
    rgba chargeColor;

    if (_chargeLevel == 3) {
        chargeColor = {255, 50, 50, 255};   // Rouge - MAX
    } else if (_chargeLevel == 2) {
        chargeColor = {255, 200, 50, 255};  // Orange
    } else if (_chargeLevel == 1) {
        chargeColor = {255, 255, 100, 255}; // Jaune
    } else {
        chargeColor = {100, 100, 255, 255}; // Bleu - charging
    }

    _context.window->drawRect(gaugeX, gaugeY, gaugeWidth * progress, gaugeHeight, chargeColor);

    // Label
    std::string label = "BEAM";
    if (_chargeLevel == 3) label = "BEAM MAX!";
    _context.window->drawText(FONT_KEY, label, gaugeX, gaugeY - 18.0f, 14, chargeColor);

    // Indicateurs de niveau
    float lv1X = gaugeX + gaugeWidth * (0.5f / 1.5f);
    float lv2X = gaugeX + gaugeWidth * (1.0f / 1.5f);
    _context.window->drawRect(lv1X, gaugeY, 2, gaugeHeight, {255, 255, 255, 100});
    _context.window->drawRect(lv2X, gaugeY, 2, gaugeHeight, {255, 255, 255, 100});
}
```

### Rendu du Wave Cannon

```cpp
// GameScene.cpp
void GameScene::renderMissiles() {
    auto missiles = _context.udpClient->getMissiles();

    for (const auto& missile : missiles) {
        if (missile.weapon_type == static_cast<uint8_t>(WeaponType::WaveCannon)) {
            // Rendu spécial Wave Cannon
            renderWaveCannon(missile);
        } else {
            // Rendu missile normal
            renderNormalMissile(missile);
        }
    }
}

void GameScene::renderWaveCannon(const MissileState& beam) {
    // Effet de trainée lumineuse
    float width = 20.0f + beam.charge_level * 15.0f;
    float length = 60.0f + beam.charge_level * 30.0f;

    rgba beamColor;
    switch (beam.charge_level) {
        case 3: beamColor = {255, 100, 100, 255}; break;  // Rouge
        case 2: beamColor = {255, 200, 100, 255}; break;  // Orange
        default: beamColor = {255, 255, 150, 255}; break; // Jaune
    }

    // Core beam
    _context.window->drawRect(
        beam.x - length/2, beam.y - width/2,
        length, width,
        beamColor
    );

    // Glow effect (outer)
    rgba glowColor = beamColor;
    glowColor.a = 100;
    _context.window->drawRect(
        beam.x - length/2 - 5, beam.y - width/2 - 5,
        length + 10, width + 10,
        glowColor
    );
}
```

---

## 2.2 Système de Power-Ups - 2-3h

### Design des Power-Ups

| Item | Drop Rate | Effet | Visuel |
|------|-----------|-------|--------|
| **Health Pack** | 15% | +25 HP | Croix verte |
| **Speed-Up** | 10% | +1 vitesse (max 3) | "S" bleu |
| **Weapon Crystal** | 20% | Upgrade arme courante | Cristal coloré |
| **Shield** | 5% | Invincibilité 3s | Bulle dorée |

### POW Armor (Ennemi Spécial)

Comme dans R-Type, certains ennemis sont des "POW Armor" qui droppent des items :

```cpp
// GameWorld.hpp
enum class EnemyType : uint8_t {
    Basic = 0,
    Tracker = 1,
    Zigzag = 2,
    Fast = 3,
    Bomber = 4,
    POWArmor = 5     // NOUVEAU - Drop items
};

// Spawn POW Armor
// Apparaît toutes les 20-30 secondes
// Plus facile à tuer (20 HP)
// Garantit un drop
```

### Modifications Protocol

```cpp
// Protocol.hpp
enum class MessageType : uint16_t {
    // ... existants ...
    PowerUpSpawned = 0x00E0,   // S→C : Item apparu
    PowerUpCollected = 0x00E1, // S→C : Item ramassé
    PowerUpExpired = 0x00E2,   // S→C : Item disparu
};

enum class PowerUpType : uint8_t {
    Health = 0,
    SpeedUp = 1,
    WeaponCrystal = 2,
    Shield = 3
};

struct PowerUpState {
    uint16_t id;
    uint16_t x, y;
    uint8_t type;           // PowerUpType
    uint8_t remaining_time; // Secondes avant expiration (10s)

    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const;
    static std::optional<PowerUpState> from_bytes(const void* buf, size_t len);
};

// Extension GameSnapshot
struct GameSnapshot {
    // ... existants ...
    uint8_t powerup_count;
    PowerUpState powerups[MAX_POWERUPS];  // MAX_POWERUPS = 8
};
```

### Modifications Serveur

```cpp
// GameWorld.hpp
struct PowerUp {
    uint16_t id;
    float x, y;
    PowerUpType type;
    float lifetime;          // Temps restant (10s)
    static constexpr float MAX_LIFETIME = 10.0f;
    static constexpr float WIDTH = 24.0f;
    static constexpr float HEIGHT = 24.0f;
};

class GameWorld {
    // ... existants ...
    std::unordered_map<uint16_t, PowerUp> _powerUps;
    uint16_t _nextPowerUpId = 1;
    float _powArmorSpawnTimer = 0.0f;

    void spawnPowerUp(float x, float y);
    void updatePowerUps(float deltaTime);
    void checkPowerUpCollisions();
    void applyPowerUp(uint8_t playerId, PowerUpType type);
    void spawnPOWArmor();
};

// GameWorld.cpp
void GameWorld::onEnemyKilled(uint16_t enemyId, uint8_t killerPlayerId) {
    auto it = _enemies.find(enemyId);
    if (it == _enemies.end()) return;

    const auto& enemy = it->second;

    // POW Armor drop garanti
    if (enemy.type == EnemyType::POWArmor) {
        spawnPowerUp(enemy.x, enemy.y);
    } else {
        // Autres ennemis : 15% de chance
        if (std::rand() % 100 < 15) {
            spawnPowerUp(enemy.x, enemy.y);
        }
    }

    // Award score...
}

void GameWorld::spawnPowerUp(float x, float y) {
    // Sélection aléatoire du type
    int roll = std::rand() % 100;
    PowerUpType type;

    if (roll < 30) type = PowerUpType::Health;           // 30%
    else if (roll < 50) type = PowerUpType::SpeedUp;     // 20%
    else if (roll < 80) type = PowerUpType::WeaponCrystal; // 30%
    else type = PowerUpType::Shield;                      // 20%

    PowerUp pu{
        .id = _nextPowerUpId++,
        .x = x,
        .y = y,
        .type = type,
        .lifetime = PowerUp::MAX_LIFETIME
    };

    _powerUps[pu.id] = pu;
    broadcastPowerUpSpawned(pu);
}

void GameWorld::updatePowerUps(float deltaTime) {
    std::vector<uint16_t> expired;

    for (auto& [id, pu] : _powerUps) {
        pu.lifetime -= deltaTime;

        // Mouvement lent vers la gauche
        pu.x -= 30.0f * deltaTime;

        if (pu.lifetime <= 0.0f || pu.x < -50.0f) {
            expired.push_back(id);
        }
    }

    for (uint16_t id : expired) {
        _powerUps.erase(id);
        broadcastPowerUpExpired(id);
    }
}

void GameWorld::checkPowerUpCollisions() {
    std::vector<uint16_t> collected;

    for (auto& [puId, pu] : _powerUps) {
        collision::AABB puBox{pu.x, pu.y, PowerUp::WIDTH, PowerUp::HEIGHT};

        for (auto& [playerId, player] : _players) {
            if (!player.alive) continue;

            collision::AABB playerBox{
                player.x, player.y,
                Player::WIDTH, Player::HEIGHT
            };

            if (puBox.intersects(playerBox)) {
                applyPowerUp(playerId, pu.type);
                collected.push_back(puId);
                broadcastPowerUpCollected(puId, playerId);
                break;
            }
        }
    }

    for (uint16_t id : collected) {
        _powerUps.erase(id);
    }
}

void GameWorld::applyPowerUp(uint8_t playerId, PowerUpType type) {
    auto& player = _players[playerId];
    auto& connected = _connectedPlayers[playerId];

    switch (type) {
        case PowerUpType::Health:
            player.health = std::min(100, player.health + 25);
            break;

        case PowerUpType::SpeedUp:
            connected.speedLevel = std::min(3, connected.speedLevel + 1);
            break;

        case PowerUpType::WeaponCrystal:
            // Upgrade l'arme courante
            connected.weaponLevel = std::min(3, connected.weaponLevel + 1);
            break;

        case PowerUpType::Shield:
            connected.shieldTimer = 3.0f;  // 3 secondes d'invincibilité
            break;
    }
}

// Spawn POW Armor périodiquement
void GameWorld::update(float deltaTime) {
    // ... existant ...

    _powArmorSpawnTimer += deltaTime;
    if (_powArmorSpawnTimer >= 25.0f) {  // Toutes les 25 secondes
        spawnPOWArmor();
        _powArmorSpawnTimer = 0.0f;
    }
}

void GameWorld::spawnPOWArmor() {
    Enemy powArmor{
        .id = _nextEnemyId++,
        .x = SCREEN_WIDTH + 50.0f,
        .y = static_cast<float>(100 + std::rand() % 400),
        .health = 20,
        .enemy_type = EnemyType::POWArmor,
        .speedX = -80.0f,  // Lent
        .shootTimer = 999.0f  // Ne tire pas
    };

    _enemies[powArmor.id] = powArmor;
    broadcastEnemySpawned(powArmor);
}
```

### Modifications Client

```cpp
// GameScene.cpp
void GameScene::renderPowerUps() {
    auto powerUps = _context.udpClient->getPowerUps();

    for (const auto& pu : powerUps) {
        std::string textureKey;
        rgba fallbackColor;

        switch (static_cast<PowerUpType>(pu.type)) {
            case PowerUpType::Health:
                textureKey = "powerup_health";
                fallbackColor = {50, 255, 50, 255};  // Vert
                break;
            case PowerUpType::SpeedUp:
                textureKey = "powerup_speed";
                fallbackColor = {50, 150, 255, 255}; // Bleu
                break;
            case PowerUpType::WeaponCrystal:
                textureKey = "powerup_weapon";
                fallbackColor = {255, 200, 50, 255}; // Or
                break;
            case PowerUpType::Shield:
                textureKey = "powerup_shield";
                fallbackColor = {255, 255, 100, 255}; // Jaune brillant
                break;
        }

        // Effet de pulsation
        float pulse = 1.0f + 0.1f * std::sin(_gameTime * 8.0f);
        float size = 24.0f * pulse;

        // Essayer sprite, sinon fallback couleur
        if (!_context.window->drawSprite(textureKey, pu.x, pu.y, size, size)) {
            _context.window->drawRect(pu.x, pu.y, size, size, fallbackColor);
        }

        // Indicateur temps restant (clignotement si < 3s)
        if (pu.remaining_time < 3) {
            if (static_cast<int>(_gameTime * 4) % 2 == 0) {
                // Flash blanc
                _context.window->drawRect(pu.x - 2, pu.y - 2, size + 4, size + 4,
                    {255, 255, 255, 100});
            }
        }
    }
}

// Feedback visuel quand power-up collecté
void GameScene::onPowerUpCollected(PowerUpType type, uint8_t playerId) {
    if (playerId == _context.udpClient->getLocalPlayerId()) {
        // Flash écran selon type
        switch (type) {
            case PowerUpType::Health:
                _screenFlash = {50, 255, 50, 80};
                break;
            case PowerUpType::SpeedUp:
                _screenFlash = {50, 150, 255, 80};
                break;
            case PowerUpType::WeaponCrystal:
                _screenFlash = {255, 200, 50, 80};
                break;
            case PowerUpType::Shield:
                _screenFlash = {255, 255, 100, 80};
                break;
        }
        _screenFlashTimer = 0.2f;
    }
}
```

---

## 2.3 Force Pod (Simplifié) - 5-6h

### Design Simplifié

Pour notre version multiplayer, on simplifie le Force Pod :

| Aspect | R-Type Original | Notre Version Simplifiée |
|--------|-----------------|--------------------------|
| Attachable avant/arrière | Oui | **Avant uniquement** |
| Détachable | Oui, vole librement | **Oui, mais suit le joueur** |
| Niveaux | 3 | **2** (sans Force, avec Force) |
| Bloque projectiles | Oui | **Oui** |
| Damage on contact | Oui | **Oui** |

### Modifications Protocol

```cpp
// Protocol.hpp
enum class MessageType : uint16_t {
    // ... existants ...
    ForceAttach = 0x00F0,      // C→S : Attacher/Détacher Force
    ForceState = 0x00F1,       // S→C : État du Force (dans snapshot)
};

struct ForceState {
    uint8_t owner_id;          // Joueur propriétaire
    uint16_t x, y;             // Position
    uint8_t is_attached;       // 0 = détaché, 1 = attaché
    uint8_t level;             // 0 = pas de Force, 1-2 = niveau

    static constexpr size_t WIRE_SIZE = 7;
};

// Extension PlayerState
struct PlayerState {
    // ... existants ...
    uint8_t has_force;         // 0 ou 1
    uint8_t force_attached;    // 0 ou 1
};
```

### Modifications Serveur

```cpp
// GameWorld.hpp
struct ForcePod {
    uint8_t ownerId;
    float x, y;
    float targetX, targetY;    // Pour mouvement smooth
    bool isAttached;
    uint8_t level;             // 1-2

    static constexpr float WIDTH = 32.0f;
    static constexpr float HEIGHT = 32.0f;
    static constexpr float SPEED = 400.0f;
    static constexpr float ATTACH_OFFSET = 50.0f;
    static constexpr uint8_t CONTACT_DAMAGE = 30;
};

class GameWorld {
    // ... existants ...
    std::unordered_map<uint8_t, ForcePod> _forcePods;  // Par joueur

    void giveForceToPlayer(uint8_t playerId);
    void updateForcePods(float deltaTime);
    void toggleForceAttach(uint8_t playerId);
    void checkForceCollisions();
    void forceBlockProjectile(uint16_t missileId);
};

// GameWorld.cpp
void GameWorld::giveForceToPlayer(uint8_t playerId) {
    if (_forcePods.find(playerId) != _forcePods.end()) {
        // Déjà un Force, upgrade
        _forcePods[playerId].level = std::min(2, _forcePods[playerId].level + 1);
        return;
    }

    auto& player = _players[playerId];
    ForcePod force{
        .ownerId = playerId,
        .x = player.x + ForcePod::ATTACH_OFFSET,
        .y = player.y,
        .targetX = player.x + ForcePod::ATTACH_OFFSET,
        .targetY = player.y,
        .isAttached = true,
        .level = 1
    };

    _forcePods[playerId] = force;
}

void GameWorld::updateForcePods(float deltaTime) {
    for (auto& [playerId, force] : _forcePods) {
        auto& player = _players[playerId];

        if (force.isAttached) {
            // Suit le joueur directement
            force.targetX = player.x + ForcePod::ATTACH_OFFSET;
            force.targetY = player.y;
        } else {
            // Mode détaché : reste devant mais ne suit pas Y
            force.targetX = player.x + ForcePod::ATTACH_OFFSET + 100.0f;
            // targetY ne change pas
        }

        // Mouvement smooth vers target
        float dx = force.targetX - force.x;
        float dy = force.targetY - force.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist > 1.0f) {
            float speed = force.isAttached ? 800.0f : ForcePod::SPEED;
            float move = speed * deltaTime;

            if (move > dist) {
                force.x = force.targetX;
                force.y = force.targetY;
            } else {
                force.x += (dx / dist) * move;
                force.y += (dy / dist) * move;
            }
        }
    }
}

void GameWorld::toggleForceAttach(uint8_t playerId) {
    if (_forcePods.find(playerId) == _forcePods.end()) return;

    auto& force = _forcePods[playerId];
    force.isAttached = !force.isAttached;

    if (!force.isAttached) {
        // Quand détaché, le Force garde sa position Y
        force.targetY = force.y;
    }
}

void GameWorld::checkForceCollisions() {
    for (auto& [playerId, force] : _forcePods) {
        collision::AABB forceBox{force.x, force.y, ForcePod::WIDTH, ForcePod::HEIGHT};

        // Collision avec ennemis
        for (auto& [enemyId, enemy] : _enemies) {
            collision::AABB enemyBox{enemy.x, enemy.y, Enemy::WIDTH, Enemy::HEIGHT};

            if (forceBox.intersects(enemyBox)) {
                // Force détruit l'ennemi au contact
                enemy.health -= ForcePod::CONTACT_DAMAGE;
                if (enemy.health <= 0) {
                    onEnemyKilled(enemyId, playerId);
                }
            }
        }

        // Collision avec missiles ennemis (bloque)
        std::vector<uint16_t> blocked;
        for (auto& [missileId, missile] : _enemyMissiles) {
            collision::AABB missileBox{missile.x, missile.y,
                Missile::WIDTH, Missile::HEIGHT};

            if (forceBox.intersects(missileBox)) {
                blocked.push_back(missileId);
            }
        }

        for (uint16_t id : blocked) {
            _enemyMissiles.erase(id);
            // Optionnel: effet visuel de blocage
        }
    }
}
```

### Modifications Client

```cpp
// GameScene.cpp
void GameScene::handleEvent(const events::Event& event) {
    // ... existant ...

    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        // F pour toggle Force attach/detach
        if (keyPressed->key == events::Key::F) {
            _context.udpClient->toggleForce();
        }
    }
}

void GameScene::renderForcePods() {
    auto forces = _context.udpClient->getForceStates();

    for (const auto& force : forces) {
        if (force.level == 0) continue;

        float size = 32.0f + (force.level - 1) * 8.0f;

        // Glow effect
        rgba glowColor = {100, 150, 255, 100};
        _context.window->drawRect(
            force.x - size/2 - 4, force.y - size/2 - 4,
            size + 8, size + 8,
            glowColor
        );

        // Core
        rgba coreColor = {150, 200, 255, 255};
        if (!force.is_attached) {
            coreColor = {255, 200, 150, 255};  // Orange si détaché
        }

        // Sprite ou fallback
        if (!_context.window->drawSprite("force_pod", force.x, force.y, size, size)) {
            _context.window->drawRect(force.x - size/2, force.y - size/2, size, size, coreColor);
        }

        // Indicateur "FORCE" pour le joueur local
        if (force.owner_id == _context.udpClient->getLocalPlayerId()) {
            std::string status = force.is_attached ? "FORCE [F]" : "FORCE (FREE)";
            _context.window->drawText(FONT_KEY, status,
                force.x - 30, force.y - 25, 10, {255, 255, 255, 200});
        }
    }
}
```

---

## 2.4 Speed-Up System - 1h

### Design

| Niveau | Vitesse | Multiplicateur |
|--------|---------|----------------|
| 0 (base) | 200 px/s | 1.0x |
| 1 | 280 px/s | 1.4x |
| 2 | 360 px/s | 1.8x |
| 3 (max) | 440 px/s | 2.2x |

**Note** : Comme dans R-Type, trop de speed-ups rend le vaisseau difficile à contrôler !

### Modifications

```cpp
// GameWorld.hpp
struct ConnectedPlayer {
    // ... existants ...
    uint8_t speedLevel = 0;  // 0-3
};

// GameWorld.cpp
float GameWorld::getPlayerSpeed(uint8_t playerId) const {
    auto it = _connectedPlayers.find(playerId);
    if (it == _connectedPlayers.end()) return 200.0f;

    static const float SPEEDS[4] = {200.0f, 280.0f, 360.0f, 440.0f};
    return SPEEDS[std::min(3, static_cast<int>(it->second.speedLevel))];
}

// Dans applyPowerUp
case PowerUpType::SpeedUp:
    connected.speedLevel = std::min(3, connected.speedLevel + 1);
    break;
```

### HUD Indicateur Vitesse

```cpp
// GameScene.cpp
void GameScene::renderSpeedIndicator() {
    uint8_t speedLevel = _context.udpClient->getLocalPlayerSpeedLevel();

    float x = 20.0f;
    float y = SCREEN_HEIGHT - 150.0f;

    _context.window->drawText(FONT_KEY, "SPEED", x, y, 12, {150, 150, 150, 255});

    // Barres de vitesse
    for (int i = 0; i < 3; ++i) {
        rgba color = (i < speedLevel) ? rgba{50, 200, 255, 255} : rgba{60, 60, 60, 255};
        _context.window->drawRect(x + i * 18, y + 15, 15, 8, color);
    }
}
```

---

## 2.5 Effets Visuels (Polish) - 2-3h

### Screen Shake

```cpp
// GameScene.hpp
class GameScene {
    float _shakeIntensity = 0.0f;
    float _shakeTimer = 0.0f;
    float _shakeOffsetX = 0.0f;
    float _shakeOffsetY = 0.0f;
};

// GameScene.cpp
void GameScene::triggerScreenShake(float intensity, float duration) {
    _shakeIntensity = intensity;
    _shakeTimer = duration;
}

void GameScene::updateScreenShake(float deltaTime) {
    if (_shakeTimer > 0.0f) {
        _shakeTimer -= deltaTime;

        // Random offset
        _shakeOffsetX = (std::rand() % 100 - 50) / 50.0f * _shakeIntensity;
        _shakeOffsetY = (std::rand() % 100 - 50) / 50.0f * _shakeIntensity;

        // Decay
        _shakeIntensity *= 0.9f;
    } else {
        _shakeOffsetX = 0.0f;
        _shakeOffsetY = 0.0f;
    }
}

// Appliquer dans render()
void GameScene::render() {
    // Appliquer offset de shake à tous les éléments
    // Ou utiliser une transformation globale si supporté
}
```

### Screen Flash

```cpp
// GameScene.hpp
rgba _screenFlash = {0, 0, 0, 0};
float _screenFlashTimer = 0.0f;

// GameScene.cpp
void GameScene::triggerScreenFlash(rgba color, float duration) {
    _screenFlash = color;
    _screenFlashTimer = duration;
}

void GameScene::renderScreenFlash() {
    if (_screenFlashTimer > 0.0f) {
        // Overlay semi-transparent
        rgba flashColor = _screenFlash;
        flashColor.a = static_cast<uint8_t>(flashColor.a * (_screenFlashTimer / 0.3f));

        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, flashColor);
    }
}

// Exemples d'utilisation
void GameScene::onPlayerHit() {
    triggerScreenFlash({255, 50, 50, 150}, 0.15f);  // Flash rouge
    triggerScreenShake(8.0f, 0.2f);
}

void GameScene::onBossPhaseChange() {
    triggerScreenFlash({255, 255, 255, 200}, 0.3f);  // Flash blanc
    triggerScreenShake(15.0f, 0.5f);
}
```

### Particules d'Explosion (Simplifié)

```cpp
// GameScene.hpp
struct Particle {
    float x, y;
    float vx, vy;
    float lifetime;
    rgba color;
    float size;
};

std::vector<Particle> _particles;

// GameScene.cpp
void GameScene::spawnExplosion(float x, float y, rgba color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (std::rand() % 360) * M_PI / 180.0f;
        float speed = 50.0f + std::rand() % 150;

        Particle p{
            .x = x,
            .y = y,
            .vx = std::cos(angle) * speed,
            .vy = std::sin(angle) * speed,
            .lifetime = 0.3f + (std::rand() % 100) / 200.0f,
            .color = color,
            .size = 3.0f + std::rand() % 5
        };

        _particles.push_back(p);
    }
}

void GameScene::updateParticles(float deltaTime) {
    for (auto& p : _particles) {
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.lifetime -= deltaTime;
        p.size *= 0.95f;  // Shrink
    }

    // Remove dead particles
    _particles.erase(
        std::remove_if(_particles.begin(), _particles.end(),
            [](const Particle& p) { return p.lifetime <= 0; }),
        _particles.end()
    );
}

void GameScene::renderParticles() {
    for (const auto& p : _particles) {
        rgba color = p.color;
        color.a = static_cast<uint8_t>(255 * (p.lifetime / 0.5f));
        _context.window->drawRect(p.x - p.size/2, p.y - p.size/2, p.size, p.size, color);
    }
}

// Utilisation
void GameScene::onEnemyDestroyed(float x, float y, uint8_t enemyType) {
    rgba color;
    switch (enemyType) {
        case 0: color = {255, 200, 50, 255}; break;   // Basic: orange
        case 1: color = {255, 50, 50, 255}; break;    // Tracker: rouge
        case 2: color = {50, 255, 50, 255}; break;    // Zigzag: vert
        case 3: color = {255, 255, 50, 255}; break;   // Fast: jaune
        case 4: color = {150, 50, 255, 255}; break;   // Bomber: violet
        default: color = {255, 255, 255, 255};
    }

    spawnExplosion(x, y, color, 15);
    triggerScreenShake(3.0f, 0.1f);
}

void GameScene::onBossDestroyed(float x, float y) {
    // Grosse explosion
    for (int i = 0; i < 5; ++i) {
        float ox = x + (std::rand() % 100 - 50);
        float oy = y + (std::rand() % 80 - 40);
        spawnExplosion(ox, oy, {255, 100, 50, 255}, 30);
    }
    triggerScreenShake(20.0f, 1.0f);
    triggerScreenFlash({255, 255, 255, 255}, 0.5f);
}
```

---

# PARTIE 3 : PLAN D'IMPLÉMENTATION

## 3.1 Ordre de Priorité

| Priorité | Feature | Effort | Dépendances | Impact |
|----------|---------|--------|-------------|--------|
| 🔴 **P0** | Wave Cannon (Charge Shot) | 3-4h | - | ⭐⭐⭐⭐⭐ |
| 🔴 **P0** | Power-up Drops | 2-3h | - | ⭐⭐⭐⭐⭐ |
| 🟠 **P1** | Force Pod (simplifié) | 5-6h | Power-ups | ⭐⭐⭐⭐⭐ |
| 🟠 **P1** | Speed-Up System | 1h | Power-ups | ⭐⭐⭐ |
| 🟡 **P2** | Screen Shake | 1h | - | ⭐⭐⭐⭐ |
| 🟡 **P2** | Screen Flash | 30min | - | ⭐⭐⭐ |
| 🟡 **P2** | Particules Explosion | 1-2h | - | ⭐⭐⭐⭐ |
| ⚪ **P3** | Bits (Options) | 3-4h | Force Pod | ⭐⭐ |

## 3.2 Planning Suggéré

```
┌──────────┬────────────────────────────────────────────────────────┐
│   Jour   │                        Tâches                          │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 1  │ [P0] Wave Cannon: Protocol + Server (2h)              │
│          │ [P0] Wave Cannon: Client charge gauge + render (2h)    │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 2  │ [P0] Power-ups: Protocol + Server spawn/collect (2h)  │
│          │ [P0] Power-ups: Client render + feedback (1h)          │
│          │ [P1] Speed-Up: Integration avec power-ups (1h)         │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 3  │ [P1] Force Pod: Protocol + Server logic (3h)          │
│          │ [P1] Force Pod: Client render + controls (2h)          │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 4  │ [P2] Effets visuels: Shake, Flash, Particules (2-3h)  │
│          │ Tests de gameplay et balance                           │
│          │ Polish et bugfix                                       │
└──────────┴────────────────────────────────────────────────────────┘
```

## 3.3 Fichiers à Modifier

| Fichier | Wave Cannon | Power-ups | Force Pod | Effets |
|---------|-------------|-----------|-----------|--------|
| `Protocol.hpp` | ✓ | ✓ | ✓ | |
| `GameWorld.hpp` | ✓ | ✓ | ✓ | |
| `GameWorld.cpp` | ✓ | ✓ | ✓ | |
| `UDPServer.cpp` | ✓ | ✓ | ✓ | |
| `UDPClient.hpp` | ✓ | ✓ | ✓ | |
| `UDPClient.cpp` | ✓ | ✓ | ✓ | |
| `GameScene.hpp` | ✓ | ✓ | ✓ | ✓ |
| `GameScene.cpp` | ✓ | ✓ | ✓ | ✓ |

---

# PARTIE 4 : ASSETS REQUIS

## 4.1 Nouveaux Sprites

| Asset | Chemin | Dimensions | Description |
|-------|--------|------------|-------------|
| Wave Cannon Lv1 | `assets/effects/beam_lv1.png` | 40x20 | Petit beam jaune |
| Wave Cannon Lv2 | `assets/effects/beam_lv2.png` | 60x30 | Beam moyen orange |
| Wave Cannon Lv3 | `assets/effects/beam_lv3.png` | 100x40 | MEGA beam rouge |
| Force Pod | `assets/player/force_pod.png` | 32x32 | Orbe bleu lumineux |
| POW Armor | `assets/enemies/pow_armor.png` | 48x48 | Ennemi qui drop items |
| Power-up Health | `assets/powerups/health.png` | 24x24 | Croix verte |
| Power-up Speed | `assets/powerups/speed.png` | 24x24 | "S" bleu |
| Power-up Weapon | `assets/powerups/weapon.png` | 24x24 | Cristal doré |
| Power-up Shield | `assets/powerups/shield.png` | 24x24 | Bulle dorée |

## 4.2 Sons Suggérés

| Son | Fichier | Description |
|-----|---------|-------------|
| Charge start | `assets/audio/charge_start.wav` | Début de charge |
| Charge loop | `assets/audio/charge_loop.wav` | Pendant la charge |
| Beam fire | `assets/audio/beam_fire.wav` | Tir du Wave Cannon |
| Power-up collect | `assets/audio/powerup.wav` | Ramasser un item |
| Force attach | `assets/audio/force_attach.wav` | Attacher le Force |
| Force detach | `assets/audio/force_detach.wav` | Détacher le Force |

---

# PARTIE 5 : CRITÈRES RNCP COUVERTS

## 5.1 Mapping Implementation → RNCP

| Implementation | Critère RNCP | Evidence |
|----------------|--------------|----------|
| Wave Cannon (charge system) | **C8** - Solution créative | Mécanique stratégique depth |
| Power-up drops | **C10** - Traduire specs | Système itemisation complet |
| Force Pod | **C12** - Solutions originales | Mécanique unique fidèle R-Type |
| Effets visuels (particules) | **C15** - UI/UX | Feedback visuel satisfaisant |
| Speed-Up progression | **C11** - Segmenter | Système progression modulaire |

## 5.2 Impact Score RNCP Estimé

| Phase | Score | Description |
|-------|-------|-------------|
| Après PR #6 (TLS) | ~80/100 | Sécurité |
| Après PR #7 (Gameplay) | ~88/100 | Boss, Score, Armes |
| **Après PRIORITE_3** | **~93-95/100** | Mécaniques R-Type authentiques |

---

# PARTIE 6 : TESTS ET VALIDATION

## 6.1 Tests Fonctionnels

| Test | Description | Critère Succès |
|------|-------------|----------------|
| Wave Cannon charge | Maintenir tir charge la jauge | 3 niveaux distincts |
| Wave Cannon release | Relâcher tire le beam | Dégâts selon niveau |
| Power-up spawn | Tuer ennemi → chance de drop | Items apparaissent |
| Power-up collect | Toucher item → effet | HP/Speed/etc. appliqué |
| Force Pod attach | Appuyer F → toggle | Force suit/ne suit pas |
| Force Pod block | Force devant → bloque missiles | Missiles détruits |
| Screen shake | Boss hit → écran tremble | Vibration visible |
| Particules | Ennemi meurt → explosion | Particules colorées |

## 6.2 Tests de Balance

| Aspect | Valeur Testée | Ajustement si besoin |
|--------|---------------|----------------------|
| Charge time | 1.5s pour max | ±0.3s |
| Power-up drop rate | 15% | ±5% |
| POW Armor spawn | 25s | ±5s |
| Force damage | 30 on contact | ±10 |
| Speed-Up max | 2.2x | ±0.2x |

---

# ANNEXES

## A. Références R-Type

- [R-Type Wikipedia](https://en.wikipedia.org/wiki/R-Type)
- [R-Type StrategyWiki Gameplay](https://strategywiki.org/wiki/R-Type/Gameplay)
- [R-Type Shmups Wiki](https://shmups.wiki/library/R-Type)
- [R-Type Wiki Fandom](https://rtype.fandom.com/wiki/R-Type)
- [Dobkeratops - Boss iconique](https://rtype.fandom.com/wiki/Dobkeratops)

## B. Inspirations Design

- [R-Type Force Pod Analysis](https://shmups.wiki/library/R-Type/Strategy)
- [Wave Cannon Mechanics](https://strategywiki.org/wiki/R-Type/Gameplay)
- [Power-up Systems in Shmups](https://shmups.wiki/library/Boghog%27s_bullet_hell_shmup_101)

## C. Assets Gratuits Recommandés

- [Kenney Space Shooter Redux](https://kenney.nl/assets/space-shooter-redux) - Sprites haute qualité
- [OpenGameArt Space Assets](https://opengameart.org/content/space-shooter-redux) - CC0
- [itch.io Space Packs](https://itch.io/game-assets/tag-space) - Variété

---

*Document généré le 14/01/2026*
*Projet: R-Type Multiplayer - EPITECH*
*Auteur: Claude Code Assistant*
