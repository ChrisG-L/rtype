/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LeaderboardDataTest - Unit tests for leaderboard data structures
*/

#include <gtest/gtest.h>
#include "application/ports/out/persistence/ILeaderboardRepository.hpp"

using namespace application::ports::out::persistence;

// ═══════════════════════════════════════════════════════════════════
// PlayerStats Tests
// ═══════════════════════════════════════════════════════════════════

class PlayerStatsTest : public ::testing::Test {
protected:
    PlayerStats stats;
};

TEST_F(PlayerStatsTest, DefaultConstructor_InitializesAllFieldsToZero) {
    EXPECT_EQ(stats.totalScore, 0u);
    EXPECT_EQ(stats.totalKills, 0u);
    EXPECT_EQ(stats.totalDeaths, 0u);
    EXPECT_EQ(stats.totalPlaytime, 0u);
    EXPECT_EQ(stats.gamesPlayed, 0u);
    EXPECT_EQ(stats.bestScore, 0u);
    EXPECT_EQ(stats.bestWave, 0u);
    EXPECT_EQ(stats.bestCombo, 0u);
    EXPECT_EQ(stats.achievements, 0u);
}

TEST_F(PlayerStatsTest, KDRatio_ReturnsCorrectValue) {
    stats.totalKills = 100;
    stats.totalDeaths = 25;

    EXPECT_FLOAT_EQ(stats.kdRatio(), 4.0f);
}

TEST_F(PlayerStatsTest, KDRatio_ReturnsKillsWhenZeroDeaths) {
    stats.totalKills = 50;
    stats.totalDeaths = 0;

    EXPECT_FLOAT_EQ(stats.kdRatio(), 50.0f);
}

TEST_F(PlayerStatsTest, AvgScore_ReturnsCorrectValue) {
    stats.totalScore = 10000;
    stats.gamesPlayed = 10;

    EXPECT_FLOAT_EQ(stats.avgScore(), 1000.0f);
}

TEST_F(PlayerStatsTest, AvgScore_ReturnsZeroWhenNoGames) {
    stats.totalScore = 0;
    stats.gamesPlayed = 0;

    EXPECT_FLOAT_EQ(stats.avgScore(), 0.0f);
}

TEST_F(PlayerStatsTest, FavoriteWeapon_ReturnsStandardByDefault) {
    stats.standardKills = 0;
    stats.spreadKills = 0;
    stats.laserKills = 0;
    stats.missileKills = 0;

    EXPECT_EQ(stats.favoriteWeapon(), 0u);  // Standard
}

TEST_F(PlayerStatsTest, FavoriteWeapon_ReturnsSpreadWhenHighest) {
    stats.standardKills = 50;
    stats.spreadKills = 100;
    stats.laserKills = 30;
    stats.missileKills = 20;

    EXPECT_EQ(stats.favoriteWeapon(), 1u);  // Spread
}

TEST_F(PlayerStatsTest, FavoriteWeapon_ReturnsLaserWhenHighest) {
    stats.standardKills = 50;
    stats.spreadKills = 60;
    stats.laserKills = 100;
    stats.missileKills = 20;

    EXPECT_EQ(stats.favoriteWeapon(), 2u);  // Laser
}

TEST_F(PlayerStatsTest, FavoriteWeapon_ReturnsMissileWhenHighest) {
    stats.standardKills = 50;
    stats.spreadKills = 60;
    stats.laserKills = 70;
    stats.missileKills = 100;

    EXPECT_EQ(stats.favoriteWeapon(), 3u);  // Missile
}

TEST_F(PlayerStatsTest, HasAchievement_ReturnsFalseByDefault) {
    EXPECT_FALSE(stats.hasAchievement(AchievementType::FirstBlood));
    EXPECT_FALSE(stats.hasAchievement(AchievementType::Exterminator));
    EXPECT_FALSE(stats.hasAchievement(AchievementType::WeaponMaster));
}

TEST_F(PlayerStatsTest, UnlockAchievement_SetsCorrectBit) {
    stats.unlockAchievement(AchievementType::FirstBlood);

    EXPECT_TRUE(stats.hasAchievement(AchievementType::FirstBlood));
    EXPECT_FALSE(stats.hasAchievement(AchievementType::Exterminator));
}

TEST_F(PlayerStatsTest, UnlockAchievement_MultipleBits) {
    stats.unlockAchievement(AchievementType::FirstBlood);
    stats.unlockAchievement(AchievementType::BossSlayer);
    stats.unlockAchievement(AchievementType::WeaponMaster);

    EXPECT_TRUE(stats.hasAchievement(AchievementType::FirstBlood));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::BossSlayer));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::WeaponMaster));
    EXPECT_FALSE(stats.hasAchievement(AchievementType::Exterminator));
}

TEST_F(PlayerStatsTest, HasAchievement_AllAchievements) {
    // Set all achievement bits
    stats.achievements = 0x3FF;  // First 10 bits set

    EXPECT_TRUE(stats.hasAchievement(AchievementType::FirstBlood));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::Exterminator));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::ComboMaster));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::BossSlayer));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::Survivor));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::SpeedDemon));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::Perfectionist));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::Veteran));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::Untouchable));
    EXPECT_TRUE(stats.hasAchievement(AchievementType::WeaponMaster));
}

// ═══════════════════════════════════════════════════════════════════
// LeaderboardEntry Tests
// ═══════════════════════════════════════════════════════════════════

class LeaderboardEntryTest : public ::testing::Test {
protected:
    LeaderboardEntry entry;
};

