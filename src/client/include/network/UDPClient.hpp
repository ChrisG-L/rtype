/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** UDPClient
*/

#ifndef UDPCLIENT_HPP_
#define UDPCLIENT_HPP_

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <optional>
#include <chrono>
#include <atomic>

#include "Protocol.hpp"
#include "NetworkEvents.hpp"

namespace client::network
{
    using boost::asio::ip::udp;

    struct NetworkPlayer {
        uint8_t id;
        uint16_t x;
        uint16_t y;
        uint8_t health;
        bool alive;
        uint16_t lastAckedInputSeq;  // For client-side prediction reconciliation
        uint8_t shipSkin;  // Ship skin variant (1-6)
        // Score system (Gameplay Phase 2)
        uint32_t score;   // Total score
        uint16_t kills;   // Total kills
        uint8_t combo;    // Combo multiplier (x10, e.g., 15 = 1.5x)
        // Weapon system (Gameplay Phase 2)
        uint8_t currentWeapon;  // WeaponType enum
        // R-Type Authentic (Phase 3)
        uint8_t chargeLevel = 0;  // Wave Cannon charge level (0-3)
        uint8_t speedLevel = 0;   // Speed upgrade level (0-3)
        uint8_t weaponLevel = 0;  // Weapon upgrade level (0-3) for damage/cooldown bonus
        uint8_t hasForce = 0;     // 1 if player has Force Pod
        uint8_t shieldTimer = 0;  // Shield remaining (tenths of seconds)
    };

    struct NetworkMissile {
        uint16_t id;
        uint8_t owner_id;
        uint16_t x;
        uint16_t y;
        uint8_t weapon_type;  // WeaponType enum (Gameplay Phase 2)
    };

    struct NetworkEnemy {
        uint16_t id;
        uint16_t x;
        uint16_t y;
        uint8_t health;
        uint8_t enemy_type;
    };

    // Boss state (Gameplay Phase 2)
    struct NetworkBoss {
        uint16_t id;
        uint16_t x;
        uint16_t y;
        uint16_t max_health;
        uint16_t health;
        uint8_t phase;
        bool is_active;
    };

    // Wave Cannon projectile (R-Type Authentic - Phase 3)
    struct NetworkWaveCannon {
        uint16_t id;
        uint8_t owner_id;
        uint16_t x;
        uint16_t y;
        uint8_t charge_level;  // 1-3
        uint8_t width;         // Beam width for rendering
        std::chrono::steady_clock::time_point spawnTime;  // For auto-expiration

        static constexpr float BEAM_LIFETIME_MS = 300.0f;  // Beam visible for 300ms
    };

    // Power-up item on the field (R-Type Authentic - Phase 3)
    struct NetworkPowerUp {
        uint16_t id;
        float x;                // Float for smooth drift simulation
        float y;
        uint8_t type;           // PowerUpType enum
        float remaining_time;   // Seconds before expiration (float for smooth countdown)

        static constexpr float DRIFT_SPEED = -30.0f;  // Must match server PowerUp::DRIFT_SPEED
    };

    // Force Pod state (R-Type Authentic - Phase 3)
    struct NetworkForce {
        uint8_t owner_id;
        uint16_t x;
        uint16_t y;
        bool is_attached;
        uint8_t level;  // 0 = no force, 1-2 = force level
    };

    // Bit Device state (R-Type Authentic - 2 orbiting satellites)
    struct NetworkBit {
        uint8_t owner_id;
        uint8_t bit_index;  // 0 or 1
        uint16_t x;
        uint16_t y;
        bool is_attached;
    };

    class UDPClient
    {
    public:
        using OnConnectedCallback = std::function<void()>;
        using OnDisconnectedCallback = std::function<void()>;
        using OnReceiveCallback = std::function<void(const std::string &)>;
        using OnErrorCallback = std::function<void(const std::string &)>;
        using OnSnapshotCallback = std::function<void(const std::vector<NetworkPlayer>&)>;
        using OnMissileSpawnedCallback = std::function<void(const NetworkMissile&)>;
        using OnMissileDestroyedCallback = std::function<void(uint16_t)>;
        using OnPlayerDiedCallback = std::function<void(uint8_t)>;

        UDPClient();
        ~UDPClient();

        void connect(const std::string &host, std::uint16_t port);
        void disconnect();
        bool isConnected() const;
        bool isConnecting() const;

        // Get last connection info (for reconnection)
        const std::string& getLastHost() const { return _lastHost; }
        std::uint16_t getLastPort() const { return _lastPort; }

        void send(const std::string &message);

        void setOnConnected(const OnConnectedCallback& callback);
        void setOnDisconnected(const OnDisconnectedCallback& callback);
        void setOnReceive(const OnReceiveCallback& callback);
        void setOnError(const OnErrorCallback& callback);
        void setOnSnapshot(const OnSnapshotCallback& callback);
        void setOnMissileSpawned(const OnMissileSpawnedCallback& callback);
        void setOnMissileDestroyed(const OnMissileDestroyedCallback& callback);
        void setOnPlayerDied(const OnPlayerDiedCallback& callback);

        // Server-authoritative: Send input keys with sequence number
        void sendPlayerInput(uint16_t keys, uint16_t sequenceNum);

        void shootMissile();
        void joinGame(const SessionToken& token, const std::string& roomCode);

