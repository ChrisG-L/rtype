/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionSystem Tests - Verifies AABB collision detection
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/systems/CollisionSystem.hpp"
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

class CollisionSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<HitboxComp>();
    }

    ECS::ECS _ecs;

    // Domain services
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Collision Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CollisionSystemTest, TwoOverlappingEntitiesDetected) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 120.0f;  // Overlapping with e1
    pos2.y = 120.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;

    system.Update(_ecs, 0, 0);

    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 1);
}

TEST_F(CollisionSystemTest, TwoDistantEntitiesNoCollision) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 500.0f;  // Far away
    pos2.y = 500.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;

    system.Update(_ecs, 0, 0);

    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 0);
}

TEST_F(CollisionSystemTest, HitboxOffsetConsideredForCollision) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    // Entity 1 at (100, 100)
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;
    hitbox1.offsetX = 0.0f;
    hitbox1.offsetY = 0.0f;

    // Entity 2 position appears far but offset brings it close
    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 300.0f;
    pos2.y = 300.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;
    hitbox2.offsetX = -200.0f;  // Actual X = 100
    hitbox2.offsetY = -200.0f;  // Actual Y = 100

    system.Update(_ecs, 0, 0);

    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 1);
}

TEST_F(CollisionSystemTest, NoDuplicateCollisions) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    // Overlapping entities
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 100.0f;
    pos2.y = 100.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;

    system.Update(_ecs, 0, 0);

    // Should only have one collision (A,B) not also (B,A)
    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 1);
}

TEST_F(CollisionSystemTest, EntityWithoutHitboxIgnored) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();  // No hitbox
    auto e3 = _ecs.entityCreate();

    // e1 and e3 overlap
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    // e2 has only position (no hitbox)
    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 100.0f;
    pos2.y = 100.0f;
    // No hitbox!

    auto& pos3 = _ecs.entityAddComponent<PositionComp>(e3);
    pos3.x = 100.0f;
    pos3.y = 100.0f;
    auto& hitbox3 = _ecs.entityAddComponent<HitboxComp>(e3);
    hitbox3.width = 50.0f;
    hitbox3.height = 50.0f;

    system.Update(_ecs, 0, 0);

    // Only e1-e3 collision (e2 has no hitbox)
    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Collision Clearing Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CollisionSystemTest, CollisionsClearedOnUpdate) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    // Overlapping entities
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 120.0f;
    pos2.y = 120.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;

    system.Update(_ecs, 0, 0);
    EXPECT_EQ(system.getCollisions().size(), 1);

    // Move e2 far away
    pos2.x = 1000.0f;
    pos2.y = 1000.0f;

    system.Update(_ecs, 0, 0);
    EXPECT_EQ(system.getCollisions().size(), 0);
}

TEST_F(CollisionSystemTest, ClearCollisionsManually) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
    hitbox1.width = 50.0f;
    hitbox1.height = 50.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 100.0f;
    pos2.y = 100.0f;
    auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
    hitbox2.width = 50.0f;
    hitbox2.height = 50.0f;

    system.Update(_ecs, 0, 0);
    EXPECT_EQ(system.getCollisions().size(), 1);

    system.clearCollisions();
    EXPECT_EQ(system.getCollisions().size(), 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Entity Group Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CollisionSystemTest, CollisionEventContainsEntityGroups) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& missilePos = _ecs.entityAddComponent<PositionComp>(missile);
    missilePos.x = 100.0f;
    missilePos.y = 100.0f;
    auto& missileHitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    missileHitbox.width = 16.0f;
    missileHitbox.height = 8.0f;

    auto& enemyPos = _ecs.entityAddComponent<PositionComp>(enemy);
    enemyPos.x = 100.0f;
    enemyPos.y = 100.0f;
    auto& enemyHitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
    enemyHitbox.width = 40.0f;
    enemyHitbox.height = 40.0f;

    system.Update(_ecs, 0, 0);

    const auto& collisions = system.getCollisions();
    ASSERT_EQ(collisions.size(), 1);

    // Check that groups are correctly identified
    bool hasMissile = (collisions[0].groupA == ECS::EntityGroup::MISSILES ||
                       collisions[0].groupB == ECS::EntityGroup::MISSILES);
    bool hasEnemy = (collisions[0].groupA == ECS::EntityGroup::ENEMIES ||
                     collisions[0].groupB == ECS::EntityGroup::ENEMIES);

    EXPECT_TRUE(hasMissile);
    EXPECT_TRUE(hasEnemy);
}

