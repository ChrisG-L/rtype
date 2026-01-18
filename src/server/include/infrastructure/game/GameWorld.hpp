/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameWorld - Manages game state and players
*/

#ifndef GAMEWORLD_HPP_
#define GAMEWORLD_HPP_

#include "Protocol.hpp"
#include <boost/asio.hpp>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <vector>
#include <array>
#include <chrono>
#include <random>

// ═══════════════════════════════════════════════════════════════════════════
// ECS Integration (Feature Flag)
// When USE_ECS_BACKEND is defined, GameWorld uses the ECS infrastructure
// alongside the existing implementation for gradual migration.
// ═══════════════════════════════════════════════════════════════════════════
#ifdef USE_ECS_BACKEND
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"

// Phase 1: Base Components
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/VelocityComp.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/components/LifetimeComp.hpp"
#include "infrastructure/ecs/components/OwnerComp.hpp"

// Phase 2: Entity Tags & AI
#include "infrastructure/ecs/components/MissileTag.hpp"
#include "infrastructure/ecs/components/EnemyTag.hpp"
#include "infrastructure/ecs/components/EnemyAIComp.hpp"
#include "infrastructure/ecs/components/PowerUpTag.hpp"
#include "infrastructure/ecs/components/WaveCannonTag.hpp"

// Phase 3: Player Components
#include "infrastructure/ecs/components/PlayerTag.hpp"
#include "infrastructure/ecs/components/ScoreComp.hpp"
#include "infrastructure/ecs/components/WeaponComp.hpp"
#include "infrastructure/ecs/components/SpeedLevelComp.hpp"

// Phase 2: Base Systems
#include "infrastructure/ecs/systems/MovementSystem.hpp"
#include "infrastructure/ecs/systems/LifetimeSystem.hpp"
#include "infrastructure/ecs/systems/CleanupSystem.hpp"
#include "infrastructure/ecs/systems/CollisionSystem.hpp"
#include "infrastructure/ecs/systems/DamageSystem.hpp"

// Phase 3: Advanced Systems
#include "infrastructure/ecs/systems/PlayerInputSystem.hpp"
#include "infrastructure/ecs/systems/WeaponSystem.hpp"
#include "infrastructure/ecs/systems/ScoreSystem.hpp"
#include "infrastructure/ecs/systems/EnemyAISystem.hpp"

// Domain Services
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"

#include <memory>

// Namespace aliases for cleaner code
namespace ecs = infrastructure::ecs;
#endif

namespace infrastructure::game {
    using boost::asio::ip::udp;

    struct ConnectedPlayer {
        uint8_t id;
        uint16_t x;
        uint16_t y;
        uint8_t health;
        bool alive;
        udp::endpoint endpoint;
        std::chrono::steady_clock::time_point lastActivity;
        uint8_t shipSkin = 1;  // Ship skin variant (1-6 for Ship1.png to Ship6.png)
        // Weapon system (Gameplay Phase 2)
        WeaponType currentWeapon = WeaponType::Standard;
        float shootCooldown = 0.0f;  // Per-player cooldown
        // R-Type Authentic Mechanics (Phase 3)
        float chargeTimer = 0.0f;      // Wave Cannon charge time
        uint8_t chargeLevel = 0;       // 0-3 charge level
        bool isCharging = false;       // Currently charging?
        uint8_t speedLevel = 0;        // Speed upgrade level (0-3)
        // Weapon levels per weapon type (0-3 each)
        // Index: 0=Standard, 1=Spread, 2=Laser, 3=Missile
        std::array<uint8_t, 4> weaponLevels = {0, 0, 0, 0};
        bool hasForce = false;         // Has Force Pod?
        uint8_t forceLevel = 0;        // Force Pod level (0-2)
        bool hasBits = false;          // Has Bit Devices? (2 orbiting satellites)
        bool godMode = false;          // Hidden: player is invincible (no HP loss)
    };

    struct Missile {
        uint16_t id;
        uint8_t owner_id;
        float x;
        float y;
        float velocityX;
        float velocityY = 0.0f;  // For spread/homing shots
        WeaponType weaponType = WeaponType::Standard;
        uint8_t weaponLevel = 0;  // Weapon upgrade level (0-3) for damage calculation
        uint16_t targetEnemyId = 0;  // For homing missiles
        bool isFromForce = false;  // True if fired by Force Pod (R-Type authentic)
        static constexpr float SPEED = 600.0f;
        static constexpr float WIDTH = 16.0f;
        static constexpr float HEIGHT = 8.0f;

        // Weapon-specific base speeds (Level 0)
        static constexpr float SPEED_STANDARD = 600.0f;
        static constexpr float SPEED_SPREAD = 550.0f;
        static constexpr float SPEED_LASER = 900.0f;
        static constexpr float SPEED_MISSILE = 350.0f;

        // Weapon-specific base damage (Level 0)
        // Balanced for ~60-70 DPS across all weapons at level 0
        static constexpr uint8_t DAMAGE_STANDARD = 20;   // 20 / 0.3s = 67 dps (reliable)
        static constexpr uint8_t DAMAGE_SPREAD = 8;      // 8×3 / 0.4s = 60 dps (area control)
        static constexpr uint8_t DAMAGE_LASER = 12;      // 12 / 0.18s = 67 dps (rapid, low per-hit)
        static constexpr uint8_t DAMAGE_MISSILE = 50;    // 50 / 0.7s = 71 dps (homing, burst)

