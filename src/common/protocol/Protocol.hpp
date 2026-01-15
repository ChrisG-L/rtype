/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Protocol
*/

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include <cstdint>
#include <cstring>
#include <chrono>
#include <optional>
#include <string>

inline uint64_t swap64(uint64_t v) { return __builtin_bswap64(v); }
inline uint32_t swap32(uint32_t v) { return __builtin_bswap32(v); }
inline uint16_t swap16(uint16_t v) { return __builtin_bswap16(v); }

static constexpr std::size_t BUFFER_SIZE = 4096;

enum class MessageType: uint16_t {
    HeartBeat = 0x0001,
    HeartBeatAck = 0x0002,
    // UDP Session authentication
    JoinGame = 0x0010,
    JoinGameAck = 0x0011,
    JoinGameNack = 0x0012,
    // UDP Game messages
    Snapshot = 0x0040,
    PlayerInput = 0x0061,
    PlayerJoin = 0x0070,
    PlayerLeave = 0x0071,
    ShootMissile = 0x0080,
    MissileSpawned = 0x0081,
    MissileDestroyed = 0x0082,
    EnemyDestroyed = 0x0091,
    PlayerDamaged = 0x00A0,
    PlayerDied = 0x00A1,
    // Score system (Gameplay Phase 2)
    ScoreUpdate = 0x00B0,
    ComboUpdate = 0x00B1,
    WaveComplete = 0x00B2,
    // Boss system (Gameplay Phase 2)
    BossSpawn = 0x00C0,
    BossPhaseChange = 0x00C1,
    BossDefeated = 0x00C2,
    // Weapon system (Gameplay Phase 2)
    SwitchWeapon = 0x00D0,
    WeaponChanged = 0x00D1,
    // TCP Authentication messages
    Login = 0x0100,
    LoginAck = 0x0101,
    Register = 0x0102,
    RegisterAck = 0x0103,
    // TCP Room Management messages
    CreateRoom = 0x0200,
    CreateRoomAck = 0x0201,
    JoinRoomByCode = 0x0210,
    JoinRoomAck = 0x0211,
    JoinRoomNack = 0x0212,
    LeaveRoom = 0x0220,
    LeaveRoomAck = 0x0221,
    SetReady = 0x0230,
    SetReadyAck = 0x0231,
    StartGame = 0x0240,
    StartGameAck = 0x0241,
    StartGameNack = 0x0242,
    // TCP Room Notifications (broadcast to room members)
    RoomUpdate = 0x0250,
    GameStarting = 0x0251,
    SetRoomConfig = 0x0252,
    SetRoomConfigAck = 0x0253,
    // Phase 2 - Kick System (0x026x)
    KickPlayer = 0x0260,
    KickPlayerAck = 0x0261,
    PlayerKicked = 0x0262,
    // Phase 2 - Room Browser (0x027x)
    BrowsePublicRooms = 0x0270,
    BrowsePublicRoomsAck = 0x0271,
    QuickJoin = 0x0272,
    QuickJoinAck = 0x0273,
    QuickJoinNack = 0x0274,
    // Phase 2 - User Settings (0x028x)
    GetUserSettings = 0x0280,
    GetUserSettingsAck = 0x0281,
    SaveUserSettings = 0x0282,
    SaveUserSettingsAck = 0x0283,
    // Phase 2 - Chat System (0x029x)
    SendChatMessage = 0x0290,
    SendChatMessageAck = 0x0291,
    ChatMessageBroadcast = 0x0292,
    ChatHistory = 0x0293,
    // Voice Chat System (0x030x)
    VoiceJoin = 0x0300,
    VoiceJoinAck = 0x0301,
    VoiceLeave = 0x0302,
    VoiceFrame = 0x0303,
    VoiceMute = 0x0304,
    // R-Type Authentic Mechanics - Phase 3 (0x040x)
    // Wave Cannon (Charge Shot)
    ChargeStart = 0x0400,       // C→S: Player started charging
    ChargeRelease = 0x0401,     // C→S: Player released charge (fire beam)
    WaveCannonFired = 0x0402,   // S→C: Wave cannon projectile spawned
    // Power-up System
    PowerUpSpawned = 0x0410,    // S→C: Power-up item appeared
    PowerUpCollected = 0x0411,  // S→C: Power-up collected by player
    PowerUpExpired = 0x0412,    // S→C: Power-up disappeared (timeout)
    // Force Pod System
    ForceToggle = 0x0420,       // C→S: Toggle Force attach/detach
    ForceStateUpdate = 0x0421,  // S→C: Force pod state changed
};

static constexpr uint8_t MAX_PLAYERS = 4;
static constexpr uint8_t MAX_MISSILES = 32;
static constexpr uint8_t MAX_ENEMIES = 16;
static constexpr uint8_t MAX_ENEMY_MISSILES = 32;
static constexpr uint8_t ENEMY_OWNER_ID = 0xFF;

// Room system constants
static constexpr size_t ROOM_NAME_LEN = 32;
static constexpr size_t ROOM_CODE_LEN = 6;
static constexpr uint8_t MAX_ROOM_PLAYERS = 6;
static constexpr uint8_t MIN_ROOM_PLAYERS = 2;

// Session token size (256 bits = 32 bytes)
static constexpr size_t TOKEN_SIZE = 32;

// Input keys bitfield
namespace InputKeys {
    constexpr uint16_t UP    = 0x0001;
    constexpr uint16_t DOWN  = 0x0002;
    constexpr uint16_t LEFT  = 0x0004;
    constexpr uint16_t RIGHT = 0x0008;
    constexpr uint16_t SHOOT = 0x0010;
    // Weapon switching (Gameplay Phase 2)
    constexpr uint16_t WEAPON_NEXT = 0x0020;  // Switch to next weapon
    constexpr uint16_t WEAPON_PREV = 0x0040;  // Switch to previous weapon
}

// Weapon types (Gameplay Phase 2)
enum class WeaponType : uint8_t {
    Standard = 0,   // Default single shot
    Spread = 1,     // 3-way spread shot
    Laser = 2,      // Fast narrow beam
    Missile = 3,    // Slow homing projectile
    COUNT           // Number of weapon types
};

static constexpr uint8_t MAX_WEAPON_TYPES = static_cast<uint8_t>(WeaponType::COUNT);

// Power-up types (R-Type Authentic - Phase 3)
// Note: R-Type original has no Shield - defense comes from Force Pod only
enum class PowerUpType : uint8_t {
    Health = 0,         // Restore 25 HP
    SpeedUp = 1,        // Increase movement speed (blue crystal)
    WeaponCrystal = 2,  // Upgrade current weapon level (red crystal)
    ForcePod = 3,       // Grant or upgrade Force Pod (orange orb)
    BitDevice = 4,      // Grant 2 orbiting Bit satellites (R-Type authentic)
    COUNT
};

static constexpr uint8_t MAX_POWERUP_TYPES = static_cast<uint8_t>(PowerUpType::COUNT);
static constexpr uint8_t MAX_POWERUPS = 8;      // Max power-ups on screen
static constexpr uint8_t MAX_SPEED_LEVEL = 3;   // Max speed upgrades
static constexpr uint8_t MAX_BITS = 8;          // Max Bit Devices on screen (2 per player × 4 players)

// Wave Cannon constants (R-Type Authentic - Phase 3)
// Balanced: Lv3 = ~2.2s to charge, deals 250 dmg = ~114 dps if spammed
// Best used vs groups (piercing) or boss burst damage
namespace WaveCannon {
    constexpr float CHARGE_TIME_LV1 = 0.6f;     // Quick shot
    constexpr float CHARGE_TIME_LV2 = 1.3f;     // Medium charge
    constexpr float CHARGE_TIME_LV3 = 2.2f;     // Full charge for max damage
    constexpr uint8_t DAMAGE_LV1 = 50;          // Kills Fast enemy (20 HP)
    constexpr uint8_t DAMAGE_LV2 = 100;         // Kills most enemies
    constexpr uint16_t DAMAGE_LV3 = 250;        // Rewards full charge risk
    constexpr float SPEED = 850.0f;             // Beam speed
    constexpr float WIDTH_LV1 = 20.0f;
    constexpr float WIDTH_LV2 = 35.0f;
    constexpr float WIDTH_LV3 = 55.0f;          // Wider for more hits
}

// Session token for UDP authentication
struct SessionToken {
    uint8_t bytes[TOKEN_SIZE];

    bool operator==(const SessionToken& other) const {
        return std::memcmp(bytes, other.bytes, TOKEN_SIZE) == 0;
    }

    bool operator!=(const SessionToken& other) const {
        return !(*this == other);
    }

    // Convert to hex string for storage/lookup
    std::string toHex() const {
        static const char hex[] = "0123456789abcdef";
        std::string result;
        result.reserve(TOKEN_SIZE * 2);
        for (size_t i = 0; i < TOKEN_SIZE; ++i) {
            result.push_back(hex[(bytes[i] >> 4) & 0x0F]);
            result.push_back(hex[bytes[i] & 0x0F]);
        }
        return result;
    }

    // Create from hex string
    static std::optional<SessionToken> fromHex(const std::string& hex) {
        if (hex.size() != TOKEN_SIZE * 2) return std::nullopt;
        SessionToken token;
        for (size_t i = 0; i < TOKEN_SIZE; ++i) {
            auto hi = hex[i * 2];
            auto lo = hex[i * 2 + 1];
            auto hexVal = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int hiVal = hexVal(hi);
            int loVal = hexVal(lo);
            if (hiVal < 0 || loVal < 0) return std::nullopt;
            token.bytes[i] = static_cast<uint8_t>((hiVal << 4) | loVal);
        }
        return token;
    }

    void to_bytes(uint8_t* buf) const {
        std::memcpy(buf, bytes, TOKEN_SIZE);
    }

