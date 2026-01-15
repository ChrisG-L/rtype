/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ConnectionScene
*/

#include "scenes/ConnectionScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/LoginScene.hpp"
#include "scenes/MainMenuScene.hpp"
#include "config/ServerConfigManager.hpp"
#include <cmath>
#include <variant>

ConnectionScene::ConnectionScene(core::ConnectionSceneMode mode, bool forceReconnect)
    : _mode(mode), _forceReconnect(forceReconnect)
{
    _statusMessage = "Initialisation...";

    // In Reconnection mode, start waiting for TCP to connect
    if (_mode == core::ConnectionSceneMode::Reconnection) {
        _reloginState = ReloginState::WaitingForTCPConnect;
    }

    // If force reconnect, we need to disconnect first
    if (_forceReconnect) {
        _disconnectPending = true;
        _statusMessage = "Changement de serveur...";
    }
}

void ConnectionScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

std::string ConnectionScene::getModeTitle() const
{
    switch (_mode) {
        case core::ConnectionSceneMode::InitialConnection:
            return "EN ATTENTE DE CONNEXION";
        case core::ConnectionSceneMode::Reconnection:
            return "RECONNEXION EN COURS";
    }
    return "CONNEXION";
}

void ConnectionScene::updateConnectionState()
{
    // Update TCP status
    if (_context.tcpClient) {
        if (_context.tcpClient->isConnected()) {
            _connectionState.tcp = core::ConnectionStatus::Connected;
        } else if (_context.tcpClient->isConnecting()) {
            _connectionState.tcp = core::ConnectionStatus::Connecting;
        } else {
            _connectionState.tcp = core::ConnectionStatus::Disconnected;
        }
    }

    // Update UDP status
    if (_context.udpClient) {
        if (_context.udpClient->isConnected()) {
            _connectionState.udp = core::ConnectionStatus::Connected;
        } else if (_context.udpClient->isConnecting()) {
            _connectionState.udp = core::ConnectionStatus::Connecting;
        } else {
            _connectionState.udp = core::ConnectionStatus::Disconnected;
        }
    }
}

void ConnectionScene::processNetworkEvents()
{
    // Process TCP events
    if (_context.tcpClient) {
        while (auto eventOpt = _context.tcpClient->pollEvent()) {
            std::visit([this](auto&& event) {
                using T = std::decay_t<decltype(event)>;

                if constexpr (std::is_same_v<T, client::network::TCPConnectedEvent>) {
                    _connectionState.tcp = core::ConnectionStatus::Connected;
                    _statusMessage = "TCP connecte";

                    // In Reconnection mode, attempt auto-relogin after TCP connects
                    if (_mode == core::ConnectionSceneMode::Reconnection &&
                        _reloginState == ReloginState::WaitingForTCPConnect) {
                        attemptAutoRelogin();
                    }
                }
                else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                    _connectionState.tcp = core::ConnectionStatus::Disconnected;
                    // Reset relogin state if disconnected during auth
                    if (_reloginState == ReloginState::WaitingForAuthResponse) {
                        _reloginState = ReloginState::WaitingForTCPConnect;
                    }
                }
                else if constexpr (std::is_same_v<T, client::network::TCPAuthSuccessEvent>) {
                    // Auto-relogin successful
                    _reloginState = ReloginState::Authenticated;
                    _statusMessage = "Authentification reussie";
                }
                else if constexpr (std::is_same_v<T, client::network::TCPAuthFailedEvent>) {
                    // Auto-relogin failed (e.g., password changed)
                    _reloginState = ReloginState::Failed;
                    _statusMessage = "Echec auth: " + event.message;
                }
                else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                    _statusMessage = "TCP: " + event.message;
                }
            }, *eventOpt);
        }
    }

    // Process UDP events
    if (_context.udpClient) {
        while (auto eventOpt = _context.udpClient->pollEvent()) {
            std::visit([this](auto&& event) {
                using T = std::decay_t<decltype(event)>;

                if constexpr (std::is_same_v<T, client::network::UDPConnectedEvent>) {
                    _connectionState.udp = core::ConnectionStatus::Connected;
                    _statusMessage = "UDP connecte";
                }
                else if constexpr (std::is_same_v<T, client::network::UDPDisconnectedEvent>) {
                    _connectionState.udp = core::ConnectionStatus::Disconnected;
                }
                else if constexpr (std::is_same_v<T, client::network::UDPErrorEvent>) {
                    _statusMessage = "UDP: " + event.message;
                }
            }, *eventOpt);
        }
    }
}

