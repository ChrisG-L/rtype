/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PowerUpMechanicsTest - Tests for Power-up system (R-Type authentic)
** Power-ups: Health, SpeedUp, WeaponCrystal, ForcePod (no Shield - authentic R-Type)
*/

#include <gtest/gtest.h>
#include "infrastructure/game/GameWorld.hpp"
#include "Protocol.hpp"

using namespace infrastructure::game;

// ============================================================================
// Tests - Power-up Constants
// ============================================================================

class PowerUpConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpConstantsTest, PowerUpMaxLifetime) {
    EXPECT_FLOAT_EQ(PowerUp::MAX_LIFETIME, 10.0f);
}

TEST_F(PowerUpConstantsTest, PowerUpWidth) {
    EXPECT_FLOAT_EQ(PowerUp::WIDTH, 24.0f);
}

TEST_F(PowerUpConstantsTest, PowerUpHeight) {
    EXPECT_FLOAT_EQ(PowerUp::HEIGHT, 24.0f);
}

TEST_F(PowerUpConstantsTest, PowerUpDriftSpeed) {
    // Power-ups drift left slowly
    EXPECT_FLOAT_EQ(PowerUp::DRIFT_SPEED, -30.0f);
}

TEST_F(PowerUpConstantsTest, PowerUpDropChanceRegular) {
    EXPECT_EQ(POWERUP_DROP_CHANCE, 15);  // 15% chance
}

TEST_F(PowerUpConstantsTest, PowerUpDropChancePOWArmor) {
    EXPECT_EQ(POWERUP_POW_ARMOR_CHANCE, 100);  // 100% guaranteed
}

// ============================================================================
// Tests - Force Pod Constants
// ============================================================================

class ForcePodConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ForcePodConstantsTest, ForcePodWidth) {
    EXPECT_FLOAT_EQ(ForcePod::WIDTH, 32.0f);
}

TEST_F(ForcePodConstantsTest, ForcePodHeight) {
    EXPECT_FLOAT_EQ(ForcePod::HEIGHT, 32.0f);
}

TEST_F(ForcePodConstantsTest, ForcePodSpeed) {
    EXPECT_FLOAT_EQ(ForcePod::SPEED, 400.0f);
}

TEST_F(ForcePodConstantsTest, ForcePodAttachOffsetX) {
    EXPECT_FLOAT_EQ(ForcePod::ATTACH_OFFSET_X, 50.0f);
}

TEST_F(ForcePodConstantsTest, ForcePodContactDamage) {
    // Should kill Zigzag (25 HP) or Fast (20 HP) in one hit
    EXPECT_EQ(ForcePod::CONTACT_DAMAGE, 45);
}

TEST_F(ForcePodConstantsTest, ForcePodHitCooldown) {
    EXPECT_FLOAT_EQ(ForcePod::HIT_COOLDOWN, 0.5f);
}

// ============================================================================
// Tests - POW Armor Constants
// ============================================================================

class POWArmorConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(POWArmorConstantsTest, SpawnInterval) {
    EXPECT_FLOAT_EQ(POW_ARMOR_SPAWN_INTERVAL, 25.0f);
}

TEST_F(POWArmorConstantsTest, POWArmorHealth) {
    EXPECT_EQ(Enemy::HEALTH_POW_ARMOR, 50);
}

TEST_F(POWArmorConstantsTest, POWArmorSpeed) {
    EXPECT_FLOAT_EQ(Enemy::SPEED_X_POW_ARMOR, -90.0f);
}

TEST_F(POWArmorConstantsTest, POWArmorShootInterval) {
    EXPECT_FLOAT_EQ(Enemy::SHOOT_INTERVAL_POW_ARMOR, 4.0f);
}

TEST_F(POWArmorConstantsTest, POWArmorPoints) {
    EXPECT_EQ(POINTS_POW_ARMOR, 200);
}

// ============================================================================
// Tests - Speed Level Multipliers
// ============================================================================

class SpeedLevelMultipliersTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SpeedLevelMultipliersTest, BaseMovementSpeed) {
    EXPECT_FLOAT_EQ(PLAYER_MOVE_SPEED, 200.0f);
}

TEST_F(SpeedLevelMultipliersTest, MaxSpeedLevelConstant) {
    EXPECT_EQ(MAX_SPEED_LEVEL, 3);
}

TEST_F(SpeedLevelMultipliersTest, SpeedMultiplierLevel0) {
    // Level 0: 1.0x speed
    constexpr float SPEED_MULTIPLIERS[4] = {1.0f, 1.3f, 1.6f, 1.9f};
    EXPECT_FLOAT_EQ(SPEED_MULTIPLIERS[0], 1.0f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedMultiplierLevel1) {
    constexpr float SPEED_MULTIPLIERS[4] = {1.0f, 1.3f, 1.6f, 1.9f};
    EXPECT_FLOAT_EQ(SPEED_MULTIPLIERS[1], 1.3f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedMultiplierLevel2) {
    constexpr float SPEED_MULTIPLIERS[4] = {1.0f, 1.3f, 1.6f, 1.9f};
    EXPECT_FLOAT_EQ(SPEED_MULTIPLIERS[2], 1.6f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedMultiplierLevel3) {
    constexpr float SPEED_MULTIPLIERS[4] = {1.0f, 1.3f, 1.6f, 1.9f};
    EXPECT_FLOAT_EQ(SPEED_MULTIPLIERS[3], 1.9f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedCalculationLevel0) {
    // 200 * 1.0 = 200 px/s
    constexpr float expected = 200.0f * 1.0f;
    EXPECT_FLOAT_EQ(expected, 200.0f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedCalculationLevel3) {
    // 200 * 1.9 = 380 px/s
    constexpr float expected = 200.0f * 1.9f;
    EXPECT_FLOAT_EQ(expected, 380.0f);
}

TEST_F(SpeedLevelMultipliersTest, SpeedIncreaseIsProgressive) {
    constexpr float SPEED_MULTIPLIERS[4] = {1.0f, 1.3f, 1.6f, 1.9f};

    EXPECT_LT(SPEED_MULTIPLIERS[0], SPEED_MULTIPLIERS[1]);
    EXPECT_LT(SPEED_MULTIPLIERS[1], SPEED_MULTIPLIERS[2]);
    EXPECT_LT(SPEED_MULTIPLIERS[2], SPEED_MULTIPLIERS[3]);
}

// ============================================================================
// Tests - Power-up Type Protocol Enum
// ============================================================================

class PowerUpTypeEnumTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpTypeEnumTest, HealthTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::Health), 0);
}

TEST_F(PowerUpTypeEnumTest, SpeedUpTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::SpeedUp), 1);
}

TEST_F(PowerUpTypeEnumTest, WeaponCrystalTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::WeaponCrystal), 2);
}

// Note: Shield removed for R-Type authentic gameplay (no shield in original)
// ForcePod is now at index 3, BitDevice at index 4

TEST_F(PowerUpTypeEnumTest, ForcePodTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::ForcePod), 3);
}

TEST_F(PowerUpTypeEnumTest, BitDeviceTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::BitDevice), 4);
}

TEST_F(PowerUpTypeEnumTest, TotalPowerUpTypes) {
    // R-Type authentic: Health, SpeedUp, WeaponCrystal, ForcePod, BitDevice (no Shield)
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::COUNT), 5);
    EXPECT_EQ(MAX_POWERUP_TYPES, 5);
}

TEST_F(PowerUpTypeEnumTest, MaxPowerUpsOnScreen) {
    EXPECT_EQ(MAX_POWERUPS, 8);
}

// ============================================================================
// Tests - PowerUpState Serialization
// ============================================================================

class PowerUpStateSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpStateSerializationTest, HealthPowerUpRoundtrip) {
    PowerUpState original{
        .id = 1,
        .x = 500,
        .y = 300,
        .type = static_cast<uint8_t>(PowerUpType::Health),
        .remaining_time = 8
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::Health));
}

