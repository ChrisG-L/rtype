/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PlayerStateExtendedTest - Tests for PlayerState with Phase 3 fields
** (chargeLevel, speedLevel, weaponLevel, hasForce)
** Note: shieldTimer kept in protocol for binary compatibility but always 0 (R-Type authentic)
*/

#include <gtest/gtest.h>
#include "Protocol.hpp"

// ============================================================================
// Tests - PlayerState Wire Size
// ============================================================================

class PlayerStateWireSizeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStateWireSizeTest, WireSizeIs23Bytes) {
    // After adding Phase 3 fields, PlayerState should be 23 bytes
    EXPECT_EQ(PlayerState::WIRE_SIZE, 23);
}

TEST_F(PlayerStateWireSizeTest, WireSizeBreakdown) {
    // Verify each field contributes to wire size:
    // id (1) + x (2) + y (2) + health (1) + alive (1) + lastAckedInputSeq (2) +
    // shipSkin (1) + score (4) + kills (2) + combo (1) + currentWeapon (1) +
    // chargeLevel (1) + speedLevel (1) + weaponLevel (1) + hasForce (1) + shieldTimer (1)
    // = 23 bytes
    constexpr size_t expected = 1 + 2 + 2 + 1 + 1 + 2 + 1 + 4 + 2 + 1 + 1 + 1 + 1 + 1 + 1 + 1;
    EXPECT_EQ(expected, 23);
    EXPECT_EQ(PlayerState::WIRE_SIZE, expected);
}

// ============================================================================
// Tests - Phase 3 Fields Serialization
// ============================================================================

class PlayerStatePhase3FieldsTest : public ::testing::Test {
protected:
    void SetUp() override {}

    PlayerState createDefaultState() {
        return PlayerState{
            .id = 0,
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
            .hasForce = 0,
            .shieldTimer = 0
        };
    }
};

TEST_F(PlayerStatePhase3FieldsTest, ChargeLevelZero) {
    PlayerState state = createDefaultState();
    state.chargeLevel = 0;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 0);
}

TEST_F(PlayerStatePhase3FieldsTest, ChargeLevelOne) {
    PlayerState state = createDefaultState();
    state.chargeLevel = 1;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 1);
}

TEST_F(PlayerStatePhase3FieldsTest, ChargeLevelTwo) {
    PlayerState state = createDefaultState();
    state.chargeLevel = 2;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 2);
}

TEST_F(PlayerStatePhase3FieldsTest, ChargeLevelThree) {
    PlayerState state = createDefaultState();
    state.chargeLevel = 3;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 3);
}

TEST_F(PlayerStatePhase3FieldsTest, SpeedLevelMax) {
    PlayerState state = createDefaultState();
    state.speedLevel = 3;  // MAX_SPEED_LEVEL

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->speedLevel, 3);
}

TEST_F(PlayerStatePhase3FieldsTest, WeaponLevelMax) {
    PlayerState state = createDefaultState();
    state.weaponLevel = 3;  // Max weapon level

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->weaponLevel, 3);
}

TEST_F(PlayerStatePhase3FieldsTest, HasForceTrue) {
    PlayerState state = createDefaultState();
    state.hasForce = 1;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->hasForce, 1);
}

TEST_F(PlayerStatePhase3FieldsTest, HasForceFalse) {
    PlayerState state = createDefaultState();
    state.hasForce = 0;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->hasForce, 0);
}

// R-Type Authentic: Shield power-up removed (defense via Force Pod only)
// shieldTimer field kept in protocol for binary compatibility but always 0
TEST_F(PlayerStatePhase3FieldsTest, ShieldTimerAlwaysZero) {
    PlayerState state = createDefaultState();
    state.shieldTimer = 0;  // Always 0 in authentic R-Type (no shield power-up)

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->shieldTimer, 0);
}

// ============================================================================
// Tests - Buffer Size Validation
// ============================================================================

class PlayerStateBufferValidationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStateBufferValidationTest, FromBytesTooSmall) {
    uint8_t buffer[10] = {0};
    auto parsed = PlayerState::from_bytes(buffer, 10);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(PlayerStateBufferValidationTest, FromBytesExactSize) {
    PlayerState state{
        .id = 1, .x = 100, .y = 200, .health = 100, .alive = 1,
        .lastAckedInputSeq = 0, .shipSkin = 1, .score = 0, .kills = 0,
        .combo = 10, .currentWeapon = 0, .chargeLevel = 0, .speedLevel = 0,
        .weaponLevel = 0, .hasForce = 0, .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    EXPECT_TRUE(parsed.has_value());
}

TEST_F(PlayerStateBufferValidationTest, FromBytesLargerBuffer) {
    PlayerState state{
        .id = 1, .x = 100, .y = 200, .health = 100, .alive = 1,
        .lastAckedInputSeq = 0, .shipSkin = 1, .score = 0, .kills = 0,
        .combo = 10, .currentWeapon = 0, .chargeLevel = 0, .speedLevel = 0,
        .weaponLevel = 0, .hasForce = 0, .shieldTimer = 0
    };

    uint8_t buffer[100] = {0};
    state.to_bytes(buffer);

    // Should work with larger buffer
    auto parsed = PlayerState::from_bytes(buffer, 100);
    EXPECT_TRUE(parsed.has_value());
}

TEST_F(PlayerStateBufferValidationTest, FromBytesNullBuffer) {
    auto parsed = PlayerState::from_bytes(nullptr, PlayerState::WIRE_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// Tests - Full State Roundtrip
// ============================================================================

class PlayerStateFullRoundtripTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStateFullRoundtripTest, AllFieldsPreserved) {
    PlayerState original{
        .id = 3,
        .x = 1500,
        .y = 800,
        .health = 75,
        .alive = 1,
        .lastAckedInputSeq = 12345,
        .shipSkin = 5,
        .score = 999999,
        .kills = 500,
        .combo = 25,  // 2.5x
        .currentWeapon = 3,  // Homing
        .chargeLevel = 2,
        .speedLevel = 3,
        .weaponLevel = 3,
        .hasForce = 1,
        .shieldTimer = 45  // 4.5 seconds
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());

    // Verify all base fields
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->alive, original.alive);
    EXPECT_EQ(parsed->lastAckedInputSeq, original.lastAckedInputSeq);
    EXPECT_EQ(parsed->shipSkin, original.shipSkin);

    // Verify score fields
    EXPECT_EQ(parsed->score, original.score);
    EXPECT_EQ(parsed->kills, original.kills);
    EXPECT_EQ(parsed->combo, original.combo);

    // Verify weapon field
    EXPECT_EQ(parsed->currentWeapon, original.currentWeapon);

    // Verify Phase 3 fields
    EXPECT_EQ(parsed->chargeLevel, original.chargeLevel);
    EXPECT_EQ(parsed->speedLevel, original.speedLevel);
    EXPECT_EQ(parsed->weaponLevel, original.weaponLevel);
    EXPECT_EQ(parsed->hasForce, original.hasForce);
    EXPECT_EQ(parsed->shieldTimer, original.shieldTimer);
}

TEST_F(PlayerStateFullRoundtripTest, MaximumValues) {
    PlayerState original{
        .id = 255,
        .x = 65535,
        .y = 65535,
        .health = 255,
        .alive = 255,
        .lastAckedInputSeq = 65535,
        .shipSkin = 255,
        .score = 0xFFFFFFFF,
        .kills = 65535,
        .combo = 255,
        .currentWeapon = 255,
        .chargeLevel = 255,
        .speedLevel = 255,
        .weaponLevel = 255,
        .hasForce = 255,
        .shieldTimer = 255
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, 255);
    EXPECT_EQ(parsed->x, 65535);
    EXPECT_EQ(parsed->y, 65535);
    EXPECT_EQ(parsed->score, 0xFFFFFFFF);
    EXPECT_EQ(parsed->chargeLevel, 255);
    EXPECT_EQ(parsed->speedLevel, 255);
    EXPECT_EQ(parsed->weaponLevel, 255);
    EXPECT_EQ(parsed->hasForce, 255);
    EXPECT_EQ(parsed->shieldTimer, 255);
}

TEST_F(PlayerStateFullRoundtripTest, MinimumValues) {
    PlayerState original{
        .id = 0,
        .x = 0,
        .y = 0,
        .health = 0,
        .alive = 0,
        .lastAckedInputSeq = 0,
        .shipSkin = 0,
        .score = 0,
        .kills = 0,
        .combo = 0,
        .currentWeapon = 0,
        .chargeLevel = 0,
        .speedLevel = 0,
        .weaponLevel = 0,
        .hasForce = 0,
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, 0);
    EXPECT_EQ(parsed->x, 0);
    EXPECT_EQ(parsed->y, 0);
    EXPECT_EQ(parsed->score, 0);
    EXPECT_EQ(parsed->chargeLevel, 0);
    EXPECT_EQ(parsed->speedLevel, 0);
    EXPECT_EQ(parsed->weaponLevel, 0);
    EXPECT_EQ(parsed->hasForce, 0);
    EXPECT_EQ(parsed->shieldTimer, 0);
}

// ============================================================================
// Tests - Byte Order (Endianness)
// ============================================================================

class PlayerStateEndiannessTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStateEndiannessTest, XYNetworkByteOrder) {
    // x and y are 16-bit and should use network byte order (big-endian)
    PlayerState state{
        .id = 0, .x = 0x1234, .y = 0x5678, .health = 0, .alive = 0,
        .lastAckedInputSeq = 0, .shipSkin = 0, .score = 0, .kills = 0,
        .combo = 0, .currentWeapon = 0, .chargeLevel = 0, .speedLevel = 0,
        .weaponLevel = 0, .hasForce = 0, .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    // x starts at byte 1 (after id)
    // In network byte order (big-endian): 0x12 0x34
    EXPECT_EQ(buffer[1], 0x12);
    EXPECT_EQ(buffer[2], 0x34);

    // y starts at byte 3
    // In network byte order: 0x56 0x78
    EXPECT_EQ(buffer[3], 0x56);
    EXPECT_EQ(buffer[4], 0x78);
}

TEST_F(PlayerStateEndiannessTest, ScoreNetworkByteOrder) {
    PlayerState state{
        .id = 0, .x = 0, .y = 0, .health = 0, .alive = 0,
        .lastAckedInputSeq = 0, .shipSkin = 0, .score = 0x12345678, .kills = 0,
        .combo = 0, .currentWeapon = 0, .chargeLevel = 0, .speedLevel = 0,
        .weaponLevel = 0, .hasForce = 0, .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    // score starts at byte 10 (after shipSkin at byte 9)
    // In network byte order (big-endian): 0x12 0x34 0x56 0x78
    EXPECT_EQ(buffer[10], 0x12);
    EXPECT_EQ(buffer[11], 0x34);
    EXPECT_EQ(buffer[12], 0x56);
    EXPECT_EQ(buffer[13], 0x78);
}

TEST_F(PlayerStateEndiannessTest, Phase3FieldsAreSingleByte) {
    // Phase 3 fields are single bytes, no endianness concerns
    PlayerState state{
        .id = 0, .x = 0, .y = 0, .health = 0, .alive = 0,
        .lastAckedInputSeq = 0, .shipSkin = 0, .score = 0, .kills = 0,
        .combo = 0, .currentWeapon = 0, .chargeLevel = 0xAA, .speedLevel = 0xBB,
        .weaponLevel = 0xCC, .hasForce = 0xDD, .shieldTimer = 0xEE
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    state.to_bytes(buffer);

    // Phase 3 fields are at bytes 18-22
    EXPECT_EQ(buffer[18], 0xAA);  // chargeLevel
    EXPECT_EQ(buffer[19], 0xBB);  // speedLevel
    EXPECT_EQ(buffer[20], 0xCC);  // weaponLevel
    EXPECT_EQ(buffer[21], 0xDD);  // hasForce
    EXPECT_EQ(buffer[22], 0xEE);  // shieldTimer
}

// ============================================================================
// Tests - Typical Game Scenarios
// ============================================================================

class PlayerStateGameScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerStateGameScenariosTest, NewPlayerState) {
    // A freshly spawned player
    PlayerState newPlayer{
        .id = 0,
        .x = 100,
        .y = 540,  // Center of screen
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 0,
        .shipSkin = 1,
        .score = 0,
        .kills = 0,
        .combo = 10,  // 1.0x base combo
        .currentWeapon = 0,  // Standard
        .chargeLevel = 0,
        .speedLevel = 0,
        .weaponLevel = 0,
        .hasForce = 0,
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    newPlayer.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->health, 100);
    EXPECT_EQ(parsed->combo, 10);
    EXPECT_EQ(parsed->speedLevel, 0);
    EXPECT_EQ(parsed->weaponLevel, 0);
}

TEST_F(PlayerStateGameScenariosTest, FullyUpgradedPlayer) {
    // Player with all power-ups maxed
    PlayerState maxedPlayer{
        .id = 1,
        .x = 500,
        .y = 400,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 1000,
        .shipSkin = 3,
        .score = 50000,
        .kills = 100,
        .combo = 30,  // 3.0x max combo
        .currentWeapon = 2,  // Laser
        .chargeLevel = 0,
        .speedLevel = 3,  // Max speed
        .weaponLevel = 3,  // Max weapon
        .hasForce = 1,    // Has Force Pod
        .shieldTimer = 0  // Shield not active
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    maxedPlayer.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->speedLevel, 3);
    EXPECT_EQ(parsed->weaponLevel, 3);
    EXPECT_EQ(parsed->hasForce, 1);
    EXPECT_EQ(parsed->combo, 30);
}

TEST_F(PlayerStateGameScenariosTest, PlayerWithForcePodDefense) {
    // R-Type Authentic: Defense via Force Pod, not shield
    // Player with Force Pod (blocks projectiles)
    PlayerState forcePlayer{
        .id = 2,
        .x = 300,
        .y = 600,
        .health = 80,
        .alive = 1,
        .lastAckedInputSeq = 500,
        .shipSkin = 2,
        .score = 10000,
        .kills = 25,
        .combo = 15,  // 1.5x combo
        .currentWeapon = 1,  // Spread
        .chargeLevel = 0,
        .speedLevel = 1,
        .weaponLevel = 1,
        .hasForce = 1,       // Has Force Pod for defense
        .shieldTimer = 0     // Always 0 (no shield in R-Type authentic)
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    forcePlayer.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->hasForce, 1);
    EXPECT_EQ(parsed->shieldTimer, 0);  // No shield in authentic R-Type
}

TEST_F(PlayerStateGameScenariosTest, ChargingWaveCannon) {
    // Player charging Wave Cannon
    PlayerState chargingPlayer{
        .id = 0,
        .x = 200,
        .y = 500,
        .health = 100,
        .alive = 1,
        .lastAckedInputSeq = 750,
        .shipSkin = 1,
        .score = 8000,
        .kills = 20,
        .combo = 12,
        .currentWeapon = 0,
        .chargeLevel = 2,  // Charging to level 2
        .speedLevel = 0,
        .weaponLevel = 0,
        .hasForce = 0,
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    chargingPlayer.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->chargeLevel, 2);
}

TEST_F(PlayerStateGameScenariosTest, DeadPlayer) {
    // Dead player state
    PlayerState deadPlayer{
        .id = 3,
        .x = 600,
        .y = 400,
        .health = 0,
        .alive = 0,
        .lastAckedInputSeq = 2000,
        .shipSkin = 4,
        .score = 25000,
        .kills = 50,
        .combo = 10,  // Reset to 1.0x
        .currentWeapon = 0,  // Reset to Standard
        .chargeLevel = 0,    // Reset
        .speedLevel = 0,     // Lost on death
        .weaponLevel = 0,    // Lost on death
        .hasForce = 0,       // Lost on death
        .shieldTimer = 0
    };

    uint8_t buffer[PlayerState::WIRE_SIZE];
    deadPlayer.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->alive, 0);
    EXPECT_EQ(parsed->health, 0);
    EXPECT_EQ(parsed->speedLevel, 0);
    EXPECT_EQ(parsed->weaponLevel, 0);
    EXPECT_EQ(parsed->hasForce, 0);
}
