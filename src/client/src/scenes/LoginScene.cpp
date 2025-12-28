/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LoginScene
*/

#include "scenes/LoginScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "network/NetworkEvents.hpp"
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
                // Auth successful - go to main menu (TCP stays open for future features)
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

void LoginScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    _usernameInput->handleEvent(event);
    _passwordInput->handleEvent(event);

    if (_isRegisterMode) {
        _emailInput->handleEvent(event);
        _confirmPasswordInput->handleEvent(event);
    }

    _submitButton->handleEvent(event);
    _switchModeButton->handleEvent(event);

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

    // Draw connection status
    std::string connStatus = (_context.tcpClient && _context.tcpClient->isConnected())
        ? "Connected" : "Disconnected";
    rgba connColor = (_context.tcpClient && _context.tcpClient->isConnected())
        ? rgba{100, 255, 100, 255} : rgba{255, 100, 100, 255};
    _context.window->drawText(FONT_KEY, connStatus, 20, SCREEN_HEIGHT - 40, 14, connColor);
}
