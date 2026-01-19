# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using an **Hexagonal Architecture** (Ports & Adapters). The project consists of a server and client communicating via UDP for real-time gameplay synchronization.

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | TCP 4125 (auth/TLS), UDP 4124 (game), UDP 4126 (voice), TCP 4127 (admin) |
| Client | C++23, SFML/SDL2 (multi-backend) | - |
| Build | CMake 3.30+, Ninja, vcpkg, Nix | - |
| Discord Bot (Admin) | Python 3.12, discord.py | TCP 4127 (connects to server) |
| Discord Bot (Leaderboard) | Python 3.12, discord.py, motor | MongoDB (connects to database) |

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
├── discord-bot/                         # Discord bots
│   ├── admin/                           # Admin bot (TCPAdminServer client)
│   └── leaderboard/                     # Leaderboard bot (MongoDB client)
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
| `PlayerInput` | 0x0061 | C→S | keys (2B), seq (2B) | Player input bitfield |
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
    uint8_t weaponLevel;    // Current weapon's upgrade level (0-3, each weapon has independent levels)
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
        SHIP_WIDTH = 64.0f, SHIP_HEIGHT = 30.0f;
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

### Fullscreen & Window Resize

The client supports fullscreen toggle and automatic window resize handling with letterboxing.

| Feature | Key/Trigger | Description |
|---------|-------------|-------------|
| Toggle Fullscreen | F11 | Switches between windowed (1920x1080) and fullscreen desktop mode |
| Letterboxing | Automatic | Maintains 16:9 aspect ratio with black bars on non-16:9 screens |
| Mouse Mapping | Automatic | Mouse coordinates are converted to logical 1920x1080 space |

**Implementation:**
- SFML: Uses `sf::View` with viewport adjustment and `mapPixelToCoords()` for mouse
- SDL2: Uses `SDL_RenderSetLogicalSize()` which handles both scaling and mouse mapping

**Platform Notes:**
- Ubuntu/Linux: Window may be resized by window manager (taskbar), letterboxing handles this
- Windows/macOS: Fullscreen desktop mode (borderless) for fast Alt+Tab

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
| `SHIP_WIDTH/HEIGHT` | 64.0f / 30.0f | GameScene.hpp, Hitboxes |
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
| `BOSS_HP_PER_PLAYER` | +1000 HP | GameWorld.cpp |
| `BOSS_CYCLE_BONUS` | +1000 pts | GameWorld.cpp |
| `BOSS_WIDTH/HEIGHT` | 150.0f / 120.0f | GameScene.hpp |
| `BOSS_PHASE1_THRESHOLD` | 65% HP | GameWorld.hpp |
| `BOSS_PHASE2_THRESHOLD` | 30% HP | GameWorld.hpp |
| `BOSS_ENRAGE_THRESHOLD` | 20% HP | GameWorld.hpp |
| `BOSS_ATTACK_COOLDOWN` | 2.0s (base) | GameWorld.hpp |

**Boss Respawn System:** Boss spawns every 10 waves (10, 20, 30...) with HP = 1500 + (cycle-1) × 500 + (playerCount-1) × 1000.

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

## Leaderboard & Achievements System

Global leaderboards with player statistics and achievements, persisted in MongoDB.

### Architecture

```
[Client: LeaderboardScene] ←TCP→ [TCPAuthServer]
                                        ↓
                              [ILeaderboardRepository]
                                        ↓
                           [MongoDBLeaderboardRepository]
                                        ↓
                                    [MongoDB]
```

### MongoDB Collections

| Collection | Description |
|------------|-------------|
| `leaderboard` | Top scores with player info |
| `player_stats` | Cumulative statistics per player |
| `game_history` | Individual game records |
| `achievements` | Unlocked achievement timestamps |

