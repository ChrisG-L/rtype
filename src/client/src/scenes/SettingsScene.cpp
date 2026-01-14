/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SettingsScene - Accessibility and control settings
*/

#include "scenes/SettingsScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "audio/VoiceChatManager.hpp"
#include "core/Logger.hpp"
#include <variant>
#include <sstream>
#include <iomanip>
#include <cmath>

SettingsScene::SettingsScene()
{
    // Copy current settings from singleton to working copy
    auto& config = accessibility::AccessibilityConfig::getInstance();

    _colorBlindMode = config.getColorBlindMode();
    _shipSkin = config.getShipSkin();

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        _keyBindings[i][0] = config.getPrimaryKey(action);
        _keyBindings[i][1] = config.getSecondaryKey(action);
    }

    // Voice settings from VoiceChatManager
    auto& voice = audio::VoiceChatManager::getInstance();
    _voiceMode = (voice.getVoiceMode() == audio::VoiceChatManager::VoiceMode::VoiceActivity) ? 1 : 0;
    _vadThreshold = static_cast<uint8_t>(voice.getVADThreshold() * 100.0f);
    _micGain = static_cast<uint8_t>(voice.getMicGain() * 100.0f);
    _voiceVolume = static_cast<uint8_t>(voice.getPlaybackVolume());

    // Audio device selection
    _selectedInputDevice = voice.getSelectedInputDevice();
    _selectedOutputDevice = voice.getSelectedOutputDevice();
    loadAudioDevices();
}

void SettingsScene::loadAudioDevices()
{
    auto& voice = audio::VoiceChatManager::getInstance();

    // Load input devices
    _inputDeviceNames.clear();
    auto inputs = voice.getInputDevices();
    for (const auto& dev : inputs) {
        _inputDeviceNames.push_back(dev.name);
        if (dev.name == _selectedInputDevice ||
            (_selectedInputDevice.empty() && dev.index == -1)) {
            _inputDeviceIndex = static_cast<int>(_inputDeviceNames.size() - 1);
        }
    }
    if (_inputDeviceNames.empty()) {
        _inputDeviceNames.push_back("Auto (default)");
    }

    // Load output devices
    _outputDeviceNames.clear();
    auto outputs = voice.getOutputDevices();
    for (const auto& dev : outputs) {
        _outputDeviceNames.push_back(dev.name);
        if (dev.name == _selectedOutputDevice ||
            (_selectedOutputDevice.empty() && dev.index == -1)) {
            _outputDeviceIndex = static_cast<int>(_outputDeviceNames.size() - 1);
        }
    }
    if (_outputDeviceNames.empty()) {
        _outputDeviceNames.push_back("Auto (default)");
    }
}

std::string SettingsScene::truncateDeviceName(const std::string& name, size_t maxLen) const
{
    if (name.length() <= maxLen) return name;
    return name.substr(0, maxLen - 3) + "...";
}

void SettingsScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _starfield = std::make_unique<ui::StarfieldBackground>(SCREEN_WIDTH, SCREEN_HEIGHT, STAR_COUNT);

    // Load ship textures for preview
    for (size_t i = 1; i <= SHIP_SKIN_COUNT; ++i) {
        std::string key = "settings_ship" + std::to_string(i);
        std::string path = "assets/spaceship/Ship" + std::to_string(i) + ".png";
        _context.window->loadTexture(key, path);
    }

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

    // === VOICE CHAT SECTION ===
    float voiceY = 240.0f;
    float voiceRowHeight = 50.0f;
    float plusMinusBtnW = 40.0f;

    // Voice Mode (PTT / VAD)
    _voiceModeBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, voiceY},
        Vec2f{180.0f, KEY_BTN_HEIGHT},
        _voiceMode == 0 ? "PUSH-TO-TALK" : "VOICE ACTIVITY",
        FONT_KEY
    );
    _voiceModeBtn->setOnClick([this]() { onVoiceModeClick(); });
    _voiceModeBtn->setNormalColor({50, 80, 120, 255});
    _voiceModeBtn->setHoveredColor({70, 100, 150, 255});

    // VAD Threshold (only relevant in VAD mode)
    float vadY = voiceY + voiceRowHeight;
    _vadThresholdMinusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, vadY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "-", FONT_KEY);
    _vadThresholdMinusBtn->setOnClick([this]() {
        if (_vadThreshold > 0) { _vadThreshold -= 5; _hasUnsavedChanges = true; }
    });
    _vadThresholdMinusBtn->setNormalColor({60, 60, 80, 255});

    _vadThresholdPlusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X + 150, vadY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "+", FONT_KEY);
    _vadThresholdPlusBtn->setOnClick([this]() {
        if (_vadThreshold < 100) { _vadThreshold += 5; _hasUnsavedChanges = true; }
    });
    _vadThresholdPlusBtn->setNormalColor({60, 60, 80, 255});

    // Mic Gain
    float micY = vadY + voiceRowHeight;
    _micGainMinusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, micY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "-", FONT_KEY);
    _micGainMinusBtn->setOnClick([this]() {
        if (_micGain > 0) { _micGain -= 10; _hasUnsavedChanges = true; }
    });
    _micGainMinusBtn->setNormalColor({60, 60, 80, 255});

    _micGainPlusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X + 150, micY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "+", FONT_KEY);
    _micGainPlusBtn->setOnClick([this]() {
        if (_micGain < 200) { _micGain += 10; _hasUnsavedChanges = true; }
    });
    _micGainPlusBtn->setNormalColor({60, 60, 80, 255});

    // Voice Volume
    float volY = micY + voiceRowHeight;
    _voiceVolumeMinusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, volY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "-", FONT_KEY);
    _voiceVolumeMinusBtn->setOnClick([this]() {
        if (_voiceVolume > 0) { _voiceVolume -= 10; _hasUnsavedChanges = true; }
    });
    _voiceVolumeMinusBtn->setNormalColor({60, 60, 80, 255});

    _voiceVolumePlusBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X + 150, volY}, Vec2f{plusMinusBtnW, KEY_BTN_HEIGHT}, "+", FONT_KEY);
    _voiceVolumePlusBtn->setOnClick([this]() {
        if (_voiceVolume < 100) { _voiceVolume += 10; _hasUnsavedChanges = true; }
    });
    _voiceVolumePlusBtn->setNormalColor({60, 60, 80, 255});

    // === AUDIO DEVICE SELECTION ===
    float inputDevY = volY + voiceRowHeight;
    std::string inputLabel = truncateDeviceName(_inputDeviceNames[_inputDeviceIndex], 30);
    _inputDeviceBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, inputDevY},
        Vec2f{350.0f, KEY_BTN_HEIGHT},
        inputLabel,
        FONT_KEY
    );
    _inputDeviceBtn->setOnClick([this]() { onInputDeviceClick(); });
    _inputDeviceBtn->setNormalColor({50, 70, 90, 255});
    _inputDeviceBtn->setHoveredColor({70, 90, 120, 255});

    float outputDevY = inputDevY + voiceRowHeight;
    std::string outputLabel = truncateDeviceName(_outputDeviceNames[_outputDeviceIndex], 30);
    _outputDeviceBtn = std::make_unique<ui::Button>(
        Vec2f{CONTROL_X, outputDevY},
        Vec2f{350.0f, KEY_BTN_HEIGHT},
        outputLabel,
        FONT_KEY
    );
    _outputDeviceBtn->setOnClick([this]() { onOutputDeviceClick(); });
    _outputDeviceBtn->setNormalColor({50, 70, 90, 255});
    _outputDeviceBtn->setHoveredColor({70, 90, 120, 255});

    // === SHIP SELECTION SECTION ===
    float shipY = 570.0f;  // Inside ship box
    float shipBtnSize = 60.0f;  // Smaller buttons
    float shipSpacing = 20.0f;
    float shipStartX = CONTROL_X;

    for (size_t i = 0; i < SHIP_SKIN_COUNT; ++i) {
        uint8_t skinId = static_cast<uint8_t>(i + 1);
        _shipSkinBtns[i] = std::make_unique<ui::Button>(
            Vec2f{shipStartX + i * (shipBtnSize + shipSpacing), shipY},
            Vec2f{shipBtnSize, shipBtnSize},
            std::to_string(skinId),
            FONT_KEY
        );
        _shipSkinBtns[i]->setOnClick([this, skinId]() { onShipSkinClick(skinId); });

        // Highlight selected skin
        if (skinId == _shipSkin) {
            _shipSkinBtns[i]->setNormalColor({80, 150, 80, 255});
            _shipSkinBtns[i]->setHoveredColor({100, 180, 100, 255});
        } else {
            _shipSkinBtns[i]->setNormalColor({50, 50, 70, 255});
            _shipSkinBtns[i]->setHoveredColor({70, 70, 100, 255});
        }
    }

    // === CONTROLS SECTION (2 columns for 12 actions) ===
    float controlsStartY = 670.0f;  // Adjusted for voice + ship sections
    float controlsRowHeight = 28.0f;  // Compact row height
    float controlsBtnHeight = 24.0f;  // Compact button height
    float keyBtnWidth = 80.0f;  // Smaller buttons for 2-column layout

    // Column 1: First 6 actions (left side)
    float col1LabelX = 80.0f;
    float col1PrimaryX = 200.0f;
    float col1SecondaryX = col1PrimaryX + keyBtnWidth + 10.0f;

    // Column 2: Last 6 actions (right side)
    float col2LabelX = SCREEN_WIDTH / 2 + 40.0f;
    float col2PrimaryX = SCREEN_WIDTH / 2 + 160.0f;
    float col2SecondaryX = col2PrimaryX + keyBtnWidth + 10.0f;

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);

        // Determine column and row (7 left, 6 right for 13 actions)
        bool isLeftColumn = (i < 7);
        size_t rowIndex = isLeftColumn ? i : (i - 7);
        float rowY = controlsStartY + rowIndex * controlsRowHeight;

        float primaryX = isLeftColumn ? col1PrimaryX : col2PrimaryX;
        float secondaryX = isLeftColumn ? col1SecondaryX : col2SecondaryX;

        // Primary key button
        _keyBindButtons[i].primary = std::make_unique<ui::Button>(
            Vec2f{primaryX, rowY},
            Vec2f{keyBtnWidth, controlsBtnHeight},
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
            Vec2f{keyBtnWidth, controlsBtnHeight},
            accessibility::AccessibilityConfig::keyToString(_keyBindings[i][1]),
            FONT_KEY
        );
        _keyBindButtons[i].secondary->setOnClick([this, action]() {
            onKeyBindClick(action, false);
        });
        _keyBindButtons[i].secondary->setNormalColor({60, 60, 80, 255});
        _keyBindButtons[i].secondary->setHoveredColor({80, 80, 110, 255});
    }

    // === BOTTOM BUTTONS (all 3 on same row) ===
    // 7 rows * 28px = 196px + some padding
    float bottomY = controlsStartY + 7 * controlsRowHeight + 15;
    float btnWidth = 160.0f;
    float btnSpacing = 20.0f;
    float totalWidth = btnWidth * 3 + btnSpacing * 2;
    float startX = centerX - totalWidth / 2;

    _resetBindingsBtn = std::make_unique<ui::Button>(
        Vec2f{startX, bottomY},
        Vec2f{btnWidth, 40.0f},
        "RESET",
        FONT_KEY
    );
    _resetBindingsBtn->setOnClick([this]() { onResetBindingsClick(); });
    _resetBindingsBtn->setNormalColor({80, 60, 60, 255});
    _resetBindingsBtn->setHoveredColor({110, 80, 80, 255});

    _applyBtn = std::make_unique<ui::Button>(
        Vec2f{startX + btnWidth + btnSpacing, bottomY},
        Vec2f{btnWidth, 40.0f},
        "APPLY & SAVE",
        FONT_KEY
    );
    _applyBtn->setOnClick([this]() { onApplyClick(); });
    _applyBtn->setNormalColor({50, 120, 80, 255});
    _applyBtn->setHoveredColor({70, 150, 100, 255});

    _backBtn = std::make_unique<ui::Button>(
        Vec2f{startX + (btnWidth + btnSpacing) * 2, bottomY},
        Vec2f{btnWidth, 40.0f},
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
        case accessibility::GameAction::PushToTalk: return "Push-to-Talk";
        case accessibility::GameAction::WeaponPrev: return "Prev Weapon";
        case accessibility::GameAction::WeaponNext: return "Next Weapon";
        case accessibility::GameAction::OpenChat: return "Open Chat";
        case accessibility::GameAction::ExpandChat: return "Expand Chat";
        case accessibility::GameAction::ForceToggle: return "Force Toggle";
        case accessibility::GameAction::ToggleControls: return "Toggle Help";
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

void SettingsScene::onShipSkinClick(uint8_t skinId)
{
    _shipSkin = skinId;
    _hasUnsavedChanges = true;

    // Update button colors to highlight selected skin
    for (size_t i = 0; i < SHIP_SKIN_COUNT; ++i) {
        if (_shipSkinBtns[i]) {
            if ((i + 1) == skinId) {
                _shipSkinBtns[i]->setNormalColor({80, 150, 80, 255});
                _shipSkinBtns[i]->setHoveredColor({100, 180, 100, 255});
            } else {
                _shipSkinBtns[i]->setNormalColor({50, 50, 70, 255});
                _shipSkinBtns[i]->setHoveredColor({70, 70, 100, 255});
            }
        }
    }
}

void SettingsScene::onVoiceModeClick()
{
    _voiceMode = (_voiceMode == 0) ? 1 : 0;
    _voiceModeBtn->setText(_voiceMode == 0 ? "PUSH-TO-TALK" : "VOICE ACTIVITY");
    _hasUnsavedChanges = true;
}

void SettingsScene::onInputDeviceClick()
{
    auto logger = client::logging::Logger::getSceneLogger();

    // Cycle through available input devices
    _inputDeviceIndex = (_inputDeviceIndex + 1) % static_cast<int>(_inputDeviceNames.size());
    std::string label = truncateDeviceName(_inputDeviceNames[_inputDeviceIndex], 30);
    _inputDeviceBtn->setText(label);

    // Update selected device (empty string = auto for index 0)
    _selectedInputDevice = (_inputDeviceIndex == 0) ? "" : _inputDeviceNames[_inputDeviceIndex];
    logger->info("Input device changed to index {}: '{}' (stored as '{}')",
                 _inputDeviceIndex, _inputDeviceNames[_inputDeviceIndex], _selectedInputDevice);
    _hasUnsavedChanges = true;
}

void SettingsScene::onOutputDeviceClick()
{
    auto logger = client::logging::Logger::getSceneLogger();

    // Cycle through available output devices
    _outputDeviceIndex = (_outputDeviceIndex + 1) % static_cast<int>(_outputDeviceNames.size());
    std::string label = truncateDeviceName(_outputDeviceNames[_outputDeviceIndex], 30);
    _outputDeviceBtn->setText(label);

    // Update selected device (empty string = auto for index 0)
    _selectedOutputDevice = (_outputDeviceIndex == 0) ? "" : _outputDeviceNames[_outputDeviceIndex];
    logger->info("Output device changed to index {}: '{}' (stored as '{}')",
                 _outputDeviceIndex, _outputDeviceNames[_outputDeviceIndex], _selectedOutputDevice);
    _hasUnsavedChanges = true;
}

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
    auto logger = client::logging::Logger::getSceneLogger();
    logger->info("Reset to defaults button clicked");

    // Reset to default key bindings
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveUp)] = {events::Key::Up, events::Key::Z};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveDown)] = {events::Key::Down, events::Key::S};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveLeft)] = {events::Key::Left, events::Key::A};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::MoveRight)] = {events::Key::Right, events::Key::D};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::Shoot)] = {events::Key::Space, events::Key::Enter};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::Pause)] = {events::Key::Escape, events::Key::P};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::PushToTalk)] = {events::Key::V, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::WeaponPrev)] = {events::Key::Q, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::WeaponNext)] = {events::Key::E, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::OpenChat)] = {events::Key::T, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::ExpandChat)] = {events::Key::O, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::ForceToggle)] = {events::Key::F, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(accessibility::GameAction::ToggleControls)] = {events::Key::H, events::Key::Unknown};
    updateAllKeyBindButtonTexts();

    // Reset voice settings to defaults
    _voiceMode = 0;  // PTT
    _vadThreshold = 5;  // 5% (higher to avoid keyboard noise)
    _micGain = 100;  // 100%
    _voiceVolume = 100;  // 100%
    if (_voiceModeBtn) {
        _voiceModeBtn->setText("PUSH-TO-TALK");
    }

    // Reset audio devices to Auto (index 0)
    _inputDeviceIndex = 0;
    _outputDeviceIndex = 0;
    _selectedInputDevice = "";  // Empty = auto
    _selectedOutputDevice = "";  // Empty = auto
    if (_inputDeviceBtn && !_inputDeviceNames.empty()) {
        _inputDeviceBtn->setText(truncateDeviceName(_inputDeviceNames[0], 30));
    }
    if (_outputDeviceBtn && !_outputDeviceNames.empty()) {
        _outputDeviceBtn->setText(truncateDeviceName(_outputDeviceNames[0], 30));
    }

    // Reset color blind mode
    _colorBlindMode = accessibility::ColorBlindMode::None;
    if (_colorBlindModeBtn) {
        _colorBlindModeBtn->setText(getColorBlindModeDisplayName(_colorBlindMode));
    }

    // Reset ship skin to default (1)
    _shipSkin = 1;
    for (size_t i = 0; i < SHIP_SKIN_COUNT; ++i) {
        if (_shipSkinBtns[i]) {
            if (i == 0) {  // Ship 1
                _shipSkinBtns[i]->setNormalColor({80, 150, 80, 255});
                _shipSkinBtns[i]->setHoveredColor({100, 180, 100, 255});
            } else {
                _shipSkinBtns[i]->setNormalColor({50, 50, 70, 255});
                _shipSkinBtns[i]->setHoveredColor({70, 70, 100, 255});
            }
        }
    }

    _hasUnsavedChanges = true;
    showInfo("All settings reset to defaults");
}

