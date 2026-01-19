/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendsScene - Friends list, friend requests, and blocked users management
*/

#ifndef FRIENDSSCENE_HPP_
#define FRIENDSSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/TextInput.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include "network/NetworkEvents.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class FriendsScene : public IScene {
public:
    FriendsScene();
    ~FriendsScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();
    void processTCPEvents();

    // Tab switching
    void onFriendsTabClick();
    void onRequestsTabClick();
    void onBlockedTabClick();
    void onBackClick();

    // Friend actions
    void onAddFriendClick();
    void onAddFriendConfirm();
    void onRemoveFriend(const std::string& email);
    void onBlockFriend(const std::string& email);
    void onMessageFriend(const std::string& email);

    // Request actions
    void onAcceptRequest(const std::string& email);
    void onRejectRequest(const std::string& email);
    void onCancelRequest(const std::string& email);

    // Blocked actions
    void onUnblockUser(const std::string& email);

    // Request data
    void requestFriendsList();
    void requestFriendRequests();
    void requestBlockedUsers();
    void requestUnreadCounts();

    // Event handlers
    void onFriendsListReceived(const client::network::TCPFriendsListEvent& event);
    void onFriendRequestsReceived(const client::network::TCPFriendRequestsEvent& event);
    void onBlockedUsersReceived(const client::network::TCPBlockedUsersEvent& event);
    void onFriendRequestAck(const client::network::TCPFriendRequestAckEvent& event);
    void onFriendStatusChanged(const client::network::TCPFriendStatusChangedEvent& event);
    void onConversationsListReceived(const client::network::TCPConversationsListEvent& event);

    // Render helpers
    void renderFriendsTab();
    void renderRequestsTab();
    void renderBlockedTab();
    void renderAddFriendDialog();
    void renderLoadingIndicator();
    std::string getStatusString(uint8_t status) const;
    rgba getStatusColor(uint8_t status) const;

    void showError(const std::string& message);
    void showInfo(const std::string& message);

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Current tab
    enum class Tab {
        Friends,
        Requests,
        Blocked
    };
    Tab _currentTab = Tab::Friends;

    // Friends data
    std::vector<client::network::FriendInfo> _friends;
    bool _friendsLoading = false;

    // Friend requests data
    std::vector<client::network::FriendRequestInfo> _incomingRequests;
    std::vector<client::network::FriendRequestInfo> _outgoingRequests;
    bool _requestsLoading = false;

    // Blocked users data
    std::vector<client::network::FriendInfo> _blockedUsers;
    bool _blockedLoading = false;

    // Unread message counts (email -> unread count)
    std::unordered_map<std::string, uint8_t> _unreadCounts;

    // Add friend dialog
    bool _showAddFriendDialog = false;
    std::unique_ptr<ui::TextInput> _addFriendEmailInput;
    std::unique_ptr<ui::Button> _addFriendConfirmBtn;
    std::unique_ptr<ui::Button> _addFriendCancelBtn;

    // Tab buttons
    std::unique_ptr<ui::Button> _friendsTabBtn;
    std::unique_ptr<ui::Button> _requestsTabBtn;
    std::unique_ptr<ui::Button> _blockedTabBtn;

    // Action buttons
    std::unique_ptr<ui::Button> _addFriendBtn;
    std::unique_ptr<ui::Button> _backBtn;

    // Status message
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Scroll offset
    int _scrollOffset = 0;
    static constexpr int VISIBLE_ENTRIES = 10;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Animation
    float _loadingDots = 0.0f;

    // Constants
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "friends_font";
    static constexpr int STAR_COUNT = 100;

    // Layout
    static constexpr float TAB_Y = 80.0f;
    static constexpr float TAB_BTN_WIDTH = 180.0f;
    static constexpr float TAB_BTN_HEIGHT = 50.0f;
    static constexpr float CONTENT_START_Y = 180.0f;
    static constexpr float ROW_HEIGHT = 60.0f;
    static constexpr float MARGIN_X = 100.0f;
    static constexpr float STATUS_DISPLAY_DURATION = 4.0f;
};

#endif /* !FRIENDSSCENE_HPP_ */
