/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour GameRule Domain Service
*/

#include <gtest/gtest.h>
#include "domain/services/GameRule.hpp"
#include "domain/Constants.hpp"

using namespace domain::services;
using namespace domain::constants;

/**
 * @brief Suite de tests pour le Domain Service GameRule
 *
 * GameRule encapsule toute la logique de calcul du jeu :
 * dégâts, score, combo, statistiques ennemis, etc.
 */
class GameRuleTest : public ::testing::Test {
protected:
    GameRule _rule;

    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests Weapon Damage
// ============================================================================

TEST_F(GameRuleTest, GetMissileDamage_Standard_Level0) {
    EXPECT_EQ(_rule.getMissileDamage(0, 0), weapon::DAMAGE_STANDARD);
}

TEST_F(GameRuleTest, GetMissileDamage_Standard_Level3) {
    uint8_t expected = weapon::DAMAGE_STANDARD * weapon::DAMAGE_MULT_LV3 / 100;
    EXPECT_EQ(_rule.getMissileDamage(0, 3), expected);
}

TEST_F(GameRuleTest, GetMissileDamage_Spread_Level0) {
    EXPECT_EQ(_rule.getMissileDamage(1, 0), weapon::DAMAGE_SPREAD);
}

TEST_F(GameRuleTest, GetMissileDamage_Laser_Level0) {
    EXPECT_EQ(_rule.getMissileDamage(2, 0), weapon::DAMAGE_LASER);
}

TEST_F(GameRuleTest, GetMissileDamage_Missile_Level0) {
    EXPECT_EQ(_rule.getMissileDamage(3, 0), weapon::DAMAGE_MISSILE);
}

// ============================================================================
// Tests Weapon Speed
// ============================================================================

TEST_F(GameRuleTest, GetMissileSpeed_Standard_Level0) {
    EXPECT_FLOAT_EQ(_rule.getMissileSpeed(0, 0), weapon::SPEED_STANDARD);
}

TEST_F(GameRuleTest, GetMissileSpeed_Laser_IsFastest) {
    EXPECT_GT(_rule.getMissileSpeed(2, 0), _rule.getMissileSpeed(0, 0));
    EXPECT_GT(_rule.getMissileSpeed(2, 0), _rule.getMissileSpeed(1, 0));
    EXPECT_GT(_rule.getMissileSpeed(2, 0), _rule.getMissileSpeed(3, 0));
}

TEST_F(GameRuleTest, GetMissileSpeed_Level3HasBonus) {
    float level0 = _rule.getMissileSpeed(0, 0);
    float level3 = _rule.getMissileSpeed(0, 3);
    EXPECT_GT(level3, level0);
}

// ============================================================================
// Tests Weapon Cooldown
// ============================================================================

TEST_F(GameRuleTest, GetWeaponCooldown_Laser_IsFastest) {
    EXPECT_LT(_rule.getWeaponCooldown(2, 0), _rule.getWeaponCooldown(0, 0));
}

TEST_F(GameRuleTest, GetWeaponCooldown_HigherLevel_LowerCooldown) {
    float level0 = _rule.getWeaponCooldown(0, 0);
    float level3 = _rule.getWeaponCooldown(0, 3);
    EXPECT_LT(level3, level0);
}

// ============================================================================
// Tests Score System
// ============================================================================

TEST_F(GameRuleTest, GetEnemyPointValue_Basic) {
    EXPECT_EQ(_rule.getEnemyPointValue(0), score::POINTS_BASIC);
}

TEST_F(GameRuleTest, GetEnemyPointValue_Bomber_HighestRegular) {
    EXPECT_GT(_rule.getEnemyPointValue(4), _rule.getEnemyPointValue(0));
    EXPECT_GT(_rule.getEnemyPointValue(4), _rule.getEnemyPointValue(1));
    EXPECT_GT(_rule.getEnemyPointValue(4), _rule.getEnemyPointValue(2));
    EXPECT_GT(_rule.getEnemyPointValue(4), _rule.getEnemyPointValue(3));
}

TEST_F(GameRuleTest, ApplyComboBonus_NoCombo) {
    EXPECT_EQ(_rule.applyComboBonus(100, 1.0f), 100);
}

TEST_F(GameRuleTest, ApplyComboBonus_1_5x) {
    EXPECT_EQ(_rule.applyComboBonus(100, 1.5f), 150);
}

TEST_F(GameRuleTest, ApplyComboBonus_MaxCombo) {
    EXPECT_EQ(_rule.applyComboBonus(100, 3.0f), 300);
}

TEST_F(GameRuleTest, IncrementCombo_FromBase) {
    float newCombo = _rule.incrementCombo(1.0f);
    EXPECT_FLOAT_EQ(newCombo, 1.0f + score::COMBO_INCREMENT);
}

TEST_F(GameRuleTest, IncrementCombo_CappedAtMax) {
    float newCombo = _rule.incrementCombo(score::COMBO_MAX);
    EXPECT_FLOAT_EQ(newCombo, score::COMBO_MAX);
}

TEST_F(GameRuleTest, DecayCombo_DuringGracePeriod_NoDecay) {
    float combo = _rule.decayCombo(2.0f, 0.1f, 1.0f);  // 1s since kill < grace
    EXPECT_FLOAT_EQ(combo, 2.0f);
}

TEST_F(GameRuleTest, DecayCombo_AfterGracePeriod_Decays) {
    float combo = _rule.decayCombo(2.0f, 1.0f, 5.0f);  // 5s since kill > grace
    EXPECT_LT(combo, 2.0f);
}

TEST_F(GameRuleTest, DecayCombo_NeverBelowOne) {
    float combo = _rule.decayCombo(1.0f, 10.0f, 100.0f);  // Long decay
    EXPECT_GE(combo, 1.0f);
}

// ============================================================================
// Tests Health & Death
// ============================================================================

TEST_F(GameRuleTest, ShouldEntityDie_True) {
    EXPECT_TRUE(_rule.shouldEntityDie(30, 50));
}

TEST_F(GameRuleTest, ShouldEntityDie_False) {
    EXPECT_FALSE(_rule.shouldEntityDie(100, 50));
}

TEST_F(GameRuleTest, ShouldEntityDie_ExactDamage) {
    EXPECT_TRUE(_rule.shouldEntityDie(50, 50));
}

TEST_F(GameRuleTest, ApplyDamage_Normal) {
    EXPECT_EQ(_rule.applyDamage(100, 30), 70);
}

TEST_F(GameRuleTest, ApplyDamage_Lethal) {
    EXPECT_EQ(_rule.applyDamage(30, 50), 0);
}

// ============================================================================
// Tests Player Movement
// ============================================================================

TEST_F(GameRuleTest, GetSpeedMultiplier_Level0) {
    EXPECT_FLOAT_EQ(_rule.getSpeedMultiplier(0), player::SPEED_MULT_LV0);
}

TEST_F(GameRuleTest, GetSpeedMultiplier_IncreasesByLevel) {
    EXPECT_LT(_rule.getSpeedMultiplier(0), _rule.getSpeedMultiplier(1));
    EXPECT_LT(_rule.getSpeedMultiplier(1), _rule.getSpeedMultiplier(2));
    EXPECT_LT(_rule.getSpeedMultiplier(2), _rule.getSpeedMultiplier(3));
}

TEST_F(GameRuleTest, GetPlayerBaseSpeed) {
    EXPECT_FLOAT_EQ(_rule.getPlayerBaseSpeed(), player::MOVE_SPEED);
}

// ============================================================================
// Tests Enemy Statistics
// ============================================================================

TEST_F(GameRuleTest, GetEnemyHealth_Bomber_Highest) {
    EXPECT_GT(_rule.getEnemyHealth(4), _rule.getEnemyHealth(0));
    EXPECT_GT(_rule.getEnemyHealth(4), _rule.getEnemyHealth(3));
}

TEST_F(GameRuleTest, GetEnemyHealth_Fast_Lowest) {
    EXPECT_LT(_rule.getEnemyHealth(3), _rule.getEnemyHealth(0));
}

TEST_F(GameRuleTest, GetEnemySpeed_Fast_IsFastest) {
    // Speeds are negative (moving left), so "fastest" = most negative
    EXPECT_LT(_rule.getEnemySpeed(3), _rule.getEnemySpeed(0));
    EXPECT_LT(_rule.getEnemySpeed(3), _rule.getEnemySpeed(4));
}

TEST_F(GameRuleTest, GetEnemyShootInterval_Bomber_Fastest) {
    EXPECT_LT(_rule.getEnemyShootInterval(4), _rule.getEnemyShootInterval(0));
}

// ============================================================================
// Tests Wave Cannon
// ============================================================================

TEST_F(GameRuleTest, GetWaveCannonDamage_IncreasesWithLevel) {
    EXPECT_LT(_rule.getWaveCannonDamage(1), _rule.getWaveCannonDamage(2));
    EXPECT_LT(_rule.getWaveCannonDamage(2), _rule.getWaveCannonDamage(3));
}

TEST_F(GameRuleTest, GetWaveCannonWidth_IncreasesWithLevel) {
    EXPECT_LT(_rule.getWaveCannonWidth(1), _rule.getWaveCannonWidth(2));
    EXPECT_LT(_rule.getWaveCannonWidth(2), _rule.getWaveCannonWidth(3));
}

TEST_F(GameRuleTest, GetWaveCannonChargeTime_IncreasesWithLevel) {
    EXPECT_LT(_rule.getWaveCannonChargeTime(1), _rule.getWaveCannonChargeTime(2));
    EXPECT_LT(_rule.getWaveCannonChargeTime(2), _rule.getWaveCannonChargeTime(3));
}

// ============================================================================
// Tests Boss
// ============================================================================

TEST_F(GameRuleTest, GetBossMaxHealth_SinglePlayer) {
    uint16_t hp = _rule.getBossMaxHealth(1, 0);
    EXPECT_EQ(hp, boss::MAX_HEALTH);
}

TEST_F(GameRuleTest, GetBossMaxHealth_MultiPlayer_ScalesUp) {
    uint16_t hp1 = _rule.getBossMaxHealth(1, 0);
    uint16_t hp4 = _rule.getBossMaxHealth(4, 0);
    EXPECT_GT(hp4, hp1);
}

TEST_F(GameRuleTest, GetBossMaxHealth_DefeatCount_ScalesUp) {
    uint16_t first = _rule.getBossMaxHealth(1, 0);
    uint16_t second = _rule.getBossMaxHealth(1, 1);
    EXPECT_GT(second, first);
}

TEST_F(GameRuleTest, ShouldBossChangePhase_Phase2) {
    // 65% threshold
    EXPECT_FALSE(_rule.shouldBossChangePhase(1000, 1500, 2));  // 66%
    EXPECT_TRUE(_rule.shouldBossChangePhase(900, 1500, 2));    // 60%
}

TEST_F(GameRuleTest, ShouldBossChangePhase_Phase3) {
    // 30% threshold
    EXPECT_FALSE(_rule.shouldBossChangePhase(500, 1500, 3));   // 33%
    EXPECT_TRUE(_rule.shouldBossChangePhase(400, 1500, 3));    // 26%
}
