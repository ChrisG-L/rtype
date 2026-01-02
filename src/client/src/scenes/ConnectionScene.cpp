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
#include <cmath>
#include <variant>

ConnectionScene::ConnectionScene(core::ConnectionSceneMode mode)
    : _mode(mode)
{
    _statusMessage = "Initialisation...";

    // In Reconnection mode, start waiting for TCP to connect
    if (_mode == core::ConnectionSceneMode::Reconnection) {
        _reloginState = ReloginState::WaitingForTCPConnect;
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

    // Attempt TCP reconnection if needed (but not if already connecting)
    if (_context.tcpClient &&
        !_context.tcpClient->isConnected() &&
        !_context.tcpClient->isConnecting()) {
        _connectionState.tcp = core::ConnectionStatus::Connecting;
        // Use stored connection info, fallback to defaults
        std::string host = _context.tcpClient->getLastHost();
        uint16_t port = _context.tcpClient->getLastPort();
        if (host.empty()) host = "127.0.0.1";
        if (port == 0) port = 4125;
        _context.tcpClient->connect(host, port);
        _statusMessage = "Connexion TCP...";
    }

    // Attempt UDP reconnection if needed (but not if already connecting)
    if (_context.udpClient &&
        !_context.udpClient->isConnected() &&
        !_context.udpClient->isConnecting()) {
        _connectionState.udp = core::ConnectionStatus::Connecting;
        // Use stored connection info, fallback to defaults
        std::string host = _context.udpClient->getLastHost();
        uint16_t port = _context.udpClient->getLastPort();
        if (host.empty()) host = "127.0.0.1";
        if (port == 0) port = 4124;
        _context.udpClient->connect(host, port);
        _statusMessage = "Connexion UDP...";
    }
}

bool ConnectionScene::isReconnected() const
{
    return _connectionState.isFullyConnected();
}

void ConnectionScene::handleEvent(const events::Event& event)
{
    // Escape key could show a "Cancel" option or quit
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            // In initial connection mode, quit the application
            if (_mode == core::ConnectionSceneMode::InitialConnection && _context.window) {
                _context.window->close();
            }
            // In reconnection mode, we could show a confirmation dialog
        }
    }
}

void ConnectionScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();

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

    // Retry timer
    _retryTimer += deltaTime;
    if (_retryTimer >= RETRY_INTERVAL) {
        _retryTimer = 0.0f;
        attemptReconnection();
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
            tcpColor = {255, 200, 100, 255};
            tcpStatus = "TCP: Connexion...";
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
            udpColor = {255, 200, 100, 255};
            udpStatus = "UDP: Connexion...";
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
        _context.window->drawText(FONT_KEY, "Appuyez sur Echap pour quitter",
            SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 50, 16, {100, 100, 120, 255});
    }
}
