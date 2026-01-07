/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RoomManager implementation
*/

#include "infrastructure/room/RoomManager.hpp"
#include <algorithm>
#include <cstring>

namespace infrastructure::room {

// Characters without ambiguity (no I/O/0/1)
static const char ROOM_CODE_CHARSET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
static constexpr size_t ROOM_CODE_LENGTH = 6;

std::string RoomManager::generateRoomCode() {
    std::uniform_int_distribution<> dist(0, sizeof(ROOM_CODE_CHARSET) - 2);
    std::string code;
    code.reserve(ROOM_CODE_LENGTH);

    do {
        code.clear();
        for (size_t i = 0; i < ROOM_CODE_LENGTH; ++i) {
            code.push_back(ROOM_CODE_CHARSET[dist(_rng)]);
        }
    } while (_roomsByCode.contains(code));

    return code;
}

std::optional<RoomManager::CreateResult> RoomManager::createRoom(
    const std::string& hostEmail,
    const std::string& hostDisplayName,
    const std::string& name,
    uint8_t maxPlayers,
    bool isPrivate)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check if player is already in a room
    if (_playerToRoom.contains(hostEmail)) {
        return std::nullopt;
    }

    // Generate unique code
    std::string code = generateRoomCode();

    // Create the room
    auto room = std::make_unique<domain::entities::Room>(name, code, maxPlayers, isPrivate);

    // Add host as first player
    auto slotOpt = room->addPlayer(hostEmail, hostDisplayName);
    if (!slotOpt) {
        return std::nullopt;
    }

    // Store room
    domain::entities::Room* roomPtr = room.get();
    _roomsByCode[code] = std::move(room);

    // Update player index
    _playerToRoom[hostEmail] = code;

    return CreateResult{.code = code, .room = roomPtr};
}

domain::entities::Room* RoomManager::getRoomByCode(const std::string& code) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _roomsByCode.find(code);
    return (it != _roomsByCode.end()) ? it->second.get() : nullptr;
}

const domain::entities::Room* RoomManager::getRoomByCode(const std::string& code) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _roomsByCode.find(code);
    return (it != _roomsByCode.end()) ? it->second.get() : nullptr;
}

domain::entities::Room* RoomManager::getRoomByPlayerEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto playerIt = _playerToRoom.find(email);
    if (playerIt == _playerToRoom.end()) {
        return nullptr;
    }
    auto roomIt = _roomsByCode.find(playerIt->second);
    return (roomIt != _roomsByCode.end()) ? roomIt->second.get() : nullptr;
}

const domain::entities::Room* RoomManager::getRoomByPlayerEmail(const std::string& email) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto playerIt = _playerToRoom.find(email);
    if (playerIt == _playerToRoom.end()) {
        return nullptr;
    }
    auto roomIt = _roomsByCode.find(playerIt->second);
    return (roomIt != _roomsByCode.end()) ? roomIt->second.get() : nullptr;
}

bool RoomManager::isPlayerInRoom(const std::string& email) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _playerToRoom.contains(email);
}

std::optional<RoomManager::JoinResult> RoomManager::joinRoomByCode(
    const std::string& code,
    const std::string& email,
    const std::string& displayName)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check if player is already in a room
    if (_playerToRoom.contains(email)) {
        return std::nullopt;
    }

    // Find room
    auto it = _roomsByCode.find(code);
    if (it == _roomsByCode.end()) {
        return std::nullopt;
    }

    domain::entities::Room* room = it->second.get();

    // Try to add player
    auto slotOpt = room->addPlayer(email, displayName);
    if (!slotOpt) {
        return std::nullopt;
    }

    // Update player index
    _playerToRoom[email] = code;

    return JoinResult{.room = room, .slotId = *slotOpt};
}

