/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponLevelSystemTest - Tests for Weapon Level System (damage/cooldown/speed multipliers)
*/

#include <gtest/gtest.h>
#include "infrastructure/game/GameWorld.hpp"
#include "Protocol.hpp"

using namespace infrastructure::game;
using Missile = infrastructure::game::Missile;

// ============================================================================
// Tests - Weapon Level System Constants
// ============================================================================

class WeaponLevelConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WeaponLevelConstantsTest, DamageMultiplierLevel0) {
    EXPECT_EQ(Missile::DAMAGE_MULT_LV0, 100);  // 1.0x
}

TEST_F(WeaponLevelConstantsTest, DamageMultiplierLevel1) {
    EXPECT_EQ(Missile::DAMAGE_MULT_LV1, 115);  // 1.15x (+15%)
}

TEST_F(WeaponLevelConstantsTest, DamageMultiplierLevel2) {
    EXPECT_EQ(Missile::DAMAGE_MULT_LV2, 130);  // 1.30x (+30%)
}

TEST_F(WeaponLevelConstantsTest, DamageMultiplierLevel3) {
    EXPECT_EQ(Missile::DAMAGE_MULT_LV3, 150);  // 1.50x (+50%)
}

TEST_F(WeaponLevelConstantsTest, CooldownMultiplierLevel0) {
    EXPECT_EQ(Missile::COOLDOWN_MULT_LV0, 100);  // 1.0x
}

TEST_F(WeaponLevelConstantsTest, CooldownMultiplierLevel1) {
    EXPECT_EQ(Missile::COOLDOWN_MULT_LV1, 95);  // 0.95x (-5%)
}

TEST_F(WeaponLevelConstantsTest, CooldownMultiplierLevel2) {
    EXPECT_EQ(Missile::COOLDOWN_MULT_LV2, 90);  // 0.90x (-10%)
}

TEST_F(WeaponLevelConstantsTest, CooldownMultiplierLevel3) {
    EXPECT_EQ(Missile::COOLDOWN_MULT_LV3, 85);  // 0.85x (-15%)
}

TEST_F(WeaponLevelConstantsTest, SpeedMultiplierLevel0) {
    EXPECT_EQ(Missile::SPEED_MULT_LV0, 100);  // 1.0x
}

TEST_F(WeaponLevelConstantsTest, SpeedMultiplierLevel1) {
    EXPECT_EQ(Missile::SPEED_MULT_LV1, 100);  // 1.0x (no bonus)
}

TEST_F(WeaponLevelConstantsTest, SpeedMultiplierLevel2) {
    EXPECT_EQ(Missile::SPEED_MULT_LV2, 100);  // 1.0x (no bonus)
}

TEST_F(WeaponLevelConstantsTest, SpeedMultiplierLevel3) {
    EXPECT_EQ(Missile::SPEED_MULT_LV3, 110);  // 1.10x (+10%)
}

// ============================================================================
// Tests - Weapon Base Stats
// ============================================================================

class WeaponBaseStatsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WeaponBaseStatsTest, StandardBaseDamage) {
    EXPECT_EQ(Missile::DAMAGE_STANDARD, 20);
}

TEST_F(WeaponBaseStatsTest, SpreadBaseDamage) {
    EXPECT_EQ(Missile::DAMAGE_SPREAD, 8);
}

TEST_F(WeaponBaseStatsTest, LaserBaseDamage) {
    EXPECT_EQ(Missile::DAMAGE_LASER, 12);
}

TEST_F(WeaponBaseStatsTest, HomingBaseDamage) {
    EXPECT_EQ(Missile::DAMAGE_MISSILE, 50);
}

TEST_F(WeaponBaseStatsTest, StandardBaseCooldown) {
    EXPECT_FLOAT_EQ(Missile::COOLDOWN_STANDARD, 0.3f);
}

TEST_F(WeaponBaseStatsTest, SpreadBaseCooldown) {
    EXPECT_FLOAT_EQ(Missile::COOLDOWN_SPREAD, 0.4f);
}

TEST_F(WeaponBaseStatsTest, LaserBaseCooldown) {
    EXPECT_FLOAT_EQ(Missile::COOLDOWN_LASER, 0.18f);
}

TEST_F(WeaponBaseStatsTest, HomingBaseCooldown) {
    EXPECT_FLOAT_EQ(Missile::COOLDOWN_MISSILE, 0.7f);
}

TEST_F(WeaponBaseStatsTest, StandardBaseSpeed) {
    EXPECT_FLOAT_EQ(Missile::SPEED_STANDARD, 600.0f);
}