// ═══════════════════════════════════════════════════════════════════════════
// R-Type Specific Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(CollisionSystemTest, MissileHitsEnemy) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    // Missile at typical game position
    auto& missilePos = _ecs.entityAddComponent<PositionComp>(missile);
    missilePos.x = 500.0f;
    missilePos.y = 300.0f;
    auto& missileHitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    missileHitbox.width = 16.0f;
    missileHitbox.height = 8.0f;

    // Enemy at same position
    auto& enemyPos = _ecs.entityAddComponent<PositionComp>(enemy);
    enemyPos.x = 500.0f;
    enemyPos.y = 300.0f;
    auto& enemyHitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
    enemyHitbox.width = 40.0f;
    enemyHitbox.height = 40.0f;

    system.Update(_ecs, 0, 0);

    EXPECT_EQ(system.getCollisions().size(), 1);
}

TEST_F(CollisionSystemTest, PlayerPicksUpPowerUp) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto powerup = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);

    auto& playerPos = _ecs.entityAddComponent<PositionComp>(player);
    playerPos.x = 400.0f;
    playerPos.y = 400.0f;
    auto& playerHitbox = _ecs.entityAddComponent<HitboxComp>(player);
    playerHitbox.width = 50.0f;
    playerHitbox.height = 30.0f;

    auto& powerupPos = _ecs.entityAddComponent<PositionComp>(powerup);
    powerupPos.x = 410.0f;  // Overlapping
    powerupPos.y = 400.0f;
    auto& powerupHitbox = _ecs.entityAddComponent<HitboxComp>(powerup);
    powerupHitbox.width = 32.0f;
    powerupHitbox.height = 32.0f;

    system.Update(_ecs, 0, 0);

    const auto& collisions = system.getCollisions();
    EXPECT_EQ(collisions.size(), 1);

    bool hasPlayer = (collisions[0].groupA == ECS::EntityGroup::PLAYERS ||
                      collisions[0].groupB == ECS::EntityGroup::PLAYERS);
    bool hasPowerup = (collisions[0].groupA == ECS::EntityGroup::POWERUPS ||
                       collisions[0].groupB == ECS::EntityGroup::POWERUPS);

    EXPECT_TRUE(hasPlayer);
    EXPECT_TRUE(hasPowerup);
}

TEST_F(CollisionSystemTest, MultipleCollisionsInOneFrame) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem system(bridge);

    // Create one missile hitting three enemies
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy1 = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto enemy2 = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto enemy3 = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    // Missile in center
    auto& missilePos = _ecs.entityAddComponent<PositionComp>(missile);
    missilePos.x = 500.0f;
    missilePos.y = 300.0f;
    auto& missileHitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    missileHitbox.width = 16.0f;
    missileHitbox.height = 8.0f;

    // All enemies overlapping with missile
    for (auto enemy : {enemy1, enemy2, enemy3}) {
        auto& pos = _ecs.entityAddComponent<PositionComp>(enemy);
        pos.x = 500.0f;
        pos.y = 300.0f;
        auto& hitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
        hitbox.width = 40.0f;
        hitbox.height = 40.0f;
    }

    system.Update(_ecs, 0, 0);

    // Should have collisions: missile-enemy1, missile-enemy2, missile-enemy3
    // Plus enemy-enemy collisions: enemy1-enemy2, enemy1-enemy3, enemy2-enemy3
    // Total: 6 collisions
    EXPECT_EQ(system.getCollisions().size(), 6);
}