    static std::optional<SessionToken> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < TOKEN_SIZE) return std::nullopt;
        SessionToken token;
        std::memcpy(token.bytes, buf, TOKEN_SIZE);
        return token;
    }
};

// JoinGame: Client sends token to authenticate UDP session
struct JoinGame {
    SessionToken token;
    uint8_t shipSkin;  // Ship skin variant (1-6)
    char roomCode[ROOM_CODE_LEN];  // Room code for multi-instance routing
    static constexpr size_t WIRE_SIZE = TOKEN_SIZE + 1 + ROOM_CODE_LEN;

    void to_bytes(uint8_t* buf) const {
        token.to_bytes(buf);
        buf[TOKEN_SIZE] = shipSkin;
        std::memcpy(buf + TOKEN_SIZE + 1, roomCode, ROOM_CODE_LEN);
    }

    static std::optional<JoinGame> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto tokenOpt = SessionToken::from_bytes(buf, len);
        if (!tokenOpt) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        JoinGame msg;
        msg.token = *tokenOpt;
        msg.shipSkin = ptr[TOKEN_SIZE];
        std::memcpy(msg.roomCode, ptr + TOKEN_SIZE + 1, ROOM_CODE_LEN);
        return msg;
    }
};

// JoinGameAck: Server confirms and assigns player ID
struct JoinGameAck {
    uint8_t player_id;
    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<JoinGameAck> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        return JoinGameAck{.player_id = ptr[0]};
    }
};

// JoinGameNack: Server rejects the join request
struct JoinGameNack {
    char reason[64];
    static constexpr size_t WIRE_SIZE = 64;

    void to_bytes(uint8_t* buf) const {
        std::memcpy(buf, reason, WIRE_SIZE);
    }

    static std::optional<JoinGameNack> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        JoinGameNack nack;
        std::memcpy(nack.reason, buf, WIRE_SIZE);
        nack.reason[WIRE_SIZE - 1] = '\0';
        return nack;
    }
};

// PlayerInput: Client sends input keys (not position!)
struct PlayerInput {
    uint16_t keys;        // Bitfield using InputKeys namespace
    uint16_t sequenceNum; // For client-side prediction reconciliation
    static constexpr size_t WIRE_SIZE = 4;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_keys = swap16(keys);
        uint16_t net_seq = swap16(sequenceNum);
        std::memcpy(buf, &net_keys, 2);
        std::memcpy(buf + 2, &net_seq, 2);
    }

    static std::optional<PlayerInput> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        uint16_t net_keys, net_seq;
        std::memcpy(&net_keys, ptr, 2);
        std::memcpy(&net_seq, ptr + 2, 2);
        return PlayerInput{
            .keys = swap16(net_keys),
            .sequenceNum = swap16(net_seq)
        };
    }
};

// TCP Authentication Protocol structures
struct Header {
    bool isAuthenticated;
    uint16_t type;
    uint32_t payload_size;

    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = isAuthenticated ? 1 : 0;
        uint16_t net_type = swap16(type);
        uint32_t net_size = swap32(payload_size);
        std::memcpy(ptr + 1, &net_type, 2);
        std::memcpy(ptr + 3, &net_size, 4);
    }

    static std::optional<Header> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        Header head;
        head.isAuthenticated = (ptr[0] != 0);
        uint16_t net_type;
        uint32_t net_size;
        std::memcpy(&net_type, ptr + 1, 2);
        std::memcpy(&net_size, ptr + 3, 4);
        head.type = swap16(net_type);
        head.payload_size = swap32(net_size);
        return head;
    }
};

static constexpr size_t MAX_USERNAME_LEN = 32;
static constexpr size_t MAX_PASSWORD_LEN = 64;
static constexpr size_t MAX_EMAIL_LEN = 255;  // RFC 5321: max 254 chars + null terminator

struct LoginMessage {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, username, MAX_USERNAME_LEN);
        std::memcpy(ptr + MAX_USERNAME_LEN, password, MAX_PASSWORD_LEN);
    }

    static std::optional<LoginMessage> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < MAX_USERNAME_LEN + MAX_PASSWORD_LEN) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        LoginMessage msg;
        std::memcpy(msg.username, ptr, MAX_USERNAME_LEN);
        std::memcpy(msg.password, ptr + MAX_USERNAME_LEN, MAX_PASSWORD_LEN);
        msg.username[MAX_USERNAME_LEN - 1] = '\0';
        msg.password[MAX_PASSWORD_LEN - 1] = '\0';
        return msg;
    }
};

struct RegisterMessage {
    char username[MAX_USERNAME_LEN];
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWORD_LEN];

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, username, MAX_USERNAME_LEN);
        std::memcpy(ptr + MAX_USERNAME_LEN, email, MAX_EMAIL_LEN);
        std::memcpy(ptr + MAX_USERNAME_LEN + MAX_EMAIL_LEN, password, MAX_PASSWORD_LEN);
    }

    static std::optional<RegisterMessage> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < MAX_USERNAME_LEN + MAX_EMAIL_LEN + MAX_PASSWORD_LEN) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        RegisterMessage msg;
        std::memcpy(msg.username, ptr, MAX_USERNAME_LEN);
        std::memcpy(msg.email, ptr + MAX_USERNAME_LEN, MAX_EMAIL_LEN);
        std::memcpy(msg.password, ptr + MAX_USERNAME_LEN + MAX_EMAIL_LEN, MAX_PASSWORD_LEN);
        msg.username[MAX_USERNAME_LEN - 1] = '\0';
        msg.email[MAX_EMAIL_LEN - 1] = '\0';
        msg.password[MAX_PASSWORD_LEN - 1] = '\0';
        return msg;
    }
};

static constexpr size_t MAX_ERROR_CODE_LEN = 32;
static constexpr size_t MAX_ERROR_MSG_LEN = 128;

struct AuthResponse {
    bool success;
    char error_code[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = 1 + MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = success ? 1 : 0;
        std::memcpy(ptr + 1, error_code, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + 1 + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<AuthResponse> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        AuthResponse resp;
        resp.success = (ptr[0] != 0);
        std::memcpy(resp.error_code, ptr + 1, MAX_ERROR_CODE_LEN);
        std::memcpy(resp.message, ptr + 1 + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        resp.error_code[MAX_ERROR_CODE_LEN - 1] = '\0';
        resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return resp;
    }
};

// AuthResponse with session token (sent on successful login)
struct AuthResponseWithToken {
    bool success;
    char error_code[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];
    SessionToken token;  // Only valid if success == true

    static constexpr size_t WIRE_SIZE = 1 + MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN + TOKEN_SIZE;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = success ? 1 : 0;
        std::memcpy(ptr + 1, error_code, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + 1 + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
        token.to_bytes(ptr + 1 + MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN);
    }

    static std::optional<AuthResponseWithToken> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        AuthResponseWithToken resp;
        resp.success = (ptr[0] != 0);
        std::memcpy(resp.error_code, ptr + 1, MAX_ERROR_CODE_LEN);
        std::memcpy(resp.message, ptr + 1 + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        resp.error_code[MAX_ERROR_CODE_LEN - 1] = '\0';
        resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        auto tokenOpt = SessionToken::from_bytes(
            ptr + 1 + MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN, TOKEN_SIZE);
        if (tokenOpt) {
            resp.token = *tokenOpt;
        }
        return resp;
    }
};

// ============================================================================
// Room Protocol Structures (TCP)
// ============================================================================

// CreateRoom: Client requests to create a new room
struct CreateRoomRequest {
    char name[ROOM_NAME_LEN];
    uint8_t maxPlayers;  // 2-6
    uint8_t isPrivate;   // 0 = public, 1 = private

    static constexpr size_t WIRE_SIZE = ROOM_NAME_LEN + 2;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, name, ROOM_NAME_LEN);
        ptr[ROOM_NAME_LEN] = maxPlayers;
        ptr[ROOM_NAME_LEN + 1] = isPrivate;
    }

    static std::optional<CreateRoomRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        CreateRoomRequest req;
        std::memcpy(req.name, ptr, ROOM_NAME_LEN);
        req.name[ROOM_NAME_LEN - 1] = '\0';
        req.maxPlayers = ptr[ROOM_NAME_LEN];
        req.isPrivate = ptr[ROOM_NAME_LEN + 1];
        return req;
    }
};

// CreateRoomAck: Server response to room creation
struct CreateRoomAck {
    uint8_t success;
    char roomCode[ROOM_CODE_LEN];
    char errorCode[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = 1 + ROOM_CODE_LEN + MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = success;
        std::memcpy(ptr + 1, roomCode, ROOM_CODE_LEN);
        std::memcpy(ptr + 1 + ROOM_CODE_LEN, errorCode, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + 1 + ROOM_CODE_LEN + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<CreateRoomAck> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        CreateRoomAck ack;
        ack.success = ptr[0];
        std::memcpy(ack.roomCode, ptr + 1, ROOM_CODE_LEN);
        std::memcpy(ack.errorCode, ptr + 1 + ROOM_CODE_LEN, MAX_ERROR_CODE_LEN);
        std::memcpy(ack.message, ptr + 1 + ROOM_CODE_LEN + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        ack.errorCode[MAX_ERROR_CODE_LEN - 1] = '\0';
        ack.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return ack;
    }
};

// JoinRoomByCode: Client requests to join a room by invitation code
struct JoinRoomByCodeRequest {
    char roomCode[ROOM_CODE_LEN];

    static constexpr size_t WIRE_SIZE = ROOM_CODE_LEN;

    void to_bytes(void* buf) const {
        std::memcpy(buf, roomCode, ROOM_CODE_LEN);
    }

    static std::optional<JoinRoomByCodeRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        JoinRoomByCodeRequest req;
        std::memcpy(req.roomCode, buf, ROOM_CODE_LEN);
        return req;
    }
};

// RoomPlayerState: State of a single player in the room lobby
// (Forward declaration moved here for JoinRoomAck)
struct RoomPlayerState {
    uint8_t slotId;
    uint8_t occupied;  // 0 = empty, 1 = occupied
    char displayName[MAX_USERNAME_LEN];
    char email[MAX_EMAIL_LEN];  // Added for kick functionality (Phase 2)
    uint8_t isReady;
    uint8_t isHost;
    uint8_t shipSkin;  // Ship skin variant (1-6)

