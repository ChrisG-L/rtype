# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using **Hexagonal Architecture** (Ports & Adapters). Server and client communicate via UDP for real-time gameplay.

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | TCP 4125 (auth/TLS), UDP 4124 (game), UDP 4126 (voice), TCP 4127 (admin) |
| Client | C++23, SFML/SDL2 (multi-backend) | - |
| Build | CMake 3.30+, Ninja, vcpkg | - |

## Project Structure

```
rtype/
├── src/
│   ├── server/                    # Hexagonal Architecture
│   │   ├── include/
│   │   │   ├── domain/            # Business logic (entities, value_objects, services)
│   │   │   ├── application/       # Use cases, ports
│   │   │   └── infrastructure/    # Adapters, network, game
│   │   └── infrastructure/game/   # GameWorld (central game state)
│   ├── client/
│   │   ├── include/
│   │   │   ├── scenes/            # IScene, GameScene, SceneManager
│   │   │   ├── network/           # UDPClient, TCPClient
│   │   │   ├── graphics/          # IWindow (abstraction)
│   │   │   └── audio/             # VoiceChatManager, OpusCodec
│   │   └── lib/                   # Graphics backends (sfml/, sdl2/)
│   └── common/
│       ├── protocol/Protocol.hpp  # Binary protocol definitions
│       └── collision/AABB.hpp     # Hitbox system
├── assets/                        # Sprites, audio, fonts
├── tests/                         # Google Test
├── discord-bot/                   # Admin & Leaderboard bots (see docs/)
└── docs/                          # MkDocs documentation
```

## Build Commands

```bash
./scripts/build.sh                    # Configure (Linux)
./scripts/build.sh --platform=windows # Windows cross-compile
./scripts/compile.sh                  # Compile
./scripts/run-client.sh               # Run client (handles PipeWire audio)
./scripts/run-client.sh --server=51.254.137.175  # Connect to VPS
./artifacts/server/linux/rtype_server # Run server
rm -rf build*/ artifacts/             # Clean build
```

## Network Protocol

Binary protocol over UDP, network byte order (big-endian). All messages start with `UDPHeader` (12 bytes).

### Core Message Types

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `HeartBeat` | 0x0001 | Both | Keep-alive |
| `Snapshot` | 0x0040 | S→C | Full game state (20Hz) |
| `PlayerInput` | 0x0061 | C→S | Movement/action bitfield |
| `PlayerJoin` | 0x0070 | S→C | New player joined |
| `PlayerLeave` | 0x0071 | S→C | Player disconnected |
| `ShootMissile` | 0x0080 | C→S | Fire request |
| `MissileSpawned` | 0x0081 | S→C | Missile created |
| `MissileDestroyed` | 0x0082 | S→C | Missile removed |
| `EnemyDestroyed` | 0x0091 | S→C | Enemy killed |
| `PlayerDamaged` | 0x00A0 | S→C | Damage notification |
| `PlayerDied` | 0x00A1 | S→C | Player died |
| `BossSpawn` | 0x00C0 | S→C | Boss appeared |
| `BossDefeated` | 0x00C2 | S→C | Boss killed |
| `VoiceFrame` | 0x0303 | Both | Opus audio data |
| `ChargeStart` | 0x0400 | C→S | Start Wave Cannon charge |
| `ChargeRelease` | 0x0401 | C→S | Release charged shot |
| `PowerUpSpawned` | 0x0410 | S→C | Power-up appeared |
| `PowerUpCollected` | 0x0411 | S→C | Power-up collected |
| `ForceToggle` | 0x0420 | C→S | Toggle Force attach/detach |

> Full message list: `src/common/protocol/Protocol.hpp`

### Key Structures

