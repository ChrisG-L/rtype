/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** FriendsScene - Friends list, friend requests, and blocked users management
*/

#include "scenes/FriendsScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/PrivateChatScene.hpp"
#include "network/NetworkEvents.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <variant>

FriendsScene::FriendsScene() {
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Constructor called");
}

void FriendsScene::loadAssets() {
    if (_assetsLoaded) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Assets loaded");
}

void FriendsScene::initUI() {
    if (_uiInitialized) return;

    // Starfield background
    _starfield = std::make_unique<ui::StarfieldBackground>(
        static_cast<int>(SCREEN_WIDTH),
        static_cast<int>(SCREEN_HEIGHT),
        STAR_COUNT
    );

    // Tab buttons
    float tabX = (SCREEN_WIDTH - 3 * TAB_BTN_WIDTH - 40) / 2;

    _friendsTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX, TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "FRIENDS",
        FONT_KEY
    );
    _friendsTabBtn->setOnClick([this]() { onFriendsTabClick(); });

    _requestsTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX + TAB_BTN_WIDTH + 20, TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "REQUESTS",
        FONT_KEY
    );
    _requestsTabBtn->setOnClick([this]() { onRequestsTabClick(); });

    _blockedTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX + 2 * (TAB_BTN_WIDTH + 20), TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "BLOCKED",
        FONT_KEY
    );
    _blockedTabBtn->setOnClick([this]() { onBlockedTabClick(); });

    // Add Friend button
    _addFriendBtn = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH - MARGIN_X - 180, CONTENT_START_Y - 50},
        Vec2f{180, 40},
        "ADD FRIEND",
        FONT_KEY
    );
    _addFriendBtn->setOnClick([this]() { onAddFriendClick(); });
    _addFriendBtn->setNormalColor({50, 120, 80, 255});
    _addFriendBtn->setHoveredColor({70, 150, 100, 255});

    // Back button
    _backBtn = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 80},
        Vec2f{200, 50},
        "BACK",
        FONT_KEY
    );
    _backBtn->setOnClick([this]() { onBackClick(); });

    // Add Friend dialog components
    float dialogCenterX = SCREEN_WIDTH / 2.0f;
    float dialogY = SCREEN_HEIGHT / 2.0f - 100;
    float inputWidth = 350.0f;
    float inputHeight = 45.0f;

    _addFriendEmailInput = std::make_unique<ui::TextInput>(
        Vec2f{dialogCenterX - inputWidth / 2, dialogY + 20},
        Vec2f{inputWidth, inputHeight},
        "Enter email address",
        FONT_KEY
    );
    _addFriendEmailInput->setMaxLength(64);

    _addFriendConfirmBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX - 170, dialogY + 90},
        Vec2f{150, 50},
        "SEND",
        FONT_KEY
    );
    _addFriendConfirmBtn->setOnClick([this]() { onAddFriendConfirm(); });
    _addFriendConfirmBtn->setNormalColor({50, 120, 80, 255});
    _addFriendConfirmBtn->setHoveredColor({70, 150, 100, 255});

    _addFriendCancelBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX + 20, dialogY + 90},
        Vec2f{150, 50},
        "CANCEL",
        FONT_KEY
    );
    _addFriendCancelBtn->setOnClick([this]() { _showAddFriendDialog = false; });
    _addFriendCancelBtn->setNormalColor({100, 60, 60, 255});
    _addFriendCancelBtn->setHoveredColor({130, 80, 80, 255});

    _uiInitialized = true;
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] UI initialized");

    // Load initial data
    requestFriendsList();
    requestUnreadCounts();
}