    static constexpr size_t WIRE_SIZE = 1 + 1 + MAX_USERNAME_LEN + MAX_EMAIL_LEN + 1 + 1 + 1;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = slotId;
        ptr[1] = occupied;
        std::memcpy(ptr + 2, displayName, MAX_USERNAME_LEN);
        std::memcpy(ptr + 2 + MAX_USERNAME_LEN, email, MAX_EMAIL_LEN);
        ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN] = isReady;
        ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN + 1] = isHost;
        ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN + 2] = shipSkin;
    }

    static std::optional<RoomPlayerState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        RoomPlayerState state;
        state.slotId = ptr[0];
        state.occupied = ptr[1];
        std::memcpy(state.displayName, ptr + 2, MAX_USERNAME_LEN);
        state.displayName[MAX_USERNAME_LEN - 1] = '\0';
        std::memcpy(state.email, ptr + 2 + MAX_USERNAME_LEN, MAX_EMAIL_LEN);
        state.email[MAX_EMAIL_LEN - 1] = '\0';
        state.isReady = ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN];
        state.isHost = ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN + 1];
        state.shipSkin = ptr[2 + MAX_USERNAME_LEN + MAX_EMAIL_LEN + 2];
        return state;
    }
};

// JoinRoomAck: Server confirms room join
struct JoinRoomAck {
    uint8_t slotId;       // Player's slot in the room (0-5)
    char roomName[ROOM_NAME_LEN];
    char roomCode[ROOM_CODE_LEN];
    uint8_t maxPlayers;
    uint8_t isHost;       // 1 if player is the host
    uint8_t playerCount;  // Number of players in room (including self)
    RoomPlayerState players[MAX_ROOM_PLAYERS];  // Current players in room

    static constexpr size_t HEADER_SIZE = 1 + ROOM_NAME_LEN + ROOM_CODE_LEN + 1 + 1 + 1;

    // Variable size: HEADER_SIZE + playerCount * RoomPlayerState::WIRE_SIZE
    size_t wire_size() const {
        return HEADER_SIZE + playerCount * RoomPlayerState::WIRE_SIZE;
    }

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = slotId;
        std::memcpy(ptr + 1, roomName, ROOM_NAME_LEN);
        std::memcpy(ptr + 1 + ROOM_NAME_LEN, roomCode, ROOM_CODE_LEN);
        ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN] = maxPlayers;
        ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN + 1] = isHost;
        ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN + 2] = playerCount;

        size_t offset = HEADER_SIZE;
        for (uint8_t i = 0; i < playerCount; ++i) {
            players[i].to_bytes(ptr + offset);
            offset += RoomPlayerState::WIRE_SIZE;
        }
    }

    static std::optional<JoinRoomAck> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < HEADER_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        JoinRoomAck ack;
        ack.slotId = ptr[0];
        std::memcpy(ack.roomName, ptr + 1, ROOM_NAME_LEN);
        ack.roomName[ROOM_NAME_LEN - 1] = '\0';
        std::memcpy(ack.roomCode, ptr + 1 + ROOM_NAME_LEN, ROOM_CODE_LEN);
        ack.maxPlayers = ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN];
        ack.isHost = ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN + 1];
        ack.playerCount = ptr[1 + ROOM_NAME_LEN + ROOM_CODE_LEN + 2];

        if (ack.playerCount > MAX_ROOM_PLAYERS) return std::nullopt;
        if (buf_len < HEADER_SIZE + ack.playerCount * RoomPlayerState::WIRE_SIZE) return std::nullopt;

        size_t offset = HEADER_SIZE;
        for (uint8_t i = 0; i < ack.playerCount; ++i) {
            auto playerOpt = RoomPlayerState::from_bytes(ptr + offset, RoomPlayerState::WIRE_SIZE);
            if (!playerOpt) return std::nullopt;
            ack.players[i] = *playerOpt;
            offset += RoomPlayerState::WIRE_SIZE;
        }
        return ack;
    }
};

// JoinRoomNack: Server rejects room join
struct JoinRoomNack {
    char errorCode[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, errorCode, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<JoinRoomNack> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        JoinRoomNack nack;
        std::memcpy(nack.errorCode, ptr, MAX_ERROR_CODE_LEN);
        std::memcpy(nack.message, ptr + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        nack.errorCode[MAX_ERROR_CODE_LEN - 1] = '\0';
        nack.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return nack;
    }
};

// LeaveRoom: Client leaves the current room (no payload)
struct LeaveRoomRequest {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<LeaveRoomRequest> from_bytes(const void*, size_t) {
        return LeaveRoomRequest{};
    }
};

// LeaveRoomAck: Server confirms room leave (no payload)
struct LeaveRoomAck {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<LeaveRoomAck> from_bytes(const void*, size_t) {
        return LeaveRoomAck{};
    }
};

// SetReady: Client sets ready status
struct SetReadyRequest {
    uint8_t isReady;  // 0 = not ready, 1 = ready

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(void* buf) const {
        static_cast<uint8_t*>(buf)[0] = isReady;
    }

    static std::optional<SetReadyRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        SetReadyRequest req;
        req.isReady = static_cast<const uint8_t*>(buf)[0];
        return req;
    }
};

// SetReadyAck: Server confirms ready status
struct SetReadyAck {
    uint8_t isReady;

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(void* buf) const {
        static_cast<uint8_t*>(buf)[0] = isReady;
    }

    static std::optional<SetReadyAck> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        SetReadyAck ack;
        ack.isReady = static_cast<const uint8_t*>(buf)[0];
        return ack;
    }
};

// StartGame: Host requests to start the game (no payload)
struct StartGameRequest {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<StartGameRequest> from_bytes(const void*, size_t) {
        return StartGameRequest{};
    }
};

// StartGameAck: Server confirms game start (no payload)
struct StartGameAck {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<StartGameAck> from_bytes(const void*, size_t) {
        return StartGameAck{};
    }
};

// StartGameNack: Server rejects game start
struct StartGameNack {
    char errorCode[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, errorCode, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<StartGameNack> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        StartGameNack nack;
        std::memcpy(nack.errorCode, ptr, MAX_ERROR_CODE_LEN);
        std::memcpy(nack.message, ptr + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        nack.errorCode[MAX_ERROR_CODE_LEN - 1] = '\0';
        nack.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return nack;
    }
};

// RoomUpdate: Full room state notification (broadcast to all room members)
struct RoomUpdate {
    char roomName[ROOM_NAME_LEN];
    char roomCode[ROOM_CODE_LEN];
    uint8_t maxPlayers;
    uint8_t playerCount;
    uint16_t gameSpeedPercent;  // 50-200, default 100
    RoomPlayerState players[MAX_ROOM_PLAYERS];

    size_t wire_size() const {
        return ROOM_NAME_LEN + ROOM_CODE_LEN + 1 + 1 + 2 + playerCount * RoomPlayerState::WIRE_SIZE;
    }

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, roomName, ROOM_NAME_LEN);
        std::memcpy(ptr + ROOM_NAME_LEN, roomCode, ROOM_CODE_LEN);
        ptr[ROOM_NAME_LEN + ROOM_CODE_LEN] = maxPlayers;
        ptr[ROOM_NAME_LEN + ROOM_CODE_LEN + 1] = playerCount;
        uint16_t net_speed = swap16(gameSpeedPercent);
        std::memcpy(ptr + ROOM_NAME_LEN + ROOM_CODE_LEN + 2, &net_speed, 2);
        size_t offset = ROOM_NAME_LEN + ROOM_CODE_LEN + 4;
        for (uint8_t i = 0; i < playerCount; ++i) {
            players[i].to_bytes(ptr + offset);
            offset += RoomPlayerState::WIRE_SIZE;
        }
    }

    static std::optional<RoomUpdate> from_bytes(const void* buf, size_t buf_len) {
        constexpr size_t HEADER_SIZE = ROOM_NAME_LEN + ROOM_CODE_LEN + 4;  // +2 for gameSpeedPercent
        if (buf == nullptr || buf_len < HEADER_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        RoomUpdate update;
        std::memcpy(update.roomName, ptr, ROOM_NAME_LEN);
        update.roomName[ROOM_NAME_LEN - 1] = '\0';
        std::memcpy(update.roomCode, ptr + ROOM_NAME_LEN, ROOM_CODE_LEN);
        update.maxPlayers = ptr[ROOM_NAME_LEN + ROOM_CODE_LEN];
        update.playerCount = ptr[ROOM_NAME_LEN + ROOM_CODE_LEN + 1];
        uint16_t net_speed;
        std::memcpy(&net_speed, ptr + ROOM_NAME_LEN + ROOM_CODE_LEN + 2, 2);
        update.gameSpeedPercent = swap16(net_speed);
        if (update.playerCount > MAX_ROOM_PLAYERS) return std::nullopt;
        size_t required = HEADER_SIZE + update.playerCount * RoomPlayerState::WIRE_SIZE;
        if (buf_len < required) return std::nullopt;
        size_t offset = HEADER_SIZE;
        for (uint8_t i = 0; i < update.playerCount; ++i) {
            auto stateOpt = RoomPlayerState::from_bytes(ptr + offset, RoomPlayerState::WIRE_SIZE);
            if (!stateOpt) return std::nullopt;
            update.players[i] = *stateOpt;
            offset += RoomPlayerState::WIRE_SIZE;
        }
        return update;
    }
};

