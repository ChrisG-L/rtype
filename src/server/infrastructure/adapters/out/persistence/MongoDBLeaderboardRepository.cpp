/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBLeaderboardRepository - MongoDB implementation for leaderboard and stats
*/

#include "infrastructure/adapters/out/persistence/MongoDBLeaderboardRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>
#include <chrono>
#include <ctime>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;

// Helper functions to safely read integers from BSON (handles both int32 and int64)
namespace {
    int64_t getInt64Safe(const bsoncxx::document::element& elem) {
        if (elem.type() == bsoncxx::type::k_int64) {
            return elem.get_int64().value;
        } else if (elem.type() == bsoncxx::type::k_int32) {
            return static_cast<int64_t>(elem.get_int32().value);
        }
        return 0;
    }

    int32_t getInt32Safe(const bsoncxx::document::element& elem) {
        if (elem.type() == bsoncxx::type::k_int32) {
            return elem.get_int32().value;
        } else if (elem.type() == bsoncxx::type::k_int64) {
            return static_cast<int32_t>(elem.get_int64().value);
        }
        return 0;
    }
}

MongoDBLeaderboardRepository::MongoDBLeaderboardRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    auto db = _mongoDB->getDatabaseConfig();
    _leaderboardCollection = std::make_unique<mongocxx::v_noabi::collection>(db["leaderboard"]);
    _playerStatsCollection = std::make_unique<mongocxx::v_noabi::collection>(db["player_stats"]);
    _gameHistoryCollection = std::make_unique<mongocxx::v_noabi::collection>(db["game_history"]);
    _achievementsCollection = std::make_unique<mongocxx::v_noabi::collection>(db["achievements"]);
    _currentGameSessionsCollection = std::make_unique<mongocxx::v_noabi::collection>(db["current_game_sessions"]);
}

std::string MongoDBLeaderboardRepository::periodToString(LeaderboardPeriod period) const {
    switch (period) {
        case LeaderboardPeriod::Weekly: return "weekly";
        case LeaderboardPeriod::Monthly: return "monthly";
        default: return "all_time";
    }
}

int64_t MongoDBLeaderboardRepository::getPeriodStartTimestamp(LeaderboardPeriod period) const {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&nowTime);

    switch (period) {
        case LeaderboardPeriod::Weekly: {
            // Start of current week (Monday)
            int daysSinceMonday = (tm->tm_wday + 6) % 7;
            tm->tm_mday -= daysSinceMonday;
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
            return std::mktime(tm);
        }
        case LeaderboardPeriod::Monthly: {
            // Start of current month
            tm->tm_mday = 1;
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
            return std::mktime(tm);
        }
        default:
            return 0; // All time - no filter
    }
}

LeaderboardEntry MongoDBLeaderboardRepository::documentToLeaderboardEntry(
    const bsoncxx::document::view& doc) const
{
    LeaderboardEntry entry;

    if (doc["email"]) entry.odId = std::string(doc["email"].get_string().value);
    if (doc["playerName"]) entry.playerName = std::string(doc["playerName"].get_string().value);
    if (doc["score"]) entry.score = static_cast<uint32_t>(getInt64Safe(doc["score"]));
    if (doc["wave"]) entry.wave = static_cast<uint16_t>(getInt32Safe(doc["wave"]));
    if (doc["kills"]) entry.kills = static_cast<uint16_t>(getInt32Safe(doc["kills"]));
    if (doc["deaths"]) entry.deaths = static_cast<uint8_t>(getInt32Safe(doc["deaths"]));
    if (doc["duration"]) entry.duration = static_cast<uint32_t>(getInt64Safe(doc["duration"]));
    if (doc["timestamp"]) entry.timestamp = getInt64Safe(doc["timestamp"]);
    if (doc["playerCount"]) entry.playerCount = static_cast<uint8_t>(getInt32Safe(doc["playerCount"]));

    return entry;
}

