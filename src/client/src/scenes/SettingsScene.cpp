/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SettingsScene - Accessibility and control settings
*/

#include "scenes/SettingsScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include <variant>
#include <sstream>
#include <iomanip>
#include <cmath>

SettingsScene::SettingsScene()
{
    // Copy current settings from singleton to working copy
    auto& config = accessibility::AccessibilityConfig::getInstance();

    _colorBlindMode = config.getColorBlindMode();

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        _keyBindings[i][0] = config.getPrimaryKey(action);
        _keyBindings[i][1] = config.getSecondaryKey(action);
    }
}

void SettingsScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    _assetsLoaded = true;
}

void SettingsScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float centerX = SCREEN_WIDTH / 2.0f;

    // === ACCESSIBILITY SECTION ===
    float accessY = 180.0f;

    // Color Blind Mode button
    _colorBlindModeBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, accessY},
        Vec2f{280.0f, KEY_BTN_HEIGHT},
        getColorBlindModeDisplayName(_colorBlindMode),
        FONT_KEY
    );
    _colorBlindModeBtn->setOnClick([this]() { onColorBlindModeClick(); });
    _colorBlindModeBtn->setNormalColor({50, 80, 120, 255});
    _colorBlindModeBtn->setHoveredColor({70, 100, 150, 255});

    // Game Speed removed - now per-room (configured in LobbyScene by host)

    // === CONTROLS SECTION ===
    float controlsStartY = 400.0f;
    float primaryX = CONTROL_X;
    float secondaryX = CONTROL_X + KEY_BTN_WIDTH + KEY_BTN_SPACING;

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        float rowY = controlsStartY + i * ROW_HEIGHT;

        // Primary key button
        _keyBindButtons[i].primary = std::make_unique<ui::Button>(
            Vec2f{primaryX, rowY},
            Vec2f{KEY_BTN_WIDTH, KEY_BTN_HEIGHT},
            accessibility::AccessibilityConfig::keyToString(_keyBindings[i][0]),
            FONT_KEY
        );
        _keyBindButtons[i].primary->setOnClick([this, action]() {
            onKeyBindClick(action, true);
        });
        _keyBindButtons[i].primary->setNormalColor({60, 60, 80, 255});
        _keyBindButtons[i].primary->setHoveredColor({80, 80, 110, 255});

        // Secondary key button
        _keyBindButtons[i].secondary = std::make_unique<ui::Button>(
            Vec2f{secondaryX, rowY},
            Vec2f{KEY_BTN_WIDTH, KEY_BTN_HEIGHT},
            accessibility::AccessibilityConfig::keyToString(_keyBindings[i][1]),
            FONT_KEY
        );
        _keyBindButtons[i].secondary->setOnClick([this, action]() {
            onKeyBindClick(action, false);
        });
        _keyBindButtons[i].secondary->setNormalColor({60, 60, 80, 255});
        _keyBindButtons[i].secondary->setHoveredColor({80, 80, 110, 255});
    }

    // Reset bindings button
    float resetY = controlsStartY + ACTION_COUNT * ROW_HEIGHT + 20;
    _resetBindingsBtn = std::make_unique<ui::Button>(
        Vec2f{centerX - 100, resetY},
        Vec2f{200.0f, KEY_BTN_HEIGHT},
        "RESET TO DEFAULTS",
        FONT_KEY
    );
    _resetBindingsBtn->setOnClick([this]() { onResetBindingsClick(); });
    _resetBindingsBtn->setNormalColor({80, 60, 60, 255});
    _resetBindingsBtn->setHoveredColor({110, 80, 80, 255});

    // === BOTTOM BUTTONS ===
    float bottomY = SCREEN_HEIGHT - 120;

    _applyBtn = std::make_unique<ui::Button>(
        Vec2f{centerX - 220, bottomY},
        Vec2f{180.0f, 55.0f},
        "APPLY & SAVE",
        FONT_KEY
    );
    _applyBtn->setOnClick([this]() { onApplyClick(); });
    _applyBtn->setNormalColor({50, 120, 80, 255});
    _applyBtn->setHoveredColor({70, 150, 100, 255});

    _backBtn = std::make_unique<ui::Button>(
        Vec2f{centerX + 40, bottomY},
        Vec2f{180.0f, 55.0f},
        "BACK",
        FONT_KEY
    );
    _backBtn->setOnClick([this]() { onBackClick(); });
    _backBtn->setNormalColor({100, 60, 60, 255});
    _backBtn->setHoveredColor({130, 80, 80, 255});

    // === EXIT CONFIRMATION DIALOG BUTTONS ===
    float dialogY = SCREEN_HEIGHT / 2 + 40;
    _confirmSaveExitBtn = std::make_unique<ui::Button>(
        Vec2f{centerX - 180, dialogY},
        Vec2f{160.0f, 50.0f},
        "SAVE & EXIT",
        FONT_KEY
    );
    _confirmSaveExitBtn->setOnClick([this]() { onConfirmSaveExitClick(); });
    _confirmSaveExitBtn->setNormalColor({50, 120, 80, 255});
    _confirmSaveExitBtn->setHoveredColor({70, 150, 100, 255});

    _confirmDiscardBtn = std::make_unique<ui::Button>(
        Vec2f{centerX + 20, dialogY},
        Vec2f{160.0f, 50.0f},
        "DISCARD",
        FONT_KEY
    );
    _confirmDiscardBtn->setOnClick([this]() { onConfirmDiscardClick(); });
    _confirmDiscardBtn->setNormalColor({100, 60, 60, 255});
    _confirmDiscardBtn->setHoveredColor({130, 80, 80, 255});

    _uiInitialized = true;
}

