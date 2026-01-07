/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MainMenuScene
*/

#include "scenes/MainMenuScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/LobbyScene.hpp"
#include "scenes/SettingsScene.hpp"
#include "scenes/RoomBrowserScene.hpp"
#include "accessibility/AccessibilityConfig.hpp"
#include <variant>

MainMenuScene::MainMenuScene()
{
}

void MainMenuScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");

    // Initialize starfield
    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

void MainMenuScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float centerX = SCREEN_WIDTH / 2.0f;
    float buttonWidth = 280.0f;
    float buttonHeight = 60.0f;
    float startY = 400.0f;
    float spacing = 80.0f;

    // Create Room button
    _createRoomButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY},
        Vec2f{buttonWidth, buttonHeight},
        "CREATE ROOM",
        FONT_KEY
    );
    _createRoomButton->setOnClick([this]() { onCreateRoomClick(); });
    _createRoomButton->setNormalColor({50, 120, 80, 255});
    _createRoomButton->setHoveredColor({70, 150, 100, 255});

    // Join Room button
    _joinRoomButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing},
        Vec2f{buttonWidth, buttonHeight},
        "JOIN ROOM",
        FONT_KEY
    );
    _joinRoomButton->setOnClick([this]() { onJoinRoomClick(); });
    _joinRoomButton->setNormalColor({50, 100, 150, 255});
    _joinRoomButton->setHoveredColor({70, 130, 180, 255});

    // Browse Rooms button
    _browseRoomsButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing * 2},
        Vec2f{buttonWidth, buttonHeight},
        "BROWSE ROOMS",
        FONT_KEY
    );
    _browseRoomsButton->setOnClick([this]() { onBrowseRoomsClick(); });
    _browseRoomsButton->setNormalColor({80, 80, 120, 255});
    _browseRoomsButton->setHoveredColor({100, 100, 150, 255});

    // Quick Join button
    _quickJoinButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing * 3},
        Vec2f{buttonWidth, buttonHeight},
        "QUICK JOIN",
        FONT_KEY
    );
    _quickJoinButton->setOnClick([this]() { onQuickJoinClick(); });
    _quickJoinButton->setNormalColor({120, 100, 50, 255});
    _quickJoinButton->setHoveredColor({150, 130, 70, 255});

    // Settings button
    _settingsButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing * 4},
        Vec2f{buttonWidth, buttonHeight},
        "SETTINGS",
        FONT_KEY
    );
    _settingsButton->setOnClick([this]() { onSettingsClick(); });

    // Quit button
    _quitButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing * 5},
        Vec2f{buttonWidth, buttonHeight},
        "QUIT",
        FONT_KEY
    );
    _quitButton->setOnClick([this]() { onQuitClick(); });
    _quitButton->setNormalColor({100, 50, 50, 255});
    _quitButton->setHoveredColor({150, 70, 70, 255});

    // === Create Room Dialog Components ===
    float dialogCenterX = SCREEN_WIDTH / 2.0f;
    float dialogY = SCREEN_HEIGHT / 2.0f - 150;
    float inputWidth = 300.0f;
    float inputHeight = 45.0f;

    // Room name input
    _roomNameInput = std::make_unique<ui::TextInput>(
        Vec2f{dialogCenterX - inputWidth / 2, dialogY},
        Vec2f{inputWidth, inputHeight},
        "Room Name",
        FONT_KEY
    );
    _roomNameInput->setMaxLength(32);

    // Max players controls
    float controlY = dialogY + 70;
    float smallBtnWidth = 50.0f;

    _maxPlayersDecBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX - 80, controlY},
        Vec2f{smallBtnWidth, inputHeight},
        "-",
        FONT_KEY
    );
    _maxPlayersDecBtn->setOnClick([this]() {
        if (_maxPlayers > 2) _maxPlayers--;
    });

    _maxPlayersIncBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX + 30, controlY},
        Vec2f{smallBtnWidth, inputHeight},
        "+",
        FONT_KEY
    );
    _maxPlayersIncBtn->setOnClick([this]() {
        if (_maxPlayers < 6) _maxPlayers++;
    });

    // Public/Private toggle
    float toggleY = dialogY + 140;
    _publicPrivateBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX - 75, toggleY},
        Vec2f{150, inputHeight},
        "PUBLIC",
        FONT_KEY
    );
    _publicPrivateBtn->setOnClick([this]() {
        _isPrivate = !_isPrivate;
        _publicPrivateBtn->setText(_isPrivate ? "PRIVATE" : "PUBLIC");
        _publicPrivateBtn->setNormalColor(_isPrivate ? rgba{120, 80, 50, 255} : rgba{50, 80, 120, 255});
    });
    _publicPrivateBtn->setNormalColor({50, 80, 120, 255});

    // Create confirm button
    float confirmY = dialogY + 220;
    _createConfirmBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX - 170, confirmY},
        Vec2f{150, 50},
        "CREATE",
        FONT_KEY
    );
    _createConfirmBtn->setOnClick([this]() { onCreateRoomConfirm(); });
    _createConfirmBtn->setNormalColor({50, 120, 80, 255});
    _createConfirmBtn->setHoveredColor({70, 150, 100, 255});

    // Cancel button (shared between dialogs)
    _cancelBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX + 20, confirmY},
        Vec2f{150, 50},
        "CANCEL",
        FONT_KEY
    );
    _cancelBtn->setOnClick([this]() { closeDialog(); });
    _cancelBtn->setNormalColor({100, 60, 60, 255});
    _cancelBtn->setHoveredColor({130, 80, 80, 255});

    // === Join Room Dialog Components ===
    _roomCodeInput = std::make_unique<ui::TextInput>(
        Vec2f{dialogCenterX - inputWidth / 2, dialogY + 20},
        Vec2f{inputWidth, inputHeight},
        "Enter Room Code",
        FONT_KEY
    );
    _roomCodeInput->setMaxLength(6);

    _joinConfirmBtn = std::make_unique<ui::Button>(
        Vec2f{dialogCenterX - 170, dialogY + 100},
        Vec2f{150, 50},
        "JOIN",
        FONT_KEY
    );
    _joinConfirmBtn->setOnClick([this]() { onJoinRoomConfirm(); });
    _joinConfirmBtn->setNormalColor({50, 100, 150, 255});
    _joinConfirmBtn->setHoveredColor({70, 130, 180, 255});

    _uiInitialized = true;
}

