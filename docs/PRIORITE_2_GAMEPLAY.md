# PRIORITE 2 - Plan d'Implementation Complet GAMEPLAY

## Contexte

Ce document fait suite à **PRIORITE_1_IMPLEMENTATION.md** (TLS + CSPRNG), dont les améliorations ont été implémentées dans la PR #6. Le score RNCP était de **77/100** lors de l'analyse initiale et a été amélioré après les travaux de sécurité et documentation.

### Déjà Réalisé (PR #6 - Janvier 2026)
- ✅ **C6** - Étude comparative (TLS/DTLS, CSPRNG) → `docs/RNCP/C6_ETUDE_COMPARATIVE.md`
- ✅ **C7** - Audit sécurité → `docs/RNCP/C7_AUDIT_SECURITE.md`
- ✅ **C8** - Évaluation prototypes (voice chat) → `docs/RNCP/C8_EVALUATION_PROTOTYPES.md`
- ✅ TLS 1.2+ sur TCP 4125 (authentification)
- ✅ CSPRNG via OpenSSL `RAND_bytes()` pour tokens de session

### À Réaliser (Cette Phase)
Focus sur le **gameplay** pour améliorer l'expérience utilisateur et couvrir les critères restants.

---

## Resume Executif

| Critere RNCP | Objectif | Statut |
|--------------|----------|--------|
| **C8** | Solution technique créative (Boss, Armes) | 🔴 À faire |
| **C10** | Traduire specs en composants (Protocol, GameWorld) | 🔴 À faire |
| **C11** | Segmenter problèmes complexes (Architecture modulaire) | 🟡 Partiel |
| **C12** | Solutions originales (IA Boss, Combo system) | 🔴 À faire |
| **C14** | Structures de données adaptées (WeaponDef, BossState) | 🔴 À faire |
| **C15** | Interfaces UI/UX optimisées (HUD, Score, Boss HP) | 🟡 Partiel |

**Objectif** : Transformer R-Type d'une démo technique en un jeu amusant et impressionnant pour la soutenance.

---

# PARTIE 1 : ANALYSE DU CODE GAMEPLAY EXISTANT

## 1.1 Architecture Gameplay Actuelle

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              SERVEUR - GameWorld                              │
│                                                                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │     Players     │  │    Missiles     │  │     Enemies     │              │
│  │  unordered_map  │  │  unordered_map  │  │  unordered_map  │              │
│  │   <id, Player>  │  │  <id, Missile>  │  │   <id, Enemy>   │              │
│  │                 │  │                 │  │                 │              │
│  │  - x, y         │  │  - x, y         │  │  - x, y         │              │
│  │  - health       │  │  - owner_id     │  │  - health       │              │
│  │  - alive        │  │  - velocityX    │  │  - enemy_type   │              │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘              │
│           │                    │                    │                        │
│           └────────────────────┼────────────────────┘                        │
│                                │                                             │
│                    ┌───────────┴───────────┐                                 │
│                    │     GameSnapshot      │  ◄── Broadcast 20Hz (50ms)      │
│                    │                       │                                 │
│                    │  players[]            │                                 │
│                    │  missiles[]           │                                 │
│                    │  enemies[]            │                                 │
│                    │  enemy_missiles[]     │                                 │
│                    │                       │                                 │
│                    │  ❌ PAS DE SCORE      │                                 │
│                    │  ❌ PAS DE BOSS       │                                 │
│                    │  ❌ PAS DE WAVE INFO  │                                 │
│                    └───────────────────────┘                                 │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 1.2 Systeme d'Ennemis Existant

### Localisation : `src/server/include/infrastructure/game/GameWorld.hpp`

```cpp
enum class EnemyType : uint8_t {
    Basic = 0,      // Sine wave movement, slow
    Tracker = 1,    // Follows nearest player
    Zigzag = 2,     // Zigzag vertical movement
    Fast = 3,       // Fast horizontal
    Bomber = 4      // Slow, fires 2 missiles
};
```

### Parametres par Type d'Ennemi

| Type | Vitesse X | PV | Intervalle Tir | Comportement |
|------|-----------|-----|----------------|--------------|
| Basic | -120 | 30 | 2.5s | Onde sinusoidale amplitude 80, freq 1.8 |
| Tracker | -100 | 25 | 2.0s | Suit le joueur Y le plus proche (150 px/s) |
| Zigzag | -140 | 20 | 3.0s | Alterne haut/bas toutes les 0.8s (300 px/s) |
| Fast | -220 | 15 | 1.5s | Sinus haute frequence (2.5x), amplitude reduite |
| Bomber | -80 | 50 | 1.0s | Descend +10px/frame, tire 2 missiles/burst |

### Spawn Progressif (GameWorld.cpp:352-363)

```cpp
EnemyType type = EnemyType::Basic;  // Default
int roll = std::rand() % 100;

if (_waveNumber >= 5 && roll < 10)       type = EnemyType::Bomber;   // 10%
else if (_waveNumber >= 4 && roll < 25)  type = EnemyType::Fast;     // 15%
else if (_waveNumber >= 3 && roll < 40)  type = EnemyType::Zigzag;   // 15%
else if (_waveNumber >= 2 && roll < 55)  type = EnemyType::Tracker;  // 15%
// else: Basic (45%)
```

## 1.3 BUG CRITIQUE : Sprites Ennemis Identiques

### Localisation : `src/client/src/scenes/GameScene.cpp:581-593`

