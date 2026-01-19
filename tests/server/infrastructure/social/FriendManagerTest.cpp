/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendManager unit tests
*/

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include "infrastructure/social/FriendManager.hpp"

using namespace infrastructure::social;

class FriendManagerTest : public ::testing::Test {
protected:
    FriendManager manager;
};

// ═══════════════════════════════════════════════════════════════════════════
// Callback Registration Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, RegisterCallbacks_UserBecomesOnline)
{
    EXPECT_FALSE(manager.isUserOnline("test@example.com"));

    FriendCallbacks callbacks;
    manager.registerCallbacks("test@example.com", callbacks);

    EXPECT_TRUE(manager.isUserOnline("test@example.com"));
}

TEST_F(FriendManagerTest, UnregisterCallbacks_UserBecomesOffline)
{
    FriendCallbacks callbacks;
    manager.registerCallbacks("test@example.com", callbacks);
    EXPECT_TRUE(manager.isUserOnline("test@example.com"));

    manager.unregisterCallbacks("test@example.com");
    EXPECT_FALSE(manager.isUserOnline("test@example.com"));
}

TEST_F(FriendManagerTest, UnregisterCallbacks_NonexistentUser_NoError)
{
    // Should not throw or crash
    EXPECT_NO_THROW(manager.unregisterCallbacks("nonexistent@example.com"));
}