void MainMenuScene::onCreateRoomClick()
{
    _dialogMode = DialogMode::CreateRoom;
    _roomNameInput->clear();
    _roomNameInput->setFocused(true);
    _maxPlayers = 4;
    _isPrivate = false;
    _publicPrivateBtn->setText("PUBLIC");
    _publicPrivateBtn->setNormalColor({50, 80, 120, 255});
}

void MainMenuScene::onJoinRoomClick()
{
    _dialogMode = DialogMode::JoinRoom;
    _roomCodeInput->clear();
    _roomCodeInput->setFocused(true);
}

void MainMenuScene::onBrowseRoomsClick()
{
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<RoomBrowserScene>());
    }
}

void MainMenuScene::onQuickJoinClick()
{
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->quickJoin();
    showInfo("Searching for a game...");
}

void MainMenuScene::onCreateRoomConfirm()
{
    std::string roomName = _roomNameInput->getText();
    if (roomName.empty()) {
        showError("Please enter a room name");
        return;
    }

    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->createRoom(roomName, _maxPlayers, _isPrivate);
    _roomState = RoomState::WaitingForCreate;
    showInfo("Creating room...");
}

void MainMenuScene::onJoinRoomConfirm()
{
    std::string roomCode = _roomCodeInput->getText();
    if (roomCode.empty()) {
        showError("Please enter a room code");
        return;
    }

    if (roomCode.length() != 6) {
        showError("Room code must be 6 characters");
        return;
    }

    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->joinRoomByCode(roomCode);
    _roomState = RoomState::WaitingForJoin;
    showInfo("Joining room...");
}

void MainMenuScene::closeDialog()
{
    _dialogMode = DialogMode::None;
    _roomState = RoomState::Idle;
}

