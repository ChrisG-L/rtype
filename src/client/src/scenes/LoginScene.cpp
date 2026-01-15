/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LoginScene
*/

#include "scenes/LoginScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/ConnectionScene.hpp"
#include "network/NetworkEvents.hpp"
#include "config/ServerConfigManager.hpp"
#include <variant>

LoginScene::LoginScene()
{
}

LoginScene::~LoginScene()
{
    // Note: No need to unregister callbacks since we use the event queue now
    // Boot.cpp callbacks are for logging only and are safe to keep
}

void LoginScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _context.window->loadTexture(BG_TEXTURE_KEY, "assets/login/loginMenuBg.jpg");

    // Initialize starfield
    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

void LoginScene::setupTCPCallbacks()
{
    // Note: We no longer use callbacks for state changes.
    // All events are now polled from the event queue in processTCPEvents().
    // The callbacks in Boot.cpp are still used for logging.
    _callbacksSetup = true;
}

void LoginScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPConnectedEvent>) {
                showSuccess("Connected to auth server");
            }
            else if constexpr (std::is_same_v<T, client::network::TCPDisconnectedEvent>) {
                showError("Disconnected from auth server");
            }
            else if constexpr (std::is_same_v<T, client::network::TCPAuthSuccessEvent>) {
                // Auth successful - request user settings from server
                if (_context.tcpClient) {
                    _context.tcpClient->requestUserSettings();
                }
                // Go to main menu (settings will be received there)
                if (_sceneManager) {
                    _sceneManager->changeScene(std::make_unique<MainMenuScene>());
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPAuthFailedEvent>) {
                showError(event.message);
            }
            else if constexpr (std::is_same_v<T, client::network::TCPErrorEvent>) {
                showError(event.message);
            }
        }, *eventOpt);
    }
}

void LoginScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float inputWidth = 350.0f;
    float inputHeight = 50.0f;
    float spacing = 70.0f;

    float startY = centerY - 100;

    // Username input
    _usernameInput = std::make_unique<ui::TextInput>(
        Vec2f{centerX - inputWidth / 2, startY},
        Vec2f{inputWidth, inputHeight},
        "Username",
        FONT_KEY
    );
    _usernameInput->setMaxLength(32);

    // Password input
    _passwordInput = std::make_unique<ui::TextInput>(
        Vec2f{centerX - inputWidth / 2, startY + spacing},
        Vec2f{inputWidth, inputHeight},
        "Password",
        FONT_KEY,
        true  // isPassword
    );
    _passwordInput->setMaxLength(255);

    // Email input (for register mode)
    _emailInput = std::make_unique<ui::TextInput>(
        Vec2f{centerX - inputWidth / 2, startY + spacing * 2},
        Vec2f{inputWidth, inputHeight},
        "Email",
        FONT_KEY
    );
    _emailInput->setMaxLength(255);
    _emailInput->setVisible(false);

    // Confirm password (for register mode)
    _confirmPasswordInput = std::make_unique<ui::TextInput>(
        Vec2f{centerX - inputWidth / 2, startY + spacing * 3},
        Vec2f{inputWidth, inputHeight},
        "Confirm Password",
        FONT_KEY,
        true
    );
    _confirmPasswordInput->setMaxLength(255);
    _confirmPasswordInput->setVisible(false);

    // Submit button
    float buttonWidth = 160.0f;
    float buttonHeight = 50.0f;
    float buttonY = startY + spacing * 2 + 20;

    _submitButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth - 10, buttonY},
        Vec2f{buttonWidth, buttonHeight},
        "LOGIN",
        FONT_KEY
    );
    _submitButton->setOnClick([this]() { onLoginClick(); });
    _submitButton->setNormalColor({50, 120, 80, 255});
    _submitButton->setHoveredColor({70, 150, 100, 255});

    // Switch mode button
    _switchModeButton = std::make_unique<ui::Button>(
        Vec2f{centerX + 10, buttonY},
        Vec2f{buttonWidth, buttonHeight},
        "REGISTER",
        FONT_KEY
    );
    _switchModeButton->setOnClick([this]() {
        if (_isRegisterMode) {
            switchToLoginMode();
        } else {
            switchToRegisterMode();
        }
    });

    // Server config button (bottom left)
    constexpr float UBUNTU_OFFSET = 30.0f;
    _serverConfigButton = std::make_unique<ui::Button>(
        Vec2f{20, SCREEN_HEIGHT - 70 - UBUNTU_OFFSET},
        Vec2f{120, 40},
        "SERVER",
        FONT_KEY
    );
    _serverConfigButton->setOnClick([this]() {
        showServerConfigUI();
    });
    _serverConfigButton->setNormalColor({60, 60, 100, 255});
    _serverConfigButton->setHoveredColor({80, 80, 130, 255});

    _uiInitialized = true;
}

