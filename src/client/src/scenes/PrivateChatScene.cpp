/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PrivateChatScene - Private messaging between friends
*/

#include "scenes/PrivateChatScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/FriendsScene.hpp"
#include "network/NetworkEvents.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <variant>
#include <ctime>
#include <iomanip>
#include <sstream>

PrivateChatScene::PrivateChatScene(const std::string& friendEmail, const std::string& friendDisplayName)
    : _friendEmail(friendEmail)
    , _friendDisplayName(friendDisplayName)
{
    client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] Chat with {}", friendDisplayName);
}

void PrivateChatScene::loadAssets() {
    if (_assetsLoaded) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;
    client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] Assets loaded");
}

void PrivateChatScene::initUI() {
    if (_uiInitialized) return;

    // Starfield background
    _starfield = std::make_unique<ui::StarfieldBackground>(
        static_cast<int>(SCREEN_WIDTH),
        static_cast<int>(SCREEN_HEIGHT),
        STAR_COUNT
    );

    // Message input
    float inputY = SCREEN_HEIGHT - MARGIN_X - INPUT_HEIGHT;
    float inputWidth = SCREEN_WIDTH - 2 * MARGIN_X - 120;

    _messageInput = std::make_unique<ui::TextInput>(
        Vec2f{MARGIN_X, inputY},
        Vec2f{inputWidth, INPUT_HEIGHT},
        "Type a message...",
        FONT_KEY
    );
    _messageInput->setMaxLength(500);

    // Send button
    _sendBtn = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH - MARGIN_X - 100, inputY},
        Vec2f{100, INPUT_HEIGHT},
        "SEND",
        FONT_KEY
    );
    _sendBtn->setOnClick([this]() { onSendClick(); });
    _sendBtn->setNormalColor({50, 120, 80, 255});
    _sendBtn->setHoveredColor({70, 150, 100, 255});

    // Load more button (at top of messages area)
    _loadMoreBtn = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - 80, HEADER_HEIGHT + 10},
        Vec2f{160, 35},
        "LOAD MORE",
        FONT_KEY
    );
    _loadMoreBtn->setOnClick([this]() { onLoadMoreClick(); });
    _loadMoreBtn->setNormalColor({60, 60, 80, 255});
    _loadMoreBtn->setHoveredColor({80, 80, 100, 255});

    // Back button
    _backBtn = std::make_unique<ui::Button>(
        Vec2f{20, 20},
        Vec2f{100, 40},
        "BACK",
        FONT_KEY
    );
    _backBtn->setOnClick([this]() { onBackClick(); });

    _uiInitialized = true;
    client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] UI initialized");

    // Load initial messages
    requestConversation();

    // Mark messages as read
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->markMessagesRead(_friendEmail);
    }
}

void PrivateChatScene::handleEvent(const events::Event& event) {
    if (!_uiInitialized) return;

    // Escape to go back
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& keyEvt = std::get<events::KeyPressed>(event);
        if (keyEvt.key == events::Key::Escape) {
            onBackClick();
            return;
        }
        // Enter to send
        if (keyEvt.key == events::Key::Enter) {
            onSendClick();
            return;
        }
        // Scroll with arrow keys
        if (keyEvt.key == events::Key::Up) {
            int maxScroll = std::max(0, static_cast<int>(_messages.size()) - VISIBLE_MESSAGES);
            if (_scrollOffset < maxScroll) {
                _scrollOffset++;
                // Auto-load more messages when reaching the top
                if (_scrollOffset >= maxScroll - 2 && _hasMoreMessages && !_messagesLoading) {
                    onLoadMoreClick();
                }
            }
        }
        if (keyEvt.key == events::Key::Down && _scrollOffset > 0) {
            _scrollOffset--;
        }
    }

    _messageInput->handleEvent(event);
    _sendBtn->handleEvent(event);
    _backBtn->handleEvent(event);

    if (_hasMoreMessages) {
        _loadMoreBtn->handleEvent(event);
    }
}

void PrivateChatScene::update(float deltaTime) {
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    // Update starfield
    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Loading animation
    _loadingDots += deltaTime * 3.0f;
    if (_loadingDots > 4.0f) _loadingDots = 0.0f;

    // Update UI components
    if (_messageInput) _messageInput->update(deltaTime);
    if (_sendBtn) _sendBtn->update(deltaTime);
    if (_backBtn) _backBtn->update(deltaTime);
    if (_loadMoreBtn && _hasMoreMessages) _loadMoreBtn->update(deltaTime);

    // Update status timer
    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }

    // NOTE: Auto-refresh REMOVED to fix message persistence bug
    // Messages now persist in _messages vector and are updated in real-time
    // via TCPPrivateMessageReceivedEvent. The old auto-refresh was calling
    // requestConversation() every 5 seconds which cleared _messages.

    // Process incoming TCP messages (real-time updates)
    processTCPEvents();
}

