# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using an **Hexagonal Architecture** (Ports & Adapters). The project consists of a server and client communicating via UDP for real-time gameplay synchronization.

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | TCP 4125 (auth/TLS), UDP 4124 (game), UDP 4126 (voice) |
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
│   │   │   │   ├── use_cases/auth/      # Login, Register
│   │   │   │   └── ports/out/           # IUserRepository, IUserSettingsRepository, ILogger, etc.
│   │   │   └── infrastructure/          # Adapters, network, game
│   │   │       ├── adapters/in/network/ # UDPServer
│   │   │       ├── game/                # GameWorld (missiles, players)
│   │   │       └── logging/             # Logger
│   │   ├── domain/                      # Domain implementations
│   │   ├── application/                 # Application implementations
│   │   ├── infrastructure/              # Infrastructure implementations
│   │   └── scripts/                     # VPS deployment (systemd, wrapper)
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
│   │   │   ├── config/                  # ServerConfigManager
│   │   │   ├── ui/                      # ServerConfigPanel
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

# Run Server
./artifacts/server/linux/rtype_server  # Server (UDP 4124)

# Run Client (RECOMMENDED - uses wrapper script for voice chat support)
./scripts/run-client.sh                # Handles PipeWire/JACK audio automatically
./scripts/run-client.sh --server=51.254.137.175  # Connect to VPS

# Run Client (direct binary - voice chat may not work on Linux/PipeWire)
./artifacts/client/linux/rtype_client  # Only if run-client.sh unavailable

# Clean build
rm -rf build*/ artifacts/
```

> **Note:** On Linux with PipeWire, the `run-client.sh` script wraps the binary with `pw-jack` to enable voice chat support. Always prefer the script over the direct binary.

## Client Server Configuration

The client supports configurable server addresses with persistence.

### Command Line

```bash
# Connect to specific server (RECOMMENDED - via script)
./scripts/run-client.sh --server=51.254.137.175       # Uses default ports (4125/4124)
./scripts/run-client.sh --server=myserver.com:4125    # Custom port (UDP = TCP - 1)

# Direct binary (voice chat may not work on Linux/PipeWire)
./artifacts/client/linux/rtype_client --server=51.254.137.175
```

### In-App Configuration

- Press **S** during connection screen to open server config
- Click **SERVER** button on login screen (bottom left)
- Auto-shows after 3 failed connection attempts
- Quick connect buttons: **FRANCE** (51.254.137.175) / **LOCAL** (127.0.0.1)
- Config is saved to `~/.config/rtype/rtype_client.json` (Linux) or `%APPDATA%/RType/rtype_client.json` (Windows)

### Config File Format

```json
{
    "host": "51.254.137.175",
    "tcpPort": 4125,
    "udpPort": 4124,
    "voicePort": 4126
}
```

### Key Files

| File | Description |
|------|-------------|
| `src/client/include/config/ServerConfigManager.hpp` | Singleton for server config |
| `src/client/src/config/ServerConfigManager.cpp` | JSON persistence, cross-platform |
| `src/client/src/scenes/ConnectionScene.cpp` | Config UI when connection fails |
| `src/client/src/scenes/LoginScene.cpp` | SERVER button + config UI on login |

## Network Protocol

Binary protocol over UDP with network byte order (big-endian). All messages start with `UDPHeader` (12 bytes).

### Message Types

| Type | Value | Direction | Payload | Description |
|------|-------|-----------|---------|-------------|
| `HeartBeat` | 0x0001 | Both | - | Keep-alive |
| `Snapshot` | 0x0040 | S→C | GameSnapshot | Full game state (20Hz) |
| `PlayerInput` | 0x0060 | C→S | keys (2B), seq (2B) | Player input bitfield |
| `PlayerJoin` | 0x0070 | S→C | player_id (1B) | New player joined |
| `PlayerLeave` | 0x0071 | S→C | player_id (1B) | Player disconnected |
| `ShootMissile` | 0x0080 | C→S | - | Fire missile request |
| `MissileSpawned` | 0x0081 | S→C | MissileState (8B) | Missile created |
| `MissileDestroyed` | 0x0082 | S→C | missile_id (2B) | Missile removed |
| `EnemyDestroyed` | 0x0091 | S→C | enemy_id (2B) | Enemy killed (immediate) |
| `PlayerDamaged` | 0x00A0 | S→C | PlayerDamaged (3B) | Player took damage |
| `PlayerDied` | 0x00A1 | S→C | player_id (1B) | Player died |
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
| `ChargeStart` | 0x0400 | C→S | - | Start Wave Cannon charge |
| `ChargeRelease` | 0x0401 | C→S | charge_level (1B) | Release charged shot |
| `WaveCannonFired` | 0x0402 | S→C | WaveCannonState | Wave Cannon beam spawned |
| `PowerUpSpawned` | 0x0410 | S→C | PowerUpState | Power-up item appeared |
| `PowerUpCollected` | 0x0411 | S→C | PowerUpCollected | Power-up collected |
| `PowerUpExpired` | 0x0412 | S→C | powerup_id (2B) | Power-up disappeared |
| `ForceToggle` | 0x0420 | C→S | - | Toggle Force attach/detach |
| `ForceStateUpdate` | 0x0421 | S→C | ForceState | Force pod state changed |

### Key Structures

```cpp
// UDPHeader (12 bytes) - All messages
struct UDPHeader {
    uint16_t type;          // MessageType
    uint16_t sequence_num;
    uint64_t timestamp;     // Milliseconds since epoch
};

