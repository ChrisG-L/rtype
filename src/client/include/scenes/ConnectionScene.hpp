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
#include "core/ConnectionState.hpp"
#include "network/NetworkEvents.hpp"
#include <memory>
#include <string>
#include <chrono>

class ConnectionScene : public IScene
{
public:
    explicit ConnectionScene(core::ConnectionSceneMode mode = core::ConnectionSceneMode::InitialConnection);
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

    bool _assetsLoaded = false;
    core::ConnectionSceneMode _mode;
    core::ConnectionState _connectionState;

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

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr float RETRY_INTERVAL = 4.0f;  // 2 seconds
    static constexpr float DOT_ANIMATION_SPEED = 0.5f;
    static constexpr int STAR_COUNT = 80;
    static constexpr const char* FONT_KEY = "connection_font";
};

#endif /* !CONNECTIONSCENE_HPP_ */
