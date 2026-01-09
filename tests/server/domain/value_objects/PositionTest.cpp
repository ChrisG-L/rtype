/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour Position Value Object
*/

#include <gtest/gtest.h>
#include "domain/value_objects/Position.hpp"
#include "domain/exceptions/PositionException.hpp"

using namespace domain::value_objects;
using namespace domain::exceptions;

/**
 * @brief Suite de tests pour le Value Object Position
 *
 * Position représente une position 3D dans l'espace de jeu.
 * Contraintes : x, y, z entre -1000.0 et 1000.0 (inclus)
 */
class PositionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Construction
// ============================================================================

/**
 * @test Création avec constructeur par défaut (0, 0, 0)
 */
TEST_F(PositionTest, CreateWithDefaultValues) {
    Position pos;
    EXPECT_FLOAT_EQ(pos.getX(), 0.0f);
    EXPECT_FLOAT_EQ(pos.getY(), 0.0f);
    EXPECT_FLOAT_EQ(pos.getZ(), 0.0f);
}

/**
 * @test Création avec valeurs personnalisées valides
 */
TEST_F(PositionTest, CreateWithCustomValues) {
    Position pos(100.0f, 200.0f, 50.0f);
    EXPECT_FLOAT_EQ(pos.getX(), 100.0f);
    EXPECT_FLOAT_EQ(pos.getY(), 200.0f);
    EXPECT_FLOAT_EQ(pos.getZ(), 50.0f);
}

/**
 * @test Création avec valeurs négatives valides
 */
TEST_F(PositionTest, CreateWithNegativeValues) {
    Position pos(-500.0f, -300.0f, -100.0f);
    EXPECT_FLOAT_EQ(pos.getX(), -500.0f);
    EXPECT_FLOAT_EQ(pos.getY(), -300.0f);
    EXPECT_FLOAT_EQ(pos.getZ(), -100.0f);
}

/**
 * @test Création avec valeurs aux limites (min)
 */
TEST_F(PositionTest, CreateWithMinBoundaryValues) {
    ASSERT_NO_THROW({
        Position pos(-1000.0f, -1000.0f, -1000.0f);
        EXPECT_FLOAT_EQ(pos.getX(), -1000.0f);
        EXPECT_FLOAT_EQ(pos.getY(), -1000.0f);
        EXPECT_FLOAT_EQ(pos.getZ(), -1000.0f);
    });
}

/**
 * @test Création avec valeurs aux limites (max)
 */
TEST_F(PositionTest, CreateWithMaxBoundaryValues) {
    ASSERT_NO_THROW({
        Position pos(1000.0f, 1000.0f, 1000.0f);
        EXPECT_FLOAT_EQ(pos.getX(), 1000.0f);
        EXPECT_FLOAT_EQ(pos.getY(), 1000.0f);
        EXPECT_FLOAT_EQ(pos.getZ(), 1000.0f);
    });
}

/**
 * @test Création avec X hors limites (trop bas)
 */
TEST_F(PositionTest, CreateWithXBelowMinThrows) {
    EXPECT_THROW({
        Position pos(-1001.0f, 0.0f, 0.0f);
    }, PositionException);
}

/**
 * @test Création avec X hors limites (trop haut)
 */
TEST_F(PositionTest, CreateWithXAboveMaxThrows) {
    EXPECT_THROW({
        Position pos(1001.0f, 0.0f, 0.0f);
    }, PositionException);
}

/**
 * @test Création avec Y hors limites
 */
TEST_F(PositionTest, CreateWithYOutOfBoundsThrows) {
    EXPECT_THROW({
        Position pos(0.0f, -1500.0f, 0.0f);
    }, PositionException);

    EXPECT_THROW({
        Position pos(0.0f, 1500.0f, 0.0f);
    }, PositionException);
}

/**
 * @test Création avec Z hors limites
 */
TEST_F(PositionTest, CreateWithZOutOfBoundsThrows) {
    EXPECT_THROW({
        Position pos(0.0f, 0.0f, -2000.0f);
    }, PositionException);

    EXPECT_THROW({
        Position pos(0.0f, 0.0f, 2000.0f);
    }, PositionException);
}

// ============================================================================
// Tests de Move
// ============================================================================

/**
 * @test Move avec delta positif
 */
TEST_F(PositionTest, MoveWithPositiveDelta) {
    Position pos(100.0f, 100.0f, 100.0f);
    Position moved = pos.move(50.0f, 25.0f, 10.0f);

    EXPECT_FLOAT_EQ(moved.getX(), 150.0f);
    EXPECT_FLOAT_EQ(moved.getY(), 125.0f);
    EXPECT_FLOAT_EQ(moved.getZ(), 110.0f);
}

