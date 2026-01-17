/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DomainBridge Tests - Verifies correct delegation to Domain services
*/

#include <gtest/gtest.h>
#include <cmath>
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"

using namespace infrastructure::ecs::bridge;
using namespace infrastructure::ecs::components;
using namespace domain::services;

// ═══════════════════════════════════════════════════════════════════════════
// DomainBridge Setup Helper
// ═══════════════════════════════════════════════════════════════════════════

class DomainBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        _bridge = std::make_unique<DomainBridge>(
            _gameRule,
            _collisionRule,
            _enemyBehavior
        );
    }

    GameRule _gameRule;
    CollisionRule _collisionRule;
    EnemyBehavior _enemyBehavior;
    std::unique_ptr<DomainBridge> _bridge;
};

// ═══════════════════════════════════════════════════════════════════════════
// Damage & Health Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, ApplyDamageNormal) {
    HealthComp health{100, 100, false};
    auto result = _bridge->applyDamage(health, 30);

    EXPECT_EQ(result.actualDamage, 30);
    EXPECT_FALSE(result.died);
    EXPECT_EQ(health.current, 70);
}

TEST_F(DomainBridgeTest, ApplyDamageLethal) {
    HealthComp health{20, 100, false};
    auto result = _bridge->applyDamage(health, 30);

    EXPECT_EQ(result.actualDamage, 30);
    EXPECT_TRUE(result.died);
    EXPECT_EQ(health.current, 0);
}

TEST_F(DomainBridgeTest, ApplyDamageExactKill) {
    HealthComp health{50, 100, false};
    auto result = _bridge->applyDamage(health, 50);

    EXPECT_EQ(result.actualDamage, 50);
    EXPECT_TRUE(result.died);
    EXPECT_EQ(health.current, 0);
}

TEST_F(DomainBridgeTest, ApplyDamageInvulnerable) {
    HealthComp health{100, 100, true};  // GodMode
    auto result = _bridge->applyDamage(health, 50);

    EXPECT_EQ(result.actualDamage, 0);
    EXPECT_FALSE(result.died);
    EXPECT_EQ(health.current, 100);  // No damage taken
}

TEST_F(DomainBridgeTest, WouldDie) {
    EXPECT_TRUE(_bridge->wouldDie(30, 50));
    EXPECT_FALSE(_bridge->wouldDie(100, 50));
    EXPECT_TRUE(_bridge->wouldDie(50, 50));  // Exact kill
}

// ═══════════════════════════════════════════════════════════════════════════
// Score & Combo Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, CalculateKillScoreNoCombo) {
    uint32_t score = _bridge->calculateKillScore(100, 1.0f);
    EXPECT_EQ(score, 100);
}

TEST_F(DomainBridgeTest, CalculateKillScoreWithCombo) {
    uint32_t score = _bridge->calculateKillScore(100, 2.0f);
    EXPECT_EQ(score, 200);
}

TEST_F(DomainBridgeTest, CalculateKillScoreMaxCombo) {
    uint32_t score = _bridge->calculateKillScore(100, 3.0f);
    EXPECT_EQ(score, 300);
}

TEST_F(DomainBridgeTest, GetEnemyPointsBasic) {
    uint16_t points = _bridge->getEnemyPoints(0);  // Basic
    EXPECT_EQ(points, 100);
}

TEST_F(DomainBridgeTest, GetEnemyPointsTracker) {
    uint16_t points = _bridge->getEnemyPoints(1);  // Tracker
    EXPECT_EQ(points, 150);
}

TEST_F(DomainBridgeTest, GetEnemyPointsBomber) {
    uint16_t points = _bridge->getEnemyPoints(4);  // Bomber
    EXPECT_EQ(points, 250);
}

TEST_F(DomainBridgeTest, IncrementCombo) {
    float newCombo = _bridge->incrementCombo(1.0f);
    EXPECT_NEAR(newCombo, 1.1f, 0.001f);

    newCombo = _bridge->incrementCombo(2.9f);
    EXPECT_NEAR(newCombo, 3.0f, 0.001f);  // Capped at max
}

