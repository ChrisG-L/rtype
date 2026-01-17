/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AchievementCheckerTest - Unit tests for AchievementChecker service
*/

#include <gtest/gtest.h>
#include "application/services/AchievementChecker.hpp"
#include "application/ports/out/persistence/ILeaderboardRepository.hpp"
#include <vector>
#include <set>

using namespace application::services;
using namespace application::ports::out::persistence;

// Mock repository for testing
class MockLeaderboardRepository : public ILeaderboardRepository {
public:
    std::set<AchievementType> unlockedAchievements;

    std::vector<LeaderboardEntry> getLeaderboard(LeaderboardPeriod, uint32_t) override {
        return {};
    }

    std::vector<LeaderboardEntry> getLeaderboard(LeaderboardPeriod, uint8_t, uint32_t) override {
        return {};
    }

    uint32_t getPlayerRank(const std::string&, LeaderboardPeriod) override {
        return 0;
    }

    uint32_t getPlayerRank(const std::string&, LeaderboardPeriod, uint8_t) override {
        return 0;
    }

    bool submitScore(const std::string&, const std::string&, const LeaderboardEntry&) override {
        return true;
    }

    std::optional<PlayerStats> getPlayerStats(const std::string&) override {
        return std::nullopt;
    }

    void updatePlayerStats(const std::string&, const std::string&, const GameHistoryEntry&) override {}

    std::vector<GameHistoryEntry> getGameHistory(const std::string&, uint32_t) override {
        return {};
    }

    std::vector<AchievementRecord> getAchievements(const std::string&) override {
        return {};
    }

    bool unlockAchievement(const std::string&, AchievementType type) override {
        if (unlockedAchievements.find(type) == unlockedAchievements.end()) {
            unlockedAchievements.insert(type);
            return true;
        }
        return false;
    }

    void resetWeeklyLeaderboard() override {}
    void resetMonthlyLeaderboard() override {}

    void saveCurrentGameSession(const std::string&, const std::string&,
                                const std::string&, const GameHistoryEntry&) override {}
    void finalizeGameSession(const std::string&, const std::string&) override {}
    std::optional<GameHistoryEntry> getCurrentGameSession(const std::string&) override {
        return std::nullopt;
    }
};

class AchievementCheckerTest : public ::testing::Test {
protected:
    MockLeaderboardRepository mockRepo;

    PlayerStats createBaseStats() {
        PlayerStats stats;
        stats.totalKills = 0;
        stats.totalDeaths = 0;
        stats.gamesPlayed = 0;
        stats.standardKills = 0;
        stats.spreadKills = 0;
        stats.laserKills = 0;
        stats.missileKills = 0;
        stats.achievements = 0;
        return stats;
    }

    GameHistoryEntry createBaseGameData() {
        GameHistoryEntry data;
        data.score = 0;
        data.wave = 0;
        data.kills = 0;
        data.deaths = 0;
        data.duration = 0;
        return data;
    }
};

// ═══════════════════════════════════════════════════════════════════
// FirstBlood Achievement Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, FirstBlood_UnlocksOnFirstKill) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;  // First kill!

    GameHistoryEntry gameData = createBaseGameData();
    gameData.kills = 1;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    ASSERT_EQ(unlocked.size(), 1);
    EXPECT_EQ(unlocked[0], AchievementType::FirstBlood);
}

TEST_F(AchievementCheckerTest, FirstBlood_DoesNotUnlockWithZeroKills) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 0;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    EXPECT_TRUE(unlocked.empty());
}

TEST_F(AchievementCheckerTest, FirstBlood_DoesNotUnlockTwice) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 5;
    stats.achievements = (1u << static_cast<uint32_t>(AchievementType::FirstBlood));

    GameHistoryEntry gameData = createBaseGameData();
    gameData.kills = 5;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    // FirstBlood should not be in the unlocked list since it's already unlocked
    for (const auto& achievement : unlocked) {
        EXPECT_NE(achievement, AchievementType::FirstBlood);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Exterminator Achievement Tests (1000 kills)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, Exterminator_UnlocksAt1000Kills) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1000;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasExterminator = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::Exterminator) hasExterminator = true;
    }
    EXPECT_TRUE(hasExterminator);
}

TEST_F(AchievementCheckerTest, Exterminator_DoesNotUnlockAt999Kills) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 999;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Exterminator);
    }
}

// ═══════════════════════════════════════════════════════════════════
// ComboMaster Achievement Tests (3.0x combo = 30 encoded)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, ComboMaster_UnlocksAt30Combo) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 30, false, false);  // 3.0x combo

    bool hasComboMaster = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::ComboMaster) hasComboMaster = true;
    }
    EXPECT_TRUE(hasComboMaster);
}

TEST_F(AchievementCheckerTest, ComboMaster_DoesNotUnlockAt29Combo) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 29, false, false);  // 2.9x combo

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::ComboMaster);
    }
}

// ═══════════════════════════════════════════════════════════════════
// BossSlayer Achievement Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, BossSlayer_UnlocksOnBossKill) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, true);  // Killed boss!

    bool hasBossSlayer = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::BossSlayer) hasBossSlayer = true;
    }
    EXPECT_TRUE(hasBossSlayer);
}

