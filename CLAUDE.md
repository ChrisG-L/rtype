# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using an **Hexagonal Architecture** (Ports & Adapters). The project consists of a server and client communicating via UDP for real-time gameplay synchronization.

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | UDP 4124 (game), UDP 4126 (voice) |
| Client | C++23, SFML/SDL2 (multi-backend) | - |
| Build | CMake 3.30+, Ninja, vcpkg, Nix | - |

## Project Structure

```
rtype/
├── src/
│   ├── server/                          # Server implementation (Hexagonal)
│   │   ├── include/
│   │   │   ├── domain/                  # Business logic
│   │   │   │   ├── entities/            # Player
│   │   │   │   ├── value_objects/       # Position, Health, PlayerId
│   │   │   │   ├── exceptions/          # DomainException, etc.
│   │   │   │   └── services/            # GameRule
│   │   │   ├── application/             # Use cases, ports
│   │   │   │   ├── use_cases/player/    # Move
│   │   │   │   ├── ports/in/            # IGameCommands
│   │   │   │   └── ports/out/           # IPlayerRepository
│   │   │   └── infrastructure/          # Adapters, network, game
│   │   │       ├── adapters/in/network/ # UDPServer
│   │   │       ├── game/                # GameWorld (missiles, players)
│   │   │       └── logging/             # Logger
│   │   ├── domain/                      # Domain implementations
│   │   ├── application/                 # Application implementations
│   │   └── infrastructure/              # Infrastructure implementations
│   │
│   ├── client/                          # Client implementation
│   │   ├── include/
│   │   │   ├── scenes/                  # GameScene, SceneManager, IScene
│   │   │   ├── network/                 # UDPClient, TCPClient
│   │   │   ├── graphics/                # IWindow, IDrawable, Graphics
│   │   │   ├── gameplay/                # EntityManager, GameObject, Missile
│   │   │   ├── audio/                   # AudioManager, VoiceChatManager, OpusCodec
│   │   │   ├── events/                  # Event system (KeyPressed, etc.)
│   │   │   ├── core/                    # Engine, GameLoop, Logger
│   │   │   ├── boot/                    # Boot
│   │   │   └── utils/                   # Vecs (Vec2u, Vec2f)
│   │   ├── src/                         # Implementations
│   │   └── lib/                         # Graphics backends
│   │       ├── sfml/                    # SFML backend
│   │       │   ├── include/             # SFMLWindow, SFMLRenderer
│   │       │   └── src/
│   │       └── sdl2/                    # SDL2 backend
│   │           ├── include/             # SDL2Window, SDL2Renderer
│   │           └── src/
│   │
│   ├── common/                          # Shared code
│   │   ├── protocol/                    # Protocol.hpp (binary protocol)
│   │   └── collision/                   # AABB.hpp (hitboxes)
│   │
│   └── ECS/                             # Entity Component System (future)
│
├── assets/                              # Game assets
│   ├── spaceship/                       # Ship1.png, missile.png
│   ├── enemies/
│   ├── backgrounds/
│   ├── audio/
│   └── fonts/
│
├── tests/                               # Google Test tests
├── docs/                                # MkDocs documentation
├── scripts/                             # Build scripts
├── third_party/vcpkg/                   # Package manager
├── .mcp.json                            # MCP server configuration
└── .claude/                             # Claude Code tooling
```

## Build Commands

```bash
# Configure (first time or after CMakeLists changes)
./scripts/build.sh                    # Linux
./scripts/build.sh --platform=windows # Windows cross-compile

# Compile
./scripts/compile.sh

# Run
./artifacts/server/linux/rtype_server  # Server (UDP 4124)
./artifacts/client/linux/rtype_client  # Client

# Clean build
rm -rf build*/ artifacts/
```

## Network Protocol

Binary protocol over UDP with network byte order (big-endian). All messages start with `UDPHeader` (12 bytes).

### Message Types

