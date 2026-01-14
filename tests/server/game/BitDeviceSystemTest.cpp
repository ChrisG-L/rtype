/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** BitDeviceSystemTest - Tests for Bit Device system (R-Type authentic)
** Bit Devices: 2 orbiting satellites that shoot when player shoots
*/

#include <gtest/gtest.h>
#include "infrastructure/game/GameWorld.hpp"
#include "Protocol.hpp"
#include <cmath>

using namespace infrastructure::game;

// ============================================================================
// Tests - BitDevice Constants
// ============================================================================

class BitDeviceConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceConstantsTest, BitDeviceWidth) {
    EXPECT_FLOAT_EQ(BitDevice::WIDTH, 24.0f);
}

TEST_F(BitDeviceConstantsTest, BitDeviceHeight) {
    EXPECT_FLOAT_EQ(BitDevice::HEIGHT, 24.0f);
}

TEST_F(BitDeviceConstantsTest, BitDeviceOrbitRadius) {
    EXPECT_FLOAT_EQ(BitDevice::ORBIT_RADIUS, 50.0f);
}

TEST_F(BitDeviceConstantsTest, BitDeviceOrbitSpeed) {
    EXPECT_FLOAT_EQ(BitDevice::ORBIT_SPEED, 3.0f);  // rad/s
}

TEST_F(BitDeviceConstantsTest, BitDeviceShootCooldown) {
    EXPECT_FLOAT_EQ(BitDevice::SHOOT_COOLDOWN, 0.4f);
}

TEST_F(BitDeviceConstantsTest, BitDeviceContactDamage) {
    EXPECT_EQ(BitDevice::CONTACT_DAMAGE, 20);
}

TEST_F(BitDeviceConstantsTest, BitDeviceHitCooldown) {
    EXPECT_FLOAT_EQ(BitDevice::HIT_COOLDOWN, 0.5f);
}

TEST_F(BitDeviceConstantsTest, BitDeviceSmallerThanForcePod) {
    // Bit Devices should be smaller than Force Pod
    EXPECT_LT(BitDevice::WIDTH, ForcePod::WIDTH);
    EXPECT_LT(BitDevice::HEIGHT, ForcePod::HEIGHT);
}

TEST_F(BitDeviceConstantsTest, BitDeviceLessDamageThanForcePod) {
    // Bit Devices deal less contact damage than Force Pod
    EXPECT_LT(BitDevice::CONTACT_DAMAGE, ForcePod::CONTACT_DAMAGE);
}

TEST_F(BitDeviceConstantsTest, BitDeviceSlowerShootThanForcePod) {
    // Bit Devices shoot slightly slower than Force Pod
    EXPECT_GT(BitDevice::SHOOT_COOLDOWN, ForcePod::SHOOT_COOLDOWN);
}

// ============================================================================
// Tests - BitDevice vs Enemy HP
// ============================================================================

class BitDeviceVsEnemyTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceVsEnemyTest, BitDeviceOneHitKillsFast) {
    // Fast enemy: 20 HP, BitDevice contact: 20 dmg
    constexpr uint8_t FAST_HP = 20;
    EXPECT_GE(BitDevice::CONTACT_DAMAGE, FAST_HP);
}

TEST_F(BitDeviceVsEnemyTest, BitDeviceTwoHitsKillsZigzag) {
    // Zigzag enemy: 25 HP, BitDevice contact: 20 dmg
    constexpr uint8_t ZIGZAG_HP = 25;
    int hitsNeeded = (ZIGZAG_HP + BitDevice::CONTACT_DAMAGE - 1) / BitDevice::CONTACT_DAMAGE;
    EXPECT_EQ(hitsNeeded, 2);
}

TEST_F(BitDeviceVsEnemyTest, BitDeviceTwoHitsKillsTracker) {
    // Tracker enemy: 35 HP, BitDevice contact: 20 dmg
    constexpr uint8_t TRACKER_HP = 35;
    int hitsNeeded = (TRACKER_HP + BitDevice::CONTACT_DAMAGE - 1) / BitDevice::CONTACT_DAMAGE;
    EXPECT_EQ(hitsNeeded, 2);
}

