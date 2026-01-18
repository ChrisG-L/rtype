/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyAISystem Tests - Verifies enemy AI movement and shooting
*/

#include <gtest/gtest.h>
#include <memory>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "systems/EnemyAISystem.hpp"
#include "components/EnemyTag.hpp"
#include "components/EnemyAIComp.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/PlayerTag.hpp"

using namespace infrastructure::ecs;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class EnemyAISystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _bridge = std::make_unique<bridge::DomainBridge>(
            _gameRule, _collisionRule, _enemyBehavior
        );
        _ecs.registerComponent<EnemyTag>();
        _ecs.registerComponent<EnemyAIComp>();
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
        _ecs.registerComponent<PlayerTag>();
    }

    ECS::EntityID createEnemy(uint8_t type, float x = 700.0f, float y = 300.0f) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
        _ecs.entityAddComponent<EnemyTag>(entity).type = type;
        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;
        auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);
        ai.baseY = y;
        ai.shootCooldown = 2.0f;  // Default shoot interval
        ai.shootInterval = 2.0f;
        _ecs.entityAddComponent<VelocityComp>(entity);
        return entity;
    }

    ECS::EntityID createPlayer(float x = 100.0f, float y = 300.0f) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
        _ecs.entityAddComponent<PlayerTag>(entity).playerId = 1;
        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;
        return entity;
    }

    ECS::ECS _ecs;
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
    std::unique_ptr<bridge::DomainBridge> _bridge;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Movement Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, BasicEnemy_MovesLeft) {
    auto enemy = createEnemy(0);  // Basic enemy
    auto& pos = _ecs.entityGetComponent<PositionComp>(enemy);
    float initialX = pos.x;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);  // 100ms

    auto& vel = _ecs.entityGetComponent<VelocityComp>(enemy);
    EXPECT_LT(vel.x, 0.0f);  // Moving left (negative X)
}

TEST_F(EnemyAISystemTest, Enemy_AliveTimeIncrements) {
    auto enemy = createEnemy(0);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    EXPECT_FLOAT_EQ(ai.aliveTime, 0.0f);

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 1000);  // 1 second

    EXPECT_NEAR(ai.aliveTime, 1.0f, 0.01f);
}

TEST_F(EnemyAISystemTest, TrackerEnemy_TargetsPlayer) {
    createPlayer(100.0f, 100.0f);  // Player at Y=100
    auto enemy = createEnemy(1, 700.0f, 300.0f);  // Tracker enemy at Y=300

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    // Tracker should have targetY set to player position
    EXPECT_NEAR(ai.targetY, 100.0f, 1.0f);
}

TEST_F(EnemyAISystemTest, ZigzagEnemy_UpdatesZigzagState) {
    auto enemy = createEnemy(2);  // Zigzag enemy
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.zigzagTimer = 0.0f;
    ai.zigzagUp = true;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 500);  // 500ms

    // Zigzag timer should have updated
    EXPECT_GT(ai.zigzagTimer, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Shooting Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, Enemy_ShootCooldownDecrements) {
    auto enemy = createEnemy(0);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.shootCooldown = 2.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 500);  // 500ms

    EXPECT_LT(ai.shootCooldown, 2.0f);
}

TEST_F(EnemyAISystemTest, Enemy_ShootsWhenCooldownExpires) {
    auto enemy = createEnemy(0, 700.0f, 300.0f);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.shootCooldown = 0.0f;  // Ready to shoot

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    auto requests = system.getMissileRequests();
    EXPECT_GE(requests.size(), 1);
}

TEST_F(EnemyAISystemTest, Enemy_MissileRequestHasCorrectPosition) {
    auto enemy = createEnemy(0, 700.0f, 300.0f);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.shootCooldown = 0.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    auto requests = system.getMissileRequests();
    ASSERT_GE(requests.size(), 1);

    // Missile should spawn slightly left of enemy
    EXPECT_LT(requests[0].x, 700.0f);
    EXPECT_FLOAT_EQ(requests[0].y, 300.0f);
}

TEST_F(EnemyAISystemTest, Enemy_CooldownResetsAfterShooting) {
    auto enemy = createEnemy(0);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.shootCooldown = 0.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    EXPECT_GT(ai.shootCooldown, 0.0f);  // Cooldown was reset
}

TEST_F(EnemyAISystemTest, GetMissileRequests_ClearsAfterRetrieval) {
    auto enemy = createEnemy(0);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    ai.shootCooldown = 0.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    auto requests1 = system.getMissileRequests();
    EXPECT_GE(requests1.size(), 1);

    auto requests2 = system.getMissileRequests();
    EXPECT_EQ(requests2.size(), 0);  // Cleared
}