TEST_F(AchievementCheckerTest, BossSlayer_DoesNotUnlockWithoutBossKill) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 100;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);  // No boss kill

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::BossSlayer);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Survivor Achievement Tests (Wave 20 without dying)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, Survivor_UnlocksAtWave20WithZeroDeaths) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 20;
    gameData.deaths = 0;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasSurvivor = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::Survivor) hasSurvivor = true;
    }
    EXPECT_TRUE(hasSurvivor);
}

TEST_F(AchievementCheckerTest, Survivor_DoesNotUnlockAtWave19) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 19;
    gameData.deaths = 0;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Survivor);
    }
}

TEST_F(AchievementCheckerTest, Survivor_DoesNotUnlockWithDeaths) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 25;
    gameData.deaths = 1;  // Died once

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Survivor);
    }
}

// ═══════════════════════════════════════════════════════════════════
// SpeedDemon Achievement Tests (Wave 10 in under 5 minutes)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, SpeedDemon_UnlocksAtWave10Under5Minutes) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 10;
    gameData.duration = 299;  // 4:59

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasSpeedDemon = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::SpeedDemon) hasSpeedDemon = true;
    }
    EXPECT_TRUE(hasSpeedDemon);
}

TEST_F(AchievementCheckerTest, SpeedDemon_DoesNotUnlockOver5Minutes) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 10;
    gameData.duration = 301;  // 5:01

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::SpeedDemon);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Veteran Achievement Tests (100 games played)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, Veteran_UnlocksAt100Games) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;
    stats.gamesPlayed = 100;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasVeteran = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::Veteran) hasVeteran = true;
    }
    EXPECT_TRUE(hasVeteran);
}

TEST_F(AchievementCheckerTest, Veteran_DoesNotUnlockAt99Games) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;
    stats.gamesPlayed = 99;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Veteran);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Untouchable Achievement Tests (Complete game without dying)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, Untouchable_UnlocksWithZeroDeaths) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 5;
    gameData.deaths = 0;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasUntouchable = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::Untouchable) hasUntouchable = true;
    }
    EXPECT_TRUE(hasUntouchable);
}

TEST_F(AchievementCheckerTest, Untouchable_DoesNotUnlockWithDeaths) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 10;
    gameData.deaths = 1;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Untouchable);
    }
}

// ═══════════════════════════════════════════════════════════════════
// WeaponMaster Achievement Tests (100+ kills with all 4 weapons)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, WeaponMaster_UnlocksWith100KillsAllWeapons) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 400;
    stats.standardKills = 100;
    stats.spreadKills = 100;
    stats.laserKills = 100;
    stats.missileKills = 100;

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    bool hasWeaponMaster = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::WeaponMaster) hasWeaponMaster = true;
    }
    EXPECT_TRUE(hasWeaponMaster);
}

TEST_F(AchievementCheckerTest, WeaponMaster_DoesNotUnlockWithOneWeaponBelow100) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 399;
    stats.standardKills = 100;
    stats.spreadKills = 100;
    stats.laserKills = 100;
    stats.missileKills = 99;  // One below

    GameHistoryEntry gameData = createBaseGameData();

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::WeaponMaster);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Perfectionist Achievement Tests (Complete wave without damage)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, Perfectionist_UnlocksWithNoDamage) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 1;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, false, false);  // tookDamageThisGame = false

    bool hasPerfectionist = false;
    for (const auto& a : unlocked) {
        if (a == AchievementType::Perfectionist) hasPerfectionist = true;
    }
    EXPECT_TRUE(hasPerfectionist);
}

TEST_F(AchievementCheckerTest, Perfectionist_DoesNotUnlockWithDamage) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 5;

    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 10, true, false);  // tookDamageThisGame = true

    for (const auto& a : unlocked) {
        EXPECT_NE(a, AchievementType::Perfectionist);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Multiple Achievements Test
// ═══════════════════════════════════════════════════════════════════

TEST_F(AchievementCheckerTest, MultipleAchievements_UnlockSimultaneously) {
    PlayerStats stats = createBaseStats();
    stats.totalKills = 1000;
    stats.gamesPlayed = 100;
    stats.standardKills = 250;
    stats.spreadKills = 250;
    stats.laserKills = 250;
    stats.missileKills = 250;

    GameHistoryEntry gameData = createBaseGameData();
    gameData.wave = 20;
    gameData.deaths = 0;
    gameData.duration = 250;  // Under 5 minutes

    // First kill, 3.0x combo, killed boss, no damage
    auto unlocked = AchievementChecker::checkAndUnlock(
        mockRepo, "test@test.com", stats, gameData, 30, false, true);

    // Should unlock many achievements
    EXPECT_GE(unlocked.size(), 5);

    std::set<AchievementType> unlockedSet(unlocked.begin(), unlocked.end());

    EXPECT_TRUE(unlockedSet.count(AchievementType::FirstBlood) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::Exterminator) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::ComboMaster) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::BossSlayer) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::Survivor) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::SpeedDemon) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::Veteran) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::Untouchable) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::WeaponMaster) > 0);
    EXPECT_TRUE(unlockedSet.count(AchievementType::Perfectionist) > 0);
}
