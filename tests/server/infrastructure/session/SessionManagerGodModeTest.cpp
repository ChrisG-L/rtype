/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour le système GodMode (fonctionnalité cachée)
**
** Ces tests valident le comportement du GodMode:
** - Toggle on/off via SessionManager
** - Persistence de l'état entre sessions
** - Callback de notification pour sync temps réel
** - Isolation entre joueurs (chaque joueur a son propre état)
*/

#include <gtest/gtest.h>
#include "infrastructure/session/SessionManager.hpp"
#include <atomic>
#include <thread>
#include <chrono>

using namespace infrastructure::session;

/**
 * @brief Suite de tests pour le système GodMode
 *
 * GodMode est une fonctionnalité cachée permettant à un joueur
 * d'être invincible (pas de perte de HP) sans que les autres
 * joueurs ne le sachent.
 */
class SessionManagerGodModeTest : public ::testing::Test {
protected:
    SessionManager _sessionManager;

    void SetUp() override {}
    void TearDown() override {}

    // Helper: crée une session et retourne le token
    std::optional<SessionToken> createTestSession(const std::string& email,
                                                   const std::string& displayName) {
        auto result = _sessionManager.createSession(email, displayName);
        if (result.has_value()) {
            return result->token;
        }
        return std::nullopt;
    }
};

// ============================================================================
// Tests de Base - État Initial
// ============================================================================

/**
 * @test GodMode est désactivé par défaut à la création de session
 */
TEST_F(SessionManagerGodModeTest, GodModeDisabledByDefault) {
    auto token = createTestSession("player@test.com", "Player");
    ASSERT_TRUE(token.has_value());

    // Vérifier que GodMode est désactivé par défaut
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("player@test.com"));

    _sessionManager.removeSession("player@test.com");
}

/**
 * @test Session inexistante retourne false pour isGodModeEnabled
 */
TEST_F(SessionManagerGodModeTest, GodModeCheckOnNonExistentSession) {
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("nonexistent@test.com"));
}

// ============================================================================
// Tests de Toggle
// ============================================================================

/**
 * @test Toggle GodMode active puis désactive
 */
TEST_F(SessionManagerGodModeTest, ToggleGodModeOnOff) {
    auto token = createTestSession("toggle@test.com", "TogglePlayer");
    ASSERT_TRUE(token.has_value());

    // État initial: désactivé
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("toggle@test.com"));

    // Premier toggle: active
    bool newState = _sessionManager.toggleGodMode("toggle@test.com");
    EXPECT_TRUE(newState);
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("toggle@test.com"));

    // Deuxième toggle: désactive
    newState = _sessionManager.toggleGodMode("toggle@test.com");
    EXPECT_FALSE(newState);
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("toggle@test.com"));

    _sessionManager.removeSession("toggle@test.com");
}

/**
 * @test Toggle sur session inexistante retourne false
 */
TEST_F(SessionManagerGodModeTest, ToggleOnNonExistentSession) {
    bool result = _sessionManager.toggleGodMode("nonexistent@test.com");
    EXPECT_FALSE(result);
}

/**
 * @test Plusieurs toggles successifs alternent correctement
 */
TEST_F(SessionManagerGodModeTest, MultipleToggles) {
    auto token = createTestSession("multi@test.com", "MultiToggle");
    ASSERT_TRUE(token.has_value());

    for (int i = 0; i < 10; ++i) {
        bool expectedState = (i % 2 == 0);  // true pour i=0,2,4...
        bool newState = _sessionManager.toggleGodMode("multi@test.com");
        EXPECT_EQ(newState, expectedState)
            << "Toggle iteration " << i << " failed";
    }

    _sessionManager.removeSession("multi@test.com");
}

// ============================================================================
// Tests de setGodMode()
// ============================================================================

/**
 * @test setGodMode() active explicitement GodMode
 */
TEST_F(SessionManagerGodModeTest, SetGodModeEnabled) {
    auto token = createTestSession("set@test.com", "SetPlayer");
    ASSERT_TRUE(token.has_value());

    _sessionManager.setGodMode("set@test.com", true);
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("set@test.com"));

    _sessionManager.removeSession("set@test.com");
}