TEST_F(WeaponBaseStatsTest, SpreadBaseSpeed) {
    EXPECT_FLOAT_EQ(Missile::SPEED_SPREAD, 550.0f);
}

TEST_F(WeaponBaseStatsTest, LaserBaseSpeed) {
    EXPECT_FLOAT_EQ(Missile::SPEED_LASER, 900.0f);
}

TEST_F(WeaponBaseStatsTest, HomingBaseSpeed) {
    EXPECT_FLOAT_EQ(Missile::SPEED_MISSILE, 350.0f);
}

// ============================================================================
// Tests - getDamageMultiplier() Function
// ============================================================================

class GetDamageMultiplierTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetDamageMultiplierTest, Level0Returns100) {
    EXPECT_EQ(Missile::getDamageMultiplier(0), 100);
}

TEST_F(GetDamageMultiplierTest, Level1Returns115) {
    EXPECT_EQ(Missile::getDamageMultiplier(1), 115);
}

TEST_F(GetDamageMultiplierTest, Level2Returns130) {
    EXPECT_EQ(Missile::getDamageMultiplier(2), 130);
}

TEST_F(GetDamageMultiplierTest, Level3Returns150) {
    EXPECT_EQ(Missile::getDamageMultiplier(3), 150);
}

TEST_F(GetDamageMultiplierTest, InvalidLevelReturnsDefault) {
    // Level > 3 should return level 0 multiplier (default)
    EXPECT_EQ(Missile::getDamageMultiplier(4), 100);
    EXPECT_EQ(Missile::getDamageMultiplier(255), 100);
}

// ============================================================================
// Tests - getCooldownMultiplier() Function
// ============================================================================

class GetCooldownMultiplierTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetCooldownMultiplierTest, Level0Returns100) {
    EXPECT_EQ(Missile::getCooldownMultiplier(0), 100);
}

TEST_F(GetCooldownMultiplierTest, Level1Returns95) {
    EXPECT_EQ(Missile::getCooldownMultiplier(1), 95);
}

TEST_F(GetCooldownMultiplierTest, Level2Returns90) {
    EXPECT_EQ(Missile::getCooldownMultiplier(2), 90);
}

TEST_F(GetCooldownMultiplierTest, Level3Returns85) {
    EXPECT_EQ(Missile::getCooldownMultiplier(3), 85);
}

TEST_F(GetCooldownMultiplierTest, InvalidLevelReturnsDefault) {
    EXPECT_EQ(Missile::getCooldownMultiplier(4), 100);
    EXPECT_EQ(Missile::getCooldownMultiplier(255), 100);
}

// ============================================================================
// Tests - getSpeedMultiplier() Function
// ============================================================================

class GetSpeedMultiplierTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetSpeedMultiplierTest, Level0Returns100) {
    EXPECT_EQ(Missile::getSpeedMultiplier(0), 100);
}

TEST_F(GetSpeedMultiplierTest, Level1Returns100) {
    // No speed bonus until level 3
    EXPECT_EQ(Missile::getSpeedMultiplier(1), 100);
}

TEST_F(GetSpeedMultiplierTest, Level2Returns100) {
    // No speed bonus until level 3
    EXPECT_EQ(Missile::getSpeedMultiplier(2), 100);
}

TEST_F(GetSpeedMultiplierTest, Level3Returns110) {
    EXPECT_EQ(Missile::getSpeedMultiplier(3), 110);
}

TEST_F(GetSpeedMultiplierTest, InvalidLevelReturnsDefault) {
    EXPECT_EQ(Missile::getSpeedMultiplier(4), 100);
}

// ============================================================================
// Tests - getBaseSpeed() Function
// ============================================================================

class GetBaseSpeedTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetBaseSpeedTest, StandardType) {
    EXPECT_FLOAT_EQ(Missile::getBaseSpeed(WeaponType::Standard), 600.0f);
}

TEST_F(GetBaseSpeedTest, SpreadType) {
    EXPECT_FLOAT_EQ(Missile::getBaseSpeed(WeaponType::Spread), 550.0f);
}

TEST_F(GetBaseSpeedTest, LaserType) {
    EXPECT_FLOAT_EQ(Missile::getBaseSpeed(WeaponType::Laser), 900.0f);
}

TEST_F(GetBaseSpeedTest, MissileType) {
    EXPECT_FLOAT_EQ(Missile::getBaseSpeed(WeaponType::Missile), 350.0f);
}

