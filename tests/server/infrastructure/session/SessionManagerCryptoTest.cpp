/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour la génération cryptographique des tokens de session
**
** Ces tests valident que le CSPRNG (OpenSSL RAND_bytes) génère des tokens:
** - Uniques (pas de collisions sur 10000 itérations)
** - Avec bonne entropie (~50% de bits à 1)
** - Non triviaux (pas all-zeros ou all-ones)
*/

#include <gtest/gtest.h>
#include "infrastructure/session/SessionManager.hpp"
#include <set>
#include <cmath>
#include <bitset>

using namespace infrastructure::session;

/**
 * @brief Suite de tests pour la génération cryptographique des tokens
 *
 * SessionManager utilise OpenSSL RAND_bytes() pour générer des tokens
 * de session cryptographiquement sécurisés (32 bytes = 256 bits).
 *
 * Ces tests vérifient les propriétés statistiques attendues d'un CSPRNG.
 */
class SessionManagerCryptoTest : public ::testing::Test {
protected:
    SessionManager _sessionManager;

    void SetUp() override {}
    void TearDown() override {}

    // Helper: compte le nombre de bits à 1 dans un token
    static int countBitsSet(const SessionToken& token) {
        int count = 0;
        for (size_t i = 0; i < TOKEN_SIZE; ++i) {
            count += std::bitset<8>(token.bytes[i]).count();
        }
        return count;
    }

    // Helper: vérifie si un token est trivial (all-zeros ou all-ones)
    static bool isTrivialToken(const SessionToken& token) {
        bool allZero = true;
        bool allOnes = true;
        for (size_t i = 0; i < TOKEN_SIZE; ++i) {
            if (token.bytes[i] != 0x00) allZero = false;
            if (token.bytes[i] != 0xFF) allOnes = false;
        }
        return allZero || allOnes;
    }
};

// ============================================================================
// Tests d'Unicité
// ============================================================================

/**
 * @test Les tokens générés sont uniques sur 1000 itérations
 *
 * Avec 256 bits d'entropie, la probabilité de collision est ~2^-128,
 * donc 1000 tokens devraient tous être uniques.
 */
TEST_F(SessionManagerCryptoTest, TokensAreUnique) {
    std::set<std::string> tokens;
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        std::string email = "user" + std::to_string(i) + "@test.com";
        std::string displayName = "User" + std::to_string(i);

        auto result = _sessionManager.createSession(email, displayName);
        ASSERT_TRUE(result.has_value())
            << "Failed to create session at iteration " << i;

        std::string hex = result->token.toHex();
        ASSERT_EQ(tokens.count(hex), 0)
            << "Token collision detected at iteration " << i
            << ": " << hex;

        tokens.insert(hex);

        // Nettoyer la session pour éviter "already has session"
        _sessionManager.removeSession(email);
    }

    EXPECT_EQ(tokens.size(), static_cast<size_t>(iterations))
        << "Not all tokens were unique";
}

/**
 * @test Tokens uniques même avec création/suppression rapide
 *
 * Vérifie que le CSPRNG ne réutilise pas de seed même sous charge.
 */
TEST_F(SessionManagerCryptoTest, TokensUniqueUnderRapidCreation) {
    std::set<std::string> tokens;
    const int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        std::string email = "rapid" + std::to_string(i) + "@test.com";

        auto result = _sessionManager.createSession(email, "Rapid" + std::to_string(i));
        ASSERT_TRUE(result.has_value());

        tokens.insert(result->token.toHex());
        _sessionManager.removeSession(email);
    }

    EXPECT_EQ(tokens.size(), static_cast<size_t>(iterations));
}

// ============================================================================
// Tests d'Entropie
// ============================================================================

/**
 * @test Les tokens ont une bonne entropie (~50% de bits à 1)
 *
 * Un bon CSPRNG devrait produire en moyenne 50% de bits à 1.
 * Avec 256 bits total, on attend ~128 bits à 1, avec tolérance de ±3σ.
 *
 * Formule: σ = sqrt(n/4) pour n bits, donc σ ≈ 8 pour 256 bits.
 * 3σ ≈ 24, donc on accepte [104, 152].
 */
TEST_F(SessionManagerCryptoTest, TokenEntropy) {
    auto result = _sessionManager.createSession("entropy@test.com", "EntropyTest");
    ASSERT_TRUE(result.has_value());

    int bitsSet = countBitsSet(result->token);
    int totalBits = TOKEN_SIZE * 8;  // 256
    int expected = totalBits / 2;    // 128
    int tolerance = static_cast<int>(3 * std::sqrt(totalBits / 4.0));  // ~24

    EXPECT_GE(bitsSet, expected - tolerance)
        << "Too few 1-bits: " << bitsSet << " (expected ~" << expected
        << ", min acceptable: " << (expected - tolerance) << ")";

    EXPECT_LE(bitsSet, expected + tolerance)
        << "Too many 1-bits: " << bitsSet << " (expected ~" << expected
        << ", max acceptable: " << (expected + tolerance) << ")";

    _sessionManager.removeSession("entropy@test.com");
}

/**
 * @test Entropie moyenne sur plusieurs tokens
 *
 * Vérifie que la moyenne des bits à 1 sur 100 tokens est proche de 50%.
 */