| Type | Value | Direction | Payload | Description |
|------|-------|-----------|---------|-------------|
| `HeartBeat` | 0x0001 | Both | - | Keep-alive |
| `Snapshot` | 0x0040 | S→C | GameSnapshot | Full game state (20Hz) |
| `PlayerInput` | 0x0061 | C→S | keys, seq (4B) | Input bitfield + sequence |
| `PlayerJoin` | 0x0070 | S→C | player_id (1B) | New player joined |
| `PlayerLeave` | 0x0071 | S→C | player_id (1B) | Player disconnected |
| `ShootMissile` | 0x0080 | C→S | - | Fire missile request |
| `MissileSpawned` | 0x0081 | S→C | MissileState (8B) | Missile created |
| `MissileDestroyed` | 0x0082 | S→C | missile_id (2B) | Missile removed |
| `ScoreUpdate` | 0x00B0 | S→C | - | Score system (in snapshot) |
| `BossSpawn` | 0x00C0 | S→C | BossState | Boss appeared |
| `BossPhaseChange` | 0x00C1 | S→C | phase (1B) | Boss phase changed |
| `BossDefeated` | 0x00C2 | S→C | - | Boss killed |
| `SwitchWeapon` | 0x00D0 | C→S | direction (1B) | Change weapon |
| `WeaponChanged` | 0x00D1 | S→C | weapon_type (1B) | Weapon confirmed |
| `VoiceJoin` | 0x0300 | C→S | VoiceJoin (38B) | Join voice channel |
| `VoiceJoinAck` | 0x0301 | S→C | player_id (1B) | Voice join confirmed |
| `VoiceLeave` | 0x0302 | C→S | player_id (1B) | Leave voice channel |
| `VoiceFrame` | 0x0303 | Both | VoiceFrame (5-485B) | Opus audio data |
| `VoiceMute` | 0x0304 | Both | VoiceMute (2B) | Mute/unmute notification |

### Key Structures

```cpp
// UDPHeader (12 bytes) - All messages
struct UDPHeader {
    uint16_t type;          // MessageType
    uint16_t sequence_num;
    uint64_t timestamp;     // Milliseconds since epoch
};

// PlayerState (18 bytes) - Includes score and weapon
struct PlayerState {
    uint8_t id;
    uint16_t x, y;
    uint8_t health;         // 0-100
    uint8_t alive;          // 0 or 1
    uint16_t lastAckedInputSeq;  // Client-side prediction
    uint8_t shipSkin;       // 1-6
    uint32_t score;         // Total score
    uint16_t kills;         // Kill count
    uint8_t combo;          // Combo x10 (15 = 1.5x)
    uint8_t currentWeapon;  // WeaponType enum
};

// MissileState (8 bytes) - Includes weapon type
struct MissileState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t weapon_type;    // WeaponType enum
};

// BossState (12 bytes) - Boss fight data
struct BossState {
    uint16_t id;
    uint16_t x, y;
    uint16_t max_health;
    uint16_t health;
    uint8_t phase;          // 0, 1, 2
    uint8_t is_active;      // 0 or 1
};

// GameSnapshot (variable size) - Broadcast at 20Hz
struct GameSnapshot {
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];      // MAX_PLAYERS = 4
    uint8_t missile_count;
    MissileState missiles[MAX_MISSILES];   // MAX_MISSILES = 32
    uint8_t enemy_count;
    EnemyState enemies[MAX_ENEMIES];       // MAX_ENEMIES = 16
    uint16_t wave_number;
    uint8_t has_boss;
    BossState boss_state;                  // If has_boss
};

// VoiceFrame (5-485 bytes) - Opus-encoded audio
struct VoiceFrame {
    uint8_t speaker_id;       // Who is speaking
    uint16_t sequence;        // For packet loss detection
    uint16_t opus_len;        // Actual Opus data length
    uint8_t opus_data[480];   // Opus-encoded audio (max 480 bytes)
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

AABB (Axis-Aligned Bounding Box) in `src/common/collision/AABB.hpp`:

```cpp
namespace collision {
    struct AABB {
        float x, y, width, height;
        constexpr bool intersects(const AABB& other) const;
        constexpr bool contains(float px, float py) const;
    };

    namespace Hitboxes {
        SHIP_WIDTH = 50.0f, SHIP_HEIGHT = 30.0f;
        MISSILE_WIDTH = 16.0f, MISSILE_HEIGHT = 8.0f;
        ENEMY_WIDTH = 40.0f, ENEMY_HEIGHT = 40.0f;
    }
}
```

## Client Architecture

### Graphics Abstraction (Multi-Backend)

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

// Implementations:
// - SFMLWindow (lib/sfml/)
// - SDL2Window (lib/sdl2/)
```

### Scene System

```cpp
// IScene interface
class IScene {
    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltatime) = 0;
    virtual void render() = 0;
    SceneContext _context;  // window, udpClient
};

// GameScene implements: HUD, players, missiles rendering
```

