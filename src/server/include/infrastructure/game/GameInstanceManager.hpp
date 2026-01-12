/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameInstanceManager - Manages multiple GameWorld instances (one per room)
*/

#ifndef GAMEINSTANCEMANAGER_HPP_
#define GAMEINSTANCEMANAGER_HPP_

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "GameWorld.hpp"

namespace infrastructure::game {

/**
 * @brief Manages multiple GameWorld instances, one per room
 *
 * This class is the key to supporting multiple independent game instances.
 * Each room gets its own GameWorld, ensuring complete isolation:
 * - Players in Room A only see players in Room A
 * - Snapshots are broadcast only to players in the same room
 * - Game state (missiles, enemies, etc.) is completely separate
 */
class GameInstanceManager {
public:
    GameInstanceManager() = default;
    ~GameInstanceManager() = default;

    // Disable copy (owns shared_ptrs)
    GameInstanceManager(const GameInstanceManager&) = delete;
    GameInstanceManager& operator=(const GameInstanceManager&) = delete;

    /**
     * @brief Get or create a GameWorld instance for a room
     * @param roomCode The unique room code
     * @return shared_ptr to the GameWorld (never null after call)
     *
     * Creates the instance lazily on first access.
     * Thread-safe. The returned shared_ptr keeps the instance alive
     * even if removeInstance is called while it's being used.
     */
    std::shared_ptr<GameWorld> getOrCreateInstance(const std::string& roomCode);

    /**
     * @brief Get an existing GameWorld instance
     * @param roomCode The room code to look up
     * @return shared_ptr to GameWorld, or nullptr if not found
     *
     * Thread-safe. The returned shared_ptr keeps the instance alive
     * even if removeInstance is called while it's being used.
     */
    std::shared_ptr<GameWorld> getInstance(const std::string& roomCode);

    /**
     * @brief Remove a GameWorld instance
     * @param roomCode The room code of the instance to remove
     *
     * Called when a room becomes empty or is closed.
     * Thread-safe.
     */
    void removeInstance(const std::string& roomCode);

    /**
     * @brief Check if an instance exists for a room
     * @param roomCode The room code to check
     * @return true if instance exists
     */
    bool hasInstance(const std::string& roomCode) const;

    /**
     * @brief Get all active room codes
     * @return Vector of room codes with active GameWorld instances
     *
     * Used by the broadcast loop to iterate over all instances.
     * Thread-safe.
     */
    std::vector<std::string> getActiveRoomCodes() const;

    /**
     * @brief Get total player count across all instances
     * @return Total number of players in all game instances
     */
    size_t getTotalPlayerCount() const;

    /**
     * @brief Get number of active instances
     * @return Number of GameWorld instances currently managed
     */
    size_t getInstanceCount() const;

private:
    mutable std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<GameWorld>> _instances;
};

} // namespace infrastructure::game

#endif /* !GAMEINSTANCEMANAGER_HPP_ */