```cpp
struct UDPHeader {        // 12 bytes - All messages
    uint16_t type;
    uint16_t sequence_num;
    uint64_t timestamp;
};

struct PlayerState {      // 23 bytes
    uint8_t id;
    uint16_t x, y;
    uint8_t health, alive;
    uint16_t lastAckedInputSeq;
    uint8_t shipSkin;
    uint32_t score;
    uint16_t kills;
    uint8_t combo;          // x10 (15 = 1.5x)
    uint8_t currentWeapon, chargeLevel, speedLevel, weaponLevel;
    uint8_t hasForce, shieldTimer;
};

struct GameSnapshot {     // Variable size - broadcast at 20Hz
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];           // MAX_PLAYERS = 4
    uint8_t missile_count;
    MissileState missiles[MAX_MISSILES];        // MAX_MISSILES = 32
    uint8_t enemy_count;
    EnemyState enemies[MAX_ENEMIES];            // MAX_ENEMIES = 16
    uint8_t enemy_missile_count;
    MissileState enemy_missiles[MAX_ENEMY_MISSILES]; // MAX_ENEMY_MISSILES = 32
    uint16_t wave_number;
    uint8_t has_boss;
    BossState boss_state;
    uint8_t force_count;
    ForceStateSnapshot forces[MAX_PLAYERS];
    uint8_t bit_count;
    BitDeviceStateSnapshot bits[MAX_BITS];      // MAX_BITS = 8
};

struct MissileState {     // 8 bytes
    uint16_t id;
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t weapon_type;
};

struct BossState {        // 12 bytes
    uint16_t id;
    uint16_t x, y;
    uint16_t max_health, health;
    uint8_t phase, is_active;
};

struct ForceStateSnapshot { // 7 bytes
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t is_attached;
    uint8_t level;          // 0 = no force, 1-2 = level
};

struct BitDeviceStateSnapshot { // 6 bytes
    uint8_t owner_id;
    uint8_t bit_index;      // 0 or 1
    uint16_t x, y;
    uint8_t is_attached;
};
```

### Serialization Pattern

All structures use `to_bytes()` and `from_bytes()` with network byte order:

```cpp
void to_bytes(uint8_t* buf) const {
    uint16_t net_x = swap16(x);  // Host to network order
    std::memcpy(buf, &net_x, 2);
}

static std::optional<T> from_bytes(const void* buf, size_t len) {
    if (len < WIRE_SIZE) return std::nullopt;
    // Parse with swap16/swap32/swap64 for network to host
}
```

## Collision System

AABB in `src/common/collision/AABB.hpp`:

```cpp
namespace collision {
    struct AABB {
        float x, y, width, height;
        constexpr bool intersects(const AABB& other) const;
        constexpr bool contains(float px, float py) const;
    };
}
```

| Entity | Hitbox W×H | Render W×H |
|--------|------------|------------|
| Ship | 64×30 | 64×64 |
| Missile | 16×8 | 32×16 |
| Enemy | 40×40 | 40×40 |
| Enemy Missile | 16×8 | 24×12 |
| Boss | 150×120 | 150×120 |

## Client Architecture

### Graphics Abstraction

```cpp
// Interface: src/client/include/graphics/IWindow.hpp
class IWindow {
    virtual void drawRect(float x, float y, float w, float h, rgba color) = 0;
    virtual void drawSprite(const std::string& key, float x, float y, float w, float h) = 0;
    virtual bool loadTexture(const std::string& key, const std::string& path) = 0;
    virtual events::Event pollEvent() = 0;
    virtual void clear() = 0;
    virtual void display() = 0;
};
// Implementations: SFMLWindow (lib/sfml/), SDL2Window (lib/sdl2/)
```

### Scene System

```cpp
class IScene {
    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltatime) = 0;
    virtual void render() = 0;
    SceneContext _context;  // window, udpClient, tcpClient
};
```

### Event System

```cpp
using Event = std::variant<None, WindowClosed, KeyPressed, KeyReleased>;
// Key enum: A-Z, Num0-9, Space, Enter, Escape, Arrows, Modifiers
```

### Fullscreen

- **F11**: Toggle fullscreen
- Letterboxing maintains 16:9 aspect ratio
- Mouse coordinates auto-mapped to 1920x1080 logical space

## Server Architecture

### GameWorld

Central game state (`src/server/infrastructure/game/GameWorld.hpp`):

```cpp
class GameWorld {
    void addPlayer(uint8_t id);
    void removePlayer(uint8_t id);
    void updatePlayerPosition(uint8_t id, uint16_t x, uint16_t y);
    void spawnMissile(uint8_t playerId);
    void updateMissiles(float deltaTime);
    std::vector<uint16_t> getDestroyedMissiles();
    GameSnapshot getSnapshot() const;  // For 20Hz broadcast
};
```

### Network Servers