void ConnectionScene::attemptAutoRelogin()
{
    if (!_context.tcpClient) return;

    // Check if we have stored credentials
    if (_context.tcpClient->hasStoredCredentials()) {
        auto [username, password] = _context.tcpClient->getStoredCredentials();
        _statusMessage = "Re-authentification...";
        _reloginState = ReloginState::WaitingForAuthResponse;
        _context.tcpClient->sendLoginData(username, password);
    } else {
        // No stored credentials - mark as failed to redirect to LoginScene
        _reloginState = ReloginState::Failed;
        _statusMessage = "Pas de credentials stockes";
    }
}

void ConnectionScene::attemptReconnection()
{
    _retryCount++;

    // After several failed attempts, show server config UI
    if (_retryCount >= MAX_RETRIES_BEFORE_CONFIG && !_showingConfigUI) {
        showServerConfigUI();
        return;
    }

    // Get server config
    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Attempt TCP reconnection if needed (but not if already connecting)
    if (_context.tcpClient &&
        !_context.tcpClient->isConnected() &&
        !_context.tcpClient->isConnecting()) {
        _connectionState.tcp = core::ConnectionStatus::Connecting;
        // Use server config
        std::string host = serverConfig.getHost();
        uint16_t port = serverConfig.getTcpPort();
        _context.tcpClient->connect(host, port);
        _statusMessage = "Connexion TCP " + host + ":" + std::to_string(port) + "...";
    }

    // Attempt UDP reconnection if needed (but not if already connecting)
    if (_context.udpClient &&
        !_context.udpClient->isConnected() &&
        !_context.udpClient->isConnecting()) {
        _connectionState.udp = core::ConnectionStatus::Connecting;
        // Use server config
        std::string host = serverConfig.getHost();
        uint16_t port = serverConfig.getUdpPort();
        _context.udpClient->connect(host, port);
        _statusMessage = "Connexion UDP " + host + ":" + std::to_string(port) + "...";
    }
}

bool ConnectionScene::isReconnected() const
{
    return _connectionState.isFullyConnected();
}

void ConnectionScene::handleEvent(const events::Event& event)
{
    // Handle server config UI events first
    if (_showingConfigUI && _context.window) {
        if (_hostInput) _hostInput->handleEvent(event);
        if (_tcpPortInput) _tcpPortInput->handleEvent(event);
        if (_udpPortInput) _udpPortInput->handleEvent(event);
        if (_franceButton) _franceButton->handleEvent(event);
        if (_localButton) _localButton->handleEvent(event);
        if (_connectButton) _connectButton->handleEvent(event);
        if (_cancelButton) _cancelButton->handleEvent(event);

        // Handle clicking outside inputs to unfocus
        if (auto* mousePressed = std::get_if<events::MouseButtonPressed>(&event)) {
            float mx = static_cast<float>(mousePressed->x);
            float my = static_cast<float>(mousePressed->y);

            // Check if click is inside any input
            bool inHost = _hostInput && mx >= _hostInput->getPos().x &&
                         mx <= _hostInput->getPos().x + _hostInput->getSize().x &&
                         my >= _hostInput->getPos().y &&
                         my <= _hostInput->getPos().y + _hostInput->getSize().y;
            bool inTcp = _tcpPortInput && mx >= _tcpPortInput->getPos().x &&
                        mx <= _tcpPortInput->getPos().x + _tcpPortInput->getSize().x &&
                        my >= _tcpPortInput->getPos().y &&
                        my <= _tcpPortInput->getPos().y + _tcpPortInput->getSize().y;
            bool inUdp = _udpPortInput && mx >= _udpPortInput->getPos().x &&
                        mx <= _udpPortInput->getPos().x + _udpPortInput->getSize().x &&
                        my >= _udpPortInput->getPos().y &&
                        my <= _udpPortInput->getPos().y + _udpPortInput->getSize().y;

            if (_hostInput) _hostInput->setFocused(inHost);
            if (_tcpPortInput) _tcpPortInput->setFocused(inTcp);
            if (_udpPortInput) _udpPortInput->setFocused(inUdp);
        }
        return;
    }

    // Escape key could show a "Cancel" option or quit
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            // In initial connection mode, quit the application
            if (_mode == core::ConnectionSceneMode::InitialConnection && _context.window) {
                _context.window->close();
            }
            // In reconnection mode, we could show a confirmation dialog
        }
        // 'S' key to show server config
        if (keyPressed->key == events::Key::S && !_showingConfigUI) {
            showServerConfigUI();
        }
    }
}

void ConnectionScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();

    // Handle pending disconnect for server change
    if (_disconnectPending) {
        _statusMessage = "Deconnexion...";
        if (_context.tcpClient) {
            _context.tcpClient->disconnect();
        }
        if (_context.udpClient) {
            _context.udpClient->disconnect();
        }
        _disconnectPending = false;
        _forceReconnect = false;
        _retryTimer = RETRY_INTERVAL;  // Trigger immediate reconnection
        _connectionState.tcp = core::ConnectionStatus::Disconnected;
        _connectionState.udp = core::ConnectionStatus::Disconnected;
        return;  // Skip this frame, reconnect on next
    }

    // Update starfield animation
    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Update spinner animation
    _spinnerAngle += deltaTime * 360.0f;  // One rotation per second
    if (_spinnerAngle >= 360.0f) {
        _spinnerAngle -= 360.0f;
    }

    // Update dot animation
    _animationTimer += deltaTime;
    if (_animationTimer >= DOT_ANIMATION_SPEED) {
        _animationTimer = 0.0f;
        _dotCount = (_dotCount + 1) % 4;
    }

    // Update server config UI if visible
    if (_showingConfigUI) {
        if (_hostInput) _hostInput->update(deltaTime);
        if (_tcpPortInput) _tcpPortInput->update(deltaTime);
        if (_udpPortInput) _udpPortInput->update(deltaTime);
        if (_franceButton) _franceButton->update(deltaTime);
        if (_localButton) _localButton->update(deltaTime);
        if (_connectButton) _connectButton->update(deltaTime);
        if (_cancelButton) _cancelButton->update(deltaTime);
    }

    // Process network events
    processNetworkEvents();

    // Update connection state
    updateConnectionState();

    // Handle relogin failure - redirect to LoginScene
    if (_mode == core::ConnectionSceneMode::Reconnection &&
        _reloginState == ReloginState::Failed) {
        if (_sceneManager) {
            // Clear stored credentials since they're invalid
            if (_context.tcpClient) {
                _context.tcpClient->clearStoredCredentials();
            }
            // Replace the entire scene stack with LoginScene
            _sceneManager->changeScene(std::make_unique<LoginScene>());
        }
        return;
    }

    // Check if fully connected
    if (_connectionState.isFullyConnected()) {
        // Save server config on successful connection
        config::ServerConfigManager::getInstance().save();

        if (_mode == core::ConnectionSceneMode::InitialConnection) {
            // Go to LoginScene
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<LoginScene>());
            }
        } else {
            // Reconnection mode - need to also be authenticated
            if (_reloginState == ReloginState::Authenticated) {
                // Pop this overlay and return to previous scene
                if (_sceneManager) {
                    _sceneManager->popScene();
                }
            }
            // If not authenticated yet, keep waiting
        }
        return;
    }

    // Retry timer (only if not showing config UI)
    if (!_showingConfigUI) {
        _retryTimer += deltaTime;
        if (_retryTimer >= RETRY_INTERVAL) {
            _retryTimer = 0.0f;
            attemptReconnection();
        }
    }
}