/**
 * @test setGodMode() désactive explicitement GodMode
 */
TEST_F(SessionManagerGodModeTest, SetGodModeDisabled) {
    auto token = createTestSession("setoff@test.com", "SetOffPlayer");
    ASSERT_TRUE(token.has_value());

    // Active d'abord
    _sessionManager.setGodMode("setoff@test.com", true);
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("setoff@test.com"));

    // Puis désactive
    _sessionManager.setGodMode("setoff@test.com", false);
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("setoff@test.com"));

    _sessionManager.removeSession("setoff@test.com");
}

/**
 * @test setGodMode() sur session inexistante ne plante pas
 */
TEST_F(SessionManagerGodModeTest, SetGodModeOnNonExistentSession) {
    // Ne doit pas planter
    _sessionManager.setGodMode("nonexistent@test.com", true);
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("nonexistent@test.com"));
}

// ============================================================================
// Tests d'Isolation entre Joueurs
// ============================================================================

/**
 * @test GodMode est isolé par joueur
 */
TEST_F(SessionManagerGodModeTest, GodModeIsolatedPerPlayer) {
    auto token1 = createTestSession("player1@test.com", "Player1");
    auto token2 = createTestSession("player2@test.com", "Player2");
    auto token3 = createTestSession("player3@test.com", "Player3");

    ASSERT_TRUE(token1.has_value());
    ASSERT_TRUE(token2.has_value());
    ASSERT_TRUE(token3.has_value());

    // Active GodMode uniquement pour player1
    _sessionManager.setGodMode("player1@test.com", true);

    // Vérifier isolation
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("player1@test.com"));
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("player2@test.com"));
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("player3@test.com"));

    // Active pour player3 aussi
    _sessionManager.setGodMode("player3@test.com", true);

    EXPECT_TRUE(_sessionManager.isGodModeEnabled("player1@test.com"));
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("player2@test.com"));
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("player3@test.com"));

    _sessionManager.removeSession("player1@test.com");
    _sessionManager.removeSession("player2@test.com");
    _sessionManager.removeSession("player3@test.com");
}

// ============================================================================
// Tests de Callback GodModeChanged
// ============================================================================

/**
 * @test Callback est appelé lors du toggle avec playerId assigné
 */
TEST_F(SessionManagerGodModeTest, CallbackCalledOnToggle) {
    std::atomic<int> callbackCount{0};
    uint8_t lastPlayerId = 0;
    std::string lastRoomCode;
    bool lastEnabled = false;

    // Enregistrer le callback
    _sessionManager.setGodModeChangedCallback(
        [&](uint8_t playerId, const std::string& roomCode, bool enabled) {
            callbackCount++;
            lastPlayerId = playerId;
            lastRoomCode = roomCode;
            lastEnabled = enabled;
        }
    );

    // Créer une session
    auto token = createTestSession("callback@test.com", "CallbackPlayer");
    ASSERT_TRUE(token.has_value());

    // Bind UDP et assigner playerId
    auto result = _sessionManager.validateAndBindUDP(*token, "192.168.1.1:5000");
    ASSERT_TRUE(result.has_value());

    _sessionManager.assignPlayerId("192.168.1.1:5000", 1);
    _sessionManager.setRoomCode("callback@test.com", "ROOM123");

    // Toggle GodMode - devrait déclencher le callback
    _sessionManager.toggleGodMode("callback@test.com");

    // Vérifier le callback
    EXPECT_EQ(callbackCount.load(), 1);
    EXPECT_EQ(lastPlayerId, 1);
    EXPECT_EQ(lastRoomCode, "ROOM123");
    EXPECT_TRUE(lastEnabled);

    // Toggle à nouveau
    _sessionManager.toggleGodMode("callback@test.com");

    EXPECT_EQ(callbackCount.load(), 2);
    EXPECT_FALSE(lastEnabled);

    _sessionManager.removeSession("callback@test.com");
}