// GameStarting: Notification that game is about to start
struct GameStarting {
    uint8_t countdownSeconds;  // 3, 2, 1, 0

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(void* buf) const {
        static_cast<uint8_t*>(buf)[0] = countdownSeconds;
    }

    static std::optional<GameStarting> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        GameStarting gs;
        gs.countdownSeconds = static_cast<const uint8_t*>(buf)[0];
        return gs;
    }
};

// SetRoomConfig: Host sets room configuration (e.g., game speed)
struct SetRoomConfigRequest {
    uint16_t gameSpeedPercent;  // 50-200 (0.5x to 2.0x)

    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(void* buf) const {
        uint16_t net_speed = swap16(gameSpeedPercent);
        std::memcpy(buf, &net_speed, 2);
    }

    static std::optional<SetRoomConfigRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        uint16_t net_speed;
        std::memcpy(&net_speed, buf, 2);
        return SetRoomConfigRequest{.gameSpeedPercent = swap16(net_speed)};
    }
};

// SetRoomConfigAck: Server confirms room config change
struct SetRoomConfigAck {
    uint8_t success;  // 1 = success, 0 = failed (not host)

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(void* buf) const {
        static_cast<uint8_t*>(buf)[0] = success;
    }

    static std::optional<SetRoomConfigAck> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        return SetRoomConfigAck{.success = static_cast<const uint8_t*>(buf)[0]};
    }
};

// ============================================================================
// Kick System Protocol Structures (TCP) - Phase 2
// ============================================================================

// KickPlayer: Host requests to kick a player from the room
struct KickPlayerRequest {
    char email[MAX_EMAIL_LEN];
    char reason[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = MAX_EMAIL_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, email, MAX_EMAIL_LEN);
        std::memcpy(ptr + MAX_EMAIL_LEN, reason, MAX_ERROR_MSG_LEN);
    }

    static std::optional<KickPlayerRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        KickPlayerRequest req;
        std::memcpy(req.email, ptr, MAX_EMAIL_LEN);
        req.email[MAX_EMAIL_LEN - 1] = '\0';
        std::memcpy(req.reason, ptr + MAX_EMAIL_LEN, MAX_ERROR_MSG_LEN);
        req.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
        return req;
    }
};

// KickPlayerAck: Server confirms kick was processed (no payload)
struct KickPlayerAck {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<KickPlayerAck> from_bytes(const void*, size_t) {
        return KickPlayerAck{};
    }
};

// PlayerKicked: Notification sent to the kicked player
struct PlayerKickedNotification {
    char reason[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        std::memcpy(buf, reason, MAX_ERROR_MSG_LEN);
    }

    static std::optional<PlayerKickedNotification> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        PlayerKickedNotification notif;
        std::memcpy(notif.reason, buf, MAX_ERROR_MSG_LEN);
        notif.reason[MAX_ERROR_MSG_LEN - 1] = '\0';
        return notif;
    }
};

// ============================================================================
// Room Browser Protocol Structures (TCP) - Phase 2
// ============================================================================

static constexpr uint8_t MAX_BROWSER_ROOMS = 20;

// BrowsePublicRooms: Client requests list of public rooms (no payload)
struct BrowsePublicRoomsRequest {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<BrowsePublicRoomsRequest> from_bytes(const void*, size_t) {
        return BrowsePublicRoomsRequest{};
    }
};

// RoomBrowserEntry: Single room in the browser list
struct RoomBrowserEntry {
    char code[ROOM_CODE_LEN];
    char name[ROOM_NAME_LEN];
    uint8_t currentPlayers;
    uint8_t maxPlayers;

    static constexpr size_t WIRE_SIZE = ROOM_CODE_LEN + ROOM_NAME_LEN + 2;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, code, ROOM_CODE_LEN);
        std::memcpy(ptr + ROOM_CODE_LEN, name, ROOM_NAME_LEN);
        ptr[ROOM_CODE_LEN + ROOM_NAME_LEN] = currentPlayers;
        ptr[ROOM_CODE_LEN + ROOM_NAME_LEN + 1] = maxPlayers;
    }

    static std::optional<RoomBrowserEntry> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        RoomBrowserEntry entry;
        std::memcpy(entry.code, ptr, ROOM_CODE_LEN);
        std::memcpy(entry.name, ptr + ROOM_CODE_LEN, ROOM_NAME_LEN);
        entry.name[ROOM_NAME_LEN - 1] = '\0';
        entry.currentPlayers = ptr[ROOM_CODE_LEN + ROOM_NAME_LEN];
        entry.maxPlayers = ptr[ROOM_CODE_LEN + ROOM_NAME_LEN + 1];
        return entry;
    }
};

// BrowsePublicRoomsAck: Server sends list of public rooms
struct BrowsePublicRoomsResponse {
    uint8_t roomCount;
    RoomBrowserEntry rooms[MAX_BROWSER_ROOMS];

    size_t wire_size() const {
        return 1 + roomCount * RoomBrowserEntry::WIRE_SIZE;
    }

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = roomCount;
        size_t offset = 1;
        for (uint8_t i = 0; i < roomCount && i < MAX_BROWSER_ROOMS; ++i) {
            rooms[i].to_bytes(ptr + offset);
            offset += RoomBrowserEntry::WIRE_SIZE;
        }
    }

    static std::optional<BrowsePublicRoomsResponse> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < 1) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        BrowsePublicRoomsResponse resp;
        resp.roomCount = ptr[0];
        if (resp.roomCount > MAX_BROWSER_ROOMS) return std::nullopt;
        size_t required = 1 + resp.roomCount * RoomBrowserEntry::WIRE_SIZE;
        if (buf_len < required) return std::nullopt;
        size_t offset = 1;
        for (uint8_t i = 0; i < resp.roomCount; ++i) {
            auto entryOpt = RoomBrowserEntry::from_bytes(ptr + offset, RoomBrowserEntry::WIRE_SIZE);
            if (!entryOpt) return std::nullopt;
            resp.rooms[i] = *entryOpt;
            offset += RoomBrowserEntry::WIRE_SIZE;
        }
        return resp;
    }
};

// QuickJoin: Client requests to join any available public room (no payload)
struct QuickJoinRequest {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<QuickJoinRequest> from_bytes(const void*, size_t) {
        return QuickJoinRequest{};
    }
};

// QuickJoinAck: Server found a room - same as JoinRoomAck (uses JoinRoomAck structure)
// QuickJoinNack: No room available
struct QuickJoinNack {
    char errorCode[MAX_ERROR_CODE_LEN];
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = MAX_ERROR_CODE_LEN + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, errorCode, MAX_ERROR_CODE_LEN);
        std::memcpy(ptr + MAX_ERROR_CODE_LEN, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<QuickJoinNack> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        QuickJoinNack nack;
        std::memcpy(nack.errorCode, ptr, MAX_ERROR_CODE_LEN);
        nack.errorCode[MAX_ERROR_CODE_LEN - 1] = '\0';
        std::memcpy(nack.message, ptr + MAX_ERROR_CODE_LEN, MAX_ERROR_MSG_LEN);
        nack.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return nack;
    }
};

// ============================================================================
// User Settings Protocol Structures (TCP) - Phase 2
// ============================================================================

static constexpr size_t COLORBLIND_MODE_LEN = 16;
static constexpr size_t KEY_BINDINGS_COUNT = 26;  // 13 actions × 2 keys (includes WeaponPrev/Next, OpenChat, ExpandChat, ForceToggle, ToggleControls)
static constexpr size_t AUDIO_DEVICE_NAME_LEN = 64;  // Audio device name storage

// UserSettingsPayload: Core settings data
struct UserSettingsPayload {
    char colorBlindMode[COLORBLIND_MODE_LEN];  // "none", "protanopia", etc.
    uint16_t gameSpeedPercent;                  // 50-200 (represents 0.5x-2.0x)
    uint8_t keyBindings[KEY_BINDINGS_COUNT];    // [action0_primary, action0_secondary, ...]
    uint8_t shipSkin;                           // Ship skin variant (1-6)
    uint8_t voiceMode;                          // 0 = PushToTalk, 1 = VoiceActivity
    uint8_t vadThreshold;                       // 0-100 (scaled from 0.0-1.0)
    uint8_t micGain;                            // 0-200 (scaled from 0.0-2.0)
    uint8_t voiceVolume;                        // 0-100
    // Audio device selection (0 = auto, stored name for matching on different machines)
    char audioInputDevice[AUDIO_DEVICE_NAME_LEN];   // Preferred input device name ("" = auto)
    char audioOutputDevice[AUDIO_DEVICE_NAME_LEN];  // Preferred output device name ("" = auto)
    // Chat settings
    uint8_t keepChatOpenAfterSend;              // 1 = keep chat open, 0 = close after send