TEST_F(LeaderboardEntryTest, DefaultConstructor_InitializesFieldsToZero) {
    EXPECT_EQ(entry.score, 0u);
    EXPECT_EQ(entry.wave, 0u);
    EXPECT_EQ(entry.kills, 0u);
    EXPECT_EQ(entry.deaths, 0u);
    EXPECT_EQ(entry.duration, 0u);
    EXPECT_EQ(entry.timestamp, 0);
    EXPECT_EQ(entry.rank, 0u);
    EXPECT_TRUE(entry.playerName.empty());
}

// ═══════════════════════════════════════════════════════════════════
// GameHistoryEntry Tests
// ═══════════════════════════════════════════════════════════════════

class GameHistoryEntryTest : public ::testing::Test {
protected:
    GameHistoryEntry entry;
};

TEST_F(GameHistoryEntryTest, DefaultConstructor_InitializesFieldsToZero) {
    EXPECT_EQ(entry.score, 0u);
    EXPECT_EQ(entry.wave, 0u);
    EXPECT_EQ(entry.kills, 0u);
    EXPECT_EQ(entry.deaths, 0u);
    EXPECT_EQ(entry.duration, 0u);
    EXPECT_EQ(entry.timestamp, 0);
    EXPECT_EQ(entry.weaponUsed, 0u);
    EXPECT_FALSE(entry.bossDefeated);
}

TEST_F(GameHistoryEntryTest, WeaponKills_DefaultToZero) {
    EXPECT_EQ(entry.standardKills, 0u);
    EXPECT_EQ(entry.spreadKills, 0u);
    EXPECT_EQ(entry.laserKills, 0u);
    EXPECT_EQ(entry.missileKills, 0u);
    EXPECT_EQ(entry.waveCannonKills, 0u);
}

TEST_F(GameHistoryEntryTest, BestCombo_DefaultsTo10) {
    // 10 = 1.0x combo (baseline)
    EXPECT_EQ(entry.bestCombo, 10u);
}

// ═══════════════════════════════════════════════════════════════════
// AchievementRecord Tests
// ═══════════════════════════════════════════════════════════════════

class AchievementRecordTest : public ::testing::Test {
protected:
    AchievementRecord record;
};

TEST_F(AchievementRecordTest, DefaultConstructor_InitializesDefaults) {
    EXPECT_EQ(record.type, AchievementType::FirstBlood);
    EXPECT_EQ(record.unlockedAt, 0);
}

TEST_F(AchievementRecordTest, ParameterizedConstructor_SetsValues) {
    AchievementRecord r(AchievementType::BossSlayer, 1234567890);

    EXPECT_EQ(r.type, AchievementType::BossSlayer);
    EXPECT_EQ(r.unlockedAt, 1234567890);
}

// ═══════════════════════════════════════════════════════════════════
// GameOverData Tests
// ═══════════════════════════════════════════════════════════════════

class GameOverDataTest : public ::testing::Test {
protected:
    GameOverData data;
};

TEST_F(GameOverDataTest, DefaultConstructor_InitializesFieldsToZero) {
    EXPECT_EQ(data.score, 0u);
    EXPECT_EQ(data.wave, 0u);
    EXPECT_EQ(data.kills, 0u);
    EXPECT_EQ(data.deaths, 0u);
    EXPECT_EQ(data.duration, 0u);
    EXPECT_EQ(data.bestCombo, 0u);
    EXPECT_EQ(data.bossKills, 0u);
    EXPECT_EQ(data.globalRank, 0u);
    EXPECT_EQ(data.weeklyRank, 0u);
    EXPECT_EQ(data.monthlyRank, 0u);
    EXPECT_FALSE(data.isNewHighScore);
    EXPECT_FALSE(data.isNewWaveRecord);
    EXPECT_TRUE(data.newAchievements.empty());
}

// ═══════════════════════════════════════════════════════════════════
// AchievementType Tests
// ═══════════════════════════════════════════════════════════════════

TEST(AchievementTypeTest, AchievementToString_ReturnsCorrectNames) {
    EXPECT_STREQ(achievementToString(AchievementType::FirstBlood), "First Blood");
    EXPECT_STREQ(achievementToString(AchievementType::Exterminator), "Exterminator");
    EXPECT_STREQ(achievementToString(AchievementType::ComboMaster), "Combo Master");
    EXPECT_STREQ(achievementToString(AchievementType::BossSlayer), "Boss Slayer");
    EXPECT_STREQ(achievementToString(AchievementType::Survivor), "Survivor");
    EXPECT_STREQ(achievementToString(AchievementType::SpeedDemon), "Speed Demon");
    EXPECT_STREQ(achievementToString(AchievementType::Perfectionist), "Perfectionist");
    EXPECT_STREQ(achievementToString(AchievementType::Veteran), "Veteran");
    EXPECT_STREQ(achievementToString(AchievementType::Untouchable), "Untouchable");
    EXPECT_STREQ(achievementToString(AchievementType::WeaponMaster), "Weapon Master");
}

TEST(AchievementTypeTest, AchievementToString_UnknownReturnsUnknown) {
    EXPECT_STREQ(achievementToString(static_cast<AchievementType>(255)), "Unknown");
}

TEST(AchievementTypeTest, AchievementCount_Is10) {
    EXPECT_EQ(static_cast<int>(AchievementType::COUNT), 10);
}

// ═══════════════════════════════════════════════════════════════════
// LeaderboardPeriod Tests
// ═══════════════════════════════════════════════════════════════════

TEST(LeaderboardPeriodTest, EnumValues_AreCorrect) {
    EXPECT_EQ(static_cast<uint8_t>(LeaderboardPeriod::AllTime), 0);
    EXPECT_EQ(static_cast<uint8_t>(LeaderboardPeriod::Weekly), 1);
    EXPECT_EQ(static_cast<uint8_t>(LeaderboardPeriod::Monthly), 2);
}
