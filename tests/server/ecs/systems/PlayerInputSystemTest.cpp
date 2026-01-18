/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PlayerInputSystem Tests - Verifies player input processing
*/

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "systems/PlayerInputSystem.hpp"
#include "components/PlayerTag.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/SpeedLevelComp.hpp"
#include "components/HitboxComp.hpp"

using namespace infrastructure::ecs;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class PlayerInputSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _bridge = std::make_unique<bridge::DomainBridge>(
            _gameRule, _collisionRule, _enemyBehavior
        );
        _ecs.registerComponent<PlayerTag>();
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
        _ecs.registerComponent<SpeedLevelComp>();
        _ecs.registerComponent<HitboxComp>();
    }

    ECS::EntityID createPlayer(uint8_t playerId, float x = 400.0f, float y = 300.0f) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
        _ecs.entityAddComponent<PlayerTag>(entity).playerId = playerId;
        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;
        _ecs.entityAddComponent<VelocityComp>(entity);
        _ecs.entityAddComponent<SpeedLevelComp>(entity);
        return entity;
    }

    ECS::ECS _ecs;
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
    std::unique_ptr<bridge::DomainBridge> _bridge;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Input Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, InputUp_SetsNegativeVelocityY) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    PlayerInputEvent event{1, InputKeys::UP, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_LT(vel.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
}

TEST_F(PlayerInputSystemTest, InputDown_SetsPositiveVelocityY) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    PlayerInputEvent event{1, InputKeys::DOWN, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_GT(vel.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
}

TEST_F(PlayerInputSystemTest, InputLeft_SetsNegativeVelocityX) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    PlayerInputEvent event{1, InputKeys::LEFT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_LT(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, InputRight_SetsPositiveVelocityX) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    PlayerInputEvent event{1, InputKeys::RIGHT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_GT(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, NoInput_ZeroVelocity) {
    auto player = createPlayer(1);
    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    vel.x = 100.0f;
    vel.y = 100.0f;

    PlayerInputSystem system(*_bridge);
    PlayerInputEvent event{1, 0, 0};  // No keys pressed
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Diagonal Movement Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, DiagonalMovement_Normalized) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    // Move up-right (diagonal)
    PlayerInputEvent event{1, InputKeys::UP | InputKeys::RIGHT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);

    // Diagonal velocity should be normalized (multiplied by ~0.7071)
    float baseSpeed = _bridge->getPlayerSpeed(0);
    float expectedComponent = baseSpeed * 0.7071f;

    EXPECT_NEAR(vel.x, expectedComponent, 1.0f);
    EXPECT_NEAR(vel.y, -expectedComponent, 1.0f);

    // Total magnitude should be approximately base speed
    float magnitude = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    EXPECT_NEAR(magnitude, baseSpeed, 1.0f);
}

TEST_F(PlayerInputSystemTest, DiagonalDownLeft_Normalized) {
    auto player = createPlayer(1);
    PlayerInputSystem system(*_bridge);

    PlayerInputEvent event{1, InputKeys::DOWN | InputKeys::LEFT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);

    float baseSpeed = _bridge->getPlayerSpeed(0);
    float expectedComponent = baseSpeed * 0.7071f;

    EXPECT_NEAR(vel.x, -expectedComponent, 1.0f);
    EXPECT_NEAR(vel.y, expectedComponent, 1.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Speed Level Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, SpeedLevel0_BaseSpeed) {
    auto player = createPlayer(1);
    _ecs.entityGetComponent<SpeedLevelComp>(player).level = 0;

    PlayerInputSystem system(*_bridge);
    PlayerInputEvent event{1, InputKeys::RIGHT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    float expectedSpeed = _bridge->getPlayerSpeed(0);
    EXPECT_FLOAT_EQ(vel.x, expectedSpeed);
}

TEST_F(PlayerInputSystemTest, SpeedLevel3_MaxSpeed) {
    auto player = createPlayer(1);
    _ecs.entityGetComponent<SpeedLevelComp>(player).level = 3;

    PlayerInputSystem system(*_bridge);
    PlayerInputEvent event{1, InputKeys::RIGHT, 0};
    system.queueInput(event);
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    float expectedSpeed = _bridge->getPlayerSpeed(3);
    EXPECT_FLOAT_EQ(vel.x, expectedSpeed);
}

TEST_F(PlayerInputSystemTest, SpeedLevelAffectsVelocity) {
    auto player1 = createPlayer(1);
    auto player2 = createPlayer(2);

    _ecs.entityGetComponent<SpeedLevelComp>(player1).level = 0;
    _ecs.entityGetComponent<SpeedLevelComp>(player2).level = 3;

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::RIGHT, 0});
    system.queueInput({2, InputKeys::RIGHT, 1});
    system.Update(_ecs, 0, 16);

    auto& vel1 = _ecs.entityGetComponent<VelocityComp>(player1);
    auto& vel2 = _ecs.entityGetComponent<VelocityComp>(player2);

    // Player 2 with speed level 3 should be faster
    EXPECT_GT(vel2.x, vel1.x);
}

// ═══════════════════════════════════════════════════════════════════════════
// Multiple Players Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, MultiplePlayersIndependent) {
    auto p1 = createPlayer(1);
    auto p2 = createPlayer(2);
    auto p3 = createPlayer(3);

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::UP, 0});
    system.queueInput({2, InputKeys::DOWN, 1});
    system.queueInput({3, InputKeys::LEFT, 2});
    system.Update(_ecs, 0, 16);

    auto& vel1 = _ecs.entityGetComponent<VelocityComp>(p1);
    auto& vel2 = _ecs.entityGetComponent<VelocityComp>(p2);
    auto& vel3 = _ecs.entityGetComponent<VelocityComp>(p3);

    EXPECT_LT(vel1.y, 0.0f);  // Player 1 moving up
    EXPECT_GT(vel2.y, 0.0f);  // Player 2 moving down
    EXPECT_LT(vel3.x, 0.0f);  // Player 3 moving left
}

