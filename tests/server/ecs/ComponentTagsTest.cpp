/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Component Tags Tests - Verifies ECS tag components for Phase 2
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/MissileTag.hpp"
#include "infrastructure/ecs/components/EnemyTag.hpp"
#include "infrastructure/ecs/components/EnemyAIComp.hpp"
#include "infrastructure/ecs/components/PowerUpTag.hpp"
#include "infrastructure/ecs/components/WaveCannonTag.hpp"

using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class ComponentTagsTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<MissileTag>();
        _ecs.registerComponent<EnemyTag>();
        _ecs.registerComponent<EnemyAIComp>();
        _ecs.registerComponent<PowerUpTag>();
        _ecs.registerComponent<WaveCannonTag>();
    }

    ECS::ECS _ecs;
};

// ═══════════════════════════════════════════════════════════════════════════
// MissileTag Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, MissileTag_DefaultValues) {
    MissileTag tag;
    EXPECT_EQ(tag.weaponType, 0);
    EXPECT_EQ(tag.baseDamage, 20);
    EXPECT_FALSE(tag.isHoming);
    EXPECT_EQ(tag.targetId, 0);
}

TEST_F(ComponentTagsTest, MissileTag_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto& tag = _ecs.entityAddComponent<MissileTag>(entity);

    tag.weaponType = 3;  // Missile type
    tag.baseDamage = 50;
    tag.isHoming = true;
    tag.targetId = 42;

    EXPECT_TRUE(_ecs.entityHasComponent<MissileTag>(entity));

    auto& retrieved = _ecs.entityGetComponent<MissileTag>(entity);
    EXPECT_EQ(retrieved.weaponType, 3);
    EXPECT_EQ(retrieved.baseDamage, 50);
    EXPECT_TRUE(retrieved.isHoming);
    EXPECT_EQ(retrieved.targetId, 42);
}

TEST_F(ComponentTagsTest, MissileTag_WeaponTypes) {
    auto standardMissile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto spreadMissile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto laserMissile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto homingMissile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& standard = _ecs.entityAddComponent<MissileTag>(standardMissile);
    standard.weaponType = 0;  // Standard
    standard.baseDamage = 20;

    auto& spread = _ecs.entityAddComponent<MissileTag>(spreadMissile);
    spread.weaponType = 1;  // Spread
    spread.baseDamage = 8;

    auto& laser = _ecs.entityAddComponent<MissileTag>(laserMissile);
    laser.weaponType = 2;  // Laser
    laser.baseDamage = 12;

    auto& homing = _ecs.entityAddComponent<MissileTag>(homingMissile);
    homing.weaponType = 3;  // Missile (homing)
    homing.baseDamage = 50;
    homing.isHoming = true;

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag>();
    EXPECT_EQ(missiles.size(), 4);
}

// ═══════════════════════════════════════════════════════════════════════════
// EnemyTag Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, EnemyTag_DefaultValues) {
    EnemyTag tag;
    EXPECT_EQ(tag.type, 0);
    EXPECT_EQ(tag.points, 100);
}

TEST_F(ComponentTagsTest, EnemyTag_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto& tag = _ecs.entityAddComponent<EnemyTag>(entity);

    tag.type = 4;  // Bomber
    tag.points = 250;

    EXPECT_TRUE(_ecs.entityHasComponent<EnemyTag>(entity));

    auto& retrieved = _ecs.entityGetComponent<EnemyTag>(entity);
    EXPECT_EQ(retrieved.type, 4);
    EXPECT_EQ(retrieved.points, 250);
}

TEST_F(ComponentTagsTest, EnemyTag_AllTypes) {
    // Test all enemy types from Protocol
    struct EnemyTypeInfo {
        uint8_t type;
        uint16_t points;
    };

    std::vector<EnemyTypeInfo> types = {
        {0, 100},   // Basic
        {1, 150},   // Tracker
        {2, 120},   // Zigzag
        {3, 180},   // Fast
        {4, 250},   // Bomber
        {5, 200}    // POWArmor
    };

    for (const auto& info : types) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
        auto& tag = _ecs.entityAddComponent<EnemyTag>(entity);
        tag.type = info.type;
        tag.points = info.points;
    }

    auto enemies = _ecs.getEntitiesByComponentsAllOf<EnemyTag>();
    EXPECT_EQ(enemies.size(), 6);
}

