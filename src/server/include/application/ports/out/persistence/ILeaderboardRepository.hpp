/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ILeaderboardRepository - Interface for leaderboard and player stats persistence
*/

#ifndef ILEADERBOARDREPOSITORY_HPP_
#define ILEADERBOARDREPOSITORY_HPP_

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace application::ports::out::persistence {

enum class LeaderboardPeriod : uint8_t {
    AllTime = 0,
    Weekly = 1,
    Monthly = 2
};

enum class AchievementType : uint8_t {
    FirstBlood = 0,
    Exterminator = 1,
    ComboMaster = 2,
    BossSlayer = 3,
    Survivor = 4,
    SpeedDemon = 5,
    Perfectionist = 6,
    Veteran = 7,
    Untouchable = 8,
    WeaponMaster = 9,
    COUNT
};

inline const char* achievementToString(AchievementType type) {
    switch (type) {
        case AchievementType::FirstBlood: return "First Blood";
        case AchievementType::Exterminator: return "Exterminator";
        case AchievementType::ComboMaster: return "Combo Master";
        case AchievementType::BossSlayer: return "Boss Slayer";
        case AchievementType::Survivor: return "Survivor";
        case AchievementType::SpeedDemon: return "Speed Demon";
        case AchievementType::Perfectionist: return "Perfectionist";
        case AchievementType::Veteran: return "Veteran";
        case AchievementType::Untouchable: return "Untouchable";
        case AchievementType::WeaponMaster: return "Weapon Master";
        default: return "Unknown";
    }
}

struct LeaderboardEntry {
    std::string odId;
    std::string playerName;
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint8_t deaths;
    uint32_t duration;
    int64_t timestamp;
    uint32_t rank;

    LeaderboardEntry()
        : score(0), wave(0), kills(0), deaths(0), duration(0), timestamp(0), rank(0) {}
};

struct PlayerStats {
    std::string odId;
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
    uint16_t bestWaveStreak;
    uint32_t totalPerfectWaves;
    uint16_t bossKills;
    uint32_t standardKills;
    uint32_t spreadKills;
    uint32_t laserKills;
    uint32_t missileKills;
    uint32_t waveCannonKills;
    uint64_t totalDamageDealt;
    uint32_t achievements;

    float kdRatio() const {
        return totalDeaths > 0 ? static_cast<float>(totalKills) / totalDeaths : static_cast<float>(totalKills);
    }

    float avgScore() const {
        return gamesPlayed > 0 ? static_cast<float>(totalScore) / gamesPlayed : 0.0f;
    }

    uint8_t favoriteWeapon() const {
        uint32_t maxKills = standardKills;
        uint8_t favorite = 0;
        if (spreadKills > maxKills) { maxKills = spreadKills; favorite = 1; }
        if (laserKills > maxKills) { maxKills = laserKills; favorite = 2; }
        if (missileKills > maxKills) { maxKills = missileKills; favorite = 3; }
        if (waveCannonKills > maxKills) { favorite = 4; }
        return favorite;
    }

    bool hasAchievement(AchievementType type) const {
        return (achievements & (1u << static_cast<uint32_t>(type))) != 0;
    }

    void unlockAchievement(AchievementType type) {
        achievements |= (1u << static_cast<uint32_t>(type));
    }

    PlayerStats()
        : totalScore(0), totalKills(0), totalDeaths(0), totalPlaytime(0), gamesPlayed(0)
        , bestScore(0), bestWave(0), bestCombo(0), bestKillStreak(0), bestWaveStreak(0)
        , totalPerfectWaves(0), bossKills(0)
        , standardKills(0), spreadKills(0), laserKills(0), missileKills(0), waveCannonKills(0)
        , totalDamageDealt(0), achievements(0) {}
};

struct GameHistoryEntry {
    std::string odId;
    std::string playerName;
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint8_t deaths;
    uint32_t duration;
    int64_t timestamp;
    uint8_t weaponUsed;
    bool bossDefeated;

    // Detailed weapon kills (optional, for accurate stats tracking)
    uint32_t standardKills = 0;
    uint32_t spreadKills = 0;
    uint32_t laserKills = 0;
    uint32_t missileKills = 0;
    uint32_t waveCannonKills = 0;
    uint8_t bossKills = 0;
    uint16_t bestCombo = 10;  // x10 (10 = 1.0x)
    uint16_t bestKillStreak = 0;  // Best consecutive kills without damage
    uint16_t bestWaveStreak = 0;  // Best waves completed without dying
    uint16_t perfectWaves = 0;    // Waves completed without taking any damage
    uint64_t totalDamageDealt = 0;  // Total damage dealt to enemies/boss this game

    GameHistoryEntry()
        : score(0), wave(0), kills(0), deaths(0), duration(0), timestamp(0)
        , weaponUsed(0), bossDefeated(false) {}
};

struct AchievementRecord {
    AchievementType type;
    int64_t unlockedAt;

    AchievementRecord() : type(AchievementType::FirstBlood), unlockedAt(0) {}
    AchievementRecord(AchievementType t, int64_t ts) : type(t), unlockedAt(ts) {}
};

struct GameOverData {
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint8_t deaths;
    uint32_t duration;
    uint16_t bestCombo;
    uint8_t bossKills;
    uint32_t globalRank;
    uint32_t weeklyRank;
    uint32_t monthlyRank;
    bool isNewHighScore;
    bool isNewWaveRecord;
    std::vector<AchievementType> newAchievements;

    GameOverData()
        : score(0), wave(0), kills(0), deaths(0), duration(0), bestCombo(0), bossKills(0)
        , globalRank(0), weeklyRank(0), monthlyRank(0)
        , isNewHighScore(false), isNewWaveRecord(false) {}
};

class ILeaderboardRepository {
public:
    virtual ~ILeaderboardRepository() = default;

    virtual std::vector<LeaderboardEntry> getLeaderboard(LeaderboardPeriod period, uint32_t limit = 50) = 0;
    virtual uint32_t getPlayerRank(const std::string& email, LeaderboardPeriod period) = 0;
    virtual bool submitScore(const std::string& email, const std::string& playerName, const LeaderboardEntry& entry) = 0;
    virtual std::optional<PlayerStats> getPlayerStats(const std::string& email) = 0;
    virtual void updatePlayerStats(const std::string& email, const std::string& playerName, const GameHistoryEntry& gameStats) = 0;
    virtual std::vector<GameHistoryEntry> getGameHistory(const std::string& email, uint32_t limit = 10) = 0;
    virtual std::vector<AchievementRecord> getAchievements(const std::string& email) = 0;
    virtual bool unlockAchievement(const std::string& email, AchievementType type) = 0;
    virtual void resetWeeklyLeaderboard() = 0;
    virtual void resetMonthlyLeaderboard() = 0;

    // Current game session management (for auto-save without duplicating stats)
    // Saves current game state - upsert, doesn't add to cumulative stats
    virtual void saveCurrentGameSession(const std::string& email, const std::string& playerName,
                                        const std::string& roomCode, const GameHistoryEntry& gameStats) = 0;
    // Finalize game session - transfers to history, updates cumulative stats, removes session
    virtual void finalizeGameSession(const std::string& email, const std::string& playerName) = 0;
    // Get current game session (for recovery)
    virtual std::optional<GameHistoryEntry> getCurrentGameSession(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !ILEADERBOARDREPOSITORY_HPP_ */
