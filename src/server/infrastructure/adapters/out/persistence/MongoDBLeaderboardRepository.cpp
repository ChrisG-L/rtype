/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBLeaderboardRepository - MongoDB implementation for leaderboard and stats
** Thread-safe implementation using connection pooling
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
    // No longer store collection objects - acquire from pool for each operation
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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto leaderboardCollection = db[LEADERBOARD_COLLECTION];

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

        auto cursor = leaderboardCollection.aggregate(pipeline);

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

    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto leaderboardCollection = db[LEADERBOARD_COLLECTION];

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

        auto cursor = leaderboardCollection.aggregate(pipeline);

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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto leaderboardCollection = db[LEADERBOARD_COLLECTION];

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

        auto result = leaderboardCollection.find_one(filter.view(), opts);
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

        auto cursor = leaderboardCollection.aggregate(pipe);
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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto leaderboardCollection = db[LEADERBOARD_COLLECTION];

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

        auto result = leaderboardCollection.find_one(filter.view(), opts);
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

        auto cursor = leaderboardCollection.aggregate(pipe);
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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto leaderboardCollection = db[LEADERBOARD_COLLECTION];

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

    auto result = leaderboardCollection.insert_one(doc.view());
    return result.has_value();
}

// =============================================================================
// Player Stats Operations
// =============================================================================

std::optional<PlayerStats> MongoDBLeaderboardRepository::getPlayerStats(const std::string& email)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto playerStatsCollection = db[PLAYER_STATS_COLLECTION];

    auto result = playerStatsCollection.find_one(make_document(kvp("email", email)));
    if (result.has_value()) {
        return documentToPlayerStats(result->view());
    }
    return std::nullopt;
}

void MongoDBLeaderboardRepository::updatePlayerStats(
    const std::string& email, const std::string& playerName, const GameHistoryEntry& gameStats)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto playerStatsCollection = db[PLAYER_STATS_COLLECTION];
    auto gameHistoryCollection = db[GAME_HISTORY_COLLECTION];

    // First, get existing stats or create new
    auto existingResult = playerStatsCollection.find_one(make_document(kvp("email", email)));
    PlayerStats stats;
    if (existingResult) {
        stats = documentToPlayerStats(existingResult->view());
    }
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

    playerStatsCollection.update_one(
        make_document(kvp("email", email)),
        make_document(kvp("$set", doc)),
        options
    );

    // Also save to game history
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // Calculate favorite weapon from kills per weapon
    uint8_t favoriteWeapon = 0;  // Standard by default
    uint32_t maxWeaponKills = gameStats.standardKills;
    if (gameStats.spreadKills > maxWeaponKills) { favoriteWeapon = 1; maxWeaponKills = gameStats.spreadKills; }
    if (gameStats.laserKills > maxWeaponKills) { favoriteWeapon = 2; maxWeaponKills = gameStats.laserKills; }
    if (gameStats.missileKills > maxWeaponKills) { favoriteWeapon = 3; maxWeaponKills = gameStats.missileKills; }
    if (gameStats.waveCannonKills > maxWeaponKills) { favoriteWeapon = 4; }

    auto historyDoc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("score", static_cast<int64_t>(gameStats.score)),
        kvp("wave", static_cast<int32_t>(gameStats.wave)),
        kvp("kills", static_cast<int32_t>(gameStats.kills)),
        kvp("deaths", static_cast<int32_t>(gameStats.deaths)),
        kvp("duration", static_cast<int64_t>(gameStats.duration)),
        kvp("timestamp", timestamp),
        kvp("weaponUsed", static_cast<int32_t>(favoriteWeapon)),
        kvp("bossDefeated", gameStats.bossDefeated),
        kvp("bestCombo", static_cast<int32_t>(gameStats.bestCombo)),
        kvp("playerCount", static_cast<int32_t>(gameStats.playerCount))
    );

    gameHistoryCollection.insert_one(historyDoc.view());
}

