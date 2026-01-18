/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBLeaderboardRepository - MongoDB implementation for leaderboard and stats
*/

#ifndef MONGODBLEADERBOARDREPOSITORY_HPP_
#define MONGODBLEADERBOARDREPOSITORY_HPP_

#include "application/ports/out/persistence/ILeaderboardRepository.hpp"
#include "MongoDBConfiguration.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>
#include <memory>

namespace infrastructure::adapters::out::persistence {

using application::ports::out::persistence::ILeaderboardRepository;
using application::ports::out::persistence::LeaderboardPeriod;
using application::ports::out::persistence::AchievementType;
using application::ports::out::persistence::LeaderboardEntry;
using application::ports::out::persistence::PlayerStats;
using application::ports::out::persistence::GameHistoryEntry;
using application::ports::out::persistence::AchievementRecord;

class MongoDBLeaderboardRepository : public ILeaderboardRepository {
private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;

    // Collection names (no longer storing collection objects)
    static constexpr const char* LEADERBOARD_COLLECTION = "leaderboard";
    static constexpr const char* PLAYER_STATS_COLLECTION = "player_stats";
    static constexpr const char* GAME_HISTORY_COLLECTION = "game_history";
    static constexpr const char* ACHIEVEMENTS_COLLECTION = "achievements";
    static constexpr const char* CURRENT_GAME_SESSIONS_COLLECTION = "current_game_sessions";

    LeaderboardEntry documentToLeaderboardEntry(const bsoncxx::document::view& doc) const;
    PlayerStats documentToPlayerStats(const bsoncxx::document::view& doc) const;
    GameHistoryEntry documentToGameHistory(const bsoncxx::document::view& doc) const;

    std::string periodToString(LeaderboardPeriod period) const;
    int64_t getPeriodStartTimestamp(LeaderboardPeriod period) const;

public:
    explicit MongoDBLeaderboardRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBLeaderboardRepository() override = default;

    // Leaderboard
    std::vector<LeaderboardEntry> getLeaderboard(LeaderboardPeriod period, uint32_t limit = 50) override;
    std::vector<LeaderboardEntry> getLeaderboard(LeaderboardPeriod period, uint8_t playerCount, uint32_t limit = 50) override;
    uint32_t getPlayerRank(const std::string& email, LeaderboardPeriod period) override;
    uint32_t getPlayerRank(const std::string& email, LeaderboardPeriod period, uint8_t playerCount) override;
    bool submitScore(const std::string& email, const std::string& playerName, const LeaderboardEntry& entry) override;

    // Player Stats
    std::optional<PlayerStats> getPlayerStats(const std::string& email) override;
    void updatePlayerStats(const std::string& email, const std::string& playerName, const GameHistoryEntry& gameStats) override;
    std::vector<GameHistoryEntry> getGameHistory(const std::string& email, uint32_t limit = 10) override;

    // Achievements
    std::vector<AchievementRecord> getAchievements(const std::string& email) override;
    bool unlockAchievement(const std::string& email, AchievementType type) override;

    // Maintenance
    void resetWeeklyLeaderboard() override;
    void resetMonthlyLeaderboard() override;

    // Current game session management
    void saveCurrentGameSession(const std::string& email, const std::string& playerName,
                                const std::string& roomCode, const GameHistoryEntry& gameStats) override;
    void finalizeGameSession(const std::string& email, const std::string& playerName) override;
    std::optional<GameHistoryEntry> getCurrentGameSession(const std::string& email) override;
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBLEADERBOARDREPOSITORY_HPP_ */
