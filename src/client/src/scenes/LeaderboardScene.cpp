/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LeaderboardScene - Global leaderboards, stats, and achievements
*/

#include "scenes/LeaderboardScene.hpp"
#include "scenes/SceneManager.hpp"
#include "network/NetworkEvents.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

LeaderboardScene::LeaderboardScene() {
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Constructor called");
}

void LeaderboardScene::loadAssets() {
    if (_assetsLoaded) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Assets loaded");
}

void LeaderboardScene::initUI() {
    if (_uiInitialized) return;

    // Starfield background
    _starfield = std::make_unique<ui::StarfieldBackground>(
        static_cast<int>(SCREEN_WIDTH),
        static_cast<int>(SCREEN_HEIGHT),
        STAR_COUNT
    );

    // Tab buttons
    float tabX = (SCREEN_WIDTH - 3 * TAB_BTN_WIDTH - 40) / 2;

    _leaderboardTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX, TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "LEADERBOARD",
        FONT_KEY
    );
    _leaderboardTabBtn->setOnClick([this]() { onLeaderboardTabClick(); });

    _statsTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX + TAB_BTN_WIDTH + 20, TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "MY STATS",
        FONT_KEY
    );
    _statsTabBtn->setOnClick([this]() { onStatsTabClick(); });

    _achievementsTabBtn = std::make_unique<ui::Button>(
        Vec2f{tabX + 2 * (TAB_BTN_WIDTH + 20), TAB_Y},
        Vec2f{TAB_BTN_WIDTH, TAB_BTN_HEIGHT},
        "ACHIEVEMENTS",
        FONT_KEY
    );
    _achievementsTabBtn->setOnClick([this]() { onAchievementsTabClick(); });

    // Period buttons (for leaderboard tab)
    float periodX = MARGIN_X;
    float periodY = CONTENT_START_Y - 50;
    float periodBtnWidth = 120.0f;
    float periodBtnHeight = 40.0f;

    _allTimeBtn = std::make_unique<ui::Button>(
        Vec2f{periodX, periodY},
        Vec2f{periodBtnWidth, periodBtnHeight},
        "ALL TIME",
        FONT_KEY
    );
    _allTimeBtn->setOnClick([this]() { onAllTimeClick(); });

    _weeklyBtn = std::make_unique<ui::Button>(
        Vec2f{periodX + 130, periodY},
        Vec2f{periodBtnWidth, periodBtnHeight},
        "WEEKLY",
        FONT_KEY
    );
    _weeklyBtn->setOnClick([this]() { onWeeklyClick(); });

    _monthlyBtn = std::make_unique<ui::Button>(
        Vec2f{periodX + 260, periodY},
        Vec2f{periodBtnWidth, periodBtnHeight},
        "MONTHLY",
        FONT_KEY
    );
    _monthlyBtn->setOnClick([this]() { onMonthlyClick(); });

    // Back button
    _backBtn = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 80},
        Vec2f{200, 50},
        "BACK",
        FONT_KEY
    );
    _backBtn->setOnClick([this]() { onBackClick(); });

    _uiInitialized = true;
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] UI initialized");

    // Load initial data
    requestLeaderboard(0);  // All-time by default
}

