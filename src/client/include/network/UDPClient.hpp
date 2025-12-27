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
    };

    struct NetworkMissile {
        uint16_t id;
        uint8_t owner_id;
        uint16_t x;
        uint16_t y;
    };

    struct NetworkEnemy {
        uint16_t id;
        uint16_t x;
        uint16_t y;
        uint8_t health;
        uint8_t enemy_type;
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

        void send(const std::string &message);

        void setOnConnected(const OnConnectedCallback& callback);
        void setOnDisconnected(const OnDisconnectedCallback& callback);
        void setOnReceive(const OnReceiveCallback& callback);
        void setOnError(const OnErrorCallback& callback);
        void setOnSnapshot(const OnSnapshotCallback& callback);
        void setOnMissileSpawned(const OnMissileSpawnedCallback& callback);
        void setOnMissileDestroyed(const OnMissileDestroyedCallback& callback);
        void setOnPlayerDied(const OnPlayerDiedCallback& callback);

        void movePlayer(uint16_t x, uint16_t y);
        void shootMissile();

        std::optional<uint8_t> getLocalPlayerId() const;
        std::vector<NetworkPlayer> getPlayers() const;
        std::vector<NetworkMissile> getMissiles() const;
        std::vector<NetworkEnemy> getEnemies() const;
        std::vector<NetworkMissile> getEnemyMissiles() const;
        bool isLocalPlayerDead() const;

        // Event queue for thread-safe event polling
        std::optional<UDPEvent> pollEvent();

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
        void handlePlayerDied(const uint8_t* payload, size_t size);

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

        OnConnectedCallback _onConnected;
        OnDisconnectedCallback _onDisconnected;
        OnReceiveCallback _onReceive;
        OnErrorCallback _onError;
        OnSnapshotCallback _onSnapshot;
        OnMissileSpawnedCallback _onMissileSpawned;
        OnMissileDestroyedCallback _onMissileDestroyed;
        OnPlayerDiedCallback _onPlayerDied;

        // Event queue for thread-safe main thread communication
        EventQueue<UDPEvent> _eventQueue;
    };

}

#endif /* !UDPCLIENT_HPP_ */