/**
 * @test Callback non appelé si pas de playerId assigné
 */
TEST_F(SessionManagerGodModeTest, CallbackNotCalledWithoutPlayerId) {
    std::atomic<int> callbackCount{0};

    _sessionManager.setGodModeChangedCallback(
        [&](uint8_t, const std::string&, bool) {
            callbackCount++;
        }
    );

    auto token = createTestSession("nopid@test.com", "NoPlayerIdPlayer");
    ASSERT_TRUE(token.has_value());

    // Toggle sans avoir assigné de playerId
    _sessionManager.toggleGodMode("nopid@test.com");

    // Callback ne devrait PAS être appelé
    EXPECT_EQ(callbackCount.load(), 0);

    _sessionManager.removeSession("nopid@test.com");
}

/**
 * @test Callback non appelé si pas de roomCode
 */
TEST_F(SessionManagerGodModeTest, CallbackNotCalledWithoutRoomCode) {
    std::atomic<int> callbackCount{0};

    _sessionManager.setGodModeChangedCallback(
        [&](uint8_t, const std::string&, bool) {
            callbackCount++;
        }
    );

    auto token = createTestSession("noroom@test.com", "NoRoomPlayer");
    ASSERT_TRUE(token.has_value());

    auto result = _sessionManager.validateAndBindUDP(*token, "192.168.1.2:5000");
    ASSERT_TRUE(result.has_value());

    _sessionManager.assignPlayerId("192.168.1.2:5000", 2);
    // Pas de setRoomCode()

    _sessionManager.toggleGodMode("noroom@test.com");

    // Callback ne devrait PAS être appelé (roomCode vide)
    EXPECT_EQ(callbackCount.load(), 0);

    _sessionManager.removeSession("noroom@test.com");
}

// ============================================================================
// Tests isPlayerInGodMode (par playerId)
// ============================================================================

/**
 * @test isPlayerInGodMode retourne l'état correct par playerId
 */
TEST_F(SessionManagerGodModeTest, IsPlayerInGodModeByPlayerId) {
    auto token = createTestSession("bypid@test.com", "ByPlayerIdPlayer");
    ASSERT_TRUE(token.has_value());

    auto result = _sessionManager.validateAndBindUDP(*token, "192.168.1.3:5000");
    ASSERT_TRUE(result.has_value());

    _sessionManager.assignPlayerId("192.168.1.3:5000", 3);

    // Pas en GodMode initialement
    EXPECT_FALSE(_sessionManager.isPlayerInGodMode(3));

    // Active GodMode
    _sessionManager.setGodMode("bypid@test.com", true);

    // Maintenant en GodMode
    EXPECT_TRUE(_sessionManager.isPlayerInGodMode(3));

    _sessionManager.removeSession("bypid@test.com");
}

/**
 * @test isPlayerInGodMode retourne false pour playerId inexistant
 */
TEST_F(SessionManagerGodModeTest, IsPlayerInGodModeNonExistentPlayerId) {
    EXPECT_FALSE(_sessionManager.isPlayerInGodMode(99));
}

// ============================================================================
// Tests de Persistance de l'État dans la Session
// ============================================================================

/**
 * @test L'état GodMode est préservé dans getSessionByEmail
 */
TEST_F(SessionManagerGodModeTest, GodModePreservedInSessionCopy) {
    auto token = createTestSession("persist@test.com", "PersistPlayer");
    ASSERT_TRUE(token.has_value());

    _sessionManager.setGodMode("persist@test.com", true);

    auto sessionOpt = _sessionManager.getSessionByEmail("persist@test.com");
    ASSERT_TRUE(sessionOpt.has_value());
    EXPECT_TRUE(sessionOpt->godMode);

    _sessionManager.removeSession("persist@test.com");
}

/**
 * @test L'état GodMode est préservé dans getSessionByEndpoint
 */