PlayerStats MongoDBLeaderboardRepository::documentToPlayerStats(
    const bsoncxx::document::view& doc) const
{
    PlayerStats stats;

    if (doc["email"]) stats.odId = std::string(doc["email"].get_string().value);
    if (doc["playerName"]) stats.playerName = std::string(doc["playerName"].get_string().value);
    if (doc["totalScore"]) stats.totalScore = static_cast<uint64_t>(getInt64Safe(doc["totalScore"]));
    if (doc["totalKills"]) stats.totalKills = static_cast<uint32_t>(getInt64Safe(doc["totalKills"]));
    if (doc["totalDeaths"]) stats.totalDeaths = static_cast<uint32_t>(getInt64Safe(doc["totalDeaths"]));
    if (doc["totalPlaytime"]) stats.totalPlaytime = static_cast<uint32_t>(getInt64Safe(doc["totalPlaytime"]));
    if (doc["gamesPlayed"]) stats.gamesPlayed = static_cast<uint32_t>(getInt64Safe(doc["gamesPlayed"]));
    if (doc["bestScore"]) stats.bestScore = static_cast<uint32_t>(getInt64Safe(doc["bestScore"]));
    if (doc["bestWave"]) stats.bestWave = static_cast<uint16_t>(getInt32Safe(doc["bestWave"]));
    if (doc["bestCombo"]) stats.bestCombo = static_cast<uint16_t>(getInt32Safe(doc["bestCombo"]));
    if (doc["bestKillStreak"]) stats.bestKillStreak = static_cast<uint16_t>(getInt32Safe(doc["bestKillStreak"]));
    if (doc["bestWaveStreak"]) stats.bestWaveStreak = static_cast<uint16_t>(getInt32Safe(doc["bestWaveStreak"]));
    if (doc["totalPerfectWaves"]) stats.totalPerfectWaves = static_cast<uint32_t>(getInt64Safe(doc["totalPerfectWaves"]));
    if (doc["bossKills"]) stats.bossKills = static_cast<uint16_t>(getInt32Safe(doc["bossKills"]));
    if (doc["standardKills"]) stats.standardKills = static_cast<uint32_t>(getInt64Safe(doc["standardKills"]));
    if (doc["spreadKills"]) stats.spreadKills = static_cast<uint32_t>(getInt64Safe(doc["spreadKills"]));
    if (doc["laserKills"]) stats.laserKills = static_cast<uint32_t>(getInt64Safe(doc["laserKills"]));
    if (doc["missileKills"]) stats.missileKills = static_cast<uint32_t>(getInt64Safe(doc["missileKills"]));
    if (doc["waveCannonKills"]) stats.waveCannonKills = static_cast<uint32_t>(getInt64Safe(doc["waveCannonKills"]));
    if (doc["totalDamageDealt"]) stats.totalDamageDealt = static_cast<uint64_t>(getInt64Safe(doc["totalDamageDealt"]));
    if (doc["achievements"]) stats.achievements = static_cast<uint32_t>(getInt64Safe(doc["achievements"]));

    return stats;
}

GameHistoryEntry MongoDBLeaderboardRepository::documentToGameHistory(
    const bsoncxx::document::view& doc) const
{
    GameHistoryEntry entry;

    if (doc["email"]) entry.odId = std::string(doc["email"].get_string().value);
    if (doc["playerName"]) entry.playerName = std::string(doc["playerName"].get_string().value);
    if (doc["score"]) entry.score = static_cast<uint32_t>(getInt64Safe(doc["score"]));
    if (doc["wave"]) entry.wave = static_cast<uint16_t>(getInt32Safe(doc["wave"]));
    if (doc["kills"]) entry.kills = static_cast<uint16_t>(getInt32Safe(doc["kills"]));
    if (doc["deaths"]) entry.deaths = static_cast<uint8_t>(getInt32Safe(doc["deaths"]));
    if (doc["duration"]) entry.duration = static_cast<uint32_t>(getInt64Safe(doc["duration"]));
    if (doc["timestamp"]) entry.timestamp = getInt64Safe(doc["timestamp"]);
    if (doc["weaponUsed"]) entry.weaponUsed = static_cast<uint8_t>(getInt32Safe(doc["weaponUsed"]));
    if (doc["bossDefeated"]) entry.bossDefeated = doc["bossDefeated"].get_bool().value;
    if (doc["playerCount"]) entry.playerCount = static_cast<uint8_t>(getInt32Safe(doc["playerCount"]));

    return entry;
}

// =============================================================================
// Leaderboard Operations
// =============================================================================

