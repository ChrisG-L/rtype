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
                       uint8_t maxPlayers, bool isHost, uint8_t slotId)
    : _roomName(roomName)
    , _roomCode(roomCode)
    , _maxPlayers(maxPlayers)
    , _slotId(slotId)
    , _isHost(isHost)
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

bool LobbyScene::isKickButtonHovered(float playerY) const
{
    // Kick button area: right side of player row
    float boxX = SCREEN_WIDTH / 2 - 400;
    float boxWidth = 800;
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
                // We were kicked from the room
                showError("You were kicked: " + (event.reason.empty() ? "No reason given" : event.reason));
                if (_sceneManager) {
                    _sceneManager->changeScene(std::make_unique<MainMenuScene>());
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPKickSuccessEvent>) {
                showInfo("Player kicked successfully");
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

    // Escape to leave
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            onLeaveClick();
        }
    }
}

void LobbyScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    processTCPEvents();
    processUDPEvents();

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Host doesn't have a Ready button
    if (!_isHost) {
        _readyButton->update(deltaTime);
    }
    _startButton->update(deltaTime);
    _leaveButton->update(deltaTime);

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

    // Draw title
    _context.window->drawText(FONT_KEY, "R-TYPE",
        SCREEN_WIDTH / 2 - 150, 40, 80, {100, 150, 255, 255});

    // Draw room info box
    float boxX = SCREEN_WIDTH / 2 - 400;
    float boxY = 150;
    float boxWidth = 800;
    float boxHeight = 550;

    // Box background
    _context.window->drawRect(boxX, boxY, boxWidth, boxHeight, {20, 20, 45, 230});

    // Box border
    _context.window->drawRect(boxX, boxY, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY + boxHeight - 3, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY, 3, boxHeight, {60, 80, 120, 255});
    _context.window->drawRect(boxX + boxWidth - 3, boxY, 3, boxHeight, {60, 80, 120, 255});

    // Room name
    _context.window->drawText(FONT_KEY, _roomName,
        SCREEN_WIDTH / 2 - 150, boxY + 20, 36, {255, 255, 255, 255});

    // Room code with label
    std::string codeText = "Code: " + _roomCode;
    _context.window->drawText(FONT_KEY, codeText,
        SCREEN_WIDTH / 2 - 80, boxY + 70, 28, {100, 200, 255, 255});

    // Player count
    std::string playerCountText = "Players: " + std::to_string(_players.size()) + "/" + std::to_string(_maxPlayers);
    _context.window->drawText(FONT_KEY, playerCountText,
        SCREEN_WIDTH / 2 - 60, boxY + 110, 20, {180, 180, 200, 255});

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
