/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ECS Integration Tests - Verifies ECS core functionality with R-Type components
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/VelocityComp.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/components/LifetimeComp.hpp"
#include "infrastructure/ecs/components/OwnerComp.hpp"

using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// ECS Setup Helper
// ═══════════════════════════════════════════════════════════════════════════

class ECSIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register all components
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
        _ecs.registerComponent<HealthComp>();
        _ecs.registerComponent<HitboxComp>();
        _ecs.registerComponent<LifetimeComp>();
        _ecs.registerComponent<OwnerComp>();
    }

    ECS::ECS _ecs;
};

// ═══════════════════════════════════════════════════════════════════════════
// Entity Creation Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, CreateEntity) {
    auto entity = _ecs.entityCreate();
    EXPECT_TRUE(_ecs.entityIsActive(entity));
}

TEST_F(ECSIntegrationTest, CreateMultipleEntities) {
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();
    auto e3 = _ecs.entityCreate();

    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST_F(ECSIntegrationTest, CreateEntityWithGroup) {
    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    EXPECT_TRUE(_ecs.entityIsActive(player));
    EXPECT_TRUE(_ecs.entityIsActive(enemy));
    EXPECT_TRUE(_ecs.entityIsActive(missile));
}

// ═══════════════════════════════════════════════════════════════════════════
// Component Operations Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, AddComponent) {
    auto entity = _ecs.entityCreate();
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;
    pos.y = 200.0f;

    EXPECT_TRUE(_ecs.entityHasComponent<PositionComp>(entity));
}

TEST_F(ECSIntegrationTest, GetComponent) {
    auto entity = _ecs.entityCreate();
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 150.0f;
    pos.y = 300.0f;

    auto& retrieved = _ecs.entityGetComponent<PositionComp>(entity);
    EXPECT_FLOAT_EQ(retrieved.x, 150.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 300.0f);
}

TEST_F(ECSIntegrationTest, ModifyComponent) {
    auto entity = _ecs.entityCreate();
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 0.0f;
    pos.y = 0.0f;

    auto& posRef = _ecs.entityGetComponent<PositionComp>(entity);
    posRef.x = 500.0f;
    posRef.y = 400.0f;

    auto& pos2 = _ecs.entityGetComponent<PositionComp>(entity);
    EXPECT_FLOAT_EQ(pos2.x, 500.0f);
    EXPECT_FLOAT_EQ(pos2.y, 400.0f);
}

TEST_F(ECSIntegrationTest, AddMultipleComponents) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;
    pos.y = 100.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = -120.0f;
    vel.y = 0.0f;

    auto& health = _ecs.entityAddComponent<HealthComp>(entity);
    health.current = 40;
    health.max = 40;

    EXPECT_TRUE(_ecs.entityHasComponent<PositionComp>(entity));
    EXPECT_TRUE(_ecs.entityHasComponent<VelocityComp>(entity));
    EXPECT_TRUE(_ecs.entityHasComponent<HealthComp>(entity));
}

TEST_F(ECSIntegrationTest, RemoveComponent) {
    auto entity = _ecs.entityCreate();
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;
    pos.y = 100.0f;

    EXPECT_TRUE(_ecs.entityHasComponent<PositionComp>(entity));

    _ecs.entityRemoveComponent<PositionComp>(entity);

    EXPECT_FALSE(_ecs.entityHasComponent<PositionComp>(entity));
}

// ═══════════════════════════════════════════════════════════════════════════
// Entity Deletion Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, DeleteEntity) {
    auto entity = _ecs.entityCreate();
    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;

    _ecs.entityDelete(entity);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

TEST_F(ECSIntegrationTest, DeleteEntityRemovesFromActiveCount) {
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();

    EXPECT_EQ(_ecs.currentEntityCount(), 2);

    _ecs.entityDelete(e1);

    EXPECT_EQ(_ecs.currentEntityCount(), 1);
    EXPECT_FALSE(_ecs.entityIsActive(e1));
    EXPECT_TRUE(_ecs.entityIsActive(e2));
}

// ═══════════════════════════════════════════════════════════════════════════
// Entity Group Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, GetEntitiesByGroup) {
    auto p1 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto p2 = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto players = _ecs.getEntityGroup(ECS::EntityGroup::PLAYERS);
    auto enemies = _ecs.getEntityGroup(ECS::EntityGroup::ENEMIES);

    EXPECT_EQ(players.size(), 2);
    EXPECT_EQ(enemies.size(), 1);
}