void MainMenuScene::showError(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void MainMenuScene::showInfo(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void MainMenuScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    bool sceneChanging = false;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this, &sceneChanging](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPRoomCreatedEvent>) {
                // Room created successfully, transition to lobby
                showInfo("Room created: " + event.roomCode);
                closeDialog();

                if (_sceneManager) {
                    std::string roomName = _roomNameInput->getText();
                    std::string displayName = _context.tcpClient->getDisplayName();
                    std::string userEmail = _context.tcpClient->getEmail();

                    // Host is the only player initially
                    std::vector<client::network::RoomPlayerInfo> players;
                    players.push_back(client::network::RoomPlayerInfo{
                        0,           // slotId
                        displayName,
                        userEmail,
                        true,        // isReady (host is always ready)
                        true         // isHost
                    });

                    _sceneManager->changeScene(std::make_unique<LobbyScene>(
                        roomName,
                        event.roomCode,
                        _maxPlayers,
                        true,  // isHost
                        0,     // slotId (host is always 0)
                        players
                    ));
                    sceneChanging = true;
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomCreateFailedEvent>) {
                showError(event.message);
                _roomState = RoomState::Idle;
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomJoinedEvent>) {
                // Joined room successfully, transition to lobby
                showInfo("Joined room!");
                closeDialog();

                if (_sceneManager) {
                    _sceneManager->changeScene(std::make_unique<LobbyScene>(
                        event.roomName,
                        event.roomCode,
                        event.maxPlayers,
                        event.isHost,
                        event.slotId,
                        event.players  // Pass initial player list
                    ));
                    sceneChanging = true;
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomJoinFailedEvent>) {
                showError(event.message);
                _roomState = RoomState::Idle;
            }
            else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from server");
                closeDialog();
            }
            else if constexpr (std::is_same_v<T, client::network::TCPQuickJoinFailedEvent>) {
                showError("Quick join failed: " + event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPUserSettingsEvent>) {
                // Apply received settings to AccessibilityConfig singleton
                auto& config = accessibility::AccessibilityConfig::getInstance();
                if (event.found) {
                    config.setColorBlindMode(
                        accessibility::AccessibilityConfig::stringToColorBlindMode(event.colorBlindMode));
                    config.setGameSpeedMultiplier(event.gameSpeed);

                    for (size_t i = 0; i < 6; ++i) {
                        auto action = static_cast<accessibility::GameAction>(i);
                        events::Key primary = static_cast<events::Key>(event.keyBindings[i * 2]);
                        events::Key secondary = static_cast<events::Key>(event.keyBindings[i * 2 + 1]);
                        config.setKeyBinding(action, primary, secondary);
                    }
                    showInfo("Settings loaded from server");
                }
                // If not found, keep defaults (already set by AccessibilityConfig constructor)
            }
            // Ignore other events (like TCPRoomUpdateEvent) - they'll be handled by LobbyScene
        }, *eventOpt);

        // Break after processing if scene is changing
        if (sceneChanging) {
            break;
        }
    }
}

void MainMenuScene::onSettingsClick()
{
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<SettingsScene>());
    }
}

void MainMenuScene::onQuitClick()
{
    if (_context.window) {
        _context.window->close();
    }
}

void MainMenuScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    if (_dialogMode == DialogMode::CreateRoom) {
        _roomNameInput->handleEvent(event);
        _maxPlayersDecBtn->handleEvent(event);
        _maxPlayersIncBtn->handleEvent(event);
        _publicPrivateBtn->handleEvent(event);
        _createConfirmBtn->handleEvent(event);
        _cancelBtn->handleEvent(event);

        // Enter to confirm, Escape to cancel
        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Enter) {
                onCreateRoomConfirm();
            } else if (keyPressed->key == events::Key::Escape) {
                closeDialog();
            }
        }
    } else if (_dialogMode == DialogMode::JoinRoom) {
        _roomCodeInput->handleEvent(event);
        _joinConfirmBtn->handleEvent(event);
        _cancelBtn->handleEvent(event);

        // Enter to confirm, Escape to cancel
        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Enter) {
                onJoinRoomConfirm();
            } else if (keyPressed->key == events::Key::Escape) {
                closeDialog();
            }
        }
    } else {
        // Main menu mode
        _createRoomButton->handleEvent(event);
        _joinRoomButton->handleEvent(event);
        _browseRoomsButton->handleEvent(event);
        _quickJoinButton->handleEvent(event);
        _settingsButton->handleEvent(event);
        _quitButton->handleEvent(event);
    }
}

void MainMenuScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    // Process TCP events for room responses
    processTCPEvents();

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Update buttons
    _createRoomButton->update(deltaTime);
    _joinRoomButton->update(deltaTime);
    _browseRoomsButton->update(deltaTime);
    _quickJoinButton->update(deltaTime);
    _settingsButton->update(deltaTime);
    _quitButton->update(deltaTime);

    // Update dialog components if visible
    if (_dialogMode == DialogMode::CreateRoom) {
        _roomNameInput->update(deltaTime);
        _maxPlayersDecBtn->update(deltaTime);
        _maxPlayersIncBtn->update(deltaTime);
        _publicPrivateBtn->update(deltaTime);
        _createConfirmBtn->update(deltaTime);
        _cancelBtn->update(deltaTime);
    } else if (_dialogMode == DialogMode::JoinRoom) {
        _roomCodeInput->update(deltaTime);
        _joinConfirmBtn->update(deltaTime);
        _cancelBtn->update(deltaTime);
    }

    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }
}