TEST_F(FriendManagerTest, RegisterCallbacks_OverwriteExisting)
{
    std::atomic<int> callCount1{0};
    std::atomic<int> callCount2{0};

    FriendCallbacks callbacks1;
    callbacks1.onFriendRemoved = [&](const std::string&) { callCount1++; };
    manager.registerCallbacks("test@example.com", callbacks1);

    // Overwrite with new callbacks
    FriendCallbacks callbacks2;
    callbacks2.onFriendRemoved = [&](const std::string&) { callCount2++; };
    manager.registerCallbacks("test@example.com", callbacks2);

    // Trigger notification - should call callbacks2, not callbacks1
    manager.notifyFriendRemoved("test@example.com", "friend@example.com");

    EXPECT_EQ(callCount1.load(), 0);
    EXPECT_EQ(callCount2.load(), 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Friend Request Received Notification Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, NotifyFriendRequestReceived_OnlineUser_CallbackCalled)
{
    std::string receivedFromEmail;
    std::string receivedFromDisplayName;

    FriendCallbacks callbacks;
    callbacks.onRequestReceived = [&](const std::string& fromEmail,
                                       const std::string& fromDisplayName) {
        receivedFromEmail = fromEmail;
        receivedFromDisplayName = fromDisplayName;
    };
    manager.registerCallbacks("recipient@example.com", callbacks);

    manager.notifyFriendRequestReceived(
        "recipient@example.com",
        "sender@example.com",
        "SenderName"
    );

    EXPECT_EQ(receivedFromEmail, "sender@example.com");
    EXPECT_EQ(receivedFromDisplayName, "SenderName");
}

TEST_F(FriendManagerTest, NotifyFriendRequestReceived_OfflineUser_NoCallback)
{
    std::atomic<bool> callbackCalled{false};

    FriendCallbacks callbacks;
    callbacks.onRequestReceived = [&](const std::string&, const std::string&) {
        callbackCalled = true;
    };
    manager.registerCallbacks("other@example.com", callbacks);

    // Notify a different user who is offline
    manager.notifyFriendRequestReceived(
        "offline@example.com",
        "sender@example.com",
        "SenderName"
    );

    EXPECT_FALSE(callbackCalled.load());
}

TEST_F(FriendManagerTest, NotifyFriendRequestReceived_NoCallback_NoError)
{
    FriendCallbacks callbacks;
    // No onRequestReceived callback set
    manager.registerCallbacks("recipient@example.com", callbacks);

    // Should not throw
    EXPECT_NO_THROW(manager.notifyFriendRequestReceived(
        "recipient@example.com",
        "sender@example.com",
        "SenderName"
    ));
}

// ═══════════════════════════════════════════════════════════════════════════
// Friend Request Accepted Notification Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, NotifyFriendRequestAccepted_OnlineUser_CallbackCalled)
{
    std::string receivedFriendEmail;
    std::string receivedFriendDisplayName;
    uint8_t receivedOnlineStatus = 255;

    FriendCallbacks callbacks;
    callbacks.onRequestAccepted = [&](const std::string& friendEmail,
                                       const std::string& friendDisplayName,
                                       uint8_t onlineStatus) {
        receivedFriendEmail = friendEmail;
        receivedFriendDisplayName = friendDisplayName;
        receivedOnlineStatus = onlineStatus;
    };
    manager.registerCallbacks("requester@example.com", callbacks);

    manager.notifyFriendRequestAccepted(
        "requester@example.com",
        "acceptor@example.com",
        "AcceptorName",
        1  // Online status
    );

    EXPECT_EQ(receivedFriendEmail, "acceptor@example.com");
    EXPECT_EQ(receivedFriendDisplayName, "AcceptorName");
    EXPECT_EQ(receivedOnlineStatus, 1);
}

TEST_F(FriendManagerTest, NotifyFriendRequestAccepted_OfflineUser_NoCallback)
{
    std::atomic<bool> callbackCalled{false};

    FriendCallbacks callbacks;
    callbacks.onRequestAccepted = [&](const std::string&, const std::string&, uint8_t) {
        callbackCalled = true;
    };
    manager.registerCallbacks("other@example.com", callbacks);

    manager.notifyFriendRequestAccepted(
        "offline@example.com",
        "acceptor@example.com",
        "AcceptorName",
        1
    );

    EXPECT_FALSE(callbackCalled.load());
}

// ═══════════════════════════════════════════════════════════════════════════
// Friend Removed Notification Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, NotifyFriendRemoved_OnlineUser_CallbackCalled)
{
    std::string receivedFriendEmail;

    FriendCallbacks callbacks;
    callbacks.onFriendRemoved = [&](const std::string& friendEmail) {
        receivedFriendEmail = friendEmail;
    };
    manager.registerCallbacks("user@example.com", callbacks);

    manager.notifyFriendRemoved("user@example.com", "exfriend@example.com");

    EXPECT_EQ(receivedFriendEmail, "exfriend@example.com");
}

TEST_F(FriendManagerTest, NotifyFriendRemoved_OfflineUser_NoCallback)
{
    std::atomic<bool> callbackCalled{false};

    FriendCallbacks callbacks;
    callbacks.onFriendRemoved = [&](const std::string&) {
        callbackCalled = true;
    };
    manager.registerCallbacks("other@example.com", callbacks);

    manager.notifyFriendRemoved("offline@example.com", "exfriend@example.com");

    EXPECT_FALSE(callbackCalled.load());
}

// ═══════════════════════════════════════════════════════════════════════════
// Friend Status Changed Notification Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, NotifyFriendStatusChanged_MultipleOnlineFriends_AllNotified)
{
    std::atomic<int> callCount{0};
    std::string lastChangedEmail;
    uint8_t lastStatus = 255;
    std::string lastRoomCode;

    FriendCallbacks callbacks;
    callbacks.onStatusChanged = [&](const std::string& changedEmail,
                                     uint8_t newStatus,
                                     const std::string& roomCode) {
        callCount++;
        lastChangedEmail = changedEmail;
        lastStatus = newStatus;
        lastRoomCode = roomCode;
    };

    // Register 3 online friends
    manager.registerCallbacks("friend1@example.com", callbacks);
    manager.registerCallbacks("friend2@example.com", callbacks);
    manager.registerCallbacks("friend3@example.com", callbacks);

    std::vector<std::string> friendEmails = {
        "friend1@example.com",
        "friend2@example.com",
        "friend3@example.com"
    };

    manager.notifyFriendStatusChanged(
        friendEmails,
        "user@example.com",
        2,  // In game
        "ABCD"
    );

    EXPECT_EQ(callCount.load(), 3);
    EXPECT_EQ(lastChangedEmail, "user@example.com");
    EXPECT_EQ(lastStatus, 2);
    EXPECT_EQ(lastRoomCode, "ABCD");
}

TEST_F(FriendManagerTest, NotifyFriendStatusChanged_SomeFriendsOffline_OnlyOnlineNotified)
{
    std::atomic<int> callCount{0};

    FriendCallbacks callbacks;
    callbacks.onStatusChanged = [&](const std::string&, uint8_t, const std::string&) {
        callCount++;
    };

    // Only register 2 friends online
    manager.registerCallbacks("friend1@example.com", callbacks);
    manager.registerCallbacks("friend3@example.com", callbacks);

    std::vector<std::string> friendEmails = {
        "friend1@example.com",
        "friend2@example.com",  // Offline
        "friend3@example.com",
        "friend4@example.com"   // Offline
    };

    manager.notifyFriendStatusChanged(
        friendEmails,
        "user@example.com",
        1,  // Online
        ""
    );

    EXPECT_EQ(callCount.load(), 2);  // Only 2 online friends notified
}

TEST_F(FriendManagerTest, NotifyFriendStatusChanged_EmptyList_NoCallbacks)
{
    std::atomic<int> callCount{0};

    FriendCallbacks callbacks;
    callbacks.onStatusChanged = [&](const std::string&, uint8_t, const std::string&) {
        callCount++;
    };
    manager.registerCallbacks("friend@example.com", callbacks);

    std::vector<std::string> emptyList;
    manager.notifyFriendStatusChanged(emptyList, "user@example.com", 1, "");

    EXPECT_EQ(callCount.load(), 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Private Message Notification Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, NotifyPrivateMessage_OnlineRecipient_CallbackCalled)
{
    std::string receivedSenderEmail;
    std::string receivedSenderDisplayName;
    std::string receivedMessage;
    uint64_t receivedTimestamp = 0;

    FriendCallbacks callbacks;
    callbacks.onPrivateMessage = [&](const std::string& senderEmail,
                                      const std::string& senderDisplayName,
                                      const std::string& message,
                                      uint64_t timestamp) {
        receivedSenderEmail = senderEmail;
        receivedSenderDisplayName = senderDisplayName;
        receivedMessage = message;
        receivedTimestamp = timestamp;
    };
    manager.registerCallbacks("recipient@example.com", callbacks);

    uint64_t now = 1234567890123;
    manager.notifyPrivateMessage(
        "recipient@example.com",
        "sender@example.com",
        "SenderName",
        "Hello, friend!",
        now
    );

    EXPECT_EQ(receivedSenderEmail, "sender@example.com");
    EXPECT_EQ(receivedSenderDisplayName, "SenderName");
    EXPECT_EQ(receivedMessage, "Hello, friend!");
    EXPECT_EQ(receivedTimestamp, now);
}

TEST_F(FriendManagerTest, NotifyPrivateMessage_OfflineRecipient_NoCallback)
{
    std::atomic<bool> callbackCalled{false};

    FriendCallbacks callbacks;
    callbacks.onPrivateMessage = [&](const std::string&, const std::string&,
                                      const std::string&, uint64_t) {
        callbackCalled = true;
    };
    manager.registerCallbacks("other@example.com", callbacks);

    manager.notifyPrivateMessage(
        "offline@example.com",
        "sender@example.com",
        "SenderName",
        "Hello!",
        1234567890123
    );

    EXPECT_FALSE(callbackCalled.load());
}

// ═══════════════════════════════════════════════════════════════════════════
// Thread Safety Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, ConcurrentRegisterUnregister_ThreadSafe)
{
    const int numThreads = 10;
    const int iterationsPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < iterationsPerThread; ++i) {
                std::string email = "user" + std::to_string(t) + "_" + std::to_string(i) + "@example.com";

                FriendCallbacks callbacks;
                callbacks.onFriendRemoved = [](const std::string&) {};

                manager.registerCallbacks(email, callbacks);

                if (manager.isUserOnline(email)) {
                    successCount++;
                }

                manager.unregisterCallbacks(email);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All registrations should have succeeded at some point
    EXPECT_EQ(successCount.load(), numThreads * iterationsPerThread);
}

TEST_F(FriendManagerTest, ConcurrentNotifications_ThreadSafe)
{
    std::atomic<int> callCount{0};

    FriendCallbacks callbacks;
    callbacks.onStatusChanged = [&](const std::string&, uint8_t, const std::string&) {
        callCount++;
    };

    // Register several users
    for (int i = 0; i < 10; ++i) {
        manager.registerCallbacks("user" + std::to_string(i) + "@example.com", callbacks);
    }

    std::vector<std::string> allUsers;
    for (int i = 0; i < 10; ++i) {
        allUsers.push_back("user" + std::to_string(i) + "@example.com");
    }

    // Concurrent notifications from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 20; ++i) {
                manager.notifyFriendStatusChanged(
                    allUsers,
                    "changer" + std::to_string(t) + "@example.com",
                    static_cast<uint8_t>(i % 3),
                    "ROOM"
                );
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 5 threads * 20 iterations * 10 users = 1000 callbacks
    EXPECT_EQ(callCount.load(), 1000);
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(FriendManagerTest, EmptyEmail_HandledGracefully)
{
    FriendCallbacks callbacks;
    callbacks.onFriendRemoved = [](const std::string&) {};

    EXPECT_NO_THROW(manager.registerCallbacks("", callbacks));
    EXPECT_TRUE(manager.isUserOnline(""));
    EXPECT_NO_THROW(manager.unregisterCallbacks(""));
    EXPECT_FALSE(manager.isUserOnline(""));
}

TEST_F(FriendManagerTest, SpecialCharactersInEmail_HandledCorrectly)
{
    std::string specialEmail = "test+filter@sub.example.com";
    std::string receivedEmail;

    FriendCallbacks callbacks;
    callbacks.onFriendRemoved = [&](const std::string& email) {
        receivedEmail = email;
    };

    manager.registerCallbacks(specialEmail, callbacks);
    manager.notifyFriendRemoved(specialEmail, "friend@example.com");

    EXPECT_EQ(receivedEmail, "friend@example.com");
}

TEST_F(FriendManagerTest, LongMessage_HandledCorrectly)
{
    std::string receivedMessage;

    FriendCallbacks callbacks;
    callbacks.onPrivateMessage = [&](const std::string&, const std::string&,
                                      const std::string& message, uint64_t) {
        receivedMessage = message;
    };
    manager.registerCallbacks("recipient@example.com", callbacks);

    // Very long message (1000 characters)
    std::string longMessage(1000, 'A');
    manager.notifyPrivateMessage(
        "recipient@example.com",
        "sender@example.com",
        "Sender",
        longMessage,
        1234567890123
    );

    EXPECT_EQ(receivedMessage.length(), 1000);
    EXPECT_EQ(receivedMessage, longMessage);
}
