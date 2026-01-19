# Friends & Private Messaging System

Social system with friend requests, friendships, blocking, and private messages.

## Architecture

```
[Client: FriendsScene/PrivateChatScene] ←TCP→ [TCPAuthServer]
                                                    ↓
                                           [FriendManager] (real-time notifications)
                                                    ↓
                        ┌───────────────────────────┼───────────────────────────┐
                        ↓                           ↓                           ↓
              [IFriendshipRepository]    [IFriendRequestRepository]    [IPrivateMessageRepository]
              [IBlockedUserRepository]
                        ↓                           ↓                           ↓
                                            [MongoDB Collections]
                                    (friendships, friend_requests, blocked_users, private_messages)
```

## TCP Protocol Messages (Friends)

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `SendFriendRequest` | 0x0600 | C→S | Send friend request |
| `FriendRequestSent` | 0x0601 | S→C | Request sent confirmation |
| `FriendRequestReceived` | 0x0602 | S→C | Incoming request notification |
| `AcceptFriendRequest` | 0x0610 | C→S | Accept request |
| `RejectFriendRequest` | 0x0611 | C→S | Reject request |
| `FriendRequestAccepted` | 0x0612 | S→C | Acceptance notification |
| `FriendRequestRejected` | 0x0613 | S→C | Rejection notification |
| `RemoveFriend` | 0x0620 | C→S | Remove friendship |
| `FriendRemoved` | 0x0621 | S→C | Removal confirmation |
| `BlockUser` | 0x0630 | C→S | Block a user |
| `UnblockUser` | 0x0631 | C→S | Unblock a user |
| `UserBlocked` | 0x0632 | S→C | Block confirmation |
| `UserUnblocked` | 0x0633 | S→C | Unblock confirmation |
| `GetFriendsList` | 0x0640 | C→S | Request friends list |
| `FriendsListResponse` | 0x0641 | S→C | Friends list data |
| `GetFriendRequests` | 0x0642 | C→S | Request pending requests |
| `FriendRequestsResponse` | 0x0643 | S→C | Pending requests data |
| `GetBlockedUsers` | 0x0644 | C→S | Request blocked list |
| `BlockedUsersResponse` | 0x0645 | S→C | Blocked users data |
| `FriendStatusChanged` | 0x0650 | S→C | Online status notification |
| `FriendError` | 0x0698 | S→C | Error response |

## TCP Protocol Messages (Private Messages)

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `SendPrivateMessage` | 0x0660 | C→S | Send private message |
| `PrivateMessageSent` | 0x0661 | S→C | Message sent confirmation |
| `PrivateMessageReceived` | 0x0662 | S→C | Incoming message notification |
| `GetConversation` | 0x0670 | C→S | Request conversation history |
| `ConversationResponse` | 0x0671 | S→C | Conversation messages |
| `GetConversationsList` | 0x0672 | C→S | Request all conversations |
| `ConversationsListResponse` | 0x0673 | S→C | Conversations summaries |
| `MarkMessagesRead` | 0x0680 | C→S | Mark messages as read |
| `MessagesMarkedRead` | 0x0681 | S→C | Read confirmation |

## Wire Structures

```cpp
// FriendEntryWire (98 bytes) - Friend in list
struct FriendEntryWire {
    char email[MAX_EMAIL_LEN];          // 64 bytes
    char displayName[MAX_USERNAME_LEN]; // 32 bytes
    uint8_t onlineStatus;               // FriendOnlineStatus enum
    uint8_t padding;
};

// FriendRequestEntryWire (100 bytes) - Pending request
struct FriendRequestEntryWire {
    char fromEmail[MAX_EMAIL_LEN];      // 64 bytes
    char fromDisplayName[MAX_USERNAME_LEN]; // 32 bytes
    uint32_t timestamp;                 // Unix timestamp
};

// PrivateMessageWire (233 bytes) - Message in conversation
struct PrivateMessageWire {
    uint64_t messageId;
    char senderEmail[MAX_EMAIL_LEN];    // 64 bytes
    char senderDisplayName[MAX_USERNAME_LEN]; // 32 bytes
    uint64_t timestamp;
    uint8_t isRead;
    char message[MAX_MESSAGE_LEN];      // 120 bytes
};

// ConversationSummaryWire (154 bytes) - Conversation preview
struct ConversationSummaryWire {
    char otherEmail[MAX_EMAIL_LEN];     // 64 bytes
    char otherDisplayName[MAX_USERNAME_LEN]; // 32 bytes
    char lastMessagePreview[50];
    uint64_t lastTimestamp;
    uint8_t unreadCount;
};
```