std::string SettingsScene::getColorBlindModeDisplayName(accessibility::ColorBlindMode mode) const
{
    switch (mode) {
        case accessibility::ColorBlindMode::None: return "NONE";
        case accessibility::ColorBlindMode::Protanopia: return "PROTANOPIA";
        case accessibility::ColorBlindMode::Deuteranopia: return "DEUTERANOPIA";
        case accessibility::ColorBlindMode::Tritanopia: return "TRITANOPIA";
        case accessibility::ColorBlindMode::HighContrast: return "HIGH CONTRAST";
        default: return "NONE";
    }
}

std::string SettingsScene::getActionDisplayName(accessibility::GameAction action) const
{
    switch (action) {
        case accessibility::GameAction::MoveUp: return "Move Up";
        case accessibility::GameAction::MoveDown: return "Move Down";
        case accessibility::GameAction::MoveLeft: return "Move Left";
        case accessibility::GameAction::MoveRight: return "Move Right";
        case accessibility::GameAction::Shoot: return "Shoot";
        case accessibility::GameAction::Pause: return "Pause";
        default: return "Unknown";
    }
}

bool SettingsScene::isKeyAlreadyUsed(events::Key key, accessibility::GameAction excludeAction, bool excludePrimary) const
{
    if (key == events::Key::Unknown) return false;

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);

        // Check primary key
        if (!(action == excludeAction && excludePrimary)) {
            if (_keyBindings[i][0] == key) return true;
        }

        // Check secondary key
        if (!(action == excludeAction && !excludePrimary)) {
            if (_keyBindings[i][1] == key) return true;
        }
    }
    return false;
}

std::string SettingsScene::findConflictingAction(events::Key key, accessibility::GameAction excludeAction, bool excludePrimary) const
{
    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);

        if (!(action == excludeAction && excludePrimary)) {
            if (_keyBindings[i][0] == key) {
                return getActionDisplayName(action) + " (Primary)";
            }
        }

        if (!(action == excludeAction && !excludePrimary)) {
            if (_keyBindings[i][1] == key) {
                return getActionDisplayName(action) + " (Secondary)";
            }
        }
    }
    return "";
}

void SettingsScene::updateKeyBindButtonText(accessibility::GameAction action, bool isPrimary)
{
    size_t idx = static_cast<size_t>(action);
    auto& btn = isPrimary ? _keyBindButtons[idx].primary : _keyBindButtons[idx].secondary;
    events::Key key = _keyBindings[idx][isPrimary ? 0 : 1];
    btn->setText(accessibility::AccessibilityConfig::keyToString(key));
}

void SettingsScene::updateAllKeyBindButtonTexts()
{
    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        updateKeyBindButtonText(action, true);
        updateKeyBindButtonText(action, false);
    }
}

void SettingsScene::onColorBlindModeClick()
{
    // Cycle through modes
    int mode = static_cast<int>(_colorBlindMode);
    mode = (mode + 1) % 5;
    _colorBlindMode = static_cast<accessibility::ColorBlindMode>(mode);
    _colorBlindModeBtn->setText(getColorBlindModeDisplayName(_colorBlindMode));
    _hasUnsavedChanges = true;
}

