/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RoomBrowserScene implementation
*/

#include "scenes/RoomBrowserScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/LobbyScene.hpp"
#include <variant>

RoomBrowserScene::RoomBrowserScene()
{
}

void RoomBrowserScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");

    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

void RoomBrowserScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float buttonWidth = 180.0f;
    float buttonHeight = 50.0f;
    float bottomY = SCREEN_HEIGHT - 100.0f;
    float spacing = 20.0f;
    float startX = SCREEN_WIDTH / 2 - (buttonWidth * 4 + spacing * 3) / 2;

    // Refresh button
    _refreshButton = std::make_unique<ui::Button>(
        Vec2f{startX, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "REFRESH",
        FONT_KEY
    );
    _refreshButton->setOnClick([this]() { onRefreshClick(); });
    _refreshButton->setNormalColor({60, 100, 150, 255});
    _refreshButton->setHoveredColor({80, 130, 180, 255});

    // Join button
    _joinButton = std::make_unique<ui::Button>(
        Vec2f{startX + buttonWidth + spacing, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "JOIN",
        FONT_KEY
    );
    _joinButton->setOnClick([this]() {
        if (_selectedRoomIndex >= 0 && _selectedRoomIndex < static_cast<int>(_rooms.size())) {
            onJoinRoomClick(_rooms[_selectedRoomIndex].code);
        }
    });
    _joinButton->setNormalColor({60, 120, 60, 255});
    _joinButton->setHoveredColor({80, 150, 80, 255});

    // Quick Join button
    _quickJoinButton = std::make_unique<ui::Button>(
        Vec2f{startX + (buttonWidth + spacing) * 2, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "QUICK JOIN",
        FONT_KEY
    );
    _quickJoinButton->setOnClick([this]() { onQuickJoinClick(); });
    _quickJoinButton->setNormalColor({150, 100, 60, 255});
    _quickJoinButton->setHoveredColor({180, 130, 80, 255});

    // Back button
    _backButton = std::make_unique<ui::Button>(
        Vec2f{startX + (buttonWidth + spacing) * 3, bottomY},
        Vec2f{buttonWidth, buttonHeight},
        "BACK",
        FONT_KEY
    );
    _backButton->setOnClick([this]() { onBackClick(); });
    _backButton->setNormalColor({120, 60, 60, 255});
    _backButton->setHoveredColor({150, 80, 80, 255});

    _uiInitialized = true;

    // Auto-refresh on first load
    onRefreshClick();
}

void RoomBrowserScene::onRefreshClick()
{
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _isLoading = true;
    _context.tcpClient->browsePublicRooms();
    showInfo("Refreshing...");
}

void RoomBrowserScene::onJoinRoomClick(const std::string& code)
{
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->joinRoomByCode(code);
    showInfo("Joining room...");
}

void RoomBrowserScene::onQuickJoinClick()
{
    if (!_context.tcpClient || !_context.tcpClient->isConnected()) {
        showError("Not connected to server");
        return;
    }

    _context.tcpClient->quickJoin();
    showInfo("Searching for a game...");
}

void RoomBrowserScene::onBackClick()
{
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void RoomBrowserScene::showError(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void RoomBrowserScene::showInfo(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void RoomBrowserScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    bool sceneChanged = false;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this, &sceneChanged](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPRoomListEvent>) {
                _rooms = event.rooms;
                _selectedRoomIndex = _rooms.empty() ? -1 : 0;
                _isLoading = false;
                if (_rooms.empty()) {
                    showInfo("No public rooms available");
                } else {
                    showInfo(std::to_string(_rooms.size()) + " room(s) found");
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomJoinedEvent>) {
                // Successfully joined a room - transition to LobbyScene
                if (_sceneManager) {
                    // Clear event queue before entering lobby to avoid stale events from previous games
                    _context.tcpClient->clearEventQueue();

                    _sceneManager->changeScene(std::make_unique<LobbyScene>(
                        event.roomName, event.roomCode, event.maxPlayers,
                        event.isHost, event.slotId, event.players
                    ));
                    sceneChanged = true;  // Stop processing, let new scene handle remaining events
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPRoomJoinFailedEvent>) {
                showError("Join failed: " + event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPQuickJoinFailedEvent>) {
                showError("Quick join failed: " + event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from server");
            }
            else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
        }, *eventOpt);

        if (sceneChanged) break;  // Let new scene handle remaining events (like ChatHistory)
    }
}

void RoomBrowserScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    // Track mouse position
    if (auto* mouseMoved = std::get_if<events::MouseMoved>(&event)) {
        _mouseX = static_cast<float>(mouseMoved->x);
        _mouseY = static_cast<float>(mouseMoved->y);
    }

    // Handle room selection with mouse click
    if (auto* mousePressed = std::get_if<events::MouseButtonPressed>(&event)) {
        if (mousePressed->button == events::MouseButton::Left) {
            _mouseX = static_cast<float>(mousePressed->x);
            _mouseY = static_cast<float>(mousePressed->y);

            // Check if clicking on room list
            float boxX = SCREEN_WIDTH / 2 - 400;
            float boxY = 180;
            float boxWidth = 800;
            float listStartY = boxY + 80;

            if (_mouseX >= boxX && _mouseX <= boxX + boxWidth) {
                for (size_t i = 0; i < _rooms.size() && i < MAX_VISIBLE_ROOMS; ++i) {
                    float rowY = listStartY + i * ROOM_ROW_HEIGHT;
                    if (_mouseY >= rowY && _mouseY < rowY + ROOM_ROW_HEIGHT) {
                        _selectedRoomIndex = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
    }

    // Handle keyboard navigation
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            onBackClick();
        } else if (keyPressed->key == events::Key::Up && !_rooms.empty()) {
            _selectedRoomIndex = std::max(0, _selectedRoomIndex - 1);
        } else if (keyPressed->key == events::Key::Down && !_rooms.empty()) {
            _selectedRoomIndex = std::min(static_cast<int>(_rooms.size()) - 1, _selectedRoomIndex + 1);
        } else if (keyPressed->key == events::Key::Enter && _selectedRoomIndex >= 0) {
            onJoinRoomClick(_rooms[_selectedRoomIndex].code);
        }
    }

    _refreshButton->handleEvent(event);
    _joinButton->handleEvent(event);
    _quickJoinButton->handleEvent(event);
    _backButton->handleEvent(event);
}

void RoomBrowserScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    processTCPEvents();

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    _refreshButton->update(deltaTime);
    _joinButton->update(deltaTime);
    _quickJoinButton->update(deltaTime);
    _backButton->update(deltaTime);

    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }
}

void RoomBrowserScene::render()
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
        SCREEN_WIDTH / 2 - 150, 30, 80, {100, 150, 255, 255});

    _context.window->drawText(FONT_KEY, "PUBLIC ROOMS",
        SCREEN_WIDTH / 2 - 120, 120, 40, {200, 200, 220, 255});

    // Draw room list box
    float boxX = SCREEN_WIDTH / 2 - 400;
    float boxY = 180;
    float boxWidth = 800;
    float boxHeight = 580;

    // Box background
    _context.window->drawRect(boxX, boxY, boxWidth, boxHeight, {20, 20, 45, 230});

    // Box border
    _context.window->drawRect(boxX, boxY, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY + boxHeight - 3, boxWidth, 3, {60, 80, 120, 255});
    _context.window->drawRect(boxX, boxY, 3, boxHeight, {60, 80, 120, 255});
    _context.window->drawRect(boxX + boxWidth - 3, boxY, 3, boxHeight, {60, 80, 120, 255});

    // Column headers
    float headerY = boxY + 20;
    _context.window->drawText(FONT_KEY, "ROOM NAME",
        boxX + 40, headerY, 20, {150, 150, 180, 255});
    _context.window->drawText(FONT_KEY, "CODE",
        boxX + 400, headerY, 20, {150, 150, 180, 255});
    _context.window->drawText(FONT_KEY, "PLAYERS",
        boxX + 550, headerY, 20, {150, 150, 180, 255});

    // Separator line
    _context.window->drawRect(boxX + 20, headerY + 35, boxWidth - 40, 2, {60, 60, 90, 255});

    // Room list
    float listStartY = boxY + 80;

    if (_rooms.empty()) {
        _context.window->drawText(FONT_KEY, _isLoading ? "Loading..." : "No rooms available",
            SCREEN_WIDTH / 2 - 100, listStartY + 100, 24, {150, 150, 170, 255});
    } else {
        for (size_t i = 0; i < _rooms.size() && i < MAX_VISIBLE_ROOMS; ++i) {
            const auto& room = _rooms[i];
            float rowY = listStartY + i * ROOM_ROW_HEIGHT;

            // Selection highlight
            if (static_cast<int>(i) == _selectedRoomIndex) {
                _context.window->drawRect(boxX + 10, rowY - 5, boxWidth - 20, ROOM_ROW_HEIGHT, {40, 60, 100, 255});
            }

            // Room name
            _context.window->drawText(FONT_KEY, room.name,
                boxX + 40, rowY + 5, 22, {255, 255, 255, 255});

            // Room code
            _context.window->drawText(FONT_KEY, room.code,
                boxX + 400, rowY + 5, 20, {100, 200, 255, 255});

            // Player count
            std::string playerText = std::to_string(room.currentPlayers) + "/" + std::to_string(room.maxPlayers);
            rgba playerColor = room.currentPlayers < room.maxPlayers
                ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
            _context.window->drawText(FONT_KEY, playerText,
                boxX + 580, rowY + 5, 20, playerColor);
        }

        // Scroll hint if more rooms
        if (_rooms.size() > MAX_VISIBLE_ROOMS) {
            _context.window->drawText(FONT_KEY, "... and more",
                SCREEN_WIDTH / 2 - 50, listStartY + MAX_VISIBLE_ROOMS * ROOM_ROW_HEIGHT + 10, 16, {120, 120, 140, 255});
        }
    }

    // Draw buttons
    _refreshButton->render(*_context.window);
    _joinButton->render(*_context.window);
    _quickJoinButton->render(*_context.window);
    _backButton->render(*_context.window);

    // Draw status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 50, 18, _statusColor);
    }

    // Draw connection status
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 20, SCREEN_HEIGHT - 40, 14, connColor);
}
