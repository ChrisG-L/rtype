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
#include <optional>
#include <vector>
#include <chrono>

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
    };

    struct Missile {
        uint16_t id;
        uint8_t owner_id;
        float x;
        float y;
        float velocityX;
        float velocityY = 0.0f;  // For spread/homing shots
        WeaponType weaponType = WeaponType::Standard;
        uint16_t targetEnemyId = 0;  // For homing missiles
        static constexpr float SPEED = 600.0f;
        static constexpr float WIDTH = 16.0f;
        static constexpr float HEIGHT = 8.0f;

        // Weapon-specific speeds
        static constexpr float SPEED_STANDARD = 600.0f;
        static constexpr float SPEED_SPREAD = 500.0f;
        static constexpr float SPEED_LASER = 1000.0f;
        static constexpr float SPEED_MISSILE = 300.0f;

        // Weapon-specific damage (applied in collision)
        static constexpr uint8_t DAMAGE_STANDARD = 15;
        static constexpr uint8_t DAMAGE_SPREAD = 10;    // Less damage but 3 shots
        static constexpr uint8_t DAMAGE_LASER = 20;     // More damage, faster
        static constexpr uint8_t DAMAGE_MISSILE = 30;   // High damage, slow, homing

        // Weapon-specific cooldowns
        static constexpr float COOLDOWN_STANDARD = 0.3f;
        static constexpr float COOLDOWN_SPREAD = 0.5f;   // Slower because 3 shots
        static constexpr float COOLDOWN_LASER = 0.15f;   // Rapid fire
        static constexpr float COOLDOWN_MISSILE = 1.0f;  // Slow reload

        static float getSpeed(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return SPEED_SPREAD;
                case WeaponType::Laser: return SPEED_LASER;
                case WeaponType::Missile: return SPEED_MISSILE;
                default: return SPEED_STANDARD;
            }
        }

        static uint8_t getDamage(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return DAMAGE_SPREAD;
                case WeaponType::Laser: return DAMAGE_LASER;
                case WeaponType::Missile: return DAMAGE_MISSILE;
                default: return DAMAGE_STANDARD;
            }
        }

        static float getCooldown(WeaponType type) {
            switch (type) {
                case WeaponType::Spread: return COOLDOWN_SPREAD;
                case WeaponType::Laser: return COOLDOWN_LASER;
                case WeaponType::Missile: return COOLDOWN_MISSILE;
                default: return COOLDOWN_STANDARD;
            }
        }
    };

    enum class EnemyType : uint8_t {
        Basic = 0,
        Tracker = 1,
        Zigzag = 2,
        Fast = 3,
        Bomber = 4
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

        static constexpr uint8_t HEALTH_BASIC = 30;
        static constexpr uint8_t HEALTH_TRACKER = 25;
        static constexpr uint8_t HEALTH_ZIGZAG = 20;
        static constexpr uint8_t HEALTH_FAST = 15;
        static constexpr uint8_t HEALTH_BOMBER = 50;

        static constexpr float WIDTH = 40.0f;
        static constexpr float HEIGHT = 40.0f;

        float getSpeedX() const {
            switch (static_cast<EnemyType>(enemy_type)) {
                case EnemyType::Tracker: return SPEED_X_TRACKER;
                case EnemyType::Zigzag: return SPEED_X_ZIGZAG;
                case EnemyType::Fast: return SPEED_X_FAST;
                case EnemyType::Bomber: return SPEED_X_BOMBER;
                default: return SPEED_X_BASIC;
            }
        }

        float getShootInterval() const {
            switch (static_cast<EnemyType>(enemy_type)) {
                case EnemyType::Tracker: return SHOOT_INTERVAL_TRACKER;
                case EnemyType::Zigzag: return SHOOT_INTERVAL_ZIGZAG;
                case EnemyType::Fast: return SHOOT_INTERVAL_FAST;
                case EnemyType::Bomber: return SHOOT_INTERVAL_BOMBER;
                default: return SHOOT_INTERVAL_BASIC;
            }
        }

        static uint8_t getHealthForType(EnemyType type) {
            switch (type) {
                case EnemyType::Tracker: return HEALTH_TRACKER;
                case EnemyType::Zigzag: return HEALTH_ZIGZAG;
                case EnemyType::Fast: return HEALTH_FAST;
                case EnemyType::Bomber: return HEALTH_BOMBER;
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
        float comboMultiplier = 1.0f;
        float comboTimer = 0.0f;       // Reset combo if > 2s without kill
        bool tookDamageThisWave = false;

        // Get combo as uint8_t (x10, e.g., 1.5x = 15)
        uint8_t getComboEncoded() const {
            return static_cast<uint8_t>(std::clamp(comboMultiplier * 10.0f, 10.0f, 30.0f));
        }
    };

    // Point values per enemy type
    static constexpr uint16_t POINTS_BASIC = 100;
    static constexpr uint16_t POINTS_TRACKER = 150;
    static constexpr uint16_t POINTS_ZIGZAG = 120;
    static constexpr uint16_t POINTS_FAST = 180;
    static constexpr uint16_t POINTS_BOMBER = 250;
    static constexpr uint16_t POINTS_BOSS = 5000;       // Boss kill bonus
    static constexpr uint16_t POINTS_WAVE_BONUS = 500;  // Bonus for completing wave without damage
    static constexpr float COMBO_DECAY_TIME = 2.0f;     // Reset combo after 2s without kill
    static constexpr float COMBO_INCREMENT = 0.1f;      // +0.1x per kill
    static constexpr float COMBO_MAX = 3.0f;            // Max 3.0x multiplier

    // Boss system constants
    static constexpr uint16_t BOSS_SPAWN_WAVE = 10;     // Spawn boss after this wave
    static constexpr uint16_t BOSS_MAX_HEALTH = 500;    // Increased HP for epic fight
    static constexpr float BOSS_PHASE2_THRESHOLD = 0.65f;  // 65% HP
    static constexpr float BOSS_PHASE3_THRESHOLD = 0.30f;  // 30% HP

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

        // ═══════════════════════════════════════════════════════════════════
        // Server-Authoritative Movement
        // ═══════════════════════════════════════════════════════════════════

        // Apply player input keys (called when PlayerInput message received)
        void applyPlayerInput(uint8_t playerId, uint16_t keys, uint16_t sequenceNum);

        // Get last acknowledged input sequence for a player (for client-side prediction)
        uint16_t getPlayerLastInputSeq(uint8_t playerId) const;

        // Update all player positions based on their inputs (called each tick)
        void updatePlayers(float deltaTime);

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

        void awardKillScore(uint8_t playerId, EnemyType enemyType);
        void updateComboTimers(float deltaTime);
        uint16_t getEnemyPointValue(EnemyType type) const;
        void onPlayerDamaged(uint8_t playerId);
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

    private:
        // Strand for serializing all operations on this GameWorld
        // All access to this GameWorld should go through this strand
        boost::asio::strand<boost::asio::io_context::executor_type> _strand;

        // Game speed configuration
        uint16_t _gameSpeedPercent = 100;      // 50-200, default 100%
        float _gameSpeedMultiplier = 1.0f;     // 0.5-2.0, derived from percent

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

        std::vector<SpawnEntry> _spawnQueue;

        uint8_t findAvailableId() const;
        void spawnEnemy(float y, EnemyType type);
        void updateEnemyMovement(Enemy& enemy, float deltaTime);
        float getNearestPlayerY() const;
    };
}

#endif /* !GAMEWORLD_HPP_ */