void ConnectionScene::render()
{
    if (!_context.window) return;

    // Dark overlay background (semi-transparent if overlay mode)
    if (_mode == core::ConnectionSceneMode::Reconnection) {
        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 200});
    } else {
        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {5, 5, 20, 255});
    }

    // Draw stars (only in initial connection mode)
    if (_mode == core::ConnectionSceneMode::InitialConnection && _starfield) {
        _starfield->render(*_context.window);
    }

    // Draw title
    std::string title = getModeTitle();
    float titleX = SCREEN_WIDTH / 2 - 250;
    float titleY = SCREEN_HEIGHT / 2 - 150;
    _context.window->drawText(FONT_KEY, title, titleX, titleY, 48, {100, 150, 255, 255});

    // Draw animated dots
    std::string dots(static_cast<size_t>(_dotCount), '.');
    _context.window->drawText(FONT_KEY, dots,
        SCREEN_WIDTH / 2 + 200, titleY, 48, {100, 150, 255, 255});

    // Draw connection status box
    float boxX = SCREEN_WIDTH / 2 - 200;
    float boxY = SCREEN_HEIGHT / 2 - 50;
    float boxWidth = 400;
    float boxHeight = 150;

    _context.window->drawRect(boxX, boxY, boxWidth, boxHeight, {20, 20, 40, 230});
    _context.window->drawRect(boxX, boxY, boxWidth, 2, {60, 60, 100, 255});
    _context.window->drawRect(boxX, boxY + boxHeight - 2, boxWidth, 2, {60, 60, 100, 255});
    _context.window->drawRect(boxX, boxY, 2, boxHeight, {60, 60, 100, 255});
    _context.window->drawRect(boxX + boxWidth - 2, boxY, 2, boxHeight, {60, 60, 100, 255});

    // Draw TCP status
    rgba tcpColor;
    std::string tcpStatus;
    switch (_connectionState.tcp) {
        case core::ConnectionStatus::Connected:
            tcpColor = {100, 255, 100, 255};
            tcpStatus = "TCP: Connecte";
            break;
        case core::ConnectionStatus::Connecting:
            tcpColor = {255, 100, 100, 255};  // Same as Disconnected
            tcpStatus = "TCP: Deconnecte";
            break;
        case core::ConnectionStatus::Disconnected:
            tcpColor = {255, 100, 100, 255};
            tcpStatus = "TCP: Deconnecte";
            break;
    }
    _context.window->drawText(FONT_KEY, tcpStatus, boxX + 30, boxY + 30, 24, tcpColor);

    // Draw UDP status
    rgba udpColor;
    std::string udpStatus;
    switch (_connectionState.udp) {
        case core::ConnectionStatus::Connected:
            udpColor = {100, 255, 100, 255};
            udpStatus = "UDP: Connecte";
            break;
        case core::ConnectionStatus::Connecting:
            udpColor = {255, 100, 100, 255};  // Same as Disconnected
            udpStatus = "UDP: Deconnecte";
            break;
        case core::ConnectionStatus::Disconnected:
            udpColor = {255, 100, 100, 255};
            udpStatus = "UDP: Deconnecte";
            break;
    }
    _context.window->drawText(FONT_KEY, udpStatus, boxX + 30, boxY + 70, 24, udpColor);

    // Draw retry count
    std::string retryText = "Tentative #" + std::to_string(_retryCount);
    _context.window->drawText(FONT_KEY, retryText, boxX + 30, boxY + 110, 18, {150, 150, 180, 255});

    // Draw status message
    _context.window->drawText(FONT_KEY, _statusMessage,
        SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 + 130, 18, {180, 180, 200, 255});

    // Draw spinner (simple rotating indicator using rectangles)
    float spinnerCenterX = SCREEN_WIDTH / 2;
    float spinnerCenterY = SCREEN_HEIGHT / 2 + 200;
    float spinnerRadius = 30.0f;

    for (int i = 0; i < 8; ++i) {
        float angle = (_spinnerAngle + i * 45.0f) * 3.14159f / 180.0f;
        float x = spinnerCenterX + std::cos(angle) * spinnerRadius - 4;
        float y = spinnerCenterY + std::sin(angle) * spinnerRadius - 4;
        uint8_t alpha = static_cast<uint8_t>(255 - i * 25);
        _context.window->drawRect(x, y, 8, 8, {100, 150, 255, alpha});
    }

    // Draw hint
    if (_mode == core::ConnectionSceneMode::InitialConnection) {
        std::string hint = _showingConfigUI ?
            "Configurez l'adresse du serveur" :
            "Appuyez sur S pour configurer le serveur, Echap pour quitter";
        _context.window->drawText(FONT_KEY, hint,
            SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT - 50, 16, {100, 100, 120, 255});
    }

    // Draw server config UI if visible
    if (_showingConfigUI) {
        renderServerConfigUI();
    }
}