// ============================================================================
// Tests - getBaseDamage() Function
// ============================================================================

class GetBaseDamageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetBaseDamageTest, StandardType) {
    EXPECT_EQ(Missile::getBaseDamage(WeaponType::Standard), 20);
}

TEST_F(GetBaseDamageTest, SpreadType) {
    EXPECT_EQ(Missile::getBaseDamage(WeaponType::Spread), 8);
}

TEST_F(GetBaseDamageTest, LaserType) {
    EXPECT_EQ(Missile::getBaseDamage(WeaponType::Laser), 12);
}

TEST_F(GetBaseDamageTest, MissileType) {
    EXPECT_EQ(Missile::getBaseDamage(WeaponType::Missile), 50);
}

// ============================================================================
// Tests - getBaseCooldown() Function
// ============================================================================

class GetBaseCooldownTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetBaseCooldownTest, StandardType) {
    EXPECT_FLOAT_EQ(Missile::getBaseCooldown(WeaponType::Standard), 0.3f);
}

TEST_F(GetBaseCooldownTest, SpreadType) {
    EXPECT_FLOAT_EQ(Missile::getBaseCooldown(WeaponType::Spread), 0.4f);
}

TEST_F(GetBaseCooldownTest, LaserType) {
    EXPECT_FLOAT_EQ(Missile::getBaseCooldown(WeaponType::Laser), 0.18f);
}

TEST_F(GetBaseCooldownTest, MissileType) {
    EXPECT_FLOAT_EQ(Missile::getBaseCooldown(WeaponType::Missile), 0.7f);
}

// ============================================================================
// Tests - getDamage() with Level Support
// ============================================================================

class GetDamageWithLevelTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetDamageWithLevelTest, StandardLevel0) {
    // 20 * 100 / 100 = 20
    EXPECT_EQ(Missile::getDamage(WeaponType::Standard, 0), 20);
}

TEST_F(GetDamageWithLevelTest, StandardLevel1) {
    // 20 * 115 / 100 = 23
    EXPECT_EQ(Missile::getDamage(WeaponType::Standard, 1), 23);
}

TEST_F(GetDamageWithLevelTest, StandardLevel2) {
    // 20 * 130 / 100 = 26
    EXPECT_EQ(Missile::getDamage(WeaponType::Standard, 2), 26);
}

TEST_F(GetDamageWithLevelTest, StandardLevel3) {
    // 20 * 150 / 100 = 30
    EXPECT_EQ(Missile::getDamage(WeaponType::Standard, 3), 30);
}

TEST_F(GetDamageWithLevelTest, SpreadLevel0) {
    // 8 * 100 / 100 = 8
    EXPECT_EQ(Missile::getDamage(WeaponType::Spread, 0), 8);
}

TEST_F(GetDamageWithLevelTest, SpreadLevel3) {
    // 8 * 150 / 100 = 12
    EXPECT_EQ(Missile::getDamage(WeaponType::Spread, 3), 12);
}

TEST_F(GetDamageWithLevelTest, LaserLevel0) {
    EXPECT_EQ(Missile::getDamage(WeaponType::Laser, 0), 12);
}

TEST_F(GetDamageWithLevelTest, LaserLevel3) {
    // 12 * 150 / 100 = 18
    EXPECT_EQ(Missile::getDamage(WeaponType::Laser, 3), 18);
}

TEST_F(GetDamageWithLevelTest, HomingLevel0) {
    EXPECT_EQ(Missile::getDamage(WeaponType::Missile, 0), 50);
}

TEST_F(GetDamageWithLevelTest, HomingLevel3) {
    // 50 * 150 / 100 = 75
    EXPECT_EQ(Missile::getDamage(WeaponType::Missile, 3), 75);
}

TEST_F(GetDamageWithLevelTest, DefaultParameterIsLevel0) {
    // getDamage(type) should default to level 0
    EXPECT_EQ(Missile::getDamage(WeaponType::Standard), 20);
    EXPECT_EQ(Missile::getDamage(WeaponType::Spread), 8);
}

// ============================================================================
// Tests - getCooldown() with Level Support
// ============================================================================

class GetCooldownWithLevelTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetCooldownWithLevelTest, StandardLevel0) {
    // 0.3 * 100 / 100 = 0.3
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Standard, 0), 0.3f);
}

TEST_F(GetCooldownWithLevelTest, StandardLevel1) {
    // 0.3 * 95 / 100 = 0.285
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Standard, 1), 0.285f);
}

