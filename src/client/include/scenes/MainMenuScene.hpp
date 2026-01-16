/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MainMenuScene
*/

#ifndef MAINMENUSCENE_HPP_
#define MAINMENUSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/TextInput.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include "network/NetworkEvents.hpp"
#include <memory>
#include <string>

class MainMenuScene : public IScene {
public:
    MainMenuScene();
    ~MainMenuScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void initUI();
    void loadAssets();
    void processTCPEvents();

    void onCreateRoomClick();
    void onJoinRoomClick();
    void onBrowseRoomsClick();
    void onQuickJoinClick();
    void onLeaderboardClick();
    void onSettingsClick();
    void onQuitClick();

    // Dialog actions
    void onCreateRoomConfirm();
    void onJoinRoomConfirm();
    void closeDialog();

    void showError(const std::string& message);
    void showInfo(const std::string& message);

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Room operation state
    enum class RoomState {
        Idle,
        WaitingForCreate,
        WaitingForJoin
    };
    RoomState _roomState = RoomState::Idle;

    // Dialog mode
    enum class DialogMode {
        None,
        CreateRoom,
        JoinRoom
    };
    DialogMode _dialogMode = DialogMode::None;

    // Main menu buttons
    std::unique_ptr<ui::Button> _createRoomButton;
    std::unique_ptr<ui::Button> _joinRoomButton;
    std::unique_ptr<ui::Button> _browseRoomsButton;
    std::unique_ptr<ui::Button> _quickJoinButton;
    std::unique_ptr<ui::Button> _leaderboardButton;
    std::unique_ptr<ui::Button> _settingsButton;
    std::unique_ptr<ui::Button> _quitButton;

    // Create room dialog components
    std::unique_ptr<ui::TextInput> _roomNameInput;
    std::unique_ptr<ui::Button> _maxPlayersDecBtn;
    std::unique_ptr<ui::Button> _maxPlayersIncBtn;
    std::unique_ptr<ui::Button> _publicPrivateBtn;
    std::unique_ptr<ui::Button> _createConfirmBtn;
    std::unique_ptr<ui::Button> _cancelBtn;
    uint8_t _maxPlayers = 4;
    bool _isPrivate = false;

    // Join room dialog components
    std::unique_ptr<ui::TextInput> _roomCodeInput;
    std::unique_ptr<ui::Button> _joinConfirmBtn;

    // Status message
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Animated starfield background
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "menu_font";
    static constexpr int STAR_COUNT = 150;
    static constexpr float STATUS_DISPLAY_DURATION = 4.0f;
};

#endif /* !MAINMENUSCENE_HPP_ */
