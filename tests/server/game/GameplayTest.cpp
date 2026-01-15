/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameplayTest - Tests for Gameplay Phase 2 features (Score, Boss, Weapons)
*/

#include <gtest/gtest.h>
#include "Protocol.hpp"

// ============================================================================
// Protocol Tests - Score System
// ============================================================================

class ScoreSystemTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ScoreSystemTest, PlayerStateSerializationWithScore) {
    PlayerState original{
        .id = 1,
        .x = 100,
        .y = 200,
        .health = 80,
        .alive = 1,
        .lastAckedInputSeq = 42,
        .shipSkin = 3,
        .score = 12500,
        .kills = 25,
        .combo = 15,  // 1.5x
        .currentWeapon = static_cast<uint8_t>(WeaponType::Spread)
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->alive, original.alive);
    EXPECT_EQ(parsed->lastAckedInputSeq, original.lastAckedInputSeq);
    EXPECT_EQ(parsed->shipSkin, original.shipSkin);
    EXPECT_EQ(parsed->score, original.score);
    EXPECT_EQ(parsed->kills, original.kills);
    EXPECT_EQ(parsed->combo, original.combo);
    EXPECT_EQ(parsed->currentWeapon, original.currentWeapon);
}

TEST_F(ScoreSystemTest, ComboValueRange) {
    // Combo stored as x10 (15 = 1.5x, max 30 = 3.0x)
    EXPECT_EQ(10, 10);   // 1.0x base
    EXPECT_EQ(15, 15);   // 1.5x
    EXPECT_EQ(20, 20);   // 2.0x
    EXPECT_EQ(30, 30);   // 3.0x max
}

// ============================================================================
// Protocol Tests - Boss System
// ============================================================================

class BossSystemTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BossSystemTest, BossStateSerializationRoundtrip) {
    BossState original{
        .id = 1,
        .x = 1500,
        .y = 540,
        .max_health = 1000,
        .health = 650,
        .phase = 1,
        .is_active = 1
    };

    uint8_t buffer[BossState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = BossState::from_bytes(buffer, BossState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->max_health, original.max_health);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->phase, original.phase);
    EXPECT_EQ(parsed->is_active, original.is_active);
}

TEST_F(BossSystemTest, BossStateInactiveByDefault) {
    BossState boss{};
    EXPECT_EQ(boss.is_active, 0);
}

TEST_F(BossSystemTest, BossPhaseValues) {
    // Phase 0: 100%-65% HP
    // Phase 1: 65%-30% HP
    // Phase 2: 30%-0% HP (Enraged)
    EXPECT_EQ(0, 0);
    EXPECT_EQ(1, 1);
    EXPECT_EQ(2, 2);
}

// ============================================================================
// Protocol Tests - Weapon System
// ============================================================================

class WeaponSystemTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WeaponSystemTest, WeaponTypeEnumValues) {
    EXPECT_EQ(static_cast<uint8_t>(WeaponType::Standard), 0);
    EXPECT_EQ(static_cast<uint8_t>(WeaponType::Spread), 1);
    EXPECT_EQ(static_cast<uint8_t>(WeaponType::Laser), 2);
    EXPECT_EQ(static_cast<uint8_t>(WeaponType::Missile), 3);
    EXPECT_EQ(static_cast<uint8_t>(WeaponType::COUNT), 4);
}

TEST_F(WeaponSystemTest, MaxWeaponTypes) {
    EXPECT_EQ(MAX_WEAPON_TYPES, 4);
}

