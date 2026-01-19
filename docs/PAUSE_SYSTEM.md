# Système de Pause

## Vue d'ensemble

Le système de pause permet aux joueurs de mettre le jeu en pause en appuyant sur **Escape**.

### Comportement

| Mode | Condition de pause | Condition de reprise |
|------|-------------------|---------------------|
| **Solo** | Appui sur Escape | Appui sur Escape |
| **Multijoueur** | **Tous** les joueurs appuient sur Escape | Un joueur appuie sur Escape pour voter unpause |

En multijoueur, l'overlay affiche le nombre de joueurs ayant voté pour la pause (ex: "2/4 players paused").

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         CLIENT                               │
├─────────────────────────────────────────────────────────────┤
│  GameScene                                                   │
│  ├─ handleEvent() → Escape → togglePause()                  │
│  ├─ togglePause() → UDPClient::sendPauseRequest()           │
│  ├─ processUDPEvents() → onPauseStateReceived()             │
│  ├─ update() → skip si _isPaused                            │
│  └─ render() → renderPauseOverlay()                         │
│                                                              │
│  UDPClient                                                   │
│  ├─ sendPauseRequest(bool wantsPause)                       │
│  └─ handlePauseStateSync() → UDPPauseStateSyncEvent         │
└─────────────────────────────────────────────────────────────┘
                           │
                           │ UDP (PauseRequest 0x0430)
                           │ UDP (PauseStateSync 0x0431)
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                         SERVER                               │
├─────────────────────────────────────────────────────────────┤
│  UDPServer                                                   │
│  ├─ handle_receive() → PauseRequest handler                 │
│  ├─ broadcastPauseState()                                   │
│  └─ updateAndBroadcastRoom() → skip gameplay si isPaused()  │
│                                                              │
│  GameWorld                                                   │
│  ├─ _pauseVotes: set<uint8_t>                               │
│  ├─ setPauseVote(playerId, wantsPause)                      │
│  ├─ isPaused() → Solo: 1 vote / Multi: tous votent          │
│  └─ getPauseState() → (isPaused, voterCount, totalPlayers)  │
└─────────────────────────────────────────────────────────────┘
```

## Protocol

### Messages UDP

| Type | Valeur | Direction | Payload |
|------|--------|-----------|---------|
| `PauseRequest` | 0x0430 | C→S | 1 byte: wantsPause (0/1) |
| `PauseStateSync` | 0x0431 | S→C | 3 bytes: isPaused, voterCount, totalPlayers |

### Structures (Protocol.hpp)

```cpp
struct PauseRequest {
    uint8_t wantsPause;  // 1 = wants pause, 0 = wants resume
    static constexpr size_t WIRE_SIZE = 1;
};

struct PauseStateSync {
    uint8_t isPaused;           // 1 = paused, 0 = running
    uint8_t pauseVoterCount;    // Nombre de joueurs voulant la pause
    uint8_t totalPlayerCount;   // Nombre total de joueurs
    static constexpr size_t WIRE_SIZE = 3;
};
```

## Fichiers clés

| Fichier | Rôle |
|---------|------|
| `src/common/protocol/Protocol.hpp` | Structures PauseRequest, PauseStateSync |
| `src/client/include/scenes/GameScene.hpp` | État pause client |
| `src/client/src/scenes/GameScene.cpp` | togglePause(), renderPauseOverlay() |
| `src/client/include/network/UDPClient.hpp` | sendPauseRequest(), handlePauseStateSync() |
| `src/client/src/network/UDPClient.cpp` | Implémentation envoi/réception |
| `src/client/include/network/NetworkEvents.hpp` | UDPPauseStateSyncEvent |
| `src/server/include/infrastructure/game/GameWorld.hpp` | _pauseVotes, isPaused() |
| `src/server/infrastructure/game/GameWorld.cpp` | Logique de vote |
| `src/server/infrastructure/adapters/in/network/UDPServer.cpp` | Handler + broadcast |

## Comportements détaillés

### Ce qui est pausé (serveur)
- Mouvement des joueurs
- Mouvement des missiles
- Spawn d'ennemis et vagues
- Mouvement des ennemis
- Collisions
- Boss
- Power-ups
- Wave Cannons
- Force Pods & Bit Devices

### Ce qui continue (serveur)
- Heartbeat / timeout detection
- Broadcast des snapshots

### Ce qui est pausé (client)
- Gameplay update
- Inputs de mouvement

### Ce qui continue (client)
- Animation des étoiles (fond)
- Rendu de l'overlay de pause
- Chat

## Cleanup

Quand un joueur quitte (`removePlayer`), son vote de pause est automatiquement retiré de `_pauseVotes`.
