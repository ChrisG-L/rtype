/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PrivateChatScene - Private messaging between friends
*/

#ifndef PRIVATECHATSCENE_HPP_
#define PRIVATECHATSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/TextInput.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include "network/NetworkEvents.hpp"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

class PrivateChatScene : public IScene {
public:
    PrivateChatScene(const std::string& friendEmail, const std::string& friendDisplayName);
    ~PrivateChatScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();
    void processTCPEvents();

    // Actions
    void onSendClick();
    void onLoadMoreClick();
    void onBackClick();

    // Data requests
    void requestConversation();

    // Event handlers
    void onConversationReceived(const client::network::TCPConversationEvent& event);
    void onMessageReceived(const client::network::TCPPrivateMessageReceivedEvent& event);
    void onMessageSent(const client::network::TCPPrivateMessageAckEvent& event);
    void onMessagesReadByRecipient(const client::network::TCPMessagesReadNotificationEvent& event);

    // Render helpers
    void renderMessages();
    void renderLoadingIndicator();
    std::string formatTimestamp(uint64_t timestamp) const;

    void showError(const std::string& message);
    void showInfo(const std::string& message);

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Friend info
    std::string _friendEmail;
    std::string _friendDisplayName;

    // Messages
    struct DisplayMessage {
        uint64_t messageId = 0;
        std::string senderEmail;
        std::string message;
        uint64_t timestamp = 0;
        bool isRead = false;           // For received messages: have I read it?
        bool isSentByMe = false;
        bool isReadByRecipient = false; // For sent messages: has the recipient read it? (✓✓)
    };
    std::vector<DisplayMessage> _messages;
    bool _messagesLoading = false;
    bool _hasMoreMessages = false;

    // UI components
    std::unique_ptr<ui::TextInput> _messageInput;
    std::unique_ptr<ui::Button> _sendBtn;
    std::unique_ptr<ui::Button> _loadMoreBtn;
    std::unique_ptr<ui::Button> _backBtn;

    // Status message
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Scroll offset (from bottom, 0 = newest messages visible)
    int _scrollOffset = 0;
    static constexpr int VISIBLE_MESSAGES = 12;
    static constexpr uint8_t MESSAGE_REQUEST_LIMIT = 50;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Animation
    float _loadingDots = 0.0f;

    // NOTE: Auto-refresh removed - we now rely on real-time message events
    // The old refresh was causing messages to disappear every 5 seconds

    // Constants
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "chat_font";
    static constexpr int STAR_COUNT = 80;

    // Layout
    static constexpr float MARGIN_X = 100.0f;
    static constexpr float HEADER_HEIGHT = 80.0f;
    static constexpr float INPUT_HEIGHT = 60.0f;
    static constexpr float MESSAGE_HEIGHT = 70.0f;
    static constexpr float STATUS_DISPLAY_DURATION = 4.0f;
};

#endif /* !PRIVATECHATSCENE_HPP_ */