TEST_F(DomainBridgeTest, GetComboGraceTime) {
    float graceTime = _bridge->getComboGraceTime();
    EXPECT_FLOAT_EQ(graceTime, 3.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Weapon Stats Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, GetMissileDamageStandard) {
    uint8_t damage = _bridge->getMissileDamage(0, 0);  // Standard, level 0
    EXPECT_EQ(damage, 20);
}

TEST_F(DomainBridgeTest, GetMissileDamageStandardLevel3) {
    uint8_t damage = _bridge->getMissileDamage(0, 3);  // Standard, level 3
    EXPECT_EQ(damage, 30);  // 20 * 1.5 = 30
}

TEST_F(DomainBridgeTest, GetMissileDamageMissile) {
    uint8_t damage = _bridge->getMissileDamage(3, 0);  // Homing Missile, level 0
    EXPECT_EQ(damage, 50);
}

TEST_F(DomainBridgeTest, GetMissileSpeedStandard) {
    float speed = _bridge->getMissileSpeed(0, 0);  // Standard, level 0
    EXPECT_FLOAT_EQ(speed, 600.0f);
}

TEST_F(DomainBridgeTest, GetMissileSpeedLaser) {
    float speed = _bridge->getMissileSpeed(2, 0);  // Laser, level 0
    EXPECT_FLOAT_EQ(speed, 900.0f);
}

TEST_F(DomainBridgeTest, GetWeaponCooldownStandard) {
    float cooldown = _bridge->getWeaponCooldown(0, 0);  // Standard, level 0
    EXPECT_FLOAT_EQ(cooldown, 0.3f);
}

TEST_F(DomainBridgeTest, GetWeaponCooldownLaser) {
    float cooldown = _bridge->getWeaponCooldown(2, 0);  // Laser, level 0
    EXPECT_FLOAT_EQ(cooldown, 0.18f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Player Movement Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, GetPlayerSpeedLevel0) {
    float speed = _bridge->getPlayerSpeed(0);
    EXPECT_FLOAT_EQ(speed, 200.0f);  // Base speed
}

TEST_F(DomainBridgeTest, GetPlayerSpeedLevel3) {
    float speed = _bridge->getPlayerSpeed(3);
    EXPECT_FLOAT_EQ(speed, 380.0f);  // 200 * 1.9 = 380
}

// ═══════════════════════════════════════════════════════════════════════════
// Collision Detection Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, CheckCollisionOverlap) {
    bool result = _bridge->checkCollision(
        100.0f, 100.0f, 50.0f, 50.0f,   // Entity 1
        120.0f, 120.0f, 50.0f, 50.0f    // Entity 2 (overlapping)
    );
    EXPECT_TRUE(result);
}

TEST_F(DomainBridgeTest, CheckCollisionNoOverlap) {
    bool result = _bridge->checkCollision(
        100.0f, 100.0f, 50.0f, 50.0f,   // Entity 1
        200.0f, 200.0f, 50.0f, 50.0f    // Entity 2 (not overlapping)
    );
    EXPECT_FALSE(result);
}

TEST_F(DomainBridgeTest, CheckCollisionEdgeTouch) {
    bool result = _bridge->checkCollision(
        100.0f, 100.0f, 50.0f, 50.0f,   // Entity 1: ends at (150, 150)
        150.0f, 100.0f, 50.0f, 50.0f    // Entity 2: starts at (150, 100)
    );
    // Edge touching is NOT a collision (exclusive bounds)
    EXPECT_FALSE(result);
}

TEST_F(DomainBridgeTest, IsOutOfBoundsLeft) {
    bool result = _bridge->isOutOfBounds(-100.0f, 500.0f, 50.0f, 50.0f);
    EXPECT_TRUE(result);
}

TEST_F(DomainBridgeTest, IsOutOfBoundsRight) {
    bool result = _bridge->isOutOfBounds(2000.0f, 500.0f, 50.0f, 50.0f);
    EXPECT_TRUE(result);
}

TEST_F(DomainBridgeTest, IsInBounds) {
    bool result = _bridge->isOutOfBounds(960.0f, 540.0f, 50.0f, 50.0f);
    EXPECT_FALSE(result);
}

TEST_F(DomainBridgeTest, ClampToScreen) {
    float x = -50.0f;
    float y = 1200.0f;
    _bridge->clampToScreen(x, y, 50.0f, 30.0f);

    EXPECT_GE(x, 0.0f);
    EXPECT_LE(y, 1080.0f - 30.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Enemy Behavior Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, CanEnemyShoot) {
    EXPECT_TRUE(_bridge->canEnemyShoot(0.0f));   // Cooldown ready
    EXPECT_FALSE(_bridge->canEnemyShoot(1.0f));  // Still on cooldown
}

TEST_F(DomainBridgeTest, GetEnemyShootIntervalBasic) {
    float interval = _bridge->getEnemyShootInterval(0);  // Basic
    EXPECT_FLOAT_EQ(interval, 2.5f);
}

TEST_F(DomainBridgeTest, GetEnemyShootIntervalFast) {
    float interval = _bridge->getEnemyShootInterval(3);  // Fast
    EXPECT_FLOAT_EQ(interval, 1.5f);
}

TEST_F(DomainBridgeTest, GetEnemyMovementBasic) {
    ZigzagState zigzag{0.0f, true};
    auto [dx, dy] = _bridge->getEnemyMovement(
        0,       // Basic
        540.0f,  // currentY
        540.0f,  // baseY
        1.0f,    // aliveTime
        0.0f,    // phaseOffset
        0.016f,  // deltaTime
        540.0f,  // targetY
        zigzag
    );

    // Basic enemy: moves left (negative dx)
    EXPECT_LT(dx, 0.0f);  // Moving left
    // dy can vary based on sine wave pattern - just verify it's finite
    EXPECT_TRUE(std::isfinite(dy));
}

TEST_F(DomainBridgeTest, GetEnemyMovementTracker) {
    ZigzagState zigzag{0.0f, true};
    auto [dx, dy] = _bridge->getEnemyMovement(
        1,       // Tracker
        300.0f,  // currentY
        300.0f,  // baseY
        1.0f,    // aliveTime
        0.0f,    // phaseOffset
        0.016f,  // deltaTime
        540.0f,  // targetY (player is below)
        zigzag
    );

    // Tracker enemy: should move towards targetY
    EXPECT_LT(dx, 0.0f);  // Moving left
    EXPECT_GT(dy, 0.0f);  // Moving down towards player
}

// ═══════════════════════════════════════════════════════════════════════════
// Integration Scenario Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DomainBridgeTest, KillEnemyScenario) {
    // Simulate killing an enemy and calculating score
    HealthComp enemyHealth{40, 40, false};

    // Apply lethal damage
    auto result = _bridge->applyDamage(enemyHealth, 50);
    EXPECT_TRUE(result.died);

    // Calculate score with 1.5x combo
    uint16_t basePoints = _bridge->getEnemyPoints(0);  // Basic enemy
    uint32_t score = _bridge->calculateKillScore(basePoints, 1.5f);
    EXPECT_EQ(score, 150);  // 100 * 1.5

    // Increment combo for next kill
    float newCombo = _bridge->incrementCombo(1.5f);
    EXPECT_NEAR(newCombo, 1.6f, 0.001f);
}

TEST_F(DomainBridgeTest, GodModeProtection) {
    // GodMode player takes no damage
    HealthComp playerHealth{100, 100, true};

    // Multiple damage attempts
    auto r1 = _bridge->applyDamage(playerHealth, 30);
    auto r2 = _bridge->applyDamage(playerHealth, 50);
    auto r3 = _bridge->applyDamage(playerHealth, 100);

    // All damage blocked
    EXPECT_EQ(r1.actualDamage, 0);
    EXPECT_EQ(r2.actualDamage, 0);
    EXPECT_EQ(r3.actualDamage, 0);
    EXPECT_FALSE(r1.died);
    EXPECT_FALSE(r2.died);
    EXPECT_FALSE(r3.died);

    // Health unchanged
    EXPECT_EQ(playerHealth.current, 100);

    // Note: wouldDie() is a hypothetical check that doesn't consider invulnerability
    // It answers "would this HP value die from this damage?" not "would THIS entity die?"
    EXPECT_TRUE(_bridge->wouldDie(50, 100));  // 50 HP would die from 100 damage
}
