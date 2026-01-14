/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SettingsScene - Accessibility and control settings
*/

#ifndef SETTINGSSCENE_HPP_
#define SETTINGSSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/StarfieldBackground.hpp"
#include "accessibility/AccessibilityConfig.hpp"
#include "../utils/Vecs.hpp"
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <optional>

class SettingsScene : public IScene
{
public:
    SettingsScene();
    ~SettingsScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();

    // Button callbacks
    void onColorBlindModeClick();
    void onShipSkinClick(uint8_t skinId);
    void onVoiceModeClick();
    void onInputDeviceClick();
    void onOutputDeviceClick();
    void onKeyBindClick(accessibility::GameAction action, bool isPrimary);
    void onResetBindingsClick();
    void onApplyClick();
    void onBackClick();

    // Helpers
    void loadAudioDevices();
    std::string truncateDeviceName(const std::string& name, size_t maxLen = 25) const;

    // Exit confirmation callbacks
    void onConfirmSaveExitClick();
    void onConfirmDiscardClick();

    // Helpers
    void showError(const std::string& message);
    void showInfo(const std::string& message);
    void processTCPEvents();
    std::string getColorBlindModeDisplayName(accessibility::ColorBlindMode mode) const;
    std::string getActionDisplayName(accessibility::GameAction action) const;
    bool isKeyAlreadyUsed(events::Key key, accessibility::GameAction excludeAction, bool excludePrimary) const;
    std::string findConflictingAction(events::Key key, accessibility::GameAction excludeAction, bool excludePrimary) const;
    void updateKeyBindButtonText(accessibility::GameAction action, bool isPrimary);
    void updateAllKeyBindButtonTexts();

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Working copy of settings (applied on Save)
    accessibility::ColorBlindMode _colorBlindMode;
    uint8_t _shipSkin = 1;  // Ship skin variant (1-6)
    static constexpr size_t ACTION_COUNT = static_cast<size_t>(accessibility::GameAction::ActionCount);
    std::array<std::array<events::Key, 2>, ACTION_COUNT> _keyBindings;

    // Voice settings
    uint8_t _voiceMode = 0;       // 0 = PTT, 1 = VAD
    uint8_t _vadThreshold = 2;    // 0-100
    uint8_t _micGain = 100;       // 0-200
    uint8_t _voiceVolume = 100;   // 0-100

    // Audio device selection
    std::string _selectedInputDevice;   // "" = auto
    std::string _selectedOutputDevice;  // "" = auto
    std::vector<std::string> _inputDeviceNames;
    std::vector<std::string> _outputDeviceNames;
    int _inputDeviceIndex = 0;   // Index in _inputDeviceNames (0 = Auto)
    int _outputDeviceIndex = 0;  // Index in _outputDeviceNames (0 = Auto)

    // Key rebinding state machine
    enum class KeyRebindState {
        None,
        Listening
    };
    KeyRebindState _rebindState = KeyRebindState::None;
    std::optional<accessibility::GameAction> _rebindAction;
    bool _rebindIsPrimary = true;
    float _rebindBlinkTimer = 0.0f;

    // Exit confirmation state
    bool _hasUnsavedChanges = false;
    bool _showExitConfirm = false;

    // Section: Accessibility
    std::unique_ptr<ui::Button> _colorBlindModeBtn;

    // Section: Voice Chat
    std::unique_ptr<ui::Button> _voiceModeBtn;
    std::unique_ptr<ui::Button> _vadThresholdMinusBtn;
    std::unique_ptr<ui::Button> _vadThresholdPlusBtn;
    std::unique_ptr<ui::Button> _micGainMinusBtn;
    std::unique_ptr<ui::Button> _micGainPlusBtn;
    std::unique_ptr<ui::Button> _voiceVolumeMinusBtn;
    std::unique_ptr<ui::Button> _voiceVolumePlusBtn;

    // Section: Audio Device Selection
    std::unique_ptr<ui::Button> _inputDeviceBtn;
    std::unique_ptr<ui::Button> _outputDeviceBtn;

    // Section: Ship Selection
    static constexpr size_t SHIP_SKIN_COUNT = 6;
    std::array<std::unique_ptr<ui::Button>, SHIP_SKIN_COUNT> _shipSkinBtns;

    // Section: Key Bindings (6 actions x 2 keys)
    struct KeyBindButtons {
        std::unique_ptr<ui::Button> primary;
        std::unique_ptr<ui::Button> secondary;
    };
    std::array<KeyBindButtons, ACTION_COUNT> _keyBindButtons;
    std::unique_ptr<ui::Button> _resetBindingsBtn;

    // Bottom buttons
    std::unique_ptr<ui::Button> _applyBtn;
    std::unique_ptr<ui::Button> _backBtn;

    // Exit confirmation dialog buttons
    std::unique_ptr<ui::Button> _confirmSaveExitBtn;
    std::unique_ptr<ui::Button> _confirmDiscardBtn;

    // Status message
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Constants
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "settings_font";
    static constexpr float STATUS_DISPLAY_DURATION = 3.0f;
    static constexpr int STAR_COUNT = 100;

    // Layout constants
    static constexpr float BOX_MARGIN_X = 60.0f;
    static constexpr float BOX_WIDTH = SCREEN_WIDTH - 2 * BOX_MARGIN_X;
    static constexpr float LABEL_X = 120.0f;
    static constexpr float CONTROL_X = 400.0f;
    static constexpr float KEY_BTN_WIDTH = 120.0f;
    static constexpr float KEY_BTN_HEIGHT = 45.0f;
    static constexpr float KEY_BTN_SPACING = 20.0f;
    static constexpr float ROW_HEIGHT = 60.0f;
};

#endif /* !SETTINGSSCENE_HPP_ */
