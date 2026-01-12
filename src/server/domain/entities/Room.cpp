/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Room entity implementation
*/

#include "domain/entities/Room.hpp"
#include <algorithm>

namespace domain::entities {

Room::Room(const std::string& name, const std::string& code,
           uint8_t maxPlayers, bool isPrivate)
    : _name(name)
    , _code(code)
    , _maxPlayers(maxPlayers)
    , _isPrivate(isPrivate)
    , _state(State::Waiting)
{
    // Initialize all slots as empty
    for (auto& slot : _slots) {
        slot = RoomSlot{};
    }
}

const std::string& Room::getName() const {
    return _name;
}

const std::string& Room::getCode() const {
    return _code;
}

uint8_t Room::getMaxPlayers() const {
    return _maxPlayers;
}

bool Room::isPrivate() const {
    return _isPrivate;
}

Room::State Room::getState() const {
    return _state;
}

uint16_t Room::getGameSpeedPercent() const {
    return _gameSpeedPercent;
}

void Room::setGameSpeedPercent(uint16_t percent) {
    _gameSpeedPercent = std::clamp(percent, MIN_GAME_SPEED_PERCENT, MAX_GAME_SPEED_PERCENT);
}

std::optional<uint8_t> Room::addPlayer(const std::string& email,
                                        const std::string& displayName,
                                        uint8_t shipSkin) {
    if (hasPlayer(email)) {
        return std::nullopt;
    }
    if (isFull()) {
        return std::nullopt;
    }
    if (_state != State::Waiting) {
        return std::nullopt;
    }

    auto slotOpt = findEmptySlot();
    if (!slotOpt) {
        return std::nullopt;
    }

    uint8_t slotId = *slotOpt;
    bool isFirstPlayer = isEmpty();

    _slots[slotId].occupied = true;
    _slots[slotId].email = email;
    _slots[slotId].displayName = displayName;
    _slots[slotId].isReady = isFirstPlayer;  // Host is always ready
    _slots[slotId].isHost = isFirstPlayer;
    _slots[slotId].shipSkin = shipSkin;

    if (isFirstPlayer) {
        _hostEmail = email;
    }

    return slotId;
}

void Room::removePlayer(const std::string& email) {
    auto slotOpt = getPlayerSlot(email);
    if (!slotOpt) {
        return;
    }

    uint8_t slotId = *slotOpt;
    bool wasHost = _slots[slotId].isHost;

    _slots[slotId] = RoomSlot{};

    if (wasHost && !isEmpty()) {
        promoteNextHost();
    }

    if (isEmpty()) {
        _hostEmail.clear();
    }
}

bool Room::hasPlayer(const std::string& email) const {
    return getPlayerSlot(email).has_value();
}

std::optional<uint8_t> Room::getPlayerSlot(const std::string& email) const {
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (_slots[i].occupied && _slots[i].email == email) {
            return i;
        }
    }
    return std::nullopt;
}

uint8_t Room::getPlayerCount() const {
    uint8_t count = 0;
    for (const auto& slot : _slots) {
        if (slot.occupied) {
            ++count;
        }
    }
    return count;
}

bool Room::isFull() const {
    return getPlayerCount() >= _maxPlayers;
}

bool Room::isEmpty() const {
    return getPlayerCount() == 0;
}

void Room::setPlayerReady(const std::string& email, bool ready) {
    auto slotOpt = getPlayerSlot(email);
    if (slotOpt) {
        _slots[*slotOpt].isReady = ready;
    }
}

bool Room::isPlayerReady(const std::string& email) const {
    auto slotOpt = getPlayerSlot(email);
    if (slotOpt) {
        return _slots[*slotOpt].isReady;
    }
    return false;
}

uint8_t Room::getReadyCount() const {
    uint8_t count = 0;
    for (const auto& slot : _slots) {
        if (slot.occupied && slot.isReady) {
            ++count;
        }
    }
    return count;
}

bool Room::canStart() const {
    return _state == State::Waiting &&
           getPlayerCount() >= MIN_PLAYERS_TO_START &&
           getReadyCount() >= MIN_PLAYERS_TO_START;
}

std::string Room::getHostEmail() const {
    return _hostEmail;
}

bool Room::isHost(const std::string& email) const {
    return !_hostEmail.empty() && _hostEmail == email;
}

void Room::promoteNextHost() {
    // Find the first occupied slot after the current host
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (_slots[i].occupied) {
            _slots[i].isHost = true;
            _slots[i].isReady = true;  // New host is always ready
            _hostEmail = _slots[i].email;
            return;
        }
    }
    _hostEmail.clear();
}

void Room::startCountdown() {
    if (_state == State::Waiting && canStart()) {
        _state = State::Starting;
    }
}

void Room::startGame() {
    if (_state == State::Starting || _state == State::Waiting) {
        _state = State::InGame;
    }
}

void Room::endGame() {
    if (_state == State::InGame) {
        _state = State::Waiting;
        // Reset ready status for all players
        for (auto& slot : _slots) {
            if (slot.occupied) {
                slot.isReady = false;
            }
        }
    }
}

void Room::close() {
    _state = State::Closed;
}

const std::array<RoomSlot, Room::MAX_SLOTS>& Room::getSlots() const {
    return _slots;
}

std::optional<uint8_t> Room::findEmptySlot() const {
    for (uint8_t i = 0; i < _maxPlayers && i < MAX_SLOTS; ++i) {
        if (!_slots[i].occupied) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<uint8_t> Room::findNextOccupiedSlot(uint8_t afterSlot) const {
    for (uint8_t i = afterSlot + 1; i < MAX_SLOTS; ++i) {
        if (_slots[i].occupied) {
            return i;
        }
    }
    // Wrap around
    for (uint8_t i = 0; i <= afterSlot && i < MAX_SLOTS; ++i) {
        if (_slots[i].occupied) {
            return i;
        }
    }
    return std::nullopt;
}

// ============================================================================
// Chat System
// ============================================================================

void Room::addChatMessage(const std::string& displayName, const std::string& message) {
    ChatMessage chatMsg;
    chatMsg.displayName = displayName;
    chatMsg.message = message;
    chatMsg.timestamp = std::chrono::system_clock::now();

    _chatHistory.push_back(std::move(chatMsg));

    // Keep only last MAX_CHAT_HISTORY messages
    if (_chatHistory.size() > MAX_CHAT_HISTORY) {
        _chatHistory.erase(_chatHistory.begin());
    }
}

const std::vector<ChatMessage>& Room::getChatHistory() const {
    return _chatHistory;
}

void Room::clearChatHistory() {
    _chatHistory.clear();
}

} // namespace domain::entities
