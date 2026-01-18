/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Player Components Tests - Verifies player-specific ECS components
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PlayerTag.hpp"
#include "infrastructure/ecs/components/ScoreComp.hpp"
#include "infrastructure/ecs/components/WeaponComp.hpp"
#include "infrastructure/ecs/components/SpeedLevelComp.hpp"

using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class PlayerComponentsTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<PlayerTag>();
        _ecs.registerComponent<ScoreComp>();
        _ecs.registerComponent<WeaponComp>();
        _ecs.registerComponent<SpeedLevelComp>();
    }

    ECS::ECS _ecs;
};

// ═══════════════════════════════════════════════════════════════════════════
// PlayerTag Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerComponentsTest, PlayerTag_DefaultValues) {
    PlayerTag tag;
    EXPECT_EQ(tag.playerId, 0);
    EXPECT_EQ(tag.shipSkin, 1);
    EXPECT_TRUE(tag.isAlive);
}

TEST_F(PlayerComponentsTest, PlayerTag_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto& tag = _ecs.entityAddComponent<PlayerTag>(entity);

    tag.playerId = 1;
    tag.shipSkin = 3;
    tag.isAlive = true;

    EXPECT_TRUE(_ecs.entityHasComponent<PlayerTag>(entity));

    auto& retrieved = _ecs.entityGetComponent<PlayerTag>(entity);
    EXPECT_EQ(retrieved.playerId, 1);
    EXPECT_EQ(retrieved.shipSkin, 3);
    EXPECT_TRUE(retrieved.isAlive);
}

TEST_F(PlayerComponentsTest, PlayerTag_MultiplePlayersDistinct) {
    auto p1 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto p2 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto p3 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto p4 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);

    _ecs.entityAddComponent<PlayerTag>(p1).playerId = 1;
    _ecs.entityAddComponent<PlayerTag>(p2).playerId = 2;
    _ecs.entityAddComponent<PlayerTag>(p3).playerId = 3;
    _ecs.entityAddComponent<PlayerTag>(p4).playerId = 4;

    auto players = _ecs.getEntitiesByComponentsAllOf<PlayerTag>();
    EXPECT_EQ(players.size(), 4);
}

// ═══════════════════════════════════════════════════════════════════════════
// ScoreComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerComponentsTest, ScoreComp_DefaultValues) {
    ScoreComp score;
    EXPECT_EQ(score.total, 0);
    EXPECT_EQ(score.kills, 0);
    EXPECT_FLOAT_EQ(score.comboMultiplier, 1.0f);
    EXPECT_FLOAT_EQ(score.comboTimer, 0.0f);
    EXPECT_FLOAT_EQ(score.maxCombo, 1.0f);
    EXPECT_EQ(score.deaths, 0);
}

TEST_F(PlayerComponentsTest, ScoreComp_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto& score = _ecs.entityAddComponent<ScoreComp>(entity);

    score.total = 15000;
    score.kills = 50;
    score.comboMultiplier = 2.5f;
    score.comboTimer = 1.5f;
    score.maxCombo = 2.8f;
    score.deaths = 2;

    auto& retrieved = _ecs.entityGetComponent<ScoreComp>(entity);
    EXPECT_EQ(retrieved.total, 15000);
    EXPECT_EQ(retrieved.kills, 50);
    EXPECT_FLOAT_EQ(retrieved.comboMultiplier, 2.5f);
    EXPECT_FLOAT_EQ(retrieved.comboTimer, 1.5f);
    EXPECT_FLOAT_EQ(retrieved.maxCombo, 2.8f);
    EXPECT_EQ(retrieved.deaths, 2);
}

TEST_F(PlayerComponentsTest, ScoreComp_ComboRange) {
    ScoreComp score;

    // Test combo at minimum
    score.comboMultiplier = 1.0f;
    EXPECT_FLOAT_EQ(score.comboMultiplier, 1.0f);

    // Test combo at maximum
    score.comboMultiplier = 3.0f;
    EXPECT_FLOAT_EQ(score.comboMultiplier, 3.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// WeaponComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerComponentsTest, WeaponComp_DefaultValues) {
    WeaponComp weapon;
    EXPECT_EQ(weapon.currentType, 0);
    EXPECT_FLOAT_EQ(weapon.shootCooldown, 0.0f);
    EXPECT_FALSE(weapon.isCharging);
    EXPECT_FLOAT_EQ(weapon.chargeTime, 0.0f);
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(weapon.weaponLevels[i], 0);
    }
}

TEST_F(PlayerComponentsTest, WeaponComp_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto& weapon = _ecs.entityAddComponent<WeaponComp>(entity);

    weapon.currentType = 2;  // Laser
    weapon.shootCooldown = 0.3f;
    weapon.isCharging = true;
    weapon.chargeTime = 0.8f;
    weapon.weaponLevels = {1, 2, 0, 0};

    auto& retrieved = _ecs.entityGetComponent<WeaponComp>(entity);
    EXPECT_EQ(retrieved.currentType, 2);
    EXPECT_FLOAT_EQ(retrieved.shootCooldown, 0.3f);
    EXPECT_TRUE(retrieved.isCharging);
    EXPECT_FLOAT_EQ(retrieved.chargeTime, 0.8f);
    EXPECT_EQ(retrieved.weaponLevels[0], 1);
    EXPECT_EQ(retrieved.weaponLevels[1], 2);
}

