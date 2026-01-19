/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PauseSystemTest - Tests for Pause System
*/

#include <gtest/gtest.h>
#include "Protocol.hpp"
#include "infrastructure/game/GameWorld.hpp"

// ============================================================================
// Protocol Tests - Pause Request
// ============================================================================

class PauseRequestTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PauseRequestTest, SerializationRoundtrip_WantsPause) {
    PauseRequest original{.wantsPause = 1};

    uint8_t buffer[PauseRequest::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PauseRequest::from_bytes(buffer, PauseRequest::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->wantsPause, 1);
}

TEST_F(PauseRequestTest, SerializationRoundtrip_WantsResume) {
    PauseRequest original{.wantsPause = 0};

    uint8_t buffer[PauseRequest::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PauseRequest::from_bytes(buffer, PauseRequest::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->wantsPause, 0);
}

TEST_F(PauseRequestTest, InvalidBufferTooSmall) {
    uint8_t buffer[0];
    auto parsed = PauseRequest::from_bytes(buffer, 0);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(PauseRequestTest, NullBuffer) {
    auto parsed = PauseRequest::from_bytes(nullptr, PauseRequest::WIRE_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// Protocol Tests - Pause State Sync
// ============================================================================

class PauseStateSyncTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PauseStateSyncTest, SerializationRoundtrip_Paused) {
    PauseStateSync original{
        .isPaused = 1,
        .pauseVoterCount = 2,
        .totalPlayerCount = 4
    };

    uint8_t buffer[PauseStateSync::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PauseStateSync::from_bytes(buffer, PauseStateSync::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->isPaused, 1);
    EXPECT_EQ(parsed->pauseVoterCount, 2);
    EXPECT_EQ(parsed->totalPlayerCount, 4);
}

TEST_F(PauseStateSyncTest, SerializationRoundtrip_NotPaused) {
    PauseStateSync original{
        .isPaused = 0,
        .pauseVoterCount = 1,
        .totalPlayerCount = 3
    };

    uint8_t buffer[PauseStateSync::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PauseStateSync::from_bytes(buffer, PauseStateSync::WIRE_SIZE);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->isPaused, 0);
    EXPECT_EQ(parsed->pauseVoterCount, 1);
    EXPECT_EQ(parsed->totalPlayerCount, 3);
}

TEST_F(PauseStateSyncTest, InvalidBufferTooSmall) {
    uint8_t buffer[2];  // Need 3 bytes
    auto parsed = PauseStateSync::from_bytes(buffer, 2);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(PauseStateSyncTest, NullBuffer) {
    auto parsed = PauseStateSync::from_bytes(nullptr, PauseStateSync::WIRE_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(PauseStateSyncTest, WireSize) {
    EXPECT_EQ(PauseStateSync::WIRE_SIZE, 3);
    EXPECT_EQ(PauseRequest::WIRE_SIZE, 1);
}

// ============================================================================
// GameWorld Tests - Pause System Logic
// ============================================================================

class GameWorldPauseTest : public ::testing::Test {
protected:
    boost::asio::io_context io_ctx;
    std::unique_ptr<infrastructure::game::GameWorld> gameWorld;

    void SetUp() override {
        gameWorld = std::make_unique<infrastructure::game::GameWorld>(io_ctx);
    }

    void TearDown() override {
        gameWorld.reset();
    }

    // Helper to add a player and get their ID
    uint8_t addPlayer() {
        boost::asio::ip::udp::endpoint ep(
            boost::asio::ip::make_address("127.0.0.1"),
            static_cast<unsigned short>(12345 + gameWorld->getPlayerCount())
        );
        auto idOpt = gameWorld->addPlayer(ep);
        EXPECT_TRUE(idOpt.has_value());
        return idOpt.value_or(0);
    }
};

TEST_F(GameWorldPauseTest, NoPlayers_NotPaused) {
    EXPECT_FALSE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 0);
}

TEST_F(GameWorldPauseTest, SoloMode_PauseOnVote) {
    uint8_t playerId = addPlayer();

    // Initially not paused
    EXPECT_FALSE(gameWorld->isPaused());

    // Player votes pause
    gameWorld->setPauseVote(playerId, true);
    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, SoloMode_UnpauseOnVoteRemoval) {
    uint8_t playerId = addPlayer();

    gameWorld->setPauseVote(playerId, true);
    EXPECT_TRUE(gameWorld->isPaused());

    gameWorld->setPauseVote(playerId, false);
    EXPECT_FALSE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, MultiPlayer_NotPausedWithPartialVotes) {
    uint8_t p1 = addPlayer();
    [[maybe_unused]] uint8_t p2 = addPlayer();

    // Only one player votes
    gameWorld->setPauseVote(p1, true);

    EXPECT_FALSE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 2);
}

TEST_F(GameWorldPauseTest, MultiPlayer_PausedWhenAllVote) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);

    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 2);
    EXPECT_EQ(total, 2);
}

TEST_F(GameWorldPauseTest, MultiPlayer_UnpauseWhenOneRemovesVote) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // One player removes vote
    gameWorld->setPauseVote(p1, false);
    EXPECT_FALSE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 2);
}

TEST_F(GameWorldPauseTest, ThreePlayers_RequiresAllVotes) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();
    uint8_t p3 = addPlayer();

    // Two out of three - not paused
    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    EXPECT_FALSE(gameWorld->isPaused());

