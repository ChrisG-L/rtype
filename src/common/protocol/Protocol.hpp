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

inline uint32_t swap64(uint32_t v) { return __builtin_bswap64(v); }
inline uint32_t swap32(uint32_t v) { return __builtin_bswap32(v); }
inline uint16_t swap16(uint16_t v) { return __builtin_bswap16(v); }

static constexpr std::size_t BUFFER_SIZE = 4096;

enum class MessageType: uint16_t {
    HeartBeat = 0x0001,
    Basic = 0x0030,
    BasicAck = 0x0031,
    Snapshot = 0x0040,
    Player = 0x0050,
    MovePlayer = 0x0060,
    PlayerJoin = 0x0070,
    PlayerLeave = 0x0071,
};

static constexpr uint8_t MAX_PLAYERS = 4;

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

struct PlayerState {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t alive;
    static constexpr size_t WIRE_SIZE = 6;

    void to_bytes(uint8_t* buf) const {
        buf[0] = id;
        uint16_t net_x = swap16(x);
        uint16_t net_y = swap16(y);
        std::memcpy(buf + 1, &net_x, 2);
        std::memcpy(buf + 3, &net_y, 2);
        buf[5] = alive;
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
        ps.alive = ptr[5];
        return ps;
    }
};

struct GameSnapshot {
    uint8_t player_count;
    PlayerState players[MAX_PLAYERS];

    size_t wire_size() const {
        return 1 + player_count * PlayerState::WIRE_SIZE;
    }

    void to_bytes(uint8_t* buf) const {
        buf[0] = player_count;
        for (uint8_t i = 0; i < player_count; ++i) {
            players[i].to_bytes(buf + 1 + i * PlayerState::WIRE_SIZE);
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
        size_t required = 1 + gs.player_count * PlayerState::WIRE_SIZE;
        if (buf_len < required) {
            return std::nullopt;
        }
        for (uint8_t i = 0; i < gs.player_count; ++i) {
            auto psOpt = PlayerState::from_bytes(ptr + 1 + i * PlayerState::WIRE_SIZE,
                                                  PlayerState::WIRE_SIZE);
            if (!psOpt) {
                return std::nullopt;
            }
            gs.players[i] = *psOpt;
        }
        return gs;
    }
};


#endif /* !PROTOCOL_HPP_ */