void FriendsScene::handleEvent(const events::Event& event) {
    if (!_uiInitialized) return;

    // Handle dialog mode first
    if (_showAddFriendDialog) {
        _addFriendEmailInput->handleEvent(event);
        _addFriendConfirmBtn->handleEvent(event);
        _addFriendCancelBtn->handleEvent(event);

        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Enter) {
                onAddFriendConfirm();
            } else if (keyPressed->key == events::Key::Escape) {
                _showAddFriendDialog = false;
            }
        }
        return;
    }

    // Escape to go back
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& keyEvt = std::get<events::KeyPressed>(event);
        if (keyEvt.key == events::Key::Escape) {
            onBackClick();
            return;
        }
        // Scroll with arrow keys
        if (keyEvt.key == events::Key::Up && _scrollOffset > 0) {
            _scrollOffset--;
        }
        if (keyEvt.key == events::Key::Down) {
            size_t maxItems = 0;
            if (_currentTab == Tab::Friends) maxItems = _friends.size();
            else if (_currentTab == Tab::Requests) maxItems = _incomingRequests.size() + _outgoingRequests.size();
            else if (_currentTab == Tab::Blocked) maxItems = _blockedUsers.size();

            if (_scrollOffset < static_cast<int>(maxItems) - VISIBLE_ENTRIES) {
                _scrollOffset++;
            }
        }
    }

    // Tab buttons
    _friendsTabBtn->handleEvent(event);
    _requestsTabBtn->handleEvent(event);
    _blockedTabBtn->handleEvent(event);
    // Add friend button only on Friends tab
    if (_currentTab == Tab::Friends) {
        _addFriendBtn->handleEvent(event);
    }
    _backBtn->handleEvent(event);

    // Handle row action button clicks
    if (std::holds_alternative<events::MouseButtonPressed>(event)) {
        auto& mouseEvt = std::get<events::MouseButtonPressed>(event);
        if (mouseEvt.button == events::MouseButton::Left) {
            float mx = static_cast<float>(mouseEvt.x);
            float my = static_cast<float>(mouseEvt.y);

            // Check clicks on action buttons in rows
            if (_currentTab == Tab::Friends) {
                float rowY = CONTENT_START_Y + 50;
                for (size_t i = _scrollOffset; i < std::min(static_cast<size_t>(_scrollOffset + VISIBLE_ENTRIES), _friends.size()); ++i) {
                    // Message button area (70x30 at SCREEN_WIDTH - MARGIN_X - 280)
                    if (mx >= SCREEN_WIDTH - MARGIN_X - 280 && mx <= SCREEN_WIDTH - MARGIN_X - 210 &&
                        my >= rowY && my <= rowY + 30) {
                        onMessageFriend(_friends[i].email);
                        return;
                    }
                    // Block button area (70x30 at SCREEN_WIDTH - MARGIN_X - 190)
                    if (mx >= SCREEN_WIDTH - MARGIN_X - 190 && mx <= SCREEN_WIDTH - MARGIN_X - 120 &&
                        my >= rowY && my <= rowY + 30) {
                        onBlockFriend(_friends[i].email);
                        return;
                    }
                    // Remove button area (80x30 at SCREEN_WIDTH - MARGIN_X - 100)
                    if (mx >= SCREEN_WIDTH - MARGIN_X - 100 && mx <= SCREEN_WIDTH - MARGIN_X - 20 &&
                        my >= rowY && my <= rowY + 30) {
                        onRemoveFriend(_friends[i].email);
                        return;
                    }
                    rowY += ROW_HEIGHT;
                }
            } else if (_currentTab == Tab::Requests) {
                // Combined list: incoming requests first, then outgoing
                // We need to account for _scrollOffset when determining which items are visible
                size_t totalRequests = _incomingRequests.size() + _outgoingRequests.size();
                size_t startIdx = static_cast<size_t>(_scrollOffset);
                size_t endIdx = std::min(startIdx + static_cast<size_t>(VISIBLE_ENTRIES), totalRequests);

                float rowY = CONTENT_START_Y + 50;  // After header

                for (size_t visibleIdx = startIdx; visibleIdx < endIdx; ++visibleIdx) {
                    if (visibleIdx < _incomingRequests.size()) {
                        // This is an incoming request
                        size_t i = visibleIdx;
                        // Accept button (70x30 at SCREEN_WIDTH - MARGIN_X - 190)
                        if (mx >= SCREEN_WIDTH - MARGIN_X - 190 && mx <= SCREEN_WIDTH - MARGIN_X - 120 &&
                            my >= rowY && my <= rowY + 30) {
                            onAcceptRequest(_incomingRequests[i].email);
                            return;
                        }
                        // Reject button (80x30 at SCREEN_WIDTH - MARGIN_X - 100)
                        if (mx >= SCREEN_WIDTH - MARGIN_X - 100 && mx <= SCREEN_WIDTH - MARGIN_X - 20 &&
                            my >= rowY && my <= rowY + 30) {
                            onRejectRequest(_incomingRequests[i].email);
                            return;
                        }
                    } else {
                        // This is an outgoing request
                        size_t i = visibleIdx - _incomingRequests.size();
                        // Cancel button (80x30 at SCREEN_WIDTH - MARGIN_X - 100)
                        if (mx >= SCREEN_WIDTH - MARGIN_X - 100 && mx <= SCREEN_WIDTH - MARGIN_X - 20 &&
                            my >= rowY && my <= rowY + 30) {
                            onCancelRequest(_outgoingRequests[i].email);
                            return;
                        }
                    }
                    rowY += ROW_HEIGHT;
                }
            } else if (_currentTab == Tab::Blocked) {
                float rowY = CONTENT_START_Y + 50;
                for (size_t i = _scrollOffset; i < std::min(static_cast<size_t>(_scrollOffset + VISIBLE_ENTRIES), _blockedUsers.size()); ++i) {
                    // Unblock button (80x30 at SCREEN_WIDTH - MARGIN_X - 100)
                    if (mx >= SCREEN_WIDTH - MARGIN_X - 100 && mx <= SCREEN_WIDTH - MARGIN_X - 20 &&
                        my >= rowY && my <= rowY + 30) {
                        onUnblockUser(_blockedUsers[i].email);
                        return;
                    }
                    rowY += ROW_HEIGHT;
                }
            }
        }
    }
}

