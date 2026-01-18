/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponSystem Tests - Verifies weapon cooldowns and missile spawning
*/

#include <gtest/gtest.h>
#include <memory>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "systems/WeaponSystem.hpp"
#include "components/PlayerTag.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/WeaponComp.hpp"
#include "components/MissileTag.hpp"
#include "components/HitboxComp.hpp"
#include "components/OwnerComp.hpp"
#include "components/LifetimeComp.hpp"
#include "components/WaveCannonTag.hpp"
#include "Protocol.hpp"

using namespace infrastructure::ecs;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class WeaponSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _bridge = std::make_unique<bridge::DomainBridge>(
            _gameRule, _collisionRule, _enemyBehavior
        );
        _ecs.registerComponent<PlayerTag>();
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
        _ecs.registerComponent<WeaponComp>();
        _ecs.registerComponent<MissileTag>();
        _ecs.registerComponent<HitboxComp>();
        _ecs.registerComponent<OwnerComp>();
        _ecs.registerComponent<LifetimeComp>();
        _ecs.registerComponent<WaveCannonTag>();
    }

    ECS::EntityID createPlayer(uint8_t playerId, float x = 100.0f, float y = 300.0f) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
        _ecs.entityAddComponent<PlayerTag>(entity).playerId = playerId;
        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;
        _ecs.entityAddComponent<WeaponComp>(entity);
        return entity;
    }

    size_t countMissiles() {
        return _ecs.getEntitiesByComponentsAllOf<MissileTag>().size();
    }

    size_t countWaveCannons() {
        return _ecs.getEntitiesByComponentsAllOf<WaveCannonTag>().size();
    }

    ECS::ECS _ecs;
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
    std::unique_ptr<bridge::DomainBridge> _bridge;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Shoot Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, Shoot_CreatesMissile) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countMissiles(), 1);
}

TEST_F(WeaponSystemTest, Shoot_MissileHasCorrectPosition) {
    createPlayer(1, 100.0f, 300.0f);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, PositionComp>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& pos = _ecs.entityGetComponent<PositionComp>(missiles[0]);
    // Missile spawns in front of player (offset ~50)
    EXPECT_GT(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 300.0f);
}

TEST_F(WeaponSystemTest, Shoot_MissileHasCorrectOwner) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, OwnerComp>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& owner = _ecs.entityGetComponent<OwnerComp>(missiles[0]);
    EXPECT_EQ(owner.ownerId, 1);
    EXPECT_TRUE(owner.isPlayerOwned);
}

TEST_F(WeaponSystemTest, Shoot_MissileHasVelocity) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, VelocityComp>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& vel = _ecs.entityGetComponent<VelocityComp>(missiles[0]);
    EXPECT_GT(vel.x, 0.0f);  // Moving right
}

TEST_F(WeaponSystemTest, Shoot_MissileHasHitbox) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, HitboxComp>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& hitbox = _ecs.entityGetComponent<HitboxComp>(missiles[0]);
    EXPECT_GT(hitbox.width, 0.0f);
    EXPECT_GT(hitbox.height, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Cooldown Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, Shoot_SetsCooldown) {
    auto player = createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    const auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    EXPECT_GT(weapon.shootCooldown, 0.0f);
}

TEST_F(WeaponSystemTest, Shoot_RespectsCooldown) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    // First shot
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);
    EXPECT_EQ(countMissiles(), 1);

    // Immediate second shot should fail (cooldown active)
    system.queueShoot({1, 1});
    system.Update(_ecs, 0, 16);
    EXPECT_EQ(countMissiles(), 1);  // Still only 1 missile
}

TEST_F(WeaponSystemTest, Cooldown_DecrementsOverTime) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.shootCooldown = 1.0f;

    WeaponSystem system(*_bridge);
    system.Update(_ecs, 0, 500);  // 500ms

    EXPECT_LT(weapon.shootCooldown, 1.0f);
    EXPECT_GT(weapon.shootCooldown, 0.0f);
}

TEST_F(WeaponSystemTest, Cooldown_AllowsShootWhenExpired) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    // First shot
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);
    EXPECT_EQ(countMissiles(), 1);

    // Wait for cooldown to expire (simulate 1 second)
    system.Update(_ecs, 0, 1000);

    // Second shot should work
    system.queueShoot({1, 1});
    system.Update(_ecs, 0, 16);
    EXPECT_EQ(countMissiles(), 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Weapon Type Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, SpreadWeapon_Creates3Missiles) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Spread);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countMissiles(), 3);
}

