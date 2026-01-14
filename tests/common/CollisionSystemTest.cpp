/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionSystemTest - Tests for AABB collision detection system
*/

#include <gtest/gtest.h>
#include "collision/AABB.hpp"

using namespace collision;

// ============================================================================
// Tests - AABB Construction
// ============================================================================

class AABBConstructionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBConstructionTest, BasicConstruction) {
    AABB box(10.0f, 20.0f, 100.0f, 50.0f);

    EXPECT_FLOAT_EQ(box.x, 10.0f);
    EXPECT_FLOAT_EQ(box.y, 20.0f);
    EXPECT_FLOAT_EQ(box.width, 100.0f);
    EXPECT_FLOAT_EQ(box.height, 50.0f);
}

TEST_F(AABBConstructionTest, ZeroPosition) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_FLOAT_EQ(box.x, 0.0f);
    EXPECT_FLOAT_EQ(box.y, 0.0f);
}

TEST_F(AABBConstructionTest, NegativePosition) {
    AABB box(-50.0f, -30.0f, 100.0f, 100.0f);

    EXPECT_FLOAT_EQ(box.x, -50.0f);
    EXPECT_FLOAT_EQ(box.y, -30.0f);
}

TEST_F(AABBConstructionTest, LargeValues) {
    AABB box(1920.0f, 1080.0f, 500.0f, 500.0f);

    EXPECT_FLOAT_EQ(box.x, 1920.0f);
    EXPECT_FLOAT_EQ(box.y, 1080.0f);
}

TEST_F(AABBConstructionTest, SmallDimensions) {
    AABB box(100.0f, 100.0f, 1.0f, 1.0f);

    EXPECT_FLOAT_EQ(box.width, 1.0f);
    EXPECT_FLOAT_EQ(box.height, 1.0f);
}

// ============================================================================
// Tests - Center Calculation
// ============================================================================

class AABBCenterTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBCenterTest, CenterXCalculation) {
    AABB box(100.0f, 200.0f, 50.0f, 30.0f);

    // centerX = x + width/2 = 100 + 25 = 125
    EXPECT_FLOAT_EQ(box.centerX(), 125.0f);
}

TEST_F(AABBCenterTest, CenterYCalculation) {
    AABB box(100.0f, 200.0f, 50.0f, 30.0f);

    // centerY = y + height/2 = 200 + 15 = 215
    EXPECT_FLOAT_EQ(box.centerY(), 215.0f);
}

TEST_F(AABBCenterTest, CenterOfUnitBox) {
    AABB box(0.0f, 0.0f, 2.0f, 2.0f);

    EXPECT_FLOAT_EQ(box.centerX(), 1.0f);
    EXPECT_FLOAT_EQ(box.centerY(), 1.0f);
}

TEST_F(AABBCenterTest, CenterWithNegativePosition) {
    AABB box(-50.0f, -50.0f, 100.0f, 100.0f);

    // centerX = -50 + 50 = 0
    // centerY = -50 + 50 = 0
    EXPECT_FLOAT_EQ(box.centerX(), 0.0f);
    EXPECT_FLOAT_EQ(box.centerY(), 0.0f);
}

// ============================================================================
// Tests - Set Position
// ============================================================================

class AABBSetPositionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBSetPositionTest, BasicSetPosition) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    box.setPosition(50.0f, 75.0f);

    EXPECT_FLOAT_EQ(box.x, 50.0f);
    EXPECT_FLOAT_EQ(box.y, 75.0f);
    // Dimensions should be unchanged
    EXPECT_FLOAT_EQ(box.width, 100.0f);
    EXPECT_FLOAT_EQ(box.height, 100.0f);
}

TEST_F(AABBSetPositionTest, SetToNegativePosition) {
    AABB box(100.0f, 100.0f, 50.0f, 50.0f);

    box.setPosition(-25.0f, -25.0f);

    EXPECT_FLOAT_EQ(box.x, -25.0f);
    EXPECT_FLOAT_EQ(box.y, -25.0f);
}

// ============================================================================
// Tests - Contains Point
// ============================================================================

class AABBContainsPointTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBContainsPointTest, PointInsideBox) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box.contains(50.0f, 50.0f));
}

TEST_F(AABBContainsPointTest, PointAtTopLeftCorner) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box.contains(0.0f, 0.0f));
}

