/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LeaderboardSortTest - Tests pour le tri des colonnes du leaderboard
*/

#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════
// Structures et enums (mêmes que LeaderboardScene)
// ═══════════════════════════════════════════════════════════════════

namespace {

enum class SortColumn {
    Rank,
    Score,
    Wave,
    Kills,
    Time
};

struct LeaderboardEntryData {
    uint32_t rank;
    std::string playerName;
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint32_t duration;
};

// Sort function (same logic as LeaderboardScene::sortLeaderboard)
void sortLeaderboard(std::vector<LeaderboardEntryData>& entries,
                     SortColumn sortColumn, bool sortAscending) {
    if (entries.empty()) return;

    std::sort(entries.begin(), entries.end(),
        [sortColumn, sortAscending](const LeaderboardEntryData& a, const LeaderboardEntryData& b) {
            bool less = false;
            switch (sortColumn) {
                case SortColumn::Rank:
                    less = a.rank < b.rank;
                    break;
                case SortColumn::Score:
                    less = a.score < b.score;
                    break;
                case SortColumn::Wave:
                    less = a.wave < b.wave;
                    break;
                case SortColumn::Kills:
                    less = a.kills < b.kills;
                    break;
                case SortColumn::Time:
                    less = a.duration < b.duration;
                    break;
            }
            return sortAscending ? less : !less;
        });
}

// Default direction logic (same as onColumnHeaderClick)
bool getDefaultSortDirection(SortColumn column) {
    // Rank default ascending (#1 first), others default descending (highest first)
    return (column == SortColumn::Rank);
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════

class LeaderboardSortTest : public ::testing::Test {
protected:
    std::vector<LeaderboardEntryData> entries;

    void SetUp() override {
        // Sample leaderboard data (not sorted)
        entries = {
            {1, "alice", 32600, 13, 56, 168},   // Rank 1
            {2, "bob",   17300, 10, 68, 122},   // Rank 2
            {3, "carol", 25000, 15, 45, 200},   // Rank 3
            {4, "dave",  12000, 8,  30, 90},    // Rank 4
            {5, "eve",   28000, 12, 72, 150},   // Rank 5
        };
    }
};

// ═══════════════════════════════════════════════════════════════════
// Sort by Rank Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, SortByRank_Ascending) {
    sortLeaderboard(entries, SortColumn::Rank, true);

    EXPECT_EQ(entries[0].rank, 1u);
    EXPECT_EQ(entries[1].rank, 2u);
    EXPECT_EQ(entries[2].rank, 3u);
    EXPECT_EQ(entries[3].rank, 4u);
    EXPECT_EQ(entries[4].rank, 5u);
}

TEST_F(LeaderboardSortTest, SortByRank_Descending) {
    sortLeaderboard(entries, SortColumn::Rank, false);

    EXPECT_EQ(entries[0].rank, 5u);
    EXPECT_EQ(entries[1].rank, 4u);
    EXPECT_EQ(entries[2].rank, 3u);
    EXPECT_EQ(entries[3].rank, 2u);
    EXPECT_EQ(entries[4].rank, 1u);
}

// ═══════════════════════════════════════════════════════════════════
// Sort by Score Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, SortByScore_Descending) {
    sortLeaderboard(entries, SortColumn::Score, false);

    // Highest score first: alice(32600) > eve(28000) > carol(25000) > bob(17300) > dave(12000)
    EXPECT_EQ(entries[0].playerName, "alice");
    EXPECT_EQ(entries[0].score, 32600u);
    EXPECT_EQ(entries[1].playerName, "eve");
    EXPECT_EQ(entries[2].playerName, "carol");
    EXPECT_EQ(entries[3].playerName, "bob");
    EXPECT_EQ(entries[4].playerName, "dave");
}

TEST_F(LeaderboardSortTest, SortByScore_Ascending) {
    sortLeaderboard(entries, SortColumn::Score, true);

    // Lowest score first
    EXPECT_EQ(entries[0].playerName, "dave");
    EXPECT_EQ(entries[0].score, 12000u);
    EXPECT_EQ(entries[4].playerName, "alice");
    EXPECT_EQ(entries[4].score, 32600u);
}

// ═══════════════════════════════════════════════════════════════════
// Sort by Wave Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, SortByWave_Descending) {
    sortLeaderboard(entries, SortColumn::Wave, false);

    // Highest wave first: carol(15) > alice(13) > eve(12) > bob(10) > dave(8)
    EXPECT_EQ(entries[0].playerName, "carol");
    EXPECT_EQ(entries[0].wave, 15u);
    EXPECT_EQ(entries[4].playerName, "dave");
    EXPECT_EQ(entries[4].wave, 8u);
}

TEST_F(LeaderboardSortTest, SortByWave_Ascending) {
    sortLeaderboard(entries, SortColumn::Wave, true);

    EXPECT_EQ(entries[0].playerName, "dave");
    EXPECT_EQ(entries[0].wave, 8u);
}

// ═══════════════════════════════════════════════════════════════════
// Sort by Kills Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, SortByKills_Descending) {
    sortLeaderboard(entries, SortColumn::Kills, false);

    // Highest kills first: eve(72) > bob(68) > alice(56) > carol(45) > dave(30)
    EXPECT_EQ(entries[0].playerName, "eve");
    EXPECT_EQ(entries[0].kills, 72u);
    EXPECT_EQ(entries[4].playerName, "dave");
    EXPECT_EQ(entries[4].kills, 30u);
}

TEST_F(LeaderboardSortTest, SortByKills_Ascending) {
    sortLeaderboard(entries, SortColumn::Kills, true);

    EXPECT_EQ(entries[0].playerName, "dave");
    EXPECT_EQ(entries[0].kills, 30u);
}

