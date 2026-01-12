/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#ifndef UDPSERVER_HPP_
#define UDPSERVER_HPP_

#include <array>
#include <boost/asio.hpp>
#include <unordered_set>
#include "Protocol.hpp"
#include "infrastructure/game/GameWorld.hpp"
#include "infrastructure/game/GameInstanceManager.hpp"
#include "infrastructure/session/SessionManager.hpp"
#include <memory>


namespace infrastructure::adapters::in::network {
    using boost::asio::ip::udp;
    using infrastructure::session::SessionManager;

    class UDPServer {
        private:
            // Message types that require authentication (session bound to endpoint)
            // Messages NOT in this set can be processed without authentication
            static inline const std::unordered_set<uint16_t> _authRequiredMessages = {
                static_cast<uint16_t>(MessageType::PlayerInput),
                static_cast<uint16_t>(MessageType::ShootMissile),
            };

            // Check if a message type requires authentication
            bool requiresAuth(uint16_t messageType) const {
                return _authRequiredMessages.contains(messageType);
            }
            boost::asio::io_context& _io_ctx;
            udp::socket _socket;
            udp::endpoint _remote_endpoint;
            game::GameInstanceManager _instanceManager;
            std::shared_ptr<SessionManager> _sessionManager;
            boost::asio::steady_timer _broadcastTimer;

            char _readBuffer[BUFFER_SIZE];

            void sendTo(const udp::endpoint& endpoint, const void* data, size_t size);
            void sendPlayerJoin(const udp::endpoint& endpoint, uint8_t playerId, game::GameWorld* gameWorld);
            void sendPlayerLeave(uint8_t playerId, game::GameWorld* gameWorld);
            void sendHeartbeatAck(const udp::endpoint& endpoint);
            void sendJoinGameAck(const udp::endpoint& endpoint, uint8_t playerId);
            void sendJoinGameNack(const udp::endpoint& endpoint, const std::string& reason);
            void broadcastSnapshotForRoom(const std::string& roomCode, game::GameWorld* gameWorld);
            void broadcastAllSnapshots();
            void broadcastMissileSpawned(uint16_t missileId, uint8_t ownerId, game::GameWorld* gameWorld);
            void broadcastMissileDestroyed(uint16_t missileId, game::GameWorld* gameWorld);
            void broadcastEnemyDestroyed(uint16_t enemyId, game::GameWorld* gameWorld);
            void broadcastPlayerDamaged(uint8_t playerId, uint8_t damage, game::GameWorld* gameWorld);
            void broadcastPlayerDied(uint8_t playerId, game::GameWorld* gameWorld);
            void scheduleBroadcast();
            void updateAndBroadcastRoom(const std::string& roomCode, game::GameWorld* gameWorld, float deltaTime);

            void do_read();
            void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

            // Helper to convert endpoint to string for SessionManager
            std::string endpointToString(const udp::endpoint& ep) const;

        public:
            UDPServer(boost::asio::io_context& io_ctx, std::shared_ptr<SessionManager> sessionManager);
            void start();
            void run();
            void stop();

            // CLI support: force disconnect a player
            void kickPlayer(uint8_t playerId);

            // CLI support: get player count
            size_t getPlayerCount() const;
    };
}
#endif /* !UDPSERVER_HPP_ */