### TCP Protocol Messages (Leaderboard)

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `GetLeaderboard` | 0x0500 | C→S | Request leaderboard (period + limit) |
| `LeaderboardData` | 0x0501 | S→C | Leaderboard entries response |
| `GetPlayerStats` | 0x0502 | C→S | Request own stats |
| `PlayerStatsData` | 0x0503 | S→C | Player statistics response |
| `GetGameHistory` | 0x0504 | C→S | Request game history |
| `GameHistoryData` | 0x0505 | S→C | Game history entries |
| `GetAchievements` | 0x0506 | C→S | Request achievements |
| `AchievementsData` | 0x0507 | S→C | Achievement bitfield |

### Wire Structures

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
    uint8_t playerCountFilter;  // Echo back requested filter
    // Followed by count * LeaderboardEntryWire
};

// PlayerStatsWire (76 bytes)
struct PlayerStatsWire {
    uint32_t gamesPlayed;
    uint32_t totalKills;
    uint32_t totalDeaths;
    uint32_t totalScore;
    uint32_t bestScore;
    uint16_t bestWave;
    uint16_t bestCombo;     // x10 (30 = 3.0x)
    uint32_t bossKills;
    uint32_t totalPlaytime; // Seconds
    uint32_t standardKills;
    uint32_t spreadKills;
    uint32_t laserKills;
    uint32_t missileKills;
    uint32_t achievements;  // Bitfield
};

// GameHistoryEntryWire (24 bytes per entry)
struct GameHistoryEntryWire {
    uint32_t score;
    uint16_t wave;
    uint16_t kills;
    uint16_t deaths;
    uint32_t duration;    // Seconds
    uint64_t timestamp;
};
```

### Leaderboard Filtering

The leaderboard supports filtering by player count (game mode):

| Filter | Description |
|--------|-------------|
| 0 | All modes combined |
| 1 | Solo (1 player) |
| 2 | Duo (2 players) |
| 3 | Trio (3 players) |
| 4-6 | 4P to 6P |

**Client UI:** In `LeaderboardScene`, buttons for "ALL", "SOLO", "DUO", "TRIO", "4P", "5P", "6P" allow filtering.
When "ALL" is selected, a "MODE" column displays the game mode for each entry.

### Achievements System

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

**Achievement Checker:** `src/server/application/services/AchievementChecker.hpp`

### Leaderboard Periods

| Period | Value | Filter |
|--------|-------|--------|
| All-Time | 0 | No filter |
| Weekly | 1 | Last 7 days |
| Monthly | 2 | Last 30 days |

### Client UI (LeaderboardScene)

Accessible via **LEADERBOARD** button in main menu.

**Tabs:**
1. **Leaderboard** - Top 50 players with rank, name, score, wave, kills
2. **Stats** - Personal statistics (games, K/D, playtime, weapon usage)
3. **Achievements** - 10 achievement badges (locked/unlocked)

**Navigation:**
- Tab buttons at top
- Period filter buttons (All-Time/Weekly/Monthly) for Leaderboard tab
- **BACK** button returns to main menu
- Scroll support for long lists

### Key Files

| File | Description |
|------|-------------|
| `src/server/include/application/ports/out/persistence/ILeaderboardRepository.hpp` | Repository interface |
| `src/server/infrastructure/adapters/out/persistence/MongoDBLeaderboardRepository.cpp` | MongoDB implementation |
| `src/server/include/application/services/AchievementChecker.hpp` | Achievement logic |
| `src/client/include/scenes/LeaderboardScene.hpp` | Client scene header |
| `src/client/src/scenes/LeaderboardScene.cpp` | Client scene implementation |
| `src/client/include/network/NetworkEvents.hpp` | TCP event types |
| `src/common/protocol/Protocol.hpp` | Wire format structures |

### Usage Example (Client)

```cpp
// In MainMenuScene::onLeaderboardClick()
auto& sceneManager = SceneManager::getInstance();
sceneManager.changeScene(std::make_unique<LeaderboardScene>());

// In LeaderboardScene - request data
auto& tcpClient = TCPClient::getInstance();
tcpClient.sendGetLeaderboard({.period = 0, .limit = 50});
tcpClient.sendGetPlayerStats();
tcpClient.sendGetAchievements();