TEST_F(AABBContainsPointTest, PointAtBottomRightCorner) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box.contains(100.0f, 100.0f));
}

TEST_F(AABBContainsPointTest, PointOnEdge) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box.contains(50.0f, 0.0f));    // Top edge
    EXPECT_TRUE(box.contains(50.0f, 100.0f));  // Bottom edge
    EXPECT_TRUE(box.contains(0.0f, 50.0f));    // Left edge
    EXPECT_TRUE(box.contains(100.0f, 50.0f));  // Right edge
}

TEST_F(AABBContainsPointTest, PointOutsideLeft) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_FALSE(box.contains(-1.0f, 50.0f));
}

TEST_F(AABBContainsPointTest, PointOutsideRight) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_FALSE(box.contains(101.0f, 50.0f));
}

TEST_F(AABBContainsPointTest, PointOutsideTop) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_FALSE(box.contains(50.0f, -1.0f));
}

TEST_F(AABBContainsPointTest, PointOutsideBottom) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_FALSE(box.contains(50.0f, 101.0f));
}

// ============================================================================
// Tests - Intersection Detection
// ============================================================================

class AABBIntersectionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBIntersectionTest, OverlappingBoxes) {
    AABB box1(0.0f, 0.0f, 100.0f, 100.0f);
    AABB box2(50.0f, 50.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box1.intersects(box2));
    EXPECT_TRUE(box2.intersects(box1));
}

TEST_F(AABBIntersectionTest, BoxesNotOverlapping) {
    AABB box1(0.0f, 0.0f, 50.0f, 50.0f);
    AABB box2(100.0f, 100.0f, 50.0f, 50.0f);

    EXPECT_FALSE(box1.intersects(box2));
    EXPECT_FALSE(box2.intersects(box1));
}

TEST_F(AABBIntersectionTest, BoxesTouchingEdge) {
    AABB box1(0.0f, 0.0f, 50.0f, 50.0f);
    AABB box2(50.0f, 0.0f, 50.0f, 50.0f);  // Touching on right edge

    // Boxes touching exactly on edge should NOT intersect
    // Because intersects uses strict < comparison
    EXPECT_FALSE(box1.intersects(box2));
}

TEST_F(AABBIntersectionTest, BoxFullyContained) {
    AABB outer(0.0f, 0.0f, 100.0f, 100.0f);
    AABB inner(25.0f, 25.0f, 50.0f, 50.0f);

    EXPECT_TRUE(outer.intersects(inner));
    EXPECT_TRUE(inner.intersects(outer));
}

TEST_F(AABBIntersectionTest, SameBox) {
    AABB box(0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box.intersects(box));
}

TEST_F(AABBIntersectionTest, BoxesWithPartialOverlap) {
    // Box1: 0-100, Box2: 80-180 (20 pixels overlap)
    AABB box1(0.0f, 0.0f, 100.0f, 100.0f);
    AABB box2(80.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box1.intersects(box2));
}

TEST_F(AABBIntersectionTest, BoxesSeparatedHorizontally) {
    AABB box1(0.0f, 0.0f, 50.0f, 100.0f);
    AABB box2(60.0f, 0.0f, 50.0f, 100.0f);  // 10 pixel gap

    EXPECT_FALSE(box1.intersects(box2));
}

TEST_F(AABBIntersectionTest, BoxesSeparatedVertically) {
    AABB box1(0.0f, 0.0f, 100.0f, 50.0f);
    AABB box2(0.0f, 60.0f, 100.0f, 50.0f);  // 10 pixel gap

    EXPECT_FALSE(box1.intersects(box2));
}

// ============================================================================
// Tests - Hitbox Constants
// ============================================================================

class HitboxConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HitboxConstantsTest, ShipDimensions) {
    EXPECT_FLOAT_EQ(Hitboxes::SHIP_WIDTH, 64.0f);
    EXPECT_FLOAT_EQ(Hitboxes::SHIP_HEIGHT, 30.0f);
}

TEST_F(HitboxConstantsTest, MissileDimensions) {
    EXPECT_FLOAT_EQ(Hitboxes::MISSILE_WIDTH, 16.0f);
    EXPECT_FLOAT_EQ(Hitboxes::MISSILE_HEIGHT, 8.0f);
}

TEST_F(HitboxConstantsTest, EnemyDimensions) {
    EXPECT_FLOAT_EQ(Hitboxes::ENEMY_WIDTH, 40.0f);
    EXPECT_FLOAT_EQ(Hitboxes::ENEMY_HEIGHT, 40.0f);
}