void PrivateChatScene::processTCPEvents() {
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPConversationEvent>) {
                onConversationReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPPrivateMessageReceivedEvent>) {
                onMessageReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPPrivateMessageAckEvent>) {
                onMessageSent(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPMessagesReadNotificationEvent>) {
                onMessagesReadByRecipient(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from server");
            } else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
        }, *eventOpt);
    }
}

void PrivateChatScene::render() {
    if (!_assetsLoaded || !_uiInitialized) return;

    // Draw starfield
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Header background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, {20, 20, 40, 230});

    // Back button
    _backBtn->render(*_context.window);

    // Friend name (centered in header)
    _context.window->drawText(FONT_KEY, "Chat with " + _friendDisplayName,
                              SCREEN_WIDTH / 2 - 100, 25, 24, {255, 255, 255, 255});

    // Email (smaller, below name)
    _context.window->drawText(FONT_KEY, _friendEmail,
                              SCREEN_WIDTH / 2 - 80, 55, 12, {150, 150, 180, 255});

    // Messages area background
    float messagesStartY = HEADER_HEIGHT + 50;
    float messagesEndY = SCREEN_HEIGHT - MARGIN_X - INPUT_HEIGHT - 20;
    _context.window->drawRect(MARGIN_X - 10, messagesStartY - 10,
                              SCREEN_WIDTH - 2 * MARGIN_X + 20, messagesEndY - messagesStartY + 20,
                              {15, 15, 30, 200});

    // Load more button (if more messages available)
    if (_hasMoreMessages && !_messagesLoading) {
        _loadMoreBtn->render(*_context.window);
    }

    // Render messages
    if (_messagesLoading && _messages.empty()) {
        renderLoadingIndicator();
    } else {
        renderMessages();
    }

    // Input area background
    float inputY = SCREEN_HEIGHT - MARGIN_X - INPUT_HEIGHT;
    _context.window->drawRect(MARGIN_X - 10, inputY - 10,
                              SCREEN_WIDTH - 2 * MARGIN_X + 20, INPUT_HEIGHT + 20,
                              {25, 25, 45, 230});

    // Message input and send button
    _messageInput->render(*_context.window);
    _sendBtn->render(*_context.window);

    // Status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 30, 14, _statusColor);
    }
}

void PrivateChatScene::renderMessages() {
    if (_messages.empty()) {
        _context.window->drawText(FONT_KEY, "No messages yet. Start the conversation!",
                                  SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2, 16, {150, 150, 150, 255});
        return;
    }

    float messagesStartY = HEADER_HEIGHT + (_hasMoreMessages ? 60 : 20);
    float messagesEndY = SCREEN_HEIGHT - MARGIN_X - INPUT_HEIGHT - 30;
    float availableHeight = messagesEndY - messagesStartY;

    // Calculate how many messages we can show
    int visibleCount = static_cast<int>(availableHeight / MESSAGE_HEIGHT);

    // Messages are stored oldest to newest, but we want to display newest at bottom
    // So we iterate from the end, applying scroll offset

    int startIdx = std::max(0, static_cast<int>(_messages.size()) - visibleCount - _scrollOffset);
    int endIdx = std::max(0, static_cast<int>(_messages.size()) - _scrollOffset);

    float rowY = messagesEndY - MESSAGE_HEIGHT;

    // Draw from newest (bottom) to oldest (top)
    for (int i = endIdx - 1; i >= startIdx && rowY >= messagesStartY; --i) {
        const auto& msg = _messages[i];

        // Message bubble background
        rgba bubbleColor = msg.isSentByMe ? rgba{50, 80, 120, 200} : rgba{60, 60, 80, 200};
        float bubbleX = msg.isSentByMe ? SCREEN_WIDTH - MARGIN_X - 600 : MARGIN_X;
        float bubbleWidth = 580.0f;

        _context.window->drawRect(bubbleX, rowY, bubbleWidth, MESSAGE_HEIGHT - 10, bubbleColor);

        // Sender name (only for received messages)
        if (!msg.isSentByMe) {
            _context.window->drawText(FONT_KEY, _friendDisplayName,
                                      bubbleX + 10, rowY + 5, 12, {100, 200, 255, 255});
        } else {
            _context.window->drawText(FONT_KEY, "You",
                                      bubbleX + 10, rowY + 5, 12, {100, 200, 100, 255});
        }

        // Message text
        _context.window->drawText(FONT_KEY, msg.message,
                                  bubbleX + 10, rowY + 22, 14, {255, 255, 255, 255});

        // Timestamp
        std::string timeStr = formatTimestamp(msg.timestamp);

        // For sent messages, add read receipt indicator
        if (msg.isSentByMe) {
            // Double check mark (✓✓) if read, single (✓) if sent but not read
            if (msg.isReadByRecipient) {
                timeStr += " vv";  // Blue double check
                _context.window->drawText(FONT_KEY, timeStr,
                                          bubbleX + bubbleWidth - 100, rowY + MESSAGE_HEIGHT - 25, 10, {80, 180, 255, 255});
            } else {
                timeStr += " v";   // Gray single check
                _context.window->drawText(FONT_KEY, timeStr,
                                          bubbleX + bubbleWidth - 90, rowY + MESSAGE_HEIGHT - 25, 10, {150, 150, 150, 255});
            }
        } else {
            _context.window->drawText(FONT_KEY, timeStr,
                                      bubbleX + bubbleWidth - 80, rowY + MESSAGE_HEIGHT - 25, 10, {120, 120, 140, 255});
        }

        rowY -= MESSAGE_HEIGHT;
    }

    // Scroll indicator
    if (_messages.size() > static_cast<size_t>(visibleCount)) {
        int showing = std::min(visibleCount, static_cast<int>(_messages.size()));
        std::string scrollInfo = "Showing " + std::to_string(showing) + " of " + std::to_string(_messages.size()) + " messages";
        _context.window->drawText(FONT_KEY, scrollInfo,
                                  SCREEN_WIDTH / 2 - 80, HEADER_HEIGHT + 20, 12, {100, 100, 120, 255});
    }
}