### Event System

```cpp
// events::Event is a std::variant
using Event = std::variant<None, WindowClosed, KeyPressed, KeyReleased>;

// Key enum: A-Z, Num0-9, Space, Enter, Escape, Arrows, Modifiers
```

### Network Client

```cpp
// UDPClient handles:
// - Connection (sendJoin, sendHeartbeat)
// - Player movement (movePlayer)
// - Missiles (shootMissile)
// - State sync (getPlayers, getMissiles, getLocalPlayerId)
```

### Voice Chat System

Real-time voice communication using Opus codec and PortAudio.

```cpp
// VoiceChatManager (singleton) handles:
// - Microphone capture via PortAudio
// - Opus encoding/decoding (48kHz mono, 32kbps)
// - Network communication with VoiceUDPServer (port 4126)
// - Playback mixing from multiple speakers

// Usage in GameScene:
auto& voice = VoiceChatManager::getInstance();
voice.init();
voice.connect(serverHost, 4126);
voice.joinVoiceChannel(sessionToken, roomCode);

// Push-to-Talk (configurable key, default: V)
// Key can be changed in Settings > Controls > Push-to-Talk
if (keyPressed(PushToTalkKey)) voice.startTalking();
if (keyReleased(PushToTalkKey)) voice.stopTalking();

// Voice Activity Detection (alternative mode)
voice.setVoiceMode(VoiceMode::VoiceActivity);
voice.setVADThreshold(0.02f);  // Sensitivity

// Audio device selection (cross-platform)
auto inputs = voice.getInputDevices();   // List available microphones
auto outputs = voice.getOutputDevices(); // List available speakers
voice.setPreferredInputDevice("Device Name");  // Or "" for auto
voice.setPreferredOutputDevice("Device Name");
voice.applyAudioDevices(inputName, outputName);  // Apply and reinit
```

| Parameter | Value | Description |
|-----------|-------|-------------|
| Port | 4126 | Separate from game (4124) |
| Codec | Opus | VoIP optimized |
| Sample Rate | 48000 Hz | Opus standard |
| Frame Size | 960 samples | 20ms @ 48kHz |
| Bitrate | 32 kbps | Good quality/bandwidth |
| Channels | 1 (mono) | Sufficient for voice |
| Device Selection | Cross-platform | Auto-filters virtual devices |
| Persistence | MongoDB | audioInputDevice, audioOutputDevice fields |

## Server Architecture (Hexagonal)

### GameWorld

Central game state manager (`src/server/infrastructure/game/GameWorld.hpp`):

```cpp
class GameWorld {
    // Players
    void addPlayer(uint8_t id);
    void removePlayer(uint8_t id);
    void updatePlayerPosition(uint8_t id, uint16_t x, uint16_t y);

    // Missiles
    void spawnMissile(uint8_t playerId);       // Creates missile at player pos
    void updateMissiles(float deltaTime);      // Moves missiles, removes OOB
    std::vector<uint16_t> getDestroyedMissiles();

    GameSnapshot getSnapshot() const;          // For broadcast
};
```

### UDPServer

Network adapter (`src/server/infrastructure/adapters/in/network/UDPServer.hpp`):

- Handles incoming messages (MovePlayer, ShootMissile)
- Broadcasts GameSnapshot at 20Hz
- Broadcasts MissileSpawned/MissileDestroyed events

### VoiceUDPServer

Voice relay server (`src/server/infrastructure/adapters/in/network/VoiceUDPServer.hpp`):

- Listens on port 4126 (separate from game UDP)
- Authenticates clients via SessionToken (reuses game auth)
- Relays VoiceFrame packets to room members
- No audio processing - pure relay (Opus encoding is client-side)

## Game Constants

