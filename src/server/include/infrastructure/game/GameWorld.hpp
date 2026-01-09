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
#include <mutex>
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
    };

    struct Missile {
        uint16_t id;
        uint8_t owner_id;
        float x;
        float y;
        float velocityX;
        static constexpr float SPEED = 600.0f;
        static constexpr float WIDTH = 16.0f;
        static constexpr float HEIGHT = 8.0f;
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

    static constexpr uint8_t DEFAULT_HEALTH = 100;
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr float MISSILE_SPAWN_OFFSET_X = 64.0f;
    static constexpr float MISSILE_SPAWN_OFFSET_Y = 15.0f;

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
        GameWorld();

        std::optional<uint8_t> addPlayer(const udp::endpoint& endpoint);
        void removePlayer(uint8_t playerId);
        void removePlayerByEndpoint(const udp::endpoint& endpoint);

        void movePlayer(uint8_t playerId, uint16_t x, uint16_t y);
        std::optional<uint8_t> getPlayerIdByEndpoint(const udp::endpoint& endpoint);

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

    private:
        std::unordered_map<uint8_t, ConnectedPlayer> _players;
        std::unordered_map<uint16_t, Missile> _missiles;
        std::vector<uint16_t> _destroyedMissiles;
        mutable std::mutex _mutex;
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

        std::vector<SpawnEntry> _spawnQueue;

        uint8_t findAvailableId() const;
        void spawnEnemy(float y, EnemyType type);
        void updateEnemyMovement(Enemy& enemy, float deltaTime);
        float getNearestPlayerY() const;
    };
}

#endif /* !GAMEWORLD_HPP_ */