TEST_F(PlayerComponentsTest, WeaponComp_GetCurrentLevel) {
    WeaponComp weapon;
    weapon.weaponLevels = {1, 2, 3, 0};

    weapon.currentType = 0;
    EXPECT_EQ(weapon.getCurrentLevel(), 1);

    weapon.currentType = 1;
    EXPECT_EQ(weapon.getCurrentLevel(), 2);

    weapon.currentType = 2;
    EXPECT_EQ(weapon.getCurrentLevel(), 3);

    weapon.currentType = 3;
    EXPECT_EQ(weapon.getCurrentLevel(), 0);

    // Invalid type
    weapon.currentType = 5;
    EXPECT_EQ(weapon.getCurrentLevel(), 0);
}

TEST_F(PlayerComponentsTest, WeaponComp_UpgradeCurrentWeapon) {
    WeaponComp weapon;
    weapon.currentType = 0;

    EXPECT_TRUE(weapon.upgradeCurrentWeapon());  // 0 -> 1
    EXPECT_EQ(weapon.weaponLevels[0], 1);

    EXPECT_TRUE(weapon.upgradeCurrentWeapon());  // 1 -> 2
    EXPECT_EQ(weapon.weaponLevels[0], 2);

    EXPECT_TRUE(weapon.upgradeCurrentWeapon());  // 2 -> 3
    EXPECT_EQ(weapon.weaponLevels[0], 3);

    EXPECT_FALSE(weapon.upgradeCurrentWeapon());  // Already max
    EXPECT_EQ(weapon.weaponLevels[0], 3);
}

TEST_F(PlayerComponentsTest, WeaponComp_IndependentWeaponLevels) {
    WeaponComp weapon;

    // Upgrade Standard
    weapon.currentType = 0;
    weapon.upgradeCurrentWeapon();
    weapon.upgradeCurrentWeapon();
    EXPECT_EQ(weapon.weaponLevels[0], 2);

    // Switch to Spread and upgrade
    weapon.currentType = 1;
    weapon.upgradeCurrentWeapon();
    EXPECT_EQ(weapon.weaponLevels[1], 1);

    // Standard should still be at 2
    weapon.currentType = 0;
    EXPECT_EQ(weapon.getCurrentLevel(), 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// SpeedLevelComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerComponentsTest, SpeedLevelComp_DefaultValues) {
    SpeedLevelComp speed;
    EXPECT_EQ(speed.level, 0);
    EXPECT_FALSE(speed.isMaxLevel());
}

TEST_F(PlayerComponentsTest, SpeedLevelComp_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto& speed = _ecs.entityAddComponent<SpeedLevelComp>(entity);

    speed.level = 2;

    auto& retrieved = _ecs.entityGetComponent<SpeedLevelComp>(entity);
    EXPECT_EQ(retrieved.level, 2);
    EXPECT_FALSE(retrieved.isMaxLevel());
}

TEST_F(PlayerComponentsTest, SpeedLevelComp_Upgrade) {
    SpeedLevelComp speed;

    EXPECT_TRUE(speed.upgrade());  // 0 -> 1
    EXPECT_EQ(speed.level, 1);
    EXPECT_FALSE(speed.isMaxLevel());

    EXPECT_TRUE(speed.upgrade());  // 1 -> 2
    EXPECT_EQ(speed.level, 2);
    EXPECT_FALSE(speed.isMaxLevel());

    EXPECT_TRUE(speed.upgrade());  // 2 -> 3
    EXPECT_EQ(speed.level, 3);
    EXPECT_TRUE(speed.isMaxLevel());

    EXPECT_FALSE(speed.upgrade());  // Already max
    EXPECT_EQ(speed.level, 3);
}

// ═══════════════════════════════════════════════════════════════════════════
// Combined Player Entity Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerComponentsTest, FullPlayerEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);

    auto& tag = _ecs.entityAddComponent<PlayerTag>(entity);
    tag.playerId = 1;
    tag.shipSkin = 2;
    tag.isAlive = true;

    auto& score = _ecs.entityAddComponent<ScoreComp>(entity);
    score.total = 5000;
    score.kills = 20;
    score.comboMultiplier = 1.5f;

    auto& weapon = _ecs.entityAddComponent<WeaponComp>(entity);
    weapon.currentType = 1;
    weapon.weaponLevels = {1, 2, 0, 0};

    auto& speed = _ecs.entityAddComponent<SpeedLevelComp>(entity);
    speed.level = 1;

    // Query for full player entities
    auto players = _ecs.getEntitiesByComponentsAllOf<
        PlayerTag, ScoreComp, WeaponComp, SpeedLevelComp
    >();
    EXPECT_EQ(players.size(), 1);

    // Verify all data
    auto& retrievedTag = _ecs.entityGetComponent<PlayerTag>(entity);
    auto& retrievedScore = _ecs.entityGetComponent<ScoreComp>(entity);
    auto& retrievedWeapon = _ecs.entityGetComponent<WeaponComp>(entity);
    auto& retrievedSpeed = _ecs.entityGetComponent<SpeedLevelComp>(entity);

    EXPECT_EQ(retrievedTag.playerId, 1);
    EXPECT_EQ(retrievedScore.total, 5000);
    EXPECT_EQ(retrievedWeapon.currentType, 1);
    EXPECT_EQ(retrievedSpeed.level, 1);
}
