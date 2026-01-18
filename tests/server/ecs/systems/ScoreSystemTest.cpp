/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ScoreSystem Tests - Verifies score calculation and combo decay
*/

#include <gtest/gtest.h>
#include <memory>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "systems/ScoreSystem.hpp"
#include "components/PlayerTag.hpp"
#include "components/ScoreComp.hpp"

using namespace infrastructure::ecs;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class ScoreSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _bridge = std::make_unique<bridge::DomainBridge>(
            _gameRule, _collisionRule, _enemyBehavior
        );
        _ecs.registerComponent<PlayerTag>();
        _ecs.registerComponent<ScoreComp>();
    }

    ECS::EntityID createPlayer(uint8_t playerId) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
        _ecs.entityAddComponent<PlayerTag>(entity).playerId = playerId;
        _ecs.entityAddComponent<ScoreComp>(entity);
        return entity;
    }

    ECS::ECS _ecs;
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
    std::unique_ptr<bridge::DomainBridge> _bridge;
};

// ═══════════════════════════════════════════════════════════════════════════
// Basic Kill Score Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, Kill_AddsPoints) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});  // 100 base points
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_GT(score.total, 0);
}

TEST_F(ScoreSystemTest, Kill_IncrementsKillCount) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_EQ(score.kills, 1);
}

TEST_F(ScoreSystemTest, MultipleKills_AccumulateScore) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.queueKill({1, 150, 1});
    system.queueKill({1, 200, 2});
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_EQ(score.kills, 3);
    EXPECT_GT(score.total, 400);  // Base sum is 450, but combo adds more
}

// ═══════════════════════════════════════════════════════════════════════════
// Combo Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, Kill_IncrementsCombo) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_GT(score.comboMultiplier, 1.0f);
}

TEST_F(ScoreSystemTest, ConsecutiveKills_BuildCombo) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    // First kill
    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);
    float comboAfter1 = _ecs.entityGetComponent<ScoreComp>(player).comboMultiplier;

    // Second kill (immediate)
    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);
    float comboAfter2 = _ecs.entityGetComponent<ScoreComp>(player).comboMultiplier;

    EXPECT_GT(comboAfter2, comboAfter1);
}

TEST_F(ScoreSystemTest, Kill_ResetsComboTimer) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboTimer = 5.0f;  // High timer

    ScoreSystem system(*_bridge);
    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);

    EXPECT_FLOAT_EQ(score.comboTimer, 0.0f);
}

TEST_F(ScoreSystemTest, Kill_TracksMaxCombo) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    // Build up combo
    for (int i = 0; i < 10; i++) {
        system.queueKill({1, 100, 0});
    }
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_GT(score.maxCombo, 1.0f);
    EXPECT_GE(score.maxCombo, score.comboMultiplier);
}

// ═══════════════════════════════════════════════════════════════════════════
// Combo Decay Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, ComboDecay_AfterGraceTime) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboMultiplier = 2.0f;
    score.comboTimer = 0.0f;

    ScoreSystem system(*_bridge);

    // Simulate time passing past grace time
    float graceTime = _bridge->getComboGraceTime();
    uint32_t msToPass = static_cast<uint32_t>((graceTime + 1.0f) * 1000);

    system.Update(_ecs, 0, msToPass);

    EXPECT_LT(score.comboMultiplier, 2.0f);
}

TEST_F(ScoreSystemTest, ComboDecay_MinimumIsOne) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboMultiplier = 1.1f;
    score.comboTimer = 10.0f;  // Well past grace time

    ScoreSystem system(*_bridge);
    system.Update(_ecs, 0, 5000);  // 5 seconds

    EXPECT_GE(score.comboMultiplier, 1.0f);
}

TEST_F(ScoreSystemTest, ComboDecay_ComboTimerIncrements) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboTimer = 0.0f;

    ScoreSystem system(*_bridge);
    system.Update(_ecs, 0, 1000);  // 1 second

    EXPECT_NEAR(score.comboTimer, 1.0f, 0.01f);
}

TEST_F(ScoreSystemTest, ComboDecay_NoDecayWithinGraceTime) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboMultiplier = 2.0f;
    score.comboTimer = 0.0f;

    ScoreSystem system(*_bridge);

    // Only advance a small amount (less than grace time)
    system.Update(_ecs, 0, 100);  // 100ms

    // Combo should not have decayed yet
    EXPECT_FLOAT_EQ(score.comboMultiplier, 2.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Bonus Points Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, BonusPoints_NoComboMultiplier) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    score.comboMultiplier = 2.0f;
    score.total = 0;

    ScoreSystem system(*_bridge);
    system.addBonusPoints(1, 100);
    system.Update(_ecs, 0, 16);

    // Bonus points should be added without combo multiplier
    EXPECT_EQ(score.total, 100);  // Exactly 100, not 200
}