void PrivateChatScene::renderLoadingIndicator() {
    std::string dots(static_cast<int>(_loadingDots) % 4, '.');
    _context.window->drawText(FONT_KEY, "Loading messages" + dots,
                              SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2, 18, {200, 200, 200, 255});
}

std::string PrivateChatScene::formatTimestamp(uint64_t timestamp) const {
    // Convert milliseconds to time_t (seconds)
    time_t rawtime = static_cast<time_t>(timestamp / 1000);
    struct tm* timeinfo = localtime(&rawtime);

    if (!timeinfo) {
        return "???";
    }

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
        << std::setfill('0') << std::setw(2) << timeinfo->tm_min;

    return oss.str();
}

void PrivateChatScene::showError(const std::string& message) {
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void PrivateChatScene::showInfo(const std::string& message) {
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

// Actions
void PrivateChatScene::onSendClick() {
    std::string message = _messageInput->getText();
    if (message.empty()) {
        return;
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendPrivateMessage(_friendEmail, message);
        _messageInput->clear();

        // Optimistically add message to display
        std::string myEmail = _context.tcpClient->getEmail();
        DisplayMessage dm;
        dm.messageId = 0;  // Will be updated when ack received
        dm.senderEmail = myEmail;
        dm.message = message;
        dm.timestamp = static_cast<uint64_t>(std::time(nullptr)) * 1000;
        dm.isRead = true;
        dm.isSentByMe = true;
        _messages.push_back(dm);

        // Scroll to bottom to show new message
        _scrollOffset = 0;
    } else {
        showError("Not connected to server");
    }
}

void PrivateChatScene::onLoadMoreClick() {
    if (_messages.empty() || _messagesLoading) return;

    // Request older messages by increasing the offset
    // offset = how many messages we already have (skip them)
    // limit = how many more to load
    uint8_t currentOffset = static_cast<uint8_t>(std::min(static_cast<size_t>(255), _messages.size()));
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _messagesLoading = true;
        _context.tcpClient->getConversation(_friendEmail, currentOffset, MESSAGE_REQUEST_LIMIT);
    }
}

void PrivateChatScene::onBackClick() {
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<FriendsScene>());
    }
}

// Data requests
void PrivateChatScene::requestConversation() {
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) return;

    _messagesLoading = true;
    // getConversation(email, offset, limit) - offset=0 to start from newest, limit=50
    _context.tcpClient->getConversation(_friendEmail, 0, MESSAGE_REQUEST_LIMIT);
    client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] Requested conversation with {} (offset=0, limit={})",
        _friendEmail, MESSAGE_REQUEST_LIMIT);
}