```cpp
void GameScene::renderEnemies() {
    auto enemies = _context.udpClient->getEnemies();
    for (const auto& enemy : enemies) {
        _context.window->drawSprite(
            ENEMY_TEXTURE_KEY,  // ← TOUJOURS "Ship1.png" !
            static_cast<float>(enemy.x),
            static_cast<float>(enemy.y),
            ENEMY_WIDTH,
            ENEMY_HEIGHT
        );
    }
}
```

**Probleme** : Le champ `enemy.enemy_type` est recu mais **jamais utilise** pour le rendu.
Les 5 types d'ennemis ont des comportements differents mais **apparaissent identiques**.

---

## 1.4 Systeme de Missiles Existant

### Structure Protocol (Protocol.hpp)

```cpp
struct MissileState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x, y;
    // ❌ PAS DE weapon_type !
    static constexpr size_t WIRE_SIZE = 7;
};

struct ShootMissile {
    // ❌ PAYLOAD VIDE - pas de selection d'arme
    static constexpr size_t WIRE_SIZE = 0;
};
```

### Constantes Missile (GameWorld.hpp)

| Constante | Valeur | Note |
|-----------|--------|------|
| `Missile::SPEED` | 600.0f | Vitesse fixe |
| `Missile::WIDTH` | 16.0f | Hitbox |
| `Missile::HEIGHT` | 8.0f | Hitbox |
| `MISSILE_SPAWN_OFFSET_X` | 64.0f | Spawn devant le vaisseau |
| `SHOOT_COOLDOWN_TIME` | 0.3s | Client-side |

**Limitation** : Un seul type de missile, pas de degats variables, pas de patterns.

---

## 1.5 Systeme de Score : INEXISTANT

- Aucun champ `score` dans `PlayerState`
- Aucun message `ScoreUpdate` dans le protocole
- Aucun tracking des kills
- `EnemyDestroyed` (0x0091) existe mais ne contient que `enemy_id`

---

# PARTIE 2 : DESIGN DU NOUVEAU GAMEPLAY

## 2.1 Inspirations R-Type Original