    static constexpr size_t WIRE_SIZE = COLORBLIND_MODE_LEN + 2 + KEY_BINDINGS_COUNT + 1 + 4 + (AUDIO_DEVICE_NAME_LEN * 2) + 1;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, colorBlindMode, COLORBLIND_MODE_LEN);
        uint16_t net_speed = swap16(gameSpeedPercent);
        std::memcpy(ptr + COLORBLIND_MODE_LEN, &net_speed, 2);
        std::memcpy(ptr + COLORBLIND_MODE_LEN + 2, keyBindings, KEY_BINDINGS_COUNT);
        size_t offset = COLORBLIND_MODE_LEN + 2 + KEY_BINDINGS_COUNT;
        ptr[offset] = shipSkin;
        ptr[offset + 1] = voiceMode;
        ptr[offset + 2] = vadThreshold;
        ptr[offset + 3] = micGain;
        ptr[offset + 4] = voiceVolume;
        // Audio device names
        std::memcpy(ptr + offset + 5, audioInputDevice, AUDIO_DEVICE_NAME_LEN);
        std::memcpy(ptr + offset + 5 + AUDIO_DEVICE_NAME_LEN, audioOutputDevice, AUDIO_DEVICE_NAME_LEN);
        // Chat settings
        ptr[offset + 5 + (AUDIO_DEVICE_NAME_LEN * 2)] = keepChatOpenAfterSend;
    }

    static std::optional<UserSettingsPayload> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        UserSettingsPayload payload;
        std::memcpy(payload.colorBlindMode, ptr, COLORBLIND_MODE_LEN);
        payload.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
        uint16_t net_speed;
        std::memcpy(&net_speed, ptr + COLORBLIND_MODE_LEN, 2);
        payload.gameSpeedPercent = swap16(net_speed);
        std::memcpy(payload.keyBindings, ptr + COLORBLIND_MODE_LEN + 2, KEY_BINDINGS_COUNT);
        size_t offset = COLORBLIND_MODE_LEN + 2 + KEY_BINDINGS_COUNT;
        payload.shipSkin = ptr[offset];
        payload.voiceMode = ptr[offset + 1];
        payload.vadThreshold = ptr[offset + 2];
        payload.micGain = ptr[offset + 3];
        payload.voiceVolume = ptr[offset + 4];
        // Audio device names
        std::memcpy(payload.audioInputDevice, ptr + offset + 5, AUDIO_DEVICE_NAME_LEN);
        payload.audioInputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';
        std::memcpy(payload.audioOutputDevice, ptr + offset + 5 + AUDIO_DEVICE_NAME_LEN, AUDIO_DEVICE_NAME_LEN);
        payload.audioOutputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';
        // Chat settings
        payload.keepChatOpenAfterSend = ptr[offset + 5 + (AUDIO_DEVICE_NAME_LEN * 2)];
        return payload;
    }
};

// GetUserSettingsRequest: Client requests their settings (no payload needed)
struct GetUserSettingsRequest {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<GetUserSettingsRequest> from_bytes(const void*, size_t) {
        return GetUserSettingsRequest{};
    }
};

// GetUserSettingsResponse: Server sends user settings
struct GetUserSettingsResponse {
    uint8_t found;  // 1 = settings found in DB, 0 = defaults
    UserSettingsPayload settings;

    static constexpr size_t WIRE_SIZE = 1 + UserSettingsPayload::WIRE_SIZE;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = found;
        settings.to_bytes(ptr + 1);
    }

    static std::optional<GetUserSettingsResponse> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        GetUserSettingsResponse resp;
        resp.found = ptr[0];
        auto settingsOpt = UserSettingsPayload::from_bytes(ptr + 1, buf_len - 1);
        if (!settingsOpt) return std::nullopt;
        resp.settings = *settingsOpt;
        return resp;
    }
};

// SaveUserSettingsRequest: Client sends settings to save
struct SaveUserSettingsRequest {
    UserSettingsPayload settings;

    static constexpr size_t WIRE_SIZE = UserSettingsPayload::WIRE_SIZE;

    void to_bytes(void* buf) const {
        settings.to_bytes(buf);
    }

    static std::optional<SaveUserSettingsRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto settingsOpt = UserSettingsPayload::from_bytes(buf, buf_len);
        if (!settingsOpt) return std::nullopt;
        SaveUserSettingsRequest req;
        req.settings = *settingsOpt;
        return req;
    }
};

// SaveUserSettingsResponse: Server confirms save
struct SaveUserSettingsResponse {
    uint8_t success;
    char message[MAX_ERROR_MSG_LEN];

    static constexpr size_t WIRE_SIZE = 1 + MAX_ERROR_MSG_LEN;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = success;
        std::memcpy(ptr + 1, message, MAX_ERROR_MSG_LEN);
    }

    static std::optional<SaveUserSettingsResponse> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        SaveUserSettingsResponse resp;
        resp.success = ptr[0];
        std::memcpy(resp.message, ptr + 1, MAX_ERROR_MSG_LEN);
        resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
        return resp;
    }
};

// ============================================================================
// Chat System Protocol Structures (TCP) - Phase 2
// ============================================================================

static constexpr size_t CHAT_MESSAGE_LEN = 256;
static constexpr uint8_t MAX_CHAT_HISTORY = 50;

// SendChatMessage: Client sends a chat message
struct SendChatMessageRequest {
    char message[CHAT_MESSAGE_LEN];

    static constexpr size_t WIRE_SIZE = CHAT_MESSAGE_LEN;

    void to_bytes(void* buf) const {
        std::memcpy(buf, message, CHAT_MESSAGE_LEN);
    }

    static std::optional<SendChatMessageRequest> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        SendChatMessageRequest req;
        std::memcpy(req.message, buf, CHAT_MESSAGE_LEN);
        req.message[CHAT_MESSAGE_LEN - 1] = '\0';
        return req;
    }
};

// SendChatMessageAck: Server confirms message received (no payload)
struct SendChatMessageAck {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(void*) const {}

    static std::optional<SendChatMessageAck> from_bytes(const void*, size_t) {
        return SendChatMessageAck{};
    }
};

// ChatMessagePayload: A single chat message (broadcast or in history)
struct ChatMessagePayload {
    char displayName[MAX_USERNAME_LEN];
    char message[CHAT_MESSAGE_LEN];
    uint32_t timestamp;  // Unix timestamp

    static constexpr size_t WIRE_SIZE = MAX_USERNAME_LEN + CHAT_MESSAGE_LEN + 4;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        std::memcpy(ptr, displayName, MAX_USERNAME_LEN);
        std::memcpy(ptr + MAX_USERNAME_LEN, message, CHAT_MESSAGE_LEN);
        uint32_t net_ts = swap32(timestamp);
        std::memcpy(ptr + MAX_USERNAME_LEN + CHAT_MESSAGE_LEN, &net_ts, 4);
    }

    static std::optional<ChatMessagePayload> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        ChatMessagePayload payload;
        std::memcpy(payload.displayName, ptr, MAX_USERNAME_LEN);
        payload.displayName[MAX_USERNAME_LEN - 1] = '\0';
        std::memcpy(payload.message, ptr + MAX_USERNAME_LEN, CHAT_MESSAGE_LEN);
        payload.message[CHAT_MESSAGE_LEN - 1] = '\0';
        uint32_t net_ts;
        std::memcpy(&net_ts, ptr + MAX_USERNAME_LEN + CHAT_MESSAGE_LEN, 4);
        payload.timestamp = swap32(net_ts);
        return payload;
    }
};

// ChatHistoryResponse: Server sends chat history on room join
struct ChatHistoryResponse {
    uint8_t messageCount;
    ChatMessagePayload messages[MAX_CHAT_HISTORY];

    size_t wire_size() const {
        return 1 + messageCount * ChatMessagePayload::WIRE_SIZE;
    }

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        ptr[0] = messageCount;
        size_t offset = 1;
        for (uint8_t i = 0; i < messageCount && i < MAX_CHAT_HISTORY; ++i) {
            messages[i].to_bytes(ptr + offset);
            offset += ChatMessagePayload::WIRE_SIZE;
        }
    }

    static std::optional<ChatHistoryResponse> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < 1) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        ChatHistoryResponse resp;
        resp.messageCount = ptr[0];
        if (resp.messageCount > MAX_CHAT_HISTORY) return std::nullopt;
        size_t required = 1 + resp.messageCount * ChatMessagePayload::WIRE_SIZE;
        if (buf_len < required) return std::nullopt;
        size_t offset = 1;
        for (uint8_t i = 0; i < resp.messageCount; ++i) {
            auto msgOpt = ChatMessagePayload::from_bytes(ptr + offset, ChatMessagePayload::WIRE_SIZE);
            if (!msgOpt) return std::nullopt;
            resp.messages[i] = *msgOpt;
            offset += ChatMessagePayload::WIRE_SIZE;
        }
        return resp;
    }
};

// ============================================================================
// UDP Protocol Structures
// ============================================================================

struct UDPHeader {
    uint16_t type;
    uint16_t sequence_num;
    uint64_t timestamp;

    static constexpr size_t WIRE_SIZE = 12;

    void to_bytes(void* buf) const {
        auto* ptr = static_cast<uint8_t*>(buf);
        uint16_t net_type = swap16(static_cast<uint16_t>(type));
        uint16_t net_sequence_num = swap16(static_cast<uint16_t>(sequence_num));
        uint64_t net_timestamp = swap64(static_cast<uint64_t>(timestamp));

        std::memcpy(ptr, &net_type, 2);
        std::memcpy(ptr + 2, &net_sequence_num, 2);
        std::memcpy(ptr + 2 + 2, &net_timestamp, 8);
    }

    static std::optional<UDPHeader> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        UDPHeader head;
        uint16_t net_type;
        uint16_t net_sequence_num;
        uint64_t net_timestamp;

        std::memcpy(&net_type, ptr, 2);
        std::memcpy(&net_sequence_num, ptr + 2, 2);
        std::memcpy(&net_timestamp, ptr + 4, 8);

        head.type = swap16(net_type);
        head.sequence_num = swap16(net_sequence_num);
        head.timestamp = swap64(net_timestamp);
        return head;
    }

    static uint64_t getTimestamp() {
        auto now = std::chrono::system_clock::now();

        auto duration = now.time_since_epoch();

        uint64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                duration)
                .count();
        return milliseconds;
    }
};

struct PlayerJoin {
    uint8_t player_id;
    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<PlayerJoin> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerJoin pj;
        pj.player_id = ptr[0];
        return pj;
    }
};

struct PlayerLeave {
    uint8_t player_id;
    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<PlayerLeave> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerLeave pl;
        pl.player_id = ptr[0];
        return pl;
    }
};

