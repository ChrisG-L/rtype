/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LeaderboardScene - Global leaderboards, stats, and achievements
*/

#ifndef LEADERBOARDSCENE_HPP_
#define LEADERBOARDSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include "Protocol.hpp"
#include <memory>
#include <string>
#include <vector>
#include <array>

class LeaderboardScene : public IScene {
public:
    LeaderboardScene();
    ~LeaderboardScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void loadAssets();
    void initUI();
    void processTCPEvents();

    // Tab switching
    void onLeaderboardTabClick();
    void onStatsTabClick();
    void onAchievementsTabClick();
    void onBackClick();

    // Leaderboard period switching
    void onAllTimeClick();
    void onWeeklyClick();
    void onMonthlyClick();

    // Request data from server
    void requestLeaderboard(uint8_t period);
    void requestPlayerStats();
    void requestAchievements();

    // Parse server responses
    void onLeaderboardDataReceived(const LeaderboardDataResponse& data);
    void onPlayerStatsReceived(const PlayerStatsWire& stats);
    void onAchievementsReceived(uint32_t bitfield);

    // Render helpers
    void renderLeaderboardTab();
    void renderStatsTab();
    void renderAchievementsTab();
    void renderLoadingIndicator();
    std::string formatDuration(uint32_t seconds) const;
    std::string formatNumber(uint64_t number) const;
    std::string getAchievementName(uint8_t type) const;
    std::string getAchievementDescription(uint8_t type) const;
    rgba getAchievementColor(uint8_t type, bool unlocked) const;

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    // Current tab
    enum class Tab {
        Leaderboard,
        Stats,
        Achievements
    };
    Tab _currentTab = Tab::Leaderboard;

    // Leaderboard data
    struct LeaderboardEntryData {
        uint32_t rank;
        std::string playerName;
        uint32_t score;
        uint16_t wave;
        uint16_t kills;
        uint32_t duration;
    };
    std::vector<LeaderboardEntryData> _leaderboardEntries;
    uint8_t _currentPeriod = 0;  // 0 = AllTime, 1 = Weekly, 2 = Monthly
    uint32_t _yourRank = 0;
    bool _leaderboardLoading = false;

    // Player stats data
    struct PlayerStatsData {
        std::string playerName;
        uint64_t totalScore;
        uint32_t totalKills;
        uint32_t totalDeaths;
        uint32_t totalPlaytime;
        uint32_t gamesPlayed;
        uint32_t bestScore;
        uint16_t bestWave;
        uint16_t bestCombo;
        uint16_t bestKillStreak;
        uint16_t bossKills;
        uint32_t standardKills;
        uint32_t spreadKills;
        uint32_t laserKills;
        uint32_t missileKills;
        float kdRatio;
        uint8_t favoriteWeapon;
    };
    PlayerStatsData _playerStats{};
    bool _statsLoading = false;
    bool _statsReceived = false;

    // Achievements data
    uint32_t _achievementsBitfield = 0;
    bool _achievementsLoading = false;
    bool _achievementsReceived = false;

    // Tab buttons
    std::unique_ptr<ui::Button> _leaderboardTabBtn;
    std::unique_ptr<ui::Button> _statsTabBtn;
    std::unique_ptr<ui::Button> _achievementsTabBtn;

    // Leaderboard period buttons
    std::unique_ptr<ui::Button> _allTimeBtn;
    std::unique_ptr<ui::Button> _weeklyBtn;
    std::unique_ptr<ui::Button> _monthlyBtn;

    // Back button
    std::unique_ptr<ui::Button> _backBtn;

    // Scroll offset for leaderboard
    int _scrollOffset = 0;
    static constexpr int VISIBLE_ENTRIES = 15;
    static constexpr int MAX_ENTRIES = 50;

    // Animated starfield
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Animation
    float _loadingDots = 0.0f;

    // Constants
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "leaderboard_font";
    static constexpr int STAR_COUNT = 100;

    // Layout
    static constexpr float TAB_Y = 80.0f;
    static constexpr float TAB_BTN_WIDTH = 200.0f;
    static constexpr float TAB_BTN_HEIGHT = 50.0f;
    static constexpr float CONTENT_START_Y = 180.0f;
    static constexpr float ROW_HEIGHT = 50.0f;
    static constexpr float MARGIN_X = 100.0f;

    // Achievement constants
    static constexpr size_t ACHIEVEMENT_COUNT = 10;
};

#endif /* !LEADERBOARDSCENE_HPP_ */
