/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendManager - Manages real-time friend notifications
*/

#include "infrastructure/social/FriendManager.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace infrastructure::social {

void FriendManager::registerCallbacks(const std::string& email, const FriendCallbacks& callbacks)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks[email] = callbacks;

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("FriendManager: Registered callbacks for {}", email);
}

void FriendManager::unregisterCallbacks(const std::string& email)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks.erase(email);

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("FriendManager: Unregistered callbacks for {}", email);
}

bool FriendManager::isUserOnline(const std::string& email) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _callbacks.find(email) != _callbacks.end();
}

void FriendManager::notifyFriendRequestReceived(
    const std::string& targetEmail,
    const std::string& fromEmail,
    const std::string& fromDisplayName)
{
    auto logger = server::logging::Logger::getMainLogger();

    // Get callback under lock, call outside lock
    FriendRequestReceivedCallback callback;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _callbacks.find(targetEmail);
        if (it != _callbacks.end() && it->second.onRequestReceived) {
            callback = it->second.onRequestReceived;
        }
    }

    if (callback) {
        logger->debug("FriendManager: Notifying {} of friend request from {}",
                      targetEmail, fromEmail);
        callback(fromEmail, fromDisplayName);
    } else {
        logger->debug("FriendManager: {} is offline, friend request notification skipped",
                      targetEmail);
    }
}

void FriendManager::notifyFriendRequestAccepted(
    const std::string& targetEmail,
    const std::string& friendEmail,
    const std::string& friendDisplayName,
    uint8_t onlineStatus)
{
    auto logger = server::logging::Logger::getMainLogger();

    FriendRequestAcceptedCallback callback;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _callbacks.find(targetEmail);
        if (it != _callbacks.end() && it->second.onRequestAccepted) {
            callback = it->second.onRequestAccepted;
        }
    }

    if (callback) {
        logger->debug("FriendManager: Notifying {} that {} accepted their request",
                      targetEmail, friendEmail);
        callback(friendEmail, friendDisplayName, onlineStatus);
    } else {
        logger->debug("FriendManager: {} is offline, acceptance notification skipped",
                      targetEmail);
    }
}

void FriendManager::notifyFriendRemoved(
    const std::string& targetEmail,
    const std::string& friendEmail)
{
    auto logger = server::logging::Logger::getMainLogger();

    FriendRemovedCallback callback;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _callbacks.find(targetEmail);
        if (it != _callbacks.end() && it->second.onFriendRemoved) {
            callback = it->second.onFriendRemoved;
        }
    }

    if (callback) {
        logger->debug("FriendManager: Notifying {} that {} removed them",
                      targetEmail, friendEmail);
        callback(friendEmail);
    }
}

void FriendManager::notifyFriendStatusChanged(
    const std::vector<std::string>& friendEmails,
    const std::string& changedEmail,
    uint8_t newStatus,
    const std::string& roomCode)
{
    auto logger = server::logging::Logger::getMainLogger();

    // Collect callbacks under lock
    std::vector<FriendStatusChangedCallback> callbacksToCall;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& friendEmail : friendEmails) {
            auto it = _callbacks.find(friendEmail);
            if (it != _callbacks.end() && it->second.onStatusChanged) {
                callbacksToCall.push_back(it->second.onStatusChanged);
            }
        }
    }

    // Call callbacks outside lock
    for (const auto& callback : callbacksToCall) {
        callback(changedEmail, newStatus, roomCode);
    }

    logger->debug("FriendManager: Notified {} friends of {}'s status change to {}",
                  callbacksToCall.size(), changedEmail, static_cast<int>(newStatus));
}

void FriendManager::notifyPrivateMessage(
    const std::string& recipientEmail,
    const std::string& senderEmail,
    const std::string& senderDisplayName,
    const std::string& message,
    uint64_t timestamp)
{
    auto logger = server::logging::Logger::getMainLogger();

    PrivateMessageReceivedCallback callback;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _callbacks.find(recipientEmail);
        if (it != _callbacks.end() && it->second.onPrivateMessage) {
            callback = it->second.onPrivateMessage;
        }
    }

    if (callback) {
        logger->debug("FriendManager: Notifying {} of private message from {}",
                      recipientEmail, senderEmail);
        callback(senderEmail, senderDisplayName, message, timestamp);
    } else {
        logger->debug("FriendManager: {} is offline, message stored for later",
                      recipientEmail);
    }
}

void FriendManager::notifyMessagesRead(
    const std::string& senderEmail,
    const std::string& readerEmail)
{
    auto logger = server::logging::Logger::getMainLogger();

    MessagesReadNotificationCallback callback;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _callbacks.find(senderEmail);
        if (it != _callbacks.end() && it->second.onMessagesRead) {
            callback = it->second.onMessagesRead;
        }
    }

    if (callback) {
        logger->debug("FriendManager: Notifying {} that {} read their messages",
                      senderEmail, readerEmail);
        callback(readerEmail);
    } else {
        logger->debug("FriendManager: {} is offline, read notification skipped",
                      senderEmail);
    }
}

} // namespace infrastructure::social
