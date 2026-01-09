/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LobbyScene
*/

#include "scenes/LobbyScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/GameScene.hpp"
#include "scenes/MainMenuScene.hpp"
#include <variant>

LobbyScene::LobbyScene(const std::string& roomName, const std::string& roomCode,
                       uint8_t maxPlayers, bool isHost, uint8_t slotId,
                       const std::vector<client::network::RoomPlayerInfo>& initialPlayers)
    : _roomName(roomName)
    , _roomCode(roomCode)
    , _maxPlayers(maxPlayers)
    , _slotId(slotId)
    , _isHost(isHost)
    , _players(initialPlayers)
{
}

void LobbyScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");

    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

void LobbyScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float bottomY = SCREEN_HEIGHT - 120.0f;

    // Ready button (left)
    _readyButton = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - buttonWidth - 120, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "READY",
        FONT_KEY
    );
    _readyButton->setOnClick([this]() { onReadyClick(); });
    _readyButton->setNormalColor({60, 120, 60, 255});
    _readyButton->setHoveredColor({80, 150, 80, 255});

    // Start button (center - host only)
    _startButton = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - buttonWidth / 2, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "START GAME",
        FONT_KEY
    );
    _startButton->setOnClick([this]() { onStartClick(); });
    _startButton->setNormalColor({50, 100, 150, 255});
    _startButton->setHoveredColor({70, 130, 180, 255});

    // Leave button (right)
    _leaveButton = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 + 120, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "LEAVE",
        FONT_KEY
    );
    _leaveButton->setOnClick([this]() { onLeaveClick(); });
    _leaveButton->setNormalColor({120, 60, 60, 255});
    _leaveButton->setHoveredColor({150, 80, 80, 255});

    // Chat input (bottom of chat panel - attached to the right of player box)
    float totalWidth = 900.0f;
    float playerBoxX = (SCREEN_WIDTH - totalWidth) / 2;
    float playerBoxWidth = 500.0f;
    float chatPanelX = playerBoxX + playerBoxWidth + 15;  // Inside chat panel with padding
    float chatInputY = 640.0f;
    float chatInputWidth = 280.0f;
    float chatInputHeight = 40.0f;

    _chatInput = std::make_unique<ui::TextInput>(
        Vec2f{chatPanelX, chatInputY},
        Vec2f{chatInputWidth, chatInputHeight},
        "Type a message...",
        FONT_KEY
    );
    _chatInput->setMaxLength(200);

    // Send chat button
    _sendChatButton = std::make_unique<ui::Button>(
        Vec2f{chatPanelX + chatInputWidth + 10, chatInputY},
        Vec2f{80.0f, chatInputHeight},
        "SEND",
        FONT_KEY
    );
    _sendChatButton->setOnClick([this]() { onSendChatClick(); });
    _sendChatButton->setNormalColor({60, 100, 140, 255});
    _sendChatButton->setHoveredColor({80, 130, 170, 255});

    _uiInitialized = true;
}

void LobbyScene::onReadyClick()
{
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    // Toggle ready state
    _isReady = !_isReady;
    _context.tcpClient->setReady(_isReady);

    // Update button text
    _readyButton->setText(_isReady ? "NOT READY" : "READY");
    _readyButton->setNormalColor(_isReady ? rgba{150, 100, 50, 255} : rgba{60, 120, 60, 255});
    _readyButton->setHoveredColor(_isReady ? rgba{180, 130, 70, 255} : rgba{80, 150, 80, 255});
}

void LobbyScene::onStartClick()
{
    if (!_isHost) {
        showError("Only the host can start the game");
        return;
    }

    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    // Count ready players
    int readyCount = 0;
    for (const auto& player : _players) {
        if (player.isReady || player.isHost) {
            readyCount++;
        }
    }

    if (readyCount < 2) {
        showError("Need at least 2 ready players");
        return;
    }

    _context.tcpClient->startGame();
    showInfo("Starting game...");
}

void LobbyScene::onLeaveClick()
{
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->leaveRoom();
    }

    // Go back to main menu
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void LobbyScene::onKickClick(const std::string& email)
{
    if (!_isHost) {
        showError("Only the host can kick players");
        return;
    }

    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->kickPlayer(email, "");
}