TEST_F(HitboxConstantsTest, ShipLargerThanMissile) {
    EXPECT_GT(Hitboxes::SHIP_WIDTH, Hitboxes::MISSILE_WIDTH);
    EXPECT_GT(Hitboxes::SHIP_HEIGHT, Hitboxes::MISSILE_HEIGHT);
}

TEST_F(HitboxConstantsTest, EnemyIsSquare) {
    EXPECT_FLOAT_EQ(Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);
}

// ============================================================================
// Tests - Game Scenarios
// ============================================================================

class CollisionGameScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CollisionGameScenariosTest, MissileHitsEnemy) {
    // Missile at position (500, 300)
    AABB missile(500.0f, 300.0f, Hitboxes::MISSILE_WIDTH, Hitboxes::MISSILE_HEIGHT);

    // Enemy at position (490, 290) - overlapping
    AABB enemy(490.0f, 290.0f, Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);

    EXPECT_TRUE(missile.intersects(enemy));
}

TEST_F(CollisionGameScenariosTest, MissileMissesEnemy) {
    // Missile at position (500, 300)
    AABB missile(500.0f, 300.0f, Hitboxes::MISSILE_WIDTH, Hitboxes::MISSILE_HEIGHT);

    // Enemy at position (600, 300) - no overlap
    AABB enemy(600.0f, 300.0f, Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);

    EXPECT_FALSE(missile.intersects(enemy));
}

TEST_F(CollisionGameScenariosTest, ShipCollidesWithEnemy) {
    // Ship at position (100, 400)
    AABB ship(100.0f, 400.0f, Hitboxes::SHIP_WIDTH, Hitboxes::SHIP_HEIGHT);

    // Enemy at position (150, 410) - partial overlap
    AABB enemy(150.0f, 410.0f, Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);

    EXPECT_TRUE(ship.intersects(enemy));
}

TEST_F(CollisionGameScenariosTest, ShipAvoidsEnemy) {
    // Ship at position (100, 400)
    AABB ship(100.0f, 400.0f, Hitboxes::SHIP_WIDTH, Hitboxes::SHIP_HEIGHT);

    // Enemy at position (200, 500) - clearly separated
    AABB enemy(200.0f, 500.0f, Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);

    EXPECT_FALSE(ship.intersects(enemy));
}

TEST_F(CollisionGameScenariosTest, PowerUpCollision) {
    // Power-up dimensions (24x24 from GameWorld)
    constexpr float POWERUP_WIDTH = 24.0f;
    constexpr float POWERUP_HEIGHT = 24.0f;

    // Ship at position (100, 400)
    AABB ship(100.0f, 400.0f, Hitboxes::SHIP_WIDTH, Hitboxes::SHIP_HEIGHT);

    // Power-up at position (120, 390) - overlapping
    AABB powerUp(120.0f, 390.0f, POWERUP_WIDTH, POWERUP_HEIGHT);

    EXPECT_TRUE(ship.intersects(powerUp));
}

TEST_F(CollisionGameScenariosTest, EnemyMissileHitsShip) {
    // Enemy missile at position (100, 410)
    AABB enemyMissile(100.0f, 410.0f, Hitboxes::MISSILE_WIDTH, Hitboxes::MISSILE_HEIGHT);

    // Ship at position (90, 400)
    AABB ship(90.0f, 400.0f, Hitboxes::SHIP_WIDTH, Hitboxes::SHIP_HEIGHT);

    EXPECT_TRUE(enemyMissile.intersects(ship));
}

TEST_F(CollisionGameScenariosTest, ForcePodHitsEnemy) {
    // Force Pod dimensions (32x32 from GameWorld)
    constexpr float FORCE_WIDTH = 32.0f;
    constexpr float FORCE_HEIGHT = 32.0f;

    // Force Pod at position (200, 350)
    AABB forcePod(200.0f, 350.0f, FORCE_WIDTH, FORCE_HEIGHT);

    // Enemy at position (220, 340) - overlapping
    AABB enemy(220.0f, 340.0f, Hitboxes::ENEMY_WIDTH, Hitboxes::ENEMY_HEIGHT);

    EXPECT_TRUE(forcePod.intersects(enemy));
}

// ============================================================================
// Tests - Edge Cases
// ============================================================================

class CollisionEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CollisionEdgeCasesTest, ZeroSizeBox) {
    AABB zeroBox(50.0f, 50.0f, 0.0f, 0.0f);
    AABB normalBox(0.0f, 0.0f, 100.0f, 100.0f);

    // A zero-size box at (50, 50) is essentially a point.
    // Since that point is inside normalBox (0,0 to 100,100), they DO intersect.
    // This is mathematically correct: the point exists within the area.
    EXPECT_TRUE(zeroBox.intersects(normalBox));

    // Zero-size box OUTSIDE another box should NOT intersect
    AABB zeroBoxOutside(150.0f, 150.0f, 0.0f, 0.0f);
    EXPECT_FALSE(zeroBoxOutside.intersects(normalBox));
}

TEST_F(CollisionEdgeCasesTest, VerySmallOverlap) {
    AABB box1(0.0f, 0.0f, 100.0f, 100.0f);
    AABB box2(99.9f, 0.0f, 100.0f, 100.0f);  // 0.1 pixel overlap

    EXPECT_TRUE(box1.intersects(box2));
}

TEST_F(CollisionEdgeCasesTest, NegativeCoordinates) {
    AABB box1(-100.0f, -100.0f, 50.0f, 50.0f);
    AABB box2(-75.0f, -75.0f, 50.0f, 50.0f);

    EXPECT_TRUE(box1.intersects(box2));
}

TEST_F(CollisionEdgeCasesTest, LargeCoordinates) {
    AABB box1(10000.0f, 10000.0f, 100.0f, 100.0f);
    AABB box2(10050.0f, 10050.0f, 100.0f, 100.0f);

    EXPECT_TRUE(box1.intersects(box2));
}

TEST_F(CollisionEdgeCasesTest, BoxAtScreenEdge) {
    // Box at right edge of 1920x1080 screen
    AABB box(1870.0f, 500.0f, 50.0f, 50.0f);

    // Missile coming from the right (should still detect collision)
    AABB missile(1860.0f, 510.0f, Hitboxes::MISSILE_WIDTH, Hitboxes::MISSILE_HEIGHT);

    EXPECT_TRUE(box.intersects(missile));
}

// ============================================================================
// Tests - Constexpr Verification
// ============================================================================

class AABBConstexprTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBConstexprTest, ConstexprConstruction) {
    constexpr AABB box(10.0f, 20.0f, 100.0f, 50.0f);

    EXPECT_FLOAT_EQ(box.x, 10.0f);
    EXPECT_FLOAT_EQ(box.y, 20.0f);
}

TEST_F(AABBConstexprTest, ConstexprIntersects) {
    constexpr AABB box1(0.0f, 0.0f, 100.0f, 100.0f);
    constexpr AABB box2(50.0f, 50.0f, 100.0f, 100.0f);
    constexpr bool result = box1.intersects(box2);

    EXPECT_TRUE(result);
}

TEST_F(AABBConstexprTest, ConstexprContains) {
    constexpr AABB box(0.0f, 0.0f, 100.0f, 100.0f);
    constexpr bool result = box.contains(50.0f, 50.0f);

    EXPECT_TRUE(result);
}

TEST_F(AABBConstexprTest, ConstexprCenter) {
    constexpr AABB box(0.0f, 0.0f, 100.0f, 100.0f);
    constexpr float cx = box.centerX();
    constexpr float cy = box.centerY();

    EXPECT_FLOAT_EQ(cx, 50.0f);
    EXPECT_FLOAT_EQ(cy, 50.0f);
}

// ============================================================================
// Tests - Symmetry
// ============================================================================

class AABBSymmetryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AABBSymmetryTest, IntersectionIsSymmetric) {
    AABB box1(0.0f, 0.0f, 100.0f, 100.0f);
    AABB box2(50.0f, 50.0f, 100.0f, 100.0f);

    // A intersects B should equal B intersects A
    EXPECT_EQ(box1.intersects(box2), box2.intersects(box1));
}

TEST_F(AABBSymmetryTest, NoIntersectionIsSymmetric) {
    AABB box1(0.0f, 0.0f, 50.0f, 50.0f);
    AABB box2(200.0f, 200.0f, 50.0f, 50.0f);

    EXPECT_EQ(box1.intersects(box2), box2.intersects(box1));
    EXPECT_FALSE(box1.intersects(box2));
}