| Constant | Value | Location |
|----------|-------|----------|
| `MOVE_SPEED` | 200.0f | GameScene.hpp |
| `SHIP_WIDTH/HEIGHT` | 50.0f / 30.0f | GameScene.hpp, Hitboxes |
| `MISSILE_WIDTH/HEIGHT` | 32.0f / 16.0f (render), 16/8 (hitbox) | GameScene.hpp, Hitboxes |
| `MISSILE_SPEED` | 600.0f | GameWorld.hpp |
| `SHOOT_COOLDOWN` | 0.3f seconds | GameScene.hpp |
| `MAX_HEALTH` | 100 | GameScene.hpp |
| `MAX_PLAYERS` | 4 | Protocol.hpp |
| `MAX_MISSILES` | 32 | Protocol.hpp |
| `BROADCAST_RATE` | 20Hz (50ms) | UDPServer.cpp |
| `VOICE_UDP_PORT` | 4126 | Protocol.hpp |
| `MAX_OPUS_FRAME_SIZE` | 480 bytes | Protocol.hpp |
| `OPUS_SAMPLE_RATE` | 48000 Hz | OpusCodec.hpp |
| `OPUS_FRAME_SIZE` | 960 samples | OpusCodec.hpp |
| `AUDIO_DEVICE_NAME_LEN` | 64 bytes | Protocol.hpp |

### Boss System Constants

| Constant | Value | Location |
|----------|-------|----------|
| `BOSS_SPAWN_WAVE` | 10 | GameWorld.hpp |
| `BOSS_MAX_HEALTH` | 1000 | GameWorld.hpp |
| `BOSS_WIDTH/HEIGHT` | 150.0f / 120.0f | GameScene.hpp |
| `BOSS_PHASE1_THRESHOLD` | 65% HP | GameWorld.hpp |
| `BOSS_PHASE2_THRESHOLD` | 30% HP | GameWorld.hpp |
| `BOSS_ENRAGE_THRESHOLD` | 20% HP | GameWorld.hpp |
| `BOSS_ATTACK_COOLDOWN` | 2.0s (base) | GameWorld.hpp |

### Weapon System Constants

| Constant | Value | Location |
|----------|-------|----------|
| `WeaponType::Standard` | 0 | Protocol.hpp |
| `WeaponType::Spread` | 1 | Protocol.hpp |
| `WeaponType::Laser` | 2 | Protocol.hpp |
| `WeaponType::Missile` | 3 | Protocol.hpp |
| `MAX_WEAPON_TYPES` | 4 | Protocol.hpp |
| `SPREAD_ANGLE` | 15.0f deg | GameWorld.hpp |
| `LASER_SPEED_MULT` | 1.5x | GameWorld.hpp |
| `HOMING_TURN_RATE` | 2.0f rad/s | GameWorld.hpp |

### Score System Constants

| Constant | Value | Location |
|----------|-------|----------|
| `ENEMY_KILL_SCORE` | 100 | GameWorld.hpp |
| `BOSS_DEFEAT_SCORE` | 5000 | GameWorld.hpp |
| `COMBO_DECAY_TIME` | 3.0s | GameWorld.hpp |
| `COMBO_MAX_MULT` | 3.0x (30) | GameWorld.hpp |
| `COMBO_INCREMENT` | 0.1x per kill | GameWorld.hpp |

## Assets

| Asset | Path | Size |
|-------|------|------|
| Ship sprite | `assets/spaceship/Ship1.png` | 64x64 |
| Missile sprite | `assets/spaceship/missile.png` | 32x32 |

Loaded in GameScene via:
```cpp
_context.window->loadTexture("ship", "assets/spaceship/Ship1.png");
_context.window->loadTexture("missile", "assets/spaceship/missile.png");
```

## Key Files

| Purpose | Path |
|---------|------|
| **Protocol** | `src/common/protocol/Protocol.hpp` |
| **Collision** | `src/common/collision/AABB.hpp` |
| **Server main** | `src/server/main.cpp` |
| **UDP Server** | `src/server/infrastructure/adapters/in/network/UDPServer.cpp` |
| **GameWorld** | `src/server/infrastructure/game/GameWorld.cpp` |
| **Player entity** | `src/server/include/domain/entities/Player.hpp` |
| **Client main** | `src/client/main.cpp` |
| **GameScene** | `src/client/src/scenes/GameScene.cpp` |
| **UDPClient** | `src/client/src/network/UDPClient.cpp` |
| **IWindow** | `src/client/include/graphics/IWindow.hpp` |
| **SFMLWindow** | `src/client/lib/sfml/src/SFMLWindow.cpp` |
| **SDL2Window** | `src/client/lib/sdl2/src/SDL2Window.cpp` |
| **VoiceUDPServer** | `src/server/infrastructure/adapters/in/network/VoiceUDPServer.cpp` |
| **VoiceChatManager** | `src/client/src/audio/VoiceChatManager.cpp` |
| **OpusCodec** | `src/client/src/audio/OpusCodec.cpp` |

