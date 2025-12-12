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
    Login = 0x0010,
    LoginAck = 0x0011,
    Register = 0x0020,
    RegisterAck = 0x0021,
    Basic = 0x0030,
    BasicAck = 0x0031,
    Snapshot = 0x0040,
    Player = 0x0050,
    MovePlayer = 0x0060,
};

struct Header {
    uint8_t isAuthenticated;
    uint16_t type;
    uint32_t payload_size;

    static constexpr size_t WIRE_SIZE = 7;

    void to_bytes(uint8_t* buf) const {
        uint8_t net_isAuthenticated = isAuthenticated;
        uint16_t net_type = swap16(static_cast<uint16_t>(type));
        uint32_t net_size = swap32(static_cast<uint32_t>(payload_size));
    
        std::memcpy(buf, &net_isAuthenticated, 1);
        std::memcpy(buf + 1, &net_type, 2);
        std::memcpy(buf + 2 + 1, &net_size, 4);
    }

    static std::optional<Header> from_bytes(const uint8_t* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        Header head;
        uint8_t net_isAuthenticated;
        uint16_t net_type;
        uint32_t net_size;
        std::memcpy(&net_isAuthenticated, buf, 1);
        std::memcpy(&net_type, buf + 1, 2);
        std::memcpy(&net_size, buf + 2 + 1, 4);
        head.isAuthenticated = net_isAuthenticated;
        head.type = swap16(net_type);
        head.payload_size = swap32(net_size);
        return head;
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


struct LoginMessage {
    char username[32];
    char password[255];

    void to_bytes(uint8_t* buf) const {
        std::memcpy(buf, username, 32);
        std::memcpy(buf + 32, password, 255);
    }

    static constexpr size_t WIRE_SIZE = 32 + 255;

    static std::optional<LoginMessage> from_bytes(const uint8_t* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        LoginMessage login;
        memcpy(&login.username, buf, 32);
        memcpy(&login.password, buf + 32, 255);
        return login;
    }
};

struct RegisterMessage {
    char username[32];
    char email[255];
    char password[255];

    void to_bytes(uint8_t* buf) const {
        std::memcpy(buf, username, 32);
        std::memcpy(buf + 32, email, 255);
        std::memcpy(buf + 32 + 255, password, 255);
    }

    static constexpr size_t WIRE_SIZE = 32 + 255 + 255;

    static std::optional<RegisterMessage> from_bytes(const uint8_t* buf, size_t buf_len) {
        if (buf == nullptr || buf_len < WIRE_SIZE) {
            return std::nullopt;
        }
        RegisterMessage registerUser;
        memcpy(&registerUser.username, buf, 32);
        memcpy(&registerUser.email, buf + 32, 255);
        memcpy(&registerUser.password, buf + 32 + 255, 255);
        return registerUser;
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


#endif /* !PROTOCOL_HPP_ */