TEST_F(GetCooldownWithLevelTest, StandardLevel2) {
    // 0.3 * 90 / 100 = 0.27
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Standard, 2), 0.27f);
}

TEST_F(GetCooldownWithLevelTest, StandardLevel3) {
    // 0.3 * 85 / 100 = 0.255
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Standard, 3), 0.255f);
}

TEST_F(GetCooldownWithLevelTest, LaserLevel0) {
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Laser, 0), 0.18f);
}

TEST_F(GetCooldownWithLevelTest, LaserLevel3) {
    // 0.18 * 85 / 100 = 0.153
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Laser, 3), 0.153f);
}

TEST_F(GetCooldownWithLevelTest, HomingLevel0) {
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Missile, 0), 0.7f);
}

TEST_F(GetCooldownWithLevelTest, HomingLevel3) {
    // 0.7 * 85 / 100 = 0.595
    EXPECT_FLOAT_EQ(Missile::getCooldown(WeaponType::Missile, 3), 0.595f);
}

// ============================================================================
// Tests - getSpeed() with Level Support
// ============================================================================

class GetSpeedWithLevelTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GetSpeedWithLevelTest, StandardLevel0) {
    // 600 * 100 / 100 = 600
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Standard, 0), 600.0f);
}

TEST_F(GetSpeedWithLevelTest, StandardLevel1NoBonus) {
    // 600 * 100 / 100 = 600 (no bonus until level 3)
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Standard, 1), 600.0f);
}

TEST_F(GetSpeedWithLevelTest, StandardLevel2NoBonus) {
    // 600 * 100 / 100 = 600 (no bonus until level 3)
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Standard, 2), 600.0f);
}

TEST_F(GetSpeedWithLevelTest, StandardLevel3) {
    // 600 * 110 / 100 = 660
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Standard, 3), 660.0f);
}

TEST_F(GetSpeedWithLevelTest, LaserLevel3) {
    // 900 * 110 / 100 = 990
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Laser, 3), 990.0f);
}

TEST_F(GetSpeedWithLevelTest, SpreadLevel3) {
    // 550 * 110 / 100 = 605
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Spread, 3), 605.0f);
}

TEST_F(GetSpeedWithLevelTest, HomingLevel3) {
    // 350 * 110 / 100 = 385
    EXPECT_FLOAT_EQ(Missile::getSpeed(WeaponType::Missile, 3), 385.0f);
}

// ============================================================================
// Tests - DPS Calculations (verify ~60-70 DPS at level 0)
// ============================================================================

class DPSCalculationsTest : public ::testing::Test {
protected:
    void SetUp() override {}

    float calculateDPS(WeaponType type, uint8_t level) {
        float damage = static_cast<float>(Missile::getDamage(type, level));
        float cooldown = Missile::getCooldown(type, level);
        return damage / cooldown;
    }
};

TEST_F(DPSCalculationsTest, StandardDPSLevel0) {
    // 20 / 0.3 = 66.67 DPS
    float dps = calculateDPS(WeaponType::Standard, 0);
    EXPECT_NEAR(dps, 66.67f, 0.5f);
}

TEST_F(DPSCalculationsTest, LaserDPSLevel0) {
    // 12 / 0.18 = 66.67 DPS
    float dps = calculateDPS(WeaponType::Laser, 0);
    EXPECT_NEAR(dps, 66.67f, 0.5f);
}

TEST_F(DPSCalculationsTest, HomingDPSLevel0) {
    // 50 / 0.7 = 71.4 DPS
    float dps = calculateDPS(WeaponType::Missile, 0);
    EXPECT_NEAR(dps, 71.4f, 0.5f);
}

TEST_F(DPSCalculationsTest, SpreadDPSLevel0SingleShot) {
    // 8 / 0.4 = 20 DPS per projectile (x3 = 60 total if all hit)
    float dps = calculateDPS(WeaponType::Spread, 0);
    EXPECT_NEAR(dps, 20.0f, 0.5f);  // Single projectile
}

TEST_F(DPSCalculationsTest, StandardDPSLevel3Improved) {
    // 30 / 0.255 = 117.6 DPS (76% increase from level 0)
    float dps = calculateDPS(WeaponType::Standard, 3);
    EXPECT_NEAR(dps, 117.6f, 0.5f);
}

TEST_F(DPSCalculationsTest, DPSIncreasesWithLevel) {
    float dps0 = calculateDPS(WeaponType::Standard, 0);
    float dps1 = calculateDPS(WeaponType::Standard, 1);
    float dps2 = calculateDPS(WeaponType::Standard, 2);
    float dps3 = calculateDPS(WeaponType::Standard, 3);

    EXPECT_LT(dps0, dps1);
    EXPECT_LT(dps1, dps2);
    EXPECT_LT(dps2, dps3);
}