// ═══════════════════════════════════════════════════════════════════════════
// EnemyAIComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, EnemyAIComp_DefaultValues) {
    EnemyAIComp ai;
    EXPECT_FLOAT_EQ(ai.shootCooldown, 0.0f);
    EXPECT_FLOAT_EQ(ai.shootInterval, 2.5f);
    EXPECT_EQ(ai.movementPattern, 0);
    EXPECT_FLOAT_EQ(ai.patternTimer, 0.0f);
    EXPECT_FLOAT_EQ(ai.baseY, 0.0f);
    EXPECT_FLOAT_EQ(ai.aliveTime, 0.0f);
    EXPECT_FLOAT_EQ(ai.phaseOffset, 0.0f);
    EXPECT_FLOAT_EQ(ai.targetY, 0.0f);
    EXPECT_FLOAT_EQ(ai.zigzagTimer, 0.0f);
    EXPECT_TRUE(ai.zigzagUp);
}

TEST_F(ComponentTagsTest, EnemyAIComp_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);

    ai.shootCooldown = 1.5f;
    ai.shootInterval = 2.0f;
    ai.movementPattern = 1;  // Sine
    ai.baseY = 300.0f;
    ai.aliveTime = 5.0f;
    ai.phaseOffset = 0.5f;

    EXPECT_TRUE(_ecs.entityHasComponent<EnemyAIComp>(entity));

    auto& retrieved = _ecs.entityGetComponent<EnemyAIComp>(entity);
    EXPECT_FLOAT_EQ(retrieved.shootCooldown, 1.5f);
    EXPECT_FLOAT_EQ(retrieved.shootInterval, 2.0f);
    EXPECT_EQ(retrieved.movementPattern, 1);
    EXPECT_FLOAT_EQ(retrieved.baseY, 300.0f);
}

TEST_F(ComponentTagsTest, EnemyAIComp_TrackerBehavior) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);

    ai.movementPattern = 3;  // Tracker
    ai.targetY = 540.0f;  // Player Y position

    auto& retrieved = _ecs.entityGetComponent<EnemyAIComp>(entity);
    EXPECT_EQ(retrieved.movementPattern, 3);
    EXPECT_FLOAT_EQ(retrieved.targetY, 540.0f);
}

TEST_F(ComponentTagsTest, EnemyAIComp_ZigzagBehavior) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);

    ai.movementPattern = 2;  // Zigzag
    ai.zigzagTimer = 0.5f;
    ai.zigzagUp = false;
    ai.baseY = 400.0f;

    auto& retrieved = _ecs.entityGetComponent<EnemyAIComp>(entity);
    EXPECT_EQ(retrieved.movementPattern, 2);
    EXPECT_FLOAT_EQ(retrieved.zigzagTimer, 0.5f);
    EXPECT_FALSE(retrieved.zigzagUp);
}

// ═══════════════════════════════════════════════════════════════════════════
// PowerUpTag Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, PowerUpTag_DefaultValues) {
    PowerUpTag tag;
    EXPECT_EQ(tag.type, 0);
}

TEST_F(ComponentTagsTest, PowerUpTag_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);
    auto& tag = _ecs.entityAddComponent<PowerUpTag>(entity);

    tag.type = 2;  // WeaponCrystal

    EXPECT_TRUE(_ecs.entityHasComponent<PowerUpTag>(entity));

    auto& retrieved = _ecs.entityGetComponent<PowerUpTag>(entity);
    EXPECT_EQ(retrieved.type, 2);
}

