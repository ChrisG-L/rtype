---
tags:
  - technique
  - reseau
  - reference
---

# Structures Binaires

Référence complète des structures réseau.

## Header Commun

```cpp
struct PacketHeader {
    uint16_t magic;      // 0x5254 ("RT")
    uint8_t type;        // Type de paquet
    uint32_t sequence;   // Numéro de séquence
    uint16_t size;       // Taille des données
};
// Total: 9 bytes
```

---

## Types (enum)

```cpp
enum class PacketType : uint8_t {
    // TCP - Auth
    AUTH_REQUEST    = 0x01,
    AUTH_RESPONSE   = 0x02,

    // TCP - Rooms
    CREATE_ROOM     = 0x03,
    JOIN_ROOM       = 0x04,
    LEAVE_ROOM      = 0x05,
    ROOM_LIST       = 0x06,
    ROOM_INFO       = 0x07,

    // TCP - Chat
    CHAT_MESSAGE    = 0x10,
    PLAYER_LIST     = 0x11,

    // TCP - Game
    GAME_START      = 0x20,
    GAME_END        = 0x21,

    // UDP - Input
    INPUT           = 0x30,

    // UDP - State
    STATE           = 0x40,
    SPAWN           = 0x41,
    DESTROY         = 0x42,
    HIT             = 0x43,
    SCORE           = 0x44,

    // UDP - Ping
    PING            = 0x50,
    PONG            = 0x51,

    // Voice
    VOICE_DATA      = 0x60,
    VOICE_MUTE      = 0x61,
    VOICE_UNMUTE    = 0x62,

    // Error
    ERROR           = 0xFF
};
```

---

## TCP Structures

### AuthRequest

```
Offset  Size  Field
0x00    32    username (null-terminated)
0x20    64    password_hash (SHA256 hex)
```

### AuthResponse

```
Offset  Size  Field
0x00    1     success (0/1)
0x01    4     player_id
0x05    128   error_msg (if fail)
```

### CreateRoom

```
Offset  Size  Field
0x00    32    room_name
0x20    1     max_players (1-4)
0x21    1     is_private
0x22    32    password (if private)
```

### RoomInfo

```
Offset  Size  Field
0x00    4     room_id
0x04    32    room_name
0x24    1     player_count
0x25    1     max_players
0x26    1     state (0=lobby, 1=playing)
0x27    N*36  players[] (PlayerInfo)
```

### PlayerInfo

```
Offset  Size  Field
0x00    4     player_id
0x04    32    username
```

### ChatMessage

```
Offset  Size  Field
0x00    4     sender_id
0x04    4     room_id
0x08    8     timestamp
0x10    256   message
```

---

## UDP Structures

### Input

```
Offset  Size  Field
0x00    4     sequence
0x04    4     player_id
0x08    1     keys (bitmask)
0x09    4     timestamp (float)
```

Keys bitmask:
```
Bit 0: UP
Bit 1: DOWN
Bit 2: LEFT
Bit 3: RIGHT
Bit 4: SHOOT
```

### State

```
Offset  Size  Field
0x00    4     tick
0x04    4     last_ack (dernier input traité)
0x08    4     entity_count
0x0C    N*24  entities[] (EntityState)
```

### EntityState

```
Offset  Size  Field
0x00    4     entity_id
0x04    1     type
0x05    4     x (float)
0x09    4     y (float)
0x0D    4     vx (float)
0x11    4     vy (float)
0x15    2     health
0x17    1     flags
```

Entity types:
```cpp
enum EntityType : uint8_t {
    PLAYER      = 0,
    ENEMY_BASIC = 1,
    ENEMY_ZIGZAG = 2,
    ENEMY_FOLLOWER = 3,
    ENEMY_SHOOTER = 4,
    BOSS        = 5,
    MISSILE     = 10,
    POWERUP     = 20
};
```

### Spawn

```
Offset  Size  Field
0x00    4     entity_id
0x04    1     type
0x05    4     x (float)
0x09    4     y (float)
0x0D    4     owner_id
```

### Destroy

```
Offset  Size  Field
0x00    4     entity_id
0x04    1     reason
```

---

## Voice Structures

### VoiceData

```
Offset  Size  Field
0x00    4     sender_id
0x04    4     sequence
0x08    8     timestamp
0x10    2     opus_length
0x12    N     opus_data[]
```

---

## Sérialisation

```cpp
class Packet {
    std::vector<uint8_t> data_;
    size_t readPos_ = 0;

public:
    template<typename T>
    void write(const T& value) {
        auto ptr = reinterpret_cast<const uint8_t*>(&value);
        data_.insert(data_.end(), ptr, ptr + sizeof(T));
    }

    void write(const std::string& str, size_t maxLen) {
        size_t len = std::min(str.size(), maxLen);
        data_.insert(data_.end(), str.begin(), str.begin() + len);
        data_.resize(data_.size() + (maxLen - len), 0);
    }

    template<typename T>
    T read() {
        T value;
        std::memcpy(&value, &data_[readPos_], sizeof(T));
        readPos_ += sizeof(T);
        return value;
    }
};
```

---

## Endianness

Toutes les valeurs sont en **little-endian** (x86 natif).

Pour la portabilité :

```cpp
uint32_t toLE(uint32_t value) {
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return __builtin_bswap32(value);
    #else
        return value;
    #endif
}
```