std::string RoomManager::leaveRoom(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto playerIt = _playerToRoom.find(email);
    if (playerIt == _playerToRoom.end()) {
        return "";
    }

    std::string code = playerIt->second;
    auto roomIt = _roomsByCode.find(code);
    if (roomIt != _roomsByCode.end()) {
        roomIt->second->removePlayer(email);

        // Clean up empty rooms
        if (roomIt->second->isEmpty()) {
            _roomsByCode.erase(roomIt);
        }
    }

    _playerToRoom.erase(playerIt);
    return code;
}

domain::entities::Room* RoomManager::setReady(const std::string& email, bool ready) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto playerIt = _playerToRoom.find(email);
    if (playerIt == _playerToRoom.end()) {
        return nullptr;
    }

    auto roomIt = _roomsByCode.find(playerIt->second);
    if (roomIt == _roomsByCode.end()) {
        return nullptr;
    }

    roomIt->second->setPlayerReady(email, ready);
    return roomIt->second.get();
}

bool RoomManager::tryStartGame(const std::string& hostEmail) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto playerIt = _playerToRoom.find(hostEmail);
    if (playerIt == _playerToRoom.end()) {
        return false;
    }

    auto roomIt = _roomsByCode.find(playerIt->second);
    if (roomIt == _roomsByCode.end()) {
        return false;
    }

    domain::entities::Room* room = roomIt->second.get();

    // Only host can start
    if (!room->isHost(hostEmail)) {
        return false;
    }

    // Check if game can start
    if (!room->canStart()) {
        return false;
    }

    room->startCountdown();
    return true;
}

domain::entities::Room* RoomManager::getStartingRoom(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto playerIt = _playerToRoom.find(email);
    if (playerIt == _playerToRoom.end()) {
        return nullptr;
    }

    auto roomIt = _roomsByCode.find(playerIt->second);
    if (roomIt == _roomsByCode.end()) {
        return nullptr;
    }

    if (roomIt->second->getState() == domain::entities::Room::State::Starting) {
        return roomIt->second.get();
    }

    return nullptr;
}

void RoomManager::removeRoom(const std::string& code) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto roomIt = _roomsByCode.find(code);
    if (roomIt == _roomsByCode.end()) {
        return;
    }

    // Remove all players from index
    const auto& slots = roomIt->second->getSlots();
    for (const auto& slot : slots) {
        if (slot.occupied) {
            _playerToRoom.erase(slot.email);
        }
    }

    _roomsByCode.erase(roomIt);
}

void RoomManager::cleanupEmptyRooms() {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<std::string> toRemove;
    for (const auto& [code, room] : _roomsByCode) {
        if (room->isEmpty() || room->getState() == domain::entities::Room::State::Closed) {
            toRemove.push_back(code);
        }
    }

    for (const auto& code : toRemove) {
        _roomsByCode.erase(code);
    }
}

std::vector<domain::entities::Room*> RoomManager::getAllRooms() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<domain::entities::Room*> rooms;
    rooms.reserve(_roomsByCode.size());
    for (auto& [code, room] : _roomsByCode) {
        rooms.push_back(room.get());
    }
    return rooms;
}

std::vector<const domain::entities::Room*> RoomManager::getAllRooms() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<const domain::entities::Room*> rooms;
    rooms.reserve(_roomsByCode.size());
    for (const auto& [code, room] : _roomsByCode) {
        rooms.push_back(room.get());
    }
    return rooms;
}

size_t RoomManager::getRoomCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _roomsByCode.size();
}

std::vector<std::string> RoomManager::getRoomMemberEmails(const std::string& code) const {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _roomsByCode.find(code);
    if (it == _roomsByCode.end()) {
        return {};
    }

    std::vector<std::string> emails;
    const auto& slots = it->second->getSlots();
    for (const auto& slot : slots) {
        if (slot.occupied) {
            emails.push_back(slot.email);
        }
    }
    return emails;
}

void RoomManager::removePlayerFromIndex(const std::string& email) {
    _playerToRoom.erase(email);
}

