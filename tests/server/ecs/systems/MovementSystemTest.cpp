/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MovementSystem Tests - Verifies position updates based on velocity
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/VelocityComp.hpp"
#include "infrastructure/ecs/systems/MovementSystem.hpp"

using namespace infrastructure::ecs::components;
using namespace infrastructure::ecs::systems;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class MovementSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
    }

    ECS::ECS _ecs;
    MovementSystem _system;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Movement Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MovementSystemTest, EntityMovesCorrectly) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;
    pos.y = 100.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = 200.0f;  // 200 pixels/second to the right
    vel.y = 50.0f;   // 50 pixels/second down

    // Update with 16ms delta (60 FPS)
    _system.Update(_ecs, 0, 16);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(entity);
    // Expected: x = 100 + 200 * 0.016 = 103.2
    // Expected: y = 100 + 50 * 0.016 = 100.8
    EXPECT_NEAR(newPos.x, 103.2f, 0.01f);
    EXPECT_NEAR(newPos.y, 100.8f, 0.01f);
}

TEST_F(MovementSystemTest, VelocityMultipliedByDeltaTime) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 0.0f;
    pos.y = 0.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = 1000.0f;  // 1000 pixels/second
    vel.y = 0.0f;

    // Update with 100ms delta
    _system.Update(_ecs, 0, 100);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(entity);
    // Expected: x = 0 + 1000 * 0.1 = 100
    EXPECT_NEAR(newPos.x, 100.0f, 0.01f);

    // Update with 500ms delta
    _system.Update(_ecs, 0, 500);

    auto& finalPos = _ecs.entityGetComponent<PositionComp>(entity);
    // Expected: x = 100 + 1000 * 0.5 = 600
    EXPECT_NEAR(finalPos.x, 600.0f, 0.01f);
}

TEST_F(MovementSystemTest, EntityWithoutVelocityNotAffected) {
    auto entityWithVel = _ecs.entityCreate();
    auto entityWithoutVel = _ecs.entityCreate();

    // First entity has both components
    auto& pos1 = _ecs.entityAddComponent<PositionComp>(entityWithVel);
    pos1.x = 100.0f;
    pos1.y = 100.0f;
    auto& vel1 = _ecs.entityAddComponent<VelocityComp>(entityWithVel);
    vel1.x = 200.0f;
    vel1.y = 0.0f;

    // Second entity only has position
    auto& pos2 = _ecs.entityAddComponent<PositionComp>(entityWithoutVel);
    pos2.x = 500.0f;
    pos2.y = 500.0f;

    _system.Update(_ecs, 0, 100);

    // First entity should have moved
    auto& newPos1 = _ecs.entityGetComponent<PositionComp>(entityWithVel);
    EXPECT_NEAR(newPos1.x, 120.0f, 0.01f);  // 100 + 200 * 0.1

    // Second entity should stay in place
    auto& newPos2 = _ecs.entityGetComponent<PositionComp>(entityWithoutVel);
    EXPECT_FLOAT_EQ(newPos2.x, 500.0f);
    EXPECT_FLOAT_EQ(newPos2.y, 500.0f);
}

TEST_F(MovementSystemTest, NegativeVelocity) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 500.0f;
    pos.y = 400.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = -120.0f;  // Moving left (like an enemy)
    vel.y = 0.0f;

    _system.Update(_ecs, 0, 1000);  // 1 second

    auto& newPos = _ecs.entityGetComponent<PositionComp>(entity);
    // Expected: x = 500 + (-120) * 1.0 = 380
    EXPECT_NEAR(newPos.x, 380.0f, 0.01f);
    EXPECT_FLOAT_EQ(newPos.y, 400.0f);
}

TEST_F(MovementSystemTest, ZeroVelocity) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 200.0f;
    pos.y = 300.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = 0.0f;
    vel.y = 0.0f;

    _system.Update(_ecs, 0, 1000);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(entity);
    EXPECT_FLOAT_EQ(newPos.x, 200.0f);
    EXPECT_FLOAT_EQ(newPos.y, 300.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Multiple Entities Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MovementSystemTest, MultipleEntitiesMove) {
    // Create 3 entities with different velocities
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();
    auto e3 = _ecs.entityCreate();

    auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
    pos1.x = 0.0f;
    pos1.y = 0.0f;
    auto& vel1 = _ecs.entityAddComponent<VelocityComp>(e1);
    vel1.x = 100.0f;
    vel1.y = 0.0f;

    auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
    pos2.x = 100.0f;
    pos2.y = 100.0f;
    auto& vel2 = _ecs.entityAddComponent<VelocityComp>(e2);
    vel2.x = 0.0f;
    vel2.y = -100.0f;  // Moving up

    auto& pos3 = _ecs.entityAddComponent<PositionComp>(e3);
    pos3.x = 200.0f;
    pos3.y = 200.0f;
    auto& vel3 = _ecs.entityAddComponent<VelocityComp>(e3);
    vel3.x = -50.0f;
    vel3.y = 50.0f;

    _system.Update(_ecs, 0, 1000);  // 1 second

    auto& newPos1 = _ecs.entityGetComponent<PositionComp>(e1);
    auto& newPos2 = _ecs.entityGetComponent<PositionComp>(e2);
    auto& newPos3 = _ecs.entityGetComponent<PositionComp>(e3);

    EXPECT_NEAR(newPos1.x, 100.0f, 0.01f);
    EXPECT_NEAR(newPos1.y, 0.0f, 0.01f);

    EXPECT_NEAR(newPos2.x, 100.0f, 0.01f);
    EXPECT_NEAR(newPos2.y, 0.0f, 0.01f);

    EXPECT_NEAR(newPos3.x, 150.0f, 0.01f);
    EXPECT_NEAR(newPos3.y, 250.0f, 0.01f);
}

// ═══════════════════════════════════════════════════════════════════════════
// R-Type Specific Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MovementSystemTest, MissileMovement) {
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(missile);
    pos.x = 100.0f;
    pos.y = 540.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(missile);
    vel.x = 600.0f;  // Standard missile speed
    vel.y = 0.0f;

    // Simulate 1 frame at 60 FPS
    _system.Update(_ecs, 0, 16);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(missile);
    // Expected: x = 100 + 600 * 0.016 = 109.6
    EXPECT_NEAR(newPos.x, 109.6f, 0.1f);
}

TEST_F(MovementSystemTest, EnemyMovement) {
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(enemy);
    pos.x = 1920.0f;  // Start at right edge
    pos.y = 300.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(enemy);
    vel.x = -120.0f;  // Basic enemy speed
    vel.y = 0.0f;

    // Simulate 1 second
    _system.Update(_ecs, 0, 1000);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(enemy);
    EXPECT_NEAR(newPos.x, 1800.0f, 0.01f);  // 1920 - 120
}

TEST_F(MovementSystemTest, PowerUpDrift) {
    auto powerup = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);

    auto& pos = _ecs.entityAddComponent<PositionComp>(powerup);
    pos.x = 800.0f;
    pos.y = 400.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(powerup);
    vel.x = -50.0f;  // Slow drift left
    vel.y = 0.0f;

    _system.Update(_ecs, 0, 1000);

    auto& newPos = _ecs.entityGetComponent<PositionComp>(powerup);
    EXPECT_NEAR(newPos.x, 750.0f, 0.01f);
}