std::vector<LeaderboardEntry> MongoDBLeaderboardRepository::getLeaderboard(
    LeaderboardPeriod period, uint32_t limit)
{
    std::vector<LeaderboardEntry> entries;

    try {
        // Use aggregation pipeline to get best score per player
        mongocxx::pipeline pipeline;

        // Stage 1: Filter by period if needed
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            pipeline.match(make_document(kvp("timestamp", make_document(kvp("$gte", startTs)))));
        }

        // Stage 2: Sort by score descending (to get best score first per player)
        pipeline.sort(make_document(kvp("score", -1)));

        // Stage 3: Group by email, keeping the first (best) document for each player
        pipeline.group(make_document(
            kvp("_id", "$email"),
            kvp("playerName", make_document(kvp("$first", "$playerName"))),
            kvp("score", make_document(kvp("$max", "$score"))),
            kvp("wave", make_document(kvp("$first", "$wave"))),
            kvp("kills", make_document(kvp("$first", "$kills"))),
            kvp("deaths", make_document(kvp("$first", "$deaths"))),
            kvp("duration", make_document(kvp("$first", "$duration"))),
            kvp("timestamp", make_document(kvp("$first", "$timestamp")))
        ));

        // Stage 4: Sort again by score (group may have reordered)
        pipeline.sort(make_document(kvp("score", -1)));

        // Stage 5: Limit results
        pipeline.limit(static_cast<int32_t>(limit));

        auto cursor = _leaderboardCollection->aggregate(pipeline);

        uint32_t rank = 1;
        for (auto&& doc : cursor) {
            LeaderboardEntry entry;
            if (doc["_id"]) entry.odId = std::string(doc["_id"].get_string().value);
            if (doc["playerName"]) entry.playerName = std::string(doc["playerName"].get_string().value);
            if (doc["score"]) entry.score = static_cast<uint32_t>(getInt64Safe(doc["score"]));
            if (doc["wave"]) entry.wave = static_cast<uint16_t>(getInt32Safe(doc["wave"]));
            if (doc["kills"]) entry.kills = static_cast<uint16_t>(getInt32Safe(doc["kills"]));
            if (doc["deaths"]) entry.deaths = static_cast<uint8_t>(getInt32Safe(doc["deaths"]));
            if (doc["duration"]) entry.duration = static_cast<uint32_t>(getInt64Safe(doc["duration"]));
            if (doc["timestamp"]) entry.timestamp = getInt64Safe(doc["timestamp"]);
            if (doc["playerCount"]) entry.playerCount = static_cast<uint8_t>(getInt32Safe(doc["playerCount"]));
            entry.rank = rank++;
            entries.push_back(entry);
        }
    } catch (const std::exception& e) {
        auto logger = server::logging::Logger::getGameLogger();
        logger->error("getLeaderboard failed: {}", e.what());
        // Return empty list on error
    }

    return entries;
}