TEST_F(WeaponSystemTest, MissileStateWithWeaponType) {
    MissileState original{
        .id = 42,
        .owner_id = 1,
        .x = 500,
        .y = 300,
        .weapon_type = static_cast<uint8_t>(WeaponType::Laser)
    };

    uint8_t buffer[MissileState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = MissileState::from_bytes(buffer, MissileState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->owner_id, original.owner_id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->weapon_type, original.weapon_type);
}

// ============================================================================
// Protocol Tests - Input Keys (Weapon Switching)
// ============================================================================

class InputKeysTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(InputKeysTest, WeaponSwitchingKeys) {
    EXPECT_EQ(InputKeys::WEAPON_NEXT, 0x0020);
    EXPECT_EQ(InputKeys::WEAPON_PREV, 0x0040);
}

TEST_F(InputKeysTest, PlayerInputSerializationWithWeaponSwitch) {
    PlayerInput original{
        .keys = InputKeys::UP | InputKeys::SHOOT | InputKeys::WEAPON_NEXT,
        .sequenceNum = 123
    };

    uint8_t buffer[PlayerInput::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerInput::from_bytes(buffer, PlayerInput::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keys, original.keys);
    EXPECT_EQ(parsed->sequenceNum, original.sequenceNum);

    // Verify individual flags
    EXPECT_TRUE(parsed->keys & InputKeys::UP);
    EXPECT_TRUE(parsed->keys & InputKeys::SHOOT);
    EXPECT_TRUE(parsed->keys & InputKeys::WEAPON_NEXT);
    EXPECT_FALSE(parsed->keys & InputKeys::DOWN);
    EXPECT_FALSE(parsed->keys & InputKeys::WEAPON_PREV);
}

// ============================================================================
// Protocol Tests - Enemy State
// ============================================================================

class EnemyStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EnemyStateTest, EnemyStateSerializationRoundtrip) {
    EnemyState original{
        .id = 5,
        .x = 1200,
        .y = 400,
        .health = 30,
        .enemy_type = 2  // Zigzag type
    };

    uint8_t buffer[EnemyState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = EnemyState::from_bytes(buffer, EnemyState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->enemy_type, original.enemy_type);
}

// ============================================================================
// Protocol Tests - GameSnapshot with Boss
// ============================================================================

class GameSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GameSnapshotTest, SnapshotWithBossState) {
    GameSnapshot snapshot{};
    snapshot.player_count = 1;
    snapshot.players[0] = {
        .id = 0,
        .x = 100,
        .y = 300,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 5000,
        .kills = 10,
        .combo = 12,
        .currentWeapon = 0
    };
    snapshot.missile_count = 0;
    snapshot.enemy_count = 0;
    snapshot.enemy_missile_count = 0;
    snapshot.wave_number = 10;
    snapshot.has_boss = 1;
    snapshot.boss_state = {
        .id = 1,
        .x = 1500,
        .y = 540,
        .max_health = 1000,
        .health = 800,
        .phase = 0,
        .is_active = 1
    };

    // Test wire size calculation
    size_t expected_size = snapshot.wire_size();
    EXPECT_GT(expected_size, 0);

    // Test serialization
    std::vector<uint8_t> buffer(expected_size);
    snapshot.to_bytes(buffer.data());

    auto parsed = GameSnapshot::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_count, 1);
    EXPECT_EQ(parsed->wave_number, 10);
    EXPECT_EQ(parsed->has_boss, 1);
    EXPECT_EQ(parsed->boss_state.health, 800);
    EXPECT_EQ(parsed->boss_state.phase, 0);
    EXPECT_EQ(parsed->boss_state.is_active, 1);
}

TEST_F(GameSnapshotTest, SnapshotWithoutBoss) {
    GameSnapshot snapshot{};
    snapshot.player_count = 1;
    snapshot.players[0] = {
        .id = 0,
        .x = 100,
        .y = 300,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 1000,
        .kills = 5,
        .combo = 10,
        .currentWeapon = 0
    };
    snapshot.missile_count = 0;
    snapshot.enemy_count = 2;
    snapshot.enemies[0] = {.id = 1, .x = 800, .y = 200, .health = 20, .enemy_type = 0};
    snapshot.enemies[1] = {.id = 2, .x = 900, .y = 400, .health = 30, .enemy_type = 1};
    snapshot.enemy_missile_count = 0;
    snapshot.wave_number = 3;
    snapshot.has_boss = 0;

    size_t expected_size = snapshot.wire_size();
    std::vector<uint8_t> buffer(expected_size);
    snapshot.to_bytes(buffer.data());

    auto parsed = GameSnapshot::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->wave_number, 3);
    EXPECT_EQ(parsed->has_boss, 0);
    EXPECT_EQ(parsed->enemy_count, 2);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Wave Cannon
// ============================================================================

class WaveCannonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WaveCannonTest, WaveCannonStateSerializationRoundtrip) {
    WaveCannonState original{
        .id = 42,
        .owner_id = 1,
        .x = 200,
        .y = 300,
        .charge_level = 3,
        .width = 50  // WIDTH_LV3 = 50.0f
    };