void RoomManager::addPlayerToIndex(const std::string& email, const std::string& code) {
    _playerToRoom[email] = code;
}

void RoomManager::registerSessionCallbacks(
    const std::string& email,
    RoomUpdateCallback onRoomUpdate,
    GameStartingCallback onGameStarting)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessionCallbacks.find(email);
    if (it != _sessionCallbacks.end()) {
        it->second.onRoomUpdate = std::move(onRoomUpdate);
        it->second.onGameStarting = std::move(onGameStarting);
    } else {
        _sessionCallbacks[email] = SessionCallbacks{
            .onRoomUpdate = std::move(onRoomUpdate),
            .onGameStarting = std::move(onGameStarting),
            .onPlayerKicked = nullptr
        };
    }
}

void RoomManager::registerKickedCallback(const std::string& email, PlayerKickedCallback cb)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessionCallbacks.find(email);
    if (it != _sessionCallbacks.end()) {
        it->second.onPlayerKicked = std::move(cb);
    } else {
        _sessionCallbacks[email] = SessionCallbacks{
            .onRoomUpdate = nullptr,
            .onGameStarting = nullptr,
            .onPlayerKicked = std::move(cb)
        };
    }
}

std::optional<RoomManager::KickResult> RoomManager::kickPlayer(
    const std::string& hostEmail,
    const std::string& targetEmail,
    const std::string& reason)
{
    PlayerKickedCallback kickedCallback;
    std::string roomCode;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        // Find the room the host is in
        auto hostRoomIt = _playerToRoom.find(hostEmail);
        if (hostRoomIt == _playerToRoom.end()) {
            return std::nullopt;  // Host not in any room
        }

        roomCode = hostRoomIt->second;
        auto roomIt = _roomsByCode.find(roomCode);
        if (roomIt == _roomsByCode.end()) {
            return std::nullopt;  // Room doesn't exist
        }

        domain::entities::Room* room = roomIt->second.get();

        // Verify the requester is the host
        if (!room->isHost(hostEmail)) {
            return std::nullopt;  // Only host can kick
        }

        // Verify target is in the same room
        auto targetRoomIt = _playerToRoom.find(targetEmail);
        if (targetRoomIt == _playerToRoom.end() || targetRoomIt->second != roomCode) {
            return std::nullopt;  // Target not in this room
        }

        // Can't kick yourself
        if (hostEmail == targetEmail) {
            return std::nullopt;
        }

        // Remove the player from the room
        room->removePlayer(targetEmail);
        _playerToRoom.erase(targetEmail);

        // Get the callback to notify the kicked player
        auto cbIt = _sessionCallbacks.find(targetEmail);
        if (cbIt != _sessionCallbacks.end() && cbIt->second.onPlayerKicked) {
            kickedCallback = cbIt->second.onPlayerKicked;
        }
    }

    // Notify the kicked player (outside lock)
    if (kickedCallback) {
        PlayerKickedNotification notif;
        std::strncpy(notif.reason, reason.c_str(), MAX_ERROR_MSG_LEN);
        notif.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
        kickedCallback(notif);
    }

    return KickResult{.roomCode = roomCode, .targetEmail = targetEmail};
}

void RoomManager::unregisterSessionCallbacks(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessionCallbacks.erase(email);
}

