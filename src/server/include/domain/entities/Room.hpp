/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Room entity for lobby system
*/

#ifndef ROOM_HPP_
#define ROOM_HPP_

#include <array>
#include <optional>
#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

namespace domain::entities {

// Chat message stored in room history
struct ChatMessage {
    std::string displayName;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

struct RoomSlot {
    bool occupied = false;
    std::string email;
    std::string displayName;
    bool isReady = false;
    bool isHost = false;
};

class Room {
public:
    static constexpr size_t MAX_SLOTS = 6;
    static constexpr size_t MIN_PLAYERS_TO_START = 2;
    static constexpr uint16_t MIN_GAME_SPEED_PERCENT = 50;
    static constexpr uint16_t MAX_GAME_SPEED_PERCENT = 200;
    static constexpr uint16_t DEFAULT_GAME_SPEED_PERCENT = 100;

    enum class State {
        Waiting,   // Waiting for players to join/ready
        Starting,  // Countdown in progress
        InGame,    // Game is running
        Closed     // Room is closed
    };

    Room(const std::string& name, const std::string& code,
         uint8_t maxPlayers, bool isPrivate);

    // Getters
    const std::string& getName() const;
    const std::string& getCode() const;
    uint8_t getMaxPlayers() const;
    bool isPrivate() const;
    State getState() const;
    uint16_t getGameSpeedPercent() const;

    // Game speed configuration (host only)
    void setGameSpeedPercent(uint16_t percent);

    // Player management
    std::optional<uint8_t> addPlayer(const std::string& email,
                                      const std::string& displayName);
    void removePlayer(const std::string& email);
    bool hasPlayer(const std::string& email) const;
    std::optional<uint8_t> getPlayerSlot(const std::string& email) const;
    uint8_t getPlayerCount() const;
    bool isFull() const;
    bool isEmpty() const;

    // Ready system
    void setPlayerReady(const std::string& email, bool ready);
    bool isPlayerReady(const std::string& email) const;
    uint8_t getReadyCount() const;
    bool canStart() const;

    // Host management
    std::string getHostEmail() const;
    bool isHost(const std::string& email) const;
    void promoteNextHost();

    // State transitions
    void startCountdown();
    void startGame();
    void endGame();
    void close();

    // Snapshot for RoomUpdate
    const std::array<RoomSlot, MAX_SLOTS>& getSlots() const;

    // Chat system
    void addChatMessage(const std::string& displayName, const std::string& message);
    const std::vector<ChatMessage>& getChatHistory() const;
    void clearChatHistory();

private:
    static constexpr size_t MAX_CHAT_HISTORY = 50;
    std::string _name;
    std::string _code;
    uint8_t _maxPlayers;
    bool _isPrivate;
    State _state = State::Waiting;
    std::string _hostEmail;
    std::array<RoomSlot, MAX_SLOTS> _slots;
    std::vector<ChatMessage> _chatHistory;
    uint16_t _gameSpeedPercent = DEFAULT_GAME_SPEED_PERCENT;

    std::optional<uint8_t> findEmptySlot() const;
    std::optional<uint8_t> findNextOccupiedSlot(uint8_t afterSlot) const;
};

} // namespace domain::entities

#endif /* !ROOM_HPP_ */