TEST_F(BitDeviceVsEnemyTest, BitDeviceTwoHitsKillsBasic) {
    // Basic enemy: 40 HP, BitDevice contact: 20 dmg
    constexpr uint8_t BASIC_HP = 40;
    int hitsNeeded = (BASIC_HP + BitDevice::CONTACT_DAMAGE - 1) / BitDevice::CONTACT_DAMAGE;
    EXPECT_EQ(hitsNeeded, 2);
}

TEST_F(BitDeviceVsEnemyTest, BitDeviceFourHitsKillsBomber) {
    // Bomber enemy: 80 HP, BitDevice contact: 20 dmg
    constexpr uint8_t BOMBER_HP = 80;
    int hitsNeeded = (BOMBER_HP + BitDevice::CONTACT_DAMAGE - 1) / BitDevice::CONTACT_DAMAGE;
    EXPECT_EQ(hitsNeeded, 4);
}

// ============================================================================
// Tests - BitDeviceStateSnapshot Serialization
// ============================================================================

class BitDeviceSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceSerializationTest, WireSize) {
    EXPECT_EQ(BitDeviceStateSnapshot::WIRE_SIZE, 6);
}

TEST_F(BitDeviceSerializationTest, BasicRoundtrip) {
    BitDeviceStateSnapshot original{
        .owner_id = 1,
        .bit_index = 0,
        .x = 500,
        .y = 300,
        .is_attached = 1
    };

    uint8_t buffer[BitDeviceStateSnapshot::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, BitDeviceStateSnapshot::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->owner_id, 1);
    EXPECT_EQ(parsed->bit_index, 0);
    EXPECT_EQ(parsed->x, 500);
    EXPECT_EQ(parsed->y, 300);
    EXPECT_EQ(parsed->is_attached, 1);
}

TEST_F(BitDeviceSerializationTest, SecondBitRoundtrip) {
    BitDeviceStateSnapshot original{
        .owner_id = 2,
        .bit_index = 1,  // Second bit
        .x = 600,
        .y = 400,
        .is_attached = 1
    };

    uint8_t buffer[BitDeviceStateSnapshot::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, BitDeviceStateSnapshot::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->owner_id, 2);
    EXPECT_EQ(parsed->bit_index, 1);
    EXPECT_EQ(parsed->x, 600);
    EXPECT_EQ(parsed->y, 400);
}

TEST_F(BitDeviceSerializationTest, IsAttachedAlwaysOne) {
    // Note: BitDeviceStateSnapshot doesn't serialize is_attached to save space
    // Bits are always attached when sent over network (no detach feature yet)
    BitDeviceStateSnapshot original{
        .owner_id = 3,
        .bit_index = 0,
        .x = 1000,
        .y = 200,
        .is_attached = 0  // Set to 0 in original
    };

    uint8_t buffer[BitDeviceStateSnapshot::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, BitDeviceStateSnapshot::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    // from_bytes() always sets is_attached = 1 (Bits always orbit)
    EXPECT_EQ(parsed->is_attached, 1);
}

TEST_F(BitDeviceSerializationTest, MaxPositionValues) {
    BitDeviceStateSnapshot original{
        .owner_id = 4,
        .bit_index = 1,
        .x = 1920,  // Max screen width
        .y = 1080,  // Max screen height
        .is_attached = 1
    };

    uint8_t buffer[BitDeviceStateSnapshot::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, BitDeviceStateSnapshot::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->x, 1920);
    EXPECT_EQ(parsed->y, 1080);
}

TEST_F(BitDeviceSerializationTest, InvalidBufferSize) {
    uint8_t buffer[3] = {0};  // Too small
    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, 3);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(BitDeviceSerializationTest, ZeroLengthBuffer) {
    // Note: from_bytes doesn't check for nullptr (would segfault)
    // Test with valid pointer but zero length instead
    uint8_t buffer[1] = {0};
    auto parsed = BitDeviceStateSnapshot::from_bytes(buffer, 0);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// Tests - Protocol Constants
// ============================================================================

class BitDeviceProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceProtocolTest, MaxBitsConstant) {
    EXPECT_EQ(MAX_BITS, 8);  // 2 bits per player × 4 players
}

TEST_F(BitDeviceProtocolTest, BitDevicePowerUpTypeValue) {
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::BitDevice), 4);
}