RoomUpdate RoomManager::buildRoomUpdate(const domain::entities::Room* room) const {
    RoomUpdate update{};

    std::strncpy(update.roomName, room->getName().c_str(), ROOM_NAME_LEN);
    update.roomName[ROOM_NAME_LEN - 1] = '\0';
    std::memcpy(update.roomCode, room->getCode().c_str(), ROOM_CODE_LEN);
    update.maxPlayers = room->getMaxPlayers();

    const auto& slots = room->getSlots();
    update.playerCount = 0;

    for (size_t i = 0; i < domain::entities::Room::MAX_SLOTS; ++i) {
        if (slots[i].occupied) {
            RoomPlayerState& state = update.players[update.playerCount];
            state.slotId = static_cast<uint8_t>(i);
            state.occupied = 1;
            std::strncpy(state.displayName, slots[i].displayName.c_str(), MAX_USERNAME_LEN);
            state.displayName[MAX_USERNAME_LEN - 1] = '\0';
            std::strncpy(state.email, slots[i].email.c_str(), MAX_EMAIL_LEN);
            state.email[MAX_EMAIL_LEN - 1] = '\0';
            state.isReady = slots[i].isReady ? 1 : 0;
            state.isHost = slots[i].isHost ? 1 : 0;
            ++update.playerCount;
        }
    }

    return update;
}

void RoomManager::broadcastRoomUpdate(domain::entities::Room* room) {
    if (!room) return;

    // Build update from room data
    RoomUpdate update = buildRoomUpdate(room);

    // Collect callbacks under lock, then call outside lock to avoid deadlock
    std::vector<RoomUpdateCallback> callbacksToCall;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto& slots = room->getSlots();
        for (const auto& slot : slots) {
            if (slot.occupied) {
                auto it = _sessionCallbacks.find(slot.email);
                if (it != _sessionCallbacks.end() && it->second.onRoomUpdate) {
                    callbacksToCall.push_back(it->second.onRoomUpdate);
                }
            }
        }
    }

    // Call callbacks outside lock
    for (const auto& callback : callbacksToCall) {
        callback(update);
    }
}

void RoomManager::broadcastGameStarting(domain::entities::Room* room, uint8_t countdown) {
    if (!room) return;

    GameStarting gs;
    gs.countdownSeconds = countdown;

    // Collect callbacks under lock, then call outside lock to avoid deadlock
    std::vector<GameStartingCallback> callbacksToCall;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto& slots = room->getSlots();
        for (const auto& slot : slots) {
            if (slot.occupied) {
                auto it = _sessionCallbacks.find(slot.email);
                if (it != _sessionCallbacks.end() && it->second.onGameStarting) {
                    callbacksToCall.push_back(it->second.onGameStarting);
                }
            }
        }
    }

    // Call callbacks outside lock
    for (const auto& callback : callbacksToCall) {
        callback(gs);
    }
}

std::vector<RoomManager::BrowserEntry> RoomManager::getPublicRooms() const {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<BrowserEntry> result;
    result.reserve(_roomsByCode.size());

    for (const auto& [code, room] : _roomsByCode) {
        // Only include public rooms that are not full and in Waiting state
        if (!room->isPrivate() &&
            !room->isFull() &&
            room->getState() == domain::entities::Room::State::Waiting) {
            result.push_back(BrowserEntry{
                .code = room->getCode(),
                .name = room->getName(),
                .currentPlayers = room->getPlayerCount(),
                .maxPlayers = room->getMaxPlayers()
            });
        }
    }

    return result;
}

std::optional<RoomManager::JoinResult> RoomManager::quickJoin(
    const std::string& email,
    const std::string& displayName)
{
    std::string selectedCode;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        // Check if player is already in a room
        if (_playerToRoom.contains(email)) {
            return std::nullopt;
        }

        // Collect eligible rooms
        std::vector<std::string> eligibleCodes;
        for (const auto& [code, room] : _roomsByCode) {
            if (!room->isPrivate() &&
                !room->isFull() &&
                room->getState() == domain::entities::Room::State::Waiting) {
                eligibleCodes.push_back(code);
            }
        }

        if (eligibleCodes.empty()) {
            return std::nullopt;
        }

        // Pick a random room
        std::uniform_int_distribution<size_t> dist(0, eligibleCodes.size() - 1);
        selectedCode = eligibleCodes[dist(_rng)];
    }

    // Use existing joinRoomByCode logic (will re-acquire lock)
    return joinRoomByCode(selectedCode, email, displayName);
}

} // namespace infrastructure::room