| Server | Port | Role |
|--------|------|------|
| UDPServer | 4124 | Game state, player input |
| TCPAuthServer | 4125 | Authentication (TLS) |
| VoiceUDPServer | 4126 | Voice relay (Opus) |
| TCPAdminServer | 4127 | Remote administration (localhost only) |

## Game Constants

| Constant | Value | File |
|----------|-------|------|
| MAX_PLAYERS | 4 | Protocol.hpp |
| MAX_MISSILES | 32 | Protocol.hpp |
| MAX_ENEMIES | 16 | Protocol.hpp |
| MAX_ENEMY_MISSILES | 32 | Protocol.hpp |
| MAX_POWERUPS | 8 | Protocol.hpp |
| MAX_BITS | 8 | Protocol.hpp |
| BROADCAST_RATE | 20Hz (50ms) | UDPServer.cpp |
| MOVE_SPEED | 200.0f | GameWorld.hpp |
| MISSILE_SPEED | 600.0f | GameWorld.hpp |
| SHOOT_COOLDOWN | 0.3s | GameScene.hpp |
| MAX_HEALTH | 100 | GameScene.hpp |
| BOSS_SPAWN_WAVE | 10 | GameWorld.hpp |
| BOSS_MAX_HEALTH | 1500 (base) | GameWorld.hpp |
| COMBO_GRACE_TIME | 3.0s | GameWorld.hpp |

> Full constants: `GameWorld.hpp`, `GameScene.hpp`, `Protocol.hpp`

### Weapon Types

| Type | Value | Special |
|------|-------|---------|
| Standard | 0 | Default |
| Spread | 1 | 15° angle spread |
| Laser | 2 | 1.5x speed |
| Missile | 3 | Homing (2.0 rad/s turn) |
| WaveCannon | 4 | Charged beam (not selectable) |

> MAX_SELECTABLE_WEAPONS = 4 (excludes WaveCannon), MAX_WEAPON_TYPES = 5

### Enemy Types

| Type | Speed | HP | Points |
|------|-------|----|----|
| Basic | -120 | 40 | 100 |
| Tracker | -100 | 35 | 150 |
| Zigzag | -140 | 30 | 120 |
| Fast | -220 | 25 | 180 |
| Bomber | -80 | 80 | 250 |
| POWArmor | -90 | 60 | 200 |

### Power-Up Types

| Type | Effect |
|------|--------|
| Health | +25 HP |
| SpeedUp | +1 speed level (max 3) |
| WeaponCrystal | +1 weapon level |
| ForcePod | Gives/upgrades Force Pod |
| BitDevice | 2 orbiting Bit Devices |

## Coding Conventions

### C++ Style

- **Standard**: C++23
- **Classes**: `PascalCase`
- **Methods**: `camelCase`
- **Private members**: `_prefixedWithUnderscore`
- **Constants**: `SCREAMING_SNAKE_CASE`
- **Headers**: `.hpp` / `.cpp`
- **Namespaces**: Match directory structure (`collision::AABB`, `events::Key`)

### Architecture Rules

1. **Domain NEVER depends on Infrastructure**
2. **Value Objects are immutable** - return new instances
3. **Network protocol uses big-endian** (network byte order)
4. **Changes to IWindow require both SFML and SDL2 implementations**

## Git Conventions

Format: `TYPE: Description`

| Type | Usage |
|------|-------|
| FEAT | New feature |
| FIX | Bug fix |
| DOCS | Documentation |
| REFACTOR | Refactoring |
| TEST | Adding tests |
| BUILD | Build system |

## Dependencies (vcpkg)

- **boost-asio**: Async networking (server)
- **gtest**: Testing framework
- **sfml**: Client graphics (SFML backend)
- **sdl2**, **sdl2-image**: Client graphics (SDL2 backend)
- **opus**: Audio codec for voice chat
- **portaudio**: Cross-platform audio I/O

## Key Files

