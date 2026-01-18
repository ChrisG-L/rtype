/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LifetimeSystem Tests - Verifies timed entity expiration
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/LifetimeComp.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/systems/LifetimeSystem.hpp"

using namespace infrastructure::ecs::components;
using namespace infrastructure::ecs::systems;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class LifetimeSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<LifetimeComp>();
        _ecs.registerComponent<PositionComp>();
    }

    ECS::ECS _ecs;
    LifetimeSystem _system;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Lifetime Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(LifetimeSystemTest, LifetimeDecrements) {
    auto entity = _ecs.entityCreate();
    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
    lifetime.remaining = 10.0f;
    lifetime.total = 10.0f;

    // Update with 1 second
    _system.Update(_ecs, 0, 1000);

    auto& updated = _ecs.entityGetComponent<LifetimeComp>(entity);
    EXPECT_NEAR(updated.remaining, 9.0f, 0.01f);
}

TEST_F(LifetimeSystemTest, EntityDeletedWhenLifetimeExpires) {
    auto entity = _ecs.entityCreate();
    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
    lifetime.remaining = 1.0f;
    lifetime.total = 1.0f;

    EXPECT_TRUE(_ecs.entityIsActive(entity));

    // Update with 2 seconds (exceeds lifetime)
    _system.Update(_ecs, 0, 2000);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

TEST_F(LifetimeSystemTest, EntityDeletedWhenLifetimeReachesExactlyZero) {
    auto entity = _ecs.entityCreate();
    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
    lifetime.remaining = 1.0f;
    lifetime.total = 1.0f;

    // Update with exactly 1 second
    _system.Update(_ecs, 0, 1000);

    EXPECT_FALSE(_ecs.entityIsActive(entity));
}

TEST_F(LifetimeSystemTest, MultipleEntitiesWithDifferentLifetimes) {
    auto e1 = _ecs.entityCreate();
    auto e2 = _ecs.entityCreate();
    auto e3 = _ecs.entityCreate();

    auto& lt1 = _ecs.entityAddComponent<LifetimeComp>(e1);
    lt1.remaining = 5.0f;
    lt1.total = 5.0f;

    auto& lt2 = _ecs.entityAddComponent<LifetimeComp>(e2);
    lt2.remaining = 2.0f;
    lt2.total = 2.0f;

    auto& lt3 = _ecs.entityAddComponent<LifetimeComp>(e3);
    lt3.remaining = 10.0f;
    lt3.total = 10.0f;

    // After 3 seconds: e2 should be deleted, e1 and e3 should remain
    _system.Update(_ecs, 0, 3000);

    EXPECT_TRUE(_ecs.entityIsActive(e1));
    EXPECT_FALSE(_ecs.entityIsActive(e2));
    EXPECT_TRUE(_ecs.entityIsActive(e3));

    // Check remaining values
    auto& lt1_updated = _ecs.entityGetComponent<LifetimeComp>(e1);
    auto& lt3_updated = _ecs.entityGetComponent<LifetimeComp>(e3);

    EXPECT_NEAR(lt1_updated.remaining, 2.0f, 0.01f);
    EXPECT_NEAR(lt3_updated.remaining, 7.0f, 0.01f);
}

TEST_F(LifetimeSystemTest, EntityWithoutLifetimeNotAffected) {
    auto entityWithLifetime = _ecs.entityCreate();
    auto entityWithoutLifetime = _ecs.entityCreate();

    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entityWithLifetime);
    lifetime.remaining = 1.0f;
    lifetime.total = 1.0f;

    // Add position to second entity (no lifetime)
    _ecs.entityAddComponent<PositionComp>(entityWithoutLifetime);

    // Update with 2 seconds
    _system.Update(_ecs, 0, 2000);

    EXPECT_FALSE(_ecs.entityIsActive(entityWithLifetime));
    EXPECT_TRUE(_ecs.entityIsActive(entityWithoutLifetime));
}

// ═══════════════════════════════════════════════════════════════════════════
// R-Type Specific Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(LifetimeSystemTest, PowerUpExpiration) {
    // Power-ups have 10 second lifetime
    auto powerup = _ecs.entityCreate(ECS::EntityGroup::POWERUPS);
    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(powerup);
    lifetime.remaining = 10.0f;
    lifetime.total = 10.0f;

    // After 5 seconds
    _system.Update(_ecs, 0, 5000);
    EXPECT_TRUE(_ecs.entityIsActive(powerup));

    auto& remaining = _ecs.entityGetComponent<LifetimeComp>(powerup);
    EXPECT_NEAR(remaining.remaining, 5.0f, 0.01f);

    // After another 6 seconds (total 11)
    _system.Update(_ecs, 0, 6000);
    EXPECT_FALSE(_ecs.entityIsActive(powerup));
}

TEST_F(LifetimeSystemTest, ProgressCalculation) {
    auto entity = _ecs.entityCreate();
    auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
    lifetime.remaining = 10.0f;
    lifetime.total = 10.0f;

    _system.Update(_ecs, 0, 3000);

    auto& updated = _ecs.entityGetComponent<LifetimeComp>(entity);
    // Progress = remaining / total = 7 / 10 = 0.7 (70% remaining)
    float progress = updated.remaining / updated.total;
    EXPECT_NEAR(progress, 0.7f, 0.01f);
}
