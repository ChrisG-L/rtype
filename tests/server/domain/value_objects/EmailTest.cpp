/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour Email Value Object
*/

#include <gtest/gtest.h>
#include "domain/value_objects/user/Email.hpp"
#include "domain/exceptions/user/EmailException.hpp"

using namespace domain::value_objects::user;
using namespace domain::exceptions::user;

/**
 * @brief Suite de tests pour le Value Object Email
 *
 * Email représente une adresse email validée.
 * Contraintes :
 * - Format email valide (regex RFC 5322 simplifiée)
 * - Longueur maximale : 254 caractères
 */
class EmailTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Construction - Emails Valides
// ============================================================================

/**
 * @test Email simple valide
 */
TEST_F(EmailTest, CreateWithSimpleValidEmail) {
    ASSERT_NO_THROW({
        Email email("test@example.com");
        EXPECT_EQ(email.value(), "test@example.com");
    });
}

/**
 * @test Email avec sous-domaine
 */
TEST_F(EmailTest, CreateWithSubdomain) {
    ASSERT_NO_THROW({
        Email email("user@mail.example.com");
        EXPECT_EQ(email.value(), "user@mail.example.com");
    });
}

/**
 * @test Email avec chiffres dans la partie locale
 */
TEST_F(EmailTest, CreateWithNumbersInLocal) {
    ASSERT_NO_THROW({
        Email email("user123@example.com");
        EXPECT_EQ(email.value(), "user123@example.com");
    });
}

/**
 * @test Email avec points dans la partie locale
 */
TEST_F(EmailTest, CreateWithDotsInLocal) {
    ASSERT_NO_THROW({
        Email email("first.last@example.com");
        EXPECT_EQ(email.value(), "first.last@example.com");
    });
}

/**
 * @test Email avec caractères spéciaux autorisés
 */
TEST_F(EmailTest, CreateWithSpecialChars) {
    ASSERT_NO_THROW({
        Email email("user+tag@example.com");
    });

    ASSERT_NO_THROW({
        Email email("user_name@example.com");
    });

    ASSERT_NO_THROW({
        Email email("user-name@example.com");
    });
}

/**
 * @test Email avec domaine court
 */
TEST_F(EmailTest, CreateWithShortDomain) {
    ASSERT_NO_THROW({
        Email email("test@ex.co");
        EXPECT_EQ(email.value(), "test@ex.co");
    });
}

// ============================================================================
// Tests de Construction - Emails Invalides
// ============================================================================

/**
 * @test Email sans @
 */
TEST_F(EmailTest, CreateWithoutAtSymbolThrows) {
    EXPECT_THROW({
        Email email("testexample.com");
    }, EmailException);
}

/**
 * @test Email sans domaine
 */
TEST_F(EmailTest, CreateWithoutDomainThrows) {
    EXPECT_THROW({
        Email email("test@");
    }, EmailException);
}

/**
 * @test Email sans partie locale
 */
TEST_F(EmailTest, CreateWithoutLocalPartThrows) {
    EXPECT_THROW({
        Email email("@example.com");
    }, EmailException);
}

/**
 * @test Email avec espaces
 */
TEST_F(EmailTest, CreateWithSpacesThrows) {
    EXPECT_THROW({
        Email email("test user@example.com");
    }, EmailException);

    EXPECT_THROW({
        Email email("test@exam ple.com");
    }, EmailException);
}

/**
 * @test Email avec double @
 */
TEST_F(EmailTest, CreateWithDoubleAtThrows) {
    EXPECT_THROW({
        Email email("test@@example.com");
    }, EmailException);
}

/**
 * @test Email vide
 */
TEST_F(EmailTest, CreateWithEmptyStringThrows) {
    EXPECT_THROW({
        Email email("");
    }, EmailException);
}

/**
 * @test Email avec seulement @
 */
TEST_F(EmailTest, CreateWithOnlyAtThrows) {
    EXPECT_THROW({
        Email email("@");
    }, EmailException);
}

/**
 * @test Email trop long (> 254 caractères)
 */
TEST_F(EmailTest, CreateWithTooLongEmailThrows) {
    // Créer un email > 254 caractères
    // "@example.com" = 12 caractères, donc 243+ pour dépasser 254
    std::string longLocal(250, 'a');
    std::string longEmail = longLocal + "@example.com";  // 250 + 12 = 262 > 254

    EXPECT_THROW({
        Email email(longEmail);
    }, EmailException);
}

/**
 * @test Email exactement à la limite (254 caractères)
 */
TEST_F(EmailTest, CreateWithMaxLengthEmail) {
    // Créer un email de 254 caractères exactement
    std::string local(241, 'a');  // 241 + @ + example.com = 241 + 1 + 11 = 253... ajustons
    std::string domain = "@ex.com";  // 7 caractères
    std::string maxEmail = std::string(247, 'a') + domain;  // 247 + 7 = 254

    ASSERT_EQ(maxEmail.length(), 254u);
    EXPECT_NO_THROW({
        Email email(maxEmail);
    });
}

// ============================================================================
// Tests des Opérateurs
// ============================================================================

/**
 * @test Opérateur == avec emails égaux
 */
TEST_F(EmailTest, EqualityOperatorTrue) {
    Email e1("test@example.com");
    Email e2("test@example.com");
    EXPECT_TRUE(e1 == e2);
}

/**
 * @test Opérateur == avec emails différents
 */
TEST_F(EmailTest, EqualityOperatorFalse) {
    Email e1("test1@example.com");
    Email e2("test2@example.com");
    EXPECT_FALSE(e1 == e2);
}

/**
 * @test Opérateur != avec emails différents
 */
TEST_F(EmailTest, InequalityOperatorTrue) {
    Email e1("user1@example.com");
    Email e2("user2@example.com");
    EXPECT_TRUE(e1 != e2);
}

/**
 * @test Opérateur != avec emails égaux
 */
TEST_F(EmailTest, InequalityOperatorFalse) {
    Email e1("test@example.com");
    Email e2("test@example.com");
    EXPECT_FALSE(e1 != e2);
}

/**
 * @test Emails sensibles à la casse
 */
TEST_F(EmailTest, EmailsAreCaseSensitive) {
    Email e1("Test@example.com");
    Email e2("test@example.com");
    // Note: selon l'implémentation, les emails peuvent être case-sensitive
    // Dans la RFC, la partie locale EST case-sensitive, le domaine NE L'EST PAS
    // L'implémentation actuelle les traite comme différents
    EXPECT_TRUE(e1 != e2);
}

// ============================================================================
// Tests de Getter
// ============================================================================

/**
 * @test value() retourne la valeur correcte
 */
TEST_F(EmailTest, ValueReturnsCorrectEmail) {
    Email email("myemail@domain.org");
    EXPECT_EQ(email.value(), "myemail@domain.org");
}

// ============================================================================
// Tests de Cas Spéciaux
// ============================================================================

/**
 * @test Email avec TLD long
 */
TEST_F(EmailTest, CreateWithLongTLD) {
    ASSERT_NO_THROW({
        Email email("user@example.museum");
    });
}

/**
 * @test Email avec chiffres dans le domaine
 */
TEST_F(EmailTest, CreateWithNumbersInDomain) {
    ASSERT_NO_THROW({
        Email email("user@123.com");
    });
}

/**
 * @test Email avec tiret dans le domaine
 */
TEST_F(EmailTest, CreateWithHyphenInDomain) {
    ASSERT_NO_THROW({
        Email email("user@my-domain.com");
    });
}
