/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour Username Value Object
*/

#include <gtest/gtest.h>
#include "domain/value_objects/user/Username.hpp"
#include "domain/exceptions/user/UsernameException.hpp"

using namespace domain::value_objects::user;
using namespace domain::exceptions::user;

/**
 * @brief Suite de tests pour le Value Object Username
 *
 * Username représente un nom d'utilisateur validé.
 * Contraintes :
 * - Longueur minimale : 3 caractères
 * - Longueur maximale : 21 caractères
 */
class UsernameTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Construction - Usernames Valides
// ============================================================================

/**
 * @test Username avec longueur minimale (3 caractères)
 */
TEST_F(UsernameTest, CreateWithMinLength) {
    ASSERT_NO_THROW({
        Username username("abc");
        EXPECT_EQ(username.value(), "abc");
    });
}

/**
 * @test Username avec longueur maximale (21 caractères)
 */
TEST_F(UsernameTest, CreateWithMaxLength) {
    std::string maxUsername(21, 'a');
    ASSERT_NO_THROW({
        Username username(maxUsername);
        EXPECT_EQ(username.value(), maxUsername);
    });
}

/**
 * @test Username avec longueur intermédiaire
 */
TEST_F(UsernameTest, CreateWithMiddleLength) {
    ASSERT_NO_THROW({
        Username username("player_one");
        EXPECT_EQ(username.value(), "player_one");
    });
}

/**
 * @test Username avec chiffres
 */
TEST_F(UsernameTest, CreateWithNumbers) {
    ASSERT_NO_THROW({
        Username username("player123");
        EXPECT_EQ(username.value(), "player123");
    });
}

/**
 * @test Username avec underscore
 */
TEST_F(UsernameTest, CreateWithUnderscore) {
    ASSERT_NO_THROW({
        Username username("my_user_name");
    });
}

/**
 * @test Username avec tiret
 */
TEST_F(UsernameTest, CreateWithHyphen) {
    ASSERT_NO_THROW({
        Username username("my-user-name");
    });
}

/**
 * @test Username mixte (lettres, chiffres, caractères spéciaux)
 */
TEST_F(UsernameTest, CreateWithMixedCharacters) {
    ASSERT_NO_THROW({
        Username username("Player_1-Test");
    });
}

// ============================================================================
// Tests de Construction - Usernames Invalides
// ============================================================================

/**
 * @test Username trop court (2 caractères)
 */
TEST_F(UsernameTest, CreateWithTooShortThrows) {
    EXPECT_THROW({
        Username username("ab");
    }, UsernameException);
}

/**
 * @test Username trop court (1 caractère)
 */
TEST_F(UsernameTest, CreateWithOneCharThrows) {
    EXPECT_THROW({
        Username username("a");
    }, UsernameException);
}

/**
 * @test Username vide
 */
TEST_F(UsernameTest, CreateWithEmptyStringThrows) {
    EXPECT_THROW({
        Username username("");
    }, UsernameException);
}

/**
 * @test Username trop long (22 caractères)
 */
TEST_F(UsernameTest, CreateWithTooLongThrows) {
    std::string tooLong(22, 'a');
    EXPECT_THROW({
        Username username(tooLong);
    }, UsernameException);
}

/**
 * @test Username beaucoup trop long (100 caractères)
 */
TEST_F(UsernameTest, CreateWithVeryLongThrows) {
    std::string veryLong(100, 'x');
    EXPECT_THROW({
        Username username(veryLong);
    }, UsernameException);
}

// ============================================================================
// Tests des Opérateurs
// ============================================================================

/**
 * @test Opérateur == avec usernames égaux
 */
TEST_F(UsernameTest, EqualityOperatorTrue) {
    Username u1("player_one");
    Username u2("player_one");
    EXPECT_TRUE(u1 == u2);
}

/**
 * @test Opérateur == avec usernames différents
 */
TEST_F(UsernameTest, EqualityOperatorFalse) {
    Username u1("player_one");
    Username u2("player_two");
    EXPECT_FALSE(u1 == u2);
}

/**
 * @test Opérateur != avec usernames différents
 */
TEST_F(UsernameTest, InequalityOperatorTrue) {
    Username u1("username1");
    Username u2("username2");
    EXPECT_TRUE(u1 != u2);
}

/**
 * @test Opérateur != avec usernames égaux
 */
TEST_F(UsernameTest, InequalityOperatorFalse) {
    Username u1("testuser");
    Username u2("testuser");
    EXPECT_FALSE(u1 != u2);
}

/**
 * @test Usernames sensibles à la casse
 */
TEST_F(UsernameTest, UsernamesAreCaseSensitive) {
    Username u1("PlayerOne");
    Username u2("playerone");
    EXPECT_TRUE(u1 != u2);
}

// ============================================================================
// Tests de Getter
// ============================================================================

/**
 * @test value() retourne la valeur correcte
 */
TEST_F(UsernameTest, ValueReturnsCorrectUsername) {
    Username username("myusername");
    EXPECT_EQ(username.value(), "myusername");
}

// ============================================================================
// Tests de Cas Limites
// ============================================================================

/**
 * @test Exactement à la limite min (3 caractères)
 */
TEST_F(UsernameTest, ExactlyMinLength) {
    EXPECT_NO_THROW({
        Username username("123");
        EXPECT_EQ(username.value().length(), 3u);
    });
}

/**
 * @test Exactement à la limite max (21 caractères)
 */
TEST_F(UsernameTest, ExactlyMaxLength) {
    std::string exact21(21, 'z');
    EXPECT_NO_THROW({
        Username username(exact21);
        EXPECT_EQ(username.value().length(), 21u);
    });
}

/**
 * @test Un caractère en dessous de la limite min
 */
TEST_F(UsernameTest, OneBelowMinLength) {
    EXPECT_THROW({
        Username username("12");  // 2 chars
    }, UsernameException);
}

/**
 * @test Un caractère au-dessus de la limite max
 */
TEST_F(UsernameTest, OneAboveMaxLength) {
    std::string oneAbove(22, 'z');
    EXPECT_THROW({
        Username username(oneAbove);
    }, UsernameException);
}

/**
 * @test Username avec espaces (si autorisé ou non selon l'implémentation)
 */
TEST_F(UsernameTest, CreateWithSpaces) {
    // Note: L'implémentation actuelle ne valide pas les espaces
    // Si les espaces doivent être interdits, ce test devrait THROW
    ASSERT_NO_THROW({
        Username username("user name");
    });
}

/**
 * @test Username uniquement numérique
 */
TEST_F(UsernameTest, CreateWithOnlyNumbers) {
    ASSERT_NO_THROW({
        Username username("123456789");
    });
}
