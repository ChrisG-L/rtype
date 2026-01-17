/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameRule - Domain service for game logic calculations
*/

#ifndef GAMERULE_HPP_
#define GAMERULE_HPP_

#include <cstdint>

namespace domain::services {

    /**
     * @brief Domain service encapsulating all game rule calculations.
     *
     * This service provides pure functions for calculating:
     * - Weapon damage, speed, and cooldowns
     * - Score and combo system
     * - Enemy statistics
     * - Wave Cannon mechanics
     * - Player speed multipliers
     *
     * All methods are const and stateless - they only depend on their inputs
     * and the constants defined in domain/Constants.hpp.
     */
    class GameRule {
    public:
        GameRule() = default;
        ~GameRule() = default;

        // ═══════════════════════════════════════════════════════════════════
        // Weapon Calculations
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get missile damage for a weapon type and level.
         * @param weaponType 0=Standard, 1=Spread, 2=Laser, 3=Missile
         * @param weaponLevel 0-3
         * @return Damage value
         */
        uint8_t getMissileDamage(uint8_t weaponType, uint8_t weaponLevel) const;

        /**
         * @brief Get missile speed for a weapon type and level.
         * @param weaponType 0=Standard, 1=Spread, 2=Laser, 3=Missile
         * @param weaponLevel 0-3
         * @return Speed in pixels per second
         */
        float getMissileSpeed(uint8_t weaponType, uint8_t weaponLevel) const;

        /**
         * @brief Get weapon cooldown for a weapon type and level.
         * @param weaponType 0=Standard, 1=Spread, 2=Laser, 3=Missile
         * @param weaponLevel 0-3
         * @return Cooldown in seconds
         */
        float getWeaponCooldown(uint8_t weaponType, uint8_t weaponLevel) const;

        // ═══════════════════════════════════════════════════════════════════
        // Score Calculations
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get base point value for an enemy type.
         * @param enemyType 0=Basic, 1=Tracker, 2=Zigzag, 3=Fast, 4=Bomber, 5=POWArmor
         * @return Base points
         */
        uint16_t getEnemyPointValue(uint8_t enemyType) const;

        /**
         * @brief Apply combo multiplier to base points.
         * @param basePoints Points before combo
         * @param comboMultiplier Current combo multiplier (1.0-3.0)
         * @return Final score with combo applied
         */
        uint32_t applyComboBonus(uint16_t basePoints, float comboMultiplier) const;

        /**
         * @brief Increment combo after a kill.
         * @param currentCombo Current combo multiplier
         * @return New combo multiplier (capped at COMBO_MAX)
         */
        float incrementCombo(float currentCombo) const;

        /**
         * @brief Decay combo over time.
         * @param currentCombo Current combo multiplier
         * @param deltaTime Time elapsed since last update
         * @param timeSinceLastKill Time since last kill (for grace period)
         * @return New combo multiplier
         */
        float decayCombo(float currentCombo, float deltaTime, float timeSinceLastKill) const;

        /**
         * @brief Get the grace time before combo starts decaying.
         * @return Grace time in seconds
         */
        float getComboGraceTime() const;

        // ═══════════════════════════════════════════════════════════════════
        // Health & Death
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Check if damage is lethal.
         * @param currentHP Current health points
         * @param damage Incoming damage
         * @return true if entity should die
         */
        bool shouldEntityDie(uint16_t currentHP, uint16_t damage) const;

        /**
         * @brief Calculate remaining HP after damage.
         * @param currentHP Current health points
         * @param damage Incoming damage
         * @return Remaining HP (minimum 0)
         */
        uint16_t applyDamage(uint16_t currentHP, uint16_t damage) const;

        // ═══════════════════════════════════════════════════════════════════
        // Player Movement
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get speed multiplier for player speed level.
         * @param speedLevel 0-3
         * @return Multiplier (1.0, 1.3, 1.6, or 1.9)
         */
        float getSpeedMultiplier(uint8_t speedLevel) const;

        /**
         * @brief Get player base move speed.
         * @return Speed in pixels per second
         */
        float getPlayerBaseSpeed() const;

        // ═══════════════════════════════════════════════════════════════════
        // Enemy Statistics
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get max health for an enemy type.
         * @param enemyType 0=Basic, 1=Tracker, 2=Zigzag, 3=Fast, 4=Bomber, 5=POWArmor
         * @return Health value
         */
        uint8_t getEnemyHealth(uint8_t enemyType) const;

        /**
         * @brief Get horizontal speed for an enemy type.
         * @param enemyType 0=Basic, 1=Tracker, etc.
         * @return Speed in pixels per second (negative = moving left)
         */
        float getEnemySpeed(uint8_t enemyType) const;

        /**
         * @brief Get shoot interval for an enemy type.
         * @param enemyType 0=Basic, 1=Tracker, etc.
         * @return Interval in seconds
         */
        float getEnemyShootInterval(uint8_t enemyType) const;

        // ═══════════════════════════════════════════════════════════════════
        // Wave Cannon
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get damage for a Wave Cannon charge level.
         * @param chargeLevel 1-3
         * @return Damage value
         */
        uint8_t getWaveCannonDamage(uint8_t chargeLevel) const;

        /**
         * @brief Get beam width for a Wave Cannon charge level.
         * @param chargeLevel 1-3
         * @return Width in pixels
         */
        float getWaveCannonWidth(uint8_t chargeLevel) const;

        /**
         * @brief Get charge time required for a level.
         * @param chargeLevel 1-3
         * @return Time in seconds
         */
        float getWaveCannonChargeTime(uint8_t chargeLevel) const;

        // ═══════════════════════════════════════════════════════════════════
        // Boss
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get boss max health based on player count and defeat count.
         * @param playerCount Number of players in game
         * @param defeatCount How many times boss has been defeated
         * @return Max health value
         */
        uint16_t getBossMaxHealth(uint8_t playerCount, uint8_t defeatCount) const;

        /**
         * @brief Check if boss should transition to a phase.
         * @param currentHP Current boss HP
         * @param maxHP Boss max HP
         * @param phase 2 or 3
         * @return true if should transition to that phase
         */
        bool shouldBossChangePhase(uint16_t currentHP, uint16_t maxHP, uint8_t phase) const;
    };

}  // namespace domain::services

#endif /* !GAMERULE_HPP_ */