struct MissileState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x;
    uint16_t y;
    uint8_t weapon_type;  // WeaponType enum (Gameplay Phase 2)
    static constexpr size_t WIRE_SIZE = 8;  // was 7, added 1 byte for weapon_type

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        std::memcpy(buf, &net_id, 2);
        buf[2] = owner_id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 3, &net_x, 2);
        std::memcpy(buf + 5, &net_y, 2);
        buf[7] = weapon_type;
    }

    static std::optional<MissileState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        MissileState ms;
        uint16_t net_id, net_x, net_y;
        std::memcpy(&net_id, ptr, 2);
        ms.id = swap16(net_id);
        ms.owner_id = ptr[2];
        std::memcpy(&net_x, ptr + 3, 2);
        std::memcpy(&net_y, ptr + 5, 2);
        ms.x = swap16(net_x);
        ms.y = swap16(net_y);
        ms.weapon_type = ptr[7];
        return ms;
    }
};

struct ShootMissile {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(uint8_t*) const {}

    static std::optional<ShootMissile> from_bytes(const void*, size_t) {
        return ShootMissile{};
    }
};

struct HeartBeatAck {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(uint8_t*) const {}

    static std::optional<HeartBeatAck> from_bytes(const void*, size_t) {
        return HeartBeatAck{};
    }
};

struct MissileSpawned {
    uint16_t missile_id;
    uint8_t owner_id;
    uint16_t x;
    uint16_t y;
    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(missile_id);
        std::memcpy(buf, &net_id, 2);
        buf[2] = owner_id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 3, &net_x, 2);
        std::memcpy(buf + 5, &net_y, 2);
    }

    static std::optional<MissileSpawned> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        MissileSpawned ms;
        uint16_t net_id, net_x, net_y;
        std::memcpy(&net_id, ptr, 2);
        ms.missile_id = swap16(net_id);
        ms.owner_id = ptr[2];
        std::memcpy(&net_x, ptr + 3, 2);
        std::memcpy(&net_y, ptr + 5, 2);
        ms.x = swap16(net_x);
        ms.y = swap16(net_y);
        return ms;
    }
};

struct MissileDestroyed {
    uint16_t missile_id;
    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(missile_id);
        std::memcpy(buf, &net_id, 2);
    }

    static std::optional<MissileDestroyed> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        MissileDestroyed md;
        uint16_t net_id;
        std::memcpy(&net_id, ptr, 2);
        md.missile_id = swap16(net_id);
        return md;
    }
};

struct PlayerState {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t health;
    uint8_t alive;
    uint16_t lastAckedInputSeq;  // Last processed input sequence (for client-side prediction)
    uint8_t shipSkin;  // Ship skin variant (1-6 for Ship1.png to Ship6.png)
    // Score system (Gameplay Phase 2)
    uint32_t score;    // Total score
    uint16_t kills;    // Total kills
    uint8_t combo;     // Combo multiplier (x10, e.g., 15 = 1.5x, max 30 = 3.0x)
    // Weapon system (Gameplay Phase 2)
    uint8_t currentWeapon;  // WeaponType enum
    // R-Type Authentic Mechanics (Phase 3)
    uint8_t chargeLevel;   // Wave Cannon charge level (0-3)
    uint8_t speedLevel;    // Speed upgrade level (0-3)
    uint8_t weaponLevel;   // Weapon upgrade level (0-3) for damage/cooldown bonus
    uint8_t hasForce;      // 1 if player has Force Pod, 0 otherwise
    uint8_t shieldTimer;   // Shield remaining time (0-255, in tenths of seconds)
    static constexpr size_t WIRE_SIZE = 23;  // was 22, added 1 byte for weaponLevel

    void to_bytes(uint8_t* buf) const {
        buf[0] = id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        uint16_t net_seq = swap16(lastAckedInputSeq);
        std::memcpy(buf + 1, &net_x, 2);
        std::memcpy(buf + 3, &net_y, 2);
        buf[5] = health;
        buf[6] = alive;
        std::memcpy(buf + 7, &net_seq, 2);
        buf[9] = shipSkin;
        // Score fields
        uint32_t net_score = swap32(score);
        uint16_t net_kills = swap16(kills);
        std::memcpy(buf + 10, &net_score, 4);
        std::memcpy(buf + 14, &net_kills, 2);
        buf[16] = combo;
        // Weapon field
        buf[17] = currentWeapon;
        // Phase 3 fields
        buf[18] = chargeLevel;
        buf[19] = speedLevel;
        buf[20] = weaponLevel;
        buf[21] = hasForce;
        buf[22] = shieldTimer;
    }

    static std::optional<PlayerState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerState ps;
        ps.id = ptr[0];
        uint16_t net_x, net_y, net_seq;
        std::memcpy(&net_x, ptr + 1, 2);
        std::memcpy(&net_y, ptr + 3, 2);
        std::memcpy(&net_seq, ptr + 7, 2);
        ps.x = swap16(net_x);
        ps.y = swap16(net_y);
        ps.health = ptr[5];
        ps.alive = ptr[6];
        ps.lastAckedInputSeq = swap16(net_seq);
        ps.shipSkin = ptr[9];
        // Score fields
        uint32_t net_score;
        uint16_t net_kills;
        std::memcpy(&net_score, ptr + 10, 4);
        std::memcpy(&net_kills, ptr + 14, 2);
        ps.score = swap32(net_score);
        ps.kills = swap16(net_kills);
        ps.combo = ptr[16];
        // Weapon field
        ps.currentWeapon = ptr[17];
        // Phase 3 fields
        ps.chargeLevel = ptr[18];
        ps.speedLevel = ptr[19];
        ps.weaponLevel = ptr[20];
        ps.hasForce = ptr[21];
        ps.shieldTimer = ptr[22];
        return ps;
    }
};

struct EnemyState {
    uint16_t id;
    uint16_t x;
    uint16_t y;
    uint8_t health;
    uint8_t enemy_type;
    static constexpr size_t WIRE_SIZE = 8;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf, &net_id, 2);
        std::memcpy(buf + 2, &net_x, 2);
        std::memcpy(buf + 4, &net_y, 2);
        buf[6] = health;
        buf[7] = enemy_type;
    }

    static std::optional<EnemyState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        EnemyState es;
        uint16_t net_id, net_x, net_y;
        std::memcpy(&net_id, ptr, 2);
        std::memcpy(&net_x, ptr + 2, 2);
        std::memcpy(&net_y, ptr + 4, 2);
        es.id = swap16(net_id);
        es.x = swap16(net_x);
        es.y = swap16(net_y);
        es.health = ptr[6];
        es.enemy_type = ptr[7];
        return es;
    }
};

struct EnemyDestroyed {
    uint16_t enemy_id;
    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(enemy_id);
        std::memcpy(buf, &net_id, 2);
    }

    static std::optional<EnemyDestroyed> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        EnemyDestroyed ed;
        uint16_t net_id;
        std::memcpy(&net_id, ptr, 2);
        ed.enemy_id = swap16(net_id);
        return ed;
    }
};

// Boss State (Gameplay Phase 2)
struct BossState {
    uint16_t id;
    uint16_t x;
    uint16_t y;
    uint16_t max_health;
    uint16_t health;
    uint8_t phase;      // 1, 2, or 3
    uint8_t is_active;  // 0 or 1
    static constexpr size_t WIRE_SIZE = 12;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        uint16_t net_max_hp = swap16(max_health);
        uint16_t net_hp = swap16(health);
        std::memcpy(buf, &net_id, 2);
        std::memcpy(buf + 2, &net_x, 2);
        std::memcpy(buf + 4, &net_y, 2);
        std::memcpy(buf + 6, &net_max_hp, 2);
        std::memcpy(buf + 8, &net_hp, 2);
        buf[10] = phase;
        buf[11] = is_active;
    }

    static std::optional<BossState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        BossState bs;
        uint16_t net_id, net_x, net_y, net_max_hp, net_hp;
        std::memcpy(&net_id, ptr, 2);
        std::memcpy(&net_x, ptr + 2, 2);
        std::memcpy(&net_y, ptr + 4, 2);
        std::memcpy(&net_max_hp, ptr + 6, 2);
        std::memcpy(&net_hp, ptr + 8, 2);
        bs.id = swap16(net_id);
        bs.x = swap16(net_x);
        bs.y = swap16(net_y);
        bs.max_health = swap16(net_max_hp);
        bs.health = swap16(net_hp);
        bs.phase = ptr[10];
        bs.is_active = ptr[11];
        return bs;
    }
};

struct PlayerDamaged {
    uint8_t player_id;
    uint8_t damage;
    uint8_t new_health;
    static constexpr size_t WIRE_SIZE = 3;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
        buf[1] = damage;
        buf[2] = new_health;
    }

    static std::optional<PlayerDamaged> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerDamaged pd;
        pd.player_id = ptr[0];
        pd.damage = ptr[1];
        pd.new_health = ptr[2];
        return pd;
    }
};

struct PlayerDied {
    uint8_t player_id;
    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<PlayerDied> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerDied pd;
        pd.player_id = ptr[0];
        return pd;
    }
};

// ForceStateSnapshot: Compact Force Pod state for GameSnapshot
struct ForceStateSnapshot {
    uint8_t owner_id;       // Player who owns this Force
    uint16_t x;
    uint16_t y;
    uint8_t is_attached;    // 0 = detached, 1 = attached
    uint8_t level;          // 1-2

    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        buf[0] = owner_id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 1, &net_x, 2);
        std::memcpy(buf + 3, &net_y, 2);
        buf[5] = is_attached;
        buf[6] = level;
    }

    static std::optional<ForceStateSnapshot> from_bytes(const uint8_t* ptr, size_t len) {
        if (len < WIRE_SIZE) return std::nullopt;
        ForceStateSnapshot fs;
        fs.owner_id = ptr[0];
        uint16_t net_x, net_y;
        std::memcpy(&net_x, ptr + 1, 2);
        std::memcpy(&net_y, ptr + 3, 2);
        fs.x = swap16(net_x);
        fs.y = swap16(net_y);
        fs.is_attached = ptr[5];
        fs.level = ptr[6];
        return fs;
    }
};