// Handle response events in update()
while (auto event = tcpClient.pollEvent()) {
    if (auto* data = std::get_if<LeaderboardDataEvent>(&*event)) {
        // Update UI with data->response.entries
    }
}
```

### Real-Time Rank Display (GameScene)

During gameplay, the player's global rank and personal best score are displayed in the HUD.

**Features:**
- **Global rank badge** - Shows current position (e.g., "RANK #42")
- **Personal best score** - Target to beat (e.g., "BEST: 32.6K")
- **Real-time updates** - Rank refreshed every 10 seconds
- **Visual feedback** - Best score turns green when current score exceeds it

**Rank Colors:**
| Position | Color |
|----------|-------|
| #1 | Gold (255, 215, 0) |
| #2 | Silver (192, 192, 192) |
| #3 | Bronze (205, 127, 50) |
| Top 10 | Light Blue (100, 200, 255) |
| Top 50 | Green (100, 255, 150) |
| Others | Gray (200, 200, 200) |

**Score Formatting:**
- `< 1000` → Raw number (e.g., "BEST: 850")
- `≥ 1000` → K format (e.g., "BEST: 32.6K")
- `≥ 1000000` → M format (e.g., "BEST: 1.2M")

**Implementation:**
```cpp
// GameScene requests rank and stats at start
_context.tcpClient->sendGetLeaderboard({.period = 0, .limit = 1});
_context.tcpClient->sendGetPlayerStats();  // For bestScore

// Periodic update every 10 seconds
if (_rankUpdateTimer >= RANK_UPDATE_INTERVAL) {
    _context.tcpClient->sendGetLeaderboard({.period = 0, .limit = 1});
    _rankUpdateTimer = 0.0f;
}
```

**Key Files:**
| File | Description |
|------|-------------|
| `src/client/include/scenes/GameScene.hpp` | Rank state variables |
| `src/client/src/scenes/GameScene.cpp` | `renderGlobalRank()` function |

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
| **TCPAdminServer** | `src/server/infrastructure/adapters/in/network/TCPAdminServer.cpp` |
| **TCPClient** | `src/client/src/network/TCPClient.cpp` |
| **SessionManager** | `src/server/infrastructure/session/SessionManager.cpp` |
| **LeaderboardScene** | `src/client/src/scenes/LeaderboardScene.cpp` |
| **ILeaderboardRepository** | `src/server/include/application/ports/out/persistence/ILeaderboardRepository.hpp` |
| **AchievementChecker** | `src/server/application/services/AchievementChecker.cpp` |

### Unit Tests (Leaderboard)

| Test File | Coverage |
|-----------|----------|
| `tests/server/application/services/AchievementCheckerTest.cpp` | All 10 achievements, edge cases |
| `tests/server/application/services/LeaderboardDataTest.cpp` | PlayerStats, GameHistoryEntry, enums |

**Run tests:**
```bash
./scripts/compile.sh && ./artifacts/tests/server_tests --gtest_filter="Achievement*:PlayerStats*:Leaderboard*"
```
| **ServerConfigManager** | `src/client/src/config/ServerConfigManager.cpp` |
| **Server wrapper** | `src/server/scripts/server-wrapper.py` |
| **Systemd service** | `src/server/scripts/rtype_server.service` |
| **Version (client)** | `src/client/include/core/Version.hpp` |
| **VersionHistoryManager** | `src/server/include/infrastructure/version/VersionHistoryManager.hpp` |
| **Platform utils** | `src/client/include/utils/Platform.hpp` |

## Version Checking System

Le système de vérification de version compare les hash git entre le client et le serveur à la connexion.

### Architecture

```
[Client: LoginScene] ←TCP→ [TCPAuthServer]
        ↓                        ↓
  Version.hpp            VersionHistoryManager
  (getClientVersion)      (loadFromFile)
        ↓                        ↓
   VersionInfo    ←────→   VersionHistory
   (13 bytes)              (451 bytes)