TEST_F(WeaponSystemTest, SpreadWeapon_MissilesHaveDifferentAngles) {
    auto player = createPlayer(1, 100.0f, 300.0f);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Spread);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, VelocityComp>();
    ASSERT_EQ(missiles.size(), 3);

    // Collect Y velocities
    std::vector<float> yVelocities;
    for (auto entity : missiles) {
        const auto& vel = _ecs.entityGetComponent<VelocityComp>(entity);
        yVelocities.push_back(vel.y);
    }

    // Should have one going up, one straight, one down
    bool hasUp = false, hasStraight = false, hasDown = false;
    for (float vy : yVelocities) {
        if (vy < -1.0f) hasUp = true;
        else if (vy > 1.0f) hasDown = true;
        else hasStraight = true;
    }
    EXPECT_TRUE(hasUp);
    EXPECT_TRUE(hasStraight);
    EXPECT_TRUE(hasDown);
}

TEST_F(WeaponSystemTest, LaserWeapon_CreatesSingleMissile) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Laser);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countMissiles(), 1);
}

TEST_F(WeaponSystemTest, MissileWeapon_MarkedAsHoming) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Missile);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& tag = _ecs.entityGetComponent<MissileTag>(missiles[0]);
    EXPECT_TRUE(tag.isHoming);
}

TEST_F(WeaponSystemTest, StandardWeapon_NotHoming) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Standard);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag>();
    ASSERT_EQ(missiles.size(), 1);

    const auto& tag = _ecs.entityGetComponent<MissileTag>(missiles[0]);
    EXPECT_FALSE(tag.isHoming);
}

// ═══════════════════════════════════════════════════════════════════════════
// Weapon Switch Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, SwitchWeapon_NextWeapon) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = 0;  // Standard

    WeaponSystem system(*_bridge);
    system.queueSwitchWeapon({1, 1});  // Next
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(weapon.currentType, 1);  // Spread
}

TEST_F(WeaponSystemTest, SwitchWeapon_PreviousWeapon) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = 2;  // Laser

    WeaponSystem system(*_bridge);
    system.queueSwitchWeapon({1, -1});  // Previous
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(weapon.currentType, 1);  // Spread
}

TEST_F(WeaponSystemTest, SwitchWeapon_WrapsAroundForward) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = 3;  // Missile (last)

    WeaponSystem system(*_bridge);
    system.queueSwitchWeapon({1, 1});  // Next
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(weapon.currentType, 0);  // Standard (wrapped)
}

TEST_F(WeaponSystemTest, SwitchWeapon_WrapsAroundBackward) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = 0;  // Standard (first)

    WeaponSystem system(*_bridge);
    system.queueSwitchWeapon({1, -1});  // Previous
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(weapon.currentType, 3);  // Missile (wrapped)
}

// ═══════════════════════════════════════════════════════════════════════════
// Wave Cannon Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, ChargeStart_SetsChargingState) {
    auto player = createPlayer(1);

    WeaponSystem system(*_bridge);
    system.queueChargeStart({1});
    system.Update(_ecs, 0, 16);

    const auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    EXPECT_TRUE(weapon.isCharging);
    EXPECT_FLOAT_EQ(weapon.chargeTime, 0.0f);
}

TEST_F(WeaponSystemTest, Charging_AccumulatesTime) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.isCharging = true;
    weapon.chargeTime = 0.0f;

    WeaponSystem system(*_bridge);
    system.Update(_ecs, 0, 500);  // 500ms

    EXPECT_GT(weapon.chargeTime, 0.0f);
    EXPECT_NEAR(weapon.chargeTime, 0.5f, 0.01f);
}

TEST_F(WeaponSystemTest, CannotShootWhileCharging) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.isCharging = true;

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countMissiles(), 0);  // No missile created
}

TEST_F(WeaponSystemTest, ChargeReleaseLv1_CreatesWaveCannon) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.isCharging = true;
    weapon.chargeTime = WaveCannon::CHARGE_TIME_LV1 + 0.1f;

    WeaponSystem system(*_bridge);
    system.queueChargeRelease({1, 1});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countWaveCannons(), 1);
    EXPECT_FALSE(weapon.isCharging);
}