// BitDeviceStateSnapshot: Compact Bit Device state for GameSnapshot
struct BitDeviceStateSnapshot {
    uint8_t owner_id;       // Player who owns this Bit
    uint8_t bit_index;      // 0 = top, 1 = bottom (position in orbit)
    uint16_t x;
    uint16_t y;
    uint8_t is_attached;    // 0 = detached, 1 = attached (orbiting)

    static constexpr size_t WIRE_SIZE = 6;

    void to_bytes(uint8_t* buf) const {
        buf[0] = owner_id;
        buf[1] = bit_index;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 2, &net_x, 2);
        std::memcpy(buf + 4, &net_y, 2);
        // Note: is_attached not serialized separately to save space (always attached for now)
    }

    static std::optional<BitDeviceStateSnapshot> from_bytes(const uint8_t* ptr, size_t len) {
        if (len < WIRE_SIZE) return std::nullopt;
        BitDeviceStateSnapshot bs;
        bs.owner_id = ptr[0];
        bs.bit_index = ptr[1];
        uint16_t net_x, net_y;
        std::memcpy(&net_x, ptr + 2, 2);
        std::memcpy(&net_y, ptr + 4, 2);
        bs.x = swap16(net_x);
        bs.y = swap16(net_y);
        bs.is_attached = 1;  // Always attached when sent
        return bs;
    }
};

struct GameSnapshot {
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];
    uint8_t missile_count;
    MissileState missiles[MAX_MISSILES];
    uint8_t enemy_count;
    EnemyState enemies[MAX_ENEMIES];
    uint8_t enemy_missile_count;
    MissileState enemy_missiles[MAX_ENEMY_MISSILES];
    // Gameplay Phase 2: wave info
    uint16_t wave_number;  // Current wave (0 = not started)
    // Gameplay Phase 2: boss state
    uint8_t has_boss;      // 0 or 1
    BossState boss_state;  // Only valid if has_boss == 1
    // Gameplay Phase 3: Force Pods
    uint8_t force_count;
    ForceStateSnapshot forces[MAX_PLAYERS];  // Max 4 forces (1 per player)
    // Gameplay Phase 3: Bit Devices (R-Type authentic)
    uint8_t bit_count;
    BitDeviceStateSnapshot bits[MAX_BITS];   // Max 8 bits (2 per player × 4 players)

    size_t wire_size() const {
        size_t size = 1 + player_count * PlayerState::WIRE_SIZE
             + 1 + missile_count * MissileState::WIRE_SIZE
             + 1 + enemy_count * EnemyState::WIRE_SIZE
             + 1 + enemy_missile_count * MissileState::WIRE_SIZE
             + 2   // wave_number
             + 1;  // has_boss
        if (has_boss) {
            size += BossState::WIRE_SIZE;
        }
        size += 1 + force_count * ForceStateSnapshot::WIRE_SIZE;  // Force Pods
        size += 1 + bit_count * BitDeviceStateSnapshot::WIRE_SIZE;  // Bit Devices
        return size;
    }

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_count;
        size_t offset = 1;
        for (uint8_t i = 0; i < player_count; ++i) {
            players[i].to_bytes(buf + offset);
            offset += PlayerState::WIRE_SIZE;
        }
        buf[offset] = missile_count;
        offset += 1;
        for (uint8_t i = 0; i < missile_count; ++i) {
            missiles[i].to_bytes(buf + offset);
            offset += MissileState::WIRE_SIZE;
        }
        buf[offset] = enemy_count;
        offset += 1;
        for (uint8_t i = 0; i < enemy_count; ++i) {
            enemies[i].to_bytes(buf + offset);
            offset += EnemyState::WIRE_SIZE;
        }
        buf[offset] = enemy_missile_count;
        offset += 1;
        for (uint8_t i = 0; i < enemy_missile_count; ++i) {
            enemy_missiles[i].to_bytes(buf + offset);
            offset += MissileState::WIRE_SIZE;
        }
        // Wave number
        uint16_t net_wave = swap16(wave_number);
        std::memcpy(buf + offset, &net_wave, 2);
        offset += 2;

        // Boss state
        buf[offset] = has_boss;
        offset += 1;
        if (has_boss) {
            boss_state.to_bytes(buf + offset);
            offset += BossState::WIRE_SIZE;
        }

        // Force Pods
        buf[offset] = force_count;
        offset += 1;
        for (uint8_t i = 0; i < force_count; ++i) {
            forces[i].to_bytes(buf + offset);
            offset += ForceStateSnapshot::WIRE_SIZE;
        }

        // Bit Devices
        buf[offset] = bit_count;
        offset += 1;
        for (uint8_t i = 0; i < bit_count; ++i) {
            bits[i].to_bytes(buf + offset);
            offset += BitDeviceStateSnapshot::WIRE_SIZE;
        }
    }

    static std::optional<GameSnapshot> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < 1) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        GameSnapshot gs;
        gs.player_count = ptr[0];
        if (gs.player_count > MAX_PLAYERS) {
            return std::nullopt;
        }
        size_t offset = 1;
        size_t required = offset + gs.player_count * PlayerState::WIRE_SIZE;
        if (buf_len < required) {
            return std::nullopt;
        }
        for (uint8_t i = 0; i < gs.player_count; ++i) {
            auto psOpt = PlayerState::from_bytes(ptr + offset, PlayerState::WIRE_SIZE);
            if (!psOpt) {
                return std::nullopt;
            }
            gs.players[i] = *psOpt;
            offset += PlayerState::WIRE_SIZE;
        }

        if (buf_len < offset + 1) {
            gs.missile_count = 0;
            gs.enemy_count = 0;
            gs.enemy_missile_count = 0;
            return gs;
        }
        gs.missile_count = ptr[offset];
        offset += 1;
        if (gs.missile_count > MAX_MISSILES) {
            return std::nullopt;
        }
        required = offset + gs.missile_count * MissileState::WIRE_SIZE;
        if (buf_len < required) {
            return std::nullopt;
        }
        for (uint8_t i = 0; i < gs.missile_count; ++i) {
            auto msOpt = MissileState::from_bytes(ptr + offset, MissileState::WIRE_SIZE);
            if (!msOpt) {
                return std::nullopt;
            }
            gs.missiles[i] = *msOpt;
            offset += MissileState::WIRE_SIZE;
        }

        if (buf_len < offset + 1) {
            gs.enemy_count = 0;
            gs.enemy_missile_count = 0;
            return gs;
        }
        gs.enemy_count = ptr[offset];
        offset += 1;
        if (gs.enemy_count > MAX_ENEMIES) {
            return std::nullopt;
        }
        required = offset + gs.enemy_count * EnemyState::WIRE_SIZE;
        if (buf_len < required) {
            return std::nullopt;
        }
        for (uint8_t i = 0; i < gs.enemy_count; ++i) {
            auto esOpt = EnemyState::from_bytes(ptr + offset, EnemyState::WIRE_SIZE);
            if (!esOpt) {
                return std::nullopt;
            }
            gs.enemies[i] = *esOpt;
            offset += EnemyState::WIRE_SIZE;
        }

        if (buf_len < offset + 1) {
            gs.enemy_missile_count = 0;
            return gs;
        }
        gs.enemy_missile_count = ptr[offset];
        offset += 1;
        if (gs.enemy_missile_count > MAX_ENEMY_MISSILES) {
            return std::nullopt;
        }
        required = offset + gs.enemy_missile_count * MissileState::WIRE_SIZE;
        if (buf_len < required) {
            return std::nullopt;
        }
        for (uint8_t i = 0; i < gs.enemy_missile_count; ++i) {
            auto msOpt = MissileState::from_bytes(ptr + offset, MissileState::WIRE_SIZE);
            if (!msOpt) {
                return std::nullopt;
            }
            gs.enemy_missiles[i] = *msOpt;
            offset += MissileState::WIRE_SIZE;
        }

        // Wave number (optional for backward compat, default 0)
        if (buf_len >= offset + 2) {
            uint16_t net_wave;
            std::memcpy(&net_wave, ptr + offset, 2);
            gs.wave_number = swap16(net_wave);
            offset += 2;
        } else {
            gs.wave_number = 0;
            gs.has_boss = 0;
            return gs;
        }

        // Boss state (optional for backward compat)
        if (buf_len >= offset + 1) {
            gs.has_boss = ptr[offset];
            offset += 1;
            if (gs.has_boss && buf_len >= offset + BossState::WIRE_SIZE) {
                auto bsOpt = BossState::from_bytes(ptr + offset, BossState::WIRE_SIZE);
                if (bsOpt) {
                    gs.boss_state = *bsOpt;
                    offset += BossState::WIRE_SIZE;
                } else {
                    gs.has_boss = 0;
                }
            }
        } else {
            gs.has_boss = 0;
            gs.force_count = 0;
            return gs;
        }

        // Force Pods (optional for backward compat)
        if (buf_len >= offset + 1) {
            gs.force_count = ptr[offset];
            offset += 1;
            if (gs.force_count > MAX_PLAYERS) {
                gs.force_count = 0;
                return gs;
            }
            size_t force_required = offset + gs.force_count * ForceStateSnapshot::WIRE_SIZE;
            if (buf_len >= force_required) {
                for (uint8_t i = 0; i < gs.force_count; ++i) {
                    auto fsOpt = ForceStateSnapshot::from_bytes(ptr + offset, ForceStateSnapshot::WIRE_SIZE);
                    if (fsOpt) {
                        gs.forces[i] = *fsOpt;
                    }
                    offset += ForceStateSnapshot::WIRE_SIZE;
                }
            } else {
                gs.force_count = 0;
            }
        } else {
            gs.force_count = 0;
            gs.bit_count = 0;
            return gs;
        }

        // Bit Devices (optional for backward compat)
        if (buf_len >= offset + 1) {
            gs.bit_count = ptr[offset];
            offset += 1;
            if (gs.bit_count > MAX_BITS) {
                gs.bit_count = 0;
                return gs;
            }
            size_t bit_required = offset + gs.bit_count * BitDeviceStateSnapshot::WIRE_SIZE;
            if (buf_len >= bit_required) {
                for (uint8_t i = 0; i < gs.bit_count; ++i) {
                    auto bsOpt = BitDeviceStateSnapshot::from_bytes(ptr + offset, BitDeviceStateSnapshot::WIRE_SIZE);
                    if (bsOpt) {
                        gs.bits[i] = *bsOpt;
                    }
                    offset += BitDeviceStateSnapshot::WIRE_SIZE;
                }
            } else {
                gs.bit_count = 0;
            }
        } else {
            gs.bit_count = 0;
        }

        return gs;
    }
};

