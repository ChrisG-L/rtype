# Leaderboard & Achievements System

Global leaderboards with player statistics and achievements, persisted in MongoDB.

## Architecture

```
[Client: LeaderboardScene] ←TCP→ [TCPAuthServer]
                                        ↓
                              [ILeaderboardRepository]
                                        ↓
                           [MongoDBLeaderboardRepository]
                                        ↓
                                    [MongoDB]
```

## MongoDB Collections

| Collection | Description |
|------------|-------------|
| `leaderboard` | Top scores with player info |
| `player_stats` | Cumulative statistics per player |
| `game_history` | Individual game records |
| `achievements` | Unlocked achievement timestamps |
| `current_game_sessions` | Auto-save of in-progress game stats |

## TCP Protocol Messages

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `GetLeaderboard` | 0x0500 | C→S | Request leaderboard (period + limit) |
| `LeaderboardData` | 0x0501 | S→C | Leaderboard entries response |
| `GetPlayerStats` | 0x0502 | C→S | Request own stats |
| `PlayerStatsData` | 0x0503 | S→C | Player statistics response |
| `GetGameHistory` | 0x0504 | C→S | Request game history |
| `GameHistoryData` | 0x0505 | S→C | Game history entries |
| `GetAchievements` | 0x0520 | C→S | Request achievements |
| `AchievementsData` | 0x0521 | S→C | Achievement bitfield |

## Wire Structures

```cpp
// GetLeaderboardRequest (3 bytes)
struct GetLeaderboardRequest {
    uint8_t period;       // 0=All-Time, 1=Weekly, 2=Monthly
    uint8_t limit;        // Max entries (default 50)
    uint8_t playerCount;  // 0=All, 1=Solo, 2=Duo, 3=Trio, 4-6=4P-6P
};

// LeaderboardEntryWire (57 bytes per entry)
struct LeaderboardEntryWire {
    uint32_t rank;
    char playerName[32];
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint32_t duration;    // Seconds
    int64_t timestamp;    // Unix timestamp
    uint8_t playerCount;  // Number of players when score was achieved
};

// LeaderboardDataResponse header (7 bytes)
struct LeaderboardDataResponse {
    uint8_t period;
    uint8_t count;
    uint32_t yourRank;
    uint8_t playerCountFilter;
    // Followed by count * LeaderboardEntryWire
};

// PlayerStatsWire (80 bytes)
struct PlayerStatsWire {
    char playerName[32];    // PLAYER_NAME_LEN
    uint64_t totalScore;    // Note: uint64_t not uint32_t
    uint32_t totalKills;
    uint32_t totalDeaths;
    uint32_t totalPlaytime; // Seconds
    uint32_t gamesPlayed;
    uint32_t bestScore;
    uint16_t bestWave;
    uint16_t bestCombo;     // x10 (30 = 3.0x)
    uint16_t bestKillStreak;
    uint16_t bossKills;
    uint32_t standardKills;
    uint32_t spreadKills;
    uint32_t laserKills;
    uint32_t missileKills;
    uint32_t achievements;  // Bitfield
};

// GameHistoryEntryWire (23 bytes per entry)
struct GameHistoryEntryWire {
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint8_t deaths;       // Note: uint8_t not uint16_t
    uint32_t duration;    // Seconds
    uint64_t timestamp;
    uint8_t playerCount;
    uint8_t gameMode;     // 0=normal, 1=boss rush, etc.
};
```

## Leaderboard Filtering

| Filter | Description |
|--------|-------------|
| 0 | All modes combined |
| 1 | Solo (1 player) |
| 2 | Duo (2 players) |
| 3 | Trio (3 players) |
| 4-6 | 4P to 6P |

## Achievements System

10 achievements tracked as a 32-bit bitfield:

| Bit | Achievement | Condition |
|-----|-------------|-----------|
| 0 | First Blood | Get 1 kill |
| 1 | Exterminator | 1000 total kills |
| 2 | Combo Master | Achieve 3.0x combo |
| 3 | Boss Slayer | Kill any boss |
| 4 | Survivor | Reach wave 20 without dying |
| 5 | Speed Demon | Wave 10 in under 5 minutes |
| 6 | Perfectionist | Complete wave without damage |
| 7 | Veteran | Play 100 games |
| 8 | Untouchable | Complete game with 0 deaths |
| 9 | Weapon Master | 100+ kills with each weapon |

## Leaderboard Periods

| Period | Value | Filter |
|--------|-------|--------|
| All-Time | 0 | No filter |
| Weekly | 1 | Last 7 days |
| Monthly | 2 | Last 30 days |

## Client UI (LeaderboardScene)

Accessible via **LEADERBOARD** button in main menu.

**Tabs:**
1. **Leaderboard** - Top 50 players with rank, name, score, wave, kills
2. **Stats** - Personal statistics (games, K/D, playtime, weapon usage)
3. **Achievements** - 10 achievement badges (locked/unlocked)

**Navigation:**
- Tab buttons at top
- Period filter buttons (All-Time/Weekly/Monthly)
- Mode filter buttons (ALL/SOLO/DUO/TRIO/4P/5P/6P)
- **BACK** button returns to main menu

## Real-Time Rank Display (GameScene)

During gameplay, the HUD shows:
- **Global rank badge** - Current position (e.g., "RANK #42")
- **Personal best score** - Target to beat (e.g., "BEST: 32.6K")
- Updates every 10 seconds

**Rank Colors:**

| Position | Color |
|----------|-------|
| #1 | Gold (255, 215, 0) |
| #2 | Silver (192, 192, 192) |
| #3 | Bronze (205, 127, 50) |
| Top 10 | Light Blue |
| Top 50 | Green |
| Others | Gray |

**Score Formatting:**
- `< 1000` → Raw number
- `≥ 1000` → K format (e.g., "32.6K")
- `≥ 1000000` → M format (e.g., "1.2M")

## Key Files

| File | Description |
|------|-------------|
| `src/server/include/application/ports/out/persistence/ILeaderboardRepository.hpp` | Repository interface |
| `src/server/infrastructure/adapters/out/persistence/MongoDBLeaderboardRepository.cpp` | MongoDB implementation |
| `src/server/include/application/services/AchievementChecker.hpp` | Achievement logic |
| `src/client/include/scenes/LeaderboardScene.hpp` | Client scene header |
| `src/client/src/scenes/LeaderboardScene.cpp` | Client scene implementation |
| `src/common/protocol/Protocol.hpp` | Wire format structures |

## Usage Example

```cpp
// Request leaderboard data
auto& tcpClient = TCPClient::getInstance();
tcpClient.sendGetLeaderboard({.period = 0, .limit = 50, .playerCount = 0});
tcpClient.sendGetPlayerStats();
tcpClient.sendGetAchievements();

// Handle response events
while (auto event = tcpClient.pollEvent()) {
    if (auto* data = std::get_if<LeaderboardDataEvent>(&*event)) {
        // Update UI with data->response.entries
    }
}
```

## Unit Tests

| Test File | Coverage |
|-----------|----------|
| `tests/server/application/services/AchievementCheckerTest.cpp` | All 10 achievements |
| `tests/server/application/services/LeaderboardDataTest.cpp` | PlayerStats, GameHistoryEntry |

```bash
./artifacts/tests/server_tests --gtest_filter="Achievement*:PlayerStats*:Leaderboard*"
```