void FriendsScene::update(float deltaTime) {
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    // Update starfield
    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Loading animation
    _loadingDots += deltaTime * 3.0f;
    if (_loadingDots > 4.0f) _loadingDots = 0.0f;

    // Update buttons
    if (_friendsTabBtn) _friendsTabBtn->update(deltaTime);
    if (_requestsTabBtn) _requestsTabBtn->update(deltaTime);
    if (_blockedTabBtn) _blockedTabBtn->update(deltaTime);
    // Add friend button only on Friends tab
    if (_currentTab == Tab::Friends && _addFriendBtn) {
        _addFriendBtn->update(deltaTime);
    }
    if (_backBtn) _backBtn->update(deltaTime);

    // Update dialog components
    if (_showAddFriendDialog) {
        _addFriendEmailInput->update(deltaTime);
        _addFriendConfirmBtn->update(deltaTime);
        _addFriendCancelBtn->update(deltaTime);
    }

    // Update status timer
    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }

    // Process incoming TCP messages
    processTCPEvents();
}

void FriendsScene::processTCPEvents() {
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPFriendsListEvent>) {
                onFriendsListReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendRequestsEvent>) {
                onFriendRequestsReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPBlockedUsersEvent>) {
                onBlockedUsersReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendRequestAckEvent>) {
                onFriendRequestAck(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendStatusChangedEvent>) {
                onFriendStatusChanged(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPConversationsListEvent>) {
                onConversationsListReceived(event);
            } else if constexpr (std::is_same_v<T, client::network::TCPPrivateMessageReceivedEvent>) {
                // Real-time notification: increment unread count for sender
                auto it = _unreadCounts.find(event.senderEmail);
                if (it != _unreadCounts.end()) {
                    it->second++;
                } else {
                    _unreadCounts[event.senderEmail] = 1;
                }
                showInfo("New message from " + event.senderDisplayName);
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendRequestReceivedEvent>) {
                showInfo("New friend request from " + event.fromDisplayName);
                // Refresh requests list
                requestFriendRequests();
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendRequestAcceptedEvent>) {
                showInfo(event.displayName + " accepted your friend request!");
                requestFriendsList();
            } else if constexpr (std::is_same_v<T, client::network::TCPFriendRemovedEvent>) {
                showInfo("Friend removed");
                requestFriendsList();
            } else if constexpr (std::is_same_v<T, client::network::TCPAcceptFriendRequestAckEvent>) {
                if (event.errorCode == 0) {
                    showInfo("Friend request accepted!");
                    requestFriendsList();
                    requestFriendRequests();
                } else {
                    showError("Failed to accept friend request");
                }
            } else if constexpr (std::is_same_v<T, client::network::TCPRejectFriendRequestAckEvent>) {
                if (event.errorCode == 0) {
                    showInfo("Friend request rejected");
                    requestFriendRequests();
                } else {
                    showError("Failed to reject friend request");
                }
            } else if constexpr (std::is_same_v<T, client::network::TCPRemoveFriendAckEvent>) {
                if (event.errorCode == 0) {
                    showInfo("Friend removed");
                    requestFriendsList();
                } else {
                    showError("Failed to remove friend");
                }
            } else if constexpr (std::is_same_v<T, client::network::TCPBlockUserAckEvent>) {
                if (event.errorCode == 0) {
                    showInfo("User blocked");
                    requestFriendsList();
                    requestBlockedUsers();
                } else {
                    showError("Failed to block user");
                }
            } else if constexpr (std::is_same_v<T, client::network::TCPUnblockUserAckEvent>) {
                if (event.errorCode == 0) {
                    showInfo("User unblocked");
                    requestBlockedUsers();
                } else {
                    showError("Failed to unblock user");
                }
            } else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from server");
            } else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
        }, *eventOpt);
    }
}