void ConnectionScene::initServerConfigUI()
{
    if (_configUIInitialized) return;

    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Panel dimensions (must match renderServerConfigUI)
    float panelWidth = 450.0f;
    float panelX = SCREEN_WIDTH / 2 - panelWidth / 2;
    float panelY = SCREEN_HEIGHT / 2 - 180.0f;

    // Layout inside panel
    float contentX = panelX + 30.0f;
    float inputWidth = panelWidth - 60.0f;  // 30px margin on each side
    float inputHeight = 40.0f;
    float labelHeight = 25.0f;
    float inputStartY = panelY + 70.0f;

    // Host input (with label above)
    _hostInput = std::make_unique<ui::TextInput>(
        Vec2f{contentX, inputStartY + labelHeight},
        Vec2f{inputWidth, inputHeight},
        "Adresse serveur",
        FONT_KEY
    );
    _hostInput->setText(serverConfig.getHost());
    _hostInput->setMaxLength(63);

    // Port inputs row (below host)
    float portsY = inputStartY + labelHeight + inputHeight + 40.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;  // 20px gap between ports

    // TCP Port input
    _tcpPortInput = std::make_unique<ui::TextInput>(
        Vec2f{contentX, portsY + labelHeight},
        Vec2f{portInputWidth, inputHeight},
        "Port TCP",
        FONT_KEY
    );
    _tcpPortInput->setText(std::to_string(serverConfig.getTcpPort()));
    _tcpPortInput->setMaxLength(5);

    // UDP Port input
    _udpPortInput = std::make_unique<ui::TextInput>(
        Vec2f{contentX + portInputWidth + 20.0f, portsY + labelHeight},
        Vec2f{portInputWidth, inputHeight},
        "Port UDP",
        FONT_KEY
    );
    _udpPortInput->setText(std::to_string(serverConfig.getUdpPort()));
    _udpPortInput->setMaxLength(5);

    // Quick connect buttons row (FRANCE / LOCAL)
    float quickBtnY = portsY + labelHeight + inputHeight + 25.0f;
    float quickBtnWidth = (inputWidth - 20.0f) / 2.0f;

    // FRANCE button
    _franceButton = std::make_unique<ui::Button>(
        Vec2f{contentX, quickBtnY},
        Vec2f{quickBtnWidth, 40},
        "FRANCE",
        FONT_KEY
    );
    _franceButton->setOnClick([this]() {
        config::ServerConfigManager::getInstance().applyFrancePreset();
        hideServerConfigUI();
        attemptReconnection();
    });
    _franceButton->setNormalColor({50, 80, 120, 255});
    _franceButton->setHoveredColor({70, 100, 150, 255});

    // LOCAL button
    _localButton = std::make_unique<ui::Button>(
        Vec2f{contentX + quickBtnWidth + 20.0f, quickBtnY},
        Vec2f{quickBtnWidth, 40},
        "LOCAL",
        FONT_KEY
    );
    _localButton->setOnClick([this]() {
        config::ServerConfigManager::getInstance().applyLocalPreset();
        hideServerConfigUI();
        attemptReconnection();
    });
    _localButton->setNormalColor({60, 100, 60, 255});
    _localButton->setHoveredColor({80, 130, 80, 255});

    // Bottom buttons row (CONNECTER / ANNULER)
    float buttonsY = quickBtnY + 55.0f;
    float buttonWidth = 150.0f;
    float buttonGap = 20.0f;
    float buttonsStartX = panelX + (panelWidth - buttonWidth * 2 - buttonGap) / 2.0f;

    // Connect button
    _connectButton = std::make_unique<ui::Button>(
        Vec2f{buttonsStartX, buttonsY},
        Vec2f{buttonWidth, 45},
        "CONNECTER",
        FONT_KEY
    );
    _connectButton->setOnClick([this]() {
        applyServerConfig();
    });

    // Cancel button
    _cancelButton = std::make_unique<ui::Button>(
        Vec2f{buttonsStartX + buttonWidth + buttonGap, buttonsY},
        Vec2f{buttonWidth, 45},
        "ANNULER",
        FONT_KEY
    );
    _cancelButton->setOnClick([this]() {
        hideServerConfigUI();
    });
    _cancelButton->setNormalColor({80, 60, 60, 255});
    _cancelButton->setHoveredColor({100, 70, 70, 255});

    _configUIInitialized = true;
}

void ConnectionScene::showServerConfigUI()
{
    initServerConfigUI();
    _showingConfigUI = true;
    _statusMessage = "Configuration du serveur";

    // Reset values from config
    auto& serverConfig = config::ServerConfigManager::getInstance();
    if (_hostInput) _hostInput->setText(serverConfig.getHost());
    if (_tcpPortInput) _tcpPortInput->setText(std::to_string(serverConfig.getTcpPort()));
    if (_udpPortInput) _udpPortInput->setText(std::to_string(serverConfig.getUdpPort()));
}

void ConnectionScene::hideServerConfigUI()
{
    _showingConfigUI = false;
    _retryCount = 0;  // Reset retry count
    _retryTimer = 0.0f;
}

