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
#include "audio/VoiceChatManager.hpp"
#include "network/NetworkEvents.hpp"
#include "network/UDPClient.hpp"
#include "ui/TextInput.hpp"
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

    // Constructor with initial chat history from lobby
    GameScene(uint16_t roomGameSpeedPercent,
              const std::vector<client::network::ChatMessageInfo>& initialChatMessages);

    // Constructor with chat history and player names (for team scoreboard)
    GameScene(uint16_t roomGameSpeedPercent,
              const std::vector<client::network::ChatMessageInfo>& initialChatMessages,
              const std::unordered_map<uint8_t, std::string>& playerNames);

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
    void renderVoiceIndicator();
    void renderScoreHUD();  // Score, combo, wave info (Gameplay Phase 2)
    void renderTeamScoreboard();  // All players' scores in real-time (multiplayer)
    void renderWeaponHUD(); // Current weapon indicator (Gameplay Phase 2)
    void renderBoss();      // Boss sprite (Gameplay Phase 2)
    void renderBossHealthBar();  // Boss HP bar (Gameplay Phase 2)
    // R-Type Authentic (Phase 3) rendering
    void renderWaveCannons();    // Wave Cannon beams
    void renderPowerUps();       // Power-up items
    void renderForcePods();      // Force pods
    void renderBitDevices();     // Bit devices (orbiting satellites)
    void renderChargeGauge();    // Wave Cannon charge indicator
    void renderSpeedIndicator(); // Speed upgrade level (below health bar)
    void renderControlsHUD();    // Controls help (bottom-right)
    void loadAssets();
    void initStars();
    void initAudio();
    void initVoiceChat();
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
    bool _voiceChatInitialized = false;

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

    // Enemy texture keys per type (matches EnemyType enum from GameWorld)
    static constexpr const char* ENEMY_TEXTURE_KEY = "enemy";  // Fallback
    static constexpr const char* ENEMY_BASIC_KEY = "enemy_basic";
    static constexpr const char* ENEMY_TRACKER_KEY = "enemy_tracker";
    static constexpr const char* ENEMY_ZIGZAG_KEY = "enemy_zigzag";
    static constexpr const char* ENEMY_FAST_KEY = "enemy_fast";
    static constexpr const char* ENEMY_BOMBER_KEY = "enemy_bomber";
    static constexpr const char* ENEMY_POW_ARMOR_KEY = "enemy_pow_armor";

    // Helper to get texture key based on enemy type
    std::string getEnemyTextureKey(uint8_t enemyType) const;

    // Boss rendering constants (Gameplay Phase 2)
    static constexpr float BOSS_WIDTH = 150.0f;
    static constexpr float BOSS_HEIGHT = 120.0f;
    static constexpr const char* BOSS_TEXTURE_KEY = "boss";

    // R-Type Authentic (Phase 3) constants
    static constexpr float POWERUP_SIZE = 32.0f;
    static constexpr float FORCE_POD_WIDTH = 32.0f;
    static constexpr float FORCE_POD_HEIGHT = 32.0f;  // Match server hitbox
    static constexpr float BIT_DEVICE_SIZE = 24.0f;   // Bit devices are smaller than Force Pod
    static constexpr float WAVECANNON_HEIGHT = 16.0f;  // Base height, actual depends on level
    static constexpr float CHARGE_GAUGE_WIDTH = 100.0f;
    static constexpr float CHARGE_GAUGE_HEIGHT = 8.0f;
    static constexpr float QUICK_TAP_THRESHOLD = 0.15f;  // Max time for quick tap (no charge)

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

    // R-Type Authentic (Phase 3) state
    bool _isCharging = false;           // Is fire button held?
    float _chargeTimer = 0.0f;          // Time spent charging
    uint8_t _clientChargeLevel = 0;     // Client-side predicted charge level (0-3)

    // Controls HUD visibility (toggle with H key by default)
    bool _showControlsHUD = true;       // Show controls help by default

    // Player names for team scoreboard (slotId -> displayName)
    std::unordered_map<uint8_t, std::string> _playerNames;

    // Global rank and best score (fetched from leaderboard, updated periodically)
    uint32_t _globalRank = 0;           // Current rank (0 = not ranked)
    uint32_t _bestScore = 0;            // Personal best score to beat
    float _rankUpdateTimer = 0.0f;      // Timer for periodic rank updates
    bool _globalRankRequested = false;  // True once we request rank for the first time
    bool _playerStatsRequested = false; // True once we request player stats for bestScore
    static constexpr float RANK_UPDATE_INTERVAL = 10.0f;  // Update every 10 seconds
    void renderGlobalRank();            // Display rank badge and best score in HUD
};
#endif /* !GAMESCENE_HPP_ */
