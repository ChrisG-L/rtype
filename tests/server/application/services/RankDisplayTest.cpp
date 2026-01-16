/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** RankDisplayTest - Tests pour l'affichage du rang et formatage du score
*/

#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════
// Helper functions (same logic as GameScene::renderGlobalRank)
// ═══════════════════════════════════════════════════════════════════

namespace {

std::string formatBestScore(uint32_t score) {
    if (score >= 1000000) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "BEST: %.1fM", score / 1000000.0f);
        return buf;
    } else if (score >= 1000) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "BEST: %.1fK", score / 1000.0f);
        return buf;
    } else {
        return "BEST: " + std::to_string(score);
    }
}

struct rgba {
    uint8_t r, g, b, a;
    bool operator==(const rgba& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

rgba getRankColor(uint32_t rank) {
    if (rank == 1) {
        return {255, 215, 0, 255};    // Gold for #1
    } else if (rank == 2) {
        return {192, 192, 192, 255};  // Silver for #2
    } else if (rank == 3) {
        return {205, 127, 50, 255};   // Bronze for #3
    } else if (rank <= 10) {
        return {100, 200, 255, 255};  // Light blue for top 10
    } else if (rank <= 50) {
        return {100, 255, 150, 255};  // Green for top 50
    } else {
        return {200, 200, 200, 255};  // Gray for others
    }
}

rgba getBestScoreColor(uint32_t currentScore, uint32_t bestScore) {
    if (currentScore > bestScore) {
        return {100, 255, 100, 255};  // Green - new record!
    } else {
        return {100, 200, 255, 255};  // Cyan - target to beat
    }
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════
// Score Formatting Tests
// ═══════════════════════════════════════════════════════════════════

class ScoreFormattingTest : public ::testing::Test {};

TEST_F(ScoreFormattingTest, FormatScore_LessThan1000_RawNumber) {
    EXPECT_EQ(formatBestScore(0), "BEST: 0");
    EXPECT_EQ(formatBestScore(1), "BEST: 1");
    EXPECT_EQ(formatBestScore(100), "BEST: 100");
    EXPECT_EQ(formatBestScore(999), "BEST: 999");
}

TEST_F(ScoreFormattingTest, FormatScore_Thousands_KFormat) {
    EXPECT_EQ(formatBestScore(1000), "BEST: 1.0K");
    EXPECT_EQ(formatBestScore(1500), "BEST: 1.5K");
    EXPECT_EQ(formatBestScore(10000), "BEST: 10.0K");
    EXPECT_EQ(formatBestScore(32600), "BEST: 32.6K");
    EXPECT_EQ(formatBestScore(999999), "BEST: 1000.0K");
}

TEST_F(ScoreFormattingTest, FormatScore_Millions_MFormat) {
    EXPECT_EQ(formatBestScore(1000000), "BEST: 1.0M");
    EXPECT_EQ(formatBestScore(1200000), "BEST: 1.2M");
    EXPECT_EQ(formatBestScore(10000000), "BEST: 10.0M");
    EXPECT_EQ(formatBestScore(999999999), "BEST: 1000.0M");
}

TEST_F(ScoreFormattingTest, FormatScore_EdgeCases) {
    // Boundary between raw and K
    EXPECT_EQ(formatBestScore(999), "BEST: 999");
    EXPECT_EQ(formatBestScore(1000), "BEST: 1.0K");

    // Boundary between K and M
    EXPECT_EQ(formatBestScore(999999), "BEST: 1000.0K");
    EXPECT_EQ(formatBestScore(1000000), "BEST: 1.0M");
}

// ═══════════════════════════════════════════════════════════════════
// Rank Color Tests
// ═══════════════════════════════════════════════════════════════════

class RankColorTest : public ::testing::Test {};

TEST_F(RankColorTest, TopThree_SpecialColors) {
    // Gold for #1
    rgba gold = getRankColor(1);
    EXPECT_EQ(gold.r, 255);
    EXPECT_EQ(gold.g, 215);
    EXPECT_EQ(gold.b, 0);

    // Silver for #2
    rgba silver = getRankColor(2);
    EXPECT_EQ(silver.r, 192);
    EXPECT_EQ(silver.g, 192);
    EXPECT_EQ(silver.b, 192);

    // Bronze for #3
    rgba bronze = getRankColor(3);
    EXPECT_EQ(bronze.r, 205);
    EXPECT_EQ(bronze.g, 127);
    EXPECT_EQ(bronze.b, 50);
}

TEST_F(RankColorTest, TopTen_LightBlue) {
    rgba expected = {100, 200, 255, 255};

    for (uint32_t rank = 4; rank <= 10; ++rank) {
        EXPECT_EQ(getRankColor(rank), expected) << "Rank " << rank << " should be light blue";
    }
}

TEST_F(RankColorTest, TopFifty_Green) {
    rgba expected = {100, 255, 150, 255};

    for (uint32_t rank = 11; rank <= 50; ++rank) {
        EXPECT_EQ(getRankColor(rank), expected) << "Rank " << rank << " should be green";
    }
}

TEST_F(RankColorTest, Others_Gray) {
    rgba expected = {200, 200, 200, 255};

    EXPECT_EQ(getRankColor(51), expected);
    EXPECT_EQ(getRankColor(100), expected);
    EXPECT_EQ(getRankColor(1000), expected);
    EXPECT_EQ(getRankColor(999999), expected);
}

// ═══════════════════════════════════════════════════════════════════
// Best Score Color Tests (New Record Detection)
// ═══════════════════════════════════════════════════════════════════

class BestScoreColorTest : public ::testing::Test {};

TEST_F(BestScoreColorTest, NewRecord_Green) {
    rgba green = {100, 255, 100, 255};

    // Current score exceeds best score -> green (new record!)
    EXPECT_EQ(getBestScoreColor(1001, 1000), green);
    EXPECT_EQ(getBestScoreColor(50000, 32600), green);
    EXPECT_EQ(getBestScoreColor(1, 0), green);
}

TEST_F(BestScoreColorTest, NotNewRecord_Cyan) {
    rgba cyan = {100, 200, 255, 255};

    // Current score equals or is less than best -> cyan (target to beat)
    EXPECT_EQ(getBestScoreColor(1000, 1000), cyan);  // Equal
    EXPECT_EQ(getBestScoreColor(500, 1000), cyan);   // Less
    EXPECT_EQ(getBestScoreColor(0, 0), cyan);        // Both zero
    EXPECT_EQ(getBestScoreColor(32600, 50000), cyan);
}

TEST_F(BestScoreColorTest, EdgeCase_JustBeating) {
    rgba green = {100, 255, 100, 255};
    rgba cyan = {100, 200, 255, 255};

    // Exactly 1 point above
    EXPECT_EQ(getBestScoreColor(1001, 1000), green);

    // Exactly equal (not new record)
    EXPECT_EQ(getBestScoreColor(1000, 1000), cyan);
}

// ═══════════════════════════════════════════════════════════════════
// Rank Update Timer Tests
// ═══════════════════════════════════════════════════════════════════

class RankUpdateTimerTest : public ::testing::Test {
protected:
    static constexpr float RANK_UPDATE_INTERVAL = 10.0f;

    bool shouldUpdateRank(float& timer, float deltaTime) {
        timer += deltaTime;
        if (timer >= RANK_UPDATE_INTERVAL) {
            timer = 0.0f;
            return true;
        }
        return false;
    }
};

TEST_F(RankUpdateTimerTest, NoUpdate_BeforeInterval) {
    float timer = 0.0f;

    // 9 seconds passed - no update yet
    EXPECT_FALSE(shouldUpdateRank(timer, 9.0f));
    EXPECT_FLOAT_EQ(timer, 9.0f);
}

TEST_F(RankUpdateTimerTest, Update_AtInterval) {
    float timer = 0.0f;

    // Exactly 10 seconds
    EXPECT_TRUE(shouldUpdateRank(timer, 10.0f));
    EXPECT_FLOAT_EQ(timer, 0.0f);  // Reset
}

TEST_F(RankUpdateTimerTest, Update_AfterInterval) {
    float timer = 0.0f;

    // 12 seconds - should trigger and reset
    EXPECT_TRUE(shouldUpdateRank(timer, 12.0f));
    EXPECT_FLOAT_EQ(timer, 0.0f);
}

TEST_F(RankUpdateTimerTest, MultipleUpdates) {
    float timer = 0.0f;

    // First 10 seconds
    EXPECT_TRUE(shouldUpdateRank(timer, 10.0f));
    EXPECT_FLOAT_EQ(timer, 0.0f);

    // Next 5 seconds - no update
    EXPECT_FALSE(shouldUpdateRank(timer, 5.0f));
    EXPECT_FLOAT_EQ(timer, 5.0f);

    // Another 5 seconds - update
    EXPECT_TRUE(shouldUpdateRank(timer, 5.0f));
    EXPECT_FLOAT_EQ(timer, 0.0f);
}

TEST_F(RankUpdateTimerTest, SmallIncrements) {
    float timer = 0.0f;

    // Simulate 60 FPS for 10 seconds (600 frames)
    constexpr float dt = 1.0f / 60.0f;
    int updateCount = 0;

    for (int i = 0; i < 600; ++i) {
        if (shouldUpdateRank(timer, dt)) {
            updateCount++;
        }
    }

    // Should have exactly 1 update in 10 seconds
    EXPECT_EQ(updateCount, 1);
}

// ═══════════════════════════════════════════════════════════════════
// Live Best Score Update Tests
// ═══════════════════════════════════════════════════════════════════

class LiveBestScoreTest : public ::testing::Test {
protected:
    // Same logic as GameScene::renderGlobalRank
    struct BestScoreDisplay {
        uint32_t displayValue;
        bool isNewRecord;
        std::string text;
    };

    BestScoreDisplay computeBestScoreDisplay(uint32_t currentScore, uint32_t savedBestScore) {
        BestScoreDisplay result;
        result.displayValue = (currentScore > savedBestScore) ? currentScore : savedBestScore;
        result.isNewRecord = currentScore > savedBestScore && savedBestScore > 0;

        // Format the text
        if (result.displayValue >= 1000000) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "BEST: %.1fM", result.displayValue / 1000000.0f);
            result.text = buf;
        } else if (result.displayValue >= 1000) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "BEST: %.1fK", result.displayValue / 1000.0f);
            result.text = buf;
        } else {
            result.text = "BEST: " + std::to_string(result.displayValue);
        }