        // Weapon-specific base cooldowns (Level 0)
        static constexpr float COOLDOWN_STANDARD = 0.3f;
        static constexpr float COOLDOWN_SPREAD = 0.4f;   // 3 shots spread
        static constexpr float COOLDOWN_LASER = 0.18f;   // Rapid fire but low damage
        static constexpr float COOLDOWN_MISSILE = 0.7f;  // High damage, needs aim assist

        // ═══════════════════════════════════════════════════════════════════
        // Weapon Level System (0-3)
        // Level 0: Base stats
        // Level 1: +15% damage, -5% cooldown
        // Level 2: +30% damage, -10% cooldown
        // Level 3: +50% damage, -15% cooldown, +10% speed
        // ═══════════════════════════════════════════════════════════════════

        // Damage multipliers per level (x100 for integer math)
        static constexpr uint8_t DAMAGE_MULT_LV0 = 100;  // 1.0x
        static constexpr uint8_t DAMAGE_MULT_LV1 = 115;  // 1.15x
        static constexpr uint8_t DAMAGE_MULT_LV2 = 130;  // 1.30x
        static constexpr uint8_t DAMAGE_MULT_LV3 = 150;  // 1.50x

        // Cooldown multipliers per level (x100, lower is faster)
        static constexpr uint8_t COOLDOWN_MULT_LV0 = 100;  // 1.0x
        static constexpr uint8_t COOLDOWN_MULT_LV1 = 95;   // 0.95x
        static constexpr uint8_t COOLDOWN_MULT_LV2 = 90;   // 0.90x
        static constexpr uint8_t COOLDOWN_MULT_LV3 = 85;   // 0.85x

        // Speed multipliers per level (x100, only LV3 gets bonus)
        static constexpr uint8_t SPEED_MULT_LV0 = 100;  // 1.0x
        static constexpr uint8_t SPEED_MULT_LV1 = 100;  // 1.0x
        static constexpr uint8_t SPEED_MULT_LV2 = 100;  // 1.0x
        static constexpr uint8_t SPEED_MULT_LV3 = 110;  // 1.10x

        static float getBaseSpeed(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return SPEED_SPREAD;
                case WeaponType::Laser: return SPEED_LASER;
                case WeaponType::Missile: return SPEED_MISSILE;
                default: return SPEED_STANDARD;
            }
        }