void LobbyScene::onSendChatClick()
{
    if (!_chatInput) return;

    std::string message = _chatInput->getText();
    if (message.empty()) return;

    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->sendChatMessage(message);
    _chatInput->clear();
}

void LobbyScene::appendChatMessage(const client::network::ChatMessageInfo& msg)
{
    _chatMessages.push_back(msg);

    // Keep only last MAX_CHAT_MESSAGES
    while (_chatMessages.size() > MAX_CHAT_MESSAGES) {
        _chatMessages.erase(_chatMessages.begin());
    }
}

void LobbyScene::renderChatPanel()
{
    if (!_context.window) return;

    // Chat panel is attached to the right of the player box
    float totalWidth = 900.0f;
    float playerBoxX = (SCREEN_WIDTH - totalWidth) / 2;
    float playerBoxWidth = 500.0f;
    float panelX = playerBoxX + playerBoxWidth;  // Right next to player box
    float panelY = 150.0f;
    float panelWidth = 400.0f;
    float panelHeight = 550.0f;  // Same height as player box

    // Chat panel background
    _context.window->drawRect(panelX, panelY, panelWidth, panelHeight, {15, 15, 35, 230});

    // Chat panel border (no left border since it's attached to player box)
    _context.window->drawRect(panelX, panelY, panelWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(panelX, panelY + panelHeight - 3, panelWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(panelX + panelWidth - 3, panelY, 3, panelHeight, {60, 80, 120, 255});

    // Chat title
    _context.window->drawText(FONT_KEY, "CHAT",
        panelX + 170, panelY + 10, 24, {150, 150, 180, 255});

    // Separator
    _context.window->drawRect(panelX + 10, panelY + 45, panelWidth - 20, 1, {50, 50, 80, 255});

    // Chat messages area
    float messagesY = panelY + 55;
    float messageSpacing = 48.0f;
    float maxMessagesHeight = panelHeight - 130;  // Leave space for input at bottom
    size_t maxVisible = static_cast<size_t>(maxMessagesHeight / messageSpacing);

    // Calculate start index to show most recent messages
    size_t startIdx = 0;
    if (_chatMessages.size() > maxVisible) {
        startIdx = _chatMessages.size() - maxVisible;
    }

    for (size_t i = startIdx; i < _chatMessages.size(); ++i) {
        const auto& msg = _chatMessages[i];
        float msgY = messagesY + (i - startIdx) * messageSpacing;

        // Display name
        _context.window->drawText(FONT_KEY, msg.displayName + ":",
            panelX + 15, msgY, 14, {100, 180, 255, 255});

        // Message (wrapped if too long)
        std::string displayMsg = msg.message;
        if (displayMsg.length() > 40) {
            displayMsg = displayMsg.substr(0, 37) + "...";
        }
        _context.window->drawText(FONT_KEY, displayMsg,
            panelX + 20, msgY + 18, 14, {200, 200, 220, 255});
    }

    // Render chat input and send button (not during countdown)
    if (!_countdown.has_value()) {
        if (_chatInput) _chatInput->render(*_context.window);
        if (_sendChatButton) _sendChatButton->render(*_context.window);
    }
}

bool LobbyScene::isKickButtonHovered(float playerY) const
{
    // Kick button area: right side of player row
    // Must match render() coordinates exactly
    float totalWidth = 900.0f;
    float boxX = (SCREEN_WIDTH - totalWidth) / 2;
    float boxWidth = 500;
    float kickX = boxX + boxWidth - 100;
    float kickWidth = 60;
    float kickHeight = 30;

    return _mouseX >= kickX && _mouseX <= kickX + kickWidth &&
           _mouseY >= playerY - 5 && _mouseY <= playerY + kickHeight;
}

void LobbyScene::showError(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void LobbyScene::showInfo(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void LobbyScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPRoomUpdateEvent>) {
                // Update room state
                _roomName = event.roomName;
                _roomCode = event.roomCode;
                _maxPlayers = event.maxPlayers;
                _players = event.players;

                // Check if we're still host
                for (const auto& player : _players) {
                    if (player.slotId == _slotId) {
                        _isHost = player.isHost;
                        _isReady = player.isReady;
                        break;
                    }
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPGameStartingEvent>) {
                _countdown = event.countdown;
                _countdownAnimTimer = 0.0f;

                if (event.countdown == 0) {
                    transitionToGame();
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomLeftEvent>) {
                // We left the room, go back to main menu
                if (_sceneManager) {
                    _sceneManager->changeScene(std::make_unique<MainMenuScene>());
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPReadyChangedEvent>) {
                _isReady = event.isReady;
                _readyButton->setText(_isReady ? "NOT READY" : "READY");
            }
            else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from server");
                // Go back to main menu after a short delay
            }
            else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPPlayerKickedEvent>) {
                // We were kicked from the room - show kick screen
                _wasKicked = true;
                _kickReason = event.reason;
            }
            else if constexpr (std::is_same_v<T, client::network::TCPKickSuccessEvent>) {
                showInfo("Player kicked successfully");
            }
            // Chat events
            else if constexpr (std::is_same_v<T, client::network::TCPChatMessageEvent>) {
                appendChatMessage(client::network::ChatMessageInfo{
                    event.displayName,
                    event.message,
                    event.timestamp
                });
            }
            else if constexpr (std::is_same_v<T, client::network::TCPChatHistoryEvent>) {
                _chatMessages.clear();
                for (const auto& msg : event.messages) {
                    appendChatMessage(msg);
                }
            }
        }, *eventOpt);
    }
}

