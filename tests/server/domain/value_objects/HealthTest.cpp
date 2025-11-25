/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour Health Value Object
*/

#include <gtest/gtest.h>
#include "domain/value_objects/Health.hpp"
#include "domain/exceptions/HealthException.hpp"

using namespace domain::value_objects;
using namespace domain::exceptions;

/**
 * @brief Suite de tests pour le Value Object Health
 *
 * Health représente les points de vie d'une entité.
 * Contraintes : valeur entre 0.0 et 5.0 (inclus)
 */
class HealthTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Construction
// ============================================================================

/**
 * @test Création avec valeur valide minimale (0.0)
 */
TEST_F(HealthTest, CreateWithMinValue) {
    ASSERT_NO_THROW({
        Health health(0.0f);
        EXPECT_FLOAT_EQ(health.value(), 0.0f);
    });
}

/**
 * @test Création avec valeur valide maximale (5.0)
 */
TEST_F(HealthTest, CreateWithMaxValue) {
    ASSERT_NO_THROW({
        Health health(5.0f);
        EXPECT_FLOAT_EQ(health.value(), 5.0f);
    });
}

/**
 * @test Création avec valeur valide intermédiaire
 */
TEST_F(HealthTest, CreateWithMiddleValue) {
    Health health(2.5f);
    EXPECT_FLOAT_EQ(health.value(), 2.5f);
}

/**
 * @test Création avec valeur négative - doit lancer HealthException
 */
TEST_F(HealthTest, CreateWithNegativeValueThrows) {
    EXPECT_THROW({
        Health health(-1.0f);
    }, HealthException);
}

/**
 * @test Création avec valeur supérieure au max - doit lancer HealthException
 */
TEST_F(HealthTest, CreateWithValueAboveMaxThrows) {
    EXPECT_THROW({
        Health health(5.1f);
    }, HealthException);
}

/**
 * @test Création avec valeur très négative
 */
TEST_F(HealthTest, CreateWithVeryNegativeValueThrows) {
    EXPECT_THROW({
        Health health(-100.0f);
    }, HealthException);
}

/**
 * @test Création avec valeur très élevée
 */
TEST_F(HealthTest, CreateWithVeryHighValueThrows) {
    EXPECT_THROW({
        Health health(100.0f);
    }, HealthException);
}

// ============================================================================
// Tests de Heal (Soins)
// ============================================================================

/**
 * @test Heal augmente la santé correctement
 */
TEST_F(HealthTest, HealIncreasesHealth) {
    Health health(2.0f);
    Health healed = health.heal(1.0f);
    EXPECT_FLOAT_EQ(healed.value(), 3.0f);
}

/**
 * @test Heal ne modifie pas l'objet original (immutabilité)
 */
TEST_F(HealthTest, HealIsImmutable) {
    Health original(2.0f);
    Health healed = original.heal(1.0f);
    EXPECT_FLOAT_EQ(original.value(), 2.0f);
    EXPECT_FLOAT_EQ(healed.value(), 3.0f);
}

/**
 * @test Heal au-delà du max lance une exception
 */
TEST_F(HealthTest, HealAboveMaxThrows) {
    Health health(4.0f);
    EXPECT_THROW({
        health.heal(2.0f);
    }, HealthException);
}

/**
 * @test Heal avec zéro ne change rien
 */
TEST_F(HealthTest, HealWithZero) {
    Health health(3.0f);
    Health healed = health.heal(0.0f);
    EXPECT_FLOAT_EQ(healed.value(), 3.0f);
}

// ============================================================================
// Tests de Damage (Dégâts)
// ============================================================================

/**
 * @test Damage réduit la santé correctement
 */
TEST_F(HealthTest, DamageDecreasesHealth) {
    Health health(3.0f);
    Health damaged = health.damage(1.0f);
    EXPECT_FLOAT_EQ(damaged.value(), 2.0f);
}

/**
 * @test Damage ne modifie pas l'objet original (immutabilité)
 */
TEST_F(HealthTest, DamageIsImmutable) {
    Health original(3.0f);
    Health damaged = original.damage(1.0f);
    EXPECT_FLOAT_EQ(original.value(), 3.0f);
    EXPECT_FLOAT_EQ(damaged.value(), 2.0f);
}

/**
 * @test Damage en dessous de zéro lance une exception
 */
TEST_F(HealthTest, DamageBelowZeroThrows) {
    Health health(1.0f);
    EXPECT_THROW({
        health.damage(2.0f);
    }, HealthException);
}

/**
 * @test Damage avec zéro ne change rien
 */
TEST_F(HealthTest, DamageWithZero) {
    Health health(3.0f);
    Health damaged = health.damage(0.0f);
    EXPECT_FLOAT_EQ(damaged.value(), 3.0f);
}

/**
 * @test Damage jusqu'à zéro exact
 */
TEST_F(HealthTest, DamageToExactlyZero) {
    Health health(2.0f);
    Health damaged = health.damage(2.0f);
    EXPECT_FLOAT_EQ(damaged.value(), 0.0f);
}

// ============================================================================
// Tests des Opérateurs
// ============================================================================

/**
 * @test Opérateur == avec valeurs égales
 */
TEST_F(HealthTest, EqualityOperatorTrue) {
    Health h1(3.0f);
    Health h2(3.0f);
    EXPECT_TRUE(h1 == h2);
}

/**
 * @test Opérateur == avec valeurs différentes
 */
TEST_F(HealthTest, EqualityOperatorFalse) {
    Health h1(3.0f);
    Health h2(2.0f);
    EXPECT_FALSE(h1 == h2);
}

/**
 * @test Opérateur < avec h1 < h2
 */
TEST_F(HealthTest, LessThanOperatorTrue) {
    Health h1(2.0f);
    Health h2(3.0f);
    EXPECT_TRUE(h1 < h2);
}

/**
 * @test Opérateur < avec h1 >= h2
 */
TEST_F(HealthTest, LessThanOperatorFalse) {
    Health h1(3.0f);
    Health h2(2.0f);
    EXPECT_FALSE(h1 < h2);
}

/**
 * @test Opérateur < avec valeurs égales
 */
TEST_F(HealthTest, LessThanOperatorEqualValues) {
    Health h1(3.0f);
    Health h2(3.0f);
    EXPECT_FALSE(h1 < h2);
}

// ============================================================================
// Tests de Cas Limites
// ============================================================================

/**
 * @test Valeurs aux bornes exactes
 */
TEST_F(HealthTest, BoundaryValues) {
    EXPECT_NO_THROW({ Health(0.0f); });
    EXPECT_NO_THROW({ Health(5.0f); });
    EXPECT_THROW({ Health(-0.001f); }, HealthException);
    EXPECT_THROW({ Health(5.001f); }, HealthException);
}

/**
 * @test Chaîne de heal et damage
 */
TEST_F(HealthTest, ChainedOperations) {
    Health health(2.0f);
    Health result = health.heal(1.0f).damage(0.5f).heal(0.5f);
    EXPECT_FLOAT_EQ(result.value(), 3.0f);
}