## Error Codes

```cpp
enum class FriendErrorCode : uint8_t {
    None = 0,
    UserNotFound = 1,
    AlreadyFriends = 2,
    RequestAlreadySent = 3,
    RequestNotFound = 4,
    CannotAddSelf = 5,
    UserBlocked = 6,
    BlockedByUser = 7,
    NotFriends = 8,
    AlreadyBlocked = 9,
    NotBlocked = 10,
    MessageTooLong = 11,
    MessageEmpty = 12,
    ConversationNotFound = 13,
    RateLimited = 14,
    ServerError = 15
};
```

## MongoDB Collections

| Collection | Description |
|------------|-------------|
| `friendships` | Bidirectional friend relationships |
| `friend_requests` | Pending friend requests |
| `blocked_users` | User block relationships |
| `private_messages` | Private message history |

## Key Files

| File | Description |
|------|-------------|
| `src/server/include/application/ports/out/persistence/IFriendshipRepository.hpp` | Friendship interface |
| `src/server/include/application/ports/out/persistence/IFriendRequestRepository.hpp` | Friend request interface |
| `src/server/include/application/ports/out/persistence/IBlockedUserRepository.hpp` | Blocked user interface |
| `src/server/include/application/ports/out/persistence/IPrivateMessageRepository.hpp` | Private message interface |
| `src/server/infrastructure/adapters/out/persistence/MongoDBFriendshipRepository.cpp` | MongoDB friendship impl |
| `src/server/infrastructure/adapters/out/persistence/MongoDBFriendRequestRepository.cpp` | MongoDB request impl |
| `src/server/infrastructure/adapters/out/persistence/MongoDBBlockedUserRepository.cpp` | MongoDB blocked impl |
| `src/server/infrastructure/adapters/out/persistence/MongoDBPrivateMessageRepository.cpp` | MongoDB message impl |
| `src/server/include/infrastructure/social/FriendManager.hpp` | Real-time notification manager |
| `src/server/infrastructure/social/FriendManager.cpp` | FriendManager implementation |
| `src/client/include/scenes/FriendsScene.hpp` | Friends UI scene header |
| `src/client/src/scenes/FriendsScene.cpp` | Friends UI implementation |
| `src/client/include/scenes/PrivateChatScene.hpp` | Chat UI scene header |
| `src/client/src/scenes/PrivateChatScene.cpp` | Chat UI implementation |

## Client Usage

```cpp
// In MainMenuScene - navigate to friends
auto& sceneManager = SceneManager::getInstance();
sceneManager.changeScene(std::make_unique<FriendsScene>());

// Send friend request
tcpClient.sendFriendRequest("friend@example.com");

// Accept/reject request
tcpClient.acceptFriendRequest("requester@example.com");
tcpClient.rejectFriendRequest("requester@example.com");

// Block/unblock user
tcpClient.blockUser("user@example.com");
tcpClient.unblockUser("user@example.com");

// Remove friend
tcpClient.removeFriend("friend@example.com");

// Send private message
tcpClient.sendPrivateMessage("friend@example.com", "Hello!");

// Get conversation history (offset, limit)
tcpClient.getConversation("friend@example.com", 0, 50);

// Get all conversations
tcpClient.getConversationsList();

// Mark messages as read
tcpClient.markMessagesRead("friend@example.com");
```

## Real-Time Notifications

The `FriendManager` handles real-time notifications:

- **Friend request received** - Notified when someone sends you a request
- **Friend request accepted** - Notified when your request is accepted
- **Friend status changed** - Notified when a friend comes online/offline
- **Private message received** - Notified when you receive a new message

These notifications are pushed to connected clients via TCP without polling.