        static uint8_t getBaseDamage(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return DAMAGE_SPREAD;
                case WeaponType::Laser: return DAMAGE_LASER;
                case WeaponType::Missile: return DAMAGE_MISSILE;
                default: return DAMAGE_STANDARD;
            }
        }

        static float getBaseCooldown(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return COOLDOWN_SPREAD;
                case WeaponType::Laser: return COOLDOWN_LASER;
                case WeaponType::Missile: return COOLDOWN_MISSILE;
                default: return COOLDOWN_STANDARD;
            }
        }

        // Get level multiplier helpers
        static uint8_t getDamageMultiplier(uint8_t level) {
            switch (level) {
                case 1: return DAMAGE_MULT_LV1;
                case 2: return DAMAGE_MULT_LV2;
                case 3: return DAMAGE_MULT_LV3;
                default: return DAMAGE_MULT_LV0;
            }
        }

        static uint8_t getCooldownMultiplier(uint8_t level) {
            switch (level) {
                case 1: return COOLDOWN_MULT_LV1;
                case 2: return COOLDOWN_MULT_LV2;
                case 3: return COOLDOWN_MULT_LV3;
                default: return COOLDOWN_MULT_LV0;
            }
        }

        static uint8_t getSpeedMultiplier(uint8_t level) {
            switch (level) {
                case 1: return SPEED_MULT_LV1;
                case 2: return SPEED_MULT_LV2;
                case 3: return SPEED_MULT_LV3;
                default: return SPEED_MULT_LV0;
            }
        }

        // Main accessors with level support
        static float getSpeed(WeaponType type, uint8_t level = 0) {
            float base = getBaseSpeed(type);
            return base * getSpeedMultiplier(level) / 100.0f;
        }

        static uint8_t getDamage(WeaponType type, uint8_t level = 0) {
            uint8_t base = getBaseDamage(type);
            return static_cast<uint8_t>(base * getDamageMultiplier(level) / 100);
        }

        static float getCooldown(WeaponType type, uint8_t level = 0) {
            float base = getBaseCooldown(type);
            return base * getCooldownMultiplier(level) / 100.0f;
        }
    };

    enum class EnemyType : uint8_t {
        Basic = 0,
        Tracker = 1,
        Zigzag = 2,
        Fast = 3,
        Bomber = 4,
        POWArmor = 5    // Special enemy that always drops a power-up
    };

    struct Enemy {
        uint16_t id;
        float x;
        float y;
        uint8_t health;
        uint8_t enemy_type;
        float baseY;
        float phaseOffset;
        float aliveTime;
        float shootCooldown;
        float targetY;
        float zigzagTimer;
        bool zigzagUp;

        static constexpr float SPEED_X_BASIC = -120.0f;
        static constexpr float SPEED_X_TRACKER = -100.0f;
        static constexpr float SPEED_X_ZIGZAG = -140.0f;
        static constexpr float SPEED_X_FAST = -220.0f;
        static constexpr float SPEED_X_BOMBER = -80.0f;
        static constexpr float SPEED_X_POW_ARMOR = -90.0f;  // Slow, easy to hit

        static constexpr float AMPLITUDE = 80.0f;
        static constexpr float FREQUENCY = 1.8f;
        static constexpr float ZIGZAG_SPEED_Y = 300.0f;
        static constexpr float ZIGZAG_INTERVAL = 0.8f;
        static constexpr float TRACKER_SPEED_Y = 150.0f;

        static constexpr float SHOOT_INTERVAL_BASIC = 2.5f;
        static constexpr float SHOOT_INTERVAL_TRACKER = 2.0f;
        static constexpr float SHOOT_INTERVAL_ZIGZAG = 3.0f;
        static constexpr float SHOOT_INTERVAL_FAST = 1.5f;
        static constexpr float SHOOT_INTERVAL_BOMBER = 1.0f;
        static constexpr float SHOOT_INTERVAL_POW_ARMOR = 4.0f;  // Rarely shoots

        // Enemy HP balanced for weapon DPS (~60-70 dps)
        // Standard (20 dmg): Basic dies in 2 shots, Bomber in 4
        static constexpr uint8_t HEALTH_BASIC = 40;     // 2 Standard shots or 5 Laser
        static constexpr uint8_t HEALTH_TRACKER = 35;   // Follows player, moderate HP
        static constexpr uint8_t HEALTH_ZIGZAG = 30;    // Dodges, survives 1 Standard shot
        static constexpr uint8_t HEALTH_FAST = 25;      // Fast but fragile
        static constexpr uint8_t HEALTH_BOMBER = 80;    // Tanky, 4 Standard shots
        static constexpr uint8_t HEALTH_POW_ARMOR = 60; // Tanky reward enemy, 100% power-up drop

        static constexpr float WIDTH = 40.0f;
        static constexpr float HEIGHT = 40.0f;

        float getSpeedX() const {
            switch (static_cast<EnemyType>(enemy_type)) {
                case EnemyType::Tracker: return SPEED_X_TRACKER;
                case EnemyType::Zigzag: return SPEED_X_ZIGZAG;
                case EnemyType::Fast: return SPEED_X_FAST;
                case EnemyType::Bomber: return SPEED_X_BOMBER;
                case EnemyType::POWArmor: return SPEED_X_POW_ARMOR;
                default: return SPEED_X_BASIC;
            }
        }

        float getShootInterval() const {
            switch (static_cast<EnemyType>(enemy_type)) {
                case EnemyType::Tracker: return SHOOT_INTERVAL_TRACKER;
                case EnemyType::Zigzag: return SHOOT_INTERVAL_ZIGZAG;
                case EnemyType::Fast: return SHOOT_INTERVAL_FAST;
                case EnemyType::Bomber: return SHOOT_INTERVAL_BOMBER;
                case EnemyType::POWArmor: return SHOOT_INTERVAL_POW_ARMOR;
                default: return SHOOT_INTERVAL_BASIC;
            }
        }

        static uint8_t getHealthForType(EnemyType type) {
            switch (type) {
                case EnemyType::Tracker: return HEALTH_TRACKER;
                case EnemyType::Zigzag: return HEALTH_ZIGZAG;
                case EnemyType::Fast: return HEALTH_FAST;
                case EnemyType::Bomber: return HEALTH_BOMBER;
                case EnemyType::POWArmor: return HEALTH_POW_ARMOR;
                default: return HEALTH_BASIC;
            }
        }
    };

    struct SpawnEntry {
        float delay;
        float spawnY;
        EnemyType type;
    };

    // Score system (Gameplay Phase 2)
    struct PlayerScore {
        uint32_t score = 0;
        uint16_t kills = 0;
        uint8_t deaths = 0;
        float comboMultiplier = 1.0f;
        float comboTimer = 0.0f;       // Reset combo if > 2s without kill
        float maxCombo = 1.0f;         // Best combo achieved this game
        bool tookDamageThisWave = false;

        // Kill streak tracking (consecutive kills without taking damage)
        uint16_t currentKillStreak = 0;
        uint16_t bestKillStreak = 0;

        // Wave streak tracking
        uint16_t currentWaveStreak = 0;   // Waves completed without dying (reset on death)
        uint16_t bestWaveStreak = 0;      // Best wave streak this game
        uint16_t perfectWaves = 0;        // Waves completed without taking any damage

        // Weapon-specific kills for leaderboard stats
        uint32_t standardKills = 0;
        uint32_t spreadKills = 0;
        uint32_t laserKills = 0;
        uint32_t missileKills = 0;
        uint32_t waveCannonKills = 0;  // Wave Cannon (charged beam) kills
        uint8_t bossKills = 0;

        // Damage tracking
        uint64_t totalDamageDealt = 0;    // Total damage dealt to enemies/boss

        // Game session tracking
        std::chrono::steady_clock::time_point gameStartTime;
        bool gameStarted = false;

        // Get combo as uint8_t (x10, e.g., 1.5x = 15)
        uint8_t getComboEncoded() const {
            return static_cast<uint8_t>(std::clamp(comboMultiplier * 10.0f, 10.0f, 30.0f));
        }

        // Get best combo as uint16_t (x10)
        uint16_t getMaxComboEncoded() const {
            return static_cast<uint16_t>(std::clamp(maxCombo * 10.0f, 10.0f, 100.0f));
        }

        // Get game duration in seconds
        uint32_t getGameDurationSeconds() const {
            if (!gameStarted) return 0;
            auto now = std::chrono::steady_clock::now();
            return static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(now - gameStartTime).count()
            );
        }

        void startGame() {
            if (!gameStarted) {
                gameStartTime = std::chrono::steady_clock::now();
                gameStarted = true;
            }
        }
    };

    // Point values per enemy type
    static constexpr uint16_t POINTS_BASIC = 100;
    static constexpr uint16_t POINTS_TRACKER = 150;
    static constexpr uint16_t POINTS_ZIGZAG = 120;
    static constexpr uint16_t POINTS_FAST = 180;
    static constexpr uint16_t POINTS_BOMBER = 250;
    static constexpr uint16_t POINTS_POW_ARMOR = 200;   // Special power-up carrier
    static constexpr uint16_t POINTS_BOSS = 5000;       // Boss kill bonus
    static constexpr uint16_t POINTS_WAVE_BONUS = 500;  // Bonus for completing wave without damage
    static constexpr float COMBO_GRACE_TIME = 3.0f;     // Grace period before decay starts
    static constexpr float COMBO_DECAY_RATE = 0.5f;    // -0.5x per second after grace period
    static constexpr float COMBO_INCREMENT = 0.1f;      // +0.1x per kill
    static constexpr float COMBO_MAX = 3.0f;            // Max 3.0x multiplier

    // Boss system constants
    static constexpr uint16_t BOSS_SPAWN_WAVE = 10;     // Spawn boss after this wave
    static constexpr uint16_t BOSS_MAX_HEALTH = 1500;   // Base HP (actual = base + 500×cycle + 1000×players)
    static constexpr uint16_t BOSS_HP_PER_PLAYER = 1000; // +1000 HP per player for multiplayer scaling
    static constexpr float BOSS_PHASE2_THRESHOLD = 0.65f;  // 65% HP (~975 HP)
    static constexpr float BOSS_PHASE3_THRESHOLD = 0.30f;  // 30% HP (~450 HP)

    // Boss phase enum
    enum class BossPhase : uint8_t {
        None = 0,
        Phase1 = 1,  // Aggressive entry + basic attacks
        Phase2 = 2,  // Summons minions + laser beams
        Phase3 = 3   // Enraged: all attacks + charge
    };

    // Boss attack state machine
    enum class BossAttack : uint8_t {
        Idle = 0,
        LinearShots,      // Basic horizontal missiles
        SpreadShot,       // Fan of missiles
        LaserCharge,      // Warning + devastating laser
        CircularBarrage,  // 360° missile burst
        DashCharge,       // Charge across screen
        SpawnMinions,     // Summon mini-enemies
        Teleport,         // Blink to new position
        HomingSwarm       // Multiple homing missiles
    };

    // Boss movement pattern
    enum class BossMovement : uint8_t {
        Idle = 0,
        Sinusoidal,       // Classic up/down wave
        Figure8,          // Figure-8 pattern
        Aggressive,       // Chase nearest player
        Retreat,          // Back away when low HP
        Charge            // Dash towards players
    };

    // Boss structure - EPIC VERSION
    struct Boss {
        uint16_t id = 9999;
        float x = 0.0f;
        float y = 0.0f;
        float baseX = 0.0f;           // Base position for patterns
        float baseY = 0.0f;
        float velocityX = 0.0f;       // Current velocity
        float velocityY = 0.0f;
        uint16_t maxHealth = BOSS_MAX_HEALTH;
        uint16_t health = BOSS_MAX_HEALTH;
        BossPhase phase = BossPhase::Phase1;
        BossAttack currentAttack = BossAttack::Idle;
        BossMovement movement = BossMovement::Sinusoidal;
        bool isActive = false;

        // Timers
        float phaseTimer = 0.0f;      // Time in current phase
        float attackTimer = 0.0f;     // Time since last attack
        float moveTimer = 0.0f;       // Movement animation timer
        float attackCooldown = 0.0f;  // Cooldown between attacks
        float stateTimer = 0.0f;      // Timer for current state (charge, laser, etc.)

        // Attack state
        bool isCharging = false;      // Charging up an attack
        bool isDashing = false;       // Currently dashing
        float chargeProgress = 0.0f;  // 0-1 charge progress
        float dashTargetX = 0.0f;     // Dash destination
        float dashTargetY = 0.0f;

        // Laser attack
        bool laserWarning = false;    // Show warning before laser
        float laserAngle = 0.0f;      // Laser aim direction
        float laserFireTimer = 0.0f;  // Timer for rapid laser projectile spawning

        // Minion spawning
        uint8_t minionsSpawned = 0;   // Count of minions spawned this phase
        float minionCooldown = 0.0f;  // Time until next minion spawn

        // Enrage mechanics
        bool isEnraged = false;       // Phase 3 enrage mode
        float enrageTimer = 0.0f;     // Time in enrage

        // Attack pattern queue
        uint8_t attackPatternIndex = 0;  // Current position in attack sequence

        static constexpr float WIDTH = 150.0f;
        static constexpr float HEIGHT = 120.0f;
        static constexpr float MOVE_SPEED = 150.0f;
        static constexpr float DASH_SPEED = 800.0f;
        static constexpr float CHARGE_TIME = 1.5f;      // Time to charge attack
        static constexpr float LASER_WARN_TIME = 1.0f;  // Warning before laser
        static constexpr float LASER_DURATION = 2.0f;   // Laser beam duration

        // Attack intervals per phase
        static constexpr float ATTACK_INTERVAL_P1 = 2.0f;
        static constexpr float ATTACK_INTERVAL_P2 = 1.5f;
        static constexpr float ATTACK_INTERVAL_P3 = 1.0f;

        // Minion spawn settings
        static constexpr uint8_t MAX_MINIONS_P2 = 4;
        static constexpr uint8_t MAX_MINIONS_P3 = 8;
        static constexpr float MINION_SPAWN_INTERVAL = 5.0f;
    };

    // ==========================================================================
    // R-Type Authentic Mechanics - Phase 3
    // ==========================================================================

    // Wave Cannon projectile (separate from regular missiles)
    struct WaveCannonProjectile {
        uint16_t id;
        uint8_t owner_id;
        float x;
        float y;
        float velocityX;
        uint8_t chargeLevel;  // 1-3
        uint8_t damage;
        float width;
        bool piercing;        // Level 2+ pierces through enemies

        // Track entities already damaged by this beam (prevents multi-frame hits)
        std::unordered_set<uint16_t> hitEnemies;
        bool hitBoss = false;

        static constexpr float SPEED = 900.0f;
        static constexpr float LENGTH = 60.0f;
    };

    // Power-up item on the field
    struct PowerUp {
        uint16_t id;
        float x;
        float y;
        PowerUpType type;
        float lifetime;       // Time until expiration

        static constexpr float MAX_LIFETIME = 10.0f;
        static constexpr float WIDTH = 24.0f;
        static constexpr float HEIGHT = 24.0f;
        static constexpr float DRIFT_SPEED = -30.0f;  // Slow drift left
    };

    // Force Pod structure
    struct ForcePod {
        uint8_t ownerId;
        float x;
        float y;
        float targetX;        // For smooth movement
        float targetY;
        bool isAttached;      // Attached to ship or free-floating
        uint8_t level;        // 1-2

        // Damage cooldown per enemy (prevents multi-frame damage)
        std::unordered_map<uint16_t, float> hitCooldowns;  // enemyId -> time remaining
        float bossHitCooldown = 0.0f;

        // R-Type Authentic: Force Pod shoots when player shoots
        float shootCooldown = 0.0f;  // Time until Force can shoot again

        static constexpr float WIDTH = 32.0f;
        static constexpr float HEIGHT = 32.0f;
        static constexpr float SPEED = 400.0f;
        static constexpr float ATTACH_OFFSET_X = 50.0f;
        static constexpr uint8_t CONTACT_DAMAGE = 30;   // Kills Fast (25 HP) in 1 hit
        static constexpr float HIT_COOLDOWN = 0.5f;     // Can hit same enemy again after 0.5s
        static constexpr float SHOOT_COOLDOWN = 0.35f;  // Slightly slower than player base cooldown
    };

    // Bit Device structure (R-Type Authentic - 2 orbiting satellites)
    struct BitDevice {
        uint8_t ownerId;
        uint8_t index;              // 0 = first bit, 1 = second bit
        float x;
        float y;
        bool isAttached = true;     // Orbiting around player
        float orbitAngle;           // Position in orbit (radians, 0-2π)
        float shootCooldown = 0.0f;

        // Damage cooldown per enemy (prevents multi-frame damage)
        std::unordered_map<uint16_t, float> hitCooldowns;
        float bossHitCooldown = 0.0f;

        static constexpr float WIDTH = 24.0f;
        static constexpr float HEIGHT = 24.0f;
        static constexpr float ORBIT_RADIUS = 50.0f;    // Distance from player center
        static constexpr float ORBIT_SPEED = 3.0f;      // Radians per second
        static constexpr float SHOOT_COOLDOWN = 0.4f;   // Slightly slower than player
        static constexpr uint8_t CONTACT_DAMAGE = 15;   // Less than Force Pod (30)
        static constexpr float HIT_COOLDOWN = 0.5f;
    };

    // Power-up drop rates (percentage)
    static constexpr uint8_t POWERUP_DROP_CHANCE = 8;        // 8% on regular enemy
    static constexpr uint8_t POWERUP_POW_ARMOR_CHANCE = 50;  // 50% on POW Armor

    // POW Armor spawn interval
    static constexpr float POW_ARMOR_SPAWN_INTERVAL = 25.0f;

    static constexpr uint8_t DEFAULT_HEALTH = 100;
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr float MISSILE_SPAWN_OFFSET_X = 64.0f;
    static constexpr float MISSILE_SPAWN_OFFSET_Y = 15.0f;

    // Player movement constants (server-authoritative)
    static constexpr float PLAYER_MOVE_SPEED = 200.0f;
    static constexpr float PLAYER_SHIP_WIDTH = 64.0f;
    static constexpr float PLAYER_SHIP_HEIGHT = 30.0f;

    static constexpr float WAVE_INTERVAL_MIN = 6.0f;
    static constexpr float WAVE_INTERVAL_MAX = 12.0f;
    static constexpr uint8_t ENEMIES_PER_WAVE_MIN = 2;
    static constexpr uint8_t ENEMIES_PER_WAVE_MAX = 6;
    static constexpr float SPAWN_X = 1950.0f;
    static constexpr float SPAWN_Y_MIN = 100.0f;
    static constexpr float SPAWN_Y_MAX = 900.0f;
    static constexpr float SPAWN_DELAY_MIN = 0.3f;
    static constexpr float SPAWN_DELAY_MAX = 1.2f;
    static constexpr uint8_t ENEMY_DAMAGE = 15;
    static constexpr uint8_t PLAYER_DAMAGE = 20;

    class GameWorld {
    public:
        // Constructor requires io_context to create the strand
        explicit GameWorld(boost::asio::io_context& io_ctx);

        // Get the strand for this GameWorld (used to serialize operations)
        boost::asio::strand<boost::asio::io_context::executor_type>& getStrand() {
            return _strand;
        }

        // ═══════════════════════════════════════════════════════════════════
        // Game Speed Configuration (per-room setting)
        // ═══════════════════════════════════════════════════════════════════

        // Set game speed from percentage (50-200, default 100)
        // This affects missiles, enemies, wave spawning, etc.
        void setGameSpeedPercent(uint16_t percent);
        uint16_t getGameSpeedPercent() const { return _gameSpeedPercent; }
        float getGameSpeedMultiplier() const { return _gameSpeedMultiplier; }

        // ═══════════════════════════════════════════════════════════════════
        // Player Management
        // ═══════════════════════════════════════════════════════════════════

        std::optional<uint8_t> addPlayer(const udp::endpoint& endpoint);
        void removePlayer(uint8_t playerId);
        void removePlayerByEndpoint(const udp::endpoint& endpoint);
        void setPlayerSkin(uint8_t playerId, uint8_t skinId);
        void setPlayerGodMode(uint8_t playerId, bool enabled);

        // ═══════════════════════════════════════════════════════════════════
        // Server-Authoritative Movement
        // ═══════════════════════════════════════════════════════════════════

        // Apply player input keys (called when PlayerInput message received)
        void applyPlayerInput(uint8_t playerId, uint16_t keys, uint16_t sequenceNum);

        // Get last acknowledged input sequence for a player (for client-side prediction)
        uint16_t getPlayerLastInputSeq(uint8_t playerId) const;

        // Update all player positions based on their inputs (called each tick)
        void updatePlayers(float deltaTime);

#ifdef USE_ECS_BACKEND
        // Phase 4.7: Run ECS systems as primary driver
        // This runs all ECS systems and syncs state back to legacy maps
        void runECSUpdate(float deltaTime);
#endif

        std::optional<uint8_t> getPlayerIdByEndpoint(const udp::endpoint& endpoint);
        std::optional<udp::endpoint> getEndpointByPlayerId(uint8_t playerId) const;

        GameSnapshot getSnapshot() const;
        std::vector<udp::endpoint> getAllEndpoints() const;
        size_t getPlayerCount() const;

        uint16_t spawnMissile(uint8_t playerId);
        void updateMissiles(float deltaTime);
        std::vector<uint16_t> getDestroyedMissiles();
        std::optional<Missile> getMissile(uint16_t missileId) const;

        void updateWaveSpawning(float deltaTime);
        void updateEnemies(float deltaTime);
        void checkCollisions();
        std::vector<uint16_t> getDestroyedEnemies();
        std::vector<std::pair<uint8_t, uint8_t>> getPlayerDamageEvents();
        std::vector<uint8_t> getDeadPlayers();

        bool isPlayerAlive(uint8_t playerId) const;

        void updatePlayerActivity(uint8_t playerId);
        std::vector<uint8_t> checkPlayerTimeouts(std::chrono::milliseconds timeout);

        // ═══════════════════════════════════════════════════════════════════
        // Score System (Gameplay Phase 2)
        // ═══════════════════════════════════════════════════════════════════

        void awardKillScore(uint8_t playerId, EnemyType enemyType, WeaponType weaponUsed = WeaponType::Standard);
        void updateComboTimers(float deltaTime);
        uint16_t getEnemyPointValue(EnemyType type) const;
        void onPlayerDamaged(uint8_t playerId);
        void onPlayerDied(uint8_t playerId);
        uint16_t getWaveNumber() const { return _waveNumber; }
        const PlayerScore& getPlayerScore(uint8_t playerId) const;

        // ═══════════════════════════════════════════════════════════════════
        // Boss System (Gameplay Phase 2) - EPIC VERSION
        // ═══════════════════════════════════════════════════════════════════

        void checkBossSpawn();
        void updateBoss(float deltaTime);
        void damageBoss(uint8_t damage, uint8_t playerId);
        bool hasBoss() const { return _boss.has_value() && _boss->isActive; }
        std::optional<Boss> getBoss() const { return _boss; }

        // Boss attack helpers
        void updateBossMovement(float deltaTime);
        void updateBossAttacks(float deltaTime);
        void selectNextBossAttack();
        void executeBossAttack(BossAttack attack);

        // Individual attack implementations
        void bossAttackLinearShots();
        void bossAttackSpreadShot();
        void bossAttackCircularBarrage();
        void bossAttackHomingSwarm();
        void bossAttackLaser(float deltaTime);
        void bossStartDash();
        void bossUpdateDash(float deltaTime);
        void bossSpawnMinions();
        void bossTeleport();

        // ═══════════════════════════════════════════════════════════════════
        // Weapon System (Gameplay Phase 2)
        // ═══════════════════════════════════════════════════════════════════

        void switchWeapon(uint8_t playerId, bool next);
        WeaponType getPlayerWeapon(uint8_t playerId) const;
        void updateShootCooldowns(float deltaTime);
        bool canPlayerShoot(uint8_t playerId) const;
        std::vector<uint16_t> spawnMissileWithWeapon(uint8_t playerId);  // Returns multiple IDs for spread

        // ═══════════════════════════════════════════════════════════════════
        // R-Type Authentic Mechanics - Phase 3
        // ═══════════════════════════════════════════════════════════════════

        // Wave Cannon (Charge Shot)
        void startCharging(uint8_t playerId);
        void updateCharging(uint8_t playerId, float deltaTime, bool fireHeld);
        void updateAllCharging(float deltaTime);  // Update all players' charge timers
        uint16_t releaseCharge(uint8_t playerId);  // Returns WaveCannon ID (0 if none created)
        uint16_t spawnWaveCannon(uint8_t playerId, uint8_t chargeLevel);
        void updateWaveCannons(float deltaTime);
        std::vector<uint16_t> getDestroyedWaveCannons();
        uint8_t getPlayerChargeLevel(uint8_t playerId) const;
        std::optional<WaveCannonProjectile> getWaveCannon(uint16_t id) const;
        const std::unordered_map<uint16_t, WaveCannonProjectile>& getWaveCannons() const { return _waveCannons; }

        // Power-up System
        void spawnPowerUp(float x, float y);
        void spawnPowerUp(float x, float y, PowerUpType type);
        void updatePowerUps(float deltaTime);
        void checkPowerUpCollisions();
        void applyPowerUp(uint8_t playerId, PowerUpType type);
        void spawnPOWArmor();
        std::vector<PowerUpCollected> getCollectedPowerUps();
        std::vector<uint16_t> getExpiredPowerUps();
        std::vector<uint16_t> getNewlySpawnedPowerUps();  // Power-ups created this tick
        std::optional<PowerUp> getPowerUp(uint16_t id) const;
        const std::unordered_map<uint16_t, PowerUp>& getPowerUps() const { return _powerUps; }

        // Force Pod System
        void giveForceToPlayer(uint8_t playerId);
        void toggleForceAttach(uint8_t playerId);
        void updateForcePods(float deltaTime);
        void checkForceCollisions();
        std::optional<ForcePod> getPlayerForce(uint8_t playerId) const;
        const std::unordered_map<uint8_t, ForcePod>& getForcePods() const { return _forcePods; }

        // Force Pod Shooting (R-Type authentic - Force shoots when player shoots)
        std::vector<uint16_t> spawnForceMissiles(uint8_t playerId);

        // Bit Device System (R-Type authentic - 2 orbiting satellites)
        void giveBitDevicesToPlayer(uint8_t playerId);
        void updateBitDevices(float deltaTime);
        void checkBitCollisions();
        bool playerHasBits(uint8_t playerId) const;
        const std::unordered_map<uint8_t, std::array<BitDevice, 2>>& getBitDevices() const { return _bitDevices; }
        std::vector<uint16_t> spawnBitMissiles(uint8_t playerId);

        // Speed helper
        float getPlayerMoveSpeed(uint8_t playerId) const;

    private:
        // Strand for serializing all operations on this GameWorld
        // All access to this GameWorld should go through this strand
        boost::asio::strand<boost::asio::io_context::executor_type> _strand;

        // Game speed configuration
        uint16_t _gameSpeedPercent = 100;      // 50-200, default 100%
        float _gameSpeedMultiplier = 1.0f;     // 0.5-2.0, derived from percent

        // PRNG for gameplay randomness only (enemy spawns, power-ups, patterns).
        // NOT used for security-sensitive operations (auth tokens, room codes).
        // std::mt19937 is acceptable here per SonarQube cpp:S2245 because:
        // 1. Predictable sequences don't provide gameplay advantage (server-side)
        // 2. No security implications - purely cosmetic/gameplay variation
        // 3. Performance matters for real-time game loop (60 FPS)
        // Security-sensitive RNG uses OpenSSL RAND_bytes (see SessionManager, RoomManager).
        mutable std::mt19937 _rng{std::random_device{}()};

        std::unordered_map<uint8_t, ConnectedPlayer> _players;
        std::unordered_map<uint8_t, uint16_t> _playerInputs;      // Player ID -> input keys bitfield
        std::unordered_map<uint8_t, uint16_t> _playerLastInputSeq; // Player ID -> last input sequence
        std::unordered_map<uint8_t, PlayerScore> _playerScores;   // Player ID -> score data
        std::unordered_map<uint16_t, Missile> _missiles;
        std::vector<uint16_t> _destroyedMissiles;
        uint8_t _nextPlayerId;
        uint16_t _nextMissileId = 1;

        std::unordered_map<uint16_t, Enemy> _enemies;
        std::unordered_map<uint16_t, Missile> _enemyMissiles;
        std::vector<uint16_t> _destroyedEnemies;
        std::vector<std::pair<uint8_t, uint8_t>> _playerDamageEvents;
        std::vector<uint8_t> _deadPlayers;
        float _waveTimer = 0.0f;
        float _currentWaveInterval = WAVE_INTERVAL_MIN;
        uint16_t _waveNumber = 0;
        uint16_t _nextEnemyId = 1;
        uint16_t _nextEnemyMissileId = 1;

        // Boss state
        std::optional<Boss> _boss;
        uint8_t _bossDefeatedCount = 0;  // Track boss defeats for recurring boss with +500 HP each

        std::vector<SpawnEntry> _spawnQueue;

        // R-Type Authentic Mechanics - Phase 3
        std::unordered_map<uint16_t, WaveCannonProjectile> _waveCannons;
        std::vector<uint16_t> _destroyedWaveCannons;
        uint16_t _nextWaveCannonId = 1;

        std::unordered_map<uint16_t, PowerUp> _powerUps;
        std::vector<PowerUpCollected> _collectedPowerUps;
        std::vector<uint16_t> _expiredPowerUps;
        std::vector<uint16_t> _newlySpawnedPowerUps;  // Power-ups created this tick
        uint16_t _nextPowerUpId = 1;
        float _powArmorSpawnTimer = 0.0f;

        std::unordered_map<uint8_t, ForcePod> _forcePods;  // Player ID -> Force Pod
        std::unordered_map<uint8_t, std::array<BitDevice, 2>> _bitDevices;  // Player ID -> 2 Bit Devices

        // ═══════════════════════════════════════════════════════════════════
        // ECS Infrastructure (Feature Flag)
        // ═══════════════════════════════════════════════════════════════════
#ifdef USE_ECS_BACKEND
        // Mapping from player ID to ECS EntityID (for entity lookup/deletion)
        std::unordered_map<uint8_t, ECS::EntityID> _playerEntityIds;
        // ECS Core (mutable to allow const methods like getSnapshot() to query entities)
        mutable ECS::ECS _ecs;

        // Domain Services (stateless, business logic)
        domain::services::GameRule _gameRule;
        domain::services::CollisionRule _collisionRule;
        domain::services::EnemyBehavior _enemyBehavior;

        // Bridge between ECS and Domain
        std::unique_ptr<ecs::bridge::DomainBridge> _domainBridge;

        // System IDs (for accessing systems later if needed)
        // Priorities: PlayerInput(0), EnemyAI(100), Weapon(200), Movement(300),
        //             Collision(400), Damage(500), Lifetime(600), Cleanup(700), Score(800)
        ECS::SystemID _playerInputSystemId = 0;
        ECS::SystemID _enemyAISystemId = 0;
        ECS::SystemID _weaponSystemId = 0;
        ECS::SystemID _movementSystemId = 0;
        ECS::SystemID _collisionSystemId = 0;
        ECS::SystemID _damageSystemId = 0;
        ECS::SystemID _lifetimeSystemId = 0;
        ECS::SystemID _cleanupSystemId = 0;
        ECS::SystemID _scoreSystemId = 0;

        // ECS initialization helpers
        void initializeECS();
        void registerSystems();

        // ECS entity creation helpers (Phase 4.2+)
        ECS::EntityID createPlayerEntity(uint8_t playerId, float x, float y, uint8_t health,
                                         uint8_t shipSkin = 1, bool godMode = false);
        void deletePlayerEntity(uint8_t playerId);

        // Missile entity creation (Phase 4.3)
        ECS::EntityID createMissileEntity(uint16_t missileId, uint8_t ownerId, float x, float y,
                                          float velX, float velY, uint8_t weaponType, uint8_t damage,
                                          bool isHoming = false, uint32_t targetId = 0);
        void deleteMissileEntity(uint16_t missileId);

        // Mapping from missile ID to ECS EntityID
        std::unordered_map<uint16_t, ECS::EntityID> _missileEntityIds;

        // Enemy entity creation (Phase 4.4)
        ECS::EntityID createEnemyEntity(uint16_t enemyId, float x, float y, uint8_t health,
                                        uint8_t enemyType, uint16_t points, float baseY,
                                        float phaseOffset, float shootCooldown, float shootInterval,
                                        float targetY = 0.0f, bool zigzagUp = true);
        void deleteEnemyEntity(uint16_t enemyId);

        // Mapping from enemy ID to ECS EntityID
        std::unordered_map<uint16_t, ECS::EntityID> _enemyEntityIds;

        // ═══════════════════════════════════════════════════════════════════
        // Phase 4.7: ECS as primary driver
        // ═══════════════════════════════════════════════════════════════════

        // Queue input to ECS PlayerInputSystem (called from applyPlayerInput)
        void queueInputToECS(uint8_t playerId, uint16_t keys, uint16_t sequenceNum);

        // Sync positions from ECS to legacy maps (temporary, until getSnapshot reads from ECS)
        void syncPlayersFromECS();
        void syncMissilesFromECS();
        void syncEnemiesFromECS();

        // Phase 5.1: Sync deleted entities from ECS to legacy maps
        // Called after ECS Update to detect entities deleted by CleanupSystem/LifetimeSystem
        void syncDeletedMissilesFromECS();
        void syncDeletedEnemiesFromECS();

        // Phase 5.3: Process kill events from DamageSystem
        // Awards score, spawns power-ups, marks enemies as destroyed
        void processECSKillEvents(const std::vector<ecs::systems::KillEvent>& killEvents);
#endif

        uint8_t findAvailableId() const;
        void spawnEnemy(float y, EnemyType type);
        void updateEnemyMovement(Enemy& enemy, float deltaTime);
        float getNearestPlayerY() const;

        // Force Pod shooting helpers
        uint16_t createForceMissile(uint8_t playerId, float x, float y,
                                    float speed, float angleY,
                                    WeaponType weapon, uint8_t level);
        uint16_t findNearestEnemy(float x, float y) const;

        // Bit Device shooting helpers
        uint16_t createBitMissile(uint8_t playerId, float x, float y,
                                   WeaponType weapon, uint8_t level);
    };
}

#endif /* !GAMEWORLD_HPP_ */