void ConnectionScene::applyServerConfig()
{
    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Get values from inputs
    std::string host = _hostInput ? _hostInput->getText() : "127.0.0.1";
    std::string tcpPortStr = _tcpPortInput ? _tcpPortInput->getText() : "4125";
    std::string udpPortStr = _udpPortInput ? _udpPortInput->getText() : "4124";

    // Validate and apply
    if (!host.empty()) {
        serverConfig.setHost(host);
    }

    try {
        int tcpPort = std::stoi(tcpPortStr);
        if (tcpPort > 0 && tcpPort <= 65535) {
            serverConfig.setTcpPort(static_cast<uint16_t>(tcpPort));
        }
    } catch (...) {}

    try {
        int udpPort = std::stoi(udpPortStr);
        if (udpPort > 0 && udpPort <= 65535) {
            serverConfig.setUdpPort(static_cast<uint16_t>(udpPort));
        }
    } catch (...) {}

    // Save config to file
    serverConfig.save();

    // Hide UI and retry connection
    hideServerConfigUI();
    _statusMessage = "Connexion a " + serverConfig.getHost() + "...";

    // Force immediate reconnection attempt
    attemptReconnection();
}

void ConnectionScene::renderServerConfigUI()
{
    if (!_context.window) return;

    // Semi-transparent overlay
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    // Config panel background (same dimensions as initServerConfigUI)
    float panelWidth = 450.0f;
    float panelHeight = 420.0f;
    float panelX = SCREEN_WIDTH / 2 - panelWidth / 2;
    float panelY = SCREEN_HEIGHT / 2 - 180.0f;

    _context.window->drawRect(panelX, panelY, panelWidth, panelHeight, {25, 25, 45, 250});

    // Panel border
    _context.window->drawRect(panelX, panelY, panelWidth, 3, {80, 100, 180, 255});
    _context.window->drawRect(panelX, panelY + panelHeight - 3, panelWidth, 3, {80, 100, 180, 255});
    _context.window->drawRect(panelX, panelY, 3, panelHeight, {80, 100, 180, 255});
    _context.window->drawRect(panelX + panelWidth - 3, panelY, 3, panelHeight, {80, 100, 180, 255});

    // Title
    _context.window->drawText(FONT_KEY, "CONFIGURATION SERVEUR",
        SCREEN_WIDTH / 2 - 140, panelY + 20, 28, {100, 150, 255, 255});

    // Layout variables (must match initServerConfigUI)
    float contentX = panelX + 30.0f;
    float inputWidth = panelWidth - 60.0f;
    float inputHeight = 40.0f;
    float labelHeight = 25.0f;
    float inputStartY = panelY + 70.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;

    // Host label (above host input)
    _context.window->drawText(FONT_KEY, "Adresse du serveur:",
        contentX, inputStartY, 18, {180, 180, 200, 255});

    // Render host input
    if (_hostInput) _hostInput->render(*_context.window);

    // Ports row
    float portsY = inputStartY + labelHeight + inputHeight + 40.0f;

    // TCP port label (above TCP input)
    _context.window->drawText(FONT_KEY, "Port TCP:",
        contentX, portsY, 18, {180, 180, 200, 255});

    // UDP port label (above UDP input)
    _context.window->drawText(FONT_KEY, "Port UDP:",
        contentX + portInputWidth + 20.0f, portsY, 18, {180, 180, 200, 255});

    // Render port inputs
    if (_tcpPortInput) _tcpPortInput->render(*_context.window);
    if (_udpPortInput) _udpPortInput->render(*_context.window);

    // Quick connect label
    float quickBtnY = portsY + labelHeight + inputHeight + 25.0f;
    _context.window->drawText(FONT_KEY, "Connexion rapide:",
        contentX, quickBtnY - 22.0f, 16, {140, 140, 160, 255});

    // Render quick connect buttons
    if (_franceButton) _franceButton->render(*_context.window);
    if (_localButton) _localButton->render(*_context.window);

    // Render bottom buttons
    if (_connectButton) _connectButton->render(*_context.window);
    if (_cancelButton) _cancelButton->render(*_context.window);

    // Manual input hint at bottom
    _context.window->drawText(FONT_KEY, "Ou saisir une adresse personnalisee ci-dessus",
        SCREEN_WIDTH / 2 - 160, panelY + panelHeight - 30, 13, {100, 100, 120, 255});
}
