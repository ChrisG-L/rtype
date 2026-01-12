/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/logging/Logger.hpp"
#include "Protocol.hpp"
#include <chrono>
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <mstcpip.h>
#endif

namespace infrastructure::adapters::in::network {

    static constexpr int BROADCAST_INTERVAL_MS = 50;
    static constexpr int PLAYER_TIMEOUT_MS = 2000;

    UDPServer::UDPServer(boost::asio::io_context& io_ctx, std::shared_ptr<SessionManager> sessionManager)
        : _io_ctx(io_ctx),
          _socket(io_ctx, udp::endpoint(udp::v4(), 4124)),
          _sessionManager(sessionManager),
          _broadcastTimer(io_ctx) {
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
    }

    std::string UDPServer::endpointToString(const udp::endpoint& ep) const {
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

    void UDPServer::start() {
        do_read();
        scheduleBroadcast();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _broadcastTimer.cancel();
        _socket.close();
    }

    void UDPServer::sendTo(const udp::endpoint& endpoint, const void* data, size_t size) {
        auto buf = std::make_shared<std::vector<uint8_t>>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size
        );

        _socket.async_send_to(
            boost::asio::buffer(buf->data(), buf->size()),
            endpoint,
            [buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    server::logging::Logger::getNetworkLogger()->error("Send error: {}", ec.message());
                }
            }
        );
    }

    void UDPServer::sendPlayerJoin(const udp::endpoint& endpoint, uint8_t playerId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerJoin::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerJoin),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PlayerJoin pj{.player_id = playerId};
        pj.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        // Broadcast to all players in the same game instance
        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getNetworkLogger()->info(
            "Player {} joined from {}:{}",
            static_cast<int>(playerId), endpoint.address().to_string(), endpoint.port());
    }

    void UDPServer::sendPlayerLeave(uint8_t playerId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerLeave::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerLeave),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PlayerLeave pl{.player_id = playerId};
        pl.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getNetworkLogger()->info("Player {} left", static_cast<int>(playerId));
    }

    void UDPServer::sendHeartbeatAck(const udp::endpoint& endpoint) {
        const size_t totalSize = UDPHeader::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::HeartBeatAck),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        sendTo(endpoint, buf.data(), buf.size());
    }

    void UDPServer::sendJoinGameAck(const udp::endpoint& endpoint, uint8_t playerId) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + JoinGameAck::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::JoinGameAck),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        JoinGameAck ack{.player_id = playerId};
        ack.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        sendTo(endpoint, buf.data(), buf.size());

        server::logging::Logger::getNetworkLogger()->debug(
            "JoinGameAck sent to {}:{} (playerId={})",
            endpoint.address().to_string(), endpoint.port(), static_cast<int>(playerId));
    }

    void UDPServer::sendJoinGameNack(const udp::endpoint& endpoint, const std::string& reason) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + JoinGameNack::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::JoinGameNack),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        JoinGameNack nack;
        std::strncpy(nack.reason, reason.c_str(), sizeof(nack.reason) - 1);
        nack.reason[sizeof(nack.reason) - 1] = '\0';
        nack.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        sendTo(endpoint, buf.data(), buf.size());

        server::logging::Logger::getNetworkLogger()->warn(
            "JoinGameNack sent to {}:{} (reason: {})",
            endpoint.address().to_string(), endpoint.port(), reason);
    }

    void UDPServer::broadcastSnapshotForRoom(const std::string& roomCode, game::GameWorld* gameWorld) {
        if (!gameWorld || gameWorld->getPlayerCount() == 0) {
            return;
        }

        GameSnapshot snapshot = gameWorld->getSnapshot();
        const size_t totalSize = UDPHeader::WIRE_SIZE + snapshot.wire_size();
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::Snapshot),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());
        snapshot.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        // Only send to players in THIS game instance
        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::broadcastAllSnapshots() {
        auto roomCodes = _instanceManager.getActiveRoomCodes();
        for (const auto& roomCode : roomCodes) {
            game::GameWorld* gameWorld = _instanceManager.getInstance(roomCode);
            if (gameWorld) {
                broadcastSnapshotForRoom(roomCode, gameWorld);
            }
        }
    }

    void UDPServer::updateAndBroadcastRoom(const std::string& roomCode, game::GameWorld* gameWorld, float deltaTime) {
        if (!gameWorld) return;

        // Check for timed out players
        auto timedOutPlayers = gameWorld->checkPlayerTimeouts(
            std::chrono::milliseconds(PLAYER_TIMEOUT_MS)
        );
        for (uint8_t playerId : timedOutPlayers) {
            sendPlayerLeave(playerId, gameWorld);
            server::logging::Logger::getNetworkLogger()->warn(
                "Player {} timed out in room {} (no heartbeat)",
                static_cast<int>(playerId), roomCode);
        }

        // Update player positions based on inputs (server-authoritative)
        gameWorld->updatePlayers(deltaTime);

        // Update missiles
        gameWorld->updateMissiles(deltaTime);

        // Update waves and enemies
        gameWorld->updateWaveSpawning(deltaTime);
        gameWorld->updateEnemies(deltaTime);

        // Check collisions
        gameWorld->checkCollisions();

        // Process destroyed missiles
        auto destroyedMissiles = gameWorld->getDestroyedMissiles();
        for (uint16_t id : destroyedMissiles) {
            broadcastMissileDestroyed(id, gameWorld);
        }

        // Process destroyed enemies
        auto destroyedEnemies = gameWorld->getDestroyedEnemies();
        for (uint16_t id : destroyedEnemies) {
            broadcastEnemyDestroyed(id, gameWorld);
        }

        // Process damage events
        auto damageEvents = gameWorld->getPlayerDamageEvents();
        for (const auto& [playerId, damage] : damageEvents) {
            broadcastPlayerDamaged(playerId, damage, gameWorld);
        }

        // Process dead players
        auto deadPlayers = gameWorld->getDeadPlayers();
        for (uint8_t playerId : deadPlayers) {
            broadcastPlayerDied(playerId, gameWorld);
        }

        // Broadcast snapshot for this room
        broadcastSnapshotForRoom(roomCode, gameWorld);
    }

    void UDPServer::scheduleBroadcast() {
        _broadcastTimer.expires_after(std::chrono::milliseconds(BROADCAST_INTERVAL_MS));
        _broadcastTimer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                float deltaTime = BROADCAST_INTERVAL_MS / 1000.0f;

                // Get all active room codes (snapshot to avoid modification during iteration)
                auto roomCodes = _instanceManager.getActiveRoomCodes();

                // Update each game instance independently
                for (const auto& roomCode : roomCodes) {
                    game::GameWorld* gameWorld = _instanceManager.getInstance(roomCode);
                    if (!gameWorld) continue;

                    updateAndBroadcastRoom(roomCode, gameWorld, deltaTime);

                    // Cleanup empty instances
                    if (gameWorld->getPlayerCount() == 0) {
                        _instanceManager.removeInstance(roomCode);
                        server::logging::Logger::getGameLogger()->info(
                            "Removed empty game instance for room '{}'", roomCode);
                    }
                }

                scheduleBroadcast();
            }
        });
    }

    void UDPServer::broadcastMissileSpawned(uint16_t missileId, uint8_t ownerId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        auto missileOpt = gameWorld->getMissile(missileId);
        if (!missileOpt) return;

        const auto& m = *missileOpt;

        const size_t totalSize = UDPHeader::WIRE_SIZE + MissileSpawned::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::MissileSpawned),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        MissileSpawned ms{
            .missile_id = missileId,
            .owner_id = ownerId,
            .x = static_cast<uint16_t>(m.x),
            .y = static_cast<uint16_t>(m.y)
        };
        ms.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::broadcastMissileDestroyed(uint16_t missileId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + MissileDestroyed::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::MissileDestroyed),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        MissileDestroyed md{.missile_id = missileId};
        md.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::broadcastEnemyDestroyed(uint16_t enemyId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + EnemyDestroyed::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::EnemyDestroyed),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        EnemyDestroyed ed{.enemy_id = enemyId};
        ed.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::broadcastPlayerDamaged(uint8_t playerId, uint8_t damage, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerDamaged::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerDamaged),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        auto snapshot = gameWorld->getSnapshot();
        uint8_t newHealth = 0;
        for (uint8_t i = 0; i < snapshot.player_count; ++i) {
            if (snapshot.players[i].id == playerId) {
                newHealth = snapshot.players[i].health;
                break;
            }
        }

        PlayerDamaged pd{
            .player_id = playerId,
            .damage = damage,
            .new_health = newHealth
        };
        pd.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::broadcastPlayerDied(uint8_t playerId, game::GameWorld* gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerDied::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerDied),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PlayerDied pd{.player_id = playerId};
        pd.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->info("Player {} died", static_cast<int>(playerId));
    }

    void UDPServer::do_read() {
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _remote_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes) {
                handle_receive(error, bytes);
            }
        );
    }

    void UDPServer::handle_receive(const boost::system::error_code& error, std::size_t bytes) {
        if (error) {
            if (error != boost::asio::error::operation_aborted) {
                server::logging::Logger::getNetworkLogger()->error("Receive error: {}", error.message());
                do_read();
            }
            return;
        }

        if (bytes < UDPHeader::WIRE_SIZE) {
            do_read();
            return;
        }

        auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
        if (!headOpt) {
            do_read();
            return;
        }

        UDPHeader head = *headOpt;
        size_t payload_size = bytes - UDPHeader::WIRE_SIZE;
        const uint8_t* payload = reinterpret_cast<const uint8_t*>(_readBuffer) + UDPHeader::WIRE_SIZE;

        std::string endpointStr = endpointToString(_remote_endpoint);

        // ═══════════════════════════════════════════════════════════════════
        // CASE 1: HeartBeat - No authentication required (connection check)
        // But if the endpoint has an active session, update activity timestamp
        // ═══════════════════════════════════════════════════════════════════
        if (head.type == static_cast<uint16_t>(MessageType::HeartBeat)) {
            sendHeartbeatAck(_remote_endpoint);

            // Update activity if this endpoint has an active session
            auto playerIdOpt = _sessionManager->getPlayerIdByEndpoint(endpointStr);
            if (playerIdOpt) {
                _sessionManager->updateActivity(endpointStr);

                // Get the room code to find the correct GameWorld
                auto roomCodeOpt = _sessionManager->getRoomCodeByEndpoint(endpointStr);
                if (roomCodeOpt) {
                    game::GameWorld* gameWorld = _instanceManager.getInstance(*roomCodeOpt);
                    if (gameWorld) {
                        gameWorld->updatePlayerActivity(*playerIdOpt);
                    }
                }
            }

            do_read();
            return;
        }

        // ═══════════════════════════════════════════════════════════════════
        // CASE 2: JoinGame - Authentication with token (creates player)
        // ═══════════════════════════════════════════════════════════════════
        if (head.type == static_cast<uint16_t>(MessageType::JoinGame)) {
            if (payload_size < JoinGame::WIRE_SIZE) {
                sendJoinGameNack(_remote_endpoint, "Invalid packet");
                do_read();
                return;
            }

            auto joinOpt = JoinGame::from_bytes(payload, payload_size);
            if (!joinOpt) {
                sendJoinGameNack(_remote_endpoint, "Invalid packet");
                do_read();
                return;
            }

            // Extract room code from JoinGame message
            std::string roomCode(joinOpt->roomCode, ROOM_CODE_LEN);

            // Validate token via SessionManager
            auto validateResult = _sessionManager->validateAndBindUDP(joinOpt->token, endpointStr);
            if (!validateResult) {
                sendJoinGameNack(_remote_endpoint, "Invalid or expired token");
                do_read();
                return;
            }

            // Get or create the GameWorld for this room
            game::GameWorld* gameWorld = _instanceManager.getOrCreateInstance(roomCode);
            if (!gameWorld) {
                sendJoinGameNack(_remote_endpoint, "Failed to create game instance");
                _sessionManager->removeSessionByEndpoint(endpointStr);
                do_read();
                return;
            }

            // Create player in the room's GameWorld
            auto playerIdOpt = gameWorld->addPlayer(_remote_endpoint);
            if (!playerIdOpt) {
                sendJoinGameNack(_remote_endpoint, "Room full");
                _sessionManager->removeSessionByEndpoint(endpointStr);
                do_read();
                return;
            }

            // Apply room game speed to this GameWorld instance
            uint16_t gameSpeedPercent = _sessionManager->getRoomGameSpeedByEndpoint(endpointStr);
            gameWorld->setGameSpeedPercent(gameSpeedPercent);
            server::logging::Logger::getGameLogger()->info(
                "Room '{}' game speed set to {}% (multiplier: {:.2f})",
                roomCode, gameSpeedPercent, gameWorld->getGameSpeedMultiplier());

            // Set player's ship skin (from JoinGame message)
            gameWorld->setPlayerSkin(*playerIdOpt, joinOpt->shipSkin);

            // Bind playerId to session
            _sessionManager->assignPlayerId(endpointStr, *playerIdOpt);

            // Send confirmation
            sendJoinGameAck(_remote_endpoint, *playerIdOpt);

            // Broadcast to other players in the same room
            sendPlayerJoin(_remote_endpoint, *playerIdOpt, gameWorld);

            server::logging::Logger::getNetworkLogger()->info(
                "Player {} (email: {}) joined room '{}' as player {}",
                validateResult->displayName, validateResult->email, roomCode, static_cast<int>(*playerIdOpt));

            do_read();
            return;
        }

        // ═══════════════════════════════════════════════════════════════════
        // MESSAGES REQUIRING AUTHENTICATION: Check if endpoint has session
        // ═══════════════════════════════════════════════════════════════════
        if (requiresAuth(head.type)) {
            auto playerIdOpt = _sessionManager->getPlayerIdByEndpoint(endpointStr);
            if (!playerIdOpt) {
                // Endpoint not authenticated - silently ignore
                do_read();
                return;
            }

            uint8_t playerId = *playerIdOpt;

            // Get the room code to find the correct GameWorld
            auto roomCodeOpt = _sessionManager->getRoomCodeByEndpoint(endpointStr);
            if (!roomCodeOpt) {
                do_read();
                return;
            }

            game::GameWorld* gameWorld = _instanceManager.getInstance(*roomCodeOpt);
            if (!gameWorld) {
                do_read();
                return;
            }

            // Update activity timestamp
            _sessionManager->updateActivity(endpointStr);
            gameWorld->updatePlayerActivity(playerId);

            // Check if player is alive for gameplay messages
            if (!gameWorld->isPlayerAlive(playerId)) {
                do_read();
                return;
            }

            // ═══════════════════════════════════════════════════════════════
            // PlayerInput (server authoritative movement)
            // ═══════════════════════════════════════════════════════════════
            if (head.type == static_cast<uint16_t>(MessageType::PlayerInput)) {
                if (payload_size >= PlayerInput::WIRE_SIZE) {
                    auto inputOpt = PlayerInput::from_bytes(payload, payload_size);
                    if (inputOpt) {
                        gameWorld->applyPlayerInput(playerId, inputOpt->keys, inputOpt->sequenceNum);
                    }
                }
            }
            // ═══════════════════════════════════════════════════════════════
            // ShootMissile
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::ShootMissile)) {
                uint16_t missileId = gameWorld->spawnMissile(playerId);
                if (missileId > 0) {
                    broadcastMissileSpawned(missileId, playerId, gameWorld);
                }
            }
        }
        // ═══════════════════════════════════════════════════════════════════
        // MESSAGES NOT REQUIRING AUTH: Process directly
        // (Currently none besides HeartBeat which is handled above)
        // ═══════════════════════════════════════════════════════════════════

        do_read();
    }

    void UDPServer::kickPlayer(uint8_t playerId) {
        // Search through all game instances to find the player
        auto roomCodes = _instanceManager.getActiveRoomCodes();
        for (const auto& roomCode : roomCodes) {
            game::GameWorld* gameWorld = _instanceManager.getInstance(roomCode);
            if (!gameWorld) continue;

            auto endpoints = gameWorld->getAllEndpoints();
            for (const auto& ep : endpoints) {
                auto pid = gameWorld->getPlayerIdByEndpoint(ep);
                if (pid && *pid == playerId) {
                    std::string endpointStr = endpointToString(ep);

                    // Clear the UDP binding from SessionManager
                    _sessionManager->clearUDPBinding(endpointStr);

                    // Remove from GameWorld
                    gameWorld->removePlayer(playerId);

                    // Broadcast PlayerLeave to remaining players in this room
                    sendPlayerLeave(playerId, gameWorld);

                    server::logging::Logger::getMainLogger()->info(
                        "[CLI] Player {} kicked from room '{}'.", static_cast<int>(playerId), roomCode);
                    return;
                }
            }
        }

        server::logging::Logger::getMainLogger()->warn(
            "[CLI] Player {} not found in any game.", static_cast<int>(playerId));
    }

    size_t UDPServer::getPlayerCount() const {
        return _instanceManager.getTotalPlayerCount();
    }
}