void LobbyScene::processUDPEvents()
{
    // In lobby, we don't process UDP events yet
    // But we consume them to avoid queue buildup
    if (!_context.udpClient) return;

    while (auto eventOpt = _context.udpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::UDPJoinGameAckEvent>) {
                // We successfully joined the game via UDP
                // Transition to GameScene
                if (_sceneManager && _transitioningToGame) {
                    _sceneManager->changeScene(std::make_unique<GameScene>());
                }
            }
            else if constexpr (std::is_same_v<T, client::network::UDPJoinGameNackEvent>) {
                showError("Failed to join game: " + event.reason);
                _transitioningToGame = false;
            }
        }, *eventOpt);
    }
}

void LobbyScene::transitionToGame()
{
    if (_transitioningToGame) return;
    _transitioningToGame = true;

    // Send JoinGame to UDP server with session token
    if (_context.udpClient && _context.tcpClient && _context.tcpClient->hasSessionToken()) {
        auto tokenOpt = _context.tcpClient->getSessionToken();
        if (tokenOpt) {
            _context.udpClient->joinGame(*tokenOpt);
            showInfo("Joining game...");
        } else {
            showError("No session token available");
            _transitioningToGame = false;
        }
    } else {
        showError("Not connected to game server");
        _transitioningToGame = false;
    }
}

void LobbyScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    // If kicked, any key returns to main menu
    if (_wasKicked) {
        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            (void)keyPressed;  // Any key works
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<MainMenuScene>());
            }
        }
        return;
    }

    // Track mouse position
    if (auto* mouseMoved = std::get_if<events::MouseMoved>(&event)) {
        _mouseX = static_cast<float>(mouseMoved->x);
        _mouseY = static_cast<float>(mouseMoved->y);
    }

    // Don't handle input during countdown
    if (_countdown.has_value()) return;

    // Host doesn't have a Ready button (always ready)
    if (!_isHost) {
        _readyButton->handleEvent(event);
    }
    if (_isHost) {
        _startButton->handleEvent(event);
    }
    _leaveButton->handleEvent(event);

    // Handle kick button clicks (host only)
    if (_isHost) {
        if (auto* mousePressed = std::get_if<events::MouseButtonPressed>(&event)) {
            if (mousePressed->button == events::MouseButton::Left) {
                // Update mouse position from click
                _mouseX = static_cast<float>(mousePressed->x);
                _mouseY = static_cast<float>(mousePressed->y);

                // Check if clicking on a kick button
                // Coordinates must match render() exactly
                float boxY = 150;
                float listY = boxY + 160;
                float playerY = listY + 55;
                float playerSpacing = 50.0f;

                for (const auto& player : _players) {
                    // Can't kick self or other hosts
                    if (player.slotId != _slotId && !player.isHost) {
                        if (isKickButtonHovered(playerY)) {
                            onKickClick(player.email);
                            break;
                        }
                    }
                    playerY += playerSpacing;
                }
            }
        }
    }

    // Handle chat input and send button
    if (_chatInput) _chatInput->handleEvent(event);
    if (_sendChatButton) _sendChatButton->handleEvent(event);

    // Escape to leave, Enter to send chat
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            onLeaveClick();
        } else if (keyPressed->key == events::Key::Enter) {
            // If chat input has focus, send message
            if (_chatInput && _chatInput->isFocused()) {
                onSendChatClick();
            }
        }
    }
}

void LobbyScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    // Always process TCP events (to receive kick notification)
    processTCPEvents();
    processUDPEvents();

    // If kicked, only update starfield for visual effect
    if (_wasKicked) {
        if (_starfield) {
            _starfield->update(deltaTime);
        }
        return;
    }

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Host doesn't have a Ready button
    if (!_isHost) {
        _readyButton->update(deltaTime);
    }
    _startButton->update(deltaTime);
    _leaveButton->update(deltaTime);

    // Update chat UI
    if (_chatInput) _chatInput->update(deltaTime);
    if (_sendChatButton) _sendChatButton->update(deltaTime);

    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }

    // Countdown animation
    if (_countdown.has_value()) {
        _countdownAnimTimer += deltaTime;
    }
}

void LobbyScene::render()
{
    if (!_context.window) return;

    // Draw space background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {8, 8, 25, 255});

    // Draw stars
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // If kicked, show kick screen overlay
    if (_wasKicked) {
        renderKickedScreen();
        return;
    }

    // Draw title
    _context.window->drawText(FONT_KEY, "R-TYPE",
        SCREEN_WIDTH / 2 - 150, 40, 80, {100, 150, 255, 255});

    // Draw room info box (players list) - centered with chat attached to the right
    float totalWidth = 900.0f;  // Player box (500) + Chat box (400)
    float boxX = (SCREEN_WIDTH - totalWidth) / 2;
    float boxY = 150;
    float boxWidth = 500;
    float boxHeight = 550;

    // Box background
    _context.window->drawRect(boxX, boxY, boxWidth, boxHeight, {20, 20, 45, 230});

    // Box border
    _context.window->drawRect(boxX, boxY, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY + boxHeight - 3, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY, 3, boxHeight, {60, 80, 120, 255});
    _context.window->drawRect(boxX + boxWidth - 3, boxY, 3, boxHeight, {60, 80, 120, 255});

    // Room name (centered in player box)
    _context.window->drawText(FONT_KEY, _roomName,
        boxX + boxWidth / 2 - 100, boxY + 20, 36, {255, 255, 255, 255});

    // Room code with label
    std::string codeText = "Code: " + _roomCode;
    _context.window->drawText(FONT_KEY, codeText,
        boxX + boxWidth / 2 - 70, boxY + 70, 28, {100, 200, 255, 255});

    // Player count
    std::string playerCountText = "Players: " + std::to_string(_players.size()) + "/" + std::to_string(_maxPlayers);
    _context.window->drawText(FONT_KEY, playerCountText,
        boxX + boxWidth / 2 - 60, boxY + 110, 20, {180, 180, 200, 255});

    // Players list header
    float listY = boxY + 160;
    _context.window->drawText(FONT_KEY, "PLAYERS",
        boxX + 40, listY, 24, {150, 150, 180, 255});
    _context.window->drawText(FONT_KEY, "STATUS",
        boxX + boxWidth - 200, listY, 24, {150, 150, 180, 255});

    // Separator line
    _context.window->drawRect(boxX + 30, listY + 35, boxWidth - 60, 2, {60, 60, 90, 255});

    // Players list
    float playerY = listY + 55;
    float playerSpacing = 50.0f;

    for (const auto& player : _players) {
        // Player name with host indicator
        std::string displayName = player.displayName;
        if (player.isHost) {
            displayName += " [HOST]";
        }
        if (player.slotId == _slotId) {
            displayName += " (you)";
        }

        rgba nameColor = player.isHost ? rgba{255, 200, 100, 255} : rgba{255, 255, 255, 255};
        if (player.slotId == _slotId) {
            nameColor = {100, 200, 255, 255};
        }

        _context.window->drawText(FONT_KEY, displayName,
            boxX + 40, playerY, 22, nameColor);

        // Ready status
        std::string statusText = player.isReady ? "READY" : "NOT READY";
        rgba statusColor = player.isReady ? rgba{100, 255, 100, 255} : rgba{200, 100, 100, 255};
        // Host is always considered ready
        if (player.isHost) {
            statusText = "HOST";
            statusColor = {255, 200, 100, 255};
        }

        _context.window->drawText(FONT_KEY, statusText,
            boxX + boxWidth - 200, playerY, 20, statusColor);

        // Kick button (host only, not for self or other hosts)
        if (_isHost && player.slotId != _slotId && !player.isHost && !_countdown.has_value()) {
            float kickX = boxX + boxWidth - 100;
            float kickWidth = 60;
            float kickHeight = 30;
            bool hovered = isKickButtonHovered(playerY);

            rgba kickBgColor = hovered ? rgba{180, 60, 60, 255} : rgba{120, 50, 50, 255};
            _context.window->drawRect(kickX, playerY - 5, kickWidth, kickHeight, kickBgColor);
            _context.window->drawText(FONT_KEY, "KICK",
                kickX + 8, playerY, 16, {255, 255, 255, 255});
        }

        playerY += playerSpacing;
    }

    // Empty slots
    for (size_t i = _players.size(); i < _maxPlayers; ++i) {
        _context.window->drawText(FONT_KEY, "- Empty -",
            boxX + 40, playerY, 20, {80, 80, 100, 255});
        playerY += playerSpacing;
    }

    // Draw chat panel on the right side
    renderChatPanel();

    // Draw countdown overlay if active
    if (_countdown.has_value()) {
        // Semi-transparent overlay
        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

        // Countdown number with animation
        float scale = 1.0f + 0.2f * std::sin(_countdownAnimTimer * 10.0f);
        int fontSize = static_cast<int>(150 * scale);
        std::string countdownText = std::to_string(*_countdown);
        if (*_countdown == 0) {
            countdownText = "GO!";
            fontSize = 120;
        }

        _context.window->drawText(FONT_KEY, countdownText,
            SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 80, fontSize, {255, 255, 100, 255});

        _context.window->drawText(FONT_KEY, "Game starting...",
            SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 100, 28, {200, 200, 220, 255});
    } else {
        // Draw buttons (only when not in countdown)
        // Host doesn't have a Ready button (always ready)
        if (!_isHost) {
            _readyButton->render(*_context.window);
        }
        if (_isHost) {
            _startButton->render(*_context.window);
        }
        _leaveButton->render(*_context.window);
    }

    // Draw status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 60, 18, _statusColor);
    }

    // Draw connection status
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 20, SCREEN_HEIGHT - 40, 14, connColor);
}

