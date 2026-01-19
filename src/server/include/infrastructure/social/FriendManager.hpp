/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendManager - Manages real-time friend notifications
*/

#ifndef FRIENDMANAGER_HPP_
#define FRIENDMANAGER_HPP_

#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "Protocol.hpp"

namespace infrastructure::social {

// Callback types for friend notifications
using FriendRequestReceivedCallback = std::function<void(
    const std::string& fromEmail,
    const std::string& fromDisplayName)>;

using FriendRequestAcceptedCallback = std::function<void(
    const std::string& friendEmail,
    const std::string& friendDisplayName,
    uint8_t onlineStatus)>;

using FriendRemovedCallback = std::function<void(const std::string& friendEmail)>;

using FriendStatusChangedCallback = std::function<void(
    const std::string& friendEmail,
    uint8_t newStatus,
    const std::string& roomCode)>;

using PrivateMessageReceivedCallback = std::function<void(
    const std::string& senderEmail,
    const std::string& senderDisplayName,
    const std::string& message,
    uint64_t timestamp)>;

using MessagesReadNotificationCallback = std::function<void(
    const std::string& readerEmail)>;

struct FriendCallbacks {
    FriendRequestReceivedCallback onRequestReceived;
    FriendRequestAcceptedCallback onRequestAccepted;
    FriendRemovedCallback onFriendRemoved;
    FriendStatusChangedCallback onStatusChanged;
    PrivateMessageReceivedCallback onPrivateMessage;
    MessagesReadNotificationCallback onMessagesRead;
};

class FriendManager {
public:
    FriendManager() = default;
    ~FriendManager() = default;

    // Callback registration (called by TCPAuthServer Session on login)
    void registerCallbacks(const std::string& email, const FriendCallbacks& callbacks);

    // Unregister callbacks (called by TCPAuthServer Session on disconnect)
    void unregisterCallbacks(const std::string& email);

    // Check if a user has registered callbacks (is online)
    bool isUserOnline(const std::string& email) const;

    // ═══════════════════════════════════════════════════════════════════
    // Notification methods (called by handlers)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Notifies a user that they received a friend request
     * @param targetEmail Who receives the notification
     * @param fromEmail Who sent the request
     * @param fromDisplayName Sender's display name
     */
    void notifyFriendRequestReceived(
        const std::string& targetEmail,
        const std::string& fromEmail,
        const std::string& fromDisplayName);

    /**
     * Notifies a user that their friend request was accepted
     * @param targetEmail Who receives the notification (original requester)
     * @param friendEmail Who accepted the request
     * @param friendDisplayName Acceptor's display name
     * @param onlineStatus Acceptor's current online status
     */
    void notifyFriendRequestAccepted(
        const std::string& targetEmail,
        const std::string& friendEmail,
        const std::string& friendDisplayName,
        uint8_t onlineStatus);

    /**
     * Notifies a user that they were removed from someone's friend list
     * @param targetEmail Who receives the notification
     * @param friendEmail Who removed them
     */
    void notifyFriendRemoved(
        const std::string& targetEmail,
        const std::string& friendEmail);

    /**
     * Notifies multiple users about a friend's status change
     * @param friendEmails List of friends to notify
     * @param changedEmail Who changed status
     * @param newStatus New online status
     * @param roomCode Room code if in game/lobby
     */
    void notifyFriendStatusChanged(
        const std::vector<std::string>& friendEmails,
        const std::string& changedEmail,
        uint8_t newStatus,
        const std::string& roomCode);

    /**
     * Notifies a user about a received private message
     * @param recipientEmail Who receives the notification
     * @param senderEmail Who sent the message
     * @param senderDisplayName Sender's display name
     * @param message Message content
     * @param timestamp Message timestamp
     */
    void notifyPrivateMessage(
        const std::string& recipientEmail,
        const std::string& senderEmail,
        const std::string& senderDisplayName,
        const std::string& message,
        uint64_t timestamp);

    /**
     * Notifies a user that their messages were read by the recipient
     * @param senderEmail Who sent the original messages (will be notified)
     * @param readerEmail Who read the messages
     */
    void notifyMessagesRead(
        const std::string& senderEmail,
        const std::string& readerEmail);

private:
    mutable std::mutex _mutex;
    std::unordered_map<std::string, FriendCallbacks> _callbacks;
};

} // namespace infrastructure::social

#endif /* !FRIENDMANAGER_HPP_ */