TEST_F(SessionManagerGodModeTest, GodModePreservedInSessionByEndpoint) {
    auto token = createTestSession("endpoint@test.com", "EndpointPlayer");
    ASSERT_TRUE(token.has_value());

    auto result = _sessionManager.validateAndBindUDP(*token, "192.168.1.4:5000");
    ASSERT_TRUE(result.has_value());

    _sessionManager.setGodMode("endpoint@test.com", true);

    auto sessionOpt = _sessionManager.getSessionByEndpoint("192.168.1.4:5000");
    ASSERT_TRUE(sessionOpt.has_value());
    EXPECT_TRUE(sessionOpt->godMode);

    _sessionManager.removeSession("endpoint@test.com");
}

// ============================================================================
// Tests de Thread Safety
// ============================================================================

/**
 * @test Toggles concurrents ne causent pas de data race
 */
TEST_F(SessionManagerGodModeTest, ConcurrentTogglesAreSafe) {
    auto token = createTestSession("concurrent@test.com", "ConcurrentPlayer");
    ASSERT_TRUE(token.has_value());

    std::atomic<int> toggleCount{0};
    const int numThreads = 10;
    const int togglesPerThread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < togglesPerThread; ++i) {
                _sessionManager.toggleGodMode("concurrent@test.com");
                toggleCount++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(toggleCount.load(), numThreads * togglesPerThread);

    // État final devrait être cohérent (après 1000 toggles pairs → désactivé)
    bool finalState = _sessionManager.isGodModeEnabled("concurrent@test.com");
    // 1000 toggles = état initial (false)
    EXPECT_FALSE(finalState);

    _sessionManager.removeSession("concurrent@test.com");
}

/**
 * @test Lectures et écritures concurrentes sont sûres
 */
TEST_F(SessionManagerGodModeTest, ConcurrentReadWriteAreSafe) {
    auto token = createTestSession("readwrite@test.com", "ReadWritePlayer");
    ASSERT_TRUE(token.has_value());

    std::atomic<bool> running{true};
    std::atomic<int> readCount{0};
    std::atomic<int> writeCount{0};

    // Thread d'écriture
    std::thread writer([&]() {
        while (running) {
            _sessionManager.toggleGodMode("readwrite@test.com");
            writeCount++;
            std::this_thread::yield();
        }
    });

    // Threads de lecture
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back([&]() {
            while (running) {
                _sessionManager.isGodModeEnabled("readwrite@test.com");
                readCount++;
                std::this_thread::yield();
            }
        });
    }

    // Laisser tourner un moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    for (auto& reader : readers) {
        reader.join();
    }

    EXPECT_GT(readCount.load(), 0);
    EXPECT_GT(writeCount.load(), 0);

    _sessionManager.removeSession("readwrite@test.com");
}

// ============================================================================
// Tests de Nettoyage
// ============================================================================

/**
 * @test GodMode est nettoyé avec la session
 */
TEST_F(SessionManagerGodModeTest, GodModeCleanedOnSessionRemove) {
    auto token = createTestSession("cleanup@test.com", "CleanupPlayer");
    ASSERT_TRUE(token.has_value());

    _sessionManager.setGodMode("cleanup@test.com", true);
    EXPECT_TRUE(_sessionManager.isGodModeEnabled("cleanup@test.com"));

    _sessionManager.removeSession("cleanup@test.com");

    // Après suppression, isGodModeEnabled retourne false
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("cleanup@test.com"));
}

/**
 * @test Recréer une session réinitialise GodMode à false
 */
TEST_F(SessionManagerGodModeTest, NewSessionHasFreshGodModeState) {
    // Première session avec GodMode activé
    auto token1 = createTestSession("fresh@test.com", "FreshPlayer");
    ASSERT_TRUE(token1.has_value());
    _sessionManager.setGodMode("fresh@test.com", true);
    _sessionManager.removeSession("fresh@test.com");

    // Nouvelle session pour le même email
    auto token2 = createTestSession("fresh@test.com", "FreshPlayer");
    ASSERT_TRUE(token2.has_value());

    // GodMode devrait être désactivé (état frais)
    EXPECT_FALSE(_sessionManager.isGodModeEnabled("fresh@test.com"));

    _sessionManager.removeSession("fresh@test.com");
}