    auto state1 = gameWorld->getPauseState();
    EXPECT_EQ(std::get<1>(state1), 2);
    EXPECT_EQ(std::get<2>(state1), 3);

    // All three - paused
    gameWorld->setPauseVote(p3, true);
    EXPECT_TRUE(gameWorld->isPaused());
}

TEST_F(GameWorldPauseTest, VoteIgnoredForNonExistentPlayer) {
    [[maybe_unused]] uint8_t p1 = addPlayer();

    // Try to set pause for non-existent player
    gameWorld->setPauseVote(99, true);

    // Should not affect pause state
    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, PlayerLeave_ClearsPauseVote) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // Player 1 leaves
    gameWorld->removePlayer(p1);

    // Now only p2 remains, and their vote should keep the game paused (solo mode)
    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, PlayerWithVoteLeaves_UnpausesIfNeeded) {
    uint8_t p1 = addPlayer();
    [[maybe_unused]] uint8_t p2 = addPlayer();

    // Only p1 votes
    gameWorld->setPauseVote(p1, true);
    EXPECT_FALSE(gameWorld->isPaused());  // 1/2

    // p1 leaves - their vote is cleared
    gameWorld->removePlayer(p1);

    // Now only p2 remains with no vote
    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, DoubleVote_NoEffect) {
    uint8_t p1 = addPlayer();

    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p1, true);  // Double vote

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 1);  // Should still be 1, not 2
}

TEST_F(GameWorldPauseTest, DoubleUnvote_NoEffect) {
    uint8_t p1 = addPlayer();

    gameWorld->setPauseVote(p1, false);  // Already not voted
    gameWorld->setPauseVote(p1, false);  // Double unvote

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
}

TEST_F(GameWorldPauseTest, FourPlayers_MaxPlayers) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();
    uint8_t p3 = addPlayer();
    uint8_t p4 = addPlayer();

    // 3 out of 4 - not paused
    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    gameWorld->setPauseVote(p3, true);
    EXPECT_FALSE(gameWorld->isPaused());

    auto state1 = gameWorld->getPauseState();
    EXPECT_EQ(std::get<1>(state1), 3);
    EXPECT_EQ(std::get<2>(state1), 4);

    // All 4 - paused
    gameWorld->setPauseVote(p4, true);
    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 4);
    EXPECT_EQ(total, 4);
}

TEST_F(GameWorldPauseTest, NewPlayerJoins_WhilePaused) {
    uint8_t p1 = addPlayer();

    // Solo player pauses
    gameWorld->setPauseVote(p1, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // New player joins
    uint8_t p2 = addPlayer();

    // Now 2 players, but only p1 voted - should unpause
    EXPECT_FALSE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 2);

    // p2 also votes - now paused again
    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());
}

TEST_F(GameWorldPauseTest, AllPlayersLeave_ResetsPauseState) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // All players leave
    gameWorld->removePlayer(p1);
    gameWorld->removePlayer(p2);

    // Back to initial state
    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 0);
}

TEST_F(GameWorldPauseTest, VoteToggle_RapidToggle) {
    uint8_t p1 = addPlayer();

    // Rapid toggling
    for (int i = 0; i < 10; ++i) {
        gameWorld->setPauseVote(p1, true);
        EXPECT_TRUE(gameWorld->isPaused());
        gameWorld->setPauseVote(p1, false);
        EXPECT_FALSE(gameWorld->isPaused());
    }

    // Final state: not paused
    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_FALSE(isPaused);
    EXPECT_EQ(voters, 0);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, MultiPlayer_VoteOrderDoesntMatter) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();
    uint8_t p3 = addPlayer();

    // Different vote orders should produce same result
    gameWorld->setPauseVote(p3, true);
    EXPECT_FALSE(gameWorld->isPaused());

    gameWorld->setPauseVote(p1, true);
    EXPECT_FALSE(gameWorld->isPaused());

    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // All voted, order was p3, p1, p2
    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 3);
    EXPECT_EQ(total, 3);
}

// ============================================================================
// Player Leave During Pause Tests
// ============================================================================

TEST_F(GameWorldPauseTest, SoloPlayer_CanStillPauseAfterOtherLeaves) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    // p1 votes, p2 doesn't
    gameWorld->setPauseVote(p1, true);
    EXPECT_FALSE(gameWorld->isPaused());  // 1/2

    // p2 leaves (simulating death + quit)
    gameWorld->removePlayer(p2);

    // Now p1 is solo and their vote should pause the game
    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 1);
}

TEST_F(GameWorldPauseTest, VoteRemovedOnLeave_UnpausesIfNeeded) {
    uint8_t p1 = addPlayer();
    uint8_t p2 = addPlayer();

    // Both vote, game paused
    gameWorld->setPauseVote(p1, true);
    gameWorld->setPauseVote(p2, true);
    EXPECT_TRUE(gameWorld->isPaused());

    // p1 leaves (their vote is removed)
    gameWorld->removePlayer(p1);

    // p2 is now solo with their vote - still paused
    EXPECT_TRUE(gameWorld->isPaused());

    auto [isPaused, voters, total] = gameWorld->getPauseState();
    EXPECT_TRUE(isPaused);
    EXPECT_EQ(voters, 1);
    EXPECT_EQ(total, 1);
}

// ============================================================================
// MessageType Enum Tests
// ============================================================================

TEST(MessageTypeTest, PauseMessageValues) {
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PauseRequest), 0x0430);
    EXPECT_EQ(static_cast<uint16_t>(MessageType::PauseStateSync), 0x0431);
}
