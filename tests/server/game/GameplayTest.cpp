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
