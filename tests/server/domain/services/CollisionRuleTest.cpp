/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour CollisionRule Domain Service
*/

#include <gtest/gtest.h>
#include "domain/services/CollisionRule.hpp"
#include "domain/Constants.hpp"

using namespace domain::services;
using namespace domain::constants;

/**
 * @brief Suite de tests pour le Domain Service CollisionRule
 *
 * CollisionRule fournit la détection de collision AABB
 * et les vérifications de limites d'écran.
 */
class CollisionRuleTest : public ::testing::Test {
protected:
    CollisionRule _rule;

    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests AABB Collision
// ============================================================================

TEST_F(CollisionRuleTest, CheckAABB_Overlapping_True) {
    HitboxData a{100.0f, 100.0f, 50.0f, 50.0f};
    HitboxData b{120.0f, 120.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.checkAABB(a, b));
}

TEST_F(CollisionRuleTest, CheckAABB_NonOverlapping_False) {
    HitboxData a{100.0f, 100.0f, 50.0f, 50.0f};
    HitboxData b{200.0f, 200.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.checkAABB(a, b));
}

TEST_F(CollisionRuleTest, CheckAABB_TouchingEdge_True) {
    HitboxData a{100.0f, 100.0f, 50.0f, 50.0f};
    HitboxData b{149.0f, 100.0f, 50.0f, 50.0f};  // 1 pixel overlap

    EXPECT_TRUE(_rule.checkAABB(a, b));
}

TEST_F(CollisionRuleTest, CheckAABB_ExactlyAdjacent_False) {
    HitboxData a{100.0f, 100.0f, 50.0f, 50.0f};
    HitboxData b{150.0f, 100.0f, 50.0f, 50.0f};  // No overlap

    EXPECT_FALSE(_rule.checkAABB(a, b));
}

TEST_F(CollisionRuleTest, CheckAABB_OneInsideOther_True) {
    HitboxData large{0.0f, 0.0f, 200.0f, 200.0f};
    HitboxData small{50.0f, 50.0f, 20.0f, 20.0f};

    EXPECT_TRUE(_rule.checkAABB(large, small));
    EXPECT_TRUE(_rule.checkAABB(small, large));
}

TEST_F(CollisionRuleTest, CheckAABB_SamePosition_True) {
    HitboxData a{100.0f, 100.0f, 50.0f, 50.0f};
    HitboxData b{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.checkAABB(a, b));
}

// ============================================================================
// Tests Out of Bounds
// ============================================================================

TEST_F(CollisionRuleTest, IsOutOfBounds_FullyInside_False) {
    HitboxData hitbox{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.isOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsOutOfBounds_FullyLeft_True) {
    HitboxData hitbox{-100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsOutOfBounds_FullyRight_True) {
    HitboxData hitbox{world::SCREEN_WIDTH + 10.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsOutOfBounds_FullyTop_True) {
    HitboxData hitbox{100.0f, -100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsOutOfBounds_FullyBottom_True) {
    HitboxData hitbox{100.0f, world::SCREEN_HEIGHT + 10.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsOutOfBounds_PartiallyOutside_False) {
    // Only partially outside - still has some part on screen
    HitboxData hitbox{-25.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.isOutOfBounds(hitbox));
}

// ============================================================================
// Tests Partially Out of Bounds
// ============================================================================

TEST_F(CollisionRuleTest, IsPartiallyOutOfBounds_FullyInside_False) {
    HitboxData hitbox{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.isPartiallyOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsPartiallyOutOfBounds_PartiallyLeft_True) {
    HitboxData hitbox{-25.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isPartiallyOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsPartiallyOutOfBounds_PartiallyRight_True) {
    HitboxData hitbox{world::SCREEN_WIDTH - 25.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.isPartiallyOutOfBounds(hitbox));
}

TEST_F(CollisionRuleTest, IsPartiallyOutOfBounds_AtEdge_False) {
    // Exactly at edge, but not beyond
    HitboxData hitbox{0.0f, 0.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.isPartiallyOutOfBounds(hitbox));
}

// ============================================================================
// Tests Contains Point
// ============================================================================

TEST_F(CollisionRuleTest, ContainsPoint_Inside_True) {
    HitboxData hitbox{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.containsPoint(hitbox, 125.0f, 125.0f));
}

TEST_F(CollisionRuleTest, ContainsPoint_Outside_False) {
    HitboxData hitbox{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_FALSE(_rule.containsPoint(hitbox, 200.0f, 200.0f));
}

TEST_F(CollisionRuleTest, ContainsPoint_OnEdge_True) {
    HitboxData hitbox{100.0f, 100.0f, 50.0f, 50.0f};

    EXPECT_TRUE(_rule.containsPoint(hitbox, 100.0f, 100.0f));  // Top-left corner
    EXPECT_TRUE(_rule.containsPoint(hitbox, 150.0f, 150.0f));  // Bottom-right corner
}

// ============================================================================
// Tests Clamp to Screen
// ============================================================================

TEST_F(CollisionRuleTest, ClampToScreen_AlreadyInside_NoChange) {
    float x = 100.0f, y = 100.0f;
    _rule.clampToScreen(x, y, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(x, 100.0f);
    EXPECT_FLOAT_EQ(y, 100.0f);
}

TEST_F(CollisionRuleTest, ClampToScreen_TooLeft_ClampedToZero) {
    float x = -50.0f, y = 100.0f;
    _rule.clampToScreen(x, y, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(x, 0.0f);
    EXPECT_FLOAT_EQ(y, 100.0f);
}

TEST_F(CollisionRuleTest, ClampToScreen_TooRight_ClampedToMax) {
    float x = world::SCREEN_WIDTH, y = 100.0f;
    _rule.clampToScreen(x, y, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(x, world::SCREEN_WIDTH - 50.0f);
    EXPECT_FLOAT_EQ(y, 100.0f);
}

TEST_F(CollisionRuleTest, ClampToScreen_TooTop_ClampedToZero) {
    float x = 100.0f, y = -50.0f;
    _rule.clampToScreen(x, y, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(x, 100.0f);
    EXPECT_FLOAT_EQ(y, 0.0f);
}

TEST_F(CollisionRuleTest, ClampToScreen_TooBottom_ClampedToMax) {
    float x = 100.0f, y = world::SCREEN_HEIGHT;
    _rule.clampToScreen(x, y, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(x, 100.0f);
    EXPECT_FLOAT_EQ(y, world::SCREEN_HEIGHT - 50.0f);
}

// ============================================================================
// Tests Screen Constants
// ============================================================================

TEST_F(CollisionRuleTest, GetScreenWidth) {
    EXPECT_FLOAT_EQ(_rule.getScreenWidth(), world::SCREEN_WIDTH);
}

TEST_F(CollisionRuleTest, GetScreenHeight) {
    EXPECT_FLOAT_EQ(_rule.getScreenHeight(), world::SCREEN_HEIGHT);
}