TEST_F(ComponentTagsTest, PowerUpTag_AllTypes) {
    // Test all power-up types from Protocol
    // 0=Health, 1=SpeedUp, 2=WeaponCrystal, 3=ForcePod, 4=BitDevice
    for (uint8_t type = 0; type < 5; ++type) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);
        auto& tag = _ecs.entityAddComponent<PowerUpTag>(entity);
        tag.type = type;
    }

    auto powerups = _ecs.getEntitiesByComponentsAllOf<PowerUpTag>();
    EXPECT_EQ(powerups.size(), 5);
}

// ═══════════════════════════════════════════════════════════════════════════
// WaveCannonTag Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, WaveCannonTag_DefaultValues) {
    WaveCannonTag tag;
    EXPECT_EQ(tag.chargeLevel, 1);
    EXPECT_FLOAT_EQ(tag.width, 20.0f);
}

TEST_F(ComponentTagsTest, WaveCannonTag_AddToEntity) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::WAVE_CANNONS);
    auto& tag = _ecs.entityAddComponent<WaveCannonTag>(entity);

    tag.chargeLevel = 3;
    tag.width = 55.0f;

    EXPECT_TRUE(_ecs.entityHasComponent<WaveCannonTag>(entity));

    auto& retrieved = _ecs.entityGetComponent<WaveCannonTag>(entity);
    EXPECT_EQ(retrieved.chargeLevel, 3);
    EXPECT_FLOAT_EQ(retrieved.width, 55.0f);
}

TEST_F(ComponentTagsTest, WaveCannonTag_ChargeLevels) {
    // Level 1: 20.0 width
    auto wc1 = _ecs.entityCreate(ECS::EntityGroup::WAVE_CANNONS);
    auto& tag1 = _ecs.entityAddComponent<WaveCannonTag>(wc1);
    tag1.chargeLevel = 1;
    tag1.width = 20.0f;

    // Level 2: 35.0 width
    auto wc2 = _ecs.entityCreate(ECS::EntityGroup::WAVE_CANNONS);
    auto& tag2 = _ecs.entityAddComponent<WaveCannonTag>(wc2);
    tag2.chargeLevel = 2;
    tag2.width = 35.0f;

    // Level 3: 55.0 width
    auto wc3 = _ecs.entityCreate(ECS::EntityGroup::WAVE_CANNONS);
    auto& tag3 = _ecs.entityAddComponent<WaveCannonTag>(wc3);
    tag3.chargeLevel = 3;
    tag3.width = 55.0f;

    auto waveCannons = _ecs.getEntitiesByComponentsAllOf<WaveCannonTag>();
    EXPECT_EQ(waveCannons.size(), 3);

    // Verify each level
    auto& r1 = _ecs.entityGetComponent<WaveCannonTag>(wc1);
    auto& r2 = _ecs.entityGetComponent<WaveCannonTag>(wc2);
    auto& r3 = _ecs.entityGetComponent<WaveCannonTag>(wc3);

    EXPECT_EQ(r1.chargeLevel, 1);
    EXPECT_FLOAT_EQ(r1.width, 20.0f);
    EXPECT_EQ(r2.chargeLevel, 2);
    EXPECT_FLOAT_EQ(r2.width, 35.0f);
    EXPECT_EQ(r3.chargeLevel, 3);
    EXPECT_FLOAT_EQ(r3.width, 55.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Combined Entity Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ComponentTagsTest, CombineEnemyTagWithAI) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& tag = _ecs.entityAddComponent<EnemyTag>(entity);
    tag.type = 1;  // Tracker
    tag.points = 150;

    auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);
    ai.movementPattern = 3;  // Tracker pattern
    ai.shootInterval = 2.0f;
    ai.targetY = 400.0f;

    EXPECT_TRUE(_ecs.entityHasComponent<EnemyTag>(entity));
    EXPECT_TRUE(_ecs.entityHasComponent<EnemyAIComp>(entity));

    // Query for entities with both components
    auto enemies = _ecs.getEntitiesByComponentsAllOf<EnemyTag, EnemyAIComp>();
    EXPECT_EQ(enemies.size(), 1);
}
