/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RoomManager - Manages game rooms/lobbies
*/

#ifndef ROOMMANAGER_HPP_
#define ROOMMANAGER_HPP_

#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <random>
#include <vector>
#include <functional>
#include "domain/entities/Room.hpp"
#include "Protocol.hpp"

namespace infrastructure::room {

class RoomManager {
public:
    RoomManager() = default;
    ~RoomManager() = default;

    // ═══════════════════════════════════════════════════════════════════
    // Room Creation
    // ═══════════════════════════════════════════════════════════════════

    struct CreateResult {
        std::string code;
        domain::entities::Room* room;
    };

    // Creates a room and adds the host as first player
    // Returns nullopt if player is already in a room
    std::optional<CreateResult> createRoom(
        const std::string& hostEmail,
        const std::string& hostDisplayName,
        const std::string& name,
        uint8_t maxPlayers,
        bool isPrivate);

    // ═══════════════════════════════════════════════════════════════════
    // Room Lookup
    // ═══════════════════════════════════════════════════════════════════

    // Get room by code
    domain::entities::Room* getRoomByCode(const std::string& code);
    const domain::entities::Room* getRoomByCode(const std::string& code) const;

    // Get room by player email
    domain::entities::Room* getRoomByPlayerEmail(const std::string& email);
    const domain::entities::Room* getRoomByPlayerEmail(const std::string& email) const;

    // Check if player is in any room
    bool isPlayerInRoom(const std::string& email) const;

    // ═══════════════════════════════════════════════════════════════════
    // Join/Leave Operations
    // ═══════════════════════════════════════════════════════════════════

    struct JoinResult {
        domain::entities::Room* room;
        uint8_t slotId;
    };

    // Join a room by code
    // Returns nullopt if room doesn't exist, is full, or player already in a room
    std::optional<JoinResult> joinRoomByCode(
        const std::string& code,
        const std::string& email,
        const std::string& displayName);

    // Leave current room
    // Returns the room code that was left (for cleanup), empty if not in room
    std::string leaveRoom(const std::string& email);

    // ═══════════════════════════════════════════════════════════════════
    // Ready System
    // ═══════════════════════════════════════════════════════════════════

    // Set player ready status
    // Returns the room for broadcasting, nullptr if not in room
    domain::entities::Room* setReady(const std::string& email, bool ready);

    // ═══════════════════════════════════════════════════════════════════
    // Kick System (Phase 2)
    // ═══════════════════════════════════════════════════════════════════

    struct KickResult {
        std::string roomCode;
        std::string targetEmail;
    };

    // Host kicks a player from the room
    // Returns the room code and target email if successful, nullopt otherwise
    std::optional<KickResult> kickPlayer(
        const std::string& hostEmail,
        const std::string& targetEmail,
        const std::string& reason);

    using PlayerKickedCallback = std::function<void(const PlayerKickedNotification&)>;

    // Register kicked callback for a player's session
    void registerKickedCallback(const std::string& email, PlayerKickedCallback cb);

    // ═══════════════════════════════════════════════════════════════════
    // Room Browser (Phase 2)
    // ═══════════════════════════════════════════════════════════════════

    struct BrowserEntry {
        std::string code;
        std::string name;
        uint8_t currentPlayers;
        uint8_t maxPlayers;
    };

    // Get list of public rooms that are not full and in Waiting state
    std::vector<BrowserEntry> getPublicRooms() const;

    // Quick join: find and join a random public room
    // Returns same as joinRoomByCode if successful
    std::optional<JoinResult> quickJoin(
        const std::string& email,
        const std::string& displayName);

    // ═══════════════════════════════════════════════════════════════════
    // Game Start
    // ═══════════════════════════════════════════════════════════════════

    // Try to start the game (only host can start)
    // Returns true if started, false if conditions not met
    bool tryStartGame(const std::string& hostEmail);

    // Get room where game is starting (for countdown)
    domain::entities::Room* getStartingRoom(const std::string& email);

    // ═══════════════════════════════════════════════════════════════════
    // Cleanup
    // ═══════════════════════════════════════════════════════════════════

    // Remove a room by code
    void removeRoom(const std::string& code);

    // Clean up empty or closed rooms
    void cleanupEmptyRooms();

    // ═══════════════════════════════════════════════════════════════════
    // Accessors (for CLI/debugging)
    // ═══════════════════════════════════════════════════════════════════

    // Get all rooms
    std::vector<domain::entities::Room*> getAllRooms();
    std::vector<const domain::entities::Room*> getAllRooms() const;

    // Get room count
    size_t getRoomCount() const;

    // Get all emails in a room (for broadcasting)
    std::vector<std::string> getRoomMemberEmails(const std::string& code) const;

    // ═══════════════════════════════════════════════════════════════════
    // Session Callbacks (for TCP broadcast)
    // ═══════════════════════════════════════════════════════════════════

    using RoomUpdateCallback = std::function<void(const RoomUpdate&)>;
    using GameStartingCallback = std::function<void(const GameStarting&)>;

    // Register callbacks for a player's session
    void registerSessionCallbacks(
        const std::string& email,
        RoomUpdateCallback onRoomUpdate,
        GameStartingCallback onGameStarting);

    // Unregister callbacks when session closes
    void unregisterSessionCallbacks(const std::string& email);

    // Broadcast to all room members
    void broadcastRoomUpdate(domain::entities::Room* room);
    void broadcastGameStarting(domain::entities::Room* room, uint8_t countdown);

private:
    mutable std::mutex _mutex;

    // Primary storage: code -> Room
    std::unordered_map<std::string, std::unique_ptr<domain::entities::Room>> _roomsByCode;

    // Secondary index: email -> room code (player can only be in one room)
    std::unordered_map<std::string, std::string> _playerToRoom;

    // Code generation
    std::string generateRoomCode();
    std::random_device _rd;
    std::mt19937 _rng{_rd()};

    // Internal helpers
    void removePlayerFromIndex(const std::string& email);
    void addPlayerToIndex(const std::string& email, const std::string& code);

    // Session callbacks for broadcasting
    struct SessionCallbacks {
        RoomUpdateCallback onRoomUpdate;
        GameStartingCallback onGameStarting;
        PlayerKickedCallback onPlayerKicked;
    };
    std::unordered_map<std::string, SessionCallbacks> _sessionCallbacks;

    // Build RoomUpdate from Room
    RoomUpdate buildRoomUpdate(const domain::entities::Room* room) const;
};

} // namespace infrastructure::room

#endif /* !ROOMMANAGER_HPP_ */
