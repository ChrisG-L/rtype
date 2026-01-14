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
          _instanceManager(io_ctx),
          _sessionManager(sessionManager),
          _broadcastTimer(io_ctx),
          _networkStats(std::make_shared<infrastructure::network::NetworkStats>()),
          _statsTimer(io_ctx) {
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

        // Register callback to handle player leaving game via TCP
        if (_sessionManager) {
            _sessionManager->setPlayerLeaveGameCallback(
                [this](uint8_t playerId, const std::string& roomCode, const std::string& endpoint) {
                    // Post to io_context to ensure thread-safety
                    boost::asio::post(_io_ctx, [this, playerId, roomCode, endpoint]() {
                        handlePlayerLeaveGame(playerId, roomCode, endpoint);
                    });
                }
            );
        }
    }

    std::string UDPServer::endpointToString(const udp::endpoint& ep) const {
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

    void UDPServer::start() {
        do_read();
        scheduleBroadcast();
        scheduleStatsUpdate();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _broadcastTimer.cancel();
        _statsTimer.cancel();
        _socket.close();
    }

    void UDPServer::sendTo(const udp::endpoint& endpoint, const void* data, size_t size) {
        // Track network stats
        std::string endpointStr = endpointToString(endpoint);
        _networkStats->addBytesSent(size);
        _networkStats->addBytesSentTo(endpointStr, size);

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

    void UDPServer::sendPlayerJoin(const udp::endpoint& endpoint, uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::sendPlayerLeave(uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::broadcastSnapshotForRoom(const std::string& roomCode, const std::shared_ptr<game::GameWorld>& gameWorld) {
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
            auto gameWorld = _instanceManager.getInstance(roomCode);
            if (gameWorld) {
                broadcastSnapshotForRoom(roomCode, gameWorld);
            }
        }
    }

    void UDPServer::updateAndBroadcastRoom(const std::string& roomCode, const std::shared_ptr<game::GameWorld>& gameWorld, float deltaTime) {
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

        // Update weapon cooldowns (Gameplay Phase 2)
        gameWorld->updateShootCooldowns(deltaTime);

        // Update missiles
        gameWorld->updateMissiles(deltaTime);

        // Update waves and enemies
        gameWorld->updateWaveSpawning(deltaTime);
        gameWorld->updateEnemies(deltaTime);

        // Check and update boss (Gameplay Phase 2)
        gameWorld->checkBossSpawn();
        gameWorld->updateBoss(deltaTime);

        // Update combo timers (Gameplay Phase 2)
        gameWorld->updateComboTimers(deltaTime);

        // R-Type Authentic (Phase 3) updates
        gameWorld->updateAllCharging(deltaTime);  // Update charge timers for all players
        gameWorld->updateWaveCannons(deltaTime);
        gameWorld->updatePowerUps(deltaTime);
        gameWorld->updateForcePods(deltaTime);
        gameWorld->updateBitDevices(deltaTime);   // Bit Devices orbit and cooldowns
        gameWorld->checkPowerUpCollisions();
        gameWorld->checkForceCollisions();
        gameWorld->checkBitCollisions();          // Bit contact damage

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

        // R-Type Authentic (Phase 3) broadcasts
        // Process newly spawned power-ups
        auto newPowerUps = gameWorld->getNewlySpawnedPowerUps();
        for (uint16_t id : newPowerUps) {
            broadcastPowerUpSpawned(id, gameWorld);
        }

        // Process collected power-ups
        auto collectedPowerUps = gameWorld->getCollectedPowerUps();
        for (const auto& collected : collectedPowerUps) {
            broadcastPowerUpCollected(collected.powerup_id, collected.player_id, collected.powerup_type, gameWorld);
        }

        // Process expired power-ups
        auto expiredPowerUps = gameWorld->getExpiredPowerUps();
        for (uint16_t id : expiredPowerUps) {
            broadcastPowerUpExpired(id, gameWorld);
        }

        // Process destroyed Wave Cannons (they're removed after hitting something)
        auto destroyedWaveCannons = gameWorld->getDestroyedWaveCannons();
        // Note: We don't broadcast WaveCannon destruction - it's handled client-side when off-screen

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

                // Update each game instance independently - POST TO EACH ROOM'S STRAND
                // This allows rooms to be processed IN PARALLEL on different threads
                for (const auto& roomCode : roomCodes) {
                    auto gameWorld = _instanceManager.getInstance(roomCode);
                    if (!gameWorld) continue;

                    // Post the update work to this room's strand
                    // Each room's strand serializes its own operations
                    // but different rooms can run concurrently on different threads
                    boost::asio::post(gameWorld->getStrand(),
                        [this, roomCode, gameWorld, deltaTime]() {
                            updateAndBroadcastRoom(roomCode, gameWorld, deltaTime);

                            // Cleanup empty instances (post to main io_context for thread safety)
                            if (gameWorld->getPlayerCount() == 0) {
                                boost::asio::post(_io_ctx, [this, roomCode]() {
                                    _instanceManager.removeInstance(roomCode);
                                    server::logging::Logger::getGameLogger()->info(
                                        "Removed empty game instance for room '{}'", roomCode);
                                });
                            }
                        });
                }

                scheduleBroadcast();
            }
        });
    }

    void UDPServer::broadcastMissileSpawned(uint16_t missileId, uint8_t ownerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::broadcastMissileDestroyed(uint16_t missileId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::broadcastEnemyDestroyed(uint16_t enemyId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::broadcastPlayerDamaged(uint8_t playerId, uint8_t damage, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    void UDPServer::broadcastPlayerDied(uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
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

    // ============================================================================
    // R-Type Authentic (Phase 3) Broadcast Methods
    // ============================================================================

    void UDPServer::broadcastWaveCannonFired(uint16_t waveCannonId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto wcOpt = gameWorld->getWaveCannon(waveCannonId);
        if (!wcOpt) return;

        const auto& wc = *wcOpt;

        const size_t totalSize = UDPHeader::WIRE_SIZE + WaveCannonState::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::WaveCannonFired),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        WaveCannonState wcState{
            .id = wc.id,
            .owner_id = wc.owner_id,
            .x = static_cast<uint16_t>(wc.x),
            .y = static_cast<uint16_t>(wc.y),
            .charge_level = wc.chargeLevel,
            .width = static_cast<uint8_t>(wc.width)
        };
        wcState.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->debug("Wave Cannon {} fired by player {}",
            waveCannonId, static_cast<int>(wc.owner_id));
    }

    void UDPServer::broadcastPowerUpSpawned(uint16_t powerUpId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto puOpt = gameWorld->getPowerUp(powerUpId);
        if (!puOpt) return;

        const auto& pu = *puOpt;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PowerUpState::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PowerUpSpawned),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PowerUpState puState{
            .id = pu.id,
            .x = static_cast<uint16_t>(pu.x),
            .y = static_cast<uint16_t>(pu.y),
            .type = static_cast<uint8_t>(pu.type),
            .remaining_time = static_cast<uint8_t>(pu.lifetime)
        };
        puState.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->debug("Power-up {} spawned (type {})",
            powerUpId, static_cast<int>(pu.type));
    }

    void UDPServer::broadcastPowerUpCollected(uint16_t powerUpId, uint8_t playerId, uint8_t powerUpType, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PowerUpCollected::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PowerUpCollected),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PowerUpCollected pc{
            .powerup_id = powerUpId,
            .player_id = playerId,
            .powerup_type = powerUpType
        };
        pc.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->debug("Power-up {} collected by player {}",
            powerUpId, static_cast<int>(playerId));
    }

    void UDPServer::broadcastPowerUpExpired(uint16_t powerUpId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + PowerUpExpired::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PowerUpExpired),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PowerUpExpired pe{.powerup_id = powerUpId};
        pe.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->debug("Power-up {} expired", powerUpId);
    }

    void UDPServer::broadcastForceStateUpdate(uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto forceOpt = gameWorld->getPlayerForce(playerId);
        if (!forceOpt) return;

        const auto& force = *forceOpt;

        const size_t totalSize = UDPHeader::WIRE_SIZE + ForceState::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::ForceStateUpdate),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        ForceState fs{
            .owner_id = force.ownerId,
            .x = static_cast<uint16_t>(force.x),
            .y = static_cast<uint16_t>(force.y),
            .is_attached = force.isAttached ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0),
            .level = force.level
        };
        fs.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        server::logging::Logger::getGameLogger()->debug("Force state updated for player {}",
            static_cast<int>(playerId));
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

        // Track network stats (bytes received)
        _networkStats->addBytesReceived(bytes);
        _networkStats->addBytesReceivedFrom(endpointStr, bytes);

        // ═══════════════════════════════════════════════════════════════════
        // CASE 1: HeartBeat - No authentication required (connection check)
        // But if the endpoint has an active session, update activity timestamp
        // ═══════════════════════════════════════════════════════════════════
        if (head.type == static_cast<uint16_t>(MessageType::HeartBeat)) {
            sendHeartbeatAck(_remote_endpoint);

            // Calculate one-way RTT (client → server) from HeartBeat timestamp
            uint64_t serverNow = UDPHeader::getTimestamp();
            uint64_t clientTimestamp = head.timestamp;
            if (serverNow >= clientTimestamp) {
                uint32_t rttMs = static_cast<uint32_t>(serverNow - clientTimestamp);
                // Cap RTT at a reasonable max (10 seconds) to filter outliers
                if (rttMs < 10000) {
                    _networkStats->updatePlayerRTT(endpointStr, rttMs);
                }
            }

            // Update activity if this endpoint has an active session
            auto playerIdOpt = _sessionManager->getPlayerIdByEndpoint(endpointStr);
            if (playerIdOpt) {
                _sessionManager->updateActivity(endpointStr);

                // Get the room code to find the correct GameWorld
                auto roomCodeOpt = _sessionManager->getRoomCodeByEndpoint(endpointStr);
                if (roomCodeOpt) {
                    auto gameWorld = _instanceManager.getInstance(*roomCodeOpt);
                    if (gameWorld) {
                        // Post to room's strand for thread safety
                        uint8_t playerId = *playerIdOpt;
                        boost::asio::post(gameWorld->getStrand(),
                            [gameWorld, playerId]() {
                                gameWorld->updatePlayerActivity(playerId);
                            });
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
            auto gameWorld = _instanceManager.getOrCreateInstance(roomCode);
            if (!gameWorld) {
                sendJoinGameNack(_remote_endpoint, "Failed to create game instance");
                _sessionManager->removeSessionByEndpoint(endpointStr);
                do_read();
                return;
            }

            // Capture endpoint for lambda (remote_endpoint is a member, may change before lambda runs)
            auto remoteEndpoint = _remote_endpoint;
            uint8_t shipSkin = joinOpt->shipSkin;
            uint16_t gameSpeedPercent = _sessionManager->getRoomGameSpeedByEndpoint(endpointStr);
            std::string displayName = validateResult->displayName;
            std::string email = validateResult->email;

            // Post player creation to room's strand for thread safety
            boost::asio::post(gameWorld->getStrand(),
                [this, gameWorld, roomCode, remoteEndpoint, endpointStr, shipSkin,
                 gameSpeedPercent, displayName, email]() {

                    // Create player in the room's GameWorld
                    auto playerIdOpt = gameWorld->addPlayer(remoteEndpoint);
                    if (!playerIdOpt) {
                        sendJoinGameNack(remoteEndpoint, "Room full");
                        _sessionManager->removeSessionByEndpoint(endpointStr);
                        return;
                    }

                    // Apply room game speed to this GameWorld instance
                    gameWorld->setGameSpeedPercent(gameSpeedPercent);
                    server::logging::Logger::getGameLogger()->info(
                        "Room '{}' game speed set to {}% (multiplier: {:.2f})",
                        roomCode, gameSpeedPercent, gameWorld->getGameSpeedMultiplier());

                    // Set player's ship skin (from JoinGame message)
                    gameWorld->setPlayerSkin(*playerIdOpt, shipSkin);

                    // Bind playerId to session (SessionManager is thread-safe)
                    _sessionManager->assignPlayerId(endpointStr, *playerIdOpt);

                    // Register player in network stats for monitoring
                    _networkStats->registerPlayer(endpointStr);

                    // Send confirmation (sendTo uses async_send_to, thread-safe)
                    sendJoinGameAck(remoteEndpoint, *playerIdOpt);

                    // Broadcast to other players in the same room
                    sendPlayerJoin(remoteEndpoint, *playerIdOpt, gameWorld);

                    server::logging::Logger::getNetworkLogger()->info(
                        "Player {} (email: {}) joined room '{}' as player {}",
                        displayName, email, roomCode, static_cast<int>(*playerIdOpt));
                });

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

            auto gameWorld = _instanceManager.getInstance(*roomCodeOpt);
            if (!gameWorld) {
                do_read();
                return;
            }

            // Update activity timestamp (SessionManager is thread-safe)
            _sessionManager->updateActivity(endpointStr);

            // ═══════════════════════════════════════════════════════════════
            // PlayerInput (server authoritative movement)
            // ═══════════════════════════════════════════════════════════════
            if (head.type == static_cast<uint16_t>(MessageType::PlayerInput)) {
                if (payload_size >= PlayerInput::WIRE_SIZE) {
                    auto inputOpt = PlayerInput::from_bytes(payload, payload_size);
                    if (inputOpt) {
                        uint16_t keys = inputOpt->keys;
                        uint16_t seqNum = inputOpt->sequenceNum;

                        // Post to room's strand for thread safety
                        boost::asio::post(gameWorld->getStrand(),
                            [gameWorld, playerId, keys, seqNum]() {
                                // Update activity and check alive status in the strand
                                gameWorld->updatePlayerActivity(playerId);
                                if (gameWorld->isPlayerAlive(playerId)) {
                                    gameWorld->applyPlayerInput(playerId, keys, seqNum);
                                }
                            });
                    }
                }
            }
            // ═══════════════════════════════════════════════════════════════
            // ShootMissile
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::ShootMissile)) {
                // Post to room's strand for thread safety
                boost::asio::post(gameWorld->getStrand(),
                    [this, gameWorld, playerId]() {
                        // Update activity and check alive status in the strand
                        gameWorld->updatePlayerActivity(playerId);
                        if (gameWorld->isPlayerAlive(playerId)) {
                            // Use weapon-based spawning (may spawn multiple for spread)
                            std::vector<uint16_t> missileIds = gameWorld->spawnMissileWithWeapon(playerId);
                            for (uint16_t missileId : missileIds) {
                                if (missileId > 0) {
                                    // broadcastMissileSpawned uses async_send_to, thread-safe
                                    broadcastMissileSpawned(missileId, playerId, gameWorld);
                                }
                            }
                        }
                    });
            }
            // ═══════════════════════════════════════════════════════════════
            // R-Type Authentic (Phase 3): ChargeStart
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::ChargeStart)) {
                boost::asio::post(gameWorld->getStrand(),
                    [gameWorld, playerId]() {
                        gameWorld->updatePlayerActivity(playerId);
                        if (gameWorld->isPlayerAlive(playerId)) {
                            gameWorld->startCharging(playerId);
                        }
                    });
            }
            // ═══════════════════════════════════════════════════════════════
            // R-Type Authentic (Phase 3): ChargeRelease
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::ChargeRelease)) {
                if (payload_size >= ChargeRelease::WIRE_SIZE) {
                    auto releaseOpt = ChargeRelease::from_bytes(payload, payload_size);
                    if (releaseOpt) {
                        boost::asio::post(gameWorld->getStrand(),
                            [this, gameWorld, playerId]() {
                                gameWorld->updatePlayerActivity(playerId);
                                if (gameWorld->isPlayerAlive(playerId)) {
                                    uint16_t waveCannonId = gameWorld->releaseCharge(playerId);
                                    if (waveCannonId > 0) {
                                        // Broadcast the Wave Cannon to all players
                                        broadcastWaveCannonFired(waveCannonId, gameWorld);
                                    }
                                }
                            });
                    }
                }
            }
            // ═══════════════════════════════════════════════════════════════
            // R-Type Authentic (Phase 3): ForceToggle
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::ForceToggle)) {
                boost::asio::post(gameWorld->getStrand(),
                    [this, gameWorld, playerId]() {
                        gameWorld->updatePlayerActivity(playerId);
                        if (gameWorld->isPlayerAlive(playerId)) {
                            gameWorld->toggleForceAttach(playerId);
                            // Broadcast force state update
                            broadcastForceStateUpdate(playerId, gameWorld);
                        }
                    });
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
            auto gameWorld = _instanceManager.getInstance(roomCode);
            if (!gameWorld) continue;

            // We need to check if player exists, which requires accessing GameWorld
            // Post the entire kick operation to the room's strand for thread safety
            boost::asio::post(gameWorld->getStrand(),
                [this, gameWorld, playerId, roomCode]() {
                    auto endpoints = gameWorld->getAllEndpoints();
                    for (const auto& ep : endpoints) {
                        auto pid = gameWorld->getPlayerIdByEndpoint(ep);
                        if (pid && *pid == playerId) {
                            std::string endpointStr = endpointToString(ep);

                            // Clear the UDP binding from SessionManager (thread-safe)
                            _sessionManager->clearUDPBinding(endpointStr);

                            // Unregister from network stats (thread-safe)
                            _networkStats->unregisterPlayer(endpointStr);

                            // Remove from GameWorld (we're in the strand, safe)
                            gameWorld->removePlayer(playerId);

                            // Broadcast PlayerLeave to remaining players in this room
                            sendPlayerLeave(playerId, gameWorld);

                            server::logging::Logger::getMainLogger()->info(
                                "[CLI] Player {} kicked from room '{}'.", static_cast<int>(playerId), roomCode);
                            return;
                        }
                    }
                });
        }
        // Note: "Player not found" warning is no longer logged here because
        // the check happens asynchronously in the strand. This is a trade-off
        // for thread safety.
    }

    size_t UDPServer::getPlayerCount() const {
        return _instanceManager.getTotalPlayerCount();
    }

    void UDPServer::scheduleStatsUpdate() {
        _statsTimer.expires_after(std::chrono::seconds(1));
        _statsTimer.async_wait([this](boost::system::error_code ec) {
            if (ec) return;

            _networkStats->calculateRates();
            scheduleStatsUpdate();  // Reschedule
        });
    }

    void UDPServer::handlePlayerLeaveGame(uint8_t playerId, const std::string& roomCode, const std::string& endpoint) {
        auto logger = server::logging::Logger::getGameLogger();

        // Unregister player from network stats (thread-safe)
        _networkStats->unregisterPlayer(endpoint);

        // Get the game instance for this room
        auto gameWorld = _instanceManager.getInstance(roomCode);
        if (!gameWorld) {
            logger->debug("Player {} left game but room '{}' instance not found",
                         static_cast<int>(playerId), roomCode);
            return;
        }

        // Post to room's strand for thread safety
        boost::asio::post(gameWorld->getStrand(),
            [this, gameWorld, playerId, roomCode, logger]() {
                // Remove player from GameWorld (we're in the strand, safe)
                gameWorld->removePlayer(playerId);

                // Broadcast PlayerLeave to remaining players in this room
                sendPlayerLeave(playerId, gameWorld);

                logger->info("Player {} left game in room '{}' (via TCP leaveRoom)",
                            static_cast<int>(playerId), roomCode);

                // Cleanup empty instance (post to main context for thread safety)
                if (gameWorld->getPlayerCount() == 0) {
                    boost::asio::post(_io_ctx, [this, roomCode, logger]() {
                        _instanceManager.removeInstance(roomCode);
                        logger->info("Removed empty game instance for room '{}'", roomCode);
                    });
                }
            });
    }
}
