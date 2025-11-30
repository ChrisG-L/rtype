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
#include <iostream>

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
    MovePlayer = 0x0040,
};

struct Header {
    uint16_t type;
    uint32_t payload_size;

    static constexpr size_t WIRE_SIZE = 6;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_type = swap16(static_cast<uint16_t>(type));
        uint32_t net_size = swap32(static_cast<uint32_t>(payload_size));
    
        std::memcpy(buf, &net_type, 2);
        std::memcpy(buf + 2, &net_size, 4);
    }

    static Header from_bytes(const uint8_t* buf) {
        Header head;
        uint16_t net_type;
        uint32_t net_size;
        std::memcpy(&net_type, buf, 2);
        std::memcpy(&net_size, buf + 2, 4);
        head.type = swap16(net_type),
        head.payload_size = swap32(net_size);
        return head;
    }
};

struct UDPHeader {
    uint16_t type;
    uint16_t sequence_num;

    static constexpr size_t WIRE_SIZE = 4;

    void to_bytes(uint8_t* buf) const {
        uint16_t net_type = swap16(static_cast<uint16_t>(type));
        uint32_t net_sequence_num = swap16(static_cast<uint16_t>(sequence_num));
    
        std::memcpy(buf, &net_type, 2);
        std::memcpy(buf + 2, &net_sequence_num, 2);
    }

    static UDPHeader from_bytes(const uint8_t* buf) {
        UDPHeader head;
        uint16_t net_type;
        uint32_t net_sequence_num;
        std::memcpy(&net_type, buf, 2);
        std::memcpy(&net_sequence_num, buf + 2, 2);
        head.type = swap16(net_type),
        head.sequence_num = swap16(net_sequence_num);
        return head;
    }
};

struct LoginMessage {
    char username[32];
    char password[255];

    void to_bytes(uint8_t* buf) const {
        std::memcpy(buf, username, 32);
        std::memcpy(buf + 32, password, 255);
    }

    static LoginMessage from_bytes(const uint8_t* buf) {
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

    static RegisterMessage from_bytes(const uint8_t* buf) {
        RegisterMessage registerUser;
        memcpy(&registerUser.username, buf, 32);
        memcpy(&registerUser.email, buf + 32, 255);
        memcpy(&registerUser.password, buf + 32 + 255, 255);
        return registerUser;
    }
};

#endif /* !PROTOCOL_HPP_ */