// PlayerState (23 bytes) - Includes score, weapon, and power-up levels
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
    uint8_t chargeLevel;    // Wave Cannon charge (0-3)
    uint8_t speedLevel;     // Speed upgrade level (0-3)
    uint8_t weaponLevel;    // Weapon upgrade level (0-3)
    uint8_t hasForce;       // Has Force Pod (0 or 1)
    uint8_t shieldTimer;    // Reserved (always 0, R-Type authentic has no shield)
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
    uint8_t force_count;
    ForceStateSnapshot forces[MAX_PLAYERS]; // Force Pods (Phase 3)
};

// PlayerDamaged (3 bytes) - Damage notification
struct PlayerDamaged {
    uint8_t player_id;
    uint8_t damage;
    uint8_t new_health;
};

// EnemyDestroyed (2 bytes) - Enemy killed notification
struct EnemyDestroyed {
    uint16_t enemy_id;
};

// ForceStateSnapshot (7 bytes) - Force Pod in snapshot
struct ForceStateSnapshot {
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t is_attached;
    uint8_t level;            // 0 = no force, 1-2 = force level
};

// VoiceFrame (5-485 bytes) - Opus-encoded audio
struct VoiceFrame {
    uint8_t speaker_id;       // Who is speaking
    uint16_t sequence;        // For packet loss detection
    uint16_t opus_len;        // Actual Opus data length
    uint8_t opus_data[480];   // Opus-encoded audio (max 480 bytes)
};

// R-Type Authentic (Phase 3) Structures

// WaveCannonState (9 bytes) - Charged beam projectile
struct WaveCannonState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t charge_level;     // 1-3
    uint8_t width;            // Beam width
};

// PowerUpState (8 bytes) - Collectible power-up
struct PowerUpState {
    uint16_t id;
    uint16_t x, y;
    uint8_t type;             // PowerUpType enum
    uint8_t remaining_time;   // Seconds before expiration
};

// PowerUpCollected (4 bytes) - Collection notification
struct PowerUpCollected {
    uint16_t powerup_id;
    uint8_t player_id;
    uint8_t powerup_type;
};

// ForceState (7 bytes) - Force Pod state
struct ForceState {
    uint8_t owner_id;
    uint16_t x, y;
    uint8_t is_attached;
    uint8_t level;            // 0 = no force, 1-2 = force level
};

// PowerUpType enum (R-Type Authentic - no Shield, defense via Force Pod)
enum class PowerUpType : uint8_t {
    Health = 0,               // +25 HP (red)
    SpeedUp = 1,              // +1 speed level (blue crystal)
    WeaponCrystal = 2,        // +1 weapon level (red crystal)
    ForcePod = 3,             // Gives/upgrades Force Pod (orange orb)
    BitDevice = 4,            // Gives 2 orbiting Bit Devices (purple)
    COUNT = 5
};