std::vector<GameHistoryEntry> MongoDBLeaderboardRepository::getGameHistory(
    const std::string& email, uint32_t limit)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto gameHistoryCollection = db[GAME_HISTORY_COLLECTION];

    std::vector<GameHistoryEntry> entries;

    mongocxx::options::find opts;
    opts.sort(make_document(kvp("timestamp", -1)));
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = gameHistoryCollection.find(
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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto achievementsCollection = db[ACHIEVEMENTS_COLLECTION];

    std::vector<AchievementRecord> records;

    auto cursor = achievementsCollection.find(make_document(kvp("email", email)));

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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto achievementsCollection = db[ACHIEVEMENTS_COLLECTION];
    auto playerStatsCollection = db[PLAYER_STATS_COLLECTION];

    // Check if already unlocked
    auto existing = achievementsCollection.find_one(
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

    auto result = achievementsCollection.insert_one(doc.view());

    // Also update the bitmask in player_stats
    if (result) {
        playerStatsCollection.update_one(
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
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto currentGameSessionsCollection = db[CURRENT_GAME_SESSIONS_COLLECTION];

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

    currentGameSessionsCollection.update_one(
        make_document(kvp("email", email)),
        make_document(kvp("$set", doc)),
        options
    );
}

void MongoDBLeaderboardRepository::finalizeGameSession(
    const std::string& email, const std::string& playerName)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto currentGameSessionsCollection = db[CURRENT_GAME_SESSIONS_COLLECTION];
    auto playerStatsCollection = db[PLAYER_STATS_COLLECTION];
    auto gameHistoryCollection = db[GAME_HISTORY_COLLECTION];

    auto logger = server::logging::Logger::getGameLogger();
    logger->debug("[MongoDB] finalizeGameSession: email={}, playerName={}", email, playerName);

    // Find the current game session
    auto sessionResult = currentGameSessionsCollection.find_one(
        make_document(kvp("email", email))
    );

    if (!sessionResult) {
        logger->debug("[MongoDB] finalizeGameSession: No current_game_session found for email={}", email);
        return;  // No session to finalize
    }

    auto sessionDoc = sessionResult->view();
    GameHistoryEntry session;
    if (sessionDoc["email"]) session.odId = std::string(sessionDoc["email"].get_string().value);
    if (sessionDoc["playerName"]) session.playerName = std::string(sessionDoc["playerName"].get_string().value);
    if (sessionDoc["score"]) session.score = static_cast<uint32_t>(getInt64Safe(sessionDoc["score"]));
    if (sessionDoc["wave"]) session.wave = static_cast<uint16_t>(getInt32Safe(sessionDoc["wave"]));
    if (sessionDoc["kills"]) session.kills = static_cast<uint16_t>(getInt32Safe(sessionDoc["kills"]));
    if (sessionDoc["deaths"]) session.deaths = static_cast<uint8_t>(getInt32Safe(sessionDoc["deaths"]));
    if (sessionDoc["duration"]) session.duration = static_cast<uint32_t>(getInt64Safe(sessionDoc["duration"]));
    if (sessionDoc["standardKills"]) session.standardKills = static_cast<uint32_t>(getInt64Safe(sessionDoc["standardKills"]));
    if (sessionDoc["spreadKills"]) session.spreadKills = static_cast<uint32_t>(getInt64Safe(sessionDoc["spreadKills"]));
    if (sessionDoc["laserKills"]) session.laserKills = static_cast<uint32_t>(getInt64Safe(sessionDoc["laserKills"]));
    if (sessionDoc["missileKills"]) session.missileKills = static_cast<uint32_t>(getInt64Safe(sessionDoc["missileKills"]));
    if (sessionDoc["waveCannonKills"]) session.waveCannonKills = static_cast<uint32_t>(getInt64Safe(sessionDoc["waveCannonKills"]));
    if (sessionDoc["bossKills"]) session.bossKills = static_cast<uint8_t>(getInt32Safe(sessionDoc["bossKills"]));
    if (sessionDoc["bestCombo"]) session.bestCombo = static_cast<uint16_t>(getInt32Safe(sessionDoc["bestCombo"]));
    if (sessionDoc["bestKillStreak"]) session.bestKillStreak = static_cast<uint16_t>(getInt32Safe(sessionDoc["bestKillStreak"]));
    if (sessionDoc["bestWaveStreak"]) session.bestWaveStreak = static_cast<uint16_t>(getInt32Safe(sessionDoc["bestWaveStreak"]));
    if (sessionDoc["perfectWaves"]) session.perfectWaves = static_cast<uint16_t>(getInt32Safe(sessionDoc["perfectWaves"]));
    if (sessionDoc["totalDamageDealt"]) session.totalDamageDealt = static_cast<uint64_t>(getInt64Safe(sessionDoc["totalDamageDealt"]));
    if (sessionDoc["bossDefeated"]) session.bossDefeated = sessionDoc["bossDefeated"].get_bool().value;

    // Only finalize if there's actual game data
    if (session.score == 0 && session.kills == 0) {
        currentGameSessionsCollection.delete_one(make_document(kvp("email", email)));
        return;
    }

    // Get existing stats or create new
    auto existingResult = playerStatsCollection.find_one(make_document(kvp("email", email)));
    PlayerStats stats;
    if (existingResult) {
        stats = documentToPlayerStats(existingResult->view());
    }
    stats.odId = email;
    stats.playerName = playerName;

    // Update cumulative stats
    stats.totalScore += session.score;
    stats.totalKills += session.kills;
    stats.totalDeaths += session.deaths;
    stats.totalPlaytime += session.duration;
    stats.gamesPlayed++;

    // Update best records
    if (session.score > stats.bestScore) stats.bestScore = session.score;
    if (session.wave > stats.bestWave) stats.bestWave = session.wave;
    if (session.bestKillStreak > stats.bestKillStreak) stats.bestKillStreak = session.bestKillStreak;
    if (session.bestWaveStreak > stats.bestWaveStreak) stats.bestWaveStreak = session.bestWaveStreak;
    if (session.bestCombo > stats.bestCombo) stats.bestCombo = session.bestCombo;
    stats.totalPerfectWaves += session.perfectWaves;

    // Update weapon kills
    if (session.standardKills > 0 || session.spreadKills > 0 ||
        session.laserKills > 0 || session.missileKills > 0 || session.waveCannonKills > 0) {
        stats.standardKills += session.standardKills;
        stats.spreadKills += session.spreadKills;
        stats.laserKills += session.laserKills;
        stats.missileKills += session.missileKills;
        stats.waveCannonKills += session.waveCannonKills;
    }

    if (session.bossKills > 0) {
        stats.bossKills += session.bossKills;
    } else if (session.bossDefeated) {
        stats.bossKills++;
    }

    stats.totalDamageDealt += session.totalDamageDealt;

    // Save updated stats
    auto statsDoc = make_document(
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

    playerStatsCollection.update_one(
        make_document(kvp("email", email)),
        make_document(kvp("$set", statsDoc)),
        options
    );

    // Also save to game history
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // Calculate favorite weapon from kills per weapon
    uint8_t favoriteWeapon = 0;  // Standard by default
    uint32_t maxKills = session.standardKills;
    if (session.spreadKills > maxKills) { favoriteWeapon = 1; maxKills = session.spreadKills; }
    if (session.laserKills > maxKills) { favoriteWeapon = 2; maxKills = session.laserKills; }
    if (session.missileKills > maxKills) { favoriteWeapon = 3; maxKills = session.missileKills; }
    if (session.waveCannonKills > maxKills) { favoriteWeapon = 4; }

    auto historyDoc = make_document(
        kvp("email", email),
        kvp("playerName", playerName),
        kvp("score", static_cast<int64_t>(session.score)),
        kvp("wave", static_cast<int32_t>(session.wave)),
        kvp("kills", static_cast<int32_t>(session.kills)),
        kvp("deaths", static_cast<int32_t>(session.deaths)),
        kvp("duration", static_cast<int64_t>(session.duration)),
        kvp("timestamp", timestamp),
        kvp("weaponUsed", static_cast<int32_t>(favoriteWeapon)),
        kvp("bossDefeated", session.bossDefeated),
        kvp("bestCombo", static_cast<int32_t>(session.bestCombo)),
        kvp("playerCount", static_cast<int32_t>(session.playerCount))
    );

    gameHistoryCollection.insert_one(historyDoc.view());

    // Delete the current session
    currentGameSessionsCollection.delete_one(make_document(kvp("email", email)));
    logger->info("[Leaderboard] Stats saved for {} (score={}, kills={}, wave={})",
                playerName, session.score, session.kills, session.wave);
}

std::optional<GameHistoryEntry> MongoDBLeaderboardRepository::getCurrentGameSession(
    const std::string& email)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto currentGameSessionsCollection = db[CURRENT_GAME_SESSIONS_COLLECTION];

    auto result = currentGameSessionsCollection.find_one(
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
