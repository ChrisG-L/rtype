/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour EnemyBehavior Domain Service
*/

#include <gtest/gtest.h>
#include "domain/services/EnemyBehavior.hpp"
#include "domain/Constants.hpp"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace domain::services;
using namespace domain::constants;

/**
 * @brief Suite de tests pour le Domain Service EnemyBehavior
 *
 * EnemyBehavior calcule les mouvements des différents types d'ennemis :
 * Basic, Tracker, Zigzag, Fast, Bomber, POWArmor
 */
class EnemyBehaviorTest : public ::testing::Test {
protected:
    EnemyBehavior _behavior;

    void SetUp() override {}
    void TearDown() override {}

    EnemyMovementInput createDefaultInput(uint8_t enemyType) {
        EnemyMovementInput input;
        input.enemyType = enemyType;
        input.currentX = 500.0f;
        input.currentY = 300.0f;
        input.baseY = 300.0f;
        input.aliveTime = 0.0f;
        input.phaseOffset = 0.0f;
        input.deltaTime = 0.016f;  // ~60 FPS
        input.zigzag = {0.0f, false};
        input.targetY = 400.0f;
        return input;
    }
};

// ============================================================================
// Tests Horizontal Movement
// ============================================================================

TEST_F(EnemyBehaviorTest, GetHorizontalMovement_AllNegative) {
    // All enemies move left (negative X)
    for (uint8_t type = 0; type <= 5; ++type) {
        float movement = _behavior.getHorizontalMovement(type, 0.016f);
        EXPECT_LT(movement, 0.0f) << "Type " << static_cast<int>(type);
    }
}

TEST_F(EnemyBehaviorTest, GetHorizontalMovement_Fast_IsFastest) {
    float basic = std::abs(_behavior.getHorizontalMovement(0, 0.1f));
    float fast = std::abs(_behavior.getHorizontalMovement(3, 0.1f));

    EXPECT_GT(fast, basic);
}

TEST_F(EnemyBehaviorTest, GetHorizontalMovement_Bomber_IsSlowest) {
    float bomber = std::abs(_behavior.getHorizontalMovement(4, 0.1f));
    float basic = std::abs(_behavior.getHorizontalMovement(0, 0.1f));

    EXPECT_LT(bomber, basic);
}

// ============================================================================
// Tests Basic Movement (Type 0)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_Basic_XDecreases) {
    auto input = createDefaultInput(0);

    auto output = _behavior.calculateMovement(input);

    EXPECT_LT(output.newX, input.currentX);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Basic_Sinusoidal) {
    auto input = createDefaultInput(0);
    input.aliveTime = 0.0f;

    auto output1 = _behavior.calculateMovement(input);

    // At time 0 with phaseOffset 0, sin(0) = 0, so Y should be at baseY
    EXPECT_NEAR(output1.newY, input.baseY, 1.0f);

    // At time that gives sin = 1 (pi/2 / frequency)
    input.aliveTime = (M_PI / 2.0f) / enemy::FREQUENCY;
    auto output2 = _behavior.calculateMovement(input);

    // Y should be at baseY + amplitude
    EXPECT_NEAR(output2.newY, input.baseY + enemy::AMPLITUDE, 1.0f);
}

// ============================================================================
// Tests Tracker Movement (Type 1)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_Tracker_MovesTowardsTarget) {
    auto input = createDefaultInput(1);
    input.currentY = 200.0f;
    input.targetY = 400.0f;  // Target is below

    auto output = _behavior.calculateMovement(input);

    // Should move towards target (Y increases)
    EXPECT_GT(output.newY, input.currentY);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Tracker_MovesAwayFromTarget) {
    auto input = createDefaultInput(1);
    input.currentY = 400.0f;
    input.targetY = 200.0f;  // Target is above

    auto output = _behavior.calculateMovement(input);

    // Should move towards target (Y decreases)
    EXPECT_LT(output.newY, input.currentY);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Tracker_ReachesTarget) {
    auto input = createDefaultInput(1);
    input.currentY = 399.0f;
    input.targetY = 400.0f;  // Very close
    input.deltaTime = 1.0f;   // Large time step

    auto output = _behavior.calculateMovement(input);

    // Should reach exactly target when close enough
    EXPECT_FLOAT_EQ(output.newY, input.targetY);
}