void LoginScene::switchToRegisterMode()
{
    _isRegisterMode = true;
    _emailInput->setVisible(true);
    _confirmPasswordInput->setVisible(true);
    _submitButton->setText("REGISTER");
    _submitButton->setOnClick([this]() { onRegisterClick(); });
    _switchModeButton->setText("LOGIN");

    // Adjust button positions
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float buttonY = centerY - 100 + 70 * 4 + 20;
    _submitButton->setPos(Vec2f{centerX - 170, buttonY});
    _switchModeButton->setPos(Vec2f{centerX + 10, buttonY});
}

void LoginScene::switchToLoginMode()
{
    _isRegisterMode = false;
    _emailInput->setVisible(false);
    _confirmPasswordInput->setVisible(false);
    _submitButton->setText("LOGIN");
    _submitButton->setOnClick([this]() { onLoginClick(); });
    _switchModeButton->setText("REGISTER");

    // Reset button positions
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float buttonY = centerY - 100 + 70 * 2 + 20;
    _submitButton->setPos(Vec2f{centerX - 170, buttonY});
    _switchModeButton->setPos(Vec2f{centerX + 10, buttonY});
}

void LoginScene::onLoginClick()
{
    std::string username = _usernameInput->getText();
    std::string password = _passwordInput->getText();

    if (username.empty()) {
        showError("Please enter a username");
        return;
    }
    if (password.empty()) {
        showError("Please enter a password");
        return;
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendLoginData(username, password);
        showSuccess("Logging in...");
    } else {
        showError("Not connected to auth server");
    }
}

void LoginScene::onRegisterClick()
{
    std::string username = _usernameInput->getText();
    std::string password = _passwordInput->getText();
    std::string email = _emailInput->getText();
    std::string confirmPassword = _confirmPasswordInput->getText();

    if (username.empty()) {
        showError("Please enter a username");
        return;
    }
    if (email.empty()) {
        showError("Please enter an email");
        return;
    }
    if (password.empty()) {
        showError("Please enter a password");
        return;
    }
    if (password != confirmPassword) {
        showError("Passwords do not match");
        return;
    }
    if (password.length() < 6) {
        showError("Password must be at least 6 characters");
        return;
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendRegisterData(username, email, password);
        showSuccess("Registering...");
    } else {
        showError("Not connected to auth server");
    }
}