void FriendsScene::render() {
    if (!_assetsLoaded || !_uiInitialized) return;

    // Draw starfield
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Title
    _context.window->drawText(FONT_KEY, "FRIENDS",
                              SCREEN_WIDTH / 2 - 80, 20, 28, {255, 215, 0, 255});

    // Highlight active tab
    rgba activeColor{100, 200, 255, 255};
    rgba inactiveColor{80, 80, 80, 255};

    if (_currentTab == Tab::Friends) {
        _friendsTabBtn->setNormalColor(activeColor);
        _requestsTabBtn->setNormalColor(inactiveColor);
        _blockedTabBtn->setNormalColor(inactiveColor);
    } else if (_currentTab == Tab::Requests) {
        _friendsTabBtn->setNormalColor(inactiveColor);
        _requestsTabBtn->setNormalColor(activeColor);
        _blockedTabBtn->setNormalColor(inactiveColor);
    } else {
        _friendsTabBtn->setNormalColor(inactiveColor);
        _requestsTabBtn->setNormalColor(inactiveColor);
        _blockedTabBtn->setNormalColor(activeColor);
    }

    // Tab buttons
    _friendsTabBtn->render(*_context.window);
    _requestsTabBtn->render(*_context.window);
    _blockedTabBtn->render(*_context.window);

    // Add friend button (only on Friends tab)
    if (_currentTab == Tab::Friends) {
        _addFriendBtn->render(*_context.window);
    }

    // Render current tab content
    switch (_currentTab) {
        case Tab::Friends:
            renderFriendsTab();
            break;
        case Tab::Requests:
            renderRequestsTab();
            break;
        case Tab::Blocked:
            renderBlockedTab();
            break;
    }

    // Back button
    _backBtn->render(*_context.window);

    // Status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 130, 18, _statusColor);
    }

    // Dialog overlay
    if (_showAddFriendDialog) {
        renderAddFriendDialog();
    }
}