std::vector<LeaderboardEntry> MongoDBLeaderboardRepository::getLeaderboard(
    LeaderboardPeriod period, uint8_t playerCount, uint32_t limit)
{
    std::vector<LeaderboardEntry> entries;

    try {
        mongocxx::pipeline pipeline;

        // Stage 1: Filter by period if needed
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            pipeline.match(make_document(kvp("timestamp", make_document(kvp("$gte", startTs)))));
        }

        // Stage 1b: Filter by player count if specified (0 = all)
        if (playerCount > 0) {
            pipeline.match(make_document(kvp("playerCount", static_cast<int32_t>(playerCount))));
        }

        // Stage 2: Sort by score descending
        pipeline.sort(make_document(kvp("score", -1)));

        // Stage 3: Group by email, keeping the best score per player
        pipeline.group(make_document(
            kvp("_id", "$email"),
            kvp("playerName", make_document(kvp("$first", "$playerName"))),
            kvp("score", make_document(kvp("$max", "$score"))),
            kvp("wave", make_document(kvp("$first", "$wave"))),
            kvp("kills", make_document(kvp("$first", "$kills"))),
            kvp("deaths", make_document(kvp("$first", "$deaths"))),
            kvp("duration", make_document(kvp("$first", "$duration"))),
            kvp("timestamp", make_document(kvp("$first", "$timestamp"))),
            kvp("playerCount", make_document(kvp("$first", "$playerCount")))
        ));

        // Stage 4: Sort again by score
        pipeline.sort(make_document(kvp("score", -1)));

        // Stage 5: Limit results
        pipeline.limit(static_cast<int32_t>(limit));

        auto cursor = _leaderboardCollection->aggregate(pipeline);

        uint32_t rank = 1;
        for (auto&& doc : cursor) {
            LeaderboardEntry entry;
            if (doc["_id"]) entry.odId = std::string(doc["_id"].get_string().value);
            if (doc["playerName"]) entry.playerName = std::string(doc["playerName"].get_string().value);
            if (doc["score"]) entry.score = static_cast<uint32_t>(getInt64Safe(doc["score"]));
            if (doc["wave"]) entry.wave = static_cast<uint16_t>(getInt32Safe(doc["wave"]));
            if (doc["kills"]) entry.kills = static_cast<uint16_t>(getInt32Safe(doc["kills"]));
            if (doc["deaths"]) entry.deaths = static_cast<uint8_t>(getInt32Safe(doc["deaths"]));
            if (doc["duration"]) entry.duration = static_cast<uint32_t>(getInt64Safe(doc["duration"]));
            if (doc["timestamp"]) entry.timestamp = getInt64Safe(doc["timestamp"]);
            if (doc["playerCount"]) entry.playerCount = static_cast<uint8_t>(getInt32Safe(doc["playerCount"]));
            entry.rank = rank++;
            entries.push_back(entry);
        }
    } catch (const std::exception& e) {
        auto logger = server::logging::Logger::getGameLogger();
        logger->error("getLeaderboard (playerCount={}) failed: {}", playerCount, e.what());
    }

    return entries;
}

uint32_t MongoDBLeaderboardRepository::getPlayerRank(
    const std::string& email, LeaderboardPeriod period)
{
    try {
        // Get player's best score for this period
        bsoncxx::builder::basic::document filter;
        filter.append(kvp("email", email));
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            filter.append(kvp("timestamp", make_document(kvp("$gte", startTs))));
        }

        mongocxx::options::find opts;
        opts.sort(make_document(kvp("score", -1)));
        opts.limit(1);

        auto result = _leaderboardCollection->find_one(filter.view(), opts);
        if (!result) return 0;

        uint32_t playerScore = static_cast<uint32_t>(getInt64Safe(result->view()["score"]));

        // Count how many UNIQUE PLAYERS have a higher best score
        // Use aggregation to group by email and get max score per player
        using bsoncxx::builder::basic::make_array;
        mongocxx::pipeline pipe;

        // Match period filter if needed
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            pipe.match(make_document(kvp("timestamp", make_document(kvp("$gte", startTs)))));
        }

        // Group by email and get max score per player
        pipe.group(make_document(
            kvp("_id", "$email"),
            kvp("maxScore", make_document(kvp("$max", "$score")))
        ));

        // Filter to only players with higher score than ours
        pipe.match(make_document(
            kvp("maxScore", make_document(kvp("$gt", static_cast<int64_t>(playerScore))))
        ));

        // Count the results
        pipe.count("count");

        auto cursor = _leaderboardCollection->aggregate(pipe);
        uint32_t count = 0;
        for (auto&& doc : cursor) {
            count = static_cast<uint32_t>(getInt64Safe(doc["count"]));
            break;
        }

        return count + 1;  // Rank = number of players ahead + 1
    } catch (const std::exception& e) {
        auto logger = server::logging::Logger::getGameLogger();
        logger->error("getPlayerRank failed for {}: {}", email, e.what());
        return 0;  // Return 0 (unranked) on error
    }
}

