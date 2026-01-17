/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AchievementChecker - Checks and awards achievements based on game stats
*/

#ifndef ACHIEVEMENTCHECKER_HPP_
#define ACHIEVEMENTCHECKER_HPP_

#include "application/ports/out/persistence/ILeaderboardRepository.hpp"
#include <vector>
#include <string>

namespace application::services {

using ports::out::persistence::ILeaderboardRepository;
using ports::out::persistence::AchievementType;
using ports::out::persistence::PlayerStats;
using ports::out::persistence::GameHistoryEntry;

/**
 * @brief Service to check and award achievements after a game
 *
 * Achievement conditions:
 * - FirstBlood: Get your first kill
 * - Exterminator: Total 1000 kills across all games
 * - ComboMaster: Achieve a 3.0x combo multiplier
 * - BossSlayer: Defeat a boss
 * - Survivor: Reach wave 20 without dying
 * - SpeedDemon: Complete wave 10 in under 5 minutes
 * - Perfectionist: Complete a wave without taking damage
 * - Veteran: Play 100 games
 * - Untouchable: Complete a game without dying
 * - WeaponMaster: Get 100+ kills with all 4 weapon types
 */
class AchievementChecker {
public:
    /**
     * @brief Check for newly unlocked achievements after a game
     *
     * @param repo Repository to check/unlock achievements
     * @param email Player's email (identifier)
     * @param stats Player's cumulative stats (already updated with this game)
     * @param gameData Data from the just-completed game
     * @param bestComboThisGame Highest combo achieved this game (x10, e.g. 30 = 3.0x)
     * @param tookDamageThisGame Whether player took any damage this game
     * @param killedBossThisGame Whether player killed a boss this game
     *
     * @return Vector of newly unlocked achievement types
     */
    static std::vector<AchievementType> checkAndUnlock(
        ILeaderboardRepository& repo,
        const std::string& email,
        const PlayerStats& stats,
        const GameHistoryEntry& gameData,
        uint16_t bestComboThisGame,
        bool tookDamageThisGame,
        bool killedBossThisGame);

    // Threshold constants
    static constexpr uint32_t KILLS_FOR_EXTERMINATOR = 1000;
    static constexpr uint16_t COMBO_FOR_MASTER = 30;  // 3.0x = 30 encoded
    static constexpr uint16_t WAVE_FOR_SURVIVOR = 20;
    static constexpr uint32_t DURATION_FOR_SPEEDDEMON = 300;  // 5 minutes in seconds
    static constexpr uint16_t WAVE_FOR_SPEEDDEMON = 10;
    static constexpr uint32_t GAMES_FOR_VETERAN = 100;
    static constexpr uint32_t KILLS_PER_WEAPON_FOR_MASTER = 100;

private:
    // Individual achievement checks
    static bool checkFirstBlood(const PlayerStats& stats);
    static bool checkExterminator(const PlayerStats& stats);
    static bool checkComboMaster(uint16_t bestComboThisGame);
    static bool checkBossSlayer(bool killedBossThisGame);
    static bool checkSurvivor(const GameHistoryEntry& gameData, bool tookDamageThisGame);
    static bool checkSpeedDemon(const GameHistoryEntry& gameData);
    static bool checkPerfectionist(bool tookDamageThisGame, uint16_t wave);
    static bool checkVeteran(const PlayerStats& stats);
    static bool checkUntouchable(const GameHistoryEntry& gameData);
    static bool checkWeaponMaster(const PlayerStats& stats);
};

} // namespace application::services

#endif /* !ACHIEVEMENTCHECKER_HPP_ */