```

### Structures Wire

```cpp
// VersionInfo (13 bytes) - Envoyé dans AuthResponseWithToken
struct VersionInfo {
    uint8_t major, minor, patch;
    uint8_t flags;              // Bit 0: isDev
    char gitHash[9];            // 8 chars + null
};

// VersionHistory (451 bytes) - 50 derniers commits
struct VersionHistory {
    uint8_t count;
    char hashes[50][9];         // Newest first
};
```

### Flux de Vérification

1. **Login** → Client envoie credentials
2. **Auth Success** → Serveur répond avec `serverVersion` + `versionHistory`
3. **Client compare** → `isVersionCompatible(client, server)`
4. **Si mismatch** → Popup avec nombre de commits de retard + bouton JENKINS

### Mode Développement

Créer un fichier `version.dev` à la racine pour bypasser la vérification :

```bash
touch version.dev    # Active le mode dev (gitignored)
```

### Variable d'Environnement

```bash
RTYPE_TEST_HASH=abc1234 ./rtype_client   # Simule un hash différent
```

### Fichiers Clés

| Fichier | Description |
|---------|-------------|
| `src/client/include/core/Version.hpp` | getClientVersion(), isDevMode(), formatVersion() |
| `src/server/include/infrastructure/version/VersionHistoryManager.hpp` | Singleton, loadFromFile(), getCommitsBehind() |
| `src/common/protocol/Protocol.hpp` | VersionInfo, VersionHistory structs |
| `src/client/src/scenes/LoginScene.cpp` | Popup mismatch, bouton JENKINS |
| `scripts/generate_version_history.sh` | Script helper pour générer l'historique |

### CI/CD

**Jenkinsfile** génère `version_history.txt` au checkout :
```groovy
sh 'git log --format="%h" -n 50 > version_history.txt'
```

**deploy-service.py** régénère l'historique à chaque déploiement VPS.

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

## TCP Admin Server (Remote Administration)

The TCPAdminServer provides remote administration capabilities via a JSON-RPC protocol over TCP.

### Configuration

| Parameter | Value |
|-----------|-------|
| Port | TCP 4127 |
| Bind Address | `127.0.0.1` (localhost only - not accessible from outside) |
| Protocol | JSON-RPC over TCP (newline-delimited) |
| Authentication | Token-based (ADMIN_TOKEN environment variable) |

### Security

The TCPAdminServer implements multiple security layers:

| Layer | Protection |
|-------|------------|
| **Network** | Binds to `127.0.0.1` only - cannot be accessed from external networks |
| **Authentication** | 256-bit token required (`openssl rand -hex 32`) |
| **Command filtering** | Dangerous commands blocked: `quit`, `exit`, `zoom`, `interact`, `net` |
| **Thread safety** | Mutex-protected token validation and connection tracking |

**Security notes:**
- If `ADMIN_TOKEN` is not set, the server refuses ALL requests
- Token comparison is exact match (no partial matching)
- Each connection is logged with source IP/port

### Protocol

**Request format:**
```json
{"cmd": "command_name", "args": "optional arguments", "token": "admin_token"}
```

**Response format:**
```json
{"success": true, "output": ["line1", "line2", ...], "error": "if any"}
```

### Available Commands

All ServerCLI commands are available except interactive ones:

| Command | Description |
|---------|-------------|
| `status` | Server status (users, sessions, uptime) |
| `sessions` | List active sessions |
| `users` | List registered users |
| `user <email>` | User details |
| `kick <email>` | Disconnect a player |
| `ban <email> [reason]` | Ban a user |
| `unban <email>` | Unban a user |
| `bans` | List banned users |
| `rooms` | List active game rooms |
| `room <code>` | Room details |
| `broadcast <msg>` | Send message to all players |
| `help` | List available commands |

**Blocked commands** (require local TUI): `quit`, `exit`, `zoom`, `interact`, `net`

### Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `ADMIN_TOKEN` | Yes | Secret token for authentication (min 32 chars recommended) |

**Generate a secure token:**
```bash
openssl rand -hex 32
```

### Key Files

| File | Description |
|------|-------------|
| `src/server/include/infrastructure/adapters/in/network/TCPAdminServer.hpp` | Header |
| `src/server/infrastructure/adapters/in/network/TCPAdminServer.cpp` | Implementation |
| `src/server/infrastructure/cli/ServerCLI.cpp` | `executeCommandWithOutput()` method |

## Discord Admin Bot

Python Discord bot that connects to TCPAdminServer for remote server administration.

### Architecture

```
[Discord] ←→ [Discord Bot (Python)] ←TCP 4127→ [TCPAdminServer (C++)] ←→ [ServerCLI]
```

### Setup

```bash
cd discord-bot/admin
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
# Edit .env with your configuration
python bot.py
```

### Configuration (.env)

| Variable | Description |
|----------|-------------|
| `DISCORD_TOKEN` | Discord bot token |
| `ADMIN_CHANNEL_ID` | Channel ID where commands are allowed |
| `ADMIN_ROLE_ID` | Role ID required to use commands |
| `RTYPE_SERVER_HOST` | R-Type server hostname (default: localhost) |
| `RTYPE_ADMIN_PORT` | TCPAdminServer port (default: 4127) |
| `ADMIN_TOKEN` | Must match server's ADMIN_TOKEN |
| `LOG_LEVEL` | Logging level (default: INFO) |

### Slash Commands

| Command | Description |
|---------|-------------|
| `/status` | Show server status |
| `/sessions` | List active sessions |
| `/rooms` | List active game rooms |
| `/room <code>` | Show room details |
| `/users` | List registered users |
| `/user <email>` | Show user details |
| `/kick <email>` | Kick a player |
| `/ban <email> [reason]` | Ban a user |
| `/unban <email>` | Unban a user |
| `/bans` | List banned users |
| `/broadcast <message>` | Send to all players |
| `/say <room> <message>` | Send to specific room |
| `/cli <command>` | Execute any CLI command |
| `/help` | Show help |

### Key Files

| File | Description |
|------|-------------|
| `discord-bot/admin/bot.py` | Main bot entry point |
| `discord-bot/admin/tcp_client.py` | Async TCP client for TCPAdminServer |
| `discord-bot/admin/config.py` | Configuration from environment |
| `discord-bot/admin/cogs/admin.py` | Server management commands |
| `discord-bot/admin/cogs/users.py` | User management commands |
| `discord-bot/admin/cogs/moderation.py` | Ban/kick commands |
| `discord-bot/admin/utils/embeds.py` | Discord embed generators |

### Systemd Service (VPS)

```ini
[Unit]
Description=R-Type Discord Admin Bot
After=network.target rtype-server.service

