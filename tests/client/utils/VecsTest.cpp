/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour les structures Vecs
*/

#include <gtest/gtest.h>
#include "utils/Vecs.hpp"

/**
 * @brief Suite de tests pour les structures vectorielles
 *
 * Les Vecs sont des structures simples représentant des vecteurs
 * 2D et 3D avec différents types (int, unsigned int, float).
 */
class VecsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests Vec2i
// ============================================================================

/**
 * @test Création de Vec2i avec valeurs positives
 */
TEST_F(VecsTest, Vec2iCreationPositive) {
    Vec2i vec{10, 20};
    EXPECT_EQ(vec.x, 10);
    EXPECT_EQ(vec.y, 20);
}

/**
 * @test Création de Vec2i avec valeurs négatives
 */
TEST_F(VecsTest, Vec2iCreationNegative) {
    Vec2i vec{-5, -15};
    EXPECT_EQ(vec.x, -5);
    EXPECT_EQ(vec.y, -15);
}

/**
 * @test Création de Vec2i avec zéros
 */
TEST_F(VecsTest, Vec2iCreationZero) {
    Vec2i vec{0, 0};
    EXPECT_EQ(vec.x, 0);
    EXPECT_EQ(vec.y, 0);
}

/**
 * @test Vec2i modification des membres
 */
TEST_F(VecsTest, Vec2iModification) {
    Vec2i vec{1, 2};
    vec.x = 100;
    vec.y = 200;
    EXPECT_EQ(vec.x, 100);
    EXPECT_EQ(vec.y, 200);
}

// ============================================================================
// Tests Vec2u
// ============================================================================

/**
 * @test Création de Vec2u avec valeurs positives
 */
TEST_F(VecsTest, Vec2uCreationPositive) {
    Vec2u vec{100u, 200u};
    EXPECT_EQ(vec.x, 100u);
    EXPECT_EQ(vec.y, 200u);
}

/**
 * @test Création de Vec2u avec zéros
 */
TEST_F(VecsTest, Vec2uCreationZero) {
    Vec2u vec{0u, 0u};
    EXPECT_EQ(vec.x, 0u);
    EXPECT_EQ(vec.y, 0u);
}

/**
 * @test Vec2u avec grandes valeurs
 */
TEST_F(VecsTest, Vec2uLargeValues) {
    Vec2u vec{1920u, 1080u};
    EXPECT_EQ(vec.x, 1920u);
    EXPECT_EQ(vec.y, 1080u);
}

/**
 * @test Vec2u modification
 */
TEST_F(VecsTest, Vec2uModification) {
    Vec2u vec{0u, 0u};
    vec.x = 1920u;
    vec.y = 1080u;
    EXPECT_EQ(vec.x, 1920u);
    EXPECT_EQ(vec.y, 1080u);
}

// ============================================================================
// Tests Vec2f
// ============================================================================

/**
 * @test Création de Vec2f avec valeurs positives
 */
TEST_F(VecsTest, Vec2fCreationPositive) {
    Vec2f vec{1.5f, 2.5f};
    EXPECT_FLOAT_EQ(vec.x, 1.5f);
    EXPECT_FLOAT_EQ(vec.y, 2.5f);
}

/**
 * @test Création de Vec2f avec valeurs négatives
 */
TEST_F(VecsTest, Vec2fCreationNegative) {
    Vec2f vec{-3.14f, -2.71f};
    EXPECT_FLOAT_EQ(vec.x, -3.14f);
    EXPECT_FLOAT_EQ(vec.y, -2.71f);
}

/**
 * @test Création de Vec2f avec zéros
 */
TEST_F(VecsTest, Vec2fCreationZero) {
    Vec2f vec{0.0f, 0.0f};
    EXPECT_FLOAT_EQ(vec.x, 0.0f);
    EXPECT_FLOAT_EQ(vec.y, 0.0f);
}

/**
 * @test Vec2f avec valeurs décimales précises
 */
TEST_F(VecsTest, Vec2fPrecision) {
    Vec2f vec{0.123456f, 0.654321f};
    EXPECT_FLOAT_EQ(vec.x, 0.123456f);
    EXPECT_FLOAT_EQ(vec.y, 0.654321f);
}

/**
 * @test Vec2f modification
 */
TEST_F(VecsTest, Vec2fModification) {
    Vec2f vec{0.0f, 0.0f};
    vec.x = 100.5f;
    vec.y = 200.75f;
    EXPECT_FLOAT_EQ(vec.x, 100.5f);
    EXPECT_FLOAT_EQ(vec.y, 200.75f);
}

// ============================================================================
// Tests Vec3i
// ============================================================================

/**
 * @test Création de Vec3i avec valeurs positives
 */
TEST_F(VecsTest, Vec3iCreationPositive) {
    Vec3i vec{10, 20, 30};
    EXPECT_EQ(vec.x, 10);
    EXPECT_EQ(vec.y, 20);
    EXPECT_EQ(vec.z, 30);
}

/**
 * @test Création de Vec3i avec valeurs négatives
 */
TEST_F(VecsTest, Vec3iCreationNegative) {
    Vec3i vec{-10, -20, -30};
    EXPECT_EQ(vec.x, -10);
    EXPECT_EQ(vec.y, -20);
    EXPECT_EQ(vec.z, -30);
}

/**
 * @test Création de Vec3i avec valeurs mixtes
 */
TEST_F(VecsTest, Vec3iCreationMixed) {
    Vec3i vec{-5, 0, 5};
    EXPECT_EQ(vec.x, -5);
    EXPECT_EQ(vec.y, 0);
    EXPECT_EQ(vec.z, 5);
}

/**
 * @test Vec3i modification
 */