        std::optional<uint8_t> getLocalPlayerId() const;
        std::vector<NetworkPlayer> getPlayers() const;
        std::vector<NetworkMissile> getMissiles() const;
        std::vector<NetworkEnemy> getEnemies() const;
        std::vector<NetworkMissile> getEnemyMissiles() const;
        bool isLocalPlayerDead() const;
        uint16_t getWaveNumber() const;
        std::optional<NetworkBoss> getBossState() const;

        // R-Type Authentic (Phase 3) getters
        std::vector<NetworkWaveCannon> getWaveCannons();  // Non-const: auto-cleans expired beams
        std::vector<NetworkPowerUp> getPowerUps() const;
        std::vector<NetworkForce> getForcePods() const;
        std::vector<NetworkBit> getBitDevices() const;
        uint8_t getLocalPlayerChargeLevel() const;

        // Power-up client-side simulation (drift + expiration)
        void updatePowerUps(float deltaTime);

        // R-Type Authentic (Phase 3) actions
        void startCharging();      // Begin charging Wave Cannon
        void releaseCharge();      // Fire charged Wave Cannon
        void toggleForce();        // Attach/detach Force Pod

        // Pause system
        void sendPauseRequest(bool wantsPause);

        // Event queue for thread-safe event polling
        std::optional<UDPEvent> pollEvent();
        void clearEventQueue();

    private:
        void asyncReceiveFrom();
        void asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize);

        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        void handleWrite(const boost::system::error_code &error);

        void handlePlayerJoin(const uint8_t* payload, size_t size);
        void handlePlayerLeave(const uint8_t* payload, size_t size);
        void handleSnapshot(const uint8_t* payload, size_t size);
        void handleMissileSpawned(const uint8_t* payload, size_t size);
        void handleMissileDestroyed(const uint8_t* payload, size_t size);
        void handleEnemyDestroyed(const uint8_t* payload, size_t size);
        void handlePlayerDied(const uint8_t* payload, size_t size);
        void handlePlayerDamaged(const uint8_t* payload, size_t size);

        // R-Type Authentic (Phase 3) handlers
        void handleWaveCannonFired(const uint8_t* payload, size_t size);
        void handlePowerUpSpawned(const uint8_t* payload, size_t size);
        void handlePowerUpCollected(const uint8_t* payload, size_t size);
        void handlePowerUpExpired(const uint8_t* payload, size_t size);
        void handleForceStateUpdate(const uint8_t* payload, size_t size);

        // Pause system handler
        void handlePauseStateSync(const uint8_t* payload, size_t size);

        void scheduleHeartbeat();
        void sendHeartbeat();

        boost::asio::io_context _ioContext;
        udp::socket _socket;
        std::jthread _ioThread;
        udp::endpoint _endpoint;

        boost::asio::steady_timer _heartbeatTimer;
        std::chrono::steady_clock::time_point _lastServerResponse;
        mutable std::mutex _heartbeatMutex;

        std::atomic<bool> _connected{false};
        std::atomic<bool> _connecting{false};  // Waiting for HeartBeatAck
        std::atomic<bool> _disconnecting{false};
        mutable std::mutex _mutex;

        std::array<char, 1> _sendBuf;
        bool _isWriting;

        char _readBuffer[BUFFER_SIZE];
        std::vector<uint8_t> _accumulator;

        std::optional<uint8_t> _localPlayerId;
        std::vector<NetworkPlayer> _players;
        mutable std::mutex _playersMutex;
        bool _isLocalPlayerDead = false;

        std::vector<NetworkMissile> _missiles;
        mutable std::mutex _missilesMutex;

        std::vector<NetworkEnemy> _enemies;
        mutable std::mutex _enemiesMutex;

        std::vector<NetworkMissile> _enemyMissiles;
        mutable std::mutex _enemyMissilesMutex;

        uint16_t _waveNumber = 0;
        mutable std::mutex _waveNumberMutex;

        std::optional<NetworkBoss> _bossState;
        mutable std::mutex _bossMutex;

        // R-Type Authentic (Phase 3) state
        std::vector<NetworkWaveCannon> _waveCannons;
        mutable std::mutex _waveCannonsMutex;

        std::vector<NetworkPowerUp> _powerUps;
        mutable std::mutex _powerUpsMutex;

        std::vector<NetworkForce> _forcePods;
        mutable std::mutex _forcePodsMutex;

        std::vector<NetworkBit> _bitDevices;
        mutable std::mutex _bitDevicesMutex;

        uint8_t _localChargeLevel = 0;  // Client-side charge level tracking
        bool _isCharging = false;
        mutable std::mutex _chargeMutex;

        OnConnectedCallback _onConnected;
        OnDisconnectedCallback _onDisconnected;
        OnReceiveCallback _onReceive;
        OnErrorCallback _onError;
        OnSnapshotCallback _onSnapshot;
        OnMissileSpawnedCallback _onMissileSpawned;
        OnMissileDestroyedCallback _onMissileDestroyed;
        OnPlayerDiedCallback _onPlayerDied;

        // Last connection info (for reconnection)
        std::string _lastHost;
        std::uint16_t _lastPort = 0;

        // Event queue for thread-safe main thread communication
        EventQueue<UDPEvent> _eventQueue;
    };

}

#endif /* !UDPCLIENT_HPP_ */