// ═══════════════════════════════════════════════════════════════════════════
// Multiple Enemies Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, MultipleEnemies_IndependentMovement) {
    auto e1 = createEnemy(0, 700.0f, 100.0f);  // Basic
    auto e2 = createEnemy(1, 700.0f, 300.0f);  // Tracker
    auto e3 = createEnemy(2, 700.0f, 500.0f);  // Zigzag

    createPlayer(100.0f, 300.0f);

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    // All should have velocities set
    auto& vel1 = _ecs.entityGetComponent<VelocityComp>(e1);
    auto& vel2 = _ecs.entityGetComponent<VelocityComp>(e2);
    auto& vel3 = _ecs.entityGetComponent<VelocityComp>(e3);

    EXPECT_NE(vel1.x, 0.0f);
    EXPECT_NE(vel2.x, 0.0f);
    EXPECT_NE(vel3.x, 0.0f);
}

TEST_F(EnemyAISystemTest, MultipleEnemies_CanShootSimultaneously) {
    auto e1 = createEnemy(0, 700.0f, 100.0f);
    auto e2 = createEnemy(0, 700.0f, 200.0f);
    auto e3 = createEnemy(0, 700.0f, 300.0f);

    // Set all to ready to shoot
    _ecs.entityGetComponent<EnemyAIComp>(e1).shootCooldown = 0.0f;
    _ecs.entityGetComponent<EnemyAIComp>(e2).shootCooldown = 0.0f;
    _ecs.entityGetComponent<EnemyAIComp>(e3).shootCooldown = 0.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    auto requests = system.getMissileRequests();
    EXPECT_EQ(requests.size(), 3);
}

// ═══════════════════════════════════════════════════════════════════════════
// Player Targeting Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, TrackerFindsNearestPlayer) {
    createPlayer(100.0f, 100.0f);  // Player 1 at Y=100
    auto p2 = createPlayer(100.0f, 500.0f);  // Player 2 at Y=500
    _ecs.entityGetComponent<PlayerTag>(p2).playerId = 2;

    // Enemy at Y=400 (closer to player 2)
    auto enemy = createEnemy(1, 700.0f, 400.0f);

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    // Should target player 2 (Y=500) as it's closer
    EXPECT_NEAR(ai.targetY, 500.0f, 1.0f);
}

TEST_F(EnemyAISystemTest, NoPlayers_UseDefaultTarget) {
    auto enemy = createEnemy(1, 700.0f, 300.0f);  // Tracker with no players

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    // Should use default target (screen center ~300)
    EXPECT_NEAR(ai.targetY, 300.0f, 50.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Enemy Type Specific Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, FastEnemy_HasVelocity) {
    auto enemy = createEnemy(3);  // Fast enemy

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(enemy);
    EXPECT_LT(vel.x, 0.0f);  // Moving left
}

TEST_F(EnemyAISystemTest, BomberEnemy_HasVelocity) {
    auto enemy = createEnemy(4);  // Bomber enemy

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(enemy);
    EXPECT_LT(vel.x, 0.0f);  // Moving left
}

TEST_F(EnemyAISystemTest, POWArmorEnemy_HasVelocity) {
    auto enemy = createEnemy(5);  // POWArmor enemy

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(enemy);
    EXPECT_LT(vel.x, 0.0f);  // Moving left
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EnemyAISystemTest, EnemyWithoutVelocityComp_PositionUpdatesDirectly) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    _ecs.entityAddComponent<EnemyTag>(entity).type = 0;
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 700.0f;
    pos.y = 300.0f;
    auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);
    ai.baseY = 300.0f;
    ai.shootCooldown = 10.0f;  // Don't shoot
    // No VelocityComp

    float initialX = pos.x;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 100);

    // Position should have been updated directly
    EXPECT_NE(pos.x, initialX);
}

TEST_F(EnemyAISystemTest, ClearMissileRequests_EmptiesVector) {
    auto enemy = createEnemy(0);
    _ecs.entityGetComponent<EnemyAIComp>(enemy).shootCooldown = 0.0f;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 16);

    system.clearMissileRequests();
    auto requests = system.getMissileRequests();
    EXPECT_EQ(requests.size(), 0);
}

TEST_F(EnemyAISystemTest, ZeroDeltaTime_NoMovement) {
    auto enemy = createEnemy(0);
    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    float initialAliveTime = ai.aliveTime;

    EnemyAISystem system(*_bridge);
    system.Update(_ecs, 0, 0);  // 0 delta time

    EXPECT_FLOAT_EQ(ai.aliveTime, initialAliveTime);
}

