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
#include <cstdio>

#ifdef _WIN32
    #include <winsock2.h>
    #include <mstcpip.h>
#endif

namespace infrastructure::adapters::in::network {

    static constexpr int BROADCAST_INTERVAL_MS = 50;

    // ════════════════════════════════════════════════════════════════════════
    // Generic broadcast method implementation (reduces code duplication)
    // ════════════════════════════════════════════════════════════════════════
    template<typename T>
    void UDPServer::broadcastToRoom(MessageType type, const T& payload,
                                     const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        const size_t totalSize = UDPHeader::WIRE_SIZE + T::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(type),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());
        payload.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = gameWorld->getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    static constexpr int PLAYER_TIMEOUT_MS = 2000;
    static constexpr int AUTO_SAVE_INTERVAL_MS = 1000;  // Auto-save every 1 second

    UDPServer::UDPServer(boost::asio::io_context& io_ctx,
                         std::shared_ptr<SessionManager> sessionManager,
                         std::shared_ptr<ILeaderboardRepository> leaderboardRepository)
        : _io_ctx(io_ctx),
          _socket(io_ctx, udp::endpoint(udp::v4(), 4124)),
          _instanceManager(io_ctx),
          _sessionManager(sessionManager),
          _leaderboardRepository(leaderboardRepository),
          _broadcastTimer(io_ctx),
          _networkStats(std::make_shared<infrastructure::network::NetworkStats>()),
          _statsTimer(io_ctx),
          _autoSaveTimer(io_ctx) {
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
                [this](uint8_t playerId, const std::string& roomCode, const std::string& endpoint,
                       const std::string& email, const std::string& displayName) {
                    // Post to io_context to ensure thread-safety
                    boost::asio::post(_io_ctx, [this, playerId, roomCode, endpoint, email, displayName]() {
                        handlePlayerLeaveGame(playerId, roomCode, endpoint, email, displayName);
                    });
                }
            );