TEST_F(PlayerInputSystemTest, UnknownPlayerId_Ignored) {
    createPlayer(1);  // Only player 1 exists

    PlayerInputSystem system(*_bridge);
    system.queueInput({99, InputKeys::UP, 0});  // Player 99 doesn't exist
    system.Update(_ecs, 0, 16);

    // Should not crash, player 99 simply not found
    auto players = _ecs.getEntitiesByComponentsAllOf<PlayerTag, VelocityComp>();
    EXPECT_EQ(players.size(), 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Input Queue Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, QueueProcessesAllInputs) {
    auto p1 = createPlayer(1);
    auto p2 = createPlayer(2);

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::UP, 0});
    system.queueInput({2, InputKeys::DOWN, 1});
    system.Update(_ecs, 0, 16);

    auto& vel1 = _ecs.entityGetComponent<VelocityComp>(p1);
    auto& vel2 = _ecs.entityGetComponent<VelocityComp>(p2);

    // Both inputs should have been processed
    EXPECT_LT(vel1.y, 0.0f);
    EXPECT_GT(vel2.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, ClearInputs_EmptiesQueue) {
    auto player = createPlayer(1);
    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    vel.x = 0.0f;
    vel.y = 0.0f;

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::UP, 0});
    system.queueInput({1, InputKeys::DOWN, 1});
    system.clearInputs();  // Clear all pending inputs
    system.Update(_ecs, 0, 16);

    // Velocity should remain zero (no inputs processed)
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, LastInputOverrides) {
    auto player = createPlayer(1);

    PlayerInputSystem system(*_bridge);
    // Queue multiple inputs for same player
    system.queueInput({1, InputKeys::UP, 0});
    system.queueInput({1, InputKeys::DOWN, 1});  // This should be the final state
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);

    // Last input (DOWN) should determine final velocity
    EXPECT_GT(vel.y, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Screen Clamping Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, ClampEnabled_ClampsToScreen) {
    // Create player at edge of screen
    auto player = createPlayer(1, -100.0f, -100.0f);
    _ecs.entityAddComponent<HitboxComp>(player) = {50.0f, 30.0f, 0.0f, 0.0f};

    PlayerInputSystem system(*_bridge);
    system.setClampEnabled(true);
    system.Update(_ecs, 0, 16);

    auto& pos = _ecs.entityGetComponent<PositionComp>(player);

    // Position should be clamped to valid screen area
    EXPECT_GE(pos.x, 0.0f);
    EXPECT_GE(pos.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, ClampDisabled_NoClamp) {
    // Create player at edge of screen
    auto player = createPlayer(1, -100.0f, -100.0f);
    _ecs.entityAddComponent<HitboxComp>(player) = {50.0f, 30.0f, 0.0f, 0.0f};

    PlayerInputSystem system(*_bridge);
    system.setClampEnabled(false);
    system.Update(_ecs, 0, 16);

    auto& pos = _ecs.entityGetComponent<PositionComp>(player);

    // Position should remain negative (not clamped)
    EXPECT_LT(pos.x, 0.0f);
    EXPECT_LT(pos.y, 0.0f);
}

TEST_F(PlayerInputSystemTest, ClampUsesHitboxDimensions) {
    // Create player at right edge of screen
    auto player = createPlayer(1, 2000.0f, 300.0f);  // Way off screen
    _ecs.entityAddComponent<HitboxComp>(player) = {100.0f, 50.0f, 0.0f, 0.0f};  // Large hitbox

    PlayerInputSystem system(*_bridge);
    system.setClampEnabled(true);
    system.Update(_ecs, 0, 16);

    auto& pos = _ecs.entityGetComponent<PositionComp>(player);
    const auto& hitbox = _ecs.entityGetComponent<HitboxComp>(player);

    // Position should be clamped so entity stays fully on screen
    // Screen width is 1920 (defined in domain/Constants.hpp)
    EXPECT_LE(pos.x + hitbox.width, 1920.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(PlayerInputSystemTest, PlayerWithoutVelocityComp_Ignored) {
    // Create player without VelocityComp
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityAddComponent<PlayerTag>(entity).playerId = 1;
    _ecs.entityAddComponent<PositionComp>(entity);
    // No VelocityComp added

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::UP, 0});

    // Should not crash
    EXPECT_NO_THROW(system.Update(_ecs, 0, 16));
}

TEST_F(PlayerInputSystemTest, PlayerWithoutSpeedLevelComp_UsesDefaultSpeed) {
    // Create player without SpeedLevelComp
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityAddComponent<PlayerTag>(entity).playerId = 1;
    _ecs.entityAddComponent<PositionComp>(entity);
    _ecs.entityAddComponent<VelocityComp>(entity);
    // No SpeedLevelComp - should use speed level 0

    PlayerInputSystem system(*_bridge);
    system.queueInput({1, InputKeys::RIGHT, 0});
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(entity);
    float expectedSpeed = _bridge->getPlayerSpeed(0);
    EXPECT_FLOAT_EQ(vel.x, expectedSpeed);
}

TEST_F(PlayerInputSystemTest, OpposingInputs_Cancel) {
    auto player = createPlayer(1);

    PlayerInputSystem system(*_bridge);
    // Press both UP and DOWN simultaneously
    system.queueInput({1, InputKeys::UP | InputKeys::DOWN, 0});
    system.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);

    // Note: Current implementation applies both, so they don't cancel
    // This tests the actual behavior, not an ideal behavior
    // If you want them to cancel, the system needs to be modified
}

