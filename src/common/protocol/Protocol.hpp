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

inline uint32_t swap64(uint32_t v) { return __builtin_bswap64(v); }
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
    Basic = 0x0030,
    BasicAck = 0x0031,
    Snapshot = 0x0040,
    Player = 0x0050,
    MovePlayer = 0x0060,
    PlayerInput = 0x0061,
    PlayerJoin = 0x0070,
    PlayerLeave = 0x0071,
    ShootMissile = 0x0080,
    MissileSpawned = 0x0081,
    MissileDestroyed = 0x0082,
    EnemySpawned = 0x0090,
    EnemyDestroyed = 0x0091,
    PlayerDamaged = 0x00A0,
    PlayerDied = 0x00A1,
    // TCP Authentication messages
    Login = 0x0100,
    LoginAck = 0x0101,
    Register = 0x0102,
    RegisterAck = 0x0103,
};

static constexpr uint8_t MAX_PLAYERS = 4;
static constexpr uint8_t MAX_MISSILES = 32;
static constexpr uint8_t MAX_ENEMIES = 16;
static constexpr uint8_t MAX_ENEMY_MISSILES = 32;
static constexpr uint8_t ENEMY_OWNER_ID = 0xFF;

// Session token size (256 bits = 32 bytes)
static constexpr size_t TOKEN_SIZE = 32;

// Input keys bitfield
namespace InputKeys {
    constexpr uint16_t UP    = 0x0001;
    constexpr uint16_t DOWN  = 0x0002;
    constexpr uint16_t LEFT  = 0x0004;
    constexpr uint16_t RIGHT = 0x0008;
    constexpr uint16_t SHOOT = 0x0010;
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
    static constexpr size_t WIRE_SIZE = TOKEN_SIZE;

    void to_bytes(uint8_t* buf) const {
        token.to_bytes(buf);
    }

    static std::optional<JoinGame> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto tokenOpt = SessionToken::from_bytes(buf, len);
        if (!tokenOpt) return std::nullopt;
        return JoinGame{.token = *tokenOpt};
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
    uint16_t keys;  // Bitfield using InputKeys namespace
    static constexpr size_t WIRE_SIZE = 2;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_keys = swap16(keys);
        std::memcpy(buf, &net_keys, 2);
    }

    static std::optional<PlayerInput> from_bytes(const void* buf, size_t len) {
        if (buf == nullptr || len < WIRE_SIZE) return std::nullopt;
        auto* ptr = static_cast<const uint8_t*>(buf);
        uint16_t net_keys;
        std::memcpy(&net_keys, ptr, 2);
        return PlayerInput{.keys = swap16(net_keys)};
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
static constexpr size_t MAX_EMAIL_LEN = 64;

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


struct MovePlayer {
    uint16_t x;
    uint16_t y;
    static constexpr size_t WIRE_SIZE = 4;


    void to_bytes(uint8_t* buf) const {
        uint16_t x_pos = swap16(static_cast<uint16_t>(x));
        uint16_t y_pos = swap16(static_cast<uint16_t>(y));

        std::memcpy(buf, &x_pos, 2);
        std::memcpy(buf + 2, &y_pos, 2);
    }

    static std::optional<MovePlayer> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        MovePlayer movePlayer;
        uint16_t x_pos;
        uint16_t y_pos;

        memcpy(&x_pos, ptr, 2);
        memcpy(&y_pos, ptr + 2, 2);

        movePlayer.x = swap16(x_pos);
        movePlayer.y = swap16(y_pos);
        return movePlayer;
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
    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_id = swap16(id);
        std::memcpy(buf, &net_id, 2);
        buf[2] = owner_id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 3, &net_x, 2);
        std::memcpy(buf + 5, &net_y, 2);
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
    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        buf[0] = id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 1, &net_x, 2);
        std::memcpy(buf + 3, &net_y, 2);
        buf[5] = health;
        buf[6] = alive;
    }

    static std::optional<PlayerState> from_bytes(const void* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        auto* ptr = static_cast<const uint8_t*>(buf);
        PlayerState ps;
        ps.id = ptr[0];
        uint16_t net_x, net_y;
        std::memcpy(&net_x, ptr + 1, 2);
        std::memcpy(&net_y, ptr + 3, 2);
        ps.x = swap16(net_x);
        ps.y = swap16(net_y);
        ps.health = ptr[5];
        ps.alive = ptr[6];
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

struct GameSnapshot {
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];
    uint8_t missile_count;
    MissileState missiles[MAX_MISSILES];
    uint8_t enemy_count;
    EnemyState enemies[MAX_ENEMIES];
    uint8_t enemy_missile_count;
    MissileState enemy_missiles[MAX_ENEMY_MISSILES];

    size_t wire_size() const {
        return 1 + player_count * PlayerState::WIRE_SIZE
             + 1 + missile_count * MissileState::WIRE_SIZE
             + 1 + enemy_count * EnemyState::WIRE_SIZE
             + 1 + enemy_missile_count * MissileState::WIRE_SIZE;
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

        return gs;
    }
};


#endif /* !PROTOCOL_HPP_ */
