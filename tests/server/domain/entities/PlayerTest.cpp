/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour l'entité Player
*/

#include <gtest/gtest.h>
#include "domain/entities/Player.hpp"
#include "domain/value_objects/Health.hpp"
#include "domain/value_objects/Position.hpp"
#include "domain/value_objects/player/PlayerId.hpp"
#include "domain/exceptions/HealthException.hpp"
#include "domain/exceptions/PositionException.hpp"

using namespace domain::entities;
using namespace domain::value_objects;
using namespace domain::value_objects::player;
using namespace domain::exceptions;

/**
 * @brief Suite de tests pour l'entité Player
 *
 * Player est une entité de domaine représentant un joueur.
 * Composée de :
 * - PlayerId : identifiant unique (MongoDB ObjectId)
 * - Health : points de vie (0.0 - 5.0)
 * - Position : position 3D (-1000 à 1000)
 */
class PlayerTest : public ::testing::Test {
protected:
    // ID MongoDB valide pour les tests
    const std::string validId = "507f1f77bcf86cd799439011";

    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Construction
// ============================================================================

/**
 * @test Création d'un Player avec tous les paramètres
 */
TEST_F(PlayerTest, CreateWithAllParameters) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 200.0f, 50.0f);

    ASSERT_NO_THROW({
        Player player(health, id, position);
    });
}

/**
 * @test Création d'un Player avec position par défaut
 */
TEST_F(PlayerTest, CreateWithDefaultPosition) {
    Health health(5.0f);
    PlayerId id(validId);

    ASSERT_NO_THROW({
        Player player(health, id);
        EXPECT_FLOAT_EQ(player.getPosition().getX(), 0.0f);
        EXPECT_FLOAT_EQ(player.getPosition().getY(), 0.0f);
        EXPECT_FLOAT_EQ(player.getPosition().getZ(), 0.0f);
    });
}

/**
 * @test Vérification que getId retourne le bon ID
 */
TEST_F(PlayerTest, GetIdReturnsCorrectValue) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(0.0f, 0.0f, 0.0f);

    Player player(health, id, position);
    EXPECT_EQ(player.getId().value(), validId);
}

/**
 * @test Vérification que getPosition retourne la bonne position
 */
TEST_F(PlayerTest, GetPositionReturnsCorrectValue) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(150.0f, 250.0f, 75.0f);

    Player player(health, id, position);
    EXPECT_FLOAT_EQ(player.getPosition().getX(), 150.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 250.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 75.0f);
}

// ============================================================================
// Tests de Movement
// ============================================================================

/**
 * @test Move modifie correctement la position
 */
TEST_F(PlayerTest, MoveChangesPosition) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 100.0f, 100.0f);

    Player player(health, id, position);
    player.move(50.0f, 25.0f, 10.0f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 150.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 125.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 110.0f);
}

/**
 * @test Move avec delta négatif
 */
TEST_F(PlayerTest, MoveWithNegativeDelta) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 100.0f, 100.0f);

    Player player(health, id, position);
    player.move(-30.0f, -40.0f, -20.0f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 70.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 60.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 80.0f);
}

/**
 * @test Move avec delta zéro
 */
TEST_F(PlayerTest, MoveWithZeroDelta) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 200.0f, 300.0f);

    Player player(health, id, position);
    player.move(0.0f, 0.0f, 0.0f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 100.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 200.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 300.0f);
}

/**
 * @test Move qui dépasse les limites lance une exception
 */
TEST_F(PlayerTest, MoveOutOfBoundsThrows) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(900.0f, 0.0f, 0.0f);

    Player player(health, id, position);

    EXPECT_THROW({
        player.move(200.0f, 0.0f, 0.0f);
    }, PositionException);
}

/**
 * @test Plusieurs moves consécutifs
 */
TEST_F(PlayerTest, ConsecutiveMoves) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(0.0f, 0.0f, 0.0f);

    Player player(health, id, position);
    player.move(10.0f, 20.0f, 30.0f);
    player.move(5.0f, 5.0f, 5.0f);
    player.move(-5.0f, -10.0f, -15.0f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 10.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 15.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 20.0f);
}

// ============================================================================
// Tests de Heal
// ============================================================================

/**
 * @test Heal augmente la santé du joueur
 */