void LoginScene::showError(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void LoginScene::showSuccess(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {100, 255, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void LoginScene::initServerConfigUI()
{
    if (_configUIInitialized) return;

    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Panel dimensions (must match renderServerConfigUI)
    float panelWidth = 450.0f;
    float panelX = SCREEN_WIDTH / 2 - panelWidth / 2;
    float panelY = SCREEN_HEIGHT / 2 - 180.0f;

    // Layout inside panel
    float contentX = panelX + 30.0f;
    float inputWidth = panelWidth - 60.0f;
    float inputHeight = 40.0f;
    float labelHeight = 25.0f;
    float inputStartY = panelY + 70.0f;

    // Host input
    _hostInput = std::make_unique<ui::TextInput>(
        Vec2f{contentX, inputStartY + labelHeight},
        Vec2f{inputWidth, inputHeight},
        "Adresse serveur",
        FONT_KEY
    );
    _hostInput->setText(serverConfig.getHost());
    _hostInput->setMaxLength(63);

    // Port inputs row
    float portsY = inputStartY + labelHeight + inputHeight + 40.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;

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

        // ConnectionScene will handle disconnect + reconnection with new config
        if (_sceneManager) {
            _sceneManager->changeScene(std::make_unique<ConnectionScene>(
                core::ConnectionSceneMode::InitialConnection, true));
        }
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

        // ConnectionScene will handle disconnect + reconnection with new config
        if (_sceneManager) {
            _sceneManager->changeScene(std::make_unique<ConnectionScene>(
                core::ConnectionSceneMode::InitialConnection, true));
        }
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

void LoginScene::showServerConfigUI()
{
    if (!_configUIInitialized) {
        initServerConfigUI();
    }

    // Refresh values from config
    auto& serverConfig = config::ServerConfigManager::getInstance();
    _hostInput->setText(serverConfig.getHost());
    _tcpPortInput->setText(std::to_string(serverConfig.getTcpPort()));
    _udpPortInput->setText(std::to_string(serverConfig.getUdpPort()));

    _showingConfigUI = true;
}

void LoginScene::hideServerConfigUI()
{
    _showingConfigUI = false;
}

void LoginScene::applyServerConfig()
{
    auto& serverConfig = config::ServerConfigManager::getInstance();

    std::string host = _hostInput->getText();
    std::string tcpPortStr = _tcpPortInput->getText();
    std::string udpPortStr = _udpPortInput->getText();

    if (host.empty()) {
        showError("Veuillez entrer une adresse");
        return;
    }

    try {
        int tcpPort = std::stoi(tcpPortStr);
        int udpPort = std::stoi(udpPortStr);

        if (tcpPort <= 0 || tcpPort > 65535 || udpPort <= 0 || udpPort > 65535) {
            showError("Port invalide (1-65535)");
            return;
        }

        serverConfig.setHost(host);
        serverConfig.setTcpPort(static_cast<uint16_t>(tcpPort));
        serverConfig.setUdpPort(static_cast<uint16_t>(udpPort));
        serverConfig.save();

        hideServerConfigUI();

        // ConnectionScene will handle disconnect + reconnection with new config
        if (_sceneManager) {
            _sceneManager->changeScene(std::make_unique<ConnectionScene>(
                core::ConnectionSceneMode::InitialConnection, true));
        }
    } catch (...) {
        showError("Port invalide");
    }
}

void LoginScene::renderServerConfigUI()
{
    if (!_context.window) return;

    // Semi-transparent overlay
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    // Panel dimensions
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

    // Layout variables
    float contentX = panelX + 30.0f;
    float inputWidth = panelWidth - 60.0f;
    float inputHeight = 40.0f;
    float labelHeight = 25.0f;
    float inputStartY = panelY + 70.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;

    // Host label
    _context.window->drawText(FONT_KEY, "Adresse du serveur:",
        contentX, inputStartY, 18, {180, 180, 200, 255});

    if (_hostInput) _hostInput->render(*_context.window);

    // Ports row
    float portsY = inputStartY + labelHeight + inputHeight + 40.0f;

    // TCP port label
    _context.window->drawText(FONT_KEY, "Port TCP:",
        contentX, portsY, 18, {180, 180, 200, 255});

    // UDP port label
    _context.window->drawText(FONT_KEY, "Port UDP:",
        contentX + portInputWidth + 20.0f, portsY, 18, {180, 180, 200, 255});

    if (_tcpPortInput) _tcpPortInput->render(*_context.window);
    if (_udpPortInput) _udpPortInput->render(*_context.window);

    // Quick connect label
    float quickBtnY = portsY + labelHeight + inputHeight + 25.0f;
    _context.window->drawText(FONT_KEY, "Connexion rapide:",
        contentX, quickBtnY - 22.0f, 16, {140, 140, 160, 255});

    if (_franceButton) _franceButton->render(*_context.window);
    if (_localButton) _localButton->render(*_context.window);

    if (_connectButton) _connectButton->render(*_context.window);
    if (_cancelButton) _cancelButton->render(*_context.window);

    // Manual input hint
    _context.window->drawText(FONT_KEY, "Ou saisir une adresse personnalisee ci-dessus",
        SCREEN_WIDTH / 2 - 160, panelY + panelHeight - 30, 13, {100, 100, 120, 255});
}

void LoginScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    // Handle server config UI if showing
    if (_showingConfigUI) {
        if (_hostInput) _hostInput->handleEvent(event);
        if (_tcpPortInput) _tcpPortInput->handleEvent(event);
        if (_udpPortInput) _udpPortInput->handleEvent(event);
        if (_franceButton) _franceButton->handleEvent(event);
        if (_localButton) _localButton->handleEvent(event);
        if (_connectButton) _connectButton->handleEvent(event);
        if (_cancelButton) _cancelButton->handleEvent(event);

        // Escape to close config UI
        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Escape) {
                hideServerConfigUI();
            }
        }
        return;
    }

    _usernameInput->handleEvent(event);
    _passwordInput->handleEvent(event);

    if (_isRegisterMode) {
        _emailInput->handleEvent(event);
        _confirmPasswordInput->handleEvent(event);
    }

    _submitButton->handleEvent(event);
    _switchModeButton->handleEvent(event);
    _serverConfigButton->handleEvent(event);

    // Tab key to switch between inputs
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Tab) {
            if (_usernameInput->isFocused()) {
                _usernameInput->setFocused(false);
                _passwordInput->setFocused(true);
            } else if (_passwordInput->isFocused()) {
                _passwordInput->setFocused(false);
                if (_isRegisterMode) {
                    _emailInput->setFocused(true);
                } else {
                    _usernameInput->setFocused(true);
                }
            } else if (_isRegisterMode && _emailInput->isFocused()) {
                _emailInput->setFocused(false);
                _confirmPasswordInput->setFocused(true);
            } else if (_isRegisterMode && _confirmPasswordInput->isFocused()) {
                _confirmPasswordInput->setFocused(false);
                _usernameInput->setFocused(true);
            } else {
                _usernameInput->setFocused(true);
            }
        }

        // Enter key to submit
        if (keyPressed->key == events::Key::Enter) {
            if (_isRegisterMode) {
                onRegisterClick();
            } else {
                onLoginClick();
            }
        }
    }
}

void LoginScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();
    if (!_callbacksSetup) setupTCPCallbacks();

    // Process TCP events from the event queue (thread-safe)
    processTCPEvents();

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    _usernameInput->update(deltaTime);
    _passwordInput->update(deltaTime);
    if (_isRegisterMode) {
        _emailInput->update(deltaTime);
        _confirmPasswordInput->update(deltaTime);
    }
    _submitButton->update(deltaTime);
    _switchModeButton->update(deltaTime);
    _serverConfigButton->update(deltaTime);

    // Update server config UI
    if (_showingConfigUI) {
        if (_hostInput) _hostInput->update(deltaTime);
        if (_tcpPortInput) _tcpPortInput->update(deltaTime);
        if (_udpPortInput) _udpPortInput->update(deltaTime);
        if (_franceButton) _franceButton->update(deltaTime);
        if (_localButton) _localButton->update(deltaTime);
        if (_connectButton) _connectButton->update(deltaTime);
        if (_cancelButton) _cancelButton->update(deltaTime);
    }

    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }
}

void LoginScene::render()
{
    if (!_context.window) return;

    // Draw space background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {10, 10, 30, 255});

    // Draw stars
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Draw title
    _context.window->drawText(FONT_KEY, "R-TYPE",
        SCREEN_WIDTH / 2 - 150, 120, 80, {100, 150, 255, 255});

    // Draw subtitle
    std::string subtitle = _isRegisterMode ? "CREATE ACCOUNT" : "LOGIN";
    _context.window->drawText(FONT_KEY, subtitle,
        SCREEN_WIDTH / 2 - 100, 220, 36, {180, 180, 200, 255});

    // Draw form background
    float formX = SCREEN_WIDTH / 2 - 220;
    float formY = SCREEN_HEIGHT / 2 - 150;
    float formWidth = 440;
    float formHeight = _isRegisterMode ? 420 : 280;
    _context.window->drawRect(formX, formY, formWidth, formHeight, {20, 20, 40, 200});

    // Form border
    _context.window->drawRect(formX, formY, formWidth, 2, {60, 60, 100, 255});
    _context.window->drawRect(formX, formY + formHeight - 2, formWidth, 2, {60, 60, 100, 255});
    _context.window->drawRect(formX, formY, 2, formHeight, {60, 60, 100, 255});
    _context.window->drawRect(formX + formWidth - 2, formY, 2, formHeight, {60, 60, 100, 255});

    // Draw UI components
    _usernameInput->render(*_context.window);
    _passwordInput->render(*_context.window);

    if (_isRegisterMode) {
        _emailInput->render(*_context.window);
        _confirmPasswordInput->render(*_context.window);
    }

    _submitButton->render(*_context.window);
    _switchModeButton->render(*_context.window);

    // Draw status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        float msgX = SCREEN_WIDTH / 2 - 150;
        float msgY = _isRegisterMode ? SCREEN_HEIGHT / 2 + 200 : SCREEN_HEIGHT / 2 + 120;
        _context.window->drawText(FONT_KEY, _statusMessage, msgX, msgY, 18, _statusColor);
    }

    // Draw connection status (next to SERVER button)
    constexpr float UBUNTU_OFFSET = 30.0f;
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 160, SCREEN_HEIGHT - 58 - UBUNTU_OFFSET, 14, connColor);

    // Draw server config button
    _serverConfigButton->render(*_context.window);

    // Draw server config UI overlay if showing
    if (_showingConfigUI) {
        renderServerConfigUI();
    }
}
