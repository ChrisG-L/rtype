/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** UDPClient
*/

#include "network/UDPClient.hpp"
#include "Protocol.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef _WIN32
    #include <unistd.h>
#else
    #include <winsock2.h>
    #include <mstcpip.h>
#endif

namespace client::network
{
    static constexpr int HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr int HEARTBEAT_TIMEOUT_MS = 5000;

    UDPClient::UDPClient()
        : _socket(_ioContext), _heartbeatTimer(_ioContext), _isWriting(false), _localPlayerId(std::nullopt)
    {
        client::logging::Logger::getNetworkLogger()->debug("UDPClient created");
    }

    UDPClient::~UDPClient()
    {
        disconnect();
        if (_ioThread.joinable()) {
            _ioThread.join();
        }
    }

    void UDPClient::setOnConnected(const OnConnectedCallback& callback)
    {
        _onConnected = callback;
    }

    void UDPClient::setOnDisconnected(const OnDisconnectedCallback& callback)
    {
        _onDisconnected = callback;
    }

    void UDPClient::setOnReceive(const OnReceiveCallback& callback)
    {
        _onReceive = callback;
    }

    void UDPClient::setOnError(const OnErrorCallback& callback)
    {
        _onError = callback;
    }

    void UDPClient::setOnSnapshot(const OnSnapshotCallback& callback)
    {
        _onSnapshot = callback;
    }

    void UDPClient::setOnMissileSpawned(const OnMissileSpawnedCallback& callback)
    {
        _onMissileSpawned = callback;
    }

    void UDPClient::setOnMissileDestroyed(const OnMissileDestroyedCallback& callback)
    {
        _onMissileDestroyed = callback;
    }

    void UDPClient::setOnPlayerDied(const OnPlayerDiedCallback& callback)
    {
        _onPlayerDied = callback;
    }