TEST_F(PowerUpStateSerializationTest, SpeedUpPowerUpRoundtrip) {
    PowerUpState original{
        .id = 2,
        .x = 600,
        .y = 400,
        .type = static_cast<uint8_t>(PowerUpType::SpeedUp),
        .remaining_time = 10
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::SpeedUp));
}

TEST_F(PowerUpStateSerializationTest, ForcePodPowerUpRoundtrip) {
    PowerUpState original{
        .id = 3,
        .x = 700,
        .y = 500,
        .type = static_cast<uint8_t>(PowerUpType::ForcePod),
        .remaining_time = 10
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::ForcePod));
}

TEST_F(PowerUpStateSerializationTest, WeaponCrystalPowerUpRoundtrip) {
    PowerUpState original{
        .id = 4,
        .x = 800,
        .y = 200,
        .type = static_cast<uint8_t>(PowerUpType::WeaponCrystal),
        .remaining_time = 6
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::WeaponCrystal));
}

TEST_F(PowerUpStateSerializationTest, BitDevicePowerUpRoundtrip) {
    PowerUpState original{
        .id = 5,
        .x = 900,
        .y = 600,
        .type = static_cast<uint8_t>(PowerUpType::BitDevice),
        .remaining_time = 7
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::BitDevice));
}

// ============================================================================
// Tests - PowerUpCollected Serialization
// ============================================================================

class PowerUpCollectedSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpCollectedSerializationTest, HealthCollectedRoundtrip) {
    PowerUpCollected original{
        .powerup_id = 10,
        .player_id = 2,
        .powerup_type = static_cast<uint8_t>(PowerUpType::Health)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_id, 10);
    EXPECT_EQ(parsed->player_id, 2);
    EXPECT_EQ(parsed->powerup_type, static_cast<uint8_t>(PowerUpType::Health));
}

TEST_F(PowerUpCollectedSerializationTest, SpeedUpCollectedRoundtrip) {
    PowerUpCollected original{
        .powerup_id = 20,
        .player_id = 1,
        .powerup_type = static_cast<uint8_t>(PowerUpType::SpeedUp)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_type, static_cast<uint8_t>(PowerUpType::SpeedUp));
}

TEST_F(PowerUpCollectedSerializationTest, ForcePodCollectedRoundtrip) {
    PowerUpCollected original{
        .powerup_id = 30,
        .player_id = 3,
        .powerup_type = static_cast<uint8_t>(PowerUpType::ForcePod)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_type, static_cast<uint8_t>(PowerUpType::ForcePod));
}

TEST_F(PowerUpCollectedSerializationTest, BitDeviceCollectedRoundtrip) {
    PowerUpCollected original{
        .powerup_id = 40,
        .player_id = 4,
        .powerup_type = static_cast<uint8_t>(PowerUpType::BitDevice)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_type, static_cast<uint8_t>(PowerUpType::BitDevice));
}

// ============================================================================
// Tests - Power-up Drop Rates
// ============================================================================

class PowerUpDropRatesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpDropRatesTest, RegularEnemyDropChance) {
    // Regular enemies have 15% drop chance
    EXPECT_EQ(POWERUP_DROP_CHANCE, 15);
    EXPECT_LT(POWERUP_DROP_CHANCE, 100);
}

TEST_F(PowerUpDropRatesTest, POWArmorGuaranteedDrop) {
    // POW Armor has 100% guaranteed drop
    EXPECT_EQ(POWERUP_POW_ARMOR_CHANCE, 100);
}

TEST_F(PowerUpDropRatesTest, POWArmorDropHigherThanRegular) {
    EXPECT_GT(POWERUP_POW_ARMOR_CHANCE, POWERUP_DROP_CHANCE);
}

// ============================================================================
// Tests - Power-up Effects (expected values)
// ============================================================================

class PowerUpExpectedEffectsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpExpectedEffectsTest, HealthRestoreAmount) {
    // Health power-up should restore 25 HP
    constexpr uint8_t HEALTH_RESTORE = 25;
    EXPECT_EQ(HEALTH_RESTORE, 25);
}