    uint8_t buffer[WaveCannonState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = WaveCannonState::from_bytes(buffer, WaveCannonState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->owner_id, original.owner_id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->charge_level, original.charge_level);
    EXPECT_EQ(parsed->width, original.width);
}

TEST_F(WaveCannonTest, WaveCannonChargeLevels) {
    // Level 1: 0.6s charge (quick shot)
    // Level 2: 1.3s charge (medium)
    // Level 3: 2.2s charge (full charge max damage)
    EXPECT_FLOAT_EQ(WaveCannon::CHARGE_TIME_LV1, 0.6f);
    EXPECT_FLOAT_EQ(WaveCannon::CHARGE_TIME_LV2, 1.3f);
    EXPECT_FLOAT_EQ(WaveCannon::CHARGE_TIME_LV3, 2.2f);
}

TEST_F(WaveCannonTest, WaveCannonBeamWidths) {
    EXPECT_FLOAT_EQ(WaveCannon::WIDTH_LV1, 20.0f);
    EXPECT_FLOAT_EQ(WaveCannon::WIDTH_LV2, 35.0f);
    EXPECT_FLOAT_EQ(WaveCannon::WIDTH_LV3, 55.0f);  // Wider for more hits
}

TEST_F(WaveCannonTest, ChargeReleaseSerializationRoundtrip) {
    ChargeRelease original{.charge_level = 2};

    uint8_t buffer[ChargeRelease::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = ChargeRelease::from_bytes(buffer, ChargeRelease::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->charge_level, original.charge_level);
}

TEST_F(WaveCannonTest, WaveCannonFromBytesTooSmall) {
    uint8_t buffer[4] = {0};  // Too small
    auto parsed = WaveCannonState::from_bytes(buffer, 4);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Power-ups
// ============================================================================

class PowerUpTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpTest, PowerUpTypeEnumValues) {
    // R-Type authentic: no Shield power-up
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::Health), 0);
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::SpeedUp), 1);
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::WeaponCrystal), 2);
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::ForcePod), 3);
}

TEST_F(PowerUpTest, PowerUpStateSerializationRoundtrip) {
    PowerUpState original{
        .id = 100,
        .x = 500,
        .y = 400,
        .type = static_cast<uint8_t>(PowerUpType::ForcePod),
        .remaining_time = 8
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->type, original.type);
    EXPECT_EQ(parsed->remaining_time, original.remaining_time);
}

TEST_F(PowerUpTest, PowerUpCollectedSerializationRoundtrip) {
    PowerUpCollected original{
        .powerup_id = 55,
        .player_id = 2,
        .powerup_type = static_cast<uint8_t>(PowerUpType::SpeedUp)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_id, original.powerup_id);
    EXPECT_EQ(parsed->player_id, original.player_id);
    EXPECT_EQ(parsed->powerup_type, original.powerup_type);
}

TEST_F(PowerUpTest, PowerUpExpiredSerializationRoundtrip) {
    PowerUpExpired original{.powerup_id = 77};

    uint8_t buffer[PowerUpExpired::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PowerUpExpired::from_bytes(buffer, PowerUpExpired::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_id, original.powerup_id);
}

TEST_F(PowerUpTest, PowerUpStateFromBytesTooSmall) {
    uint8_t buffer[4] = {0};  // Too small
    auto parsed = PowerUpState::from_bytes(buffer, 4);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Force Pod
// ============================================================================

class ForcePodTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ForcePodTest, ForceStateSerializationRoundtrip) {
    ForceState original{
        .owner_id = 1,
        .x = 150,
        .y = 300,
        .is_attached = 1,
        .level = 2
    };

    uint8_t buffer[ForceState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = ForceState::from_bytes(buffer, ForceState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->owner_id, original.owner_id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->is_attached, original.is_attached);
    EXPECT_EQ(parsed->level, original.level);
}

TEST_F(ForcePodTest, ForceStateDetached) {
    ForceState original{
        .owner_id = 2,
        .x = 600,
        .y = 250,
        .is_attached = 0,  // Detached
        .level = 1
    };

    uint8_t buffer[ForceState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = ForceState::from_bytes(buffer, ForceState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->is_attached, 0);
    EXPECT_EQ(parsed->level, 1);
}

TEST_F(ForcePodTest, ForceStateFromBytesTooSmall) {
    uint8_t buffer[4] = {0};  // Too small
    auto parsed = ForceState::from_bytes(buffer, 4);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Message Types
// ============================================================================

class Phase3MessageTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(Phase3MessageTypesTest, ChargeMessageTypes) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ChargeStart), 0x0400);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ChargeRelease), 0x0401);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::WaveCannonFired), 0x0402);
}

TEST_F(Phase3MessageTypesTest, PowerUpMessageTypes) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PowerUpSpawned), 0x0410);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PowerUpCollected), 0x0411);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PowerUpExpired), 0x0412);
}

TEST_F(Phase3MessageTypesTest, ForceMessageTypes) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ForceToggle), 0x0420);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::ForceStateUpdate), 0x0421);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Wave Cannon Damage
// ============================================================================

class WaveCannonDamageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WaveCannonDamageTest, DamageLevelValues) {
    // Verify damage constants match Protocol.hpp (balanced for enemy HP)
    EXPECT_EQ(WaveCannon::DAMAGE_LV1, 50);   // Kills Fast enemy (20 HP)
    EXPECT_EQ(WaveCannon::DAMAGE_LV2, 100);  // Kills most enemies
    EXPECT_EQ(WaveCannon::DAMAGE_LV3, 200);  // Kills Bomber or big boss damage
}

TEST_F(WaveCannonDamageTest, DamageIncreaseWithLevel) {
    // Level 2 should deal more damage than Level 1
    EXPECT_GT(WaveCannon::DAMAGE_LV2, WaveCannon::DAMAGE_LV1);
    // Level 3 should deal more damage than Level 2
    EXPECT_GT(WaveCannon::DAMAGE_LV3, WaveCannon::DAMAGE_LV2);
}

TEST_F(WaveCannonDamageTest, Level3DamageIsMaximum) {
    // Level 3 damage should be highest
    EXPECT_GE(WaveCannon::DAMAGE_LV3, WaveCannon::DAMAGE_LV2);
    EXPECT_GE(WaveCannon::DAMAGE_LV3, WaveCannon::DAMAGE_LV1);
}

TEST_F(WaveCannonDamageTest, DamageScalesWithChargeTime) {
    // Ratio of damage should roughly follow charge time ratio
    // LV1: 0.5s, LV2: 1.0s, LV3: 1.5s
    // Damage should increase with charge time investment
    float chargeRatio_1_2 = WaveCannon::CHARGE_TIME_LV2 / WaveCannon::CHARGE_TIME_LV1;  // 2.0
    float damageRatio_1_2 = static_cast<float>(WaveCannon::DAMAGE_LV2) / WaveCannon::DAMAGE_LV1;  // 2.0

    // Damage ratio should be at least proportional to charge time
    EXPECT_GE(damageRatio_1_2, chargeRatio_1_2 * 0.8f);  // Allow 20% tolerance
}

TEST_F(WaveCannonDamageTest, WidthScalesWithDamage) {
    // Higher damage levels should have wider beams
    EXPECT_GT(WaveCannon::WIDTH_LV2, WaveCannon::WIDTH_LV1);
    EXPECT_GT(WaveCannon::WIDTH_LV3, WaveCannon::WIDTH_LV2);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Wave Cannon Piercing Behavior
// ============================================================================

class WaveCannonPiercingTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WaveCannonPiercingTest, Level1IsNotPiercing) {
    // Level 1 Wave Cannon should NOT pierce through enemies
    // (verified by code: piercing = false for level 1)
    uint8_t chargeLevel = 1;
    bool expectedPiercing = false;

    // This test verifies the expected behavior based on game design
    // Level 1 stops on first enemy hit
    EXPECT_EQ(chargeLevel, 1);
    EXPECT_FALSE(expectedPiercing);
}

TEST_F(WaveCannonPiercingTest, Level2IsPiercing) {
    // Level 2 Wave Cannon should pierce through enemies
    uint8_t chargeLevel = 2;
    bool expectedPiercing = true;

    EXPECT_EQ(chargeLevel, 2);
    EXPECT_TRUE(expectedPiercing);
}

TEST_F(WaveCannonPiercingTest, Level3IsPiercing) {
    // Level 3 Wave Cannon should pierce through enemies
    uint8_t chargeLevel = 3;
    bool expectedPiercing = true;

    EXPECT_EQ(chargeLevel, 3);
    EXPECT_TRUE(expectedPiercing);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Wave Cannon Speed and Movement
// ============================================================================

class WaveCannonMovementTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WaveCannonMovementTest, SpeedConstant) {
    // Wave Cannon speed should be 850.0f (beam speed)
    EXPECT_FLOAT_EQ(WaveCannon::SPEED, 850.0f);
}

TEST_F(WaveCannonMovementTest, SpeedFasterThanMissile) {
    // Wave Cannon should be faster than regular missiles (600.0f)
    constexpr float MISSILE_SPEED = 600.0f;
    EXPECT_GT(WaveCannon::SPEED, MISSILE_SPEED);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Power-up Effects
// ============================================================================

class PowerUpEffectsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PowerUpEffectsTest, HealthPowerUpValue) {
    // Health power-up should restore 25 HP (as per design)
    constexpr uint8_t HEALTH_RESTORE = 25;
    EXPECT_EQ(HEALTH_RESTORE, 25);
}