void LobbyScene::renderKickedScreen()
{
    // Semi-transparent overlay
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    const std::string kickedText = "KICKED";
    float textX = SCREEN_WIDTH / 2.0f - 80.0f;
    float textY = SCREEN_HEIGHT / 2.0f - 70.0f;

    _context.window->drawText(FONT_KEY, kickedText, textX, textY, 48, {255, 150, 50, 255});

    const std::string instructionText = "You have been kicked from the room";
    float instrX = SCREEN_WIDTH / 2.0f - 180.0f;
    float instrY = textY + 70.0f;
    _context.window->drawText(FONT_KEY, instructionText, instrX, instrY, 24, {200, 200, 200, 255});

    // Display kick reason if available
    if (!_kickReason.empty()) {
        std::string reasonText = "Reason: " + _kickReason;
        float reasonX = SCREEN_WIDTH / 2.0f - 150.0f;
        float reasonY = instrY + 40.0f;
        _context.window->drawText(FONT_KEY, reasonText, reasonX, reasonY, 20, {255, 200, 100, 255});
    }

    const std::string hintText = "Press any key to return to menu";
    float hintX = SCREEN_WIDTH / 2.0f - 150.0f;
    float hintY = _kickReason.empty() ? (instrY + 50.0f) : (instrY + 90.0f);
    _context.window->drawText(FONT_KEY, hintText, hintX, hintY, 18, {150, 150, 150, 255});
}