void LeaderboardScene::handleEvent(const events::Event& event) {
    if (!_uiInitialized) return;

    // Escape to go back
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& keyEvt = std::get<events::KeyPressed>(event);
        if (keyEvt.key == events::Key::Escape) {
            onBackClick();
            return;
        }
        // Scroll with arrow keys
        if (keyEvt.key == events::Key::Up && _scrollOffset > 0) {
            _scrollOffset--;
        }
        if (keyEvt.key == events::Key::Down &&
            _scrollOffset < static_cast<int>(_leaderboardEntries.size()) - VISIBLE_ENTRIES) {
            _scrollOffset++;
        }
    }

    // Handle clicks on column headers for sorting (only in leaderboard tab)
    if (_currentTab == Tab::Leaderboard && std::holds_alternative<events::MouseButtonPressed>(event)) {
        auto& mouseEvt = std::get<events::MouseButtonPressed>(event);
        if (mouseEvt.button == events::MouseButton::Left) {
            float mx = static_cast<float>(mouseEvt.x);
            float my = static_cast<float>(mouseEvt.y);
            float headerY = CONTENT_START_Y;

            // Check if click is in header row (Y range)
            if (my >= headerY && my <= headerY + 25) {
                // Check column hit boxes
                // SCORE column: MARGIN_X + 400, width ~100
                if (mx >= MARGIN_X + 400 && mx < MARGIN_X + 500) {
                    onColumnHeaderClick(SortColumn::Score);
                }
                // WAVE column: MARGIN_X + 580, width ~80
                else if (mx >= MARGIN_X + 580 && mx < MARGIN_X + 660) {
                    onColumnHeaderClick(SortColumn::Wave);
                }
                // KILLS column: MARGIN_X + 700, width ~80
                else if (mx >= MARGIN_X + 700 && mx < MARGIN_X + 780) {
                    onColumnHeaderClick(SortColumn::Kills);
                }
                // TIME column: MARGIN_X + 820, width ~100
                else if (mx >= MARGIN_X + 820 && mx < MARGIN_X + 920) {
                    onColumnHeaderClick(SortColumn::Time);
                }
            }
        }
    }

    // Tab buttons
    _leaderboardTabBtn->handleEvent(event);
    _statsTabBtn->handleEvent(event);
    _achievementsTabBtn->handleEvent(event);

    // Period buttons (only in leaderboard tab)
    if (_currentTab == Tab::Leaderboard) {
        _allTimeBtn->handleEvent(event);
        _weeklyBtn->handleEvent(event);
        _monthlyBtn->handleEvent(event);
    }

    _backBtn->handleEvent(event);
}

void LeaderboardScene::update(float deltaTime) {
    loadAssets();
    initUI();

    // Update starfield
    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Loading animation
    _loadingDots += deltaTime * 3.0f;
    if (_loadingDots > 4.0f) _loadingDots = 0.0f;

    // Update buttons
    if (_leaderboardTabBtn) _leaderboardTabBtn->update(deltaTime);
    if (_statsTabBtn) _statsTabBtn->update(deltaTime);
    if (_achievementsTabBtn) _achievementsTabBtn->update(deltaTime);
    if (_allTimeBtn) _allTimeBtn->update(deltaTime);
    if (_weeklyBtn) _weeklyBtn->update(deltaTime);
    if (_monthlyBtn) _monthlyBtn->update(deltaTime);
    if (_backBtn) _backBtn->update(deltaTime);

    // Process incoming TCP messages
    processTCPEvents();
}

void LeaderboardScene::processTCPEvents() {
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::LeaderboardDataEvent>) {
                onLeaderboardDataReceived(event.response);
                // Also use the parsed entries
                _leaderboardEntries.clear();
                for (const auto& entry : event.entries) {
                    LeaderboardEntryData e;
                    e.rank = entry.rank;
                    e.playerName = std::string(entry.playerName);
                    e.score = entry.score;
                    e.wave = entry.wave;
                    e.kills = entry.kills;
                    e.duration = entry.duration;
                    _leaderboardEntries.push_back(e);
                }
                _leaderboardLoading = false;
            } else if constexpr (std::is_same_v<T, client::network::PlayerStatsDataEvent>) {
                onPlayerStatsReceived(event.stats);
            } else if constexpr (std::is_same_v<T, client::network::AchievementsDataEvent>) {
                onAchievementsReceived(event.bitfield);
            }
        }, *eventOpt);
    }
}

void LeaderboardScene::render() {
    if (!_assetsLoaded || !_uiInitialized) return;

    _context.window->clear();

    // Draw starfield
    if (_starfield) {
        _starfield->render(*_context.window);
    }

    // Title
    _context.window->drawText(FONT_KEY, "LEADERBOARD & STATS",
                              SCREEN_WIDTH / 2 - 180, 20, 28, {255, 215, 0, 255});

    // Highlight active tab
    rgba activeColor{100, 200, 255, 255};
    rgba inactiveColor{80, 80, 80, 255};

    if (_currentTab == Tab::Leaderboard) {
        _leaderboardTabBtn->setNormalColor(activeColor);
        _statsTabBtn->setNormalColor(inactiveColor);
        _achievementsTabBtn->setNormalColor(inactiveColor);
    } else if (_currentTab == Tab::Stats) {
        _leaderboardTabBtn->setNormalColor(inactiveColor);
        _statsTabBtn->setNormalColor(activeColor);
        _achievementsTabBtn->setNormalColor(inactiveColor);
    } else {
        _leaderboardTabBtn->setNormalColor(inactiveColor);
        _statsTabBtn->setNormalColor(inactiveColor);
        _achievementsTabBtn->setNormalColor(activeColor);
    }

    // Tab buttons
    _leaderboardTabBtn->render(*_context.window);
    _statsTabBtn->render(*_context.window);
    _achievementsTabBtn->render(*_context.window);

    // Render current tab content
    switch (_currentTab) {
        case Tab::Leaderboard:
            renderLeaderboardTab();
            break;
        case Tab::Stats:
            renderStatsTab();
            break;
        case Tab::Achievements:
            renderAchievementsTab();
            break;
    }

    // Back button
    _backBtn->render(*_context.window);

    _context.window->display();
}