void FriendsScene::renderFriendsTab() {
    if (_friendsLoading) {
        renderLoadingIndicator();
        return;
    }

    if (_friends.empty()) {
        _context.window->drawText(FONT_KEY, "No friends yet. Add some friends!",
                                  SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2, 18, {200, 200, 200, 255});
        return;
    }

    // Column headers
    float headerY = CONTENT_START_Y;
    rgba headerColor{200, 200, 200, 255};

    _context.window->drawText(FONT_KEY, "NAME", MARGIN_X, headerY, 16, headerColor);
    _context.window->drawText(FONT_KEY, "STATUS", MARGIN_X + 300, headerY, 16, headerColor);
    _context.window->drawText(FONT_KEY, "ACTIONS", SCREEN_WIDTH - MARGIN_X - 200, headerY, 16, headerColor);

    // Separator line
    _context.window->drawRect(MARGIN_X, headerY + 30, SCREEN_WIDTH - 2 * MARGIN_X, 2, {100, 100, 100, 255});

    // Friend entries
    float rowY = headerY + 50;
    for (size_t i = _scrollOffset; i < std::min(static_cast<size_t>(_scrollOffset + VISIBLE_ENTRIES), _friends.size()); ++i) {
        const auto& friendInfo = _friends[i];
        rgba rowColor{255, 255, 255, 255};

        // Name
        _context.window->drawText(FONT_KEY, friendInfo.displayName, MARGIN_X, rowY, 16, rowColor);

        // Email (smaller, dimmer)
        _context.window->drawText(FONT_KEY, friendInfo.email, MARGIN_X, rowY + 20, 12, {150, 150, 150, 255});

        // Status with color
        std::string statusStr = getStatusString(friendInfo.onlineStatus);
        rgba statusColor = getStatusColor(friendInfo.onlineStatus);
        _context.window->drawText(FONT_KEY, statusStr, MARGIN_X + 300, rowY + 10, 14, statusColor);

        // Room code if in game/lobby
        if (!friendInfo.currentRoomCode.empty() && friendInfo.onlineStatus >= 2) {
            _context.window->drawText(FONT_KEY, "Room: " + friendInfo.currentRoomCode,
                MARGIN_X + 420, rowY + 10, 12, {150, 200, 255, 255});
        }

        // Action buttons (rendered as colored rectangles with text)
        // Message button
        _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 280, rowY, 70, 30, {50, 100, 150, 255});
        _context.window->drawText(FONT_KEY, "MSG", SCREEN_WIDTH - MARGIN_X - 270, rowY + 7, 12, {255, 255, 255, 255});

        // Unread count badge (next to MSG button)
        auto unreadIt = _unreadCounts.find(friendInfo.email);
        if (unreadIt != _unreadCounts.end() && unreadIt->second > 0) {
            // Red badge with unread count
            float badgeX = SCREEN_WIDTH - MARGIN_X - 285;
            float badgeY = rowY - 5;
            float badgeSize = 22.0f;

            // Badge background (red circle approximated with rounded rect)
            _context.window->drawRect(badgeX, badgeY, badgeSize, badgeSize, {220, 60, 60, 255});

            // Badge text
            std::string countStr = std::to_string(unreadIt->second);
            if (unreadIt->second > 9) {
                countStr = "9+";
            }
            float textOffsetX = (unreadIt->second > 9) ? 2.0f : 6.0f;
            _context.window->drawText(FONT_KEY, countStr, badgeX + textOffsetX, badgeY + 4, 11, {255, 255, 255, 255});
        }

        // Block button
        _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 190, rowY, 70, 30, {150, 100, 50, 255});
        _context.window->drawText(FONT_KEY, "BLOCK", SCREEN_WIDTH - MARGIN_X - 183, rowY + 7, 12, {255, 255, 255, 255});

        // Remove button
        _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 100, rowY, 80, 30, {150, 50, 50, 255});
        _context.window->drawText(FONT_KEY, "REMOVE", SCREEN_WIDTH - MARGIN_X - 95, rowY + 7, 12, {255, 255, 255, 255});

        rowY += ROW_HEIGHT;
    }

    // Scroll indicator
    if (_friends.size() > static_cast<size_t>(VISIBLE_ENTRIES)) {
        std::string scrollInfo = std::to_string(_scrollOffset + 1) + "-" +
                                 std::to_string(std::min(_scrollOffset + VISIBLE_ENTRIES, static_cast<int>(_friends.size()))) +
                                 " of " + std::to_string(_friends.size());
        _context.window->drawText(FONT_KEY, scrollInfo,
                                  SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 160, 14, {150, 150, 150, 255});
    }
}