uint32_t MongoDBLeaderboardRepository::getPlayerRank(
    const std::string& email, LeaderboardPeriod period, uint8_t playerCount)
{
    try {
        // Get player's best score for this period and player count
        bsoncxx::builder::basic::document filter;
        filter.append(kvp("email", email));
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            filter.append(kvp("timestamp", make_document(kvp("$gte", startTs))));
        }
        if (playerCount > 0) {
            filter.append(kvp("playerCount", static_cast<int32_t>(playerCount)));
        }

        mongocxx::options::find opts;
        opts.sort(make_document(kvp("score", -1)));
        opts.limit(1);

        auto result = _leaderboardCollection->find_one(filter.view(), opts);
        if (!result) return 0;

        uint32_t playerScore = static_cast<uint32_t>(getInt64Safe(result->view()["score"]));

        // Count unique players with higher best score
        mongocxx::pipeline pipe;

        // Match period filter if needed
        if (period != LeaderboardPeriod::AllTime) {
            int64_t startTs = getPeriodStartTimestamp(period);
            pipe.match(make_document(kvp("timestamp", make_document(kvp("$gte", startTs)))));
        }

        // Match player count filter if needed
        if (playerCount > 0) {
            pipe.match(make_document(kvp("playerCount", static_cast<int32_t>(playerCount))));
        }

        // Group by email and get max score per player
        pipe.group(make_document(
            kvp("_id", "$email"),
            kvp("maxScore", make_document(kvp("$max", "$score")))
        ));

        // Filter to only players with higher score
        pipe.match(make_document(
            kvp("maxScore", make_document(kvp("$gt", static_cast<int64_t>(playerScore))))
        ));

        pipe.count("count");

        auto cursor = _leaderboardCollection->aggregate(pipe);
        uint32_t count = 0;
        for (auto&& doc : cursor) {
            count = static_cast<uint32_t>(getInt64Safe(doc["count"]));
            break;
        }

        return count + 1;
    } catch (const std::exception& e) {
        auto logger = server::logging::Logger::getGameLogger();
        logger->error("getPlayerRank (playerCount={}) failed for {}: {}", playerCount, email, e.what());
        return 0;
    }
}

bool MongoDBLeaderboardRepository::submitScore(
    const std::string& email, const std::string& playerName, const LeaderboardEntry& entry)
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    auto doc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("score", static_cast<int64_t>(entry.score)),
        kvp("wave", static_cast<int32_t>(entry.wave)),
        kvp("kills", static_cast<int32_t>(entry.kills)),
        kvp("deaths", static_cast<int32_t>(entry.deaths)),
        kvp("duration", static_cast<int64_t>(entry.duration)),
        kvp("timestamp", timestamp),
        kvp("playerCount", static_cast<int32_t>(entry.playerCount))
    );

    auto result = _leaderboardCollection->insert_one(doc.view());
    return result.has_value();
}

// =============================================================================
// Player Stats Operations
// =============================================================================

std::optional<PlayerStats> MongoDBLeaderboardRepository::getPlayerStats(const std::string& email)
{
    auto result = _playerStatsCollection->find_one(make_document(kvp("email", email)));
    if (result.has_value()) {
        return documentToPlayerStats(result->view());
    }
    return std::nullopt;
}