void LeaderboardScene::renderLeaderboardTab() {
    // Period buttons
    rgba allTimeColor = (_currentPeriod == 0) ? rgba{100, 200, 100, 255} : rgba{60, 60, 60, 255};
    rgba weeklyColor = (_currentPeriod == 1) ? rgba{100, 200, 100, 255} : rgba{60, 60, 60, 255};
    rgba monthlyColor = (_currentPeriod == 2) ? rgba{100, 200, 100, 255} : rgba{60, 60, 60, 255};

    _allTimeBtn->setNormalColor(allTimeColor);
    _weeklyBtn->setNormalColor(weeklyColor);
    _monthlyBtn->setNormalColor(monthlyColor);

    _allTimeBtn->render(*_context.window);
    _weeklyBtn->render(*_context.window);
    _monthlyBtn->render(*_context.window);

    if (_leaderboardLoading) {
        renderLoadingIndicator();
        return;
    }

    // Column headers (clickable for sorting)
    float headerY = CONTENT_START_Y;
    rgba headerColor{200, 200, 200, 255};
    rgba headerActiveColor{255, 215, 0, 255};  // Gold for active sort column

    // Helper to get sort indicator
    auto getSortIndicator = [this](SortColumn col) -> std::string {
        if (_sortColumn != col) return "";
        return _sortAscending ? " ^" : " v";
    };

    // Helper to get header color (active = gold)
    auto getHeaderColor = [&](SortColumn col) -> rgba {
        return (_sortColumn == col) ? headerActiveColor : headerColor;
    };

    // Draw clickable column headers with sort indicators
    _context.window->drawText(FONT_KEY, "RANK", MARGIN_X, headerY, 16, headerColor);  // Rank not sortable (always by rank from server)
    _context.window->drawText(FONT_KEY, "PLAYER", MARGIN_X + 100, headerY, 16, headerColor);  // Player not sortable

    std::string scoreText = "SCORE" + getSortIndicator(SortColumn::Score);
    _context.window->drawText(FONT_KEY, scoreText, MARGIN_X + 400, headerY, 16, getHeaderColor(SortColumn::Score));

    std::string waveText = "WAVE" + getSortIndicator(SortColumn::Wave);
    _context.window->drawText(FONT_KEY, waveText, MARGIN_X + 580, headerY, 16, getHeaderColor(SortColumn::Wave));

    std::string killsText = "KILLS" + getSortIndicator(SortColumn::Kills);
    _context.window->drawText(FONT_KEY, killsText, MARGIN_X + 700, headerY, 16, getHeaderColor(SortColumn::Kills));

    std::string timeText = "TIME" + getSortIndicator(SortColumn::Time);
    _context.window->drawText(FONT_KEY, timeText, MARGIN_X + 820, headerY, 16, getHeaderColor(SortColumn::Time));

    // Separator line
    _context.window->drawRect(MARGIN_X, headerY + 30, SCREEN_WIDTH - 2 * MARGIN_X, 2, {100, 100, 100, 255});

    // Entries
    float rowY = headerY + 50;
    for (int i = _scrollOffset; i < std::min(_scrollOffset + VISIBLE_ENTRIES, static_cast<int>(_leaderboardEntries.size())); ++i) {
        const auto& entry = _leaderboardEntries[i];

        // Highlight top 3
        rgba rowColor{255, 255, 255, 255};
        if (entry.rank == 1) rowColor = {255, 215, 0, 255};  // Gold
        else if (entry.rank == 2) rowColor = {192, 192, 192, 255};  // Silver
        else if (entry.rank == 3) rowColor = {205, 127, 50, 255};  // Bronze

        _context.window->drawText(FONT_KEY, "#" + std::to_string(entry.rank),
                                  MARGIN_X, rowY, 16, rowColor);
        _context.window->drawText(FONT_KEY, entry.playerName,
                                  MARGIN_X + 100, rowY, 16, rowColor);
        _context.window->drawText(FONT_KEY, formatNumber(entry.score),
                                  MARGIN_X + 400, rowY, 16, rowColor);
        _context.window->drawText(FONT_KEY, std::to_string(entry.wave),
                                  MARGIN_X + 580, rowY, 16, rowColor);
        _context.window->drawText(FONT_KEY, std::to_string(entry.kills),
                                  MARGIN_X + 700, rowY, 16, rowColor);
        _context.window->drawText(FONT_KEY, formatDuration(entry.duration),
                                  MARGIN_X + 820, rowY, 16, rowColor);

        rowY += ROW_HEIGHT;
    }

    // Your rank
    if (_yourRank > 0) {
        _context.window->drawText(FONT_KEY, "Your Rank: #" + std::to_string(_yourRank),
                                  SCREEN_WIDTH - MARGIN_X - 200, CONTENT_START_Y - 50, 18, {100, 200, 255, 255});
    }

    // Scroll indicator
    if (_leaderboardEntries.size() > static_cast<size_t>(VISIBLE_ENTRIES)) {
        std::string scrollInfo = std::to_string(_scrollOffset + 1) + "-" +
                                 std::to_string(std::min(_scrollOffset + VISIBLE_ENTRIES, static_cast<int>(_leaderboardEntries.size()))) +
                                 " of " + std::to_string(_leaderboardEntries.size());
        _context.window->drawText(FONT_KEY, scrollInfo,
                                  SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 130, 14, {150, 150, 150, 255});
    }
}

