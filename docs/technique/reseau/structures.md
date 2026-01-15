---
tags:
  - technique
  - reseau
  - reference
---

# Structures Binaires

Référence complète des structures réseau R-Type.

## Endianness

Toutes les valeurs multi-octets sont en **network byte order** (big-endian).

```cpp
// Conversion host → network
inline uint16_t swap16(uint16_t v) { return __builtin_bswap16(v); }
inline uint32_t swap32(uint32_t v) { return __builtin_bswap32(v); }
inline uint64_t swap64(uint64_t v) { return __builtin_bswap64(v); }
```

---

## Headers

### TCP Header (7 bytes)

```
┌───────────────┬──────────┬─────────────┐
│ isAuthenticated│ Type     │ PayloadSize │
│ 1 byte        │ 2 bytes  │ 4 bytes     │
└───────────────┴──────────┴─────────────┘
```

```cpp
struct Header {
    uint8_t isAuthenticated;   // 0 ou 1
    uint16_t type;             // MessageType (network order)
    uint32_t payload_size;     // Taille payload (network order)
};
```

### UDP Header (12 bytes)

```
┌──────────┬──────────┬────────────┐
│ Type     │ Sequence │ Timestamp  │
│ 2 bytes  │ 2 bytes  │ 8 bytes    │
└──────────┴──────────┴────────────┘
```

```cpp
struct UDPHeader {
    uint16_t type;          // MessageType (network order)
    uint16_t sequence_num;  // Numéro de séquence
    uint64_t timestamp;     // Millisecondes depuis epoch
};
```

---

## Types (enum MessageType)

```cpp
enum class MessageType : uint16_t {
    // UDP - Heartbeat
    HeartBeat        = 0x0001,
    HeartBeatAck     = 0x0002,

    // UDP - Session
    JoinGame         = 0x0010,
    JoinGameAck      = 0x0011,
    JoinGameNack     = 0x0012,

    // UDP - Game State
    Snapshot         = 0x0040,
    PlayerInput      = 0x0061,
    PlayerJoin       = 0x0070,
    PlayerLeave      = 0x0071,

    // UDP - Combat
    ShootMissile     = 0x0080,
    MissileSpawned   = 0x0081,
    MissileDestroyed = 0x0082,
    EnemyDestroyed   = 0x0091,
    PlayerDamaged    = 0x00A0,
    PlayerDied       = 0x00A1,

    // TCP - Auth
    Login            = 0x0100,
    LoginAck         = 0x0101,
    Register         = 0x0102,
    RegisterAck      = 0x0103,

    // TCP - Rooms
    CreateRoom       = 0x0200,
    CreateRoomAck    = 0x0201,
    JoinRoomByCode   = 0x0210,
    JoinRoomAck      = 0x0211,
    JoinRoomNack     = 0x0212,
    LeaveRoom        = 0x0220,
    SetReady         = 0x0230,
    StartGame        = 0x0240,
    RoomUpdate       = 0x0250,
    GameStarting     = 0x0251,

    // TCP - Room Browser
    BrowsePublicRooms    = 0x0270,
    BrowsePublicRoomsAck = 0x0271,
    QuickJoin            = 0x0272,
    QuickJoinAck         = 0x0273,
    QuickJoinNack        = 0x0274,

    // TCP - Settings
    GetUserSettings      = 0x0280,
    GetUserSettingsAck   = 0x0281,
    SaveUserSettings     = 0x0282,
    SaveUserSettingsAck  = 0x0283,

    // TCP - Chat
    SendChatMessage      = 0x0290,
    ChatMessageBroadcast = 0x0292,
    ChatHistory          = 0x0293,

    // Voice (UDP 4126)
    VoiceJoin        = 0x0300,
    VoiceJoinAck     = 0x0301,
    VoiceLeave       = 0x0302,
    VoiceFrame       = 0x0303,
    VoiceMute        = 0x0304,
};
```

---

## Structures TCP

### LoginMessage (96 bytes)

```
Offset  Size  Field
0x00    32    username (null-terminated)
0x20    64    password
```

### AuthResponseWithToken (193 bytes)

