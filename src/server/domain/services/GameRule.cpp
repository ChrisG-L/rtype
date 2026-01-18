/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameRule - Implementation
*/

#include "domain/services/GameRule.hpp"
#include "domain/Constants.hpp"
#include <algorithm>
#include <cmath>

namespace domain::services {

    using namespace domain::constants;

    // ═══════════════════════════════════════════════════════════════════
    // Weapon Calculations
    // ═══════════════════════════════════════════════════════════════════

    uint8_t GameRule::getMissileDamage(uint8_t weaponType, uint8_t weaponLevel) const {
        // Get base damage for weapon type
        uint8_t baseDamage = 0;
        switch (weaponType) {
            case 0: baseDamage = weapon::DAMAGE_STANDARD; break;
            case 1: baseDamage = weapon::DAMAGE_SPREAD; break;
            case 2: baseDamage = weapon::DAMAGE_LASER; break;
            case 3: baseDamage = weapon::DAMAGE_MISSILE; break;
            default: baseDamage = weapon::DAMAGE_STANDARD; break;
        }

        // Get damage multiplier for level
        uint8_t multiplier = weapon::DAMAGE_MULT_LV0;
        switch (weaponLevel) {
            case 1: multiplier = weapon::DAMAGE_MULT_LV1; break;
            case 2: multiplier = weapon::DAMAGE_MULT_LV2; break;
            case 3: multiplier = weapon::DAMAGE_MULT_LV3; break;
            default: break;
        }

        return static_cast<uint8_t>(baseDamage * multiplier / 100);
    }

    float GameRule::getMissileSpeed(uint8_t weaponType, uint8_t weaponLevel) const {
        // Get base speed for weapon type
        float baseSpeed = 0.0f;
        switch (weaponType) {
            case 0: baseSpeed = weapon::SPEED_STANDARD; break;
            case 1: baseSpeed = weapon::SPEED_SPREAD; break;
            case 2: baseSpeed = weapon::SPEED_LASER; break;
            case 3: baseSpeed = weapon::SPEED_MISSILE; break;
            default: baseSpeed = weapon::SPEED_STANDARD; break;
        }

        // Get speed multiplier for level (only LV3 gets bonus)
        uint8_t multiplier = weapon::SPEED_MULT_LV0;
        if (weaponLevel == 3) {
            multiplier = weapon::SPEED_MULT_LV3;
        }

        return baseSpeed * multiplier / 100.0f;
    }