void LeaderboardScene::renderStatsTab() {
    if (_statsLoading) {
        renderLoadingIndicator();
        return;
    }

    if (!_statsReceived) {
        _context.window->drawText(FONT_KEY, "No stats available yet. Play some games!",
                                  SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2, 18, {200, 200, 200, 255});
        return;
    }

    float x = MARGIN_X;
    float y = CONTENT_START_Y;
    float labelWidth = 300.0f;
    rgba labelColor{180, 180, 180, 255};
    rgba valueColor{255, 255, 255, 255};
    rgba highlightColor{255, 215, 0, 255};

    // Player name header
    _context.window->drawText(FONT_KEY, _playerStats.playerName + "'s Statistics",
                              x, y, 24, highlightColor);
    y += 60;

    // Draw stats helper
    auto drawStat = [&](const std::string& label, const std::string& value, bool highlight = false) {
        _context.window->drawText(FONT_KEY, label, x, y, 16, labelColor);
        _context.window->drawText(FONT_KEY, value, x + labelWidth, y, 16, highlight ? highlightColor : valueColor);
        y += 35;
    };

    // General stats
    drawStat("Total Score:", formatNumber(_playerStats.totalScore));
    drawStat("Best Score:", formatNumber(_playerStats.bestScore), true);
    drawStat("Games Played:", std::to_string(_playerStats.gamesPlayed));
    drawStat("Total Playtime:", formatDuration(_playerStats.totalPlaytime));

    y += 20;  // Spacing

    // Combat stats
    drawStat("Total Kills:", formatNumber(_playerStats.totalKills));
    drawStat("Total Deaths:", std::to_string(_playerStats.totalDeaths));
    std::ostringstream kdStream;
    kdStream << std::fixed << std::setprecision(2) << _playerStats.kdRatio;
    drawStat("K/D Ratio:", kdStream.str());
    drawStat("Best Wave:", std::to_string(_playerStats.bestWave), true);
    drawStat("Best Combo:", std::to_string(_playerStats.bestCombo / 10) + "." + std::to_string(_playerStats.bestCombo % 10) + "x");
    drawStat("Best Kill Streak:", std::to_string(_playerStats.bestKillStreak));
    drawStat("Boss Kills:", std::to_string(_playerStats.bossKills));

    // Weapon stats (right column)
    float rightX = SCREEN_WIDTH / 2 + 100;
    float rightY = CONTENT_START_Y + 60;

    _context.window->drawText(FONT_KEY, "Weapon Statistics", rightX, rightY - 30, 20, highlightColor);

    std::array<std::string, 4> weaponNames = {"Standard", "Spread", "Laser", "Missile"};
    std::array<uint32_t, 4> weaponKills = {
        _playerStats.standardKills,
        _playerStats.spreadKills,
        _playerStats.laserKills,
        _playerStats.missileKills
    };

    uint32_t totalWeaponKills = weaponKills[0] + weaponKills[1] + weaponKills[2] + weaponKills[3];

    for (size_t i = 0; i < 4; ++i) {
        // Progress bar
        float barWidth = 300.0f;
        float barHeight = 25.0f;
        float fillPercent = totalWeaponKills > 0 ?
                            static_cast<float>(weaponKills[i]) / static_cast<float>(totalWeaponKills) : 0.0f;

        // Background
        _context.window->drawRect(rightX, rightY, barWidth, barHeight, {40, 40, 40, 255});
        // Fill
        rgba barColor = (i == _playerStats.favoriteWeapon) ? rgba{100, 200, 100, 255} : rgba{80, 120, 180, 255};
        _context.window->drawRect(rightX, rightY, barWidth * fillPercent, barHeight, barColor);
        // Label
        _context.window->drawText(FONT_KEY, weaponNames[i] + ": " + formatNumber(weaponKills[i]),
                                  rightX + 10, rightY + 3, 14, {255, 255, 255, 255});

        rightY += 40;
    }

    // Favorite weapon indicator
    rightY += 10;
    _context.window->drawText(FONT_KEY, "Favorite: " + weaponNames[_playerStats.favoriteWeapon],
                              rightX, rightY, 16, {255, 200, 100, 255});
}

