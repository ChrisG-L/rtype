/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IPrivateMessageRepository - Interface for private message persistence
*/

#ifndef IPRIVATEMESSAGEREPOSITORY_HPP_
#define IPRIVATEMESSAGEREPOSITORY_HPP_

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace application::ports::out::persistence {

struct PrivateMessageData {
    uint64_t id;                // MongoDB ObjectId as uint64
    std::string senderEmail;
    std::string recipientEmail;
    std::string senderDisplayName;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    bool isRead;
};

struct ConversationSummaryData {
    std::string otherEmail;
    std::string otherDisplayName;
    std::string lastMessage;
    std::chrono::system_clock::time_point lastTimestamp;
    uint8_t unreadCount;
};

class IPrivateMessageRepository {
public:
    virtual ~IPrivateMessageRepository() = default;

    /**
     * Saves a private message
     * @param senderEmail Sender's email
     * @param recipientEmail Recipient's email
     * @param senderDisplayName Sender's display name
     * @param message Message content
     * @return Unique message ID
     */
    virtual uint64_t saveMessage(
        const std::string& senderEmail,
        const std::string& recipientEmail,
        const std::string& senderDisplayName,
        const std::string& message) = 0;

    /**
     * Gets conversation history between two users
     * @param email1 First participant
     * @param email2 Second participant
     * @param offset Pagination offset
     * @param limit Max messages (default 50)
     * @return Messages sorted by timestamp descending (newest first)
     */
    virtual std::vector<PrivateMessageData> getConversation(
        const std::string& email1,
        const std::string& email2,
        size_t offset = 0,
        size_t limit = 50) = 0;

    /**
     * Gets list of conversations for a user
     * @param email User's email
     * @param limit Max conversations
     * @return Summaries sorted by last message timestamp
     */
    virtual std::vector<ConversationSummaryData> getConversationsList(
        const std::string& email,
        size_t limit = 50) = 0;

    /**
     * Marks messages in a conversation as read
     * @param readerEmail Who is reading
     * @param senderEmail Who sent the messages
     */
    virtual void markAsRead(
        const std::string& readerEmail,
        const std::string& senderEmail) = 0;

    /**
     * Counts total unread messages for a user
     * @param email Recipient's email
     * @return Total unread count
     */
    virtual size_t getUnreadCount(const std::string& email) = 0;

    /**
     * Counts unread messages from a specific sender
     * @param recipientEmail Recipient's email
     * @param senderEmail Sender's email
     * @return Unread count from this sender
     */
    virtual size_t getUnreadCountFrom(
        const std::string& recipientEmail,
        const std::string& senderEmail) = 0;

    // ============ Admin functions ============

    /**
     * [ADMIN] Gets all messages involving a user (sent or received)
     * @param email User's email
     * @param limit Max messages
     * @return All messages sorted by timestamp descending
     */
    virtual std::vector<PrivateMessageData> getMessagesByUser(
        const std::string& email,
        size_t limit = 100) = 0;

    /**
     * [ADMIN] Gets all messages in the system (for monitoring)
     * @param limit Max messages
     * @param beforeTimestamp Only get messages before this timestamp (0 = no filter)
     * @return All messages sorted by timestamp descending
     */
    virtual std::vector<PrivateMessageData> getAllMessages(
        size_t limit = 100,
        uint64_t beforeTimestamp = 0) = 0;

    /**
     * [ADMIN] Search messages by content
     * @param searchTerm Term to search for
     * @param limit Max messages
     * @return Matching messages
     */
    virtual std::vector<PrivateMessageData> searchMessages(
        const std::string& searchTerm,
        size_t limit = 50) = 0;

    /**
     * [ADMIN] Gets statistics about private messages
     * @return Pair of (total messages, total conversations)
     */
    virtual std::pair<size_t, size_t> getMessageStats() = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IPRIVATEMESSAGEREPOSITORY_HPP_ */