            // Register callback to handle GodMode changes in real-time
            _sessionManager->setGodModeChangedCallback(
                [this](uint8_t playerId, const std::string& roomCode, bool enabled) {
                    auto gameWorld = _instanceManager.getInstance(roomCode);
                    if (gameWorld) {
                        // Post to room's strand for thread-safety
                        boost::asio::post(gameWorld->getStrand(),
                            [gameWorld, playerId, enabled]() {
                                gameWorld->setPlayerGodMode(playerId, enabled);
                            });
                    }
                }
            );
        }
    }

    UDPServer::~UDPServer() {
        // Clear callbacks to prevent use-after-free during shutdown
        // When io_context is destroyed, it may trigger Session destructors which call
        // notifyPlayerLeaveGame(). If the callback still references this UDPServer,
        // it would cause a stack-use-after-scope error.
        if (_sessionManager) {
            _sessionManager->setPlayerLeaveGameCallback(nullptr);
            _sessionManager->setGodModeChangedCallback(nullptr);
        }
    }

    std::string UDPServer::endpointToString(const udp::endpoint& ep) const {
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

    void UDPServer::start() {
        do_read();
        scheduleBroadcast();
        scheduleStatsUpdate();
        scheduleAutoSave();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _broadcastTimer.cancel();
        _statsTimer.cancel();
        _autoSaveTimer.cancel();
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

        PlayerJoin pj{.player_id = playerId};
        broadcastToRoom(MessageType::PlayerJoin, pj, gameWorld);

        server::logging::Logger::getNetworkLogger()->info(
            "Player {} joined from {}:{}",
            static_cast<int>(playerId), endpoint.address().to_string(), endpoint.port());
    }

    void UDPServer::sendPlayerLeave(uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        PlayerLeave pl{.player_id = playerId};
        broadcastToRoom(MessageType::PlayerLeave, pl, gameWorld);

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
        std::snprintf(nack.reason, sizeof(nack.reason), "%s", reason.c_str());
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

        // Check for timed out players (even when paused - players can still disconnect)
        auto timedOutPlayers = gameWorld->checkPlayerTimeouts(
            std::chrono::milliseconds(PLAYER_TIMEOUT_MS)
        );
        for (uint8_t playerId : timedOutPlayers) {
            sendPlayerLeave(playerId, gameWorld);
            server::logging::Logger::getNetworkLogger()->warn(
                "Player {} timed out in room {} (no heartbeat)",
                static_cast<int>(playerId), roomCode);
        }

        // ═══════════════════════════════════════════════════════════════════
        // PAUSE SYSTEM: Skip gameplay updates when game is paused
        // Solo: paused if single player presses Escape
        // Multiplayer: paused only when ALL players press Escape
        // ═══════════════════════════════════════════════════════════════════
        if (!gameWorld->isPaused()) {
            // ═══════════════════════════════════════════════════════════════════
            // Phase 4.7: ECS drives core game logic (movement, collisions)
            // When ECS is enabled, it handles player/missile/enemy movement
            // and syncs state back to legacy maps for event collection
            // ═══════════════════════════════════════════════════════════════════
#ifdef USE_ECS_BACKEND
            gameWorld->runECSUpdate(deltaTime);
            // Skip updatePlayers() - ECS PlayerInputSystem + MovementSystem handles it
#else
            // Update player positions based on inputs (server-authoritative)
            gameWorld->updatePlayers(deltaTime);
#endif

            // Update weapon cooldowns (Gameplay Phase 2)
            gameWorld->updateShootCooldowns(deltaTime);

            // Update missiles (movement + bounds checking)
            // Note: When ECS is fully integrated, MovementSystem handles movement
            // but legacy updateMissiles still handles homing logic and bounds
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
                broadcastPlayerDied(playerId, roomCode, gameWorld);
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
        }
        // End of pause-skippable gameplay updates

        // Broadcast snapshot for this room (always, even when paused - shows pause state)
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
        MissileSpawned ms{
            .missile_id = missileId,
            .owner_id = ownerId,
            .x = static_cast<uint16_t>(m.x),
            .y = static_cast<uint16_t>(m.y)
        };
        broadcastToRoom(MessageType::MissileSpawned, ms, gameWorld);
    }

    void UDPServer::broadcastMissileDestroyed(uint16_t missileId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        MissileDestroyed md{.missile_id = missileId};
        broadcastToRoom(MessageType::MissileDestroyed, md, gameWorld);
    }

    void UDPServer::broadcastEnemyDestroyed(uint16_t enemyId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        EnemyDestroyed ed{.enemy_id = enemyId};
        broadcastToRoom(MessageType::EnemyDestroyed, ed, gameWorld);
    }

    void UDPServer::broadcastPlayerDamaged(uint8_t playerId, uint8_t damage, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        // Get current health from snapshot
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
        broadcastToRoom(MessageType::PlayerDamaged, pd, gameWorld);
    }

    void UDPServer::broadcastPlayerDied(uint8_t playerId, const std::string& roomCode,
                                          const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        PlayerDied pd{.player_id = playerId};
        broadcastToRoom(MessageType::PlayerDied, pd, gameWorld);

        // Save player stats on death (incremental save)
        savePlayerStatsOnDeath(playerId, roomCode, gameWorld);

        server::logging::Logger::getGameLogger()->info("Player {} died (stats saved)", static_cast<int>(playerId));
    }

    // ============================================================================
    // R-Type Authentic (Phase 3) Broadcast Methods
    // ============================================================================

    void UDPServer::broadcastWaveCannonFired(uint16_t waveCannonId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto wcOpt = gameWorld->getWaveCannon(waveCannonId);
        if (!wcOpt) return;

        const auto& wc = *wcOpt;
        WaveCannonState wcState{
            .id = wc.id,
            .owner_id = wc.owner_id,
            .x = static_cast<uint16_t>(wc.x),
            .y = static_cast<uint16_t>(wc.y),
            .charge_level = wc.chargeLevel,
            .width = static_cast<uint8_t>(wc.width)
        };
        broadcastToRoom(MessageType::WaveCannonFired, wcState, gameWorld);

        server::logging::Logger::getGameLogger()->debug("Wave Cannon {} fired by player {}",
            waveCannonId, static_cast<int>(wc.owner_id));
    }

    void UDPServer::broadcastPowerUpSpawned(uint16_t powerUpId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto puOpt = gameWorld->getPowerUp(powerUpId);
        if (!puOpt) return;

        const auto& pu = *puOpt;
        PowerUpState puState{
            .id = pu.id,
            .x = static_cast<uint16_t>(pu.x),
            .y = static_cast<uint16_t>(pu.y),
            .type = static_cast<uint8_t>(pu.type),
            .remaining_time = static_cast<uint8_t>(pu.lifetime)
        };
        broadcastToRoom(MessageType::PowerUpSpawned, puState, gameWorld);

        server::logging::Logger::getGameLogger()->debug("Power-up {} spawned (type {})",
            powerUpId, static_cast<int>(pu.type));
    }

    void UDPServer::broadcastPowerUpCollected(uint16_t powerUpId, uint8_t playerId, uint8_t powerUpType, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        PowerUpCollected pc{
            .powerup_id = powerUpId,
            .player_id = playerId,
            .powerup_type = powerUpType
        };
        broadcastToRoom(MessageType::PowerUpCollected, pc, gameWorld);

        server::logging::Logger::getGameLogger()->debug("Power-up {} collected by player {}",
            powerUpId, static_cast<int>(playerId));
    }

    void UDPServer::broadcastPowerUpExpired(uint16_t powerUpId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        PowerUpExpired pe{.powerup_id = powerUpId};
        broadcastToRoom(MessageType::PowerUpExpired, pe, gameWorld);

        server::logging::Logger::getGameLogger()->debug("Power-up {} expired", powerUpId);
    }

    void UDPServer::broadcastForceStateUpdate(uint8_t playerId, const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto forceOpt = gameWorld->getPlayerForce(playerId);
        if (!forceOpt) return;

        const auto& force = *forceOpt;
        ForceState fs{
            .owner_id = force.ownerId,
            .x = static_cast<uint16_t>(force.x),
            .y = static_cast<uint16_t>(force.y),
            .is_attached = force.isAttached ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0),
            .level = force.level
        };
        broadcastToRoom(MessageType::ForceStateUpdate, fs, gameWorld);

        server::logging::Logger::getGameLogger()->debug("Force state updated for player {}",
            static_cast<int>(playerId));
    }

    void UDPServer::broadcastPauseState(const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!gameWorld) return;

        auto [isPaused, voterCount, totalPlayers] = gameWorld->getPauseState();

        PauseStateSync pss{
            .isPaused = isPaused ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0),
            .pauseVoterCount = voterCount,
            .totalPlayerCount = totalPlayers
        };
        broadcastToRoom(MessageType::PauseStateSync, pss, gameWorld);

        server::logging::Logger::getGameLogger()->debug(
            "Pause state broadcast: isPaused={}, voters={}/{}",
            isPaused, voterCount, totalPlayers);
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

                    // Set player's GodMode state from session (hidden feature)
                    if (_sessionManager->isGodModeEnabled(email)) {
                        gameWorld->setPlayerGodMode(*playerIdOpt, true);
                    }

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
            // ═══════════════════════════════════════════════════════════════
            // Pause System: PauseRequest
            // ═══════════════════════════════════════════════════════════════
            else if (head.type == static_cast<uint16_t>(MessageType::PauseRequest)) {
                auto pauseReqOpt = PauseRequest::from_bytes(payload, payload_size);
                if (!pauseReqOpt) {
                    server::logging::Logger::getNetworkLogger()->warn(
                        "Invalid PauseRequest from player {}", playerId);
                } else {
                    bool wantsPause = (pauseReqOpt->wantsPause != 0);

                    boost::asio::post(gameWorld->getStrand(),
                        [this, gameWorld, playerId, wantsPause]() {
                            gameWorld->updatePlayerActivity(playerId);
                            gameWorld->setPauseVote(playerId, wantsPause);

                            // Broadcast pause state to all players
                            broadcastPauseState(gameWorld);
                        });
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

    void UDPServer::scheduleAutoSave() {
        _autoSaveTimer.expires_after(std::chrono::milliseconds(AUTO_SAVE_INTERVAL_MS));
        _autoSaveTimer.async_wait([this](boost::system::error_code ec) {
            if (ec) return;

            autoSaveAllPlayerStats();
            scheduleAutoSave();  // Reschedule
        });
    }

    void UDPServer::autoSaveAllPlayerStats() {
        if (!_leaderboardRepository || !_sessionManager) return;

        auto logger = server::logging::Logger::getGameLogger();
        auto roomCodes = _instanceManager.getActiveRoomCodes();

        for (const auto& roomCode : roomCodes) {
            auto gameWorld = _instanceManager.getInstance(roomCode);
            if (!gameWorld) continue;

            // Post to each room's strand for thread safety
            boost::asio::post(gameWorld->getStrand(),
                [this, roomCode, gameWorld, logger]() {
                    // Get all endpoints in this room
                    auto endpoints = gameWorld->getAllEndpoints();

                    for (const auto& endpoint : endpoints) {
                        std::string endpointStr = endpointToString(endpoint);

                        // Get player info from session
                        auto session = _sessionManager->getSessionByEndpoint(endpointStr);
                        if (!session || !session->playerId.has_value()) continue;

                        uint8_t playerId = *session->playerId;
                        const auto& scoreData = gameWorld->getPlayerScore(playerId);

                        // Only save if player has played (has score or kills)
                        if (scoreData.score == 0 && scoreData.kills == 0) continue;

                        // Create game history entry for current session save
                        application::ports::out::persistence::GameHistoryEntry historyEntry;
                        historyEntry.playerName = session->displayName;
                        historyEntry.score = scoreData.score;
                        historyEntry.wave = gameWorld->getWaveNumber();
                        historyEntry.kills = scoreData.kills;
                        historyEntry.deaths = scoreData.deaths;
                        historyEntry.duration = scoreData.getGameDurationSeconds();
                        historyEntry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count();
                        historyEntry.bossDefeated = scoreData.bossKills > 0;
                        historyEntry.standardKills = scoreData.standardKills;
                        historyEntry.spreadKills = scoreData.spreadKills;
                        historyEntry.laserKills = scoreData.laserKills;
                        historyEntry.missileKills = scoreData.missileKills;
                        historyEntry.waveCannonKills = scoreData.waveCannonKills;
                        historyEntry.bossKills = scoreData.bossKills;
                        historyEntry.bestCombo = scoreData.getMaxComboEncoded();
                        historyEntry.bestKillStreak = scoreData.bestKillStreak;
                        historyEntry.bestWaveStreak = scoreData.bestWaveStreak;
                        historyEntry.perfectWaves = scoreData.perfectWaves;
                        historyEntry.totalDamageDealt = scoreData.totalDamageDealt;
                        historyEntry.playerCount = static_cast<uint8_t>(gameWorld->getPlayerCount());

                        try {
                            // Save current game session (upsert - doesn't duplicate stats)
                            // This only updates the current_game_sessions collection
                            _leaderboardRepository->saveCurrentGameSession(
                                session->email, session->displayName, roomCode, historyEntry);
                            logger->debug("Auto-saved session for {} ({}): score={}, kills={}, wave={}, stdKills={}, spreadKills={}, laserKills={}, missileKills={}, waveCannonKills={}, dmg={}",
                                         session->displayName, static_cast<int>(playerId),
                                         scoreData.score, scoreData.kills, gameWorld->getWaveNumber(),
                                         scoreData.standardKills, scoreData.spreadKills, scoreData.laserKills,
                                         scoreData.missileKills, scoreData.waveCannonKills, scoreData.totalDamageDealt);
                        } catch (const std::exception& e) {
                            logger->error("Auto-save failed for {}: {}", session->displayName, e.what());
                        }
                    }
                });
        }
    }

    void UDPServer::savePlayerStatsOnDeath(uint8_t playerId, const std::string& roomCode,
                                           const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!_leaderboardRepository || !_sessionManager || !gameWorld) return;

        auto logger = server::logging::Logger::getGameLogger();

        // Find the endpoint for this player
        auto endpointOpt = gameWorld->getEndpointByPlayerId(playerId);
        if (!endpointOpt) return;

        std::string endpointStr = endpointToString(*endpointOpt);
        auto session = _sessionManager->getSessionByEndpoint(endpointStr);
        if (!session) return;

        const auto& scoreData = gameWorld->getPlayerScore(playerId);

        // Only save if player has played
        if (scoreData.score == 0 && scoreData.kills == 0) return;

        // Create game history entry for current session save
        application::ports::out::persistence::GameHistoryEntry historyEntry;
        historyEntry.playerName = session->displayName;
        historyEntry.score = scoreData.score;
        historyEntry.wave = gameWorld->getWaveNumber();
        historyEntry.kills = scoreData.kills;
        historyEntry.deaths = scoreData.deaths;
        historyEntry.duration = scoreData.getGameDurationSeconds();
        historyEntry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        historyEntry.bossDefeated = scoreData.bossKills > 0;
        historyEntry.standardKills = scoreData.standardKills;
        historyEntry.spreadKills = scoreData.spreadKills;
        historyEntry.laserKills = scoreData.laserKills;
        historyEntry.missileKills = scoreData.missileKills;
        historyEntry.waveCannonKills = scoreData.waveCannonKills;
        historyEntry.bossKills = scoreData.bossKills;
        historyEntry.bestCombo = scoreData.getMaxComboEncoded();
        historyEntry.bestKillStreak = scoreData.bestKillStreak;
        historyEntry.bestWaveStreak = scoreData.bestWaveStreak;
        historyEntry.perfectWaves = scoreData.perfectWaves;
        historyEntry.totalDamageDealt = scoreData.totalDamageDealt;
        historyEntry.playerCount = static_cast<uint8_t>(gameWorld->getPlayerCount());

        try {
            // Save current game session (upsert - same as auto-save)
            // This ensures the latest state is saved without duplicating stats
            // Final transfer to cumulative stats happens on disconnect (finalizeGameSession)
            _leaderboardRepository->saveCurrentGameSession(
                session->email, session->displayName, roomCode, historyEntry);

            logger->info("Saved session on death for {} ({}): score={}, kills={}, wave={}, deaths={}",
                        session->displayName, static_cast<int>(playerId),
                        scoreData.score, scoreData.kills, gameWorld->getWaveNumber(), scoreData.deaths);
        } catch (const std::exception& e) {
            logger->error("Failed to save session on death for {}: {}", session->displayName, e.what());
        }
    }

    void UDPServer::handlePlayerLeaveGame(uint8_t playerId, const std::string& roomCode, const std::string& endpoint,
                                           const std::string& email, const std::string& displayName) {
        auto logger = server::logging::Logger::getGameLogger();
        logger->info("handlePlayerLeaveGame called: playerId={}, roomCode={}, endpoint={}, email={}, displayName={}",
                     static_cast<int>(playerId), roomCode, endpoint, email, displayName);

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
            [this, gameWorld, playerId, roomCode, logger, email, displayName]() {
                // Save player stats BEFORE removing (so we still have the data)
                if (!email.empty() && _leaderboardRepository) {
                    savePlayerStats(playerId, email, displayName, gameWorld);
                }

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

    void UDPServer::savePlayerStats(uint8_t playerId, const std::string& email, const std::string& displayName,
                                     const std::shared_ptr<game::GameWorld>& gameWorld) {
        if (!_leaderboardRepository || !gameWorld) return;

        auto logger = server::logging::Logger::getGameLogger();
        logger->info("savePlayerStats called for {} (playerId={})", displayName, static_cast<int>(playerId));

        // Get player's score data
        const auto& scoreData = gameWorld->getPlayerScore(playerId);
        logger->info("savePlayerStats scoreData: score={}, kills={}, deaths={}, stdKills={}, missileKills={}",
                    scoreData.score, scoreData.kills, scoreData.deaths,
                    scoreData.standardKills, scoreData.missileKills);

        // Only save if player actually played (has score or kills)
        if (scoreData.score == 0 && scoreData.kills == 0) {
            logger->info("Player {} ({}) has no stats to save - calling finalizeGameSession anyway", static_cast<int>(playerId), displayName);
            // Still cleanup any empty session
            try {
                _leaderboardRepository->finalizeGameSession(email, displayName);
                logger->info("finalizeGameSession called (empty stats) for {}", displayName);
            } catch (const std::exception& e) {
                logger->error("finalizeGameSession failed (empty stats) for {}: {}", displayName, e.what());
            }
            return;
        }

        // Create leaderboard entry for submission
        application::ports::out::persistence::LeaderboardEntry entry;
        entry.playerName = displayName;
        entry.score = scoreData.score;
        entry.wave = gameWorld->getWaveNumber();
        entry.kills = scoreData.kills;
        entry.deaths = scoreData.deaths;
        entry.duration = scoreData.getGameDurationSeconds();
        entry.playerCount = static_cast<uint8_t>(gameWorld->getPlayerCount());
        entry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        try {
            // Submit to leaderboard (best score tracking)
            bool submitted = _leaderboardRepository->submitScore(email, displayName, entry);

            // Finalize the game session: transfers from current_game_sessions to
            // cumulative player_stats and game_history, then deletes the session
            _leaderboardRepository->finalizeGameSession(email, displayName);

            // Check and unlock achievements
            checkAndUnlockAchievements(email, scoreData, gameWorld->getWaveNumber());

            logger->info("Finalized stats for player {} ({}): score={}, kills={}, wave={}, duration={}s, submitted={}, stdKills={}, spreadKills={}, laserKills={}, missileKills={}, waveCannonKills={}, dmg={}",
                        static_cast<int>(playerId), displayName, scoreData.score, scoreData.kills,
                        gameWorld->getWaveNumber(), scoreData.getGameDurationSeconds(), submitted,
                        scoreData.standardKills, scoreData.spreadKills, scoreData.laserKills,
                        scoreData.missileKills, scoreData.waveCannonKills, scoreData.totalDamageDealt);
        } catch (const std::exception& e) {
            logger->error("Failed to finalize stats for player {}: {}", displayName, e.what());
        }
    }

    void UDPServer::checkAndUnlockAchievements(const std::string& email,
                                               const game::PlayerScore& scoreData,
                                               uint16_t wave) {
        if (!_leaderboardRepository) return;

        using AchievementType = application::ports::out::persistence::AchievementType;

        try {
            // Get current stats for cumulative checks
            auto statsOpt = _leaderboardRepository->getPlayerStats(email);
            if (!statsOpt) return;

            const auto& stats = *statsOpt;

            // First Blood - Get 1 kill (check current game)
            if (scoreData.kills >= 1 && !stats.hasAchievement(AchievementType::FirstBlood)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::FirstBlood);
            }

            // Exterminator - 1000 total kills
            if (stats.totalKills >= 1000 && !stats.hasAchievement(AchievementType::Exterminator)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::Exterminator);
            }

            // Combo Master - Achieve 3.0x combo
            if (scoreData.maxCombo >= 3.0f && !stats.hasAchievement(AchievementType::ComboMaster)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::ComboMaster);
            }

            // Boss Slayer - Kill any boss
            if (scoreData.bossKills > 0 && !stats.hasAchievement(AchievementType::BossSlayer)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::BossSlayer);
            }

            // Survivor - Reach wave 20 without dying
            if (wave >= 20 && scoreData.deaths == 0 && !stats.hasAchievement(AchievementType::Survivor)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::Survivor);
            }

            // Speed Demon - Wave 10 in under 5 minutes (300 seconds)
            if (wave >= 10 && scoreData.getGameDurationSeconds() < 300 && !stats.hasAchievement(AchievementType::SpeedDemon)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::SpeedDemon);
            }

            // Veteran - Play 100 games
            if (stats.gamesPlayed >= 100 && !stats.hasAchievement(AchievementType::Veteran)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::Veteran);
            }

            // Untouchable - Complete game with 0 deaths
            if (scoreData.deaths == 0 && wave >= 5 && !stats.hasAchievement(AchievementType::Untouchable)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::Untouchable);
            }

            // Weapon Master - 100+ kills with each weapon
            if (stats.standardKills >= 100 && stats.spreadKills >= 100 &&
                stats.laserKills >= 100 && stats.missileKills >= 100 &&
                !stats.hasAchievement(AchievementType::WeaponMaster)) {
                _leaderboardRepository->unlockAchievement(email, AchievementType::WeaponMaster);
            }

        } catch (const std::exception& e) {
            auto logger = server::logging::Logger::getGameLogger();
            logger->error("Failed to check achievements for {}: {}", email, e.what());
        }
    }
}
