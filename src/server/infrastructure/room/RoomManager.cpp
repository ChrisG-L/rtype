/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RoomManager implementation
*/

#include "infrastructure/room/RoomManager.hpp"
#include "infrastructure/logging/Logger.hpp"  // server::logging::Logger
#include <algorithm>
#include <cstring>

namespace infrastructure::room {

using application::ports::out::persistence::ChatMessageData;

// Characters without ambiguity (no I/O/0/1)
static const char ROOM_CODE_CHARSET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
static constexpr size_t ROOM_CODE_LENGTH = 6;
static constexpr int MAX_CODE_GENERATION_ATTEMPTS = 1000;

RoomManager::RoomManager(std::shared_ptr<IChatMessageRepository> chatRepo)
    : _chatMessageRepository(std::move(chatRepo))
{
}

std::string RoomManager::generateRoomCode() {
    std::uniform_int_distribution<> dist(0, sizeof(ROOM_CODE_CHARSET) - 2);
    std::string code;
    code.reserve(ROOM_CODE_LENGTH);

    for (int attempt = 0; attempt < MAX_CODE_GENERATION_ATTEMPTS; ++attempt) {
        code.clear();
        for (size_t i = 0; i < ROOM_CODE_LENGTH; ++i) {
            code.push_back(ROOM_CODE_CHARSET[dist(_rng)]);
        }

        // Check if code is not in use by an active room
        if (_roomsByCode.contains(code)) {
            continue;
        }

        // Check if code has chat history in MongoDB (if repository available)
        if (_chatMessageRepository && _chatMessageRepository->hasHistoryForCode(code)) {
            continue;
        }

        return code;
    }

    // All codes exhausted - delete oldest chat history and retry
    auto logger = server::logging::Logger::getMainLogger();
    logger->error("All room codes exhausted after {} attempts! Deleting oldest chat history.",
                  MAX_CODE_GENERATION_ATTEMPTS);

    if (_chatMessageRepository) {
        _chatMessageRepository->deleteOldestRoomHistory();
    }

    // Generate a new code (should now be available)
    code.clear();
    for (size_t i = 0; i < ROOM_CODE_LENGTH; ++i) {
        code.push_back(ROOM_CODE_CHARSET[dist(_rng)]);
    }
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

size_t RoomManager::forceCloseRoom(const std::string& code) {
    // Collect kicked callbacks outside the lock to avoid deadlock
    std::vector<std::pair<PlayerKickedCallback, std::string>> callbacksToCall;
    size_t playerCount = 0;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto roomIt = _roomsByCode.find(code);
        if (roomIt == _roomsByCode.end()) {
            return 0;
        }

        // Notify and remove all players
        const auto& slots = roomIt->second->getSlots();
        for (const auto& slot : slots) {
            if (slot.occupied) {
                ++playerCount;
                _playerToRoom.erase(slot.email);

                // Collect callback for notification
                auto cbIt = _sessionCallbacks.find(slot.email);
                if (cbIt != _sessionCallbacks.end() && cbIt->second.onPlayerKicked) {
                    callbacksToCall.push_back({cbIt->second.onPlayerKicked, slot.email});
                }
            }
        }

        _roomsByCode.erase(roomIt);
    }

    // Send kicked notifications outside lock
    PlayerKickedNotification notif;
    std::strncpy(notif.reason, "Room closed by administrator", MAX_ERROR_MSG_LEN);
    notif.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
    for (const auto& [callback, email] : callbacksToCall) {
        callback(notif);
    }

    return playerCount;
}

std::string RoomManager::adminKickFromRoom(
    const std::string& code,
    const std::string& targetEmail,
    const std::string& reason)
{
    PlayerKickedCallback kickedCallback;
    std::string roomCode;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        // Find the room
        auto roomIt = _roomsByCode.find(code);
        if (roomIt == _roomsByCode.end()) {
            return "";  // Room not found
        }

        auto* room = roomIt->second.get();

        // Check if target is in the room
        if (!room->hasPlayer(targetEmail)) {
            return "";  // Player not in this room
        }

        // Can't kick the host (admin should use closeroom instead)
        if (room->isHost(targetEmail)) {
            return "";  // Can't kick the host
        }

        // Remove from room
        room->removePlayer(targetEmail);
        _playerToRoom.erase(targetEmail);
        roomCode = code;

        // Get callback for notification
        auto cbIt = _sessionCallbacks.find(targetEmail);
        if (cbIt != _sessionCallbacks.end() && cbIt->second.onPlayerKicked) {
            kickedCallback = cbIt->second.onPlayerKicked;
        }
    }

    // Notify the kicked player outside lock
    if (kickedCallback) {
        PlayerKickedNotification notif;
        std::string reasonStr = reason.empty() ? "Kicked by administrator" : reason;
        std::strncpy(notif.reason, reasonStr.c_str(), MAX_ERROR_MSG_LEN);
        notif.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
        kickedCallback(notif);
    }

    // Broadcast room update to remaining members
    auto* room = getRoomByCode(code);
    if (room && !room->isEmpty()) {
        broadcastRoomUpdate(room);
    }

    return roomCode;
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
            .onPlayerKicked = nullptr,
            .onChatMessage = nullptr
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
            .onPlayerKicked = std::move(cb),
            .onChatMessage = nullptr
        };
    }
}