    float GameRule::getWeaponCooldown(uint8_t weaponType, uint8_t weaponLevel) const {
        // Get base cooldown for weapon type
        float baseCooldown = 0.0f;
        switch (weaponType) {
            case 0: baseCooldown = weapon::COOLDOWN_STANDARD; break;
            case 1: baseCooldown = weapon::COOLDOWN_SPREAD; break;
            case 2: baseCooldown = weapon::COOLDOWN_LASER; break;
            case 3: baseCooldown = weapon::COOLDOWN_MISSILE; break;
            default: baseCooldown = weapon::COOLDOWN_STANDARD; break;
        }

        // Get cooldown multiplier for level (lower = faster)
        uint8_t multiplier = weapon::COOLDOWN_MULT_LV0;
        switch (weaponLevel) {
            case 1: multiplier = weapon::COOLDOWN_MULT_LV1; break;
            case 2: multiplier = weapon::COOLDOWN_MULT_LV2; break;
            case 3: multiplier = weapon::COOLDOWN_MULT_LV3; break;
            default: break;
        }

        return baseCooldown * multiplier / 100.0f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Score Calculations
    // ═══════════════════════════════════════════════════════════════════

    uint16_t GameRule::getEnemyPointValue(uint8_t enemyType) const {
        switch (enemyType) {
            case 0: return score::POINTS_BASIC;
            case 1: return score::POINTS_TRACKER;
            case 2: return score::POINTS_ZIGZAG;
            case 3: return score::POINTS_FAST;
            case 4: return score::POINTS_BOMBER;
            case 5: return score::POINTS_POW_ARMOR;
            default: return score::POINTS_BASIC;
        }
    }

    uint32_t GameRule::applyComboBonus(uint16_t basePoints, float comboMultiplier) const {
        return static_cast<uint32_t>(basePoints * comboMultiplier);
    }

    float GameRule::incrementCombo(float currentCombo) const {
        return std::min(currentCombo + score::COMBO_INCREMENT, score::COMBO_MAX);
    }

    float GameRule::decayCombo(float currentCombo, float deltaTime, float timeSinceLastKill) const {
        // No decay during grace period
        if (timeSinceLastKill < score::COMBO_GRACE_TIME) {
            return currentCombo;
        }

        // Decay after grace period
        float newCombo = currentCombo - (score::COMBO_DECAY_RATE * deltaTime);
        return std::max(newCombo, 1.0f);  // Minimum combo is 1.0x
    }

    float GameRule::getComboGraceTime() const {
        return score::COMBO_GRACE_TIME;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Health & Death
    // ═══════════════════════════════════════════════════════════════════

    bool GameRule::shouldEntityDie(uint16_t currentHP, uint16_t damage) const {
        return damage >= currentHP;
    }

    uint16_t GameRule::applyDamage(uint16_t currentHP, uint16_t damage) const {
        if (damage >= currentHP) {
            return 0;
        }
        return currentHP - damage;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Player Movement
    // ═══════════════════════════════════════════════════════════════════

    float GameRule::getSpeedMultiplier(uint8_t speedLevel) const {
        switch (speedLevel) {
            case 0: return player::SPEED_MULT_LV0;
            case 1: return player::SPEED_MULT_LV1;
            case 2: return player::SPEED_MULT_LV2;
            case 3: return player::SPEED_MULT_LV3;
            default: return player::SPEED_MULT_LV3;  // Cap at max
        }
    }

    float GameRule::getPlayerBaseSpeed() const {
        return player::MOVE_SPEED;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Enemy Statistics
    // ═══════════════════════════════════════════════════════════════════

    uint8_t GameRule::getEnemyHealth(uint8_t enemyType) const {
        switch (enemyType) {
            case 0: return enemy::HEALTH_BASIC;
            case 1: return enemy::HEALTH_TRACKER;
            case 2: return enemy::HEALTH_ZIGZAG;
            case 3: return enemy::HEALTH_FAST;
            case 4: return enemy::HEALTH_BOMBER;
            case 5: return enemy::HEALTH_POW_ARMOR;
            default: return enemy::HEALTH_BASIC;
        }
    }

    float GameRule::getEnemySpeed(uint8_t enemyType) const {
        switch (enemyType) {
            case 0: return enemy::SPEED_X_BASIC;
            case 1: return enemy::SPEED_X_TRACKER;
            case 2: return enemy::SPEED_X_ZIGZAG;
            case 3: return enemy::SPEED_X_FAST;
            case 4: return enemy::SPEED_X_BOMBER;
            case 5: return enemy::SPEED_X_POW_ARMOR;
            default: return enemy::SPEED_X_BASIC;
        }
    }

    float GameRule::getEnemyShootInterval(uint8_t enemyType) const {
        switch (enemyType) {
            case 0: return enemy::SHOOT_INTERVAL_BASIC;
            case 1: return enemy::SHOOT_INTERVAL_TRACKER;
            case 2: return enemy::SHOOT_INTERVAL_ZIGZAG;
            case 3: return enemy::SHOOT_INTERVAL_FAST;
            case 4: return enemy::SHOOT_INTERVAL_BOMBER;
            case 5: return enemy::SHOOT_INTERVAL_POW_ARMOR;
            default: return enemy::SHOOT_INTERVAL_BASIC;
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // Wave Cannon
    // ═══════════════════════════════════════════════════════════════════

    uint8_t GameRule::getWaveCannonDamage(uint8_t chargeLevel) const {
        switch (chargeLevel) {
            case 1: return wavecannon::DAMAGE_LV1;
            case 2: return wavecannon::DAMAGE_LV2;
            case 3: return wavecannon::DAMAGE_LV3;
            default: return wavecannon::DAMAGE_LV1;
        }
    }

    float GameRule::getWaveCannonWidth(uint8_t chargeLevel) const {
        switch (chargeLevel) {
            case 1: return wavecannon::WIDTH_LV1;
            case 2: return wavecannon::WIDTH_LV2;
            case 3: return wavecannon::WIDTH_LV3;
            default: return wavecannon::WIDTH_LV1;
        }
    }

    float GameRule::getWaveCannonChargeTime(uint8_t chargeLevel) const {
        switch (chargeLevel) {
            case 1: return wavecannon::CHARGE_TIME_LV1;
            case 2: return wavecannon::CHARGE_TIME_LV2;
            case 3: return wavecannon::CHARGE_TIME_LV3;
            default: return wavecannon::CHARGE_TIME_LV1;
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // Boss
    // ═══════════════════════════════════════════════════════════════════

    uint16_t GameRule::getBossMaxHealth(uint8_t playerCount, uint8_t defeatCount) const {
        // Base HP + scaling per defeat + scaling per player
        // Formula from GameWorld: base + 500×defeatCount + 1000×(playerCount-1)
        uint16_t health = boss::MAX_HEALTH;
        health += static_cast<uint16_t>(defeatCount * 500);
        if (playerCount > 1) {
            health += static_cast<uint16_t>((playerCount - 1) * boss::HP_PER_PLAYER);
        }
        return health;
    }

    bool GameRule::shouldBossChangePhase(uint16_t currentHP, uint16_t maxHP, uint8_t phase) const {
        if (maxHP == 0) return false;

        float healthPercent = static_cast<float>(currentHP) / static_cast<float>(maxHP);

        switch (phase) {
            case 2: return healthPercent <= boss::PHASE2_THRESHOLD;
            case 3: return healthPercent <= boss::PHASE3_THRESHOLD;
            default: return false;
        }
    }

}  // namespace domain::services
