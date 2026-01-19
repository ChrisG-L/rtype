/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendsProtocolTest - Tests for Friends System protocol structures
*/

#include <gtest/gtest.h>
#include "Protocol.hpp"
#include <cstring>

// ============================================================================
// FriendInfoWire Tests
// ============================================================================

class FriendInfoWireTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendInfoWireTest, WireSizeCorrect) {
    // email (64) + displayName (32) + onlineStatus (1) + lastSeen (4) + roomCode (7) = 108
    EXPECT_EQ(FriendInfoWire::WIRE_SIZE, MAX_EMAIL_LEN + MAX_USERNAME_LEN + 1 + 4 + ROOM_CODE_LEN);
}

TEST_F(FriendInfoWireTest, SerializationRoundTrip) {
    FriendInfoWire original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.email, "friend@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.displayName, "FriendName", MAX_USERNAME_LEN - 1);
    original.onlineStatus = static_cast<uint8_t>(FriendOnlineStatus::Online);
    original.lastSeen = 1705678901;
    std::strncpy(original.roomCode, "ABCDE", ROOM_CODE_LEN - 1);

    uint8_t buffer[FriendInfoWire::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = FriendInfoWire::from_bytes(buffer, FriendInfoWire::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->email, "friend@example.com");
    EXPECT_STREQ(parsed->displayName, "FriendName");
    EXPECT_EQ(parsed->onlineStatus, static_cast<uint8_t>(FriendOnlineStatus::Online));
    EXPECT_EQ(parsed->lastSeen, 1705678901u);
    EXPECT_STREQ(parsed->roomCode, "ABCDE");
}

TEST_F(FriendInfoWireTest, InvalidBufferSize) {
    uint8_t buffer[10];
    auto parsed = FriendInfoWire::from_bytes(buffer, 10);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(FriendInfoWireTest, AllOnlineStatuses) {
    FriendInfoWire entry;
    std::memset(&entry, 0, sizeof(entry));

    // Test all defined statuses: Offline, Online, InGame, InLobby
    const uint8_t statuses[] = {
        static_cast<uint8_t>(FriendOnlineStatus::Offline),
        static_cast<uint8_t>(FriendOnlineStatus::Online),
        static_cast<uint8_t>(FriendOnlineStatus::InGame),
        static_cast<uint8_t>(FriendOnlineStatus::InLobby)
    };

    for (auto status : statuses) {
        entry.onlineStatus = status;
        uint8_t buffer[FriendInfoWire::WIRE_SIZE];
        entry.to_bytes(buffer);

        auto parsed = FriendInfoWire::from_bytes(buffer, FriendInfoWire::WIRE_SIZE);
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(parsed->onlineStatus, status);
    }
}

// ============================================================================
// FriendRequestInfoWire Tests
// ============================================================================

class FriendRequestInfoWireTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendRequestInfoWireTest, WireSizeCorrect) {
    // email (64) + displayName (32) + timestamp (4) + isIncoming (1) = 101
    EXPECT_EQ(FriendRequestInfoWire::WIRE_SIZE, MAX_EMAIL_LEN + MAX_USERNAME_LEN + 4 + 1);
}

TEST_F(FriendRequestInfoWireTest, SerializationRoundTrip) {
    FriendRequestInfoWire original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.email, "requester@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.displayName, "RequesterName", MAX_USERNAME_LEN - 1);
    original.timestamp = 1705678901;
    original.isIncoming = 1;

    uint8_t buffer[FriendRequestInfoWire::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = FriendRequestInfoWire::from_bytes(buffer, FriendRequestInfoWire::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->email, "requester@example.com");
    EXPECT_STREQ(parsed->displayName, "RequesterName");
    EXPECT_EQ(parsed->timestamp, 1705678901u);
    EXPECT_EQ(parsed->isIncoming, 1u);
}

TEST_F(FriendRequestInfoWireTest, OutgoingRequest) {
    FriendRequestInfoWire req;
    std::memset(&req, 0, sizeof(req));
    req.isIncoming = 0;

    uint8_t buffer[FriendRequestInfoWire::WIRE_SIZE];
    req.to_bytes(buffer);

    auto parsed = FriendRequestInfoWire::from_bytes(buffer, FriendRequestInfoWire::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->isIncoming, 0u);
}

TEST_F(FriendRequestInfoWireTest, InvalidBufferSize) {
    uint8_t buffer[50];
    auto parsed = FriendRequestInfoWire::from_bytes(buffer, 50);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// PrivateMessageWire Tests
// ============================================================================

class PrivateMessageWireTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PrivateMessageWireTest, WireSizeCorrect) {
    // email (64) + displayName (32) + message (256) + timestamp (8) + isRead (1) = 361
    EXPECT_EQ(PrivateMessageWire::WIRE_SIZE, MAX_EMAIL_LEN + MAX_USERNAME_LEN + MAX_MESSAGE_LEN + 8 + 1);
}

TEST_F(PrivateMessageWireTest, SerializationRoundTrip) {
    PrivateMessageWire original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.senderEmail, "sender@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.senderDisplayName, "SenderName", MAX_USERNAME_LEN - 1);
    std::strncpy(original.message, "Hello, this is a test message!", MAX_MESSAGE_LEN - 1);
    original.timestamp = 1705678901234ULL;
    original.isRead = 1;

    uint8_t buffer[PrivateMessageWire::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PrivateMessageWire::from_bytes(buffer, PrivateMessageWire::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->senderEmail, "sender@example.com");
    EXPECT_STREQ(parsed->senderDisplayName, "SenderName");
    EXPECT_STREQ(parsed->message, "Hello, this is a test message!");
    EXPECT_EQ(parsed->timestamp, 1705678901234ULL);
    EXPECT_EQ(parsed->isRead, 1u);
}

TEST_F(PrivateMessageWireTest, UnreadMessage) {
    PrivateMessageWire msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.isRead = 0;

    uint8_t buffer[PrivateMessageWire::WIRE_SIZE];
    msg.to_bytes(buffer);

    auto parsed = PrivateMessageWire::from_bytes(buffer, PrivateMessageWire::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->isRead, 0u);
}

TEST_F(PrivateMessageWireTest, InvalidBufferSize) {
    uint8_t buffer[50];
    auto parsed = PrivateMessageWire::from_bytes(buffer, 50);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// ConversationSummaryWire Tests
// ============================================================================

class ConversationSummaryWireTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ConversationSummaryWireTest, WireSizeCorrect) {
    // email (64) + displayName (32) + lastMessage (256) + lastTimestamp (8) + unreadCount (1) + onlineStatus (1) = 362
    EXPECT_EQ(ConversationSummaryWire::WIRE_SIZE, MAX_EMAIL_LEN + MAX_USERNAME_LEN + MAX_MESSAGE_LEN + 8 + 1 + 1);
}

TEST_F(ConversationSummaryWireTest, SerializationRoundTrip) {
    ConversationSummaryWire original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.otherEmail, "other@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.otherDisplayName, "OtherUser", MAX_USERNAME_LEN - 1);
    std::strncpy(original.lastMessage, "Hey, how are you?", MAX_MESSAGE_LEN - 1);
    original.lastTimestamp = 1705678901234ULL;
    original.unreadCount = 5;
    original.onlineStatus = static_cast<uint8_t>(FriendOnlineStatus::Online);

    uint8_t buffer[ConversationSummaryWire::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = ConversationSummaryWire::from_bytes(buffer, ConversationSummaryWire::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->otherEmail, "other@example.com");
    EXPECT_STREQ(parsed->otherDisplayName, "OtherUser");
    EXPECT_STREQ(parsed->lastMessage, "Hey, how are you?");
    EXPECT_EQ(parsed->lastTimestamp, 1705678901234ULL);
    EXPECT_EQ(parsed->unreadCount, 5u);
    EXPECT_EQ(parsed->onlineStatus, static_cast<uint8_t>(FriendOnlineStatus::Online));
}

TEST_F(ConversationSummaryWireTest, InvalidBufferSize) {
    uint8_t buffer[50];
    auto parsed = ConversationSummaryWire::from_bytes(buffer, 50);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// FriendErrorCode Tests
// ============================================================================

class FriendErrorCodeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendErrorCodeTest, ErrorCodeValues) {
    // Values from Protocol.hpp enum class FriendErrorCode
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::Success), 0);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::UserNotFound), 1);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::AlreadyFriends), 2);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::RequestAlreadySent), 3);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::RequestAlreadyReceived), 4);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::IsBlocked), 5);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::BlockedByUser), 6);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::CannotAddSelf), 7);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::MaxFriendsReached), 8);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::MaxRequestsReached), 9);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::NotFriends), 10);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::RequestNotFound), 11);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::InvalidRequest), 12);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::Blocked), 13);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::DatabaseError), 14);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::AlreadyBlocked), 15);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::NotBlocked), 16);
    EXPECT_EQ(static_cast<uint8_t>(FriendErrorCode::InternalError), 99);
}