[Service]
Type=simple
User=alexandre
WorkingDirectory=/opt/rtype/discord-bot/admin
ExecStart=/opt/rtype/discord-bot/admin/venv/bin/python bot.py
Restart=always
RestartSec=10
Environment="ADMIN_TOKEN=your_token_here"

[Install]
WantedBy=multi-user.target
```

## Discord Leaderboard Bot

Python Discord bot that queries MongoDB to display player statistics, leaderboards, and achievements.

### Architecture

```
[Discord] ←→ [Leaderboard Bot (Python)] ←MongoDB→ [player_stats, leaderboard, achievements]
```

### Setup

```bash
cd discord-bot/leaderboard
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
# Edit .env with your configuration
python bot.py
```

### Configuration (.env)

| Variable | Description |
|----------|-------------|
| `DISCORD_TOKEN` | Discord bot token |
| `MONGODB_URI` | MongoDB connection string |
| `MONGODB_DATABASE` | Database name (default: rtype) |
| `DISCORD_GUILD_ID` | Guild ID for fast command sync (dev only, optional) |
| `STATS_CHANNEL_ID` | Channel ID for stats updates (optional) |
| `ALLOWED_CHANNEL_IDS` | Comma-separated channel IDs where bot responds (empty = all) |
| `LOG_LEVEL` | Logging level (default: INFO) |
| `EMBED_COLOR` | Default embed color in hex (default: 0x00FF00) |

**Channel IDs (R-Type Discord):**
- `#r-type-leaderboard`: `1461894332435927226`
- `#r-type-admin`: `1462054690152775833`

