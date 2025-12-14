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

namespace infrastructure::game {
    using boost::asio::ip::udp;

    struct ConnectedPlayer {
        uint8_t id;
        uint16_t x;
        uint16_t y;
        bool alive;
        udp::endpoint endpoint;
    };

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

    private:
        std::unordered_map<uint8_t, ConnectedPlayer> _players;
        mutable std::mutex _mutex;
        uint8_t _nextPlayerId;

        uint8_t findAvailableId() const;
    };
}

#endif /* !GAMEWORLD_HPP_ */