void RoomManager::registerChatCallback(const std::string& email, ChatMessageCallback cb)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessionCallbacks.find(email);
    if (it != _sessionCallbacks.end()) {
        it->second.onChatMessage = std::move(cb);
    } else {
        _sessionCallbacks[email] = SessionCallbacks{
            .onRoomUpdate = nullptr,
            .onGameStarting = nullptr,
            .onPlayerKicked = nullptr,
            .onChatMessage = std::move(cb)
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
        std::string reasonStr = reason.empty() ? "Kicked by room host" : reason;
        std::strncpy(notif.reason, reasonStr.c_str(), MAX_ERROR_MSG_LEN);
        notif.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
        kickedCallback(notif);
    }

    return KickResult{.roomCode = roomCode, .targetEmail = targetEmail};
}

domain::entities::Room* RoomManager::setRoomGameSpeed(const std::string& hostEmail, uint16_t gameSpeedPercent) {
    std::lock_guard<std::mutex> lock(_mutex);

    // Find the room the host is in
    auto hostRoomIt = _playerToRoom.find(hostEmail);
    if (hostRoomIt == _playerToRoom.end()) {
        return nullptr;  // Host not in any room
    }

    auto roomIt = _roomsByCode.find(hostRoomIt->second);
    if (roomIt == _roomsByCode.end()) {
        return nullptr;  // Room doesn't exist
    }

    domain::entities::Room* room = roomIt->second.get();

    // Verify the requester is the host
    if (!room->isHost(hostEmail)) {
        return nullptr;  // Only host can change config
    }

    // Set the game speed (Room::setGameSpeedPercent handles clamping)
    room->setGameSpeedPercent(gameSpeedPercent);

    return room;
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
    update.gameSpeedPercent = room->getGameSpeedPercent();

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

// ============================================================================
// Chat System
// ============================================================================

bool RoomManager::sendChatMessage(const std::string& email, const std::string& message) {
    std::string displayName;
    std::string roomCode;
    domain::entities::Room* room = nullptr;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto playerIt = _playerToRoom.find(email);
        if (playerIt == _playerToRoom.end()) {
            return false;  // Player not in any room
        }

        roomCode = playerIt->second;
        auto roomIt = _roomsByCode.find(roomCode);
        if (roomIt == _roomsByCode.end()) {
            return false;  // Room doesn't exist
        }

        room = roomIt->second.get();

        // Get the player's display name
        auto slotOpt = room->getPlayerSlot(email);
        if (!slotOpt) {
            return false;
        }

        const auto& slots = room->getSlots();
        displayName = slots[*slotOpt].displayName;

        // Store the message in the room's history (in-memory)
        room->addChatMessage(displayName, message);
    }

    // Persist to MongoDB (outside lock)
    if (_chatMessageRepository) {
        auto logger = server::logging::Logger::getMainLogger();
        logger->debug("sendChatMessage: Saving to MongoDB - room={}, from={}, msg={}", roomCode, displayName, message);
        ChatMessageData data{
            roomCode,
            displayName,
            message,
            std::chrono::system_clock::now()
        };
        _chatMessageRepository->save(data);
        logger->debug("sendChatMessage: Message saved to MongoDB");
    }

    // Broadcast the message (outside lock)
    broadcastChatMessage(room, displayName, message);

    return true;
}

std::vector<domain::entities::ChatMessage> RoomManager::getChatHistory(const std::string& code) const {
    auto logger = server::logging::Logger::getMainLogger();

    // First, try to load from MongoDB if repository available
    if (_chatMessageRepository) {
        logger->debug("getChatHistory: Loading from MongoDB for room {}", code);
        auto dbMessages = _chatMessageRepository->findByRoomCode(code, MAX_CHAT_HISTORY);
        logger->debug("getChatHistory: Found {} messages in MongoDB for room {}", dbMessages.size(), code);
        if (!dbMessages.empty()) {
            std::vector<domain::entities::ChatMessage> result;
            result.reserve(dbMessages.size());
            for (const auto& msg : dbMessages) {
                result.push_back(domain::entities::ChatMessage{
                    msg.displayName,
                    msg.message,
                    msg.timestamp
                });
            }
            return result;
        }
    } else {
        logger->warn("getChatHistory: No MongoDB repository available, using in-memory only");
    }

    // Fallback to in-memory (existing behavior)
    logger->debug("getChatHistory: Fallback to in-memory for room {}", code);
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _roomsByCode.find(code);
    if (it == _roomsByCode.end()) {
        logger->debug("getChatHistory: Room {} not found in memory", code);
        return {};
    }

    auto history = it->second->getChatHistory();
    logger->debug("getChatHistory: Found {} messages in memory for room {}", history.size(), code);
    return history;
}

void RoomManager::broadcastChatMessage(domain::entities::Room* room, const std::string& displayName, const std::string& message) {
    if (!room) return;

    // Build the payload
    ChatMessagePayload payload{};
    std::strncpy(payload.displayName, displayName.c_str(), MAX_USERNAME_LEN);
    payload.displayName[MAX_USERNAME_LEN - 1] = '\0';
    std::strncpy(payload.message, message.c_str(), CHAT_MESSAGE_LEN);
    payload.message[CHAT_MESSAGE_LEN - 1] = '\0';
    payload.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // Collect callbacks under lock
    std::vector<ChatMessageCallback> callbacksToCall;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto& slots = room->getSlots();
        for (const auto& slot : slots) {
            if (slot.occupied) {
                auto it = _sessionCallbacks.find(slot.email);
                if (it != _sessionCallbacks.end() && it->second.onChatMessage) {
                    callbacksToCall.push_back(it->second.onChatMessage);
                }
            }
        }
    }

    // Call callbacks outside lock
    for (const auto& callback : callbacksToCall) {
        callback(payload);
    }
}

} // namespace infrastructure::room
