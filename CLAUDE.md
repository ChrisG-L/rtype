# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using an **Hexagonal Architecture** (Ports & Adapters). The project consists of a server and client communicating via UDP for real-time gameplay synchronization.

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | UDP 4124 |
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
│   │   │   ├── network/                 # UDPClient
│   │   │   ├── graphics/                # IWindow, IDrawable, Graphics
│   │   │   ├── gameplay/                # EntityManager, GameObject, Missile
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
| `MovePlayer` | 0x0060 | C→S | x, y (4B) | Player position update |
| `PlayerJoin` | 0x0070 | S→C | player_id (1B) | New player joined |
| `PlayerLeave` | 0x0071 | S→C | player_id (1B) | Player disconnected |
| `ShootMissile` | 0x0080 | C→S | - | Fire missile request |
| `MissileSpawned` | 0x0081 | S→C | MissileState (7B) | Missile created |
| `MissileDestroyed` | 0x0082 | S→C | missile_id (2B) | Missile removed |

### Key Structures

```cpp
// UDPHeader (12 bytes) - All messages
struct UDPHeader {
    uint16_t type;          // MessageType
    uint16_t sequence_num;
    uint64_t timestamp;     // Milliseconds since epoch
};

// PlayerState (7 bytes)
struct PlayerState {
    uint8_t id;
    uint16_t x, y;
    uint8_t health;         // 0-100
    uint8_t alive;          // 0 or 1
};

// MissileState (7 bytes)
struct MissileState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x, y;
};

// GameSnapshot (variable size) - Broadcast at 20Hz
struct GameSnapshot {
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];  // MAX_PLAYERS = 4
    uint8_t missile_count;
    MissileState missiles[MAX_MISSILES]; // MAX_MISSILES = 32
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