TEST_F(ScoreSystemTest, BonusPoints_DoesNotIncrementKillCount) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    system.addBonusPoints(1, 100);
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_EQ(score.kills, 0);  // No kill was recorded
}

TEST_F(ScoreSystemTest, BonusPoints_DoesNotAffectCombo) {
    auto player = createPlayer(1);
    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    float initialCombo = score.comboMultiplier;

    ScoreSystem system(*_bridge);
    system.addBonusPoints(1, 100);
    system.Update(_ecs, 0, 16);

    EXPECT_FLOAT_EQ(score.comboMultiplier, initialCombo);
}

// ═══════════════════════════════════════════════════════════════════════════
// Score Change Events Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, Kill_EmitsScoreChangedEvent) {
    createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);

    auto events = system.getScoreChanges();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].playerId, 1);
    EXPECT_GT(events[0].newTotal, 0);
    EXPECT_GT(events[0].pointsAdded, 0);
}

TEST_F(ScoreSystemTest, GetScoreChanges_ClearsAfterRetrieval) {
    createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.Update(_ecs, 0, 16);

    auto events1 = system.getScoreChanges();
    EXPECT_EQ(events1.size(), 1);

    auto events2 = system.getScoreChanges();
    EXPECT_EQ(events2.size(), 0);  // Cleared
}

TEST_F(ScoreSystemTest, MultipleKills_MultipleEvents) {
    createPlayer(1);
    createPlayer(2);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.queueKill({2, 150, 1});
    system.Update(_ecs, 0, 16);

    auto events = system.getScoreChanges();
    EXPECT_EQ(events.size(), 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Multiple Players Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, MultiplePlayersIndependent) {
    auto p1 = createPlayer(1);
    auto p2 = createPlayer(2);

    ScoreSystem system(*_bridge);
    system.queueKill({1, 100, 0});
    system.queueKill({1, 100, 0});
    system.queueKill({2, 50, 1});
    system.Update(_ecs, 0, 16);

    const auto& score1 = _ecs.entityGetComponent<ScoreComp>(p1);
    const auto& score2 = _ecs.entityGetComponent<ScoreComp>(p2);

    EXPECT_EQ(score1.kills, 2);
    EXPECT_EQ(score2.kills, 1);
    EXPECT_GT(score1.total, score2.total);
}

TEST_F(ScoreSystemTest, ComboDecay_IndependentPerPlayer) {
    auto p1 = createPlayer(1);
    auto p2 = createPlayer(2);

    auto& score1 = _ecs.entityGetComponent<ScoreComp>(p1);
    auto& score2 = _ecs.entityGetComponent<ScoreComp>(p2);

    score1.comboMultiplier = 2.0f;
    score1.comboTimer = 10.0f;  // Past grace time

    score2.comboMultiplier = 2.0f;
    score2.comboTimer = 0.0f;   // Within grace time

    ScoreSystem system(*_bridge);
    system.Update(_ecs, 0, 1000);

    // Player 1's combo should decay
    EXPECT_LT(score1.comboMultiplier, 2.0f);

    // Player 2's combo should not decay yet
    EXPECT_FLOAT_EQ(score2.comboMultiplier, 2.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(ScoreSystemTest, UnknownPlayer_NoEffect) {
    createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({99, 100, 0});  // Player 99 doesn't exist
    EXPECT_NO_THROW(system.Update(_ecs, 0, 16));

    auto events = system.getScoreChanges();
    EXPECT_EQ(events.size(), 0);  // No event generated
}

TEST_F(ScoreSystemTest, PlayerWithoutScoreComp_NoEffect) {
    auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    _ecs.entityAddComponent<PlayerTag>(entity).playerId = 1;
    // No ScoreComp

    ScoreSystem system(*_bridge);
    system.queueKill({1, 100, 0});
    EXPECT_NO_THROW(system.Update(_ecs, 0, 16));
}

TEST_F(ScoreSystemTest, ClearKills_EmptiesQueue) {
    createPlayer(1);
    ScoreSystem system(*_bridge);

    system.queueKill({1, 100, 0});
    system.queueKill({1, 200, 1});
    system.clearKills();
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(_ecs.getEntitiesByComponentsAllOf<ScoreComp>()[0]);
    EXPECT_EQ(score.total, 0);
    EXPECT_EQ(score.kills, 0);
}

TEST_F(ScoreSystemTest, HighScoreAccumulation) {
    auto player = createPlayer(1);
    ScoreSystem system(*_bridge);

    // Simulate many kills
    for (int i = 0; i < 100; i++) {
        system.queueKill({1, 100, 0});
    }
    system.Update(_ecs, 0, 16);

    const auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_EQ(score.kills, 100);
    EXPECT_GT(score.total, 10000);  // At least base points
}