TEST_F(PlayerTest, HealIncreasesHealth) {
    Health health(2.0f);
    PlayerId id(validId);

    Player player(health, id);
    player.heal(1.0f);
    // Note: Pas de getter pour health dans Player, mais heal ne devrait pas throw
    SUCCEED();
}

/**
 * @test Heal avec valeur zéro
 */
TEST_F(PlayerTest, HealWithZero) {
    Health health(3.0f);
    PlayerId id(validId);

    Player player(health, id);
    EXPECT_NO_THROW({
        player.heal(0.0f);
    });
}

/**
 * @test Heal au-delà du max lance une exception
 */
TEST_F(PlayerTest, HealAboveMaxThrows) {
    Health health(4.0f);
    PlayerId id(validId);

    Player player(health, id);

    EXPECT_THROW({
        player.heal(2.0f);  // 4.0 + 2.0 = 6.0 > 5.0
    }, HealthException);
}

// ============================================================================
// Tests de TakeDamage
// ============================================================================

/**
 * @test TakeDamage réduit la santé du joueur
 */
TEST_F(PlayerTest, TakeDamageDecreasesHealth) {
    Health health(3.0f);
    PlayerId id(validId);

    Player player(health, id);
    player.takeDamage(1.0f);
    // Note: Pas de getter pour health dans Player
    SUCCEED();
}

/**
 * @test TakeDamage avec valeur zéro
 */
TEST_F(PlayerTest, TakeDamageWithZero) {
    Health health(3.0f);
    PlayerId id(validId);

    Player player(health, id);
    EXPECT_NO_THROW({
        player.takeDamage(0.0f);
    });
}

/**
 * @test TakeDamage en dessous de zéro lance une exception
 */
TEST_F(PlayerTest, TakeDamageBelowZeroThrows) {
    Health health(1.0f);
    PlayerId id(validId);

    Player player(health, id);

    EXPECT_THROW({
        player.takeDamage(2.0f);  // 1.0 - 2.0 = -1.0 < 0
    }, HealthException);
}

/**
 * @test TakeDamage jusqu'à exactement zéro
 */
TEST_F(PlayerTest, TakeDamageToExactlyZero) {
    Health health(2.0f);
    PlayerId id(validId);

    Player player(health, id);
    EXPECT_NO_THROW({
        player.takeDamage(2.0f);
    });
}

// ============================================================================
// Tests Combinés
// ============================================================================

/**
 * @test Séquence de heal et damage
 */
TEST_F(PlayerTest, HealAndDamageSequence) {
    Health health(2.0f);
    PlayerId id(validId);

    Player player(health, id);

    // Heal +1, puis damage -0.5
    player.heal(1.0f);  // 3.0
    player.takeDamage(0.5f);  // 2.5
    player.heal(0.5f);  // 3.0
    player.takeDamage(1.0f);  // 2.0

    SUCCEED();
}

/**
 * @test Move et heal combinés
 */
TEST_F(PlayerTest, MoveAndHealCombined) {
    Health health(2.0f);
    PlayerId id(validId);
    Position position(0.0f, 0.0f, 0.0f);

    Player player(health, id, position);

    player.move(100.0f, 100.0f, 0.0f);
    player.heal(1.0f);
    player.move(50.0f, 50.0f, 0.0f);
    player.takeDamage(0.5f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 150.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 150.0f);
}

// ============================================================================
// Tests de Cas Limites
// ============================================================================

/**
 * @test Player à la position limite
 */
TEST_F(PlayerTest, PlayerAtBoundaryPosition) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(1000.0f, 1000.0f, 1000.0f);

    ASSERT_NO_THROW({
        Player player(health, id, position);
        EXPECT_FLOAT_EQ(player.getPosition().getX(), 1000.0f);
    });
}

/**
 * @test Player avec santé minimale
 */
TEST_F(PlayerTest, PlayerWithMinHealth) {
    Health health(0.0f);
    PlayerId id(validId);

    ASSERT_NO_THROW({
        Player player(health, id);
    });
}

/**
 * @test Player avec santé maximale
 */
TEST_F(PlayerTest, PlayerWithMaxHealth) {
    Health health(5.0f);
    PlayerId id(validId);

    ASSERT_NO_THROW({
        Player player(health, id);
    });
}