void LeaderboardScene::renderAchievementsTab() {
    if (_achievementsLoading) {
        renderLoadingIndicator();
        return;
    }

    float x = MARGIN_X;
    float y = CONTENT_START_Y;
    float boxWidth = (SCREEN_WIDTH - 2 * MARGIN_X - 40) / 2;
    float boxHeight = 80.0f;

    _context.window->drawText(FONT_KEY, "Achievements", x, y - 40, 24, {255, 215, 0, 255});

    // Count unlocked
    int unlockedCount = 0;
    for (size_t i = 0; i < ACHIEVEMENT_COUNT; ++i) {
        if (_achievementsBitfield & (1u << i)) unlockedCount++;
    }
    std::string countStr = std::to_string(unlockedCount) + " / " + std::to_string(ACHIEVEMENT_COUNT) + " Unlocked";
    _context.window->drawText(FONT_KEY, countStr,
                              SCREEN_WIDTH - MARGIN_X - 180, y - 40, 16, {180, 180, 180, 255});

    // Draw achievement boxes in 2 columns
    for (size_t i = 0; i < ACHIEVEMENT_COUNT; ++i) {
        bool unlocked = (_achievementsBitfield & (1u << i)) != 0;
        float col = static_cast<float>(i % 2);
        float row = static_cast<float>(i / 2);

        float bx = x + col * (boxWidth + 40);
        float by = y + row * (boxHeight + 20);

        // Box background
        rgba bgColor = unlocked ? rgba{40, 80, 40, 200} : rgba{40, 40, 40, 200};
        _context.window->drawRect(bx, by, boxWidth, boxHeight, bgColor);

        // Border
        rgba borderColor = getAchievementColor(static_cast<uint8_t>(i), unlocked);
        _context.window->drawRect(bx, by, boxWidth, 3, borderColor);  // Top border

        // Icon placeholder (colored square)
        _context.window->drawRect(bx + 10, by + 15, 50, 50, borderColor);

        // Achievement name
        _context.window->drawText(FONT_KEY, getAchievementName(static_cast<uint8_t>(i)),
                                  bx + 75, by + 15, 16, unlocked ? rgba{255, 255, 255, 255} : rgba{120, 120, 120, 255});

        // Description
        _context.window->drawText(FONT_KEY, getAchievementDescription(static_cast<uint8_t>(i)),
                                  bx + 75, by + 45, 12, unlocked ? rgba{200, 200, 200, 255} : rgba{100, 100, 100, 255});

        // Lock icon for locked achievements
        if (!unlocked) {
            _context.window->drawText(FONT_KEY, "LOCKED",
                                      bx + 20, by + 35, 10, {80, 80, 80, 255});
        }
    }
}

void LeaderboardScene::renderLoadingIndicator() {
    std::string dots(static_cast<int>(_loadingDots) % 4, '.');
    _context.window->drawText(FONT_KEY, "Loading" + dots,
                              SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2, 20, {200, 200, 200, 255});
}