// ═══════════════════════════════════════════════════════════════════
// Sort by Time Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, SortByTime_Descending) {
    sortLeaderboard(entries, SortColumn::Time, false);

    // Longest time first: carol(200) > alice(168) > eve(150) > bob(122) > dave(90)
    EXPECT_EQ(entries[0].playerName, "carol");
    EXPECT_EQ(entries[0].duration, 200u);
    EXPECT_EQ(entries[4].playerName, "dave");
    EXPECT_EQ(entries[4].duration, 90u);
}

TEST_F(LeaderboardSortTest, SortByTime_Ascending) {
    sortLeaderboard(entries, SortColumn::Time, true);

    EXPECT_EQ(entries[0].playerName, "dave");
    EXPECT_EQ(entries[0].duration, 90u);
}

// ═══════════════════════════════════════════════════════════════════
// Default Direction Tests
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, DefaultDirection_RankIsAscending) {
    EXPECT_TRUE(getDefaultSortDirection(SortColumn::Rank));
}

TEST_F(LeaderboardSortTest, DefaultDirection_ScoreIsDescending) {
    EXPECT_FALSE(getDefaultSortDirection(SortColumn::Score));
}

TEST_F(LeaderboardSortTest, DefaultDirection_WaveIsDescending) {
    EXPECT_FALSE(getDefaultSortDirection(SortColumn::Wave));
}

TEST_F(LeaderboardSortTest, DefaultDirection_KillsIsDescending) {
    EXPECT_FALSE(getDefaultSortDirection(SortColumn::Kills));
}

TEST_F(LeaderboardSortTest, DefaultDirection_TimeIsDescending) {
    EXPECT_FALSE(getDefaultSortDirection(SortColumn::Time));
}

// ═══════════════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════════════

TEST_F(LeaderboardSortTest, EmptyList_NoError) {
    std::vector<LeaderboardEntryData> empty;
    EXPECT_NO_THROW(sortLeaderboard(empty, SortColumn::Score, false));
    EXPECT_TRUE(empty.empty());
}

TEST_F(LeaderboardSortTest, SingleEntry_NoChange) {
    std::vector<LeaderboardEntryData> single = {{1, "solo", 1000, 5, 10, 60}};
    sortLeaderboard(single, SortColumn::Score, false);
    EXPECT_EQ(single.size(), 1u);
    EXPECT_EQ(single[0].playerName, "solo");
}

TEST_F(LeaderboardSortTest, EqualValues_StableOrder) {
    // Two entries with same score
    std::vector<LeaderboardEntryData> tiedEntries = {
        {1, "first",  10000, 5, 20, 100},
        {2, "second", 10000, 5, 20, 100},
    };

    sortLeaderboard(tiedEntries, SortColumn::Score, false);

    // With equal scores, order may vary (std::sort is not stable)
    // Just verify both are still present
    EXPECT_EQ(tiedEntries.size(), 2u);
    bool hasFirst = (tiedEntries[0].playerName == "first" || tiedEntries[1].playerName == "first");
    bool hasSecond = (tiedEntries[0].playerName == "second" || tiedEntries[1].playerName == "second");
    EXPECT_TRUE(hasFirst);
    EXPECT_TRUE(hasSecond);
}

// ═══════════════════════════════════════════════════════════════════
// Toggle Direction Tests (simulating user clicks)
// ═══════════════════════════════════════════════════════════════════

class ToggleDirectionTest : public ::testing::Test {
protected:
    SortColumn _sortColumn = SortColumn::Rank;
    bool _sortAscending = true;

    void onColumnHeaderClick(SortColumn column) {
        if (_sortColumn == column) {
            _sortAscending = !_sortAscending;
        } else {
            _sortColumn = column;
            _sortAscending = getDefaultSortDirection(column);
        }
    }
};

TEST_F(ToggleDirectionTest, ClickSameColumn_TogglesDirection) {
    // Initial: Rank ascending
    EXPECT_EQ(_sortColumn, SortColumn::Rank);
    EXPECT_TRUE(_sortAscending);

    // Click Rank again -> descending
    onColumnHeaderClick(SortColumn::Rank);
    EXPECT_EQ(_sortColumn, SortColumn::Rank);
    EXPECT_FALSE(_sortAscending);

    // Click Rank again -> ascending
    onColumnHeaderClick(SortColumn::Rank);
    EXPECT_TRUE(_sortAscending);
}

TEST_F(ToggleDirectionTest, ClickDifferentColumn_SetsDefaultDirection) {
    // Initial: Rank ascending
    EXPECT_EQ(_sortColumn, SortColumn::Rank);
    EXPECT_TRUE(_sortAscending);

    // Click Score -> descending (default for non-Rank)
    onColumnHeaderClick(SortColumn::Score);
    EXPECT_EQ(_sortColumn, SortColumn::Score);
    EXPECT_FALSE(_sortAscending);

    // Click Kills -> descending (default)
    onColumnHeaderClick(SortColumn::Kills);
    EXPECT_EQ(_sortColumn, SortColumn::Kills);
    EXPECT_FALSE(_sortAscending);

    // Click Rank -> ascending (default for Rank)
    onColumnHeaderClick(SortColumn::Rank);
    EXPECT_EQ(_sortColumn, SortColumn::Rank);
    EXPECT_TRUE(_sortAscending);
}

TEST_F(ToggleDirectionTest, MultipleToggles_Score) {
    onColumnHeaderClick(SortColumn::Score);  // -> Score descending
    EXPECT_FALSE(_sortAscending);

    onColumnHeaderClick(SortColumn::Score);  // -> Score ascending
    EXPECT_TRUE(_sortAscending);

    onColumnHeaderClick(SortColumn::Score);  // -> Score descending
    EXPECT_FALSE(_sortAscending);
}