        if (result.isNewRecord) {
            result.text += " NEW!";
        }

        return result;
    }
};

TEST_F(LiveBestScoreTest, BelowBest_ShowsSavedBest) {
    auto display = computeBestScoreDisplay(5000, 10000);

    EXPECT_EQ(display.displayValue, 10000u);
    EXPECT_FALSE(display.isNewRecord);
    EXPECT_EQ(display.text, "BEST: 10.0K");
}

TEST_F(LiveBestScoreTest, AboveBest_ShowsCurrentScore) {
    auto display = computeBestScoreDisplay(15000, 10000);

    EXPECT_EQ(display.displayValue, 15000u);
    EXPECT_TRUE(display.isNewRecord);
    EXPECT_EQ(display.text, "BEST: 15.0K NEW!");
}

TEST_F(LiveBestScoreTest, LiveUpdate_ScoreIncreases) {
    // Simulates score increasing during gameplay
    uint32_t savedBest = 10000;

    // Start below best
    auto display1 = computeBestScoreDisplay(5000, savedBest);
    EXPECT_EQ(display1.displayValue, 10000u);
    EXPECT_FALSE(display1.isNewRecord);

    // Equal to best (not new record yet)
    auto display2 = computeBestScoreDisplay(10000, savedBest);
    EXPECT_EQ(display2.displayValue, 10000u);
    EXPECT_FALSE(display2.isNewRecord);

    // Just beat the best
    auto display3 = computeBestScoreDisplay(10001, savedBest);
    EXPECT_EQ(display3.displayValue, 10001u);
    EXPECT_TRUE(display3.isNewRecord);
    EXPECT_EQ(display3.text, "BEST: 10.0K NEW!");

    // Keep going higher - value follows score
    auto display4 = computeBestScoreDisplay(25000, savedBest);
    EXPECT_EQ(display4.displayValue, 25000u);
    EXPECT_TRUE(display4.isNewRecord);
    EXPECT_EQ(display4.text, "BEST: 25.0K NEW!");
}

TEST_F(LiveBestScoreTest, FirstGame_NoBestYet) {
    // savedBestScore is 0 (no previous games)
    auto display = computeBestScoreDisplay(5000, 0);

    EXPECT_EQ(display.displayValue, 5000u);
    // isNewRecord should be false because savedBestScore is 0
    EXPECT_FALSE(display.isNewRecord);
    EXPECT_EQ(display.text, "BEST: 5.0K");
}

TEST_F(LiveBestScoreTest, ExactlyEqualToBest_NotNewRecord) {
    auto display = computeBestScoreDisplay(10000, 10000);

    EXPECT_EQ(display.displayValue, 10000u);
    EXPECT_FALSE(display.isNewRecord);
    EXPECT_EQ(display.text, "BEST: 10.0K");  // No "NEW!" because not strictly greater
}