// ============================================================================
// Payload Tests
// ============================================================================

class FriendsPayloadTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendsPayloadTest, SendFriendRequestPayloadSize) {
    EXPECT_EQ(SendFriendRequestPayload::WIRE_SIZE, MAX_EMAIL_LEN);
}

TEST_F(FriendsPayloadTest, SendFriendRequestPayloadRoundTrip) {
    SendFriendRequestPayload original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.targetEmail, "target@example.com", MAX_EMAIL_LEN - 1);

    uint8_t buffer[SendFriendRequestPayload::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = SendFriendRequestPayload::from_bytes(buffer, SendFriendRequestPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->targetEmail, "target@example.com");
}

TEST_F(FriendsPayloadTest, FriendRequestAckPayload) {
    FriendRequestAckPayload original;
    std::memset(&original, 0, sizeof(original));
    original.errorCode = static_cast<uint8_t>(FriendErrorCode::AlreadyFriends);
    std::strncpy(original.targetEmail, "friend@example.com", MAX_EMAIL_LEN - 1);

    uint8_t buffer[FriendRequestAckPayload::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = FriendRequestAckPayload::from_bytes(buffer, FriendRequestAckPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->errorCode, static_cast<uint8_t>(FriendErrorCode::AlreadyFriends));
    EXPECT_STREQ(parsed->targetEmail, "friend@example.com");
}

TEST_F(FriendsPayloadTest, FriendRequestReceivedPayload) {
    FriendRequestReceivedPayload original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.fromEmail, "sender@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.fromDisplayName, "SenderName", MAX_USERNAME_LEN - 1);

    uint8_t buffer[FriendRequestReceivedPayload::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = FriendRequestReceivedPayload::from_bytes(buffer, FriendRequestReceivedPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->fromEmail, "sender@example.com");
    EXPECT_STREQ(parsed->fromDisplayName, "SenderName");
}

TEST_F(FriendsPayloadTest, RespondFriendRequestPayloadSize) {
    EXPECT_EQ(RespondFriendRequestPayload::WIRE_SIZE, MAX_EMAIL_LEN);
}

TEST_F(FriendsPayloadTest, GetFriendsListPayloadSize) {
    EXPECT_EQ(GetFriendsListPayload::WIRE_SIZE, 2u);
}

TEST_F(FriendsPayloadTest, SendPrivateMessagePayloadSize) {
    EXPECT_EQ(SendPrivateMessagePayload::WIRE_SIZE, MAX_EMAIL_LEN + MAX_MESSAGE_LEN);
}

TEST_F(FriendsPayloadTest, GetConversationPayloadSize) {
    EXPECT_EQ(GetConversationPayload::WIRE_SIZE, MAX_EMAIL_LEN + 2);
}

TEST_F(FriendsPayloadTest, MarkMessagesReadPayloadSize) {
    EXPECT_EQ(MarkMessagesReadPayload::WIRE_SIZE, MAX_EMAIL_LEN);
}

TEST_F(FriendsPayloadTest, FriendRequestAcceptedPayload) {
    FriendRequestAcceptedPayload original;
    std::memset(&original, 0, sizeof(original));
    std::strncpy(original.friendEmail, "newfriend@example.com", MAX_EMAIL_LEN - 1);
    std::strncpy(original.friendDisplayName, "NewFriend", MAX_USERNAME_LEN - 1);
    original.onlineStatus = static_cast<uint8_t>(FriendOnlineStatus::InGame);

    uint8_t buffer[FriendRequestAcceptedPayload::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = FriendRequestAcceptedPayload::from_bytes(buffer, FriendRequestAcceptedPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_STREQ(parsed->friendEmail, "newfriend@example.com");
    EXPECT_STREQ(parsed->friendDisplayName, "NewFriend");
    EXPECT_EQ(parsed->onlineStatus, static_cast<uint8_t>(FriendOnlineStatus::InGame));
}

// ============================================================================
// MessageType Tests
// ============================================================================

class FriendsMessageTypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendsMessageTypeTest, FriendMessageTypes) {
    // Friend request flow
    EXPECT_EQ(static_cast<uint16_t>(MessageType::SendFriendRequest), 0x0600);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::SendFriendRequestAck), 0x0601);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendRequestReceived), 0x0602);

    // Accept friend request
    EXPECT_EQ(static_cast<uint16_t>(MessageType::AcceptFriendRequest), 0x0610);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::AcceptFriendRequestAck), 0x0611);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendRequestAccepted), 0x0612);

    // Reject friend request
    EXPECT_EQ(static_cast<uint16_t>(MessageType::RejectFriendRequest), 0x0620);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::RejectFriendRequestAck), 0x0621);

    // Remove friend
    EXPECT_EQ(static_cast<uint16_t>(MessageType::RemoveFriend), 0x0630);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::RemoveFriendAck), 0x0631);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendRemoved), 0x0632);

    // Block/Unblock
    EXPECT_EQ(static_cast<uint16_t>(MessageType::BlockUser), 0x0640);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::BlockUserAck), 0x0641);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::UnblockUser), 0x0650);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::UnblockUserAck), 0x0651);

    // Lists
    EXPECT_EQ(static_cast<uint16_t>(MessageType::GetFriendsList), 0x0660);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendsListData), 0x0661);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::GetFriendRequests), 0x0670);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendRequestsData), 0x0671);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::GetBlockedUsers), 0x0672);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::BlockedUsersData), 0x0673);

    // Status
    EXPECT_EQ(static_cast<uint16_t>(MessageType::FriendStatusChanged), 0x0680);
}

