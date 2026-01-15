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
        +TCP_PORT = 4242
        +UDP_PORT = 4243
        +VOICE_PORT = 4244
        +MAGIC_NUMBER = 0x52545950
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
| 4242 | TCP | Authentification, chat, rooms |
| 4243 | UDP | Gameplay temps réel |
| 4244 | UDP | Voice chat |

---

## Format de Paquet

```
┌────────────────────────────────────────────┐
│ Magic (4 bytes) │ Type (1) │ Size (2) │ ... │
├────────────────────────────────────────────┤
│              Payload (N bytes)             │
└────────────────────────────────────────────┘
```

**Magic Number:** `0x52545950` ("RTYP" en ASCII)

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