```
Offset  Size  Field
0x00    1     success (0/1)
0x01    32    error_code
0x21    128   message
0xA1    32    token (SessionToken)
```

### SessionToken (32 bytes)

```
Offset  Size  Field
0x00    32    bytes[32] (CSPRNG)
```

### RoomPlayerState (292 bytes)

```
Offset  Size  Field
0x00    1     slotId (0-5)
0x01    1     occupied (0/1)
0x02    32    displayName
0x22    255   email
0x121   1     isReady
0x122   1     isHost
0x123   1     shipSkin (1-6)
```

---

## Structures UDP

### PlayerInput (4 bytes)

```
Offset  Size  Field
0x00    2     keys (bitfield, network order)
0x02    2     sequenceNum (network order)
```

Keys bitfield:
```
Bit 0: UP     (0x0001)
Bit 1: DOWN   (0x0002)
Bit 2: LEFT   (0x0004)
Bit 3: RIGHT  (0x0008)
Bit 4: SHOOT  (0x0010)
```

### PlayerState (10 bytes)

```
Offset  Size  Field
0x00    1     id
0x01    2     x (network order)
0x03    2     y (network order)
0x05    1     health (0-100)
0x06    1     alive (0/1)
0x07    2     lastAckedInputSeq (network order)
0x09    1     shipSkin (1-6)
```

### MissileState (7 bytes)

```
Offset  Size  Field
0x00    2     id (network order)
0x02    1     owner_id (0xFF = enemy)
0x03    2     x (network order)
0x05    2     y (network order)
```

### EnemyState (8 bytes)

```
Offset  Size  Field
0x00    2     id (network order)
0x02    2     x (network order)
0x04    2     y (network order)
0x06    1     health
0x07    1     enemy_type
```

### GameSnapshot (variable)

```
Offset  Size     Field
0x00    1        player_count
0x01    N*10     players[N] (PlayerState)
...     1        missile_count
...     M*7      missiles[M] (MissileState)
...     1        enemy_count
...     E*8      enemies[E] (EnemyState)
...     1        enemy_missile_count
...     EM*7     enemy_missiles[EM] (MissileState)
```

### JoinGame (39 bytes)

```
Offset  Size  Field
0x00    32    token (SessionToken)
0x20    1     shipSkin (1-6)
0x21    6     roomCode
```

---

## Structures Voice

### VoiceJoin (38 bytes)

```
Offset  Size  Field
0x00    32    token (SessionToken)
0x20    6     roomCode
```

### VoiceFrame (5 + opus_len bytes)

```
Offset  Size  Field
0x00    1     speaker_id
0x01    2     sequence (network order)
0x03    2     opus_len (network order)
0x05    N     opus_data[N] (max 480)
```

### VoiceMute (2 bytes)

```
Offset  Size  Field
0x00    1     player_id
0x01    1     muted (0/1)
```

---

## Constantes

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `MAX_PLAYERS` | 4 | Joueurs max en jeu |
| `MAX_ROOM_PLAYERS` | 6 | Joueurs max en room |
| `MAX_MISSILES` | 32 | Missiles joueurs |
| `MAX_ENEMIES` | 16 | Ennemis max |
| `MAX_ENEMY_MISSILES` | 32 | Missiles ennemis |
| `TOKEN_SIZE` | 32 | Taille token (bytes) |
| `ROOM_CODE_LEN` | 6 | Taille code room |
| `MAX_USERNAME_LEN` | 32 | Taille username |
| `MAX_EMAIL_LEN` | 255 | Taille email |
| `MAX_OPUS_FRAME_SIZE` | 480 | Taille max Opus |

---

## Sérialisation

Pattern standard utilisé dans le code :

```cpp
void to_bytes(uint8_t* buf) const {
    uint16_t net_x = swap16(x);  // Host → Network
    std::memcpy(buf, &net_x, 2);
}

static std::optional<T> from_bytes(const void* buf, size_t len) {
    if (len < WIRE_SIZE) return std::nullopt;
    uint16_t net_x;
    std::memcpy(&net_x, buf, 2);
    return T{.x = swap16(net_x)};  // Network → Host
}
```