void LeaderboardScene::onLeaderboardTabClick() {
    _currentTab = Tab::Leaderboard;
    if (_leaderboardEntries.empty()) {
        requestLeaderboard(_currentPeriod);
    }
}

void LeaderboardScene::onStatsTabClick() {
    _currentTab = Tab::Stats;
    if (!_statsReceived) {
        requestPlayerStats();
    }
}

void LeaderboardScene::onAchievementsTabClick() {
    _currentTab = Tab::Achievements;
    if (!_achievementsReceived) {
        requestAchievements();
    }
}

void LeaderboardScene::onAllTimeClick() {
    if (_currentPeriod != 0) {
        _currentPeriod = 0;
        requestLeaderboard(0);
    }
}

void LeaderboardScene::onWeeklyClick() {
    if (_currentPeriod != 1) {
        _currentPeriod = 1;
        requestLeaderboard(1);
    }
}

void LeaderboardScene::onMonthlyClick() {
    if (_currentPeriod != 2) {
        _currentPeriod = 2;
        requestLeaderboard(2);
    }
}

void LeaderboardScene::onBackClick() {
    if (_sceneManager) {
        _sceneManager->popScene();
    }
}

void LeaderboardScene::requestLeaderboard(uint8_t period) {
    if (!_context.tcpClient) return;

    _leaderboardLoading = true;
    _leaderboardEntries.clear();
    _scrollOffset = 0;

    GetLeaderboardRequest req;
    req.period = period;
    req.limit = MAX_ENTRIES;

    _context.tcpClient->sendGetLeaderboard(req);
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Requested leaderboard period={}", period);
}

void LeaderboardScene::requestPlayerStats() {
    if (!_context.tcpClient) return;

    _statsLoading = true;
    _context.tcpClient->sendGetPlayerStats();
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Requested player stats");
}

void LeaderboardScene::requestAchievements() {
    if (!_context.tcpClient) return;

    _achievementsLoading = true;
    _context.tcpClient->sendGetAchievements();
    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Requested achievements");
}

void LeaderboardScene::onLeaderboardDataReceived(const LeaderboardDataResponse& data) {
    _leaderboardLoading = false;
    _yourRank = data.yourRank;

    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Received leaderboard data, yourRank={}",
                 _yourRank);
}

void LeaderboardScene::onPlayerStatsReceived(const PlayerStatsWire& stats) {
    _statsLoading = false;
    _statsReceived = true;

    _playerStats.playerName = std::string(stats.playerName);
    _playerStats.totalScore = stats.totalScore;
    _playerStats.totalKills = stats.totalKills;
    _playerStats.totalDeaths = stats.totalDeaths;
    _playerStats.totalPlaytime = stats.totalPlaytime;
    _playerStats.gamesPlayed = stats.gamesPlayed;
    _playerStats.bestScore = stats.bestScore;
    _playerStats.bestWave = stats.bestWave;
    _playerStats.bestCombo = stats.bestCombo;
    _playerStats.bestKillStreak = stats.bestKillStreak;
    _playerStats.bossKills = stats.bossKills;
    _playerStats.standardKills = stats.standardKills;
    _playerStats.spreadKills = stats.spreadKills;
    _playerStats.laserKills = stats.laserKills;
    _playerStats.missileKills = stats.missileKills;
    _playerStats.kdRatio = _playerStats.totalDeaths > 0 ?
                           static_cast<float>(_playerStats.totalKills) / static_cast<float>(_playerStats.totalDeaths) :
                           static_cast<float>(_playerStats.totalKills);

    // Find favorite weapon
    uint32_t maxKills = _playerStats.standardKills;
    _playerStats.favoriteWeapon = 0;
    if (_playerStats.spreadKills > maxKills) { maxKills = _playerStats.spreadKills; _playerStats.favoriteWeapon = 1; }
    if (_playerStats.laserKills > maxKills) { maxKills = _playerStats.laserKills; _playerStats.favoriteWeapon = 2; }
    if (_playerStats.missileKills > maxKills) { _playerStats.favoriteWeapon = 3; }

    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Received player stats for {}", _playerStats.playerName);
}

void LeaderboardScene::onAchievementsReceived(uint32_t bitfield) {
    _achievementsLoading = false;
    _achievementsReceived = true;
    _achievementsBitfield = bitfield;

    client::logging::Logger::getSceneLogger()->info("[LeaderboardScene] Received achievements bitfield: 0x{:08X}", bitfield);
}