// ============================================================================
// Tests Zigzag Movement (Type 2)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_Zigzag_StateUpdates) {
    auto input = createDefaultInput(2);
    input.zigzag = {0.0f, false};

    auto output = _behavior.calculateMovement(input);

    // Timer should have increased
    EXPECT_GT(output.zigzag.timer, input.zigzag.timer);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Zigzag_FlipsDirection) {
    auto input = createDefaultInput(2);
    input.zigzag = {enemy::ZIGZAG_INTERVAL - 0.01f, false};
    input.deltaTime = 0.02f;  // Pushes timer past interval

    auto output = _behavior.calculateMovement(input);

    // Direction should have flipped
    EXPECT_TRUE(output.zigzag.goingUp);
    EXPECT_NEAR(output.zigzag.timer, 0.0f, 0.02f);  // Timer reset
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Zigzag_MovesUp) {
    auto input = createDefaultInput(2);
    input.zigzag = {0.0f, true};  // Going up

    auto output = _behavior.calculateMovement(input);

    // Y should decrease (going up in screen coords)
    EXPECT_LT(output.newY, input.currentY);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Zigzag_MovesDown) {
    auto input = createDefaultInput(2);
    input.zigzag = {0.0f, false};  // Going down

    auto output = _behavior.calculateMovement(input);

    // Y should increase (going down in screen coords)
    EXPECT_GT(output.newY, input.currentY);
}

// ============================================================================
// Tests Fast Movement (Type 3)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_Fast_SmallerAmplitude) {
    // Compare amplitude with Basic
    float basicAmp = _behavior.getMovementAmplitude(0);
    float fastAmp = _behavior.getMovementAmplitude(3);

    EXPECT_LT(fastAmp, basicAmp);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Fast_HigherFrequency) {
    float basicFreq = _behavior.getMovementFrequency(0);
    float fastFreq = _behavior.getMovementFrequency(3);

    EXPECT_GT(fastFreq, basicFreq);
}

// ============================================================================
// Tests Bomber Movement (Type 4)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_Bomber_SmallerAmplitude) {
    float basicAmp = _behavior.getMovementAmplitude(0);
    float bomberAmp = _behavior.getMovementAmplitude(4);

    EXPECT_LT(bomberAmp, basicAmp);
}

TEST_F(EnemyBehaviorTest, CalculateMovement_Bomber_BaseYDriftsDown) {
    auto input = createDefaultInput(4);

    auto output = _behavior.calculateMovement(input);

    // Bomber's baseY should increase (drift downward)
    EXPECT_GT(output.newBaseY, input.baseY);
}

// ============================================================================
// Tests POWArmor Movement (Type 5)
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_POWArmor_SameAsBasic) {
    auto inputBasic = createDefaultInput(0);
    auto inputPOW = createDefaultInput(5);

    auto outputBasic = _behavior.calculateMovement(inputBasic);
    auto outputPOW = _behavior.calculateMovement(inputPOW);

    // Y movement should be identical (same pattern)
    EXPECT_FLOAT_EQ(outputBasic.newY, outputPOW.newY);
}

// ============================================================================
// Tests Shooting
// ============================================================================

TEST_F(EnemyBehaviorTest, CanShoot_CooldownZero_True) {
    EXPECT_TRUE(_behavior.canShoot(0.0f));
}

TEST_F(EnemyBehaviorTest, CanShoot_CooldownNegative_True) {
    EXPECT_TRUE(_behavior.canShoot(-0.5f));
}

TEST_F(EnemyBehaviorTest, CanShoot_CooldownPositive_False) {
    EXPECT_FALSE(_behavior.canShoot(0.5f));
}

// ============================================================================
// Tests Zigzag Helpers
// ============================================================================

TEST_F(EnemyBehaviorTest, ShouldZigzagFlip_BelowInterval_False) {
    EXPECT_FALSE(_behavior.shouldZigzagFlip(enemy::ZIGZAG_INTERVAL - 0.1f));
}

TEST_F(EnemyBehaviorTest, ShouldZigzagFlip_AtInterval_True) {
    EXPECT_TRUE(_behavior.shouldZigzagFlip(enemy::ZIGZAG_INTERVAL));
}

TEST_F(EnemyBehaviorTest, ShouldZigzagFlip_AboveInterval_True) {
    EXPECT_TRUE(_behavior.shouldZigzagFlip(enemy::ZIGZAG_INTERVAL + 0.1f));
}

TEST_F(EnemyBehaviorTest, GetZigzagInterval) {
    EXPECT_FLOAT_EQ(_behavior.getZigzagInterval(), enemy::ZIGZAG_INTERVAL);
}

TEST_F(EnemyBehaviorTest, GetZigzagSpeed) {
    EXPECT_FLOAT_EQ(_behavior.getZigzagSpeed(), enemy::ZIGZAG_SPEED_Y);
}

TEST_F(EnemyBehaviorTest, GetTrackerSpeed) {
    EXPECT_FLOAT_EQ(_behavior.getTrackerSpeed(), enemy::TRACKER_SPEED_Y);
}

// ============================================================================
// Tests Y Clamping
// ============================================================================

TEST_F(EnemyBehaviorTest, CalculateMovement_ClampedToScreen) {
    auto input = createDefaultInput(1);
    input.currentY = -100.0f;  // Above screen
    input.targetY = -200.0f;   // Target even higher

    auto output = _behavior.calculateMovement(input);

    // Should be clamped to screen bounds
    EXPECT_GE(output.newY, 0.0f);
    EXPECT_LE(output.newY, world::SCREEN_HEIGHT - enemy::HEIGHT);
}