void SettingsScene::onApplyClick()
{
    auto logger = client::logging::Logger::getSceneLogger();
    logger->info("Apply clicked - saving input='{}' output='{}'", _selectedInputDevice, _selectedOutputDevice);

    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Apply settings to singleton
    config.setColorBlindMode(_colorBlindMode);
    config.setShipSkin(_shipSkin);

    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);
        config.setKeyBinding(action, _keyBindings[i][0], _keyBindings[i][1]);
    }

    // Apply voice settings to VoiceChatManager
    auto& voice = audio::VoiceChatManager::getInstance();
    voice.applySettings(_voiceMode, _vadThreshold, _micGain, _voiceVolume);

    // Apply audio device selection
    voice.applyAudioDevices(_selectedInputDevice, _selectedOutputDevice);

    // Send to server if connected
    if (_context.tcpClient && _context.tcpClient->isConnected() && _context.tcpClient->isAuthenticated()) {
        UserSettingsPayload payload;
        std::strncpy(payload.colorBlindMode,
            accessibility::AccessibilityConfig::colorBlindModeToString(_colorBlindMode).c_str(),
            COLORBLIND_MODE_LEN - 1);
        payload.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
        payload.gameSpeedPercent = 100;

        for (size_t i = 0; i < ACTION_COUNT; ++i) {
            payload.keyBindings[i * 2] = static_cast<uint8_t>(_keyBindings[i][0]);
            payload.keyBindings[i * 2 + 1] = static_cast<uint8_t>(_keyBindings[i][1]);
        }

        payload.shipSkin = _shipSkin;
        payload.voiceMode = _voiceMode;
        payload.vadThreshold = _vadThreshold;
        payload.micGain = _micGain;
        payload.voiceVolume = _voiceVolume;

        // Audio device names
        std::strncpy(payload.audioInputDevice, _selectedInputDevice.c_str(), AUDIO_DEVICE_NAME_LEN - 1);
        payload.audioInputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';
        std::strncpy(payload.audioOutputDevice, _selectedOutputDevice.c_str(), AUDIO_DEVICE_NAME_LEN - 1);
        payload.audioOutputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';

        // Chat settings
        payload.keepChatOpenAfterSend = config.getKeepChatOpenAfterSend() ? 1 : 0;

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
                    _shipSkin = event.shipSkin;
                    // Game speed ignored - now per-room (configured in LobbyScene by host)

                    for (size_t i = 0; i < ACTION_COUNT; ++i) {
                        _keyBindings[i][0] = static_cast<events::Key>(event.keyBindings[i * 2]);
                        _keyBindings[i][1] = static_cast<events::Key>(event.keyBindings[i * 2 + 1]);
                    }

                    // Voice settings
                    _voiceMode = event.voiceMode;
                    _vadThreshold = event.vadThreshold;
                    _micGain = event.micGain;
                    _voiceVolume = event.voiceVolume;

                    // Audio device selection
                    _selectedInputDevice = event.audioInputDevice;
                    _selectedOutputDevice = event.audioOutputDevice;

                    // Update UI
                    if (_colorBlindModeBtn) {
                        _colorBlindModeBtn->setText(getColorBlindModeDisplayName(_colorBlindMode));
                    }
                    // Update ship skin buttons
                    for (size_t i = 0; i < SHIP_SKIN_COUNT; ++i) {
                        if (_shipSkinBtns[i]) {
                            if ((i + 1) == _shipSkin) {
                                _shipSkinBtns[i]->setNormalColor({80, 150, 80, 255});
                                _shipSkinBtns[i]->setHoveredColor({100, 180, 100, 255});
                            } else {
                                _shipSkinBtns[i]->setNormalColor({50, 50, 70, 255});
                                _shipSkinBtns[i]->setHoveredColor({70, 70, 100, 255});
                            }
                        }
                    }
                    updateAllKeyBindButtonTexts();

                    // Update voice mode button
                    if (_voiceModeBtn) {
                        _voiceModeBtn->setText(_voiceMode == 0 ? "PUSH-TO-TALK" : "VOICE ACTIVITY");
                    }

                    // Update audio device buttons - find matching index
                    _inputDeviceIndex = 0;  // Default to auto
                    for (size_t i = 0; i < _inputDeviceNames.size(); ++i) {
                        if (_inputDeviceNames[i] == _selectedInputDevice) {
                            _inputDeviceIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    if (_inputDeviceBtn) {
                        _inputDeviceBtn->setText(truncateDeviceName(_inputDeviceNames[_inputDeviceIndex], 30));
                    }

                    _outputDeviceIndex = 0;  // Default to auto
                    for (size_t i = 0; i < _outputDeviceNames.size(); ++i) {
                        if (_outputDeviceNames[i] == _selectedOutputDevice) {
                            _outputDeviceIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    if (_outputDeviceBtn) {
                        _outputDeviceBtn->setText(truncateDeviceName(_outputDeviceNames[_outputDeviceIndex], 30));
                    }

                    // Chat settings - apply directly to AccessibilityConfig
                    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
                    accessConfig.setKeepChatOpenAfterSend(event.keepChatOpenAfterSend);

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

    // Voice chat buttons
    _voiceModeBtn->handleEvent(event);
    _vadThresholdMinusBtn->handleEvent(event);
    _vadThresholdPlusBtn->handleEvent(event);
    _micGainMinusBtn->handleEvent(event);
    _micGainPlusBtn->handleEvent(event);
    _voiceVolumeMinusBtn->handleEvent(event);
    _voiceVolumePlusBtn->handleEvent(event);

    // Audio device buttons
    _inputDeviceBtn->handleEvent(event);
    _outputDeviceBtn->handleEvent(event);

    // Ship skin buttons
    for (auto& btn : _shipSkinBtns) {
        if (btn) btn->handleEvent(event);
    }

    for (auto& bindBtn : _keyBindButtons) {
        bindBtn.primary->handleEvent(event);
        bindBtn.secondary->handleEvent(event);
    }

    // Debug: Log mouse clicks to check if Reset button receives them
    if (auto* released = std::get_if<events::MouseButtonReleased>(&event)) {
        auto evtLogger = client::logging::Logger::getSceneLogger();
        auto resetPos = _resetBindingsBtn->getPos();
        auto resetSize = _resetBindingsBtn->getSize();
        float mx = static_cast<float>(released->x);
        float my = static_cast<float>(released->y);
        bool inX = (mx >= resetPos.x) && (mx <= resetPos.x + resetSize.x);
        bool inY = (my >= resetPos.y) && (my <= resetPos.y + resetSize.y);
        evtLogger->info("Mouse at ({},{}) - Reset btn [{},{}] to [{},{}] - inX={} inY={} contains={}",
                        mx, my, resetPos.x, resetPos.y,
                        resetPos.x + resetSize.x, resetPos.y + resetSize.y,
                        inX, inY, _resetBindingsBtn->contains(mx, my));
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

    // Voice chat buttons
    _voiceModeBtn->update(deltaTime);
    _vadThresholdMinusBtn->update(deltaTime);
    _vadThresholdPlusBtn->update(deltaTime);
    _micGainMinusBtn->update(deltaTime);
    _micGainPlusBtn->update(deltaTime);
    _voiceVolumeMinusBtn->update(deltaTime);
    _voiceVolumePlusBtn->update(deltaTime);

    // Audio device buttons
    _inputDeviceBtn->update(deltaTime);
    _outputDeviceBtn->update(deltaTime);

    // Ship skin buttons
    for (auto& btn : _shipSkinBtns) {
        if (btn) btn->update(deltaTime);
    }

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
    float accessBoxY = 100.0f;
    float accessBoxHeight = 440.0f;  // Increased for audio device selection

    _context.window->drawRect(BOX_MARGIN_X, accessBoxY, BOX_WIDTH, accessBoxHeight, {20, 20, 40, 200});
    _context.window->drawRect(BOX_MARGIN_X, accessBoxY, BOX_WIDTH, 3, {60, 80, 120, 255});

    _context.window->drawText(FONT_KEY, "ACCESSIBILITY & VOICE",
        BOX_MARGIN_X + 20, accessBoxY + 15, 24, {150, 150, 180, 255});

    // Color Blind Mode
    _context.window->drawText(FONT_KEY, "Color Blind Mode:",
        LABEL_X, 180, 18, {200, 200, 220, 255});
    _colorBlindModeBtn->render(*_context.window);

    // Voice Mode
    _context.window->drawText(FONT_KEY, "Voice Mode:",
        LABEL_X, 240, 18, {200, 200, 220, 255});
    _voiceModeBtn->render(*_context.window);

    // VAD Threshold
    _context.window->drawText(FONT_KEY, "VAD Sensitivity:",
        LABEL_X, 290, 18, {200, 200, 220, 255});
    _vadThresholdMinusBtn->render(*_context.window);
    _context.window->drawText(FONT_KEY, std::to_string(_vadThreshold) + "%",
        CONTROL_X + 55, 297, 16, {255, 255, 255, 255});
    _vadThresholdPlusBtn->render(*_context.window);

    // Mic Gain
    _context.window->drawText(FONT_KEY, "Mic Gain:",
        LABEL_X, 340, 18, {200, 200, 220, 255});
    _micGainMinusBtn->render(*_context.window);
    _context.window->drawText(FONT_KEY, std::to_string(_micGain) + "%",
        CONTROL_X + 55, 347, 16, {255, 255, 255, 255});
    _micGainPlusBtn->render(*_context.window);

    // Voice Volume
    _context.window->drawText(FONT_KEY, "Voice Volume:",
        LABEL_X, 390, 18, {200, 200, 220, 255});
    _voiceVolumeMinusBtn->render(*_context.window);
    _context.window->drawText(FONT_KEY, std::to_string(_voiceVolume) + "%",
        CONTROL_X + 55, 397, 16, {255, 255, 255, 255});
    _voiceVolumePlusBtn->render(*_context.window);

    // Input Device
    _context.window->drawText(FONT_KEY, "Input Device:",
        LABEL_X, 440, 18, {200, 200, 220, 255});
    _inputDeviceBtn->render(*_context.window);

    // Output Device
    _context.window->drawText(FONT_KEY, "Output Device:",
        LABEL_X, 490, 18, {200, 200, 220, 255});
    _outputDeviceBtn->render(*_context.window);

    // === SHIP SELECTION SECTION ===
    float shipBoxY = 550.0f;
    float shipBoxHeight = 90.0f;

    _context.window->drawRect(BOX_MARGIN_X, shipBoxY, BOX_WIDTH, shipBoxHeight, {20, 20, 40, 200});
    _context.window->drawRect(BOX_MARGIN_X, shipBoxY, BOX_WIDTH, 3, {60, 80, 120, 255});

    _context.window->drawText(FONT_KEY, "SHIP",
        BOX_MARGIN_X + 20, shipBoxY + 10, 20, {150, 150, 180, 255});

    _context.window->drawText(FONT_KEY, "Ship Skin:",
        LABEL_X, 575, 16, {200, 200, 220, 255});

    float shipBtnSize = 60.0f;
    float shipSpacing = 20.0f;
    float shipStartX = CONTROL_X;
    for (size_t i = 0; i < SHIP_SKIN_COUNT; ++i) {
        if (_shipSkinBtns[i]) {
            _shipSkinBtns[i]->render(*_context.window);
            std::string textureKey = "settings_ship" + std::to_string(i + 1);
            float spriteX = shipStartX + i * (shipBtnSize + shipSpacing) + 6;
            float spriteY = 570.0f + 6;
            _context.window->drawSprite(textureKey, spriteX, spriteY, 48.0f, 48.0f);
        }
    }

    // === CONTROLS SECTION (2 columns layout) ===
    float controlsBoxY = 650.0f;
    float controlsBoxHeight = 260.0f;  // 7 rows + header + bottom buttons (2 columns)

    // Section box
    _context.window->drawRect(BOX_MARGIN_X, controlsBoxY, BOX_WIDTH, controlsBoxHeight, {20, 20, 40, 200});
    _context.window->drawRect(BOX_MARGIN_X, controlsBoxY, BOX_WIDTH, 3, {60, 80, 120, 255});

    // Section title
    _context.window->drawText(FONT_KEY, "CONTROLS",
        BOX_MARGIN_X + 20, controlsBoxY + 8, 18, {150, 150, 180, 255});

    // Column 1 positions
    float col1LabelX = 80.0f;
    float col1PrimaryX = 200.0f;
    float keyBtnWidth = 80.0f;

    // Column 2 positions
    float col2LabelX = SCREEN_WIDTH / 2 + 40.0f;
    float col2PrimaryX = SCREEN_WIDTH / 2 + 160.0f;

    // Key bindings (2 columns: 7 left, 6 right for 13 actions)
    float controlsStartY = 670.0f;
    float controlsRowHeight = 28.0f;
    for (size_t i = 0; i < ACTION_COUNT; ++i) {
        auto action = static_cast<accessibility::GameAction>(i);

        // Determine column and row (7 left, 6 right)
        bool isLeftColumn = (i < 7);
        size_t rowIndex = isLeftColumn ? i : (i - 7);
        float rowY = controlsStartY + rowIndex * controlsRowHeight;
        float labelX = isLeftColumn ? col1LabelX : col2LabelX;

        _context.window->drawText(FONT_KEY, getActionDisplayName(action),
            labelX, rowY + 4, 12, {200, 200, 220, 255});

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