void MongoDBLeaderboardRepository::updatePlayerStats(
    const std::string& email, const std::string& playerName, const GameHistoryEntry& gameStats)
{
    // First, get existing stats or create new
    auto existing = getPlayerStats(email);
    PlayerStats stats = existing.value_or(PlayerStats{});
    stats.odId = email;
    stats.playerName = playerName;

    // Update cumulative stats
    stats.totalScore += gameStats.score;
    stats.totalKills += gameStats.kills;
    stats.totalDeaths += gameStats.deaths;
    stats.totalPlaytime += gameStats.duration;
    stats.gamesPlayed++;

    // Update best records
    if (gameStats.score > stats.bestScore) stats.bestScore = gameStats.score;
    if (gameStats.wave > stats.bestWave) stats.bestWave = gameStats.wave;
    if (gameStats.bestKillStreak > stats.bestKillStreak) stats.bestKillStreak = gameStats.bestKillStreak;
    if (gameStats.bestWaveStreak > stats.bestWaveStreak) stats.bestWaveStreak = gameStats.bestWaveStreak;
    if (gameStats.bestCombo > stats.bestCombo) stats.bestCombo = gameStats.bestCombo;

    // Cumulative perfect waves
    stats.totalPerfectWaves += gameStats.perfectWaves;

    // Update weapon kills - use detailed stats if available, otherwise fall back to weaponUsed
    if (gameStats.standardKills > 0 || gameStats.spreadKills > 0 ||
        gameStats.laserKills > 0 || gameStats.missileKills > 0 || gameStats.waveCannonKills > 0) {
        // Detailed stats available
        stats.standardKills += gameStats.standardKills;
        stats.spreadKills += gameStats.spreadKills;
        stats.laserKills += gameStats.laserKills;
        stats.missileKills += gameStats.missileKills;
        stats.waveCannonKills += gameStats.waveCannonKills;
    } else {
        // Fallback to weaponUsed (old behavior)
        switch (gameStats.weaponUsed) {
            case 0: stats.standardKills += gameStats.kills; break;
            case 1: stats.spreadKills += gameStats.kills; break;
            case 2: stats.laserKills += gameStats.kills; break;
            case 3: stats.missileKills += gameStats.kills; break;
            case 4: stats.waveCannonKills += gameStats.kills; break;
        }
    }

    // Boss kills - use detailed count if available
    if (gameStats.bossKills > 0) {
        stats.bossKills += gameStats.bossKills;
    } else if (gameStats.bossDefeated) {
        stats.bossKills++;
    }

    // Total damage dealt
    stats.totalDamageDealt += gameStats.totalDamageDealt;

    // Save updated stats
    auto doc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("totalScore", static_cast<int64_t>(stats.totalScore)),
        kvp("totalKills", static_cast<int64_t>(stats.totalKills)),
        kvp("totalDeaths", static_cast<int64_t>(stats.totalDeaths)),
        kvp("totalPlaytime", static_cast<int64_t>(stats.totalPlaytime)),
        kvp("gamesPlayed", static_cast<int64_t>(stats.gamesPlayed)),
        kvp("bestScore", static_cast<int64_t>(stats.bestScore)),
        kvp("bestWave", static_cast<int32_t>(stats.bestWave)),
        kvp("bestCombo", static_cast<int32_t>(stats.bestCombo)),
        kvp("bestKillStreak", static_cast<int32_t>(stats.bestKillStreak)),
        kvp("bestWaveStreak", static_cast<int32_t>(stats.bestWaveStreak)),
        kvp("totalPerfectWaves", static_cast<int64_t>(stats.totalPerfectWaves)),
        kvp("bossKills", static_cast<int32_t>(stats.bossKills)),
        kvp("standardKills", static_cast<int64_t>(stats.standardKills)),
        kvp("spreadKills", static_cast<int64_t>(stats.spreadKills)),
        kvp("laserKills", static_cast<int64_t>(stats.laserKills)),
        kvp("missileKills", static_cast<int64_t>(stats.missileKills)),
        kvp("waveCannonKills", static_cast<int64_t>(stats.waveCannonKills)),
        kvp("totalDamageDealt", static_cast<int64_t>(stats.totalDamageDealt)),
        kvp("achievements", static_cast<int64_t>(stats.achievements)),
        kvp("updatedAt", bsoncxx::types::b_date{std::chrono::system_clock::now()})
    );

    mongocxx::options::update options;
    options.upsert(true);

    _playerStatsCollection->update_one(
        make_document(kvp("email", email)),
        make_document(kvp("$set", doc)),
        options
    );

    // Also save to game history
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    auto historyDoc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("score", static_cast<int64_t>(gameStats.score)),
        kvp("wave", static_cast<int32_t>(gameStats.wave)),
        kvp("kills", static_cast<int32_t>(gameStats.kills)),
        kvp("deaths", static_cast<int32_t>(gameStats.deaths)),
        kvp("duration", static_cast<int64_t>(gameStats.duration)),
        kvp("timestamp", timestamp),
        kvp("weaponUsed", static_cast<int32_t>(gameStats.weaponUsed)),
        kvp("bossDefeated", gameStats.bossDefeated)
    );

    _gameHistoryCollection->insert_one(historyDoc.view());
}

std::vector<GameHistoryEntry> MongoDBLeaderboardRepository::getGameHistory(
    const std::string& email, uint32_t limit)
{
    std::vector<GameHistoryEntry> entries;

    mongocxx::options::find opts;
    opts.sort(make_document(kvp("timestamp", -1)));
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = _gameHistoryCollection->find(
        make_document(kvp("email", email)), opts);

    for (auto&& doc : cursor) {
        entries.push_back(documentToGameHistory(doc));
    }

    return entries;
}

// =============================================================================
// Achievement Operations
// =============================================================================