### Slash Commands

| Command | Description |
|---------|-------------|
| `/leaderboard [category] [period] [mode] [limit]` | Display leaderboard (score/kills/wave/kd/bosses/playtime) |
| `/daily [mode]` | Shortcut for top 10 of last 24h |
| `/stats <player>` | Player statistics (K/D, records, activity) |
| `/kills <player>` | Kills breakdown by weapon type |
| `/achievements <player>` | Player achievements with rarity % |
| `/history <player> [limit]` | Recent game history (default 5, max 10) |
| `/rank <player> [mode]` | Player ranking across periods |
| `/compare <player1> <player2>` | Compare two players |
| `/weapon <type> [limit]` | Top players by specific weapon with % usage (default 10, max 50) |
| `/online` | Currently active game sessions |
| `/server-stats` | Server-wide statistics |

### Features

| Feature | Description |
|---------|-------------|
| **Pagination** | Button navigation for large leaderboards (⏮️◀️▶️⏭️) |
| **Achievement Rarity** | Shows % of players who have each achievement (💎<5% 🟣<15% 🔵<30% 🟢<50% ⚪50%+) |
| **Weapon Stats** | % utilization per weapon type |
| **Mode Filtering** | Filter by Solo/Duo/Trio/4P (👤👥) |
| **Period Filtering** | All-Time/Daily/Weekly/Monthly |
| **Player Autocomplete** | Search players as you type |

### Key Files

| File | Description |
|------|-------------|
| `discord-bot/leaderboard/bot.py` | Main bot entry point |
| `discord-bot/leaderboard/database/mongodb.py` | MongoDB connection singleton |
| `discord-bot/leaderboard/database/leaderboard_repo.py` | Leaderboard queries |
| `discord-bot/leaderboard/database/player_stats_repo.py` | Player stats & achievements |
| `discord-bot/leaderboard/database/session_repo.py` | Active sessions queries |
| `discord-bot/leaderboard/cogs/leaderboard.py` | /leaderboard, /daily, /weapon, /rank |
| `discord-bot/leaderboard/cogs/stats.py` | /stats, /kills, /compare, /server-stats |
| `discord-bot/leaderboard/cogs/achievements.py` | /achievements |
| `discord-bot/leaderboard/cogs/history.py` | /history |
| `discord-bot/leaderboard/cogs/online.py` | /online |
| `discord-bot/leaderboard/utils/embeds.py` | Discord embed generators |
| `discord-bot/leaderboard/utils/formatters.py` | Number/duration/timestamp formatting |
| `discord-bot/leaderboard/utils/pagination.py` | Paginated views with buttons |

### MongoDB Collections Used

| Collection | Fields Used |
|------------|-------------|
| `player_stats` | playerName, totalKills, totalDeaths, bestScore, bestWave, achievements (bitfield), weapon kills |
| `leaderboard` | playerName, score, wave, kills, timestamp, playerCount |
| `game_history` | email, score, wave, kills, deaths, duration, timestamp, weapon kills |
| `achievements` | email, type (bit), unlockedAt |
| `users` | username, email |
| `current_sessions` | roomCode, players, currentWave |

### Systemd Service (VPS)

```ini
[Unit]
Description=R-Type Discord Leaderboard Bot
After=network.target mongod.service

[Service]
Type=simple
User=alexandre
WorkingDirectory=/opt/rtype/discord-bot/leaderboard
ExecStart=/opt/rtype/discord-bot/leaderboard/venv/bin/python bot.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

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
