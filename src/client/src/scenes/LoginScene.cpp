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
#include "core/Version.hpp"
#include "utils/Platform.hpp"
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
                // Auth successful - but don't navigate yet if version popup is showing
                // (TCPVersionMismatchEvent is processed before TCPAuthSuccessEvent in the same poll loop)
                if (_showVersionPopup) {
                    // Version mismatch popup is showing, don't navigate
                    // User will navigate via the popup buttons
                    return;
                }
                // Request user settings from server
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
            else if constexpr (std::is_same_v<T, client::network::TCPVersionMismatchEvent>) {
                // Version mismatch detected - show update popup
                // This event is always emitted BEFORE TCPAuthSuccessEvent
                _showVersionPopup = true;
                _versionPopupClientVersion = client::core::formatVersion(event.clientVersion);
                _versionPopupServerVersion = client::core::formatVersion(event.serverVersion);
                _versionPopupCommitsBehind = event.commitsBehind;
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
    constexpr float UBUNTU_OFFSET = 0.0f;  // Removed: fullscreen mode handles this
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

void LoginScene::showServerConfigUI()
{
    if (!_configPanel) {
        _configPanel = std::make_unique<ui::ServerConfigPanel>(SCREEN_WIDTH, SCREEN_HEIGHT, FONT_KEY);
        _configPanel->setOnConnect([this]() {
            hideServerConfigUI();
            // ConnectionScene will handle disconnect + reconnection with new config
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<ConnectionScene>(
                    core::ConnectionSceneMode::InitialConnection, true));
            }
        });
        _configPanel->setOnCancel([this]() {
            hideServerConfigUI();
        });
    }
    _configPanel->refreshFromConfig();
    _showingConfigUI = true;
}

void LoginScene::hideServerConfigUI()
{
    _showingConfigUI = false;
}

void LoginScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    // Handle version popup if showing
    if (_showVersionPopup) {
        if (_versionPopupCloseBtn) _versionPopupCloseBtn->handleEvent(event);
        if (_versionPopupUpdateBtn) _versionPopupUpdateBtn->handleEvent(event);
        return;
    }

    // Handle server config UI if showing
    if (_showingConfigUI && _configPanel) {
        _configPanel->handleEvent(event);
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
    if (_showingConfigUI && _configPanel) {
        _configPanel->update(deltaTime);
    }

    // Update version popup buttons
    if (_showVersionPopup) {
        if (_versionPopupCloseBtn) _versionPopupCloseBtn->update(deltaTime);
        if (_versionPopupUpdateBtn) _versionPopupUpdateBtn->update(deltaTime);
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
    constexpr float UBUNTU_OFFSET = 0.0f;  // Removed: fullscreen mode handles this
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 160, SCREEN_HEIGHT - 58 - UBUNTU_OFFSET, 14, connColor);

    // Draw server config button
    _serverConfigButton->render(*_context.window);

    // Draw server config UI overlay if showing
    if (_showingConfigUI && _configPanel) {
        _configPanel->render(*_context.window);
    }

    // Draw version mismatch popup overlay if showing
    if (_showVersionPopup) {
        renderVersionPopup();
    }
}

void LoginScene::renderVersionPopup()
{
    if (!_context.window) return;

    // Semi-transparent overlay
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    // Popup box
    float popupWidth = 600.0f;
    float popupHeight = 350.0f;
    float popupX = (SCREEN_WIDTH - popupWidth) / 2;
    float popupY = (SCREEN_HEIGHT - popupHeight) / 2;

    // Background
    _context.window->drawRect(popupX, popupY, popupWidth, popupHeight, {30, 30, 50, 255});

    // Border
    _context.window->drawRect(popupX, popupY, popupWidth, 3, {255, 150, 50, 255});
    _context.window->drawRect(popupX, popupY + popupHeight - 3, popupWidth, 3, {255, 150, 50, 255});
    _context.window->drawRect(popupX, popupY, 3, popupHeight, {255, 150, 50, 255});
    _context.window->drawRect(popupX + popupWidth - 3, popupY, 3, popupHeight, {255, 150, 50, 255});

    // Title
    _context.window->drawText(FONT_KEY, "UPDATE REQUIRED",
        popupX + popupWidth / 2 - 120, popupY + 30, 32, {255, 200, 100, 255});

    // Warning icon (just text for now)
    _context.window->drawText(FONT_KEY, "!",
        popupX + popupWidth / 2 - 10, popupY + 80, 48, {255, 150, 50, 255});

    // Message - show how many commits behind
    std::string behindMsg;
    if (_versionPopupCommitsBehind > 0) {
        behindMsg = "You are " + std::to_string(_versionPopupCommitsBehind) + " commit" +
                   (_versionPopupCommitsBehind > 1 ? "s" : "") + " behind.";
    } else if (_versionPopupCommitsBehind == 0) {
        behindMsg = "Version mismatch detected.";
    } else {
        behindMsg = "Your version is too old to determine exact difference.";
    }
    _context.window->drawText(FONT_KEY, behindMsg,
        popupX + 50, popupY + 150, 20, {255, 180, 100, 255});

    _context.window->drawText(FONT_KEY, "Your version: " + _versionPopupClientVersion,
        popupX + 50, popupY + 185, 18, {150, 150, 200, 255});

    _context.window->drawText(FONT_KEY, "Server version: " + _versionPopupServerVersion,
        popupX + 50, popupY + 215, 18, {150, 200, 150, 255});

    _context.window->drawText(FONT_KEY, "Please download the latest version from Jenkins.",
        popupX + 50, popupY + 255, 18, {180, 180, 180, 255});

    // Initialize buttons if not done
    if (!_versionPopupCloseBtn) {
        _versionPopupCloseBtn = std::make_unique<ui::Button>(
            Vec2f{popupX + popupWidth / 2 - 180, popupY + popupHeight - 70},
            Vec2f{150, 45},
            "CONTINUE",
            FONT_KEY
        );
        _versionPopupCloseBtn->setOnClick([this]() { onVersionPopupClose(); });
        _versionPopupCloseBtn->setNormalColor({80, 80, 100, 255});
        _versionPopupCloseBtn->setHoveredColor({100, 100, 130, 255});
    }

    if (!_versionPopupUpdateBtn) {
        _versionPopupUpdateBtn = std::make_unique<ui::Button>(
            Vec2f{popupX + popupWidth / 2 + 30, popupY + popupHeight - 70},
            Vec2f{150, 45},
            "JENKINS",
            FONT_KEY
        );
        _versionPopupUpdateBtn->setOnClick([this]() { onVersionPopupUpdate(); });
        _versionPopupUpdateBtn->setNormalColor({50, 120, 80, 255});
        _versionPopupUpdateBtn->setHoveredColor({70, 150, 100, 255});
    }

    _versionPopupCloseBtn->render(*_context.window);
    _versionPopupUpdateBtn->render(*_context.window);
}

void LoginScene::onVersionPopupClose()
{
    _showVersionPopup = false;
    // User chose to continue anyway - proceed to main menu
    if (_context.tcpClient) {
        _context.tcpClient->requestUserSettings();
    }
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void LoginScene::onVersionPopupUpdate() const
{
    // Open Jenkins artifacts page (latest successful build)
    client::utils::openUrlInBrowser(
        "http://51.254.137.175:8080/job/BuildBranch/job/main/lastSuccessfulBuild/artifact/artifacts/"
    );
}