void FriendsScene::renderRequestsTab() {
    if (_requestsLoading) {
        renderLoadingIndicator();
        return;
    }

    size_t totalRequests = _incomingRequests.size() + _outgoingRequests.size();

    if (totalRequests == 0) {
        _context.window->drawText(FONT_KEY, "No friend requests",
                                  SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 18, {200, 200, 200, 255});
        return;
    }

    // Column headers
    float headerY = CONTENT_START_Y;
    rgba headerColor{200, 200, 200, 255};

    _context.window->drawText(FONT_KEY, "NAME", MARGIN_X, headerY, 16, headerColor);
    _context.window->drawText(FONT_KEY, "TYPE", MARGIN_X + 300, headerY, 16, headerColor);
    _context.window->drawText(FONT_KEY, "ACTIONS", SCREEN_WIDTH - MARGIN_X - 200, headerY, 16, headerColor);

    // Separator line
    _context.window->drawRect(MARGIN_X, headerY + 30, SCREEN_WIDTH - 2 * MARGIN_X, 2, {100, 100, 100, 255});

    // Combined list with scroll
    size_t startIdx = static_cast<size_t>(_scrollOffset);
    size_t endIdx = std::min(startIdx + static_cast<size_t>(VISIBLE_ENTRIES), totalRequests);

    float rowY = headerY + 50;

    for (size_t visibleIdx = startIdx; visibleIdx < endIdx; ++visibleIdx) {
        if (visibleIdx < _incomingRequests.size()) {
            // Incoming request
            const auto& req = _incomingRequests[visibleIdx];

            // Name and email
            _context.window->drawText(FONT_KEY, req.displayName, MARGIN_X, rowY, 16, {255, 255, 255, 255});
            _context.window->drawText(FONT_KEY, req.email, MARGIN_X, rowY + 20, 12, {150, 150, 150, 255});

            // Type indicator
            _context.window->drawText(FONT_KEY, "INCOMING", MARGIN_X + 300, rowY + 10, 14, {100, 200, 100, 255});

            // Accept button
            _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 190, rowY, 70, 30, {50, 150, 50, 255});
            _context.window->drawText(FONT_KEY, "ACCEPT", SCREEN_WIDTH - MARGIN_X - 185, rowY + 7, 12, {255, 255, 255, 255});

            // Reject button
            _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 100, rowY, 80, 30, {150, 50, 50, 255});
            _context.window->drawText(FONT_KEY, "REJECT", SCREEN_WIDTH - MARGIN_X - 95, rowY + 7, 12, {255, 255, 255, 255});
        } else {
            // Outgoing request
            const auto& req = _outgoingRequests[visibleIdx - _incomingRequests.size()];

            // Name and email
            _context.window->drawText(FONT_KEY, req.displayName, MARGIN_X, rowY, 16, {255, 255, 255, 255});
            _context.window->drawText(FONT_KEY, req.email, MARGIN_X, rowY + 20, 12, {150, 150, 150, 255});

            // Type indicator
            _context.window->drawText(FONT_KEY, "OUTGOING", MARGIN_X + 300, rowY + 10, 14, {200, 150, 100, 255});

            // Cancel button
            _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 100, rowY, 80, 30, {150, 100, 50, 255});
            _context.window->drawText(FONT_KEY, "CANCEL", SCREEN_WIDTH - MARGIN_X - 95, rowY + 7, 12, {255, 255, 255, 255});
        }

        rowY += ROW_HEIGHT;
    }

    // Scroll indicator
    if (totalRequests > static_cast<size_t>(VISIBLE_ENTRIES)) {
        std::string scrollInfo = std::to_string(_scrollOffset + 1) + "-" +
                                 std::to_string(std::min(_scrollOffset + VISIBLE_ENTRIES, static_cast<int>(totalRequests))) +
                                 " of " + std::to_string(totalRequests);
        _context.window->drawText(FONT_KEY, scrollInfo,
                                  SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 160, 14, {150, 150, 150, 255});
    }
}

void FriendsScene::renderBlockedTab() {
    if (_blockedLoading) {
        renderLoadingIndicator();
        return;
    }

    if (_blockedUsers.empty()) {
        _context.window->drawText(FONT_KEY, "No blocked users",
                                  SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 18, {200, 200, 200, 255});
        return;
    }

    // Column headers
    float headerY = CONTENT_START_Y;
    rgba headerColor{200, 200, 200, 255};

    _context.window->drawText(FONT_KEY, "NAME", MARGIN_X, headerY, 16, headerColor);
    _context.window->drawText(FONT_KEY, "ACTIONS", SCREEN_WIDTH - MARGIN_X - 100, headerY, 16, headerColor);

    // Separator
    _context.window->drawRect(MARGIN_X, headerY + 30, SCREEN_WIDTH - 2 * MARGIN_X, 2, {100, 100, 100, 255});

    // Blocked users list
    float rowY = headerY + 50;
    for (size_t i = _scrollOffset; i < std::min(static_cast<size_t>(_scrollOffset + VISIBLE_ENTRIES), _blockedUsers.size()); ++i) {
        const auto& user = _blockedUsers[i];

        // Name
        _context.window->drawText(FONT_KEY, user.displayName, MARGIN_X, rowY, 16, {255, 255, 255, 255});
        _context.window->drawText(FONT_KEY, user.email, MARGIN_X, rowY + 20, 12, {150, 150, 150, 255});

        // Unblock button
        _context.window->drawRect(SCREEN_WIDTH - MARGIN_X - 100, rowY, 80, 30, {50, 100, 150, 255});
        _context.window->drawText(FONT_KEY, "UNBLOCK", SCREEN_WIDTH - MARGIN_X - 98, rowY + 7, 12, {255, 255, 255, 255});

        rowY += ROW_HEIGHT;
    }
}