TEST_F(SessionManagerCryptoTest, AverageEntropyOverMultipleTokens) {
    const int iterations = 100;
    int totalBitsSet = 0;

    for (int i = 0; i < iterations; ++i) {
        std::string email = "avg" + std::to_string(i) + "@test.com";
        auto result = _sessionManager.createSession(email, "Avg" + std::to_string(i));
        ASSERT_TRUE(result.has_value());

        totalBitsSet += countBitsSet(result->token);
        _sessionManager.removeSession(email);
    }

    double averageBitsSet = static_cast<double>(totalBitsSet) / iterations;
    double expectedAverage = (TOKEN_SIZE * 8) / 2.0;  // 128.0

    // Tolérance: ±5% de la valeur attendue
    double tolerance = expectedAverage * 0.05;  // ~6.4

    EXPECT_NEAR(averageBitsSet, expectedAverage, tolerance)
        << "Average bits set (" << averageBitsSet
        << ") deviates too much from expected (" << expectedAverage << ")";
}

// ============================================================================
// Tests de Non-Trivialité
// ============================================================================

/**
 * @test Les tokens ne sont pas triviaux (all-zeros ou all-ones)
 *
 * Un CSPRNG ne devrait jamais générer un token de 32 bytes tous à 0x00
 * ou tous à 0xFF. La probabilité est 2^-256, donc impossible en pratique.
 */
TEST_F(SessionManagerCryptoTest, TokenNotTrivial) {
    auto result = _sessionManager.createSession("trivial@test.com", "TrivialTest");
    ASSERT_TRUE(result.has_value());

    EXPECT_FALSE(isTrivialToken(result->token))
        << "Token is trivial (all zeros or all ones): " << result->token.toHex();

    _sessionManager.removeSession("trivial@test.com");
}

/**
 * @test Aucun token trivial sur 1000 générations
 */
TEST_F(SessionManagerCryptoTest, NoTrivialTokensGenerated) {
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        std::string email = "notrivial" + std::to_string(i) + "@test.com";
        auto result = _sessionManager.createSession(email, "NoTrivial" + std::to_string(i));
        ASSERT_TRUE(result.has_value());

        EXPECT_FALSE(isTrivialToken(result->token))
            << "Trivial token generated at iteration " << i;

        _sessionManager.removeSession(email);
    }
}

// ============================================================================
// Tests de Distribution
// ============================================================================

/**
 * @test Distribution uniforme des octets
 *
 * Vérifie que chaque valeur d'octet (0-255) apparaît avec une fréquence
 * raisonnable sur un grand nombre de tokens.
 */
TEST_F(SessionManagerCryptoTest, ByteDistribution) {
    const int iterations = 500;
    std::array<int, 256> byteFrequency{};

    for (int i = 0; i < iterations; ++i) {
        std::string email = "dist" + std::to_string(i) + "@test.com";
        auto result = _sessionManager.createSession(email, "Dist" + std::to_string(i));
        ASSERT_TRUE(result.has_value());

        for (size_t j = 0; j < TOKEN_SIZE; ++j) {
            byteFrequency[result->token.bytes[j]]++;
        }

        _sessionManager.removeSession(email);
    }

    // Avec 500 tokens * 32 bytes = 16000 octets
    // Chaque valeur 0-255 devrait apparaître ~62.5 fois en moyenne
    int totalBytes = iterations * TOKEN_SIZE;
    double expectedFreq = static_cast<double>(totalBytes) / 256.0;

    // Vérifier qu'aucune valeur n'est sur-représentée (>3x attendu)
    // ou sous-représentée (<0.2x attendu)
    int overRepresented = 0;
    int underRepresented = 0;

    for (int i = 0; i < 256; ++i) {
        if (byteFrequency[i] > expectedFreq * 3.0) overRepresented++;
        if (byteFrequency[i] < expectedFreq * 0.2) underRepresented++;
    }

    EXPECT_LT(overRepresented, 3)
        << "Too many over-represented byte values: " << overRepresented;
    EXPECT_LT(underRepresented, 5)
        << "Too many under-represented byte values: " << underRepresented;
}

// ============================================================================
// Tests de Format du Token
// ============================================================================

/**
 * @test Le token a la bonne taille (32 bytes = 256 bits)
 */
TEST_F(SessionManagerCryptoTest, TokenHasCorrectSize) {
    EXPECT_EQ(TOKEN_SIZE, 32u)
        << "TOKEN_SIZE should be 32 bytes (256 bits) for security";
}

/**
 * @test Conversion toHex() produit une string de 64 caractères
 */
TEST_F(SessionManagerCryptoTest, TokenHexConversion) {
    auto result = _sessionManager.createSession("hex@test.com", "HexTest");
    ASSERT_TRUE(result.has_value());

    std::string hex = result->token.toHex();

    EXPECT_EQ(hex.size(), TOKEN_SIZE * 2)
        << "Hex string should be 64 characters (2 per byte)";

    // Vérifier que tous les caractères sont hexadécimaux
    for (char c : hex) {
        bool isHex = (c >= '0' && c <= '9') ||
                     (c >= 'a' && c <= 'f') ||
                     (c >= 'A' && c <= 'F');
        EXPECT_TRUE(isHex) << "Invalid hex character: " << c;
    }

    _sessionManager.removeSession("hex@test.com");
}

/**
 * @test Conversion roundtrip SessionToken -> Hex -> SessionToken
 */
TEST_F(SessionManagerCryptoTest, TokenHexRoundtrip) {
    auto result = _sessionManager.createSession("roundtrip@test.com", "RoundtripTest");
    ASSERT_TRUE(result.has_value());

    std::string hex = result->token.toHex();

    // Reconvertir depuis hex
    auto parsed = SessionToken::fromHex(hex);
    ASSERT_TRUE(parsed.has_value())
        << "Failed to parse hex string back to token";

    // Comparer byte par byte
    for (size_t i = 0; i < TOKEN_SIZE; ++i) {
        EXPECT_EQ(result->token.bytes[i], parsed->bytes[i])
            << "Mismatch at byte " << i;
    }

    _sessionManager.removeSession("roundtrip@test.com");
}