std::vector<AchievementRecord> MongoDBLeaderboardRepository::getAchievements(const std::string& email)
{
    std::vector<AchievementRecord> records;

    auto cursor = _achievementsCollection->find(make_document(kvp("email", email)));

    for (auto&& doc : cursor) {
        AchievementRecord record;
        if (doc["type"]) {
            record.type = static_cast<AchievementType>(getInt32Safe(doc["type"]));
        }
        if (doc["unlockedAt"]) {
            record.unlockedAt = getInt64Safe(doc["unlockedAt"]);
        }
        records.push_back(record);
    }

    return records;
}

bool MongoDBLeaderboardRepository::unlockAchievement(const std::string& email, AchievementType type)
{
    // Check if already unlocked
    auto existing = _achievementsCollection->find_one(
        make_document(
            kvp("email", email),
            kvp("type", static_cast<int32_t>(type))
        )
    );

    if (existing) return false; // Already has achievement

    // Unlock it
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    auto doc = make_document(
        kvp("email", email),
        kvp("type", static_cast<int32_t>(type)),
        kvp("unlockedAt", timestamp)
    );

    auto result = _achievementsCollection->insert_one(doc.view());

    // Also update the bitmask in player_stats
    if (result) {
        _playerStatsCollection->update_one(
            make_document(kvp("email", email)),
            make_document(kvp("$bit", make_document(
                kvp("achievements", make_document(
                    kvp("or", static_cast<int64_t>(1u << static_cast<uint32_t>(type)))
                ))
            )))
        );
    }

    return result.has_value();
}

// =============================================================================
// Maintenance Operations
// =============================================================================

void MongoDBLeaderboardRepository::resetWeeklyLeaderboard()
{
    int64_t weekAgo = getPeriodStartTimestamp(LeaderboardPeriod::Weekly);
    // For weekly reset, we don't delete, we just use timestamp filtering
    // But if needed, we could archive old entries here
}

void MongoDBLeaderboardRepository::resetMonthlyLeaderboard()
{
    int64_t monthAgo = getPeriodStartTimestamp(LeaderboardPeriod::Monthly);
    // Same as weekly - timestamp-based filtering handles this
}

// =============================================================================
// Current Game Session Management (for auto-save without stat duplication)
// =============================================================================

void MongoDBLeaderboardRepository::saveCurrentGameSession(
    const std::string& email, const std::string& playerName,
    const std::string& roomCode, const GameHistoryEntry& gameStats)
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // Upsert: update if exists, insert if not
    // This avoids creating duplicate entries for the same game session
    auto doc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("roomCode", roomCode),
        kvp("score", static_cast<int64_t>(gameStats.score)),
        kvp("wave", static_cast<int32_t>(gameStats.wave)),
        kvp("kills", static_cast<int32_t>(gameStats.kills)),
        kvp("deaths", static_cast<int32_t>(gameStats.deaths)),
        kvp("duration", static_cast<int64_t>(gameStats.duration)),
        kvp("standardKills", static_cast<int64_t>(gameStats.standardKills)),
        kvp("spreadKills", static_cast<int64_t>(gameStats.spreadKills)),
        kvp("laserKills", static_cast<int64_t>(gameStats.laserKills)),
        kvp("missileKills", static_cast<int64_t>(gameStats.missileKills)),
        kvp("waveCannonKills", static_cast<int64_t>(gameStats.waveCannonKills)),
        kvp("bossKills", static_cast<int32_t>(gameStats.bossKills)),
        kvp("bestCombo", static_cast<int32_t>(gameStats.bestCombo)),
        kvp("bestKillStreak", static_cast<int32_t>(gameStats.bestKillStreak)),
        kvp("bestWaveStreak", static_cast<int32_t>(gameStats.bestWaveStreak)),
        kvp("perfectWaves", static_cast<int32_t>(gameStats.perfectWaves)),
        kvp("totalDamageDealt", static_cast<int64_t>(gameStats.totalDamageDealt)),
        kvp("bossDefeated", gameStats.bossDefeated),
        kvp("playerCount", static_cast<int32_t>(gameStats.playerCount)),
        kvp("updatedAt", timestamp)
    );

    mongocxx::options::update options;
    options.upsert(true);

    _currentGameSessionsCollection->update_one(
        make_document(kvp("email", email)),
        make_document(kvp("$set", doc)),
        options
    );
}