void MainMenuScene::render()
{
    if (!_context.window) return;

    // Draw space background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {5, 5, 20, 255});

    // Draw stars
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Draw title
    _context.window->drawText(FONT_KEY, "R-TYPE",
        SCREEN_WIDTH / 2 - 150, 150, 80, {100, 150, 255, 255});

    // Draw subtitle
    _context.window->drawText(FONT_KEY, "MULTIPLAYER",
        SCREEN_WIDTH / 2 - 90, 250, 32, {150, 150, 180, 255});

    // Draw main menu buttons
    _createRoomButton->render(*_context.window);
    _joinRoomButton->render(*_context.window);
    _browseRoomsButton->render(*_context.window);
    _quickJoinButton->render(*_context.window);
    _settingsButton->render(*_context.window);
    _quitButton->render(*_context.window);

    // Draw dialog overlay if active
    if (_dialogMode != DialogMode::None) {
        // Semi-transparent overlay
        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

        // Dialog box
        float dialogX = SCREEN_WIDTH / 2 - 250;
        float dialogY = SCREEN_HEIGHT / 2 - 200;
        float dialogWidth = 500;
        float dialogHeight = _dialogMode == DialogMode::CreateRoom ? 350 : 250;

        // Dialog background
        _context.window->drawRect(dialogX, dialogY, dialogWidth, dialogHeight, {25, 25, 50, 250});

        // Dialog border
        _context.window->drawRect(dialogX, dialogY, dialogWidth, 3, {80, 100, 150, 255});
        _context.window->drawRect(dialogX, dialogY + dialogHeight - 3, dialogWidth, 3, {80, 100, 150, 255});
        _context.window->drawRect(dialogX, dialogY, 3, dialogHeight, {80, 100, 150, 255});
        _context.window->drawRect(dialogX + dialogWidth - 3, dialogY, 3, dialogHeight, {80, 100, 150, 255});

        if (_dialogMode == DialogMode::CreateRoom) {
            // Dialog title
            _context.window->drawText(FONT_KEY, "CREATE ROOM",
                SCREEN_WIDTH / 2 - 80, dialogY + 20, 28, {255, 255, 255, 255});

            // Room name label
            _context.window->drawText(FONT_KEY, "Room Name:",
                SCREEN_WIDTH / 2 - 150, dialogY + 65, 18, {180, 180, 200, 255});

            _roomNameInput->render(*_context.window);

            // Max players label and value
            _context.window->drawText(FONT_KEY, "Max Players:",
                SCREEN_WIDTH / 2 - 150, dialogY + 135, 18, {180, 180, 200, 255});
            _context.window->drawText(FONT_KEY, std::to_string(_maxPlayers),
                SCREEN_WIDTH / 2 - 10, dialogY + 135, 24, {255, 255, 255, 255});

            _maxPlayersDecBtn->render(*_context.window);
            _maxPlayersIncBtn->render(*_context.window);

            // Visibility label
            _context.window->drawText(FONT_KEY, "Visibility:",
                SCREEN_WIDTH / 2 - 150, dialogY + 205, 18, {180, 180, 200, 255});

            _publicPrivateBtn->render(*_context.window);

            _createConfirmBtn->render(*_context.window);
            _cancelBtn->setPos(Vec2f{SCREEN_WIDTH / 2 + 20, dialogY + 270});
            _cancelBtn->render(*_context.window);
        } else if (_dialogMode == DialogMode::JoinRoom) {
            // Dialog title
            _context.window->drawText(FONT_KEY, "JOIN ROOM",
                SCREEN_WIDTH / 2 - 65, dialogY + 20, 28, {255, 255, 255, 255});

            // Code label
            _context.window->drawText(FONT_KEY, "Enter 6-character code:",
                SCREEN_WIDTH / 2 - 120, dialogY + 65, 18, {180, 180, 200, 255});

            _roomCodeInput->render(*_context.window);

            _joinConfirmBtn->render(*_context.window);

            // Reposition cancel button for join dialog (aligned with JOIN button)
            _cancelBtn->setPos(Vec2f{SCREEN_WIDTH / 2 + 20, dialogY + 150});
            _cancelBtn->render(*_context.window);
        }
    }

    // Draw status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 80, 20, _statusColor);
    }

    // Draw connection status
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 20, SCREEN_HEIGHT - 40, 14, connColor);

    // Draw version
    _context.window->drawText(FONT_KEY, "v0.2.0",
        SCREEN_WIDTH - 100, SCREEN_HEIGHT - 40, 16, {80, 80, 100, 255});
}