void FriendsScene::renderAddFriendDialog() {
    // Semi-transparent overlay
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    // Dialog box
    float dialogX = SCREEN_WIDTH / 2 - 250;
    float dialogY = SCREEN_HEIGHT / 2 - 120;
    float dialogWidth = 500;
    float dialogHeight = 200;

    // Background
    _context.window->drawRect(dialogX, dialogY, dialogWidth, dialogHeight, {25, 25, 50, 250});

    // Border
    _context.window->drawRect(dialogX, dialogY, dialogWidth, 3, {80, 100, 150, 255});
    _context.window->drawRect(dialogX, dialogY + dialogHeight - 3, dialogWidth, 3, {80, 100, 150, 255});
    _context.window->drawRect(dialogX, dialogY, 3, dialogHeight, {80, 100, 150, 255});
    _context.window->drawRect(dialogX + dialogWidth - 3, dialogY, 3, dialogHeight, {80, 100, 150, 255});

    // Title
    _context.window->drawText(FONT_KEY, "ADD FRIEND",
        SCREEN_WIDTH / 2 - 70, dialogY + 20, 24, {255, 255, 255, 255});

    // Label
    _context.window->drawText(FONT_KEY, "Enter friend's email:",
        SCREEN_WIDTH / 2 - 100, dialogY + 65, 16, {180, 180, 200, 255});

    // Input and buttons
    _addFriendEmailInput->render(*_context.window);
    _addFriendConfirmBtn->render(*_context.window);
    _addFriendCancelBtn->render(*_context.window);
}

void FriendsScene::renderLoadingIndicator() {
    std::string dots(static_cast<int>(_loadingDots) % 4, '.');
    _context.window->drawText(FONT_KEY, "Loading" + dots,
                              SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2, 20, {200, 200, 200, 255});
}

std::string FriendsScene::getStatusString(uint8_t status) const {
    switch (status) {
        case 0: return "Offline";
        case 1: return "Online";
        case 2: return "In Lobby";
        case 3: return "In Game";
        default: return "Unknown";
    }
}

rgba FriendsScene::getStatusColor(uint8_t status) const {
    switch (status) {
        case 0: return {150, 150, 150, 255};  // Offline - gray
        case 1: return {100, 200, 100, 255};  // Online - green
        case 2: return {100, 150, 255, 255};  // In Lobby - blue
        case 3: return {255, 200, 100, 255};  // In Game - orange
        default: return {255, 255, 255, 255};
    }
}

void FriendsScene::showError(const std::string& message) {
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void FriendsScene::showInfo(const std::string& message) {
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

// Tab switching
void FriendsScene::onFriendsTabClick() {
    _currentTab = Tab::Friends;
    _scrollOffset = 0;
    if (_friends.empty()) {
        requestFriendsList();
    }
}

void FriendsScene::onRequestsTabClick() {
    _currentTab = Tab::Requests;
    _scrollOffset = 0;
    requestFriendRequests();
}

void FriendsScene::onBlockedTabClick() {
    _currentTab = Tab::Blocked;
    _scrollOffset = 0;
    if (_blockedUsers.empty()) {
        requestBlockedUsers();
    }
}

void FriendsScene::onBackClick() {
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

// Friend actions
void FriendsScene::onAddFriendClick() {
    _showAddFriendDialog = true;
    _addFriendEmailInput->clear();
    _addFriendEmailInput->setFocused(true);
}

void FriendsScene::onAddFriendConfirm() {
    std::string email = _addFriendEmailInput->getText();
    if (email.empty()) {
        showError("Please enter an email address");
        return;
    }

    if (email.find('@') == std::string::npos) {
        showError("Invalid email address");
        return;
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendFriendRequest(email);
        _showAddFriendDialog = false;
        showInfo("Friend request sent!");
    } else {
        showError("Not connected to server");
    }
}

void FriendsScene::onRemoveFriend(const std::string& email) {
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->removeFriend(email);
    }
}

void FriendsScene::onBlockFriend(const std::string& email) {
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->blockUser(email);
    }
}

void FriendsScene::onMessageFriend(const std::string& email) {
    // Find friend info
    for (const auto& f : _friends) {
        if (f.email == email) {
            // Clear unread count for this friend since user is opening the chat
            _unreadCounts.erase(f.email);

            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<PrivateChatScene>(f.email, f.displayName));
            }
            return;
        }
    }
}