TEST_F(FriendsMessageTypeTest, PrivateMessageTypes) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::SendPrivateMessage), 0x0690);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::SendPrivateMessageAck), 0x0691);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PrivateMessageReceived), 0x0692);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::GetConversation), 0x0693);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ConversationData), 0x0694);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::GetConversationsList), 0x0695);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ConversationsListData), 0x0696);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::MarkMessagesRead), 0x0697);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::MarkMessagesReadAck), 0x0698);
}

// ============================================================================
// FriendOnlineStatus Tests
// ============================================================================

class FriendOnlineStatusTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendOnlineStatusTest, StatusValues) {
    EXPECT_EQ(static_cast<uint8_t>(FriendOnlineStatus::Offline), 0);
    EXPECT_EQ(static_cast<uint8_t>(FriendOnlineStatus::Online), 1);
    EXPECT_EQ(static_cast<uint8_t>(FriendOnlineStatus::InGame), 2);
    EXPECT_EQ(static_cast<uint8_t>(FriendOnlineStatus::InLobby), 3);
}

// ============================================================================
// FriendshipStatus Tests
// ============================================================================

class FriendshipStatusTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FriendshipStatusTest, StatusValues) {
    EXPECT_EQ(static_cast<uint8_t>(FriendshipStatus::Pending), 0);
    EXPECT_EQ(static_cast<uint8_t>(FriendshipStatus::Accepted), 1);
}
