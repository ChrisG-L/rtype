---
tags:
  - technique
  - architecture
  - reseau
---

# Vue d'Ensemble des Protocoles

R-Type utilise 3 protocoles de communication.

## Résumé

| Protocole | Port | Usage |
|-----------|------|-------|
| **TCP** | 4125 | Auth (TLS), rooms, chat |
| **UDP** | 4124 | Game sync |
| **Voice** | 4126 | Audio temps réel |

```mermaid
flowchart LR
    subgraph Client
        C1[Auth]
        C2[Game]
        C3[Voice]
    end

    subgraph Server
        S1[TCP Server]
        S2[UDP Server]
        S3[Voice Server]
    end

    C1 <-->|TCP 4125| S1
    C2 <-->|UDP 4124| S2
    C3 <-->|UDP 4126| S3

    style S1 fill:#3b82f6,color:#fff
    style S2 fill:#10b981,color:#fff
    style S3 fill:#f59e0b,color:#000
```

---

## TCP - Fiable

Pour les opérations **critiques** qui nécessitent garantie de livraison.

| Type | Description |
|------|-------------|
| `AUTH_REQUEST` | Connexion |
| `AUTH_RESPONSE` | Résultat auth |
| `CREATE_ROOM` | Créer une room |
| `JOIN_ROOM` | Rejoindre |
| `LEAVE_ROOM` | Quitter |
| `CHAT_MESSAGE` | Message texte |
| `ROOM_LIST` | Liste des rooms |

---

## UDP - Rapide

Pour le **game state** où la latence est critique.

| Type | Description |
|------|-------------|
| `INPUT` | Actions joueur |
| `STATE` | État du monde |
| `SPAWN` | Nouvelle entité |
| `DESTROY` | Entité détruite |
| `HIT` | Collision/dégâts |

---

## Voice - Temps Réel

Pour l'**audio** avec codec Opus.

| Type | Description |
|------|-------------|
| `VOICE_DATA` | Données audio |
| `VOICE_MUTE` | Joueur mute |
| `VOICE_UNMUTE` | Joueur unmute |

---

## Format des Headers

### Header TCP (7 bytes)

```
┌───────────────┬──────────┬─────────────┐
│ isAuthenticated│ Type     │ PayloadSize │
│ 1 byte        │ 2 bytes  │ 4 bytes     │
└───────────────┴──────────┴─────────────┘
```

```cpp
struct Header {
    uint8_t isAuthenticated;  // 0 ou 1
    uint16_t type;            // Network byte order
    uint32_t payload_size;    // Network byte order
};
```

### Header UDP (12 bytes)

```
┌──────────┬─────────┬────────────┬─────────┐
│ Type     │ Seq     │ Timestamp  │ Data    │
│ 2 bytes  │ 2 bytes │ 8 bytes    │ ...     │
└──────────┴─────────┴────────────┴─────────┘
```

```cpp
struct UDPHeader {
    uint16_t type;          // MessageType (network order)
    uint16_t sequence_num;  // Numéro de séquence
    uint64_t timestamp;     // Millisecondes depuis epoch
};
```

---

## Diagramme de Séquence

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server

    Note over C,S: Phase Auth (TCP)
    C->>S: AUTH_REQUEST
    S->>C: AUTH_RESPONSE (OK)

    Note over C,S: Phase Lobby (TCP)
    C->>S: JOIN_ROOM
    S->>C: ROOM_INFO

    Note over C,S: Phase Game (UDP 4124)
    loop 20 Hz (Snapshot)
        C->>S: PlayerInput
        S->>C: Snapshot
    end

    Note over C,S: Voice (UDP 4126)
    loop Continu
        C->>S: VoiceFrame
        S->>C: VoiceFrame (autres joueurs)
    end
```

---

## Détails

- [TCP Protocol](../reseau/tcp.md) - Authentification et rooms
- [UDP Protocol](../reseau/udp.md) - Synchronisation jeu
- [Voice Protocol](../reseau/voice.md) - Audio temps réel
- [Structures](../reseau/structures.md) - Référence binaire