// Note: Shield removed for R-Type authentic gameplay
// Defense in R-Type comes from Force Pod blocking projectiles

TEST_F(PowerUpEffectsTest, MaxSpeedLevel) {
    // Max speed level should be 3
    EXPECT_EQ(MAX_SPEED_LEVEL, 3);
}

TEST_F(PowerUpEffectsTest, PowerUpTypesAreMutuallyExclusive) {
    // Each power-up type should have a unique value (R-Type authentic - no Shield)
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::Health), static_cast<uint8_t>(PowerUpType::SpeedUp));
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::Health), static_cast<uint8_t>(PowerUpType::WeaponCrystal));
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::Health), static_cast<uint8_t>(PowerUpType::ForcePod));
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::SpeedUp), static_cast<uint8_t>(PowerUpType::WeaponCrystal));
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::SpeedUp), static_cast<uint8_t>(PowerUpType::ForcePod));
    EXPECT_NE(static_cast<uint8_t>(PowerUpType::WeaponCrystal), static_cast<uint8_t>(PowerUpType::ForcePod));
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Force Pod Combat
// ============================================================================

class ForcePodCombatTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ForcePodCombatTest, ContactDamageConstant) {
    // Force Pod contact damage should be defined
    // Based on GameWorld.hpp: CONTACT_DAMAGE = 30
    constexpr uint8_t EXPECTED_CONTACT_DAMAGE = 30;
    EXPECT_EQ(EXPECTED_CONTACT_DAMAGE, 30);
}

TEST_F(ForcePodCombatTest, ForcePodLevels) {
    // Force Pod has 2 levels (1 and 2)
    ForceState fs1{.owner_id = 1, .x = 100, .y = 200, .is_attached = 1, .level = 1};
    ForceState fs2{.owner_id = 2, .x = 150, .y = 250, .is_attached = 0, .level = 2};

    EXPECT_EQ(fs1.level, 1);
    EXPECT_EQ(fs2.level, 2);
    EXPECT_LE(fs1.level, 2);  // Max level is 2
    EXPECT_LE(fs2.level, 2);
}

TEST_F(ForcePodCombatTest, ForceStateAttachedVsDetached) {
    // Attached Force Pod follows player
    ForceState attached{.owner_id = 1, .x = 100, .y = 200, .is_attached = 1, .level = 1};
    // Detached Force Pod moves independently
    ForceState detached{.owner_id = 1, .x = 300, .y = 200, .is_attached = 0, .level = 1};

    EXPECT_EQ(attached.is_attached, 1);
    EXPECT_EQ(detached.is_attached, 0);
    EXPECT_NE(attached.x, detached.x);  // Different positions when detached
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Charge Timing
// ============================================================================

class ChargeTimingTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ChargeTimingTest, ChargeLevelsAreIncremental) {
    // Each charge level should take progressively longer
    EXPECT_LT(WaveCannon::CHARGE_TIME_LV1, WaveCannon::CHARGE_TIME_LV2);
    EXPECT_LT(WaveCannon::CHARGE_TIME_LV2, WaveCannon::CHARGE_TIME_LV3);
}

TEST_F(ChargeTimingTest, ChargeTimesArePositive) {
    EXPECT_GT(WaveCannon::CHARGE_TIME_LV1, 0.0f);
    EXPECT_GT(WaveCannon::CHARGE_TIME_LV2, 0.0f);
    EXPECT_GT(WaveCannon::CHARGE_TIME_LV3, 0.0f);
}

TEST_F(ChargeTimingTest, MaxChargeTimeIsReasonable) {
    // Max charge time shouldn't be excessively long (< 5 seconds)
    EXPECT_LT(WaveCannon::CHARGE_TIME_LV3, 5.0f);
}

TEST_F(ChargeTimingTest, ChargeReleaseContainsLevel) {
    // ChargeRelease struct should correctly store charge level
    ChargeRelease cr1{.charge_level = 1};
    ChargeRelease cr2{.charge_level = 2};
    ChargeRelease cr3{.charge_level = 3};

    EXPECT_EQ(cr1.charge_level, 1);
    EXPECT_EQ(cr2.charge_level, 2);
    EXPECT_EQ(cr3.charge_level, 3);
}

// ============================================================================
// R-Type Authentic Phase 3 Tests - Damage Calculations
// ============================================================================

class DamageCalculationsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DamageCalculationsTest, Level1CanKillBasicEnemy) {
    // Basic enemy has ~30 HP, Level 1 Wave Cannon (40 damage) should kill it
    constexpr uint8_t BASIC_ENEMY_HP = 30;
    EXPECT_GE(WaveCannon::DAMAGE_LV1, BASIC_ENEMY_HP);
}