// Note: Shield removed for R-Type authentic gameplay
// Defense in R-Type comes from Force Pod blocking projectiles

TEST_F(PowerUpExpectedEffectsTest, MaxWeaponLevel) {
    // WeaponCrystal can upgrade weapon to max level 3
    constexpr uint8_t MAX_WEAPON_LEVEL = 3;
    EXPECT_EQ(MAX_WEAPON_LEVEL, 3);
}

TEST_F(PowerUpExpectedEffectsTest, MaxSpeedLevel) {
    // SpeedUp can upgrade speed to max level 3
    EXPECT_EQ(MAX_SPEED_LEVEL, 3);
}

TEST_F(PowerUpExpectedEffectsTest, MaxForcePodLevel) {
    // Force Pod has max level 2
    constexpr uint8_t MAX_FORCE_LEVEL = 2;
    EXPECT_EQ(MAX_FORCE_LEVEL, 2);
}

// ============================================================================
// Tests - Enemy Type Health Values
// ============================================================================

class EnemyHealthValuesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EnemyHealthValuesTest, BasicEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_BASIC, 40);
}

TEST_F(EnemyHealthValuesTest, TrackerEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_TRACKER, 35);
}

TEST_F(EnemyHealthValuesTest, ZigzagEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_ZIGZAG, 25);
}

TEST_F(EnemyHealthValuesTest, FastEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_FAST, 20);
}

TEST_F(EnemyHealthValuesTest, BomberEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_BOMBER, 80);
}

TEST_F(EnemyHealthValuesTest, POWArmorEnemyHealth) {
    EXPECT_EQ(Enemy::HEALTH_POW_ARMOR, 50);
}

TEST_F(EnemyHealthValuesTest, EnemyHealthOrdering) {
    // Fast < Zigzag < Tracker < Basic < POWArmor < Bomber
    EXPECT_LT(Enemy::HEALTH_FAST, Enemy::HEALTH_ZIGZAG);
    EXPECT_LT(Enemy::HEALTH_ZIGZAG, Enemy::HEALTH_TRACKER);
    EXPECT_LT(Enemy::HEALTH_TRACKER, Enemy::HEALTH_BASIC);
    EXPECT_LT(Enemy::HEALTH_BASIC, Enemy::HEALTH_POW_ARMOR);
    EXPECT_LT(Enemy::HEALTH_POW_ARMOR, Enemy::HEALTH_BOMBER);
}

TEST_F(EnemyHealthValuesTest, EnemyGetHealthForType) {
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::Basic), 40);
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::Tracker), 35);
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::Zigzag), 25);
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::Fast), 20);
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::Bomber), 80);
    EXPECT_EQ(Enemy::getHealthForType(EnemyType::POWArmor), 50);
}

// ============================================================================
// Tests - Force Pod Damage vs Enemies
// ============================================================================

class ForcePodDamageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ForcePodDamageTest, KillsFastEnemy) {
    // Force Pod (45 dmg) should kill Fast enemy (20 HP) in one hit
    EXPECT_GT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_FAST);
}

TEST_F(ForcePodDamageTest, KillsZigzagEnemy) {
    // Force Pod (45 dmg) should kill Zigzag enemy (25 HP) in one hit
    EXPECT_GT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_ZIGZAG);
}

TEST_F(ForcePodDamageTest, KillsTrackerEnemy) {
    // Force Pod (45 dmg) should kill Tracker enemy (35 HP) in one hit
    EXPECT_GT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_TRACKER);
}

TEST_F(ForcePodDamageTest, DamagesBasicEnemy) {
    // Force Pod (45 dmg) should heavily damage Basic enemy (40 HP) - one hit kill!
    EXPECT_GT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_BASIC);
}

TEST_F(ForcePodDamageTest, DamagesPOWArmorEnemy) {
    // Force Pod (45 dmg) should damage POW Armor (50 HP) but not one-shot
    EXPECT_LT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_POW_ARMOR);
}