    void UDPClient::connect(const std::string &host, std::uint16_t port)
    {
        auto logger = client::logging::Logger::getNetworkLogger();

        // Close any existing connection (connected or connecting)
        if (_connected.load() || _connecting.load()) {
            logger->warn("Already connected or connecting, disconnecting...");
            disconnect();
        }

        // Wait for previous IO thread to finish
        if (_ioThread.joinable()) {
            _ioThread.join();
        }

        // Ensure socket is closed before reopening
        if (_socket.is_open()) {
            boost::system::error_code ec;
            _socket.close(ec);
        }

        // Reset io_context for reuse
        _ioContext.restart();

        // Store connection info for potential reconnection
        _lastHost = host;
        _lastPort = port;

        logger->info("Connecting to {}:{}...", host, port);

        try {
            udp::resolver resolver(_ioContext);
            auto results = resolver.resolve(host, std::to_string(port));
            _endpoint = *results.begin();

            _socket.open(udp::v4());

            // Bind à un port local (nécessaire sur Windows avant async_receive_from)
            _socket.bind(udp::endpoint(udp::v4(), 0));

            // Windows: désactiver ICMP Port Unreachable qui cause des erreurs sur UDP
            #ifdef _WIN32
                BOOL bNewBehavior = FALSE;
                DWORD dwBytesReturned = 0;
                WSAIoctl(
                    _socket.native_handle(), SIO_UDP_CONNRESET,
                    &bNewBehavior, sizeof(bNewBehavior),
                    NULL, 0, &dwBytesReturned, NULL, NULL
                );
            #endif

            _disconnecting.store(false);
            _connecting.store(true);  // Wait for HeartBeatAck to confirm connection

            asyncReceiveFrom();

            _ioThread = std::jthread([this, logger]() {
                logger->debug("IO thread started");
                _ioContext.run();
                logger->debug("IO thread terminated");
            });

            logger->info("Waiting for server response UDP at {}:{}...",
                        _endpoint.address().to_string(), _endpoint.port());

            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                _lastServerResponse = std::chrono::steady_clock::now();
            }

            // Send initial heartbeat to verify server is reachable
            sendHeartbeat();
            scheduleHeartbeat();

            // Note: _onConnected and connected event will be triggered
            // when we receive HeartBeatAck in handleRead()

        } catch (const std::exception &e) {
            logger->error("Connection error: {}", e.what());
            _eventQueue.push(UDPErrorEvent{std::string("Connexion echouee: ") + e.what()});
            if (_onError) {
                _onError(std::string("Connexion echouee: ") + e.what());
            }
        }
    }

    void UDPClient::disconnect()
    {
        // Prevent multiple disconnect calls
        if (_disconnecting.exchange(true)) {
            return;
        }

        // Check if we were connected or connecting
        bool wasConnected = _connected.load();
        bool wasConnecting = _connecting.load();

        if (!wasConnected && !wasConnecting) {
            _disconnecting.store(false);
            return;
        }

        _connecting.store(false);

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Disconnecting UDP...");

        _connected.store(false);

        _heartbeatTimer.cancel();

        _ioContext.stop();

        boost::system::error_code ec;
        if (_socket.is_open()) {
            _socket.close(ec);
        }

        {
            std::lock_guard<std::mutex> plock(_playersMutex);
            _localPlayerId = std::nullopt;
            _players.clear();
            _isLocalPlayerDead = false;
        }
        {
            std::lock_guard<std::mutex> mlock(_missilesMutex);
            _missiles.clear();
        }
        {
            std::lock_guard<std::mutex> elock(_enemiesMutex);
            _enemies.clear();
        }
        {
            std::lock_guard<std::mutex> emlock(_enemyMissilesMutex);
            _enemyMissiles.clear();
        }

        _accumulator.clear();

        // Only push disconnected event if we were actually connected
        // (not just "connecting" waiting for HeartBeatAck)
        if (wasConnected) {
            _eventQueue.push(UDPDisconnectedEvent{});
            if (_onDisconnected) {
                _onDisconnected();
            }
        }

        logger->info("UDP disconnected successfully");
    }

    bool UDPClient::isConnected() const
    {
        return _connected.load() && _socket.is_open();
    }

    bool UDPClient::isConnecting() const
    {
        return _connecting.load() && _socket.is_open();
    }

    std::optional<uint8_t> UDPClient::getLocalPlayerId() const
    {
        std::lock_guard<std::mutex> lock(_playersMutex);
        return _localPlayerId;
    }

    std::vector<NetworkPlayer> UDPClient::getPlayers() const
    {
        std::lock_guard<std::mutex> lock(_playersMutex);
        return _players;
    }

    std::vector<NetworkMissile> UDPClient::getMissiles() const
    {
        std::lock_guard<std::mutex> lock(_missilesMutex);
        return _missiles;
    }

    std::vector<NetworkEnemy> UDPClient::getEnemies() const
    {
        std::lock_guard<std::mutex> lock(_enemiesMutex);
        return _enemies;
    }

    std::vector<NetworkMissile> UDPClient::getEnemyMissiles() const
    {
        std::lock_guard<std::mutex> lock(_enemyMissilesMutex);
        return _enemyMissiles;
    }

    bool UDPClient::isLocalPlayerDead() const
    {
        std::lock_guard<std::mutex> lock(_playersMutex);
        return _isLocalPlayerDead;
    }

    void UDPClient::asyncReceiveFrom()
    {
        // Check socket is open (works for both connected and connecting states)
        if (!_socket.is_open()) {
            return;
        }
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _endpoint,
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }

    void UDPClient::asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize) {
        // Check socket is open (works for both connected and connecting states)
        if (!_socket.is_open()) {
            client::logging::Logger::getNetworkLogger()->warn("Socket not open");
            return;
        }
        _socket.async_send_to(
            boost::asio::buffer(buf->data(), totalSize),
            _endpoint,
            [buf](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    client::logging::Logger::getNetworkLogger()->error("UDP write error: {}", error.message());
                }
            }
        );
    }

    void UDPClient::handlePlayerJoin(const uint8_t* payload, size_t size) {
        auto pjOpt = PlayerJoin::from_bytes(payload, size);
        if (!pjOpt) return;

        std::lock_guard<std::mutex> lock(_playersMutex);
        _localPlayerId = pjOpt->player_id;

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Joined game as player {}", static_cast<int>(*_localPlayerId));

        // Note: UDPConnectedEvent is pushed when HeartBeatAck is received
        // PlayerJoin just updates the player ID
        _eventQueue.push(UDPPlayerJoinedEvent{pjOpt->player_id});
    }

    void UDPClient::handlePlayerLeave(const uint8_t* payload, size_t size) {
        auto plOpt = PlayerLeave::from_bytes(payload, size);
        if (!plOpt) return;

        std::lock_guard<std::mutex> lock(_playersMutex);
        _players.erase(
            std::remove_if(_players.begin(), _players.end(),
                [&](const NetworkPlayer& p) { return p.id == plOpt->player_id; }),
            _players.end()
        );

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Player {} left the game", static_cast<int>(plOpt->player_id));

        _eventQueue.push(UDPPlayerLeftEvent{plOpt->player_id});
    }

    void UDPClient::handleSnapshot(const uint8_t* payload, size_t size) {
        auto gsOpt = GameSnapshot::from_bytes(payload, size);
        if (!gsOpt) return;

        std::vector<NetworkPlayer> newPlayers;
        newPlayers.reserve(gsOpt->player_count);

        for (uint8_t i = 0; i < gsOpt->player_count; ++i) {
            const auto& ps = gsOpt->players[i];
            newPlayers.push_back(NetworkPlayer{
                .id = ps.id,
                .x = ps.x,
                .y = ps.y,
                .health = ps.health,
                .alive = ps.alive != 0
            });
        }

        std::vector<NetworkMissile> newMissiles;
        newMissiles.reserve(gsOpt->missile_count);

        for (uint8_t i = 0; i < gsOpt->missile_count; ++i) {
            const auto& ms = gsOpt->missiles[i];
            newMissiles.push_back(NetworkMissile{
                .id = ms.id,
                .owner_id = ms.owner_id,
                .x = ms.x,
                .y = ms.y
            });
        }

        std::vector<NetworkEnemy> newEnemies;
        newEnemies.reserve(gsOpt->enemy_count);

        for (uint8_t i = 0; i < gsOpt->enemy_count; ++i) {
            const auto& es = gsOpt->enemies[i];
            newEnemies.push_back(NetworkEnemy{
                .id = es.id,
                .x = es.x,
                .y = es.y,
                .health = es.health,
                .enemy_type = es.enemy_type
            });
        }

        std::vector<NetworkMissile> newEnemyMissiles;
        newEnemyMissiles.reserve(gsOpt->enemy_missile_count);

        for (uint8_t i = 0; i < gsOpt->enemy_missile_count; ++i) {
            const auto& ms = gsOpt->enemy_missiles[i];
            newEnemyMissiles.push_back(NetworkMissile{
                .id = ms.id,
                .owner_id = ms.owner_id,
                .x = ms.x,
                .y = ms.y
            });
        }

        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _players = std::move(newPlayers);
        }
        {
            std::lock_guard<std::mutex> lock(_missilesMutex);
            _missiles = std::move(newMissiles);
        }
        {
            std::lock_guard<std::mutex> lock(_enemiesMutex);
            _enemies = std::move(newEnemies);
        }
        {
            std::lock_guard<std::mutex> lock(_enemyMissilesMutex);
            _enemyMissiles = std::move(newEnemyMissiles);
        }

        if (_onSnapshot) {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _onSnapshot(_players);
        }
    }

    void UDPClient::handleMissileSpawned(const uint8_t* payload, size_t size) {
        auto msOpt = MissileSpawned::from_bytes(payload, size);
        if (!msOpt) return;

        NetworkMissile missile{
            .id = msOpt->missile_id,
            .owner_id = msOpt->owner_id,
            .x = msOpt->x,
            .y = msOpt->y
        };

        {
            std::lock_guard<std::mutex> lock(_missilesMutex);
            _missiles.push_back(missile);
        }

        if (_onMissileSpawned) {
            _onMissileSpawned(missile);
        }
    }

    void UDPClient::handleMissileDestroyed(const uint8_t* payload, size_t size) {
        auto mdOpt = MissileDestroyed::from_bytes(payload, size);
        if (!mdOpt) return;

        {
            std::lock_guard<std::mutex> lock(_missilesMutex);
            _missiles.erase(
                std::remove_if(_missiles.begin(), _missiles.end(),
                    [&](const NetworkMissile& m) { return m.id == mdOpt->missile_id; }),
                _missiles.end()
            );
        }

        if (_onMissileDestroyed) {
            _onMissileDestroyed(mdOpt->missile_id);
        }
    }

    void UDPClient::handlePlayerDied(const uint8_t* payload, size_t size) {
        auto pdOpt = PlayerDied::from_bytes(payload, size);
        if (!pdOpt) return;

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Player {} died", static_cast<int>(pdOpt->player_id));

        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            if (_localPlayerId && *_localPlayerId == pdOpt->player_id) {
                _isLocalPlayerDead = true;
                logger->info("Local player died!");
            }
        }

        if (_onPlayerDied) {
            _onPlayerDied(pdOpt->player_id);
        }
    }

    void UDPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        // Ignore operation_aborted - expected when intentionally disconnecting
        if (error == boost::asio::error::operation_aborted) {
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();
        if (!error && bytes > 0) {
            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                _lastServerResponse = std::chrono::steady_clock::now();
            }

            if (bytes >= UDPHeader::WIRE_SIZE) {
                auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
                if (!headOpt) {
                    asyncReceiveFrom();
                    return;
                }
                UDPHeader head = *headOpt;
                size_t payload_size = bytes - UDPHeader::WIRE_SIZE;
                const uint8_t* payload = reinterpret_cast<const uint8_t*>(_readBuffer) + UDPHeader::WIRE_SIZE;

                switch (static_cast<MessageType>(head.type)) {
                    case MessageType::HeartBeatAck:
                        // Confirm connection on first HeartBeatAck
                        if (_connecting.exchange(false)) {
                            _connected.store(true);
                            logger->info("UDP connection confirmed by server");
                            _eventQueue.push(UDPConnectedEvent{0});  // Player ID will be set by PlayerJoin
                            if (_onConnected) {
                                _onConnected();
                            }
                        }
                        break;
                    case MessageType::PlayerJoin:
                        handlePlayerJoin(payload, payload_size);
                        break;
                    case MessageType::PlayerLeave:
                        handlePlayerLeave(payload, payload_size);
                        break;
                    case MessageType::Snapshot:
                        handleSnapshot(payload, payload_size);
                        break;
                    case MessageType::MissileSpawned:
                        handleMissileSpawned(payload, payload_size);
                        break;
                    case MessageType::MissileDestroyed:
                        handleMissileDestroyed(payload, payload_size);
                        break;
                    case MessageType::PlayerDied:
                        handlePlayerDied(payload, payload_size);
                        break;
                    default:
                        break;
                }
                asyncReceiveFrom();
            }
        } else {
            if (error == boost::asio::error::eof) {
                logger->info("Server disconnected");
            } else {
                logger->error("Read error: {}", error.message());
            }

            _eventQueue.push(UDPErrorEvent{"Erreur lecture: " + error.message()});
            if (_onError) {
                _onError("Erreur lecture: " + error.message());
            }

            disconnect();
        }
    }


    void UDPClient::movePlayer(uint16_t x, uint16_t y) {
        MovePlayer movePlayer = {.x = x, .y = y};

        UDPHeader head = {
            .type = static_cast<uint16_t>(MessageType::MovePlayer),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };

        const size_t totalSize = UDPHeader::WIRE_SIZE + MovePlayer::WIRE_SIZE;

        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        movePlayer.to_bytes(buf->data() + UDPHeader::WIRE_SIZE);
        asyncSendTo(buf, totalSize);
    }

    void UDPClient::shootMissile() {
        UDPHeader head = {
            .type = static_cast<uint16_t>(MessageType::ShootMissile),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };

        const size_t totalSize = UDPHeader::WIRE_SIZE;
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);
        head.to_bytes(buf->data());
        asyncSendTo(buf, totalSize);
    }

    void UDPClient::sendHeartbeat() {
        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::HeartBeat),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        auto buf = std::make_shared<std::vector<uint8_t>>(UDPHeader::WIRE_SIZE);
        head.to_bytes(buf->data());
        asyncSendTo(buf, UDPHeader::WIRE_SIZE);
    }

    void UDPClient::scheduleHeartbeat() {
        _heartbeatTimer.expires_after(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
        _heartbeatTimer.async_wait([this](boost::system::error_code ec) {
            // Continue if connected OR connecting (waiting for HeartBeatAck)
            if (ec || (!_connected.load() && !_connecting.load())) {
                return;
            }

            std::chrono::steady_clock::time_point lastResponse;
            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                lastResponse = _lastServerResponse;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastResponse
            ).count();

            if (elapsed > HEARTBEAT_TIMEOUT_MS) {
                auto logger = client::logging::Logger::getNetworkLogger();

                if (_connecting.load()) {
                    logger->warn("Initial connection timeout ({}ms) - server not responding", elapsed);
                    _eventQueue.push(UDPErrorEvent{"Timeout: Serveur UDP injoignable"});
                } else {
                    logger->warn("Server heartbeat timeout ({}ms)", elapsed);
                    _eventQueue.push(UDPErrorEvent{"Timeout: Serveur injoignable"});
                }

                if (_onError) {
                    _onError("Timeout: Serveur injoignable");
                }

                disconnect();
                return;
            }

            sendHeartbeat();
            scheduleHeartbeat();
        });
    }

    std::optional<UDPEvent> UDPClient::pollEvent() {
        return _eventQueue.poll();
    }

}