// BitDeviceStateSnapshot (6 bytes) - Bit Device in snapshot
struct BitDeviceStateSnapshot {
    uint8_t owner_id;
    uint8_t bit_index;        // 0 = first bit, 1 = second bit
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

- Handles incoming messages (PlayerInput, ShootMissile)
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
| `BOSS_MAX_HEALTH` | 1500 (base) | GameWorld.hpp |
| `BOSS_HP_PER_CYCLE` | +500 HP | GameWorld.cpp |
| `BOSS_CYCLE_BONUS` | +1000 pts | GameWorld.cpp |
| `BOSS_WIDTH/HEIGHT` | 150.0f / 120.0f | GameScene.hpp |
| `BOSS_PHASE1_THRESHOLD` | 65% HP | GameWorld.hpp |
| `BOSS_PHASE2_THRESHOLD` | 30% HP | GameWorld.hpp |
| `BOSS_ENRAGE_THRESHOLD` | 20% HP | GameWorld.hpp |
| `BOSS_ATTACK_COOLDOWN` | 2.0s (base) | GameWorld.hpp |

**Boss Respawn System:** Boss spawns every 10 waves (10, 20, 30...) with HP = 1500 + (cycle-1) × 500.

### R-Type Authentic (Phase 3) Constants

| Constant | Value | Location |
|----------|-------|----------|
| `CHARGE_TIME_LV1` | 0.6s | Protocol.hpp (WaveCannon) |
| `CHARGE_TIME_LV2` | 1.3s | Protocol.hpp (WaveCannon) |
| `CHARGE_TIME_LV3` | 2.2s | Protocol.hpp (WaveCannon) |
| `WIDTH_LV1` | 20.0f | Protocol.hpp (beam width) |
| `WIDTH_LV2` | 35.0f | Protocol.hpp (beam width) |
| `WIDTH_LV3` | 55.0f | Protocol.hpp (beam width) |
| `DAMAGE_LV1` | 50 | Protocol.hpp (WaveCannon) |
| `DAMAGE_LV2` | 100 | Protocol.hpp (WaveCannon) |
| `DAMAGE_LV3` | 250 | Protocol.hpp (WaveCannon) |
| `WAVE_CANNON_SPEED` | 850.0f | Protocol.hpp (beam speed) |
| `POWERUP_DROP_CHANCE` | 8% | GameWorld.hpp |
| `POWERUP_POW_ARMOR_CHANCE` | 50% | GameWorld.hpp (POWArmor enemy) |
| `POWERUP_LIFETIME` | 10.0s | GameWorld.hpp |
| `POWERUP_SIZE` | 32.0f | GameScene.hpp |
| `FORCE_POD_WIDTH/HEIGHT` | 32.0f / 32.0f | GameScene.hpp |
| `FORCE_POD_CONTACT_DAMAGE` | 30 | GameWorld.hpp |
| `BIT_DEVICE_SIZE` | 24.0f | GameScene.hpp |
| `BIT_DEVICE_ORBIT_RADIUS` | 50.0f | GameWorld.hpp |
| `BIT_DEVICE_ORBIT_SPEED` | 3.0 rad/s | GameWorld.hpp |
| `BIT_DEVICE_CONTACT_DAMAGE` | 15 | GameWorld.hpp |
| `BIT_DEVICE_SHOOT_COOLDOWN` | 0.4s | GameWorld.hpp |
| `MAX_BITS` | 8 (2×4 players) | Protocol.hpp |
| `SHIELD_DURATION` | 3.0s | GameWorld.cpp |
| `SPEED_UP_MAX_LEVEL` | 3 | Protocol.hpp |
| `SPEED_MULTIPLIERS` | 1.0/1.3/1.6/1.9 | GameWorld.cpp |
| `SPEEDUP_MAX_BONUS` | +500 pts | GameWorld.cpp (when already at max)

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
| `POINTS_BASIC` | 100 | GameWorld.hpp |
| `POINTS_TRACKER` | 150 | GameWorld.hpp |
| `POINTS_ZIGZAG` | 120 | GameWorld.hpp |
| `POINTS_FAST` | 180 | GameWorld.hpp |
| `POINTS_BOMBER` | 250 | GameWorld.hpp |
| `POINTS_POW_ARMOR` | 200 | GameWorld.hpp |
| `POINTS_BOSS` | 5000 | GameWorld.hpp |
| `COMBO_DECAY_TIME` | 3.0s | GameWorld.hpp |
| `COMBO_MAX_MULT` | 3.0x | GameWorld.hpp |
| `COMBO_INCREMENT` | 0.1x per kill | GameWorld.hpp |

### Enemy Type Constants (GameWorld.hpp)

| Type | Value | Speed | HP | Shoot Interval | Points | Special |
|------|-------|-------|----|----|--------|---------|
| Basic | 0 | -120 | 40 | 2.5s | 100 | - |
| Tracker | 1 | -100 | 35 | 2.0s | 150 | Follows player Y |
| Zigzag | 2 | -140 | 30 | 3.0s | 120 | Zig-zag movement |
| Fast | 3 | -220 | 25 | 1.5s | 180 | Very fast |
| Bomber | 4 | -80 | 80 | 1.0s | 250 | Tanky, rapid fire |
| POWArmor | 5 | -90 | 60 | 4.0s | 200 | 50% power-up drop |

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
| **TCPAuthServer** | `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp` |
| **TCPClient** | `src/client/src/network/TCPClient.cpp` |
| **SessionManager** | `src/server/infrastructure/session/SessionManager.cpp` |
| **ServerConfigManager** | `src/client/src/config/ServerConfigManager.cpp` |
| **Server wrapper** | `src/server/scripts/server-wrapper.py` |
| **Systemd service** | `src/server/scripts/rtype_server.service` |

## TLS Security

Authentication traffic (TCP port 4125) is encrypted using TLS 1.2+.

### Configuration

| Parameter | Value |
|-----------|-------|
| Port | TCP 4125 |
| Protocol | TLS 1.2 minimum |
| Cipher Suites | ECDHE + AES-GCM / ChaCha20-Poly1305 |
| Certificate | `certs/server.crt` (auto-generated for dev) |
| Private Key | `certs/server.key` (auto-generated for dev) |

### Certificate Generation

```bash
./scripts/generate_dev_certs.sh        # Creates certs/ directory
```

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `TLS_CERT_FILE` | `certs/server.crt` | Path to TLS certificate |
| `TLS_KEY_FILE` | `certs/server.key` | Path to TLS private key |

### Security Notes

- Session tokens are generated using OpenSSL `RAND_bytes()` (CSPRNG)
- Client uses `verify_none` for self-signed certificates (development only)
- In production, use `verify_peer` with proper CA certificates

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

## Hidden Features (Developer Only)

### GodMode

Hidden invincibility feature for testing/debugging. Not visible to other players.

| Aspect | Details |
|--------|---------|
| **Activation** | Type `/toggleGodMode` in chat during gameplay |
| **Effect** | Player takes no damage from enemy missiles |
| **Visibility** | Command not broadcast, other players unaware |
| **Persistence** | Saved to MongoDB (user_settings.godMode) |
| **Auto-load** | State restored on login |

**Architecture Flow:**
```
[Chat: /toggleGodMode] → TCPAuthServer (intercept, not broadcast)
        ↓
SessionManager.toggleGodMode() → GodModeChangedCallback
        ↓
UDPServer → GameWorld.setPlayerGodMode()
        ↓
checkCollisions() skips damage if player.godMode == true
```

**Key Files:**
| File | Role |
|------|------|
| `SessionManager.hpp/cpp` | Session state, callbacks, toggle logic |
| `TCPAuthServer.cpp` | Command interception, DB persistence |
| `UDPServer.cpp` | Callback registration, initial state on join |
| `GameWorld.cpp` | Damage bypass in collision checks |
| `IUserSettingsRepository.hpp` | `godMode` field in UserSettingsData |

## Notes for Claude

1. **Read files before modifying** - Use `Read` tool to understand context
2. **Follow Hexagonal Architecture** - Domain has no external dependencies
3. **Protocol is binary** - Use `to_bytes()`/`from_bytes()` with byte swap
4. **Multi-backend graphics** - Changes to IWindow require both SFML and SDL2
5. **Missiles are server-authoritative** - Client sends request, server spawns
6. **GameSnapshot is the source of truth** - Broadcast at 20Hz
7. **Use agentDB** - Query file context and impact before refactoring
8. **Never compile** - The user compiles the project themselves. If compilation is needed to verify changes, stop and ask the user to compile, then wait for their feedback before continuing.