Sources : [R-Type Wikipedia](https://en.wikipedia.org/wiki/R-Type), [Shmups Wiki Strategy](https://shmups.wiki/library/R-Type/Strategy)

### Mecaniques Cles du R-Type Original

1. **Force Pod** : Orbe detachable offrant protection et puissance de feu additionnelle
2. **Charge Shot** : Tir charge plus puissant (maintenir le bouton)
3. **3 Types de Laser** : Rouge (Helix), Bleu (Rebond), Jaune (Tracking)
4. **Boss Memorables** : Dobkeratops, Cyst, Compiler avec patterns distinctifs
5. **Difficulte Progressive** : Patterns rigides mais justes, memorisation recompensee

### Best Practices Shoot'em Up Modernes

Sources : [Fat Pug Studio Scoring](https://www.fatpugstudio.com/dev-log/scoring-system-design/), [Shmups Wiki](https://shmups.wiki/library/Boghog's_bullet_hell_shmup_101)

1. **Score** : Points par kill + multiplicateur combo + bonus vague sans degat
2. **Armes** : Roles distincts (spread, laser, heavy) avec balance
3. **Bombs** : Smart bomb pour situations critiques (panic button)
4. **Risk/Reward** : Grazing (frôler les balles), combos temps-limite
5. **Feedback Visuel** : Effets satisfaisants, ecran shake, particules

---

## 2.2 Architecture Cible

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         NOUVEAU SERVEUR - GameWorld                           │
│                                                                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │     Players     │  │    Missiles     │  │     Enemies     │              │
│  │                 │  │                 │  │                 │              │
│  │  + score        │  │  + weapon_type  │  │  + point_value  │              │
│  │  + kills        │  │  + damage       │  │  (par type)     │              │
│  │  + combo        │  │  + pattern      │  │                 │              │
│  │  + weapon_slot  │  │                 │  │                 │              │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘              │
│           │                    │                    │                        │
│           │         ┌─────────────────────┐        │                        │
│           │         │        BOSS         │        │                        │
│           │         │                     │        │                        │
│           │         │  - current_phase    │        │                        │
│           │         │  - max_health       │        │                        │
│           │         │  - health           │        │                        │
│           │         │  - pattern_timer    │        │                        │
│           │         │  - attack_patterns[]│        │                        │
│           │         └──────────┬──────────┘        │                        │
│           │                    │                   │                        │
│           └────────────────────┼───────────────────┘                        │
│                                │                                             │
│                    ┌───────────┴───────────┐                                 │
│                    │   GameSnapshot v2     │                                 │
│                    │                       │                                 │
│                    │  + wave_number        │                                 │
│                    │  + boss_state         │                                 │
│                    │  + scores[]           │                                 │
│                    └───────────────────────┘                                 │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

# PARTIE 3 : SPECIFICATIONS TECHNIQUES DETAILLEES

## 3.1 Fix Sprites Ennemis (Quick Win - 30 min)

### Objectif
Utiliser le champ `enemy_type` deja transmis pour afficher des sprites differents.

### Fichiers a Modifier

| Fichier | Modification |
|---------|--------------|
| `GameScene.cpp` | Ajouter `getEnemyTextureKey(uint8_t type)` |
| `GameScene.cpp` | Charger 5 textures ennemis |

### Implementation

```cpp
// GameScene.hpp - Nouveaux texture keys
static constexpr const char* ENEMY_BASIC_KEY = "enemy_basic";
static constexpr const char* ENEMY_TRACKER_KEY = "enemy_tracker";
static constexpr const char* ENEMY_ZIGZAG_KEY = "enemy_zigzag";
static constexpr const char* ENEMY_FAST_KEY = "enemy_fast";
static constexpr const char* ENEMY_BOMBER_KEY = "enemy_bomber";

// GameScene.cpp - Chargement assets
void GameScene::loadAssets() {
    // ... existant ...
    _context.window->loadTexture(ENEMY_BASIC_KEY, "assets/enemies/basic.png");
    _context.window->loadTexture(ENEMY_TRACKER_KEY, "assets/enemies/tracker.png");
    _context.window->loadTexture(ENEMY_ZIGZAG_KEY, "assets/enemies/zigzag.png");
    _context.window->loadTexture(ENEMY_FAST_KEY, "assets/enemies/fast.png");
    _context.window->loadTexture(ENEMY_BOMBER_KEY, "assets/enemies/bomber.png");
}

// GameScene.cpp - Helper function
std::string GameScene::getEnemyTextureKey(uint8_t enemyType) {
    switch (enemyType) {
        case 0: return ENEMY_BASIC_KEY;
        case 1: return ENEMY_TRACKER_KEY;
        case 2: return ENEMY_ZIGZAG_KEY;
        case 3: return ENEMY_FAST_KEY;
        case 4: return ENEMY_BOMBER_KEY;
        default: return ENEMY_BASIC_KEY;
    }
}

// GameScene.cpp - Rendu modifie
void GameScene::renderEnemies() {
    auto enemies = _context.udpClient->getEnemies();
    for (const auto& enemy : enemies) {
        _context.window->drawSprite(
            getEnemyTextureKey(enemy.enemy_type),  // ← UTILISE LE TYPE
            static_cast<float>(enemy.x),
            static_cast<float>(enemy.y),
            ENEMY_WIDTH,
            ENEMY_HEIGHT
        );
    }
}
```

### Assets Requis

| Fichier | Description | Taille Suggeree |
|---------|-------------|-----------------|
| `assets/enemies/basic.png` | Ennemi standard | 64x64 |
| `assets/enemies/tracker.png` | Ennemi poursuiveur (rouge) | 64x64 |
| `assets/enemies/zigzag.png` | Ennemi zigzag (vert) | 64x64 |
| `assets/enemies/fast.png` | Ennemi rapide (jaune) | 48x48 |
| `assets/enemies/bomber.png` | Ennemi lourd (gros, violet) | 80x80 |

---

## 3.2 Systeme de Score (2-3h)

### Objectif
Implementer un systeme de score avec points par kill, combo, et multiplicateur.

### Design du Score

| Action | Points Base | Multiplicateur |
|--------|-------------|----------------|
| Kill Basic | 100 | x combo |
| Kill Tracker | 150 | x combo |
| Kill Zigzag | 120 | x combo |
| Kill Fast | 180 | x combo |
| Kill Bomber | 250 | x combo |
| Kill Boss | 5000 | x phase |
| Vague sans degat | 500 | fixe |
| Combo (kills < 2s) | - | +0.1 par kill (max 3.0x) |

### Modifications Protocol (Protocol.hpp)

```cpp
// Nouveau message type
enum class MessageType : uint16_t {
    // ... existants ...
    ScoreUpdate = 0x00B0,      // S→C : Mise a jour score joueur
    ComboUpdate = 0x00B1,      // S→C : Combo change
    WaveComplete = 0x00B2,     // S→C : Vague terminee + bonus
};

// Nouvelle structure
struct ScoreUpdate {
    uint8_t player_id;
    uint32_t score;           // Score total
    uint16_t kills;           // Nombre de kills
    uint8_t combo_multiplier; // x10 (ex: 15 = 1.5x)

    static constexpr size_t WIRE_SIZE = 8;

    void to_bytes(uint8_t* buf) const;
    static std::optional<ScoreUpdate> from_bytes(const void* buf, size_t len);
};

// Extension PlayerState (ajouter au snapshot)
struct PlayerState {
    uint8_t id;
    uint16_t x, y;
    uint8_t health;
    uint8_t alive;
    uint32_t score;           // NOUVEAU
    uint8_t combo;            // NOUVEAU (x10)
    static constexpr size_t WIRE_SIZE = 12;  // etait 7
};
```

### Modifications Serveur (GameWorld)

```cpp
// GameWorld.hpp - Ajouts
struct PlayerScore {
    uint32_t score = 0;
    uint16_t kills = 0;
    float comboMultiplier = 1.0f;
    float comboTimer = 0.0f;       // Reset si > 2s sans kill
    bool tookDamageThisWave = false;
};

class GameWorld {
    // ... existant ...
    std::unordered_map<uint8_t, PlayerScore> _playerScores;

    // Nouvelles methodes
    void awardKillScore(uint8_t playerId, EnemyType enemyType);
    void updateComboTimers(float deltaTime);
    void onWaveComplete();
    uint16_t getEnemyPointValue(EnemyType type) const;
};

// GameWorld.cpp - Implementation
uint16_t GameWorld::getEnemyPointValue(EnemyType type) const {
    switch (type) {
        case EnemyType::Basic:   return 100;
        case EnemyType::Tracker: return 150;
        case EnemyType::Zigzag:  return 120;
        case EnemyType::Fast:    return 180;
        case EnemyType::Bomber:  return 250;
        default: return 100;
    }
}

void GameWorld::awardKillScore(uint8_t playerId, EnemyType enemyType) {
    auto& score = _playerScores[playerId];

    uint16_t basePoints = getEnemyPointValue(enemyType);
    uint32_t points = static_cast<uint32_t>(basePoints * score.comboMultiplier);

    score.score += points;
    score.kills++;

    // Augmenter combo (max 3.0x)
    score.comboMultiplier = std::min(3.0f, score.comboMultiplier + 0.1f);
    score.comboTimer = 0.0f;  // Reset timer
}

void GameWorld::updateComboTimers(float deltaTime) {
    for (auto& [playerId, score] : _playerScores) {
        score.comboTimer += deltaTime;
        if (score.comboTimer > 2.0f) {
            score.comboMultiplier = 1.0f;  // Reset combo
        }
    }
}
```

### Modifications Client (GameScene)

```cpp
// GameScene.cpp - Rendu HUD Score
void GameScene::renderScoreHUD() {
    auto localId = _context.udpClient->getLocalPlayerId();
    auto players = _context.udpClient->getPlayers();

    // Trouver score local
    uint32_t score = 0;
    uint8_t combo = 10;  // 1.0x par defaut
    for (const auto& p : players) {
        if (p.id == localId) {
            score = p.score;
            combo = p.combo;
            break;
        }
    }

    // Afficher score (top-right)
    float scoreX = SCREEN_WIDTH - 250.0f;
    float scoreY = 20.0f;

    std::string scoreText = "SCORE: " + std::to_string(score);
    _context.window->drawText(FONT_KEY, scoreText, scoreX, scoreY, 24, {255, 255, 255, 255});

    // Afficher combo si > 1.0x
    if (combo > 10) {
        float comboValue = combo / 10.0f;
        std::string comboText = "COMBO x" + std::to_string(comboValue).substr(0, 3);
        rgba comboColor = {255, 200, 0, 255};  // Jaune/or
        _context.window->drawText(FONT_KEY, comboText, scoreX, scoreY + 30.0f, 20, comboColor);
    }
}
```

---

## 3.3 Systeme d'Armes Multiples (4-6h)

### Design des Armes

| Arme | Touche | Cooldown | Degats | Pattern | Description |
|------|--------|----------|--------|---------|-------------|
| **Basic** | 1 | 0.25s | 15 | 1 tir droit | Arme de depart |
| **Spread** | 2 | 0.40s | 10x3 | 3 tirs eventail | Couverture large |
| **Laser** | 3 | 0.80s | 40 | 1 rayon perçant | Traverse les ennemis |
| **Heavy** | 4 | 1.00s | 60 | 1 gros projectile | Lent mais devastateur |

### Modifications Protocol

```cpp
// Protocol.hpp
enum class WeaponType : uint8_t {
    Basic = 0,
    Spread = 1,
    Laser = 2,
    Heavy = 3
};

// Message client modifie
struct ShootMissile {
    uint8_t weapon_slot;  // 0-3
    static constexpr size_t WIRE_SIZE = 1;  // etait 0

    void to_bytes(uint8_t* buf) const {
        buf[0] = weapon_slot;
    }
    static std::optional<ShootMissile> from_bytes(const void* buf, size_t len) {
        if (len < WIRE_SIZE) return std::nullopt;
        return ShootMissile{buf[0]};
    }
};

// MissileState etendu
struct MissileState {
    uint16_t id;
    uint8_t owner_id;
    uint8_t weapon_type;   // NOUVEAU
    uint16_t x, y;
    static constexpr size_t WIRE_SIZE = 8;  // etait 7
};
```

### Modifications Serveur

```cpp
// GameWorld.hpp
struct WeaponDefinition {
    WeaponType type;
    float speed;
    float cooldown;
    uint8_t damage;
    uint8_t projectiles;     // Nombre de missiles
    float spreadAngle;       // Pour Spread (degrees)
    bool piercing;           // Pour Laser
};

static const std::array<WeaponDefinition, 4> WEAPON_DEFS = {{
    {WeaponType::Basic,  600.0f, 0.25f, 15, 1, 0.0f,  false},
    {WeaponType::Spread, 500.0f, 0.40f, 10, 3, 15.0f, false},
    {WeaponType::Laser,  800.0f, 0.80f, 40, 1, 0.0f,  true},
    {WeaponType::Heavy,  300.0f, 1.00f, 60, 1, 0.0f,  false}
}};

// Missile etendu
struct Missile {
    uint16_t id;
    uint8_t owner_id;
    WeaponType weapon_type;
    float x, y;
    float velocityX, velocityY;  // Pour spread
    uint8_t damage;
    bool piercing;
};

// GameWorld - Spawn multi-projectile
std::vector<uint16_t> GameWorld::spawnMissiles(uint8_t playerId, WeaponType weapon) {
    std::vector<uint16_t> ids;
    const auto& def = WEAPON_DEFS[static_cast<int>(weapon)];

    auto it = _players.find(playerId);
    if (it == _players.end()) return ids;

    float baseX = it->second.x + MISSILE_SPAWN_OFFSET_X;
    float baseY = it->second.y + MISSILE_SPAWN_OFFSET_Y;

    for (uint8_t i = 0; i < def.projectiles; ++i) {
        float angle = 0.0f;
        if (def.projectiles > 1) {
            // Spread: -15, 0, +15 degrees
            angle = -def.spreadAngle + (2.0f * def.spreadAngle * i / (def.projectiles - 1));
        }

        float radians = angle * M_PI / 180.0f;
        float vx = def.speed * std::cos(radians);
        float vy = def.speed * std::sin(radians);

        Missile m{
            .id = _nextMissileId++,
            .owner_id = playerId,
            .weapon_type = weapon,
            .x = baseX,
            .y = baseY,
            .velocityX = vx,
            .velocityY = vy,
            .damage = def.damage,
            .piercing = def.piercing
        };

        _missiles[m.id] = m;
        ids.push_back(m.id);
    }

    return ids;
}
```

### Modifications Client

```cpp
// GameScene.hpp
uint8_t _selectedWeapon = 0;
std::array<float, 4> _weaponCooldowns = {0, 0, 0, 0};

// GameScene.cpp - Input handling
void GameScene::handleWeaponSwitch(events::Key key) {
    if (key == events::Key::Num1) _selectedWeapon = 0;
    else if (key == events::Key::Num2) _selectedWeapon = 1;
    else if (key == events::Key::Num3) _selectedWeapon = 2;
    else if (key == events::Key::Num4) _selectedWeapon = 3;
}

void GameScene::handleShoot() {
    if (_weaponCooldowns[_selectedWeapon] <= 0.0f) {
        _context.udpClient->shootMissile(_selectedWeapon);

        // Cooldowns par arme
        static const float COOLDOWNS[4] = {0.25f, 0.40f, 0.80f, 1.00f};
        _weaponCooldowns[_selectedWeapon] = COOLDOWNS[_selectedWeapon];
    }
}

// GameScene.cpp - Rendu HUD armes
void GameScene::renderWeaponHUD() {
    float startX = 20.0f;
    float startY = SCREEN_HEIGHT - 80.0f;

    static const char* WEAPON_NAMES[4] = {"BASIC", "SPREAD", "LASER", "HEAVY"};
    static const rgba WEAPON_COLORS[4] = {
        {200, 200, 200, 255},  // Basic: gris
        {100, 200, 255, 255},  // Spread: bleu clair
        {255, 100, 100, 255},  // Laser: rouge
        {255, 200, 50, 255}    // Heavy: orange
    };

    for (int i = 0; i < 4; ++i) {
        float x = startX + i * 100.0f;
        rgba color = WEAPON_COLORS[i];

        // Arme selectionnee = plus lumineux
        if (i == _selectedWeapon) {
            color.a = 255;
            // Dessiner indicateur
            _context.window->drawRect(x - 5, startY - 5, 90, 40, {255, 255, 255, 100});
        } else {
            color.a = 150;
        }

        // Nom de l'arme
        _context.window->drawText(FONT_KEY, WEAPON_NAMES[i], x, startY, 16, color);

        // Touche
        std::string keyText = "[" + std::to_string(i + 1) + "]";
        _context.window->drawText(FONT_KEY, keyText, x, startY + 20, 14, {150, 150, 150, 255});

        // Cooldown bar
        if (_weaponCooldowns[i] > 0.0f) {
            static const float MAX_CD[4] = {0.25f, 0.40f, 0.80f, 1.00f};
            float cdRatio = _weaponCooldowns[i] / MAX_CD[i];
            _context.window->drawRect(x, startY + 35, 80 * (1.0f - cdRatio), 5, color);
        }
    }
}
```

---

## 3.4 Systeme de Boss (4-6h)

### Design du Boss

Inspire de [R-Type Dobkeratops](https://shmups.wiki/library/R-Type/Strategy) et patterns classiques.

#### Boss "Nemesis" - Stats

| Phase | HP | Vitesse | Patterns | Trigger |
|-------|-----|---------|----------|---------|
| Phase 1 | 300 | Lent | Tirs lineaires | 100% HP |
| Phase 2 | - | Moyen | + Missiles tracking | 60% HP |
| Phase 3 | - | Rapide | + Barrage circulaire | 30% HP |

#### Spawn du Boss

- Apparait apres la **vague 10** (ou configurable)
- Vagues normales suspendues pendant le combat
- Musique change (si audio implemente)

### Modifications Protocol

```cpp
// Protocol.hpp
struct BossState {
    uint16_t id;
    uint16_t x, y;
    uint16_t max_health;
    uint16_t health;
    uint8_t phase;           // 1, 2, 3
    uint8_t is_active;       // 0 ou 1

    static constexpr size_t WIRE_SIZE = 12;

    void to_bytes(uint8_t* buf) const;
    static std::optional<BossState> from_bytes(const void* buf, size_t len);
};

// MessageType
enum class MessageType : uint16_t {
    // ... existants ...
    BossSpawn = 0x00C0,       // S→C : Boss apparait
    BossPhaseChange = 0x00C1, // S→C : Phase change
    BossDefeated = 0x00C2,    // S→C : Boss vaincu
};

// GameSnapshot etendu
struct GameSnapshot {
    // ... existant ...
    uint8_t has_boss;         // 0 ou 1
    BossState boss_state;     // Si has_boss == 1
    uint8_t wave_number;      // Numero de vague actuel
};
```

### Modifications Serveur

```cpp
// GameWorld.hpp
enum class BossPhase : uint8_t {
    None = 0,
    Phase1 = 1,  // Tirs lineaires
    Phase2 = 2,  // + Tracking
    Phase3 = 3   // + Barrage
};

struct Boss {
    uint16_t id;
    float x, y;
    float baseY;              // Pour mouvement sinusoidal
    uint16_t maxHealth;
    uint16_t health;
    BossPhase phase;
    float phaseTimer;         // Timer pour patterns
    float shootTimer;         // Timer pour tirs
    float moveTimer;          // Timer pour mouvements
    bool isActive;

    // Patterns
    void updatePhase1(float dt, GameWorld& world);
    void updatePhase2(float dt, GameWorld& world);
    void updatePhase3(float dt, GameWorld& world);
};

class GameWorld {
    // ... existant ...
    std::optional<Boss> _boss;
    uint8_t _bossSpawnWave = 10;  // Configurable

    void checkBossSpawn();
    void updateBoss(float deltaTime);
    void damageBoss(uint8_t damage, uint8_t playerId);
    void onBossDefeated();
};

// GameWorld.cpp
void GameWorld::checkBossSpawn() {
    if (!_boss.has_value() && _waveNumber >= _bossSpawnWave && _enemies.empty()) {
        // Spawn boss a droite de l'ecran
        Boss boss{
            .id = 9999,
            .x = SCREEN_WIDTH - 100.0f,
            .y = SCREEN_HEIGHT / 2.0f,
            .baseY = SCREEN_HEIGHT / 2.0f,
            .maxHealth = 300,
            .health = 300,
            .phase = BossPhase::Phase1,
            .phaseTimer = 0.0f,
            .shootTimer = 0.0f,
            .moveTimer = 0.0f,
            .isActive = true
        };
        _boss = boss;

        // Notifier clients
        broadcastBossSpawn();
    }
}

void GameWorld::updateBoss(float deltaTime) {
    if (!_boss.has_value() || !_boss->isActive) return;

    auto& boss = _boss.value();
    boss.phaseTimer += deltaTime;
    boss.shootTimer += deltaTime;
    boss.moveTimer += deltaTime;

    // Mouvement sinusoidal vertical
    boss.y = boss.baseY + std::sin(boss.moveTimer * 0.5f) * 100.0f;

    // Changement de phase base sur HP
    if (boss.health <= boss.maxHealth * 0.30f && boss.phase != BossPhase::Phase3) {
        boss.phase = BossPhase::Phase3;
        broadcastBossPhaseChange(3);
    } else if (boss.health <= boss.maxHealth * 0.60f && boss.phase == BossPhase::Phase1) {
        boss.phase = BossPhase::Phase2;
        broadcastBossPhaseChange(2);
    }

    // Executer pattern selon phase
    switch (boss.phase) {
        case BossPhase::Phase1: boss.updatePhase1(deltaTime, *this); break;
        case BossPhase::Phase2: boss.updatePhase2(deltaTime, *this); break;
        case BossPhase::Phase3: boss.updatePhase3(deltaTime, *this); break;
        default: break;
    }
}

void Boss::updatePhase1(float dt, GameWorld& world) {
    // Tir lineaire toutes les 1.5s
    if (shootTimer >= 1.5f) {
        shootTimer = 0.0f;
        // Spawn 3 missiles horizontaux
        for (int i = -1; i <= 1; ++i) {
            world.spawnEnemyMissile(x - 50.0f, y + i * 30.0f, -400.0f, 0.0f);
        }
    }
}

void Boss::updatePhase2(float dt, GameWorld& world) {
    updatePhase1(dt, world);  // Garde phase 1

    // + Missile tracking toutes les 2s
    if (phaseTimer >= 2.0f) {
        phaseTimer = 0.0f;
        // Trouver joueur le plus proche
        auto target = world.getNearestPlayer(x, y);
        if (target.has_value()) {
            float dx = target->x - x;
            float dy = target->y - y;
            float len = std::sqrt(dx*dx + dy*dy);
            if (len > 0) {
                world.spawnEnemyMissile(x - 50.0f, y, -300.0f * (dx/len), 300.0f * (dy/len));
            }
        }
    }
}

void Boss::updatePhase3(float dt, GameWorld& world) {
    updatePhase2(dt, world);  // Garde phase 1 + 2

    // + Barrage circulaire toutes les 3s
    static float barrageTimer = 0.0f;
    barrageTimer += dt;
    if (barrageTimer >= 3.0f) {
        barrageTimer = 0.0f;
        // 8 missiles en cercle
        for (int i = 0; i < 8; ++i) {
            float angle = i * (360.0f / 8.0f) * M_PI / 180.0f;
            float vx = std::cos(angle) * 250.0f;
            float vy = std::sin(angle) * 250.0f;
            world.spawnEnemyMissile(x, y, vx, vy);
        }
    }
}
```

### Modifications Client

```cpp
// GameScene.cpp - Rendu Boss
void GameScene::renderBoss() {
    auto bossState = _context.udpClient->getBossState();
    if (!bossState.has_value() || !bossState->is_active) return;

    const auto& boss = bossState.value();

    // Sprite boss (grand)
    _context.window->drawSprite(
        BOSS_TEXTURE_KEY,
        static_cast<float>(boss.x),
        static_cast<float>(boss.y),
        BOSS_WIDTH,    // 150
        BOSS_HEIGHT    // 120
    );

    // Barre de vie boss (haut de l'ecran, centree)
    renderBossHealthBar(boss);
}

void GameScene::renderBossHealthBar(const BossState& boss) {
    float barWidth = 400.0f;
    float barHeight = 25.0f;
    float barX = (SCREEN_WIDTH - barWidth) / 2.0f;
    float barY = 30.0f;

    // Background
    _context.window->drawRect(barX, barY, barWidth, barHeight, {40, 40, 40, 200});

    // HP bar
    float hpRatio = static_cast<float>(boss.health) / static_cast<float>(boss.max_health);
    rgba hpColor;
    if (boss.phase == 3) hpColor = {255, 50, 50, 255};      // Rouge phase 3
    else if (boss.phase == 2) hpColor = {255, 150, 50, 255}; // Orange phase 2
    else hpColor = {255, 220, 50, 255};                      // Jaune phase 1

    _context.window->drawRect(barX, barY, barWidth * hpRatio, barHeight, hpColor);

    // Nom du boss
    _context.window->drawText(FONT_KEY, "NEMESIS", barX, barY - 25.0f, 20, {255, 255, 255, 255});

    // Indicateur de phase
    std::string phaseText = "PHASE " + std::to_string(boss.phase);
    _context.window->drawText(FONT_KEY, phaseText, barX + barWidth + 10.0f, barY + 3.0f, 18, hpColor);
}
```

---

## 3.5 Amelioration UI/HUD (1-2h)

### Elements HUD Complets

```
┌────────────────────────────────────────────────────────────────────────────┐
│  [HP BAR████████░░]  WAVE 7                    SCORE: 12,450   COMBO x2.1  │
│                                                                             │
│                            ╔═══════════════════╗                           │
│                            ║   NEMESIS         ║  (si boss actif)         │
│                            ║ [██████████░░░░░] ║  PHASE 2                  │
│                            ╚═══════════════════╝                           │
│                                                                             │
│                                                                             │
│                                      ████                                   │
│            ▶                         ████  BOSS                            │
│         PLAYER                       ████                                   │
│                                                                             │
│                     ●  ●  ●                                                │
│                   MISSILES                                                  │
│                                                                             │
│  [1]BASIC  [2]SPREAD  [3]LASER  [4]HEAVY              [Voice: Push-to-Talk]│
│  ████████  ░░░░░░░░  ░░░░░░░░  ░░░░░░░░                                    │
│  (selected)                                                                 │
└────────────────────────────────────────────────────────────────────────────┘
```

### Implementation Rendu Complet

```cpp
void GameScene::render() {
    renderBackground();
    renderEnemies();
    renderBoss();           // NOUVEAU
    renderPlayers();
    renderMissiles();
    renderEnemyMissiles();
    renderHUD();            // Sante + Vague
    renderScoreHUD();       // NOUVEAU
    renderWeaponHUD();      // NOUVEAU
    renderBossHealthBar();  // NOUVEAU (si boss actif)
    renderVoiceIndicator();
    renderChatOverlay();
    renderDeathScreen();    // si mort
}
```

---

# PARTIE 4 : ASSETS REQUIS

## 4.1 Sprites

| Asset | Chemin | Dimensions | Description |
|-------|--------|------------|-------------|
| Ennemi Basic | `assets/enemies/basic.png` | 64x64 | Vaisseau gris standard |
| Ennemi Tracker | `assets/enemies/tracker.png` | 64x64 | Vaisseau rouge agressif |
| Ennemi Zigzag | `assets/enemies/zigzag.png` | 64x64 | Vaisseau vert erratique |
| Ennemi Fast | `assets/enemies/fast.png` | 48x48 | Petit vaisseau jaune |
| Ennemi Bomber | `assets/enemies/bomber.png` | 80x80 | Gros vaisseau violet |
| Boss Nemesis | `assets/boss/nemesis.png` | 150x120 | Boss principal |
| Missile Spread | `assets/missiles/spread.png` | 24x24 | Projectile bleu |
| Missile Laser | `assets/missiles/laser.png` | 64x8 | Rayon rouge |
| Missile Heavy | `assets/missiles/heavy.png` | 40x40 | Gros projectile orange |

## 4.2 Sources Gratuites Recommandees

1. **OpenGameArt.org** - Sprites gratuits
   - https://opengameart.org/content/space-shooter-redux
   - https://opengameart.org/content/spaceship-set-32x32px

2. **Kenney.nl** - Assets de qualite
   - https://kenney.nl/assets/space-shooter-redux

3. **itch.io** - Packs gratuits
   - https://itch.io/game-assets/tag-space

---

# PARTIE 5 : PLAN D'IMPLEMENTATION

## 5.1 Ordre de Priorite

| Priorite | Tache | Temps | Dependances | Impact Demo |
|----------|-------|-------|-------------|-------------|
| 🔴 P1 | Fix sprites ennemis | 30 min | Assets | Visuel immediat |
| 🔴 P2 | Systeme de score | 2-3h | Protocol | Feedback joueur |
| 🟠 P3 | Boss system | 4-6h | P1, P2 | WOW factor |
| 🟠 P4 | Armes multiples | 4-6h | Protocol | Gameplay depth |
| 🟡 P5 | UI/HUD polish | 1-2h | P2, P3, P4 | Polish final |

## 5.2 Planning Suggere

```
┌──────────┬────────────────────────────────────────────────────────┐
│   Jour   │                        Taches                          │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 1  │ [P1] Fix sprites ennemis (30 min)                     │
│          │ [P2] Score: Protocol + Server (2h)                     │
│          │ [P2] Score: Client HUD (1h)                            │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 2  │ [P3] Boss: Protocol + Struct (1h)                      │
│          │ [P3] Boss: Server logic + phases (3h)                  │
│          │ [P3] Boss: Client render + HP bar (2h)                 │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 3  │ [P4] Armes: Protocol + WeaponDef (1h)                  │
│          │ [P4] Armes: Server spawn multi-projectile (2h)         │
│          │ [P4] Armes: Client input + HUD (2h)                    │
├──────────┼────────────────────────────────────────────────────────┤
│  Jour 4  │ [P5] UI polish + tests + bugfix                        │
│          │ Telecharger/creer assets manquants                     │
│          │ Tests de gameplay et balance                           │
└──────────┴────────────────────────────────────────────────────────┘
```

## 5.3 Fichiers a Modifier

| Fichier | P1 | P2 | P3 | P4 | P5 |
|---------|----|----|----|----|-----|
| `Protocol.hpp` | | ✓ | ✓ | ✓ | |
| `GameWorld.hpp` | | ✓ | ✓ | ✓ | |
| `GameWorld.cpp` | | ✓ | ✓ | ✓ | |
| `UDPServer.cpp` | | ✓ | ✓ | ✓ | |
| `UDPClient.cpp` | | ✓ | ✓ | | |
| `UDPClient.hpp` | | ✓ | ✓ | | |
| `GameScene.cpp` | ✓ | ✓ | ✓ | ✓ | ✓ |
| `GameScene.hpp` | ✓ | ✓ | ✓ | ✓ | ✓ |

---

# PARTIE 6 : CRITERES RNCP COUVERTS

## 6.1 Mapping Implementation → RNCP

| Implementation | Critere RNCP | Evidence |
|----------------|--------------|----------|
| Boss system (phases, IA) | **C8** - Solution creative | Patterns originaux, difficulte progressive |
| Protocol extensions | **C10** - Traduire specs | Structures binaires, sérialisation |
| Score/Combo/Weapons | **C11** - Segmenter | Modules independants, responsabilites claires |
| Boss AI patterns | **C12** - Solutions originales | IA state-machine, patterns memorables |
| WeaponDefinition, BossState | **C14** - Structures donnees | Optimise pour reseau et gameplay |
| HUD Score/Armes/Boss HP | **C15** - UI/UX | Feedback visuel, accessibilite |

## 6.2 Impact Score RNCP

| Phase | Score | Description |
|-------|-------|-------------|
| Analyse initiale (12/01) | 77/100 | Avant améliorations |
| Après PR #6 (TLS/CSPRNG) | ~80/100 | Sécurité + Documentation RNCP |
| Après PRIORITE_2 (Gameplay) | Objectif 90+ | Gameplay complet + Polish |

### Critères RNCP Couverts par Phase

| Critère | PR #6 | PRIORITE_2 | Description |
|---------|-------|------------|-------------|
| C6 | ✅ | - | Étude comparative |
| C7 | ✅ | - | Audit sécurité |
| C8 | ✅ | ✅ | Prototypes (voice) + Créativité (boss) |
| C10 | - | ✅ | Traduction specs → composants |
| C11 | ✅ | ✅ | Segmentation problèmes |
| C12 | - | ✅ | Solutions originales (IA boss) |
| C14 | - | ✅ | Structures données optimisées |
| C15 | - | ✅ | UI/UX (HUD, feedback visuel) |

---

# PARTIE 7 : TESTS ET VALIDATION

## 7.1 Tests Fonctionnels

| Test | Description | Critere Succes |
|------|-------------|----------------|
| Sprites ennemis | Verifier 5 sprites differents | Visuellement distincts |
| Score increment | Kill ennemi = score augmente | Points corrects par type |
| Combo decay | 2s sans kill = combo reset | Multiplicateur revient a 1.0x |
| Boss spawn | Vague 10 terminee = boss | Boss visible avec HP bar |
| Boss phases | HP < 60% = phase 2, < 30% = phase 3 | Patterns changent |
| Armes switch | Touches 1-4 changent arme | HUD reflète selection |
| Armes cooldown | Cooldown respecte par arme | Impossible de spam |

## 7.2 Tests Performance

| Metrique | Cible | Comment Mesurer |
|----------|-------|-----------------|
| FPS | ≥ 60 | Compteur FPS in-game |
| Snapshot size | < 1KB | Wireshark |
| Latence tir | < 100ms | Ressenti joueur |

---

# ANNEXES

## A. References

- [R-Type Wikipedia](https://en.wikipedia.org/wiki/R-Type)
- [R-Type Strategy Guide](https://shmups.wiki/library/R-Type/Strategy)
- [Scoring System Design](https://www.fatpugstudio.com/dev-log/scoring-system-design/)
- [Shmup Mechanics](https://chaotik.co.za/shootem-up-mechanics/)
- [Bullet Hell 101](https://shmups.wiki/library/Boghog's_bullet_hell_shmup_101)

## B. Assets Gratuits

- [Kenney Space Shooter](https://kenney.nl/assets/space-shooter-redux)
- [OpenGameArt Spaceships](https://opengameart.org/content/space-shooter-redux)

---

*Document genere le 14/01/2026*
*Projet: R-Type Multiplayer - EPITECH*
*Auteur: Claude Code Assistant*
