---
tags:
  - api
  - reseau
---

# API Réseau

Classes du protocole réseau R-Type.

## Architecture

```mermaid
classDiagram
    class Protocol {
        <<enumeration>>
        +TCP_AUTH_PORT = 4125
        +UDP_GAME_PORT = 4124
        +UDP_VOICE_PORT = 4126
        +TCP_ADMIN_PORT = 4127
    }

    class PacketType {
        <<enumeration>>
        +Login
        +JoinRoom
        +GameSnapshot
        +Input
        +Chat
        ...
    }

    class Packet {
        +uint32_t magic
        +PacketType type
        +uint16_t size
        +byte[] payload
        +serialize() vector~byte~
        +deserialize(data)
    }

    class Serializer {
        +write(T value)
        +read~T~() T
        +data() vector~byte~
    }

    Packet --> PacketType
    Packet --> Serializer
```

---

## Protocole

| Port | Protocole | Usage |
|------|-----------|-------|
| 4125 | TCP + TLS | Authentification, chat, rooms |
| 4124 | UDP | Gameplay temps réel |
| 4126 | UDP | Voice chat |
| 4127 | TCP | Administration (localhost only) |

---

## Format de Paquet

### TCP Header (7 bytes)

```
┌───────────────────┬──────────┬─────────────┐
│ isAuthenticated   │ Type     │ PayloadSize │
│ 1 byte            │ 2 bytes  │ 4 bytes     │
└───────────────────┴──────────┴─────────────┘
```

### UDP Header (12 bytes)

```
┌──────────┬──────────┬────────────┐
│ Type     │ Sequence │ Timestamp  │
│ 2 bytes  │ 2 bytes  │ 8 bytes    │
└──────────┴──────────┴────────────┘
```

**Note:** Toutes les valeurs multi-octets sont en network byte order (big-endian).

---

## Flux de Données

```mermaid
sequenceDiagram
    participant App as Application
    participant Pkt as Packet
    participant Ser as Serializer
    participant Net as Network

    App->>Pkt: create(type, data)
    Pkt->>Ser: serialize(data)
    Ser-->>Pkt: bytes
    Pkt-->>Net: send(bytes)

    Net-->>Pkt: receive(bytes)
    Pkt->>Ser: deserialize(bytes)
    Ser-->>Pkt: data
    Pkt-->>App: packet
```

---

## Classes

<div class="grid-cards">
  <div class="card">
    <h3><a href="protocol/">Protocol</a></h3>
    <p>Constantes et types de paquets</p>
  </div>
  <div class="card">
    <h3><a href="packet/">Packet</a></h3>
    <p>Structure des paquets</p>
  </div>
  <div class="card">
    <h3><a href="serialization/">Serialization</a></h3>
    <p>Sérialisation binaire</p>
  </div>
</div>