// Event handlers
void PrivateChatScene::onConversationReceived(const client::network::TCPConversationEvent& event) {
    _messagesLoading = false;
    _hasMoreMessages = event.hasMore;

    std::string myEmail;
    if (_context.tcpClient) {
        myEmail = _context.tcpClient->getEmail();
    }

    // Server returns messages newest-first (descending timestamp order)
    // We need to store them oldest-first for proper display

    // Determine if this is:
    // 1. Initial load (_messages is empty)
    // 2. Loading older messages (pagination via "Load More" button)
    bool isInitialLoad = _messages.empty();
    bool isLoadingOlder = !isInitialLoad && !event.messages.empty();

    if (isLoadingOlder) {
        // Server returns newest-first, so when loading older messages:
        // - event.messages[0] is the newest of the batch (closest to our oldest)
        // - event.messages[last] is the oldest of the batch
        // We need to prepend them in reverse order (oldest first)

        // Check if received messages are older than what we have
        // Since server returns newest-first, last message is the oldest
        if (!event.messages.empty()) {
            uint64_t oldestExisting = _messages.front().timestamp;
            uint64_t newestReceived = event.messages.front().timestamp;

            // If the newest received is older than our oldest, prepend them
            if (newestReceived < oldestExisting) {
                std::vector<DisplayMessage> olderMessages;
                // Reverse iterate to get oldest-first order
                for (auto it = event.messages.rbegin(); it != event.messages.rend(); ++it) {
                    const auto& msg = *it;
                    DisplayMessage dm;
                    dm.messageId = msg.messageId;
                    dm.senderEmail = msg.senderEmail;
                    dm.message = msg.message;
                    dm.timestamp = msg.timestamp;
                    dm.isSentByMe = (msg.senderEmail == myEmail);
                    // For sent messages: isRead means recipient has read it
                    // For received messages: isRead means we have read it
                    dm.isRead = msg.isRead;
                    dm.isReadByRecipient = dm.isSentByMe ? msg.isRead : false;
                    olderMessages.push_back(dm);
                }
                // Adjust scroll offset to maintain reading position
                // Since we prepend messages at the beginning, we need to increase the offset
                _scrollOffset += static_cast<int>(olderMessages.size());
                _messages.insert(_messages.begin(), olderMessages.begin(), olderMessages.end());
                client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] Prepended {} older messages, scrollOffset now {}",
                    olderMessages.size(), _scrollOffset);
            }
        }
        // If received messages are not older, ignore them (avoid duplicates)
    } else if (isInitialLoad) {
        // Initial load: populate the message list
        // Reverse iterate since server returns newest-first, but we want oldest-first
        for (auto it = event.messages.rbegin(); it != event.messages.rend(); ++it) {
            const auto& msg = *it;
            DisplayMessage dm;
            dm.messageId = msg.messageId;
            dm.senderEmail = msg.senderEmail;
            dm.message = msg.message;
            dm.timestamp = msg.timestamp;
            dm.isSentByMe = (msg.senderEmail == myEmail);
            // For sent messages: isRead means recipient has read it
            // For received messages: isRead means we have read it
            dm.isRead = msg.isRead;
            dm.isReadByRecipient = dm.isSentByMe ? msg.isRead : false;
            _messages.push_back(dm);
        }
        client::logging::Logger::getSceneLogger()->info("[PrivateChatScene] Initial load: {} messages, hasMore={}",
            _messages.size(), event.hasMore);
    }
    // NOTE: We no longer clear messages on subsequent loads to preserve chat history
}

void PrivateChatScene::onMessageReceived(const client::network::TCPPrivateMessageReceivedEvent& event) {
    // Only add if it's from our chat partner
    if (event.senderEmail == _friendEmail) {
        DisplayMessage dm;
        dm.messageId = 0;
        dm.senderEmail = event.senderEmail;
        dm.message = event.message;
        dm.timestamp = event.timestamp;
        dm.isRead = false;
        dm.isSentByMe = false;
        _messages.push_back(dm);

        // Mark as read since we're viewing the chat
        if (_context.tcpClient && _context.tcpClient->isConnected()) {
            _context.tcpClient->markMessagesRead(_friendEmail);
        }

    }
}

void PrivateChatScene::onMessageSent(const client::network::TCPPrivateMessageAckEvent& event) {
    if (event.errorCode != 0) {
        switch (event.errorCode) {
            case 1: showError("Not friends with this user"); break;
            case 2: showError("Blocked by this user"); break;
            case 3: showError("Message too long"); break;
            default: showError("Failed to send message"); break;
        }
        // Remove optimistically added message
        if (!_messages.empty() && _messages.back().messageId == 0) {
            _messages.pop_back();
        }
    }
    // If successful, the message ID would be set, but we already added it optimistically
}

void PrivateChatScene::onMessagesReadByRecipient(const client::network::TCPMessagesReadNotificationEvent& event) {
    // Only care about notifications from the friend we're chatting with
    if (event.readerEmail != _friendEmail) {
        return;
    }

    // Mark all our sent messages to this friend as read by them
    for (auto& msg : _messages) {
        if (msg.isSentByMe && !msg.isReadByRecipient) {
            msg.isReadByRecipient = true;
        }
    }

    client::logging::Logger::getSceneLogger()->debug("[PrivateChatScene] {} read our messages", event.readerEmail);
}
