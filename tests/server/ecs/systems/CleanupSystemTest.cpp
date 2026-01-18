/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CleanupSystem Tests - Verifies out-of-bounds entity removal
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/systems/CleanupSystem.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"

using namespace infrastructure::ecs::components;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::bridge;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class CleanupSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<HitboxComp>();
    }

    ECS::ECS _ecs;

    // Domain services (needed for DomainBridge)
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Cleanup Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CleanupSystemTest, EntityInScreenNotDeleted) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 500.0f;  // Middle of screen
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 16.0f;
    hitbox.height = 8.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_TRUE(_ecs.entityIsActive(entity));
}

TEST_F(CleanupSystemTest, EntityOutOfBoundsLeftDeleted) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = -100.0f;  // Fully off-screen left
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 40.0f;
    hitbox.height = 40.0f;

    EXPECT_TRUE(_ecs.entityIsActive(entity));

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

TEST_F(CleanupSystemTest, EntityOutOfBoundsRightDeleted) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 2000.0f;  // Fully off-screen right (screen width is 1920)
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 16.0f;
    hitbox.height = 8.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

TEST_F(CleanupSystemTest, EntityPartiallyOffScreenNotDeleted) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = -5.0f;  // Partially off-screen (left edge)
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 16.0f;  // Extends from -5 to 11 (still visible)
    hitbox.height = 8.0f;

    system.Update(_ecs, 0, 0);

    // Should still be active (partially visible)
    EXPECT_TRUE(_ecs.entityIsActive(entity));
}

// ═══════════════════════════════════════════════════════════════════════════
// Player Exclusion Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CleanupSystemTest, PlayerNotDeletedEvenIfOutOfBounds) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);

    auto& pos = _ecs.entityAddComponent<PositionComp>(player);
    pos.x = -200.0f;  // Fully off-screen
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(player);
    hitbox.width = 50.0f;
    hitbox.height = 30.0f;

    system.Update(_ecs, 0, 0);

    // Player should NOT be deleted
    EXPECT_TRUE(_ecs.entityIsActive(player));
}

TEST_F(CleanupSystemTest, MixedEntitiesOnlyNonPlayersDeleted) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    // All entities are out of bounds
    auto& playerPos = _ecs.entityAddComponent<PositionComp>(player);
    playerPos.x = -200.0f;
    playerPos.y = 400.0f;
    auto& playerHitbox = _ecs.entityAddComponent<HitboxComp>(player);
    playerHitbox.width = 50.0f;
    playerHitbox.height = 30.0f;

    auto& enemyPos = _ecs.entityAddComponent<PositionComp>(enemy);
    enemyPos.x = -200.0f;
    enemyPos.y = 400.0f;
    auto& enemyHitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
    enemyHitbox.width = 40.0f;
    enemyHitbox.height = 40.0f;

    auto& missilePos = _ecs.entityAddComponent<PositionComp>(missile);
    missilePos.x = 2000.0f;  // Off right
    missilePos.y = 400.0f;
    auto& missileHitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    missileHitbox.width = 16.0f;
    missileHitbox.height = 8.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_TRUE(_ecs.entityIsActive(player));   // Player survives
    EXPECT_FALSE(_ecs.entityIsActive(enemy));   // Enemy deleted
    EXPECT_FALSE(_ecs.entityIsActive(missile)); // Missile deleted
}

// ═══════════════════════════════════════════════════════════════════════════
// Hitbox Offset Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CleanupSystemTest, HitboxOffsetConsideredForBounds) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 10.0f;  // Position seems on-screen

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
    hitbox.width = 16.0f;
    hitbox.height = 8.0f;
    hitbox.offsetX = -50.0f;  // But offset moves it off-screen

    // Actual X = 10 + (-50) = -40, which with width 16 means entity is at [-40, -24]
    // This should be off-screen

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

// ═══════════════════════════════════════════════════════════════════════════
// Entity Without Hitbox Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CleanupSystemTest, EntityWithoutHitboxNotProcessed) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto entityWithHitbox = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto entityWithoutHitbox = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    // Entity with hitbox (out of bounds)
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(entityWithHitbox);
    pos1.x = -200.0f;
    pos1.y = 400.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(entityWithHitbox);
    hitbox1.width = 16.0f;
    hitbox1.height = 8.0f;

    // Entity without hitbox (out of bounds but no hitbox)
    auto& pos2 = _ecs.entityAddComponent<PositionComp>(entityWithoutHitbox);
    pos2.x = -200.0f;
    pos2.y = 400.0f;
    // No hitbox added

    system.Update(_ecs, 0, 0);

    // Only entity with hitbox should be deleted
    EXPECT_FALSE(_ecs.entityIsActive(entityWithHitbox));
    EXPECT_TRUE(_ecs.entityIsActive(entityWithoutHitbox));
}

// ═══════════════════════════════════════════════════════════════════════════
// R-Type Specific Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CleanupSystemTest, EnemyLeavingScreenLeft) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    // Enemy that has moved past the left edge
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(enemy);
    pos.x = -50.0f;  // Past left edge
    pos.y = 300.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
    hitbox.width = 40.0f;
    hitbox.height = 40.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(enemy));
}

TEST_F(CleanupSystemTest, MissileExitingRight) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    // Missile that has exited the right side
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(missile);
    pos.x = 1950.0f;  // Past right edge (screen width 1920)
    pos.y = 540.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    hitbox.width = 16.0f;
    hitbox.height = 8.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(missile));
}

TEST_F(CleanupSystemTest, PowerUpDriftingOffScreen) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CleanupSystem system(bridge);

    auto powerup = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);

    auto& pos = _ecs.entityAddComponent<PositionComp>(powerup);
    pos.x = -50.0f;  // Drifted off left
    pos.y = 400.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(powerup);
    hitbox.width = 32.0f;
    hitbox.height = 32.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_FALSE(_ecs.entityIsActive(powerup));
}