TEST_F(BitDeviceProtocolTest, PowerUpTypeCountIncludesBitDevice) {
    // Health=0, SpeedUp=1, WeaponCrystal=2, ForcePod=3, BitDevice=4, COUNT=5
    EXPECT_EQ(static_cast<uint8_t>(PowerUpType::COUNT), 5);
}

// ============================================================================
// Tests - Orbit Mechanics
// ============================================================================

class BitDeviceOrbitTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceOrbitTest, OrbitCompleteTime) {
    // Time for one complete orbit (2π radians)
    float orbitTime = (2.0f * M_PI) / BitDevice::ORBIT_SPEED;
    EXPECT_NEAR(orbitTime, 2.094f, 0.01f);  // ~2.1 seconds
}

TEST_F(BitDeviceOrbitTest, OrbitPositionTop) {
    // At angle = -π/2 (top), y should be -ORBIT_RADIUS from center
    float angle = -M_PI / 2.0f;
    float offsetY = std::sin(angle) * BitDevice::ORBIT_RADIUS;
    EXPECT_NEAR(offsetY, -50.0f, 0.01f);
}

TEST_F(BitDeviceOrbitTest, OrbitPositionBottom) {
    // At angle = π/2 (bottom), y should be +ORBIT_RADIUS from center
    float angle = M_PI / 2.0f;
    float offsetY = std::sin(angle) * BitDevice::ORBIT_RADIUS;
    EXPECT_NEAR(offsetY, 50.0f, 0.01f);
}

TEST_F(BitDeviceOrbitTest, OrbitPositionRight) {
    // At angle = 0 (right), x should be +ORBIT_RADIUS from center
    float angle = 0.0f;
    float offsetX = std::cos(angle) * BitDevice::ORBIT_RADIUS;
    EXPECT_NEAR(offsetX, 50.0f, 0.01f);
}

TEST_F(BitDeviceOrbitTest, OrbitPositionLeft) {
    // At angle = π (left), x should be -ORBIT_RADIUS from center
    float angle = M_PI;
    float offsetX = std::cos(angle) * BitDevice::ORBIT_RADIUS;
    EXPECT_NEAR(offsetX, -50.0f, 0.01f);
}

TEST_F(BitDeviceOrbitTest, TwoBitsOppositePositions) {
    // Two bits start at opposite positions (180° apart)
    float angle1 = -M_PI / 2.0f;  // Top
    float angle2 = M_PI / 2.0f;   // Bottom

    float y1 = std::sin(angle1) * BitDevice::ORBIT_RADIUS;
    float y2 = std::sin(angle2) * BitDevice::ORBIT_RADIUS;

    // Should be opposite
    EXPECT_NEAR(y1, -y2, 0.01f);
}

// ============================================================================
// Tests - Shoot Cooldown Mechanics
// ============================================================================

class BitDeviceShootTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceShootTest, CooldownSlowerThanPlayer) {
    // Player base cooldown is 0.3s, Bits are 0.4s
    constexpr float PLAYER_BASE_COOLDOWN = 0.3f;
    EXPECT_GT(BitDevice::SHOOT_COOLDOWN, PLAYER_BASE_COOLDOWN);
}

TEST_F(BitDeviceShootTest, MaxShotsPerSecondPerBit) {
    // Shots per second = 1 / cooldown
    float shotsPerSecond = 1.0f / BitDevice::SHOOT_COOLDOWN;
    EXPECT_NEAR(shotsPerSecond, 2.5f, 0.01f);
}

TEST_F(BitDeviceShootTest, TwoBitsCombinedFireRate) {
    // Two bits together = double the fire rate
    float combinedShotsPerSecond = 2.0f / BitDevice::SHOOT_COOLDOWN;
    EXPECT_NEAR(combinedShotsPerSecond, 5.0f, 0.01f);
}

// ============================================================================
// Tests - BitDevice Struct Initialization
// ============================================================================

class BitDeviceStructTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceStructTest, DefaultIsAttached) {
    BitDevice bit{};
    EXPECT_TRUE(bit.isAttached);
}

TEST_F(BitDeviceStructTest, DefaultShootCooldown) {
    BitDevice bit{};
    EXPECT_FLOAT_EQ(bit.shootCooldown, 0.0f);
}

TEST_F(BitDeviceStructTest, DefaultBossHitCooldown) {
    BitDevice bit{};
    EXPECT_FLOAT_EQ(bit.bossHitCooldown, 0.0f);
}

TEST_F(BitDeviceStructTest, HitCooldownsMapEmpty) {
    BitDevice bit{};
    EXPECT_TRUE(bit.hitCooldowns.empty());
}

// ============================================================================
// Tests - PowerUpType BitDevice Serialization
// ============================================================================

class BitDevicePowerUpSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDevicePowerUpSerializationTest, PowerUpStateBitDevice) {
    PowerUpState state{
        .id = 42,
        .x = 800,
        .y = 450,
        .type = static_cast<uint8_t>(PowerUpType::BitDevice),
        .remaining_time = 8
    };

    uint8_t buffer[PowerUpState::WIRE_SIZE];
    state.to_bytes(buffer);

    auto parsed = PowerUpState::from_bytes(buffer, PowerUpState::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, static_cast<uint8_t>(PowerUpType::BitDevice));
}

TEST_F(BitDevicePowerUpSerializationTest, PowerUpCollectedBitDevice) {
    PowerUpCollected collected{
        .powerup_id = 99,
        .player_id = 2,
        .powerup_type = static_cast<uint8_t>(PowerUpType::BitDevice)
    };

    uint8_t buffer[PowerUpCollected::WIRE_SIZE];
    collected.to_bytes(buffer);

    auto parsed = PowerUpCollected::from_bytes(buffer, PowerUpCollected::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->powerup_type, static_cast<uint8_t>(PowerUpType::BitDevice));
}

// ============================================================================
// Tests - Comparison with Force Pod
// ============================================================================

class BitDeviceVsForcePodTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BitDeviceVsForcePodTest, SizeComparison) {
    // Force Pod: 32x32, Bit: 24x24
    EXPECT_EQ(ForcePod::WIDTH, 32.0f);
    EXPECT_EQ(ForcePod::HEIGHT, 32.0f);
    EXPECT_EQ(BitDevice::WIDTH, 24.0f);
    EXPECT_EQ(BitDevice::HEIGHT, 24.0f);
}

TEST_F(BitDeviceVsForcePodTest, ContactDamageComparison) {
    // Force Pod: 45 dmg, Bit: 20 dmg
    EXPECT_EQ(ForcePod::CONTACT_DAMAGE, 45);
    EXPECT_EQ(BitDevice::CONTACT_DAMAGE, 20);
}

TEST_F(BitDeviceVsForcePodTest, ShootCooldownComparison) {
    // Force Pod: 0.35s, Bit: 0.4s
    EXPECT_FLOAT_EQ(ForcePod::SHOOT_COOLDOWN, 0.35f);
    EXPECT_FLOAT_EQ(BitDevice::SHOOT_COOLDOWN, 0.4f);
}

TEST_F(BitDeviceVsForcePodTest, HitCooldownSame) {
    // Both have same hit cooldown
    EXPECT_FLOAT_EQ(ForcePod::HIT_COOLDOWN, BitDevice::HIT_COOLDOWN);
}

TEST_F(BitDeviceVsForcePodTest, TotalContactDPSComparison) {
    // Force Pod DPS = 45 / 0.5 = 90 per enemy
    // 2 Bits DPS = 2 * (20 / 0.5) = 80 per enemy
    float forcePodDPS = ForcePod::CONTACT_DAMAGE / ForcePod::HIT_COOLDOWN;
    float twoBitsDPS = 2.0f * (BitDevice::CONTACT_DAMAGE / BitDevice::HIT_COOLDOWN);

    EXPECT_NEAR(forcePodDPS, 90.0f, 0.1f);
    EXPECT_NEAR(twoBitsDPS, 80.0f, 0.1f);
    EXPECT_GT(forcePodDPS, twoBitsDPS);
}
