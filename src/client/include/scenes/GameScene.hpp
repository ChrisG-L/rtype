/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#ifndef GAMESCENE_HPP_
#define GAMESCENE_HPP_

#include "scenes/IScene.hpp"
#include "events/Event.hpp"
#include "graphics/IWindow.hpp"
#include "accessibility/AccessibilityConfig.hpp"
#include "audio/AudioManager.hpp"
#include "network/NetworkEvents.hpp"
#include "network/UDPClient.hpp"
#include "ui/TextInput.hpp"
#include <iostream>
#include <unordered_set>
#include <vector>
#include <deque>
#include <random>
#include <memory>

class GameScene : public IScene
{
public:
    // Constructor accepts room game speed percent (50-200, default 100)
    explicit GameScene(uint16_t roomGameSpeedPercent = 100);
    ~GameScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltatime) override;
    void render() override;

private:
    // Room game speed (replaces per-player AccessibilityConfig gameSpeed)
    float _roomGameSpeedMultiplier = 1.0f;
    void renderBackground();
    void renderHUD();
    void renderPlayers();
    void renderMissiles();
    void renderEnemies();
    void renderEnemyMissiles();
    void renderDeathScreen();
    void renderKickedScreen();
    void renderChatOverlay();
    void loadAssets();
    void initStars();
    void initAudio();
    void initChatUI();
    void processUDPEvents();
    void processTCPEvents();
    void onSendChatMessage();
    void appendChatMessage(const client::network::ChatMessageInfo& msg);

    // Client-side prediction
    void applyInputToPosition(float& x, float& y, uint16_t keys, float dt);
    void reconcileWithServer(const client::network::NetworkPlayer& serverState);

    uint16_t _localX = 100;
    uint16_t _localY = 300;
    std::unordered_set<events::Key> _keysPressed;

    // Client-side prediction state
    struct PendingInput {
        uint16_t sequenceNum;
        uint16_t keys;
        float deltaTime;
    };
    std::deque<PendingInput> _pendingInputs;
    float _predictedX = 100.0f;
    float _predictedY = 300.0f;
    uint16_t _nextInputSeq = 0;

    float _shootCooldown = 0.0f;
    bool _assetsLoaded = false;
    bool _starsInitialized = false;
    bool _audioInitialized = false;

    // Kick state (set when player is kicked via TCP)
    bool _wasKicked = false;
    std::string _kickReason;

    struct Star {
        float x, y;
        float speed;
        float size;
        uint8_t brightness;
    };
    std::vector<Star> _stars;

    static constexpr float MOVE_SPEED = 200.0f;
    static constexpr float SHIP_WIDTH = 64.0f;
    static constexpr float SHIP_HEIGHT = 30.0f;
    static constexpr float MISSILE_WIDTH = 32.0f;
    static constexpr float MISSILE_HEIGHT = 16.0f;

    static constexpr float HUD_HEALTH_BAR_WIDTH = 200.0f;
    static constexpr float HUD_HEALTH_BAR_HEIGHT = 20.0f;
    static constexpr float HUD_MARGIN = 20.0f;
    static constexpr uint8_t MAX_HEALTH = 100;

    static constexpr float SHOOT_COOLDOWN_TIME = 0.3f;
    static constexpr const char* SHIP_TEXTURE_KEY = "ship";
    static constexpr const char* MISSILE_TEXTURE_KEY = "missile";
    static constexpr const char* FONT_KEY = "main";
    static constexpr unsigned int LABEL_FONT_SIZE = 14;

    static constexpr float ENEMY_WIDTH = 40.0f;
    static constexpr float ENEMY_HEIGHT = 40.0f;
    static constexpr float ENEMY_MISSILE_WIDTH = 24.0f;
    static constexpr float ENEMY_MISSILE_HEIGHT = 12.0f;
    static constexpr const char* ENEMY_TEXTURE_KEY = "enemy";

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr size_t STAR_COUNT = 150;
    static constexpr float STAR_MIN_SPEED = 20.0f;
    static constexpr float STAR_MAX_SPEED = 150.0f;

    // Chat overlay (Phase 2)
    struct ChatDisplayMessage {
        std::string displayName;
        std::string message;
        float displayTime;  // Time remaining to display (0 = expired/archived)
        bool expired = false;  // True when timer reached 0, message goes to collapsed list
    };
    std::vector<ChatDisplayMessage> _chatDisplayMessages;
    std::unique_ptr<ui::TextInput> _chatInput;
    bool _chatInputOpen = false;
    bool _chatUIInitialized = false;
    bool _skipNextTextEntered = false;  // Skip first TextEntered after opening chat (avoid 'T' in input)
    bool _chatExpanded = false;  // Show all messages (including expired) when expanded
    static constexpr float CHAT_MESSAGE_DISPLAY_TIME = 8.0f;
    static constexpr size_t MAX_CHAT_DISPLAY_MESSAGES = 20;  // Keep more history
    static constexpr size_t ALWAYS_VISIBLE_MESSAGES = 3;  // Last N messages never expire
};
#endif /* !GAMESCENE_HPP_ */