TEST_F(ECSIntegrationTest, EntityGroupContainsCorrectEntities) {
    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto players = _ecs.getEntityGroup(ECS::EntityGroup::PLAYERS);

    bool found = false;
    for (auto e : players) {
        if (e == player) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ═══════════════════════════════════════════════════════════════════════════
// Query Tests (getEntitiesByComponentsAllOf)
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, QuerySingleComponent) {
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();
    _ecs.entityCreate();  // e3 has no position

    _ecs.entityAddComponent<PositionComp>(e1);
    _ecs.entityAddComponent<PositionComp>(e2);

    auto entities = _ecs.getEntitiesByComponentsAllOf<PositionComp>();
    EXPECT_EQ(entities.size(), 2);
}

TEST_F(ECSIntegrationTest, QueryMultipleComponents) {
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();
    auto e3 = _ecs.entityCreate();

    // e1: Position + Velocity
    _ecs.entityAddComponent<PositionComp>(e1);
    _ecs.entityAddComponent<VelocityComp>(e1);

    // e2: Position only
    _ecs.entityAddComponent<PositionComp>(e2);

    // e3: Position + Velocity + Health
    _ecs.entityAddComponent<PositionComp>(e3);
    _ecs.entityAddComponent<VelocityComp>(e3);
    _ecs.entityAddComponent<HealthComp>(e3);

    auto moving = _ecs.getEntitiesByComponentsAllOf<PositionComp, VelocityComp>();
    EXPECT_EQ(moving.size(), 2);  // e1 and e3
}

TEST_F(ECSIntegrationTest, QueryNoMatches) {
    auto e1 = _ecs.entityCreate();
    _ecs.entityAddComponent<PositionComp>(e1);

    auto entities = _ecs.getEntitiesByComponentsAllOf<VelocityComp>();
    EXPECT_EQ(entities.size(), 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// R-Type Specific Entity Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ECSIntegrationTest, CreatePlayerEntity) {
    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);

    auto& pos = _ecs.entityAddComponent<PositionComp>(player);
    pos.x = 100.0f;
    pos.y = 540.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(player);
    vel.x = 0.0f;
    vel.y = 0.0f;

    auto& health = _ecs.entityAddComponent<HealthComp>(player);
    health.current = 100;
    health.max = 100;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(player);
    hitbox.width = 50.0f;
    hitbox.height = 30.0f;

    EXPECT_TRUE(_ecs.entityHasComponent<PositionComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<VelocityComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<HealthComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<HitboxComp>(player));

    auto& healthCheck = _ecs.entityGetComponent<HealthComp>(player);
    EXPECT_EQ(healthCheck.current, 100);
}

TEST_F(ECSIntegrationTest, CreateMissileEntity) {
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(missile);
    pos.x = 200.0f;
    pos.y = 540.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(missile);
    vel.x = 600.0f;
    vel.y = 0.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(missile);
    hitbox.width = 16.0f;
    hitbox.height = 8.0f;

    auto& owner = _ecs.entityAddComponent<OwnerComp>(missile);
    owner.ownerId = 1;
    owner.isPlayerOwned = true;

    auto& ownerCheck = _ecs.entityGetComponent<OwnerComp>(missile);
    EXPECT_EQ(ownerCheck.ownerId, 1);
    EXPECT_TRUE(ownerCheck.isPlayerOwned);
}

TEST_F(ECSIntegrationTest, CreateEnemyEntity) {
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    auto& pos = _ecs.entityAddComponent<PositionComp>(enemy);
    pos.x = 1920.0f;
    pos.y = 300.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(enemy);
    vel.x = -120.0f;
    vel.y = 0.0f;

    auto& health = _ecs.entityAddComponent<HealthComp>(enemy);
    health.current = 40;
    health.max = 40;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(enemy);
    hitbox.width = 40.0f;
    hitbox.height = 40.0f;

    auto& healthCheck = _ecs.entityGetComponent<HealthComp>(enemy);
    EXPECT_EQ(healthCheck.current, 40);
    EXPECT_EQ(healthCheck.max, 40);
}

TEST_F(ECSIntegrationTest, CreatePowerUpEntity) {
    auto powerup = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);

    auto& pos = _ecs.entityAddComponent<PositionComp>(powerup);
    pos.x = 500.0f;
    pos.y = 400.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(powerup);
    vel.x = -50.0f;
    vel.y = 0.0f;

    auto& hitbox = _ecs.entityAddComponent<HitboxComp>(powerup);
    hitbox.width = 32.0f;
    hitbox.height = 32.0f;

    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(powerup);
    lifetime.remaining = 10.0f;
    lifetime.total = 10.0f;

    auto& lifetimeCheck = _ecs.entityGetComponent<LifetimeComp>(powerup);
    EXPECT_FLOAT_EQ(lifetimeCheck.remaining, 10.0f);
    EXPECT_FLOAT_EQ(lifetimeCheck.total, 10.0f);
}

TEST_F(ECSIntegrationTest, SimulateMovementUpdate) {
    auto entity = _ecs.entityCreate();

    auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
    pos.x = 100.0f;
    pos.y = 100.0f;

    auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
    vel.x = 200.0f;
    vel.y = 50.0f;

    // Simulate a movement system update (deltaTime = 0.016 = 60 FPS)
    float dt = 0.016f;
    auto& posRef = _ecs.entityGetComponent<PositionComp>(entity);
    auto& velRef = _ecs.entityGetComponent<VelocityComp>(entity);

    posRef.x += velRef.x * dt;
    posRef.y += velRef.y * dt;

    EXPECT_NEAR(posRef.x, 103.2f, 0.01f);
    EXPECT_NEAR(posRef.y, 100.8f, 0.01f);
}