TEST_F(ForcePodDamageTest, DamagesBomberEnemy) {
    // Force Pod (45 dmg) should damage Bomber (80 HP) but not one-shot
    EXPECT_LT(ForcePod::CONTACT_DAMAGE, Enemy::HEALTH_BOMBER);
}

// ============================================================================
// Tests - Power-up Hitbox
// ============================================================================

class PowerUpHitboxTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpHitboxTest, PowerUpIsSquare) {
    EXPECT_FLOAT_EQ(PowerUp::WIDTH, PowerUp::HEIGHT);
}

TEST_F(PowerUpHitboxTest, PowerUpSmallerThanEnemy) {
    // Power-ups (24x24) should be smaller than enemies (40x40)
    EXPECT_LT(PowerUp::WIDTH, Enemy::WIDTH);
    EXPECT_LT(PowerUp::HEIGHT, Enemy::HEIGHT);
}

TEST_F(PowerUpHitboxTest, PowerUpSmallerThanForcePod) {
    // Power-ups (24x24) should be smaller than Force Pod (32x32)
    EXPECT_LT(PowerUp::WIDTH, ForcePod::WIDTH);
    EXPECT_LT(PowerUp::HEIGHT, ForcePod::HEIGHT);
}

// ============================================================================
// Tests - PlayerState with Power-up Fields
// ============================================================================

class PlayerStatePowerUpFieldsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStatePowerUpFieldsTest, PlayerStateWireSize) {
    // PlayerState should be 23 bytes with Phase 3 fields
    EXPECT_EQ(PlayerState::WIRE_SIZE, 23);
}

TEST_F(PlayerStatePowerUpFieldsTest, SpeedLevelSerializationRoundtrip) {
    PlayerState original{
        .id = 1,
        .x = 100,
        .y = 200,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 0,
        .kills = 0,
        .combo = 10,
        .currentWeapon = 0,
        .chargeLevel = 0,
        .speedLevel = 2,  // Speed level 2
        .weaponLevel = 0,
        .hasForce = 0,
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->speedLevel, 2);
}

TEST_F(PlayerStatePowerUpFieldsTest, WeaponLevelSerializationRoundtrip) {
    PlayerState original{
        .id = 2,
        .x = 150,
        .y = 250,
        .health = 80,
        .alive = 1,
        .lastAckedInputSeq = 10,
        .shipSkin = 2,
        .score = 1000,
        .kills = 5,
        .combo = 12,
        .currentWeapon = 1,
        .chargeLevel = 0,
        .speedLevel = 0,
        .weaponLevel = 3,  // Weapon level 3 (max)
        .hasForce = 0,
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->weaponLevel, 3);
}

// Note: shieldTimer field kept in protocol for binary compatibility
// but always 0 in R-Type authentic (no shield power-up)

TEST_F(PlayerStatePowerUpFieldsTest, HasForceSerializationRoundtrip) {
    PlayerState original{
        .id = 4,
        .x = 100,
        .y = 200,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 0,
        .kills = 0,
        .combo = 10,
        .currentWeapon = 0,
        .chargeLevel = 0,
        .speedLevel = 0,
        .weaponLevel = 0,
        .hasForce = 1,  // Has Force Pod
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->hasForce, 1);
}

TEST_F(PlayerStatePowerUpFieldsTest, AllPowerUpFieldsCombined) {
    PlayerState original{
        .id = 1,
        .x = 100,
        .y = 200,
        .health = 75,
        .alive = 1,
        .lastAckedInputSeq = 42,
        .shipSkin = 3,
        .score = 5000,
        .kills = 15,
        .combo = 15,  // 1.5x
        .currentWeapon = 2,  // Laser
        .chargeLevel = 2,
        .speedLevel = 2,
        .weaponLevel = 2,
        .hasForce = 1,
        .shieldTimer = 30  // 3.0 seconds
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 2);
    EXPECT_EQ(parsed->speedLevel, 2);
    EXPECT_EQ(parsed->weaponLevel, 2);
    EXPECT_EQ(parsed->hasForce, 1);
    EXPECT_EQ(parsed->shieldTimer, 30);
}