TEST_F(WeaponSystemTest, ChargeReleaseLv3_CorrectChargeLevel) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.isCharging = true;
    weapon.chargeTime = WaveCannon::CHARGE_TIME_LV3 + 0.1f;

    WeaponSystem system(*_bridge);
    system.queueChargeRelease({1, 3});
    system.Update(_ecs, 0, 16);

    auto waveCannons = _ecs.getEntitiesByComponentsAllOf<WaveCannonTag>();
    ASSERT_EQ(waveCannons.size(), 1);

    const auto& tag = _ecs.entityGetComponent<WaveCannonTag>(waveCannons[0]);
    EXPECT_EQ(tag.chargeLevel, 3);
    EXPECT_FLOAT_EQ(tag.width, WaveCannon::WIDTH_LV3);
}

TEST_F(WeaponSystemTest, ChargeReleaseNotEnough_NoWaveCannon) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.isCharging = true;
    weapon.chargeTime = 0.1f;  // Not enough charge

    WeaponSystem system(*_bridge);
    system.queueChargeRelease({1, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countWaveCannons(), 0);
    EXPECT_FALSE(weapon.isCharging);  // Still resets charging state
}

// ═══════════════════════════════════════════════════════════════════════════
// Spawn Event Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, GetSpawnedMissiles_ReturnsEvents) {
    createPlayer(1, 100.0f, 300.0f);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto events = system.getSpawnedMissiles();
    ASSERT_EQ(events.size(), 1);

    EXPECT_EQ(events[0].ownerId, 1);
    EXPECT_GT(events[0].x, 100.0f);
    EXPECT_FLOAT_EQ(events[0].y, 300.0f);
}

TEST_F(WeaponSystemTest, GetSpawnedMissiles_ClearsAfterRetrieval) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto events1 = system.getSpawnedMissiles();
    EXPECT_EQ(events1.size(), 1);

    auto events2 = system.getSpawnedMissiles();
    EXPECT_EQ(events2.size(), 0);  // Cleared
}

TEST_F(WeaponSystemTest, SpreadWeapon_MultipleSpawnEvents) {
    auto player = createPlayer(1);
    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);
    weapon.currentType = static_cast<uint8_t>(WeaponType::Spread);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.Update(_ecs, 0, 16);

    auto events = system.getSpawnedMissiles();
    EXPECT_EQ(events.size(), 3);
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WeaponSystemTest, UnknownPlayer_NoMissile) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({99, 0});  // Player 99 doesn't exist
    EXPECT_NO_THROW(system.Update(_ecs, 0, 16));
    EXPECT_EQ(countMissiles(), 0);
}

TEST_F(WeaponSystemTest, PlayerWithoutWeaponComp_NoMissile) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityAddComponent<PlayerTag>(entity).playerId = 1;
    _ecs.entityAddComponent<PositionComp>(entity);
    // No WeaponComp

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    EXPECT_NO_THROW(system.Update(_ecs, 0, 16));
    EXPECT_EQ(countMissiles(), 0);
}

TEST_F(WeaponSystemTest, ClearRequests_EmptiesAllQueues) {
    createPlayer(1);
    WeaponSystem system(*_bridge);

    system.queueShoot({1, 0});
    system.queueChargeStart({1});
    system.queueSwitchWeapon({1, 1});
    system.clearRequests();
    system.Update(_ecs, 0, 16);

    // Nothing should have happened
    EXPECT_EQ(countMissiles(), 0);
}

TEST_F(WeaponSystemTest, MultiplePlayersIndependent) {
    createPlayer(1, 100.0f, 100.0f);
    createPlayer(2, 100.0f, 200.0f);

    WeaponSystem system(*_bridge);
    system.queueShoot({1, 0});
    system.queueShoot({2, 1});
    system.Update(_ecs, 0, 16);

    EXPECT_EQ(countMissiles(), 2);

    // Verify different owners
    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, OwnerComp>();
    std::vector<uint8_t> owners;
    for (auto entity : missiles) {
        owners.push_back(_ecs.entityGetComponent<OwnerComp>(entity).ownerId);
    }
    EXPECT_NE(owners[0], owners[1]);
}