## Coding Conventions

### C++ Style

- **Standard**: C++23
- **Naming**:
  - Classes: `PascalCase`
  - Methods/Functions: `camelCase`
  - Private members: `_prefixedWithUnderscore`
  - Constants: `SCREAMING_SNAKE_CASE`
- **Headers**: `.hpp` for headers, `.cpp` for implementations
- **Namespaces**: Match directory structure (`collision::AABB`, `events::Key`)

### Architecture Rules

1. **Domain NEVER depends on Infrastructure**
2. **Value Objects are immutable** - return new instances
3. **Interfaces in headers, implementations in source files**
4. **Network protocol uses network byte order** (big-endian)

## Dependencies (vcpkg)

- **boost-asio**: Async networking (server)
- **gtest**: Testing framework
- **sfml**: Client graphics (SFML backend)
- **sdl2**, **sdl2-image**: Client graphics (SDL2 backend)
- **opus**: Audio codec for voice chat
- **portaudio**: Cross-platform audio I/O

## Git Conventions

Commit format: `TYPE: Description`

| Type | Usage |
|------|-------|
| FEAT | New feature |
| FIX | Bug fix |
| DOCS | Documentation |
| REFACTOR | Code refactoring |
| TEST | Adding tests |
| BUILD | Build system changes |

## MCP & AgentDB

The project includes Claude Code tooling in `.claude/`:

### Analysis Agents (8 agents)

| Agent | Phase | Use Case |
|-------|-------|----------|
| **analyzer** | 1 | Impact analysis (LOCAL/MODULE/GLOBAL) |
| **security** | 1 | Security audit, CWE detection, regressions |
| **reviewer** | 1 | Code review, patterns, conventions |
| **risk** | 2 | Risk score calculation (0-100) |
| **synthesis** | 2 | Aggregates reports from 4 agents |
| **sonar** | 2 | Enriches SonarQube issues with AgentDB |
| **meta-synthesis** | 3 | Deduplicates and merges all findings |
| **web-synthesizer** | 4 | Exports JSON for web interface |

### AgentDB MCP Tools

| Tool | Description |
|------|-------------|
| `get_file_context` | Full view of a file (symbols, deps, errors) |
| `get_symbol_callers` | Find all callers of a function (recursive) |
| `get_symbol_callees` | Find all functions called by a symbol |
| `get_file_impact` | Calculate impact of modifying a file |
| `get_error_history` | Historical bugs for a file/module |
| `get_patterns` | Code patterns applicable to a file |
| `get_architecture_decisions` | ADRs for a module |
| `search_symbols` | Search symbols by pattern |
| `get_file_metrics` | Complexity, lines, coverage |
| `get_module_summary` | Overview of a module |

### Jira MCP Tools

| Tool | Description |
|------|-------------|
| `get_issue` | Get Jira ticket by key |
| `search_issues` | JQL search |
| `get_issue_from_text` | Extract tickets from commit message |
| `get_project_info` | Project metadata |

### Commands

```bash
# Incremental analysis (from last checkpoint)
/analyze

# Full analysis (from merge-base)
/analyze --all

# Reset checkpoint to HEAD
/analyze --reset

# Analyze specific files
/analyze --files src/server/UDPServer.cpp

# AgentDB maintenance
python .claude/scripts/bootstrap.py            # Initial setup
python .claude/scripts/bootstrap.py --incremental  # Update
```

### Verdicts

| Score | Verdict | Action |
|-------|---------|--------|
| ≥80 | 🟢 APPROVE | Can merge |
| ≥60 | 🟡 REVIEW | Human review recommended |
| ≥40 | 🟠 CAREFUL | Deep review required |
| <40 | 🔴 REJECT | Do not merge |

## Notes for Claude

1. **Read files before modifying** - Use `Read` tool to understand context
2. **Follow Hexagonal Architecture** - Domain has no external dependencies
3. **Protocol is binary** - Use `to_bytes()`/`from_bytes()` with byte swap
4. **Multi-backend graphics** - Changes to IWindow require both SFML and SDL2
5. **Missiles are server-authoritative** - Client sends request, server spawns
6. **GameSnapshot is the source of truth** - Broadcast at 20Hz
7. **Use agentDB** - Query file context and impact before refactoring
8. **Never compile** - The user compiles the project themselves. If compilation is needed to verify changes, stop and ask the user to compile, then wait for their feedback before continuing.
