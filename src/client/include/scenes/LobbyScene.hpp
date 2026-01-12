/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LobbyScene - Room lobby waiting for game start
*/

#ifndef LOBBYSCENE_HPP_
#define LOBBYSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/TextInput.hpp"
#include "ui/StarfieldBackground.hpp"
#include "network/NetworkEvents.hpp"
#include "../utils/Vecs.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

class LobbyScene : public IScene
{
public:
    LobbyScene(const std::string& roomName, const std::string& roomCode,
               uint8_t maxPlayers, bool isHost, uint8_t slotId,
               const std::vector<client::network::RoomPlayerInfo>& initialPlayers = {});
    ~LobbyScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();
    void processTCPEvents();
    void processUDPEvents();

    void onReadyClick();
    void onStartClick();
    void onLeaveClick();
    void onKickClick(const std::string& email);
    void onSendChatClick();
    void onSpeedDecreaseClick();
    void onSpeedIncreaseClick();

    void showError(const std::string& message);
    void showInfo(const std::string& message);

    // Chat rendering
    void renderChatPanel();
    void appendChatMessage(const client::network::ChatMessageInfo& msg);

    // Returns true if mouse is hovering over kick area for this player
    bool isKickButtonHovered(float playerY) const;

    void transitionToGame();

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Room state
    std::string _roomName;
    std::string _roomCode;
    uint8_t _maxPlayers;
    uint8_t _slotId;
    bool _isHost;
    bool _isReady = false;
    std::vector<client::network::RoomPlayerInfo> _players;
    uint16_t _roomGameSpeedPercent = 100;  // 50-200, host configurable

    // Countdown state
    std::optional<uint8_t> _countdown;
    float _countdownAnimTimer = 0.0f;

    // UI Components
    std::unique_ptr<ui::Button> _readyButton;
    std::unique_ptr<ui::Button> _startButton;
    std::unique_ptr<ui::Button> _leaveButton;

    // Game speed UI (host only)
    std::unique_ptr<ui::Button> _speedDecBtn;
    std::unique_ptr<ui::Button> _speedIncBtn;

    // Chat UI Components
    std::unique_ptr<ui::TextInput> _chatInput;
    std::unique_ptr<ui::Button> _sendChatButton;
    std::vector<client::network::ChatMessageInfo> _chatMessages;
    float _chatScrollOffset = 0.0f;
    static constexpr size_t MAX_VISIBLE_CHAT_MESSAGES = 8;
    static constexpr size_t MAX_CHAT_MESSAGES = 50;

    // Status message
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Transition state
    bool _transitioningToGame = false;

    // Kick state (when we get kicked from the room)
    bool _wasKicked = false;
    std::string _kickReason;
    void renderKickedScreen();

    // Mouse state for kick buttons
    float _mouseX = 0.0f;
    float _mouseY = 0.0f;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "lobby_font";
    static constexpr float STATUS_DISPLAY_DURATION = 3.0f;
    static constexpr int STAR_COUNT = 100;
    static constexpr size_t SHIP_SKIN_COUNT = 6;  // Number of ship skins available
    static constexpr float SHIP_PREVIEW_SIZE = 40.0f;  // Size of ship preview in player list
};

#endif /* !LOBBYSCENE_HPP_ */
