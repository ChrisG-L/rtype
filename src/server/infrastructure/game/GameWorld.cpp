/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameWorld
*/

#include "infrastructure/game/GameWorld.hpp"
#include <algorithm>

namespace infrastructure::game {

    GameWorld::GameWorld() : _nextPlayerId(1) {}

    uint8_t GameWorld::findAvailableId() const {
        for (uint8_t id = 1; id <= MAX_PLAYERS; ++id) {
            if (_players.find(id) == _players.end()) {
                return id;
            }
        }
        return 0;
    }

    std::optional<uint8_t> GameWorld::addPlayer(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);

        for (const auto& [id, player] : _players) {
            if (player.endpoint == endpoint) {
                return id;
            }
        }

        if (_players.size() >= MAX_PLAYERS) {
            return std::nullopt;
        }

        uint8_t newId = findAvailableId();
        if (newId == 0) {
            return std::nullopt;
        }

        uint16_t startX = 100;
        uint16_t startY = static_cast<uint16_t>(100 + (newId - 1) * 100);

        ConnectedPlayer player{
            .id = newId,
            .x = startX,
            .y = startY,
            .alive = true,
            .endpoint = endpoint
        };

        _players[newId] = player;
        return newId;
    }

    void GameWorld::removePlayer(uint8_t playerId) {
        std::lock_guard<std::mutex> lock(_mutex);
        _players.erase(playerId);
    }

    void GameWorld::removePlayerByEndpoint(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it = _players.begin(); it != _players.end(); ++it) {
            if (it->second.endpoint == endpoint) {
                _players.erase(it);
                return;
            }
        }
    }

    void GameWorld::movePlayer(uint8_t playerId, uint16_t x, uint16_t y) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _players.find(playerId);
        if (it != _players.end()) {
            it->second.x = x;
            it->second.y = y;
        }
    }

    std::optional<uint8_t> GameWorld::getPlayerIdByEndpoint(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& [id, player] : _players) {
            if (player.endpoint == endpoint) {
                return id;
            }
        }
        return std::nullopt;
    }

    GameSnapshot GameWorld::getSnapshot() const {
        std::lock_guard<std::mutex> lock(_mutex);
        GameSnapshot snapshot{};
        snapshot.player_count = 0;

        for (const auto& [id, player] : _players) {
            if (snapshot.player_count >= MAX_PLAYERS) break;
            snapshot.players[snapshot.player_count] = PlayerState{
                .id = player.id,
                .x = player.x,
                .y = player.y,
                .alive = static_cast<uint8_t>(player.alive ? 1 : 0)
            };
            snapshot.player_count++;
        }
        return snapshot;
    }

    std::vector<udp::endpoint> GameWorld::getAllEndpoints() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<udp::endpoint> endpoints;
        endpoints.reserve(_players.size());
        for (const auto& [id, player] : _players) {
            endpoints.push_back(player.endpoint);
        }
        return endpoints;
    }

    size_t GameWorld::getPlayerCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _players.size();
    }
}