// ============================================================================
// Voice Chat Protocol Structures (UDP port 4126)
// ============================================================================

// Voice chat constants
static constexpr uint16_t VOICE_UDP_PORT = 4126;
static constexpr size_t MAX_OPUS_FRAME_SIZE = 480;  // Max Opus frame at 32kbps, 20ms

// VoiceJoin: Client requests to join voice channel for a room
struct VoiceJoin {
    SessionToken token;                    // 32 bytes - reuse existing auth
    char roomCode[ROOM_CODE_LEN];          // 6 bytes

    static constexpr size_t WIRE_SIZE = TOKEN_SIZE + ROOM_CODE_LEN;

    void to_bytes(uint8_t* buf) const {
        token.to_bytes(buf);
        std::memcpy(buf + TOKEN_SIZE, roomCode, ROOM_CODE_LEN);
    }

    static std::optional<VoiceJoin> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto tokenOpt = SessionToken::from_bytes(buf, len);
        if (!tokenOpt) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        VoiceJoin msg;
        msg.token = *tokenOpt;
        std::memcpy(msg.roomCode, ptr + TOKEN_SIZE, ROOM_CODE_LEN);
        return msg;
    }
};

// VoiceJoinAck: Server confirms voice channel join
struct VoiceJoinAck {
    uint8_t player_id;                     // 1 byte - assigned player ID

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<VoiceJoinAck> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        return VoiceJoinAck{.player_id = ptr[0]};
    }
};

// VoiceLeave: Client leaves voice channel
struct VoiceLeave {
    uint8_t player_id;                     // 1 byte

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
    }

    static std::optional<VoiceLeave> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        return VoiceLeave{.player_id = ptr[0]};
    }
};

// VoiceFrame: Compressed audio data (Opus codec)
struct VoiceFrame {
    uint8_t speaker_id;                    // 1 byte - who is speaking
    uint16_t sequence;                     // 2 bytes - for packet loss detection
    uint16_t opus_len;                     // 2 bytes - actual Opus data length
    uint8_t opus_data[MAX_OPUS_FRAME_SIZE]; // max 480 bytes

    static constexpr size_t HEADER_SIZE = 5;  // speaker_id + sequence + opus_len
    static constexpr size_t MAX_WIRE_SIZE = HEADER_SIZE + MAX_OPUS_FRAME_SIZE;

    // Returns actual wire size (header + opus_len)
    size_t wire_size() const {
        return HEADER_SIZE + opus_len;
    }

    void to_bytes(uint8_t* buf) const {
        buf[0] = speaker_id;
        uint16_t net_seq = swap16(sequence);
        uint16_t net_len = swap16(opus_len);
        std::memcpy(buf + 1, &net_seq, 2);
        std::memcpy(buf + 3, &net_len, 2);
        std::memcpy(buf + HEADER_SIZE, opus_data, opus_len);
    }

    static std::optional<VoiceFrame> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < HEADER_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);

        VoiceFrame frame;
        frame.speaker_id = ptr[0];

        uint16_t net_seq, net_len;
        std::memcpy(&net_seq, ptr + 1, 2);
        std::memcpy(&net_len, ptr + 3, 2);
        frame.sequence = swap16(net_seq);
        frame.opus_len = swap16(net_len);

        // Validate opus_len
        if (frame.opus_len > MAX_OPUS_FRAME_SIZE) return std::nullopt;
        if (len < HEADER_SIZE + frame.opus_len) return std::nullopt;

        std::memcpy(frame.opus_data, ptr + HEADER_SIZE, frame.opus_len);
        return frame;
    }
};

// VoiceMute: Notification that a player muted/unmuted
struct VoiceMute {
    uint8_t player_id;                     // 1 byte
    uint8_t muted;                         // 1 byte - 0 = unmuted, 1 = muted

    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_id;
        buf[1] = muted;
    }

    static std::optional<VoiceMute> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        return VoiceMute{.player_id = ptr[0], .muted = ptr[1]};
    }
};

// =============================================================================
// R-Type Authentic Mechanics - Phase 3
// =============================================================================

// ChargeRelease: Client releases charge to fire Wave Cannon
struct ChargeRelease {
    uint8_t charge_level;  // 1-3 (0 means no charge, fire normal shot)

    static constexpr size_t WIRE_SIZE = 1;

    void to_bytes(uint8_t* buf) const {
        buf[0] = charge_level;
    }

    static std::optional<ChargeRelease> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        return ChargeRelease{.charge_level = ptr[0]};
    }
};

// WaveCannonState: Wave Cannon projectile state (in snapshot or spawned message)
struct WaveCannonState {
    uint16_t id;
    uint8_t owner_id;
    uint16_t x;
    uint16_t y;
    uint8_t charge_level;  // 1-3
    uint8_t width;         // Beam width (for rendering)

    static constexpr size_t WIRE_SIZE = 9;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf, &net_id, 2);
        buf[2] = owner_id;
        std::memcpy(buf + 3, &net_x, 2);
        std::memcpy(buf + 5, &net_y, 2);
        buf[7] = charge_level;
        buf[8] = width;
    }

    static std::optional<WaveCannonState> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        WaveCannonState wc;
        uint16_t net_id, net_x, net_y;
        std::memcpy(&net_id, ptr, 2);
        std::memcpy(&net_x, ptr + 3, 2);
        std::memcpy(&net_y, ptr + 5, 2);
        wc.id = swap16(net_id);
        wc.owner_id = ptr[2];
        wc.x = swap16(net_x);
        wc.y = swap16(net_y);
        wc.charge_level = ptr[7];
        wc.width = ptr[8];
        return wc;
    }
};

// PowerUpState: Power-up item on the field
struct PowerUpState {
    uint16_t id;
    uint16_t x;
    uint16_t y;
    uint8_t type;           // PowerUpType enum
    uint8_t remaining_time; // Seconds before expiration (max 10s)

    static constexpr size_t WIRE_SIZE = 8;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf, &net_id, 2);
        std::memcpy(buf + 2, &net_x, 2);
        std::memcpy(buf + 4, &net_y, 2);
        buf[6] = type;
        buf[7] = remaining_time;
    }

    static std::optional<PowerUpState> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        PowerUpState pu;
        uint16_t net_id, net_x, net_y;
        std::memcpy(&net_id, ptr, 2);
        std::memcpy(&net_x, ptr + 2, 2);
        std::memcpy(&net_y, ptr + 4, 2);
        pu.id = swap16(net_id);
        pu.x = swap16(net_x);
        pu.y = swap16(net_y);
        pu.type = ptr[6];
        pu.remaining_time = ptr[7];
        return pu;
    }
};

// PowerUpCollected: Notification when a player collects a power-up
struct PowerUpCollected {
    uint16_t powerup_id;
    uint8_t player_id;
    uint8_t powerup_type;  // PowerUpType enum

    static constexpr size_t WIRE_SIZE = 4;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(powerup_id);
        std::memcpy(buf, &net_id, 2);
        buf[2] = player_id;
        buf[3] = powerup_type;
    }

    static std::optional<PowerUpCollected> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        PowerUpCollected pc;
        uint16_t net_id;
        std::memcpy(&net_id, ptr, 2);
        pc.powerup_id = swap16(net_id);
        pc.player_id = ptr[2];
        pc.powerup_type = ptr[3];
        return pc;
    }
};

// PowerUpExpired: Notification when a power-up times out
struct PowerUpExpired {
    uint16_t powerup_id;

    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(powerup_id);
        std::memcpy(buf, &net_id, 2);
    }

    static std::optional<PowerUpExpired> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        PowerUpExpired pe;
        uint16_t net_id;
        std::memcpy(&net_id, ptr, 2);
        pe.powerup_id = swap16(net_id);
        return pe;
    }
};

// ForceState: Force Pod state for a player
struct ForceState {
    uint8_t owner_id;       // Player who owns this Force
    uint16_t x;
    uint16_t y;
    uint8_t is_attached;    // 0 = detached (free-floating), 1 = attached to ship
    uint8_t level;          // 0 = no Force, 1-2 = Force level

    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        buf[0] = owner_id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 1, &net_x, 2);
        std::memcpy(buf + 3, &net_y, 2);
        buf[5] = is_attached;
        buf[6] = level;
    }

    static std::optional<ForceState> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        ForceState fs;
        fs.owner_id = ptr[0];
        uint16_t net_x, net_y;
        std::memcpy(&net_x, ptr + 1, 2);
        std::memcpy(&net_y, ptr + 3, 2);
        fs.x = swap16(net_x);
        fs.y = swap16(net_y);
        fs.is_attached = ptr[5];
        fs.level = ptr[6];
        return fs;
    }
};

// ForceToggle: Client requests to attach/detach Force Pod
struct ForceToggle {
    static constexpr size_t WIRE_SIZE = 0;

    void to_bytes(uint8_t*) const {}

    static std::optional<ForceToggle> from_bytes(const void*, size_t) {
        return ForceToggle{};
    }
};

#endif /* !PROTOCOL_HPP_ */