TEST_F(VecsTest, Vec3iModification) {
    Vec3i vec{0, 0, 0};
    vec.x = 1;
    vec.y = 2;
    vec.z = 3;
    EXPECT_EQ(vec.x, 1);
    EXPECT_EQ(vec.y, 2);
    EXPECT_EQ(vec.z, 3);
}

// ============================================================================
// Tests Vec3f
// ============================================================================

/**
 * @test Création de Vec3f avec valeurs positives
 */
TEST_F(VecsTest, Vec3fCreationPositive) {
    Vec3f vec{1.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(vec.x, 1.0f);
    EXPECT_FLOAT_EQ(vec.y, 2.0f);
    EXPECT_FLOAT_EQ(vec.z, 3.0f);
}

/**
 * @test Création de Vec3f avec valeurs négatives
 */
TEST_F(VecsTest, Vec3fCreationNegative) {
    Vec3f vec{-1.5f, -2.5f, -3.5f};
    EXPECT_FLOAT_EQ(vec.x, -1.5f);
    EXPECT_FLOAT_EQ(vec.y, -2.5f);
    EXPECT_FLOAT_EQ(vec.z, -3.5f);
}

/**
 * @test Création de Vec3f avec zéros
 */
TEST_F(VecsTest, Vec3fCreationZero) {
    Vec3f vec{0.0f, 0.0f, 0.0f};
    EXPECT_FLOAT_EQ(vec.x, 0.0f);
    EXPECT_FLOAT_EQ(vec.y, 0.0f);
    EXPECT_FLOAT_EQ(vec.z, 0.0f);
}

/**
 * @test Vec3f pour représenter une position 3D typique
 */
TEST_F(VecsTest, Vec3fPosition3D) {
    Vec3f position{100.5f, 200.25f, 50.0f};
    EXPECT_FLOAT_EQ(position.x, 100.5f);
    EXPECT_FLOAT_EQ(position.y, 200.25f);
    EXPECT_FLOAT_EQ(position.z, 50.0f);
}

/**
 * @test Vec3f modification
 */
TEST_F(VecsTest, Vec3fModification) {
    Vec3f vec{0.0f, 0.0f, 0.0f};
    vec.x = 10.1f;
    vec.y = 20.2f;
    vec.z = 30.3f;
    EXPECT_FLOAT_EQ(vec.x, 10.1f);
    EXPECT_FLOAT_EQ(vec.y, 20.2f);
    EXPECT_FLOAT_EQ(vec.z, 30.3f);
}

// ============================================================================
// Tests Vec4i
// ============================================================================

/**
 * @test Création de Vec4i avec valeurs positives
 */
TEST_F(VecsTest, Vec4iCreationPositive) {
    Vec4i vec{1, 2, 3};
    EXPECT_EQ(vec.x, 1);
    EXPECT_EQ(vec.y, 2);
    EXPECT_EQ(vec.z, 3);
}

/**
 * @test Vec4i modification
 */
TEST_F(VecsTest, Vec4iModification) {
    Vec4i vec{0, 0, 0};
    vec.x = 255;
    vec.y = 128;
    vec.z = 64;
    EXPECT_EQ(vec.x, 255);
    EXPECT_EQ(vec.y, 128);
    EXPECT_EQ(vec.z, 64);
}

// ============================================================================
// Tests d'Usage Pratique
// ============================================================================

/**
 * @test Vec2f pour représenter une échelle
 */
TEST_F(VecsTest, Vec2fAsScale) {
    Vec2f scale{1.0f, 1.0f};
    EXPECT_FLOAT_EQ(scale.x, 1.0f);
    EXPECT_FLOAT_EQ(scale.y, 1.0f);

    scale.x = 2.0f;  // Double la largeur
    scale.y = 0.5f;  // Moitié de la hauteur
    EXPECT_FLOAT_EQ(scale.x, 2.0f);
    EXPECT_FLOAT_EQ(scale.y, 0.5f);
}

/**
 * @test Vec2u pour représenter une résolution
 */
TEST_F(VecsTest, Vec2uAsResolution) {
    Vec2u resolution{1920u, 1080u};
    EXPECT_EQ(resolution.x, 1920u);
    EXPECT_EQ(resolution.y, 1080u);
}

/**
 * @test Vec2i pour représenter une position écran
 */
TEST_F(VecsTest, Vec2iAsScreenPosition) {
    Vec2i pos{800, 600};
    EXPECT_EQ(pos.x, 800);
    EXPECT_EQ(pos.y, 600);
}

/**
 * @test Copie de Vec2f
 */
TEST_F(VecsTest, Vec2fCopy) {
    Vec2f original{10.0f, 20.0f};
    Vec2f copy = original;

    EXPECT_FLOAT_EQ(copy.x, 10.0f);
    EXPECT_FLOAT_EQ(copy.y, 20.0f);

    // Modifier la copie ne change pas l'original
    copy.x = 100.0f;
    EXPECT_FLOAT_EQ(original.x, 10.0f);
    EXPECT_FLOAT_EQ(copy.x, 100.0f);
}

/**
 * @test Initialisation par liste
 */
TEST_F(VecsTest, BraceInitialization) {
    Vec2i v2i = {1, 2};
    Vec2u v2u = {3u, 4u};
    Vec2f v2f = {5.0f, 6.0f};
    Vec3i v3i = {7, 8, 9};
    Vec3f v3f = {10.0f, 11.0f, 12.0f};

    EXPECT_EQ(v2i.x, 1);
    EXPECT_EQ(v2u.x, 3u);
    EXPECT_FLOAT_EQ(v2f.x, 5.0f);
    EXPECT_EQ(v3i.z, 9);
    EXPECT_FLOAT_EQ(v3f.z, 12.0f);
}