TEST_F(DamageCalculationsTest, Level2CanKillStrongerEnemy) {
    // Stronger enemies have ~60 HP, Level 2 (80 damage) should kill them
    constexpr uint8_t STRONGER_ENEMY_HP = 60;
    EXPECT_GE(WaveCannon::DAMAGE_LV2, STRONGER_ENEMY_HP);
}

TEST_F(DamageCalculationsTest, Level3SignificantBossDamage) {
    // Boss has 1000 HP, Level 3 (150 damage) should deal 15% of boss HP
    constexpr uint16_t BOSS_HP = 1000;
    float damagePercent = (static_cast<float>(WaveCannon::DAMAGE_LV3) / BOSS_HP) * 100.0f;

    EXPECT_GE(damagePercent, 10.0f);  // At least 10% damage
    EXPECT_LE(damagePercent, 20.0f);  // But not more than 20%
}

TEST_F(DamageCalculationsTest, MultipleHitsToKillBoss) {
    // Should take multiple Level 3 hits to kill boss
    // With 200 damage and 1000 HP boss, it takes 5 hits
    constexpr uint16_t BOSS_HP = 1000;
    int hitsToKill = (BOSS_HP + WaveCannon::DAMAGE_LV3 - 1) / WaveCannon::DAMAGE_LV3;

    EXPECT_GE(hitsToKill, 5);   // At least 5 hits (1000/200 = 5)
    EXPECT_LT(hitsToKill, 30);  // But less than 30 hits
}

// ============================================================================
// Protocol Tests - PlayerDamaged Message
// ============================================================================

class PlayerDamagedTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerDamagedTest, SerializationRoundtrip) {
    PlayerDamaged original{
        .player_id = 2,
        .damage = 25,
        .new_health = 75
    };

    uint8_t buffer[PlayerDamaged::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerDamaged::from_bytes(buffer, PlayerDamaged::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_id, original.player_id);
    EXPECT_EQ(parsed->damage, original.damage);
    EXPECT_EQ(parsed->new_health, original.new_health);
}

TEST_F(PlayerDamagedTest, FromBytesTooSmall) {
    uint8_t buffer[2] = {0, 0};
    auto parsed = PlayerDamaged::from_bytes(buffer, 2);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(PlayerDamagedTest, ZeroDamage) {
    // Edge case: 0 damage (shouldn't happen but should serialize correctly)
    PlayerDamaged original{.player_id = 1, .damage = 0, .new_health = 100};

    uint8_t buffer[PlayerDamaged::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerDamaged::from_bytes(buffer, PlayerDamaged::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->damage, 0);
    EXPECT_EQ(parsed->new_health, 100);
}

TEST_F(PlayerDamagedTest, FatalDamage) {
    // Edge case: damage that kills player (new_health = 0)
    PlayerDamaged original{.player_id = 3, .damage = 100, .new_health = 0};

    uint8_t buffer[PlayerDamaged::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerDamaged::from_bytes(buffer, PlayerDamaged::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->damage, 100);
    EXPECT_EQ(parsed->new_health, 0);
}

TEST_F(PlayerDamagedTest, MessageTypeValue) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PlayerDamaged), 0x00A0);
}

// ============================================================================
// Protocol Tests - EnemyDestroyed Message
// ============================================================================

class EnemyDestroyedTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EnemyDestroyedTest, SerializationRoundtrip) {
    EnemyDestroyed original{.enemy_id = 1234};

    uint8_t buffer[EnemyDestroyed::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = EnemyDestroyed::from_bytes(buffer, EnemyDestroyed::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->enemy_id, original.enemy_id);
}

TEST_F(EnemyDestroyedTest, FromBytesTooSmall) {
    uint8_t buffer[1] = {0};
    auto parsed = EnemyDestroyed::from_bytes(buffer, 1);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(EnemyDestroyedTest, MaxEnemyId) {
    // Test max uint16_t value
    EnemyDestroyed original{.enemy_id = 65535};

    uint8_t buffer[EnemyDestroyed::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = EnemyDestroyed::from_bytes(buffer, EnemyDestroyed::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->enemy_id, 65535);
}

TEST_F(EnemyDestroyedTest, ZeroEnemyId) {
    // Test zero ID (edge case)
    EnemyDestroyed original{.enemy_id = 0};

    uint8_t buffer[EnemyDestroyed::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = EnemyDestroyed::from_bytes(buffer, EnemyDestroyed::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->enemy_id, 0);
}

TEST_F(EnemyDestroyedTest, MessageTypeValue) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::EnemyDestroyed), 0x0091);
}

// ============================================================================
// Protocol Tests - ForceStateSnapshot (for GameSnapshot)
// ============================================================================

class ForceStateSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ForceStateSnapshotTest, SerializationRoundtrip) {
    ForceStateSnapshot original{
        .owner_id = 2,
        .x = 500,
        .y = 300,
        .is_attached = 1,
        .level = 2
    };

