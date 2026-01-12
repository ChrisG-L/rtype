/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IChatMessageRepository - Interface for chat message persistence
*/

#ifndef ICHATMESSAGEREPOSITORY_HPP_
#define ICHATMESSAGEREPOSITORY_HPP_

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace application::ports::out::persistence {

// Data transfer object for chat messages
struct ChatMessageData {
    std::string roomCode;
    std::string displayName;
    std::string message;
    std::chrono::system_clock::time_point timestamp;

    ChatMessageData() = default;

    ChatMessageData(const std::string& code, const std::string& name,
                    const std::string& msg, std::chrono::system_clock::time_point ts)
        : roomCode(code), displayName(name), message(msg), timestamp(ts) {}
};

class IChatMessageRepository {
public:
    virtual ~IChatMessageRepository() = default;

    /**
     * Save a chat message to the database
     * @param message The message data to save
     */
    virtual void save(const ChatMessageData& message) = 0;

    /**
     * Find all messages for a room, ordered by timestamp ascending
     * @param roomCode The room code to search for
     * @param limit Maximum number of messages to return (default 50)
     * @return Vector of messages, oldest first
     */
    virtual std::vector<ChatMessageData> findByRoomCode(const std::string& roomCode, size_t limit = 50) = 0;

    /**
     * Check if a room code has any chat history
     * @param roomCode The room code to check
     * @return true if there are messages for this room code
     */
    virtual bool hasHistoryForCode(const std::string& roomCode) = 0;

    /**
     * Delete all messages for a room code
     * @param roomCode The room code whose messages should be deleted
     */
    virtual void deleteByRoomCode(const std::string& roomCode) = 0;

    /**
     * Delete the oldest room's chat history (for code exhaustion scenario)
     * Finds the room with the oldest message and deletes all its messages
     */
    virtual void deleteOldestRoomHistory() = 0;
};

} // namespace application::ports::out::persistence

#endif /* !ICHATMESSAGEREPOSITORY_HPP_ */