std::string LeaderboardScene::formatDuration(uint32_t seconds) const {
    uint32_t hours = seconds / 3600;
    uint32_t mins = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << mins << "m";
    } else if (mins > 0) {
        oss << mins << "m " << secs << "s";
    } else {
        oss << secs << "s";
    }
    return oss.str();
}

std::string LeaderboardScene::formatNumber(uint64_t number) const {
    if (number >= 1000000) {
        return std::to_string(number / 1000000) + "." + std::to_string((number % 1000000) / 100000) + "M";
    } else if (number >= 1000) {
        return std::to_string(number / 1000) + "." + std::to_string((number % 1000) / 100) + "K";
    }
    return std::to_string(number);
}

std::string LeaderboardScene::getAchievementName(uint8_t type) const {
    static const std::array<std::string, ACHIEVEMENT_COUNT> names = {
        "First Blood",
        "Exterminator",
        "Combo Master",
        "Boss Slayer",
        "Survivor",
        "Speed Demon",
        "Perfectionist",
        "Veteran",
        "Untouchable",
        "Weapon Master"
    };
    return type < ACHIEVEMENT_COUNT ? names[type] : "Unknown";
}

std::string LeaderboardScene::getAchievementDescription(uint8_t type) const {
    static const std::array<std::string, ACHIEVEMENT_COUNT> descriptions = {
        "Get your first kill",
        "Reach 1000 total kills",
        "Achieve a 3.0x combo multiplier",
        "Defeat a boss",
        "Reach wave 20 without dying",
        "Complete wave 10 in under 5 minutes",
        "Complete a wave without taking damage",
        "Play 100 games",
        "Complete a game without dying",
        "Get 100+ kills with all 4 weapons"
    };
    return type < ACHIEVEMENT_COUNT ? descriptions[type] : "";
}

rgba LeaderboardScene::getAchievementColor(uint8_t type, bool unlocked) const {
    if (!unlocked) return {80, 80, 80, 255};

    // Different colors for different achievement rarities
    static const std::array<rgba, ACHIEVEMENT_COUNT> colors = {
        rgba{150, 200, 100, 255},  // FirstBlood - Common (green)
        rgba{255, 215, 0, 255},    // Exterminator - Legendary (gold)
        rgba{200, 100, 255, 255},  // ComboMaster - Epic (purple)
        rgba{255, 100, 100, 255},  // BossSlayer - Rare (red)
        rgba{255, 215, 0, 255},    // Survivor - Legendary (gold)
        rgba{100, 200, 255, 255},  // SpeedDemon - Rare (blue)
        rgba{200, 200, 200, 255},  // Perfectionist - Uncommon (silver)
        rgba{200, 100, 255, 255},  // Veteran - Epic (purple)
        rgba{255, 215, 0, 255},    // Untouchable - Legendary (gold)
        rgba{255, 215, 0, 255}     // WeaponMaster - Legendary (gold)
    };
    return type < ACHIEVEMENT_COUNT ? colors[type] : rgba{255, 255, 255, 255};
}

void LeaderboardScene::onColumnHeaderClick(SortColumn column) {
    if (_sortColumn == column) {
        // Toggle direction if clicking same column
        _sortAscending = !_sortAscending;
    } else {
        // New column - set default direction
        _sortColumn = column;
        // Rank default ascending, others default descending (highest first)
        _sortAscending = (column == SortColumn::Rank);
    }
    sortLeaderboard();
}

void LeaderboardScene::sortLeaderboard() {
    if (_leaderboardEntries.empty()) return;

    std::sort(_leaderboardEntries.begin(), _leaderboardEntries.end(),
        [this](const LeaderboardEntryData& a, const LeaderboardEntryData& b) {
            bool less = false;
            switch (_sortColumn) {
                case SortColumn::Rank:
                    less = a.rank < b.rank;
                    break;
                case SortColumn::Score:
                    less = a.score < b.score;
                    break;
                case SortColumn::Wave:
                    less = a.wave < b.wave;
                    break;
                case SortColumn::Kills:
                    less = a.kills < b.kills;
                    break;
                case SortColumn::Time:
                    less = a.duration < b.duration;
                    break;
            }
            return _sortAscending ? less : !less;
        });

    _scrollOffset = 0;  // Reset scroll when sorting
}
