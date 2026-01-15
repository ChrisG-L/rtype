/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LoginScene
*/

#ifndef LOGINSCENE_HPP_
#define LOGINSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/TextInput.hpp"
#include "ui/StarfieldBackground.hpp"
#include "ui/ServerConfigPanel.hpp"
#include "../utils/Vecs.hpp"
#include <memory>
#include <vector>

class LoginScene : public IScene {
public:
    LoginScene();
    ~LoginScene() override;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void initUI();
    void loadAssets();
    void onLoginClick();
    void onRegisterClick();
    void switchToRegisterMode();
    void switchToLoginMode();
    void showError(const std::string& message);
    void showSuccess(const std::string& message);
    void setupTCPCallbacks();
    void processTCPEvents();

    // Server config UI
    void showServerConfigUI();
    void hideServerConfigUI();

    bool _assetsLoaded = false;
    bool _uiInitialized = false;
    bool _isRegisterMode = false;
    bool _callbacksSetup = false;
    bool _showingConfigUI = false;

    // UI Components
    std::unique_ptr<ui::TextInput> _usernameInput;
    std::unique_ptr<ui::TextInput> _passwordInput;
    std::unique_ptr<ui::TextInput> _emailInput;
    std::unique_ptr<ui::TextInput> _confirmPasswordInput;
    std::unique_ptr<ui::Button> _submitButton;
    std::unique_ptr<ui::Button> _switchModeButton;
    std::unique_ptr<ui::Button> _serverConfigButton;

    // Server config panel
    std::unique_ptr<ui::ServerConfigPanel> _configPanel;

    std::string _statusMessage;
    rgba _statusColor{255, 100, 100, 255};
    float _statusDisplayTimer = 0.0f;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "login_font";
    static constexpr const char* BG_TEXTURE_KEY = "login_bg";
    static constexpr float STATUS_DISPLAY_DURATION = 5.0f;
    static constexpr int STAR_COUNT = 100;
};

#endif /* !LOGINSCENE_HPP_ */
