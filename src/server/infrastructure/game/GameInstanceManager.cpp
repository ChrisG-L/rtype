/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameInstanceManager implementation
*/

#include "infrastructure/game/GameInstanceManager.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace infrastructure::game {

GameWorld* GameInstanceManager::getOrCreateInstance(const std::string& roomCode) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _instances.find(roomCode);
    if (it != _instances.end()) {
        return it->second.get();
    }

    // Create new instance
    auto gameWorld = std::make_unique<GameWorld>();
    GameWorld* ptr = gameWorld.get();
    _instances.emplace(roomCode, std::move(gameWorld));

    server::logging::Logger::getGameLogger()->info(
        "GameInstanceManager: Created new game instance for room '{}'", roomCode);

    return ptr;
}

GameWorld* GameInstanceManager::getInstance(const std::string& roomCode) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _instances.find(roomCode);
    if (it != _instances.end()) {
        return it->second.get();
    }
    return nullptr;
}

void GameInstanceManager::removeInstance(const std::string& roomCode) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _instances.find(roomCode);
    if (it != _instances.end()) {
        server::logging::Logger::getGameLogger()->info(
            "GameInstanceManager: Removed game instance for room '{}'", roomCode);
        _instances.erase(it);
    }
}

bool GameInstanceManager::hasInstance(const std::string& roomCode) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _instances.contains(roomCode);
}

std::vector<std::string> GameInstanceManager::getActiveRoomCodes() const {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<std::string> codes;
    codes.reserve(_instances.size());
    for (const auto& [code, _] : _instances) {
        codes.push_back(code);
    }
    return codes;
}

size_t GameInstanceManager::getTotalPlayerCount() const {
    std::lock_guard<std::mutex> lock(_mutex);

    size_t total = 0;
    for (const auto& [_, instance] : _instances) {
        total += instance->getPlayerCount();
    }
    return total;
}

size_t GameInstanceManager::getInstanceCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _instances.size();
}

} // namespace infrastructure::game
