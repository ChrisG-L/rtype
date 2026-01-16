/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AchievementChecker - Checks and awards achievements based on game stats
*/

#include "application/services/AchievementChecker.hpp"

namespace application::services {

std::vector<AchievementType> AchievementChecker::checkAndUnlock(
    ILeaderboardRepository& repo,
    const std::string& email,
    const PlayerStats& stats,
    const GameHistoryEntry& gameData,
    uint16_t bestComboThisGame,
    bool tookDamageThisGame,
    bool killedBossThisGame)
{
    std::vector<AchievementType> newlyUnlocked;

    // Check each achievement and try to unlock if condition is met
    auto tryUnlock = [&](AchievementType type, bool condition) {
        if (condition && !stats.hasAchievement(type)) {
            if (repo.unlockAchievement(email, type)) {
                newlyUnlocked.push_back(type);
            }
        }
    };

    // First Blood - Get your first kill (check cumulative stats)
    tryUnlock(AchievementType::FirstBlood, checkFirstBlood(stats));

    // Exterminator - 1000 total kills
    tryUnlock(AchievementType::Exterminator, checkExterminator(stats));

    // Combo Master - Achieve 3.0x combo
    tryUnlock(AchievementType::ComboMaster, checkComboMaster(bestComboThisGame));

    // Boss Slayer - Kill a boss
    tryUnlock(AchievementType::BossSlayer, checkBossSlayer(killedBossThisGame));

    // Survivor - Reach wave 20 without dying (0 deaths this game)
    tryUnlock(AchievementType::Survivor, checkSurvivor(gameData, tookDamageThisGame));

    // Speed Demon - Complete wave 10 in under 5 minutes
    tryUnlock(AchievementType::SpeedDemon, checkSpeedDemon(gameData));

    // Perfectionist - Complete a wave without taking damage
    tryUnlock(AchievementType::Perfectionist, checkPerfectionist(tookDamageThisGame, gameData.wave));

    // Veteran - Play 100 games
    tryUnlock(AchievementType::Veteran, checkVeteran(stats));

    // Untouchable - Complete a game without dying (0 deaths)
    tryUnlock(AchievementType::Untouchable, checkUntouchable(gameData));

    // Weapon Master - 100+ kills with all 4 weapon types
    tryUnlock(AchievementType::WeaponMaster, checkWeaponMaster(stats));

    return newlyUnlocked;
}

bool AchievementChecker::checkFirstBlood(const PlayerStats& stats) {
    return stats.totalKills >= 1;
}

bool AchievementChecker::checkExterminator(const PlayerStats& stats) {
    return stats.totalKills >= KILLS_FOR_EXTERMINATOR;
}

bool AchievementChecker::checkComboMaster(uint16_t bestComboThisGame) {
    return bestComboThisGame >= COMBO_FOR_MASTER;
}

bool AchievementChecker::checkBossSlayer(bool killedBossThisGame) {
    return killedBossThisGame;
}

bool AchievementChecker::checkSurvivor(const GameHistoryEntry& gameData, bool tookDamageThisGame) {
    // Must reach wave 20+ without dying (0 deaths AND wave >= 20)
    return gameData.wave >= WAVE_FOR_SURVIVOR && gameData.deaths == 0;
}

bool AchievementChecker::checkSpeedDemon(const GameHistoryEntry& gameData) {
    // Reach wave 10 in under 5 minutes (300 seconds)
    return gameData.wave >= WAVE_FOR_SPEEDDEMON && gameData.duration <= DURATION_FOR_SPEEDDEMON;
}

bool AchievementChecker::checkPerfectionist(bool tookDamageThisGame, uint16_t wave) {
    // Complete at least 1 wave without taking any damage
    return !tookDamageThisGame && wave >= 1;
}

bool AchievementChecker::checkVeteran(const PlayerStats& stats) {
    return stats.gamesPlayed >= GAMES_FOR_VETERAN;
}

bool AchievementChecker::checkUntouchable(const GameHistoryEntry& gameData) {
    // Complete a game (wave >= 1) without dying
    return gameData.deaths == 0 && gameData.wave >= 1;
}

bool AchievementChecker::checkWeaponMaster(const PlayerStats& stats) {
    return stats.standardKills >= KILLS_PER_WEAPON_FOR_MASTER
        && stats.spreadKills >= KILLS_PER_WEAPON_FOR_MASTER
        && stats.laserKills >= KILLS_PER_WEAPON_FOR_MASTER
        && stats.missileKills >= KILLS_PER_WEAPON_FOR_MASTER;
}

} // namespace application::services