// Request actions
void FriendsScene::onAcceptRequest(const std::string& email) {
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->acceptFriendRequest(email);
    }
}

void FriendsScene::onRejectRequest(const std::string& email) {
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->rejectFriendRequest(email);
    }
}

void FriendsScene::onCancelRequest(const std::string& email) {
    // Canceling = rejecting from our side (reuse the same mechanism)
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->rejectFriendRequest(email);
    }
}

// Blocked actions
void FriendsScene::onUnblockUser(const std::string& email) {
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->unblockUser(email);
    }
}

// Data requests
void FriendsScene::requestFriendsList() {
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) return;

    _friendsLoading = true;
    _context.tcpClient->getFriendsList(0);  // 0 = all statuses
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Requested friends list");
}

void FriendsScene::requestFriendRequests() {
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) return;

    _requestsLoading = true;
    _context.tcpClient->getFriendRequests();
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Requested friend requests");
}

void FriendsScene::requestBlockedUsers() {
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) return;

    _blockedLoading = true;
    _context.tcpClient->getBlockedUsers();
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Requested blocked users");
}

void FriendsScene::requestUnreadCounts() {
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) return;

    _context.tcpClient->getConversationsList();
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Requested conversations for unread counts");
}

// Event handlers
void FriendsScene::onFriendsListReceived(const client::network::TCPFriendsListEvent& event) {
    _friendsLoading = false;
    _friends = event.friends;
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Received {} friends", _friends.size());
}

void FriendsScene::onFriendRequestsReceived(const client::network::TCPFriendRequestsEvent& event) {
    _requestsLoading = false;
    _incomingRequests = event.incoming;
    _outgoingRequests = event.outgoing;
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Received {} incoming, {} outgoing requests",
        _incomingRequests.size(), _outgoingRequests.size());
}

void FriendsScene::onBlockedUsersReceived(const client::network::TCPBlockedUsersEvent& event) {
    _blockedLoading = false;
    _blockedUsers = event.blockedUsers;
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Received {} blocked users", _blockedUsers.size());
}

void FriendsScene::onFriendRequestAck(const client::network::TCPFriendRequestAckEvent& event) {
    if (event.errorCode == 0) {
        showInfo("Friend request sent to " + event.targetEmail);
    } else {
        // Error codes: 1 = already friends, 2 = pending request, 3 = blocked, 4 = not found, 5 = self
        switch (event.errorCode) {
            case 1: showError("Already friends with this user"); break;
            case 2: showError("Friend request already pending"); break;
            case 3: showError("You are blocked by this user"); break;
            case 4: showError("User not found"); break;
            case 5: showError("Cannot send friend request to yourself"); break;
            default: showError("Failed to send friend request"); break;
        }
    }
}

void FriendsScene::onFriendStatusChanged(const client::network::TCPFriendStatusChangedEvent& event) {
    // Update friend's status in the list
    for (auto& f : _friends) {
        if (f.email == event.friendEmail) {
            f.onlineStatus = event.newStatus;
            f.currentRoomCode = event.roomCode;
            break;
        }
    }
}

void FriendsScene::onConversationsListReceived(const client::network::TCPConversationsListEvent& event) {
    // Update unread counts from conversations list
    _unreadCounts.clear();
    for (const auto& conv : event.conversations) {
        if (conv.unreadCount > 0) {
            _unreadCounts[conv.otherEmail] = conv.unreadCount;
        }
    }
    client::logging::Logger::getSceneLogger()->info("[FriendsScene] Updated unread counts for {} conversations",
        _unreadCounts.size());
}