/**
 * @test Move avec delta négatif
 */
TEST_F(PositionTest, MoveWithNegativeDelta) {
    Position pos(100.0f, 100.0f, 100.0f);
    Position moved = pos.move(-50.0f, -25.0f, -10.0f);

    EXPECT_FLOAT_EQ(moved.getX(), 50.0f);
    EXPECT_FLOAT_EQ(moved.getY(), 75.0f);
    EXPECT_FLOAT_EQ(moved.getZ(), 90.0f);
}

/**
 * @test Move ne modifie pas l'objet original (immutabilité)
 */
TEST_F(PositionTest, MoveIsImmutable) {
    Position original(100.0f, 100.0f, 100.0f);
    (void)original.move(50.0f, 50.0f, 50.0f);

    EXPECT_FLOAT_EQ(original.getX(), 100.0f);
    EXPECT_FLOAT_EQ(original.getY(), 100.0f);
    EXPECT_FLOAT_EQ(original.getZ(), 100.0f);
}

/**
 * @test Move avec delta zéro
 */
TEST_F(PositionTest, MoveWithZeroDelta) {
    Position pos(100.0f, 200.0f, 300.0f);
    Position moved = pos.move(0.0f, 0.0f, 0.0f);

    EXPECT_FLOAT_EQ(moved.getX(), 100.0f);
    EXPECT_FLOAT_EQ(moved.getY(), 200.0f);
    EXPECT_FLOAT_EQ(moved.getZ(), 300.0f);
}

/**
 * @test Move qui dépasse les limites lance une exception
 */
TEST_F(PositionTest, MoveOutOfBoundsThrows) {
    Position pos(900.0f, 0.0f, 0.0f);

    EXPECT_THROW({
        pos.move(200.0f, 0.0f, 0.0f);
    }, PositionException);
}

/**
 * @test Move en chaîne
 */
TEST_F(PositionTest, ChainedMove) {
    Position pos(0.0f, 0.0f, 0.0f);
    Position result = pos.move(10.0f, 20.0f, 30.0f)
                         .move(5.0f, 5.0f, 5.0f)
                         .move(-5.0f, -10.0f, -15.0f);

    EXPECT_FLOAT_EQ(result.getX(), 10.0f);
    EXPECT_FLOAT_EQ(result.getY(), 15.0f);
    EXPECT_FLOAT_EQ(result.getZ(), 20.0f);
}

// ============================================================================
// Tests des Opérateurs
// ============================================================================

/**
 * @test Opérateur == avec positions égales
 */
TEST_F(PositionTest, EqualityOperatorTrue) {
    Position p1(100.0f, 200.0f, 300.0f);
    Position p2(100.0f, 200.0f, 300.0f);
    EXPECT_TRUE(p1 == p2);
}

/**
 * @test Opérateur == avec positions différentes
 */
TEST_F(PositionTest, EqualityOperatorFalse) {
    Position p1(100.0f, 200.0f, 300.0f);
    Position p2(100.0f, 200.0f, 301.0f);
    EXPECT_FALSE(p1 == p2);
}

/**
 * @test Opérateur != avec positions différentes
 */
TEST_F(PositionTest, InequalityOperatorTrue) {
    Position p1(100.0f, 200.0f, 300.0f);
    Position p2(101.0f, 200.0f, 300.0f);
    EXPECT_TRUE(p1 != p2);
}

/**
 * @test Opérateur != avec positions égales
 */
TEST_F(PositionTest, InequalityOperatorFalse) {
    Position p1(100.0f, 200.0f, 300.0f);
    Position p2(100.0f, 200.0f, 300.0f);
    EXPECT_FALSE(p1 != p2);
}

/**
 * @test Position par défaut égale à (0, 0, 0)
 */
TEST_F(PositionTest, DefaultPositionEquality) {
    Position p1;
    Position p2(0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(p1 == p2);
}

// ============================================================================
// Tests de Cas Limites
// ============================================================================

/**
 * @test Valeurs très proches des limites
 */
TEST_F(PositionTest, ValuesNearBoundaries) {
    EXPECT_NO_THROW({ Position(999.9f, 999.9f, 999.9f); });
    EXPECT_NO_THROW({ Position(-999.9f, -999.9f, -999.9f); });
}

/**
 * @test Move aux limites exactes
 */
TEST_F(PositionTest, MoveToExactBoundary) {
    Position pos(900.0f, 0.0f, 0.0f);
    EXPECT_NO_THROW({
        Position moved = pos.move(100.0f, 0.0f, 0.0f);
        EXPECT_FLOAT_EQ(moved.getX(), 1000.0f);
    });
}