| Purpose | Path |
|---------|------|
| Protocol | `src/common/protocol/Protocol.hpp` |
| Collision | `src/common/collision/AABB.hpp` |
| Server main | `src/server/main.cpp` |
| GameWorld | `src/server/infrastructure/game/GameWorld.cpp` |
| UDPServer | `src/server/infrastructure/adapters/in/network/UDPServer.cpp` |
| TCPAuthServer | `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp` |
| Client main | `src/client/main.cpp` |
| GameScene | `src/client/src/scenes/GameScene.cpp` |
| UDPClient | `src/client/src/network/UDPClient.cpp` |
| TCPClient | `src/client/src/network/TCPClient.cpp` |
| IWindow | `src/client/include/graphics/IWindow.hpp` |
| SFMLWindow | `src/client/lib/sfml/src/SFMLWindow.cpp` |
| SDL2Window | `src/client/lib/sdl2/src/SDL2Window.cpp` |
| VoiceChatManager | `src/client/src/audio/VoiceChatManager.cpp` |

## MCP & AgentDB

```bash
/analyze                # Incremental analysis (from last checkpoint)
/analyze --all          # Full analysis (from merge-base)
/analyze --reset        # Reset checkpoint to HEAD
/analyze --files <path> # Analyze specific files
```

### AgentDB Tools

| Tool | Description |
|------|-------------|
| `get_file_context` | File symbols, deps, errors |
| `get_symbol_callers` | Find all callers (recursive) |
| `get_symbol_callees` | Find all functions called by a symbol |
| `get_file_impact` | Impact of modifying a file |
| `get_error_history` | Historical bugs for a file/module |
| `search_symbols` | Search symbols by pattern |

### Analysis Verdicts

| Score | Verdict | Action |
|-------|---------|--------|
| ≥80 | APPROVE | Can merge |
| ≥60 | REVIEW | Human review recommended |
| ≥40 | CAREFUL | Deep review required |
| <40 | REJECT | Do not merge |

## Hidden Features (Developer Only)

### GodMode

| Aspect | Details |
|--------|---------|
| Activation | Type `/toggleGodMode` in chat during gameplay |
| Effect | Player takes no damage |
| Persistence | Saved to MongoDB (user_settings.godMode) |

## Friends & Private Messaging

Social system with friend requests, friendships, blocking, and private messages via TCP.

### TCP Message Ranges

| Range | Feature |
|-------|---------|
| 0x0600-0x0650 | Friends (requests, accept/reject, block, status) |
| 0x0660-0x0681 | Private messages (send, receive, history) |

### Key Components

| Component | Description |
|-----------|-------------|
| `FriendManager` | Real-time notification manager (server) |
| `FriendsScene` | Friends UI (client) |
| `PrivateChatScene` | Chat UI (client) |

### MongoDB Collections

`friendships`, `friend_requests`, `blocked_users`, `private_messages`

> Full details: [docs/friends-system.md](docs/friends-system.md)

## Notes for Claude

1. **Read files before modifying** - Use `Read` tool first
2. **Follow Hexagonal Architecture** - Domain has no external deps
3. **Protocol is binary** - Use `to_bytes()`/`from_bytes()` with byte swap
4. **Multi-backend graphics** - Changes to IWindow require both SFML and SDL2
5. **Missiles are server-authoritative** - Client requests, server spawns
6. **GameSnapshot is source of truth** - 20Hz broadcast
7. **Use AgentDB** - Query file context and impact before refactoring
8. **Never compile** - Ask user to compile and wait for feedback

## External Documentation

Detailed documentation in `docs/`:

- [docs/technique/audio/voice-chat.md](docs/technique/audio/voice-chat.md) - Voice system, Opus, PortAudio
- [docs/configuration/serveur.md](docs/configuration/serveur.md) - Server config, TLS, TCPAdminServer
- [docs/api/reseau/protocol.md](docs/api/reseau/protocol.md) - Full protocol reference
- [docs/developpement/vps.md](docs/developpement/vps.md) - VPS deployment, systemd
- [docs/developpement/discord-admin-bot.md](docs/developpement/discord-admin-bot.md) - Discord Admin Bot
- [docs/developpement/discord-leaderboard-bot.md](docs/developpement/discord-leaderboard-bot.md) - Discord Leaderboard Bot
- [docs/guide/gameplay/leaderboard.md](docs/guide/gameplay/leaderboard.md) - Leaderboard UI guide
- [docs/leaderboard.md](docs/leaderboard.md) - Leaderboard wire protocol, MongoDB
- [docs/friends-system.md](docs/friends-system.md) - Friends & private messaging system
- [docs/version-checking.md](docs/version-checking.md) - Client/server version sync
