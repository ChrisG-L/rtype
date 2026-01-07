/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RoomBrowserScene - Browse and join public rooms
*/

#ifndef ROOMBROWSERSCENE_HPP_
#define ROOMBROWSERSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/StarfieldBackground.hpp"
#include "network/NetworkEvents.hpp"
#include "../utils/Vecs.hpp"
#include <memory>
#include <string>
#include <vector>

class RoomBrowserScene : public IScene
{
public:
    RoomBrowserScene();
    ~RoomBrowserScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();
    void processTCPEvents();

    void onRefreshClick();
    void onJoinRoomClick(const std::string& code);
    void onQuickJoinClick();
    void onBackClick();

    void showError(const std::string& message);
    void showInfo(const std::string& message);

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Room list
    std::vector<client::network::RoomBrowserInfo> _rooms;
    int _selectedRoomIndex = -1;
    float _scrollOffset = 0.0f;

    // UI Components
    std::unique_ptr<ui::Button> _refreshButton;
    std::unique_ptr<ui::Button> _joinButton;
    std::unique_ptr<ui::Button> _quickJoinButton;
    std::unique_ptr<ui::Button> _backButton;
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Status
    std::string _statusMessage;
    rgba _statusColor{255, 255, 255, 255};
    float _statusDisplayTimer = 0.0f;

    // Loading state
    bool _isLoading = false;

    // Mouse state for room selection
    float _mouseX = 0.0f;
    float _mouseY = 0.0f;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "browser_font";
    static constexpr float STATUS_DISPLAY_DURATION = 3.0f;
    static constexpr int STAR_COUNT = 100;
    static constexpr float ROOM_ROW_HEIGHT = 50.0f;
    static constexpr int MAX_VISIBLE_ROOMS = 10;
};

#endif /* !ROOMBROWSERSCENE_HPP_ */