// Game speed callbacks removed - now per-room (configured in LobbyScene by host)

void SettingsScene::onKeyBindClick(accessibility::GameAction action, bool isPrimary)
{
    _rebindState = KeyRebindState::Listening;
    _rebindAction = action;
    _rebindIsPrimary = isPrimary;
    _rebindBlinkTimer = 0.0f;

    // Visual feedback - change button text
    size_t idx = static_cast<size_t>(action);
    auto& btn = isPrimary ? _keyBindButtons[idx].primary : _keyBindButtons[idx].secondary;
    btn->setText("Press key...");
    btn->setNormalColor({150, 100, 50, 255});
}

void SettingsScene::onResetBindingsClick()
{
    // Reset to default bindings
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveUp)] = {events::Key::Up, events::Key::Z};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveDown)] = {events::Key::Down, events::Key::S};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveLeft)] = {events::Key::Left, events::Key::Q};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveRight)] = {events::Key::Right, events::Key::D};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::Shoot)] = {events::Key::Space, events::Key::Enter};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::Pause)] = {events::Key::Escape, events::Key::P};

    updateAllKeyBindButtonTexts();
    _hasUnsavedChanges = true;
    showInfo("Key bindings reset to defaults");
}

void SettingsScene::onApplyClick()
{
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Apply settings to singleton
    config.setColorBlindMode(_colorBlindMode);
    // Game speed removed - now per-room (configured in LobbyScene by host)

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        config.setKeyBinding(action, _keyBindings[i][0], _keyBindings[i][1]);
    }

    // Send to server if connected
    if (_context.tcpClient && _context.tcpClient->isConnected() && _context.tcpClient->isAuthenticated()) {
        UserSettingsPayload payload;
        std::strncpy(payload.colorBlindMode,
            accessibility::AccessibilityConfig::colorBlindModeToString(_colorBlindMode).c_str(),
            COLORBLIND_MODE_LEN - 1);
        payload.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
        payload.gameSpeedPercent = 100;  // Game speed now per-room, not per-player

        for (size_t i = 0; i < ACTION_COUNT; ++i) {
            payload.keyBindings[i * 2] = static_cast<uint8_t>(_keyBindings[i][0]);
            payload.keyBindings[i * 2 + 1] = static_cast<uint8_t>(_keyBindings[i][1]);
        }

        _context.tcpClient->saveUserSettings(payload);
        showInfo("Saving to server...");
    } else {
        showError("Not connected - cannot save settings");
    }

    _hasUnsavedChanges = false;
}

void SettingsScene::onBackClick()
{
    if (_hasUnsavedChanges) {
        _showExitConfirm = true;
    } else {
        if (_sceneManager) {
            _sceneManager->changeScene(std::make_unique<MainMenuScene>());
        }
    }
}

void SettingsScene::onConfirmSaveExitClick()
{
    onApplyClick();
    _showExitConfirm = false;
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void SettingsScene::onConfirmDiscardClick()
{
    _showExitConfirm = false;
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void SettingsScene::showError(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {255, 100, 100, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void SettingsScene::showInfo(const std::string& message)
{
    _statusMessage = message;
    _statusColor = {100, 200, 255, 255};
    _statusDisplayTimer = STATUS_DISPLAY_DURATION;
}

void SettingsScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPSaveSettingsResultEvent>) {
                if (event.success) {
                    showInfo("Settings saved to server!");
                } else {
                    showError("Server save failed: " + event.message);
                }
            }
            else if constexpr (std::is_same_v<T, client::network::TCPUserSettingsEvent>) {
                // Apply received settings (if we requested them)
                if (event.found) {
                    _colorBlindMode = accessibility::AccessibilityConfig::stringToColorBlindMode(event.colorBlindMode);
                    // Game speed ignored - now per-room (configured in LobbyScene by host)

                    for (size_t i = 0; i < ACTION_COUNT; ++i) {
                        _keyBindings[i][0] = static_cast<events::Key>(event.keyBindings[i * 2]);
                        _keyBindings[i][1] = static_cast<events::Key>(event.keyBindings[i * 2 + 1]);
                    }

                    // Update UI
                    if (_colorBlindModeBtn) {
                        _colorBlindModeBtn->setText(getColorBlindModeDisplayName(_colorBlindMode));
                    }
                    updateAllKeyBindButtonTexts();

                    showInfo("Settings loaded from server");
                }
            }
            // Ignore other event types
        }, *eventOpt);
    }
}

void SettingsScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    // Handle exit confirmation dialog
    if (_showExitConfirm) {
        _confirmSaveExitBtn->handleEvent(event);
        _confirmDiscardBtn->handleEvent(event);

        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Escape) {
                _showExitConfirm = false;
            }
        }
        return;
    }

    // Handle key rebinding mode
    if (_rebindState == KeyRebindState::Listening && _rebindAction.has_value()) {
        if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
            if (keyPressed->key == events::Key::Escape) {
                // Cancel rebind
                _rebindState = KeyRebindState::None;
                updateKeyBindButtonText(*_rebindAction, _rebindIsPrimary);

                // Restore button color
                size_t idx = static_cast<size_t>(*_rebindAction);
                auto& btn = _rebindIsPrimary ? _keyBindButtons[idx].primary : _keyBindButtons[idx].secondary;
                btn->setNormalColor({60, 60, 80, 255});

                _rebindAction.reset();
            } else {
                // Check for conflict
                if (isKeyAlreadyUsed(keyPressed->key, *_rebindAction, _rebindIsPrimary)) {
                    std::string conflict = findConflictingAction(keyPressed->key, *_rebindAction, _rebindIsPrimary);
                    showError("Key already assigned to: " + conflict);
                    // Stay in listening mode
                } else {
                    // Accept the key
                    size_t idx = static_cast<size_t>(*_rebindAction);
                    _keyBindings[idx][_rebindIsPrimary ? 0 : 1] = keyPressed->key;

                    updateKeyBindButtonText(*_rebindAction, _rebindIsPrimary);

                    // Restore button color
                    auto& btn = _rebindIsPrimary ? _keyBindButtons[idx].primary : _keyBindButtons[idx].secondary;
                    btn->setNormalColor({60, 60, 80, 255});

                    _hasUnsavedChanges = true;
                    _rebindState = KeyRebindState::None;
                    _rebindAction.reset();
                }
            }
            return;
        }
        return;
    }

    // Normal event handling
    _colorBlindModeBtn->handleEvent(event);
    // Speed buttons removed - game speed is now per-room (LobbyScene)

    for (auto& bindBtn : _keyBindButtons) {
        bindBtn.primary->handleEvent(event);
        bindBtn.secondary->handleEvent(event);
    }
    _resetBindingsBtn->handleEvent(event);

    _applyBtn->handleEvent(event);
    _backBtn->handleEvent(event);

    // Escape to go back
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            onBackClick();
        }
    }
}

void SettingsScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();

    // Process TCP events for save confirmation
    processTCPEvents();

    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Update blink timer for rebind feedback
    if (_rebindState == KeyRebindState::Listening) {
        _rebindBlinkTimer += deltaTime;
    }

    // Update all buttons
    _colorBlindModeBtn->update(deltaTime);
    // Speed buttons removed - game speed is now per-room (LobbyScene)

    for (auto& bindBtn : _keyBindButtons) {
        bindBtn.primary->update(deltaTime);
        bindBtn.secondary->update(deltaTime);
    }
    _resetBindingsBtn->update(deltaTime);

    _applyBtn->update(deltaTime);
    _backBtn->update(deltaTime);

    _confirmSaveExitBtn->update(deltaTime);
    _confirmDiscardBtn->update(deltaTime);

    if (_statusDisplayTimer > 0) {
        _statusDisplayTimer -= deltaTime;
    }
}