void MongoDBLeaderboardRepository::finalizeGameSession(
    const std::string& email, const std::string& playerName)
{
    auto logger = server::logging::Logger::getGameLogger();
    logger->debug("[MongoDB] finalizeGameSession: email={}, playerName={}", email, playerName);

    // Find the current game session
    auto sessionOpt = getCurrentGameSession(email);
    if (!sessionOpt) {
        logger->debug("[MongoDB] finalizeGameSession: No current_game_session found for email={}", email);
        return;  // No session to finalize
    }

    const auto& session = *sessionOpt;

    // Only finalize if there's actual game data
    if (session.score == 0 && session.kills == 0) {
        _currentGameSessionsCollection->delete_one(make_document(kvp("email", email)));
        return;
    }

    // Transfer to updatePlayerStats (which adds to cumulative stats and game_history)
    updatePlayerStats(email, playerName, session);

    // Delete the current session
    _currentGameSessionsCollection->delete_one(make_document(kvp("email", email)));
    logger->info("[Leaderboard] Stats saved for {} (score={}, kills={}, wave={})",
                playerName, session.score, session.kills, session.wave);
}

std::optional<GameHistoryEntry> MongoDBLeaderboardRepository::getCurrentGameSession(
    const std::string& email)
{
    auto result = _currentGameSessionsCollection->find_one(
        make_document(kvp("email", email))
    );

    if (!result) return std::nullopt;

    auto doc = result->view();
    GameHistoryEntry entry;

    if (doc["email"]) entry.odId = std::string(doc["email"].get_string().value);
    if (doc["playerName"]) entry.playerName = std::string(doc["playerName"].get_string().value);
    if (doc["score"]) entry.score = static_cast<uint32_t>(getInt64Safe(doc["score"]));
    if (doc["wave"]) entry.wave = static_cast<uint16_t>(getInt32Safe(doc["wave"]));
    if (doc["kills"]) entry.kills = static_cast<uint16_t>(getInt32Safe(doc["kills"]));
    if (doc["deaths"]) entry.deaths = static_cast<uint8_t>(getInt32Safe(doc["deaths"]));
    if (doc["duration"]) entry.duration = static_cast<uint32_t>(getInt64Safe(doc["duration"]));
    if (doc["standardKills"]) entry.standardKills = static_cast<uint32_t>(getInt64Safe(doc["standardKills"]));
    if (doc["spreadKills"]) entry.spreadKills = static_cast<uint32_t>(getInt64Safe(doc["spreadKills"]));
    if (doc["laserKills"]) entry.laserKills = static_cast<uint32_t>(getInt64Safe(doc["laserKills"]));
    if (doc["missileKills"]) entry.missileKills = static_cast<uint32_t>(getInt64Safe(doc["missileKills"]));
    if (doc["waveCannonKills"]) entry.waveCannonKills = static_cast<uint32_t>(getInt64Safe(doc["waveCannonKills"]));
    if (doc["bossKills"]) entry.bossKills = static_cast<uint8_t>(getInt32Safe(doc["bossKills"]));
    if (doc["bestCombo"]) entry.bestCombo = static_cast<uint16_t>(getInt32Safe(doc["bestCombo"]));
    if (doc["bestKillStreak"]) entry.bestKillStreak = static_cast<uint16_t>(getInt32Safe(doc["bestKillStreak"]));
    if (doc["bestWaveStreak"]) entry.bestWaveStreak = static_cast<uint16_t>(getInt32Safe(doc["bestWaveStreak"]));
    if (doc["perfectWaves"]) entry.perfectWaves = static_cast<uint16_t>(getInt32Safe(doc["perfectWaves"]));
    if (doc["totalDamageDealt"]) entry.totalDamageDealt = static_cast<uint64_t>(getInt64Safe(doc["totalDamageDealt"]));
    if (doc["bossDefeated"]) entry.bossDefeated = doc["bossDefeated"].get_bool().value;
    if (doc["playerCount"]) entry.playerCount = static_cast<uint8_t>(getInt32Safe(doc["playerCount"]));
    if (doc["updatedAt"]) entry.timestamp = getInt64Safe(doc["updatedAt"]);

    return entry;
}

} // namespace infrastructure::adapters::out::persistence
