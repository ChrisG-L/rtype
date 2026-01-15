/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ConnectionScene - Handles connection/reconnection with animated UI
*/

#ifndef CONNECTIONSCENE_HPP_
#define CONNECTIONSCENE_HPP_

#include "IScene.hpp"
#include "ui/StarfieldBackground.hpp"
#include "ui/TextInput.hpp"
#include "ui/Button.hpp"
#include "core/ConnectionState.hpp"
#include "network/NetworkEvents.hpp"
#include <memory>
#include <string>
#include <chrono>

class ConnectionScene : public IScene
{
public:
    explicit ConnectionScene(core::ConnectionSceneMode mode = core::ConnectionSceneMode::InitialConnection,
                             bool forceReconnect = false);
    ~ConnectionScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

    // Check if reconnection is complete
    bool isReconnected() const;

private:
    void loadAssets();
    void attemptReconnection();
    void processNetworkEvents();
    void updateConnectionState();
    void attemptAutoRelogin();
    std::string getModeTitle() const;

    // Server config UI
    void initServerConfigUI();
    void showServerConfigUI();
    void hideServerConfigUI();
    void applyServerConfig();
    void renderServerConfigUI();

    bool _assetsLoaded = false;
    core::ConnectionSceneMode _mode;
    core::ConnectionState _connectionState;
    bool _forceReconnect = false;
    bool _disconnectPending = false;

    // Auto-relogin state (Reconnection mode only)
    enum class ReloginState {
        NotStarted,
        WaitingForTCPConnect,
        SendingLogin,
        WaitingForAuthResponse,
        Authenticated,
        Failed
    };
    ReloginState _reloginState = ReloginState::NotStarted;

    // Retry timer
    float _retryTimer = 0.0f;
    int _retryCount = 0;
    bool _isRetrying = false;

    // Animation
    float _animationTimer = 0.0f;
    int _dotCount = 0;
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Spinner animation
    float _spinnerAngle = 0.0f;

    // Status message
    std::string _statusMessage;

    // Server config UI (shown after failed connection attempts)
    bool _showingConfigUI = false;
    std::unique_ptr<ui::TextInput> _hostInput;
    std::unique_ptr<ui::TextInput> _tcpPortInput;
    std::unique_ptr<ui::TextInput> _udpPortInput;
    std::unique_ptr<ui::Button> _connectButton;
    std::unique_ptr<ui::Button> _cancelButton;
    std::unique_ptr<ui::Button> _franceButton;
    std::unique_ptr<ui::Button> _localButton;
    bool _configUIInitialized = false;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr float RETRY_INTERVAL = 4.0f;
    static constexpr float DOT_ANIMATION_SPEED = 0.5f;
    static constexpr int STAR_COUNT = 80;
    static constexpr int MAX_RETRIES_BEFORE_CONFIG = 3;
    static constexpr const char* FONT_KEY = "connection_font";
};

#endif /* !CONNECTIONSCENE_HPP_ */