void SettingsScene::render()
{
    if (!_context.window) return;

    // Background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {8, 8, 25, 255});

    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Title
    _context.window->drawText(FONT_KEY, "R-TYPE SETTINGS",
        SCREEN_WIDTH / 2 - 180, 40, 48, {100, 150, 255, 255});

    // === ACCESSIBILITY SECTION ===
    float accessBoxY = 120.0f;
    float accessBoxHeight = 160.0f;

    // Section box
    _context.window->drawRect(BOX_MARGIN_X, accessBoxY, BOX_WIDTH, accessBoxHeight, {20, 20, 40, 200});
    _context.window->drawRect(BOX_MARGIN_X, accessBoxY, BOX_WIDTH, 3, {60, 80, 120, 255});

    // Section title
    _context.window->drawText(FONT_KEY, "ACCESSIBILITY",
        BOX_MARGIN_X + 20, accessBoxY + 15, 24, {150, 150, 180, 255});

    // Color Blind Mode label
    _context.window->drawText(FONT_KEY, "Color Blind Mode:",
        LABEL_X, 185, 20, {200, 200, 220, 255});
    _colorBlindModeBtn->render(*_context.window);

    // Game Speed removed - now per-room (configured in LobbyScene by host)

    // === CONTROLS SECTION ===
    float controlsBoxY = 320.0f;
    float controlsBoxHeight = 450.0f;

    // Section box
    _context.window->drawRect(BOX_MARGIN_X, controlsBoxY, BOX_WIDTH, controlsBoxHeight, {20, 20, 40, 200});
    _context.window->drawRect(BOX_MARGIN_X, controlsBoxY, BOX_WIDTH, 3, {60, 80, 120, 255});

    // Section title
    _context.window->drawText(FONT_KEY, "CONTROLS",
        BOX_MARGIN_X + 20, controlsBoxY + 15, 24, {150, 150, 180, 255});

    // Column headers
    _context.window->drawText(FONT_KEY, "Primary",
        CONTROL_X + 25, controlsBoxY + 55, 16, {120, 120, 150, 255});
    _context.window->drawText(FONT_KEY, "Secondary",
        CONTROL_X + KEY_BTN_WIDTH + KEY_BTN_SPACING + 15, controlsBoxY + 55, 16, {120, 120, 150, 255});

    // Key bindings
    float controlsStartY = 400.0f;
    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        float rowY = controlsStartY + i * ROW_HEIGHT;

        // Action label
        _context.window->drawText(FONT_KEY, getActionDisplayName(action),
            LABEL_X, rowY + 12, 20, {200, 200, 220, 255});

        _keyBindButtons[i].primary->render(*_context.window);
        _keyBindButtons[i].secondary->render(*_context.window);
    }

    _resetBindingsBtn->render(*_context.window);

    // === BOTTOM BUTTONS ===
    _applyBtn->render(*_context.window);
    _backBtn->render(*_context.window);

    // Unsaved changes indicator
    if (_hasUnsavedChanges) {
        _context.window->drawText(FONT_KEY, "*",
            _applyBtn->getPos().x - 20, _applyBtn->getPos().y + 15, 30, {255, 200, 100, 255});
    }

    // Status message
    if (_statusDisplayTimer > 0 && !_statusMessage.empty()) {
        _context.window->drawText(FONT_KEY, _statusMessage,
            SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 50, 18, _statusColor);
    }

    // === EXIT CONFIRMATION DIALOG ===
    if (_showExitConfirm) {
        // Overlay
        _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

        // Dialog box
        float dialogW = 450.0f;
        float dialogH = 180.0f;
        float dialogX = SCREEN_WIDTH / 2 - dialogW / 2;
        float dialogY = SCREEN_HEIGHT / 2 - dialogH / 2;

        _context.window->drawRect(dialogX, dialogY, dialogW, dialogH, {30, 30, 55, 250});
        _context.window->drawRect(dialogX, dialogY, dialogW, 3, {80, 100, 150, 255});
        _context.window->drawRect(dialogX, dialogY + dialogH - 3, dialogW, 3, {80, 100, 150, 255});
        _context.window->drawRect(dialogX, dialogY, 3, dialogH, {80, 100, 150, 255});
        _context.window->drawRect(dialogX + dialogW - 3, dialogY, 3, dialogH, {80, 100, 150, 255});

        // Dialog text
        _context.window->drawText(FONT_KEY, "UNSAVED CHANGES",
            SCREEN_WIDTH / 2 - 100, dialogY + 25, 24, {255, 200, 100, 255});
        _context.window->drawText(FONT_KEY, "You have unsaved changes.",
            SCREEN_WIDTH / 2 - 130, dialogY + 70, 18, {200, 200, 220, 255});
        _context.window->drawText(FONT_KEY, "What would you like to do?",
            SCREEN_WIDTH / 2 - 130, dialogY + 95, 18, {200, 200, 220, 255});

        _confirmSaveExitBtn->render(*_context.window);
        _confirmDiscardBtn->render(*_context.window);
    }
}