TEST_F(DPSCalculationsTest, Level3DPSIsApprox76PercentHigher) {
    float dps0 = calculateDPS(WeaponType::Standard, 0);
    float dps3 = calculateDPS(WeaponType::Standard, 3);

    float increase = (dps3 - dps0) / dps0 * 100.0f;
    // Should be approximately 76% increase
    EXPECT_NEAR(increase, 76.0f, 2.0f);
}

// ============================================================================
// Tests - Edge Cases
// ============================================================================

class WeaponLevelEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WeaponLevelEdgeCasesTest, Level0IsDefault) {
    // Verify level 0 provides base stats (no modification)
    EXPECT_EQ(Missile::getDamageMultiplier(0), 100);
    EXPECT_EQ(Missile::getCooldownMultiplier(0), 100);
    EXPECT_EQ(Missile::getSpeedMultiplier(0), 100);
}

TEST_F(WeaponLevelEdgeCasesTest, MaxLevelIs3) {
    // Level 3 should be the maximum effective level
    auto dmgMult3 = Missile::getDamageMultiplier(3);
    auto dmgMult4 = Missile::getDamageMultiplier(4);

    // Level 4 should fallback to level 0
    EXPECT_NE(dmgMult3, dmgMult4);
    EXPECT_EQ(dmgMult4, 100);  // Default/level 0
}

TEST_F(WeaponLevelEdgeCasesTest, AllWeaponTypesHaveStats) {
    // Verify all weapon types return valid values
    for (uint8_t type = 0; type < static_cast<uint8_t>(WeaponType::COUNT); ++type) {
        auto weaponType = static_cast<WeaponType>(type);
        EXPECT_GT(Missile::getBaseDamage(weaponType), 0);
        EXPECT_GT(Missile::getBaseCooldown(weaponType), 0.0f);
        EXPECT_GT(Missile::getBaseSpeed(weaponType), 0.0f);
    }
}

TEST_F(WeaponLevelEdgeCasesTest, HigherLevelAlwaysGivesMoreDamage) {
    for (uint8_t type = 0; type < static_cast<uint8_t>(WeaponType::COUNT); ++type) {
        auto weaponType = static_cast<WeaponType>(type);

        auto dmg0 = Missile::getDamage(weaponType, 0);
        auto dmg1 = Missile::getDamage(weaponType, 1);
        auto dmg2 = Missile::getDamage(weaponType, 2);
        auto dmg3 = Missile::getDamage(weaponType, 3);

        EXPECT_LE(dmg0, dmg1);
        EXPECT_LE(dmg1, dmg2);
        EXPECT_LE(dmg2, dmg3);
    }
}

TEST_F(WeaponLevelEdgeCasesTest, HigherLevelAlwaysGivesLowerCooldown) {
    for (uint8_t type = 0; type < static_cast<uint8_t>(WeaponType::COUNT); ++type) {
        auto weaponType = static_cast<WeaponType>(type);

        auto cd0 = Missile::getCooldown(weaponType, 0);
        auto cd1 = Missile::getCooldown(weaponType, 1);
        auto cd2 = Missile::getCooldown(weaponType, 2);
        auto cd3 = Missile::getCooldown(weaponType, 3);

        EXPECT_GE(cd0, cd1);
        EXPECT_GE(cd1, cd2);
        EXPECT_GE(cd2, cd3);
    }
}

TEST_F(WeaponLevelEdgeCasesTest, OnlyLevel3GivesSpeedBonus) {
    for (uint8_t type = 0; type < static_cast<uint8_t>(WeaponType::COUNT); ++type) {
        auto weaponType = static_cast<WeaponType>(type);

        auto speed0 = Missile::getSpeed(weaponType, 0);
        auto speed1 = Missile::getSpeed(weaponType, 1);
        auto speed2 = Missile::getSpeed(weaponType, 2);
        auto speed3 = Missile::getSpeed(weaponType, 3);

        // Levels 0, 1, 2 should have same speed
        EXPECT_FLOAT_EQ(speed0, speed1);
        EXPECT_FLOAT_EQ(speed1, speed2);

        // Level 3 should be faster (10% bonus)
        EXPECT_GT(speed3, speed2);
        EXPECT_FLOAT_EQ(speed3, speed0 * 1.1f);
    }
}