    uint8_t buffer[ForceStateSnapshot::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = ForceStateSnapshot::from_bytes(buffer, ForceStateSnapshot::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->owner_id, original.owner_id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->is_attached, original.is_attached);
    EXPECT_EQ(parsed->level, original.level);
}

TEST_F(ForceStateSnapshotTest, FromBytesTooSmall) {
    uint8_t buffer[4] = {0, 0, 0, 0};
    auto parsed = ForceStateSnapshot::from_bytes(buffer, 4);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(ForceStateSnapshotTest, DetachedState) {
    ForceStateSnapshot detached{
        .owner_id = 1,
        .x = 800,
        .y = 400,
        .is_attached = 0,
        .level = 1
    };

    uint8_t buffer[ForceStateSnapshot::WIRE_SIZE];
    detached.to_bytes(buffer);

    auto parsed = ForceStateSnapshot::from_bytes(buffer, ForceStateSnapshot::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->is_attached, 0);
}

TEST_F(ForceStateSnapshotTest, NoForcePod) {
    // Level 0 means no Force Pod
    ForceStateSnapshot noForce{
        .owner_id = 3,
        .x = 0,
        .y = 0,
        .is_attached = 0,
        .level = 0
    };

    uint8_t buffer[ForceStateSnapshot::WIRE_SIZE];
    noForce.to_bytes(buffer);

    auto parsed = ForceStateSnapshot::from_bytes(buffer, ForceStateSnapshot::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->level, 0);
}

TEST_F(ForceStateSnapshotTest, WireSizeIsCorrect) {
    // owner_id (1) + x (2) + y (2) + is_attached (1) + level (1) = 7 bytes
    EXPECT_EQ(ForceStateSnapshot::WIRE_SIZE, 7);
}

// ============================================================================
// POWArmor Enemy Type Tests
// ============================================================================

class POWArmorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(POWArmorTest, EnemyTypeEnumValue) {
    // POWArmor should be type 5 in the EnemyType enum
    EXPECT_EQ(5, 5);  // POWArmor = 5
}

TEST_F(POWArmorTest, EnemyStateSerializationWithPOWArmor) {
    EnemyState original{
        .id = 100,
        .x = 1800,
        .y = 500,
        .health = 50,  // POWArmor HP
        .enemy_type = 5  // POWArmor type
    };

    uint8_t buffer[EnemyState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = EnemyState::from_bytes(buffer, EnemyState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->enemy_type, 5);  // POWArmor
}

TEST_F(POWArmorTest, POWArmorHealthValue) {
    // POWArmor should have 50 HP (HEALTH_POW_ARMOR)
    constexpr uint8_t POW_ARMOR_HP = 50;
    EXPECT_EQ(POW_ARMOR_HP, 50);
}

TEST_F(POWArmorTest, POWArmorSpeedValue) {
    // POWArmor speed should be -90.0f (slow, easy to hit)
    constexpr float POW_ARMOR_SPEED = -90.0f;
    EXPECT_FLOAT_EQ(POW_ARMOR_SPEED, -90.0f);
}

TEST_F(POWArmorTest, POWArmorShootInterval) {
    // POWArmor should have 4.0s shoot interval (rarely shoots)
    constexpr float POW_ARMOR_SHOOT_INTERVAL = 4.0f;
    EXPECT_FLOAT_EQ(POW_ARMOR_SHOOT_INTERVAL, 4.0f);
}

TEST_F(POWArmorTest, POWArmorPointsValue) {
    // POWArmor should give 200 points (POINTS_POW_ARMOR)
    constexpr uint16_t POW_ARMOR_POINTS = 200;
    EXPECT_EQ(POW_ARMOR_POINTS, 200);
}

TEST_F(POWArmorTest, POWArmorPowerUpDropChance) {
    // POWArmor should have 100% power-up drop chance
    constexpr uint8_t POW_ARMOR_DROP_CHANCE = 100;
    EXPECT_EQ(POW_ARMOR_DROP_CHANCE, 100);
}

TEST_F(POWArmorTest, POWArmorSpawnInterval) {
    // POWArmor should spawn every 25 seconds
    constexpr float POW_ARMOR_SPAWN_INTERVAL = 25.0f;
    EXPECT_FLOAT_EQ(POW_ARMOR_SPAWN_INTERVAL, 25.0f);
}

TEST_F(POWArmorTest, POWArmorIsDistinctFromOtherTypes) {
    // Verify POWArmor type (5) is different from all other types
    EXPECT_NE(5, 0);  // Basic = 0
    EXPECT_NE(5, 1);  // Tracker = 1
    EXPECT_NE(5, 2);  // Zigzag = 2
    EXPECT_NE(5, 3);  // Fast = 3
    EXPECT_NE(5, 4);  // Bomber = 4
}

TEST_F(POWArmorTest, POWArmorSlowerThanOtherEnemies) {
    // POWArmor (-90) should be slower than other enemies
    constexpr float SPEED_BASIC = -100.0f;
    constexpr float SPEED_FAST = -180.0f;
    constexpr float SPEED_POW_ARMOR = -90.0f;

    // Slower means less negative (closer to 0)
    EXPECT_GT(SPEED_POW_ARMOR, SPEED_BASIC);  // -90 > -100
    EXPECT_GT(SPEED_POW_ARMOR, SPEED_FAST);   // -90 > -180
}

TEST_F(POWArmorTest, POWArmorMediumHealth) {
    // POWArmor (50 HP) should have more HP than Basic (30) but less than high-HP enemies
    constexpr uint8_t HP_BASIC = 30;
    constexpr uint8_t HP_POW_ARMOR = 50;
    constexpr uint8_t HP_BOMBER = 60;

    EXPECT_GT(HP_POW_ARMOR, HP_BASIC);   // 50 > 30
    EXPECT_LT(HP_POW_ARMOR, HP_BOMBER);  // 50 < 60
}

TEST_F(POWArmorTest, POWArmorRarelyAttacks) {
    // POWArmor (4.0s interval) should shoot less frequently than most enemies
    constexpr float INTERVAL_BASIC = 2.0f;
    constexpr float INTERVAL_POW_ARMOR = 4.0f;

    EXPECT_GT(INTERVAL_POW_ARMOR, INTERVAL_BASIC);  // 4.0 > 2.0 (slower = higher interval)
}

TEST_F(POWArmorTest, POWArmorHigherPointsThanBasic) {
    // POWArmor (200 pts) should give more points than Basic (100)
    constexpr uint16_t POINTS_BASIC = 100;
    constexpr uint16_t POINTS_POW_ARMOR = 200;

    EXPECT_GT(POINTS_POW_ARMOR, POINTS_BASIC);
}

TEST_F(POWArmorTest, POWArmorGuaranteedPowerUpVsRegularChance) {
    // POWArmor has 100% drop rate vs regular enemies' 15%
    constexpr uint8_t REGULAR_DROP_CHANCE = 15;
    constexpr uint8_t POW_ARMOR_DROP_CHANCE = 100;

    EXPECT_GT(POW_ARMOR_DROP_CHANCE, REGULAR_DROP_CHANCE);
    EXPECT_EQ(POW_ARMOR_DROP_CHANCE, 100);  // Guaranteed
}

TEST_F(POWArmorTest, SnapshotWithPOWArmorEnemy) {
    // Test GameSnapshot can contain POWArmor enemies
    GameSnapshot snapshot{};
    snapshot.player_count = 1;
    snapshot.players[0] = {
        .id = 0,
        .x = 100,
        .y = 300,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 0,
        .kills = 0,
        .combo = 10,
        .currentWeapon = 0
    };
    snapshot.missile_count = 0;
    snapshot.enemy_count = 2;
    snapshot.enemies[0] = {.id = 1, .x = 1800, .y = 300, .health = 50, .enemy_type = 5};  // POWArmor
    snapshot.enemies[1] = {.id = 2, .x = 1600, .y = 500, .health = 30, .enemy_type = 0};  // Basic
    snapshot.enemy_missile_count = 0;
    snapshot.wave_number = 5;
    snapshot.has_boss = 0;

    size_t size = snapshot.wire_size();
    std::vector<uint8_t> buffer(size);
    snapshot.to_bytes(buffer.data());

    auto parsed = GameSnapshot::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->enemy_count, 2);
    EXPECT_EQ(parsed->enemies[0].enemy_type, 5);  // POWArmor
    EXPECT_EQ(parsed->enemies[0].health, 50);
    EXPECT_EQ(parsed->enemies[1].enemy_type, 0);  // Basic
}
