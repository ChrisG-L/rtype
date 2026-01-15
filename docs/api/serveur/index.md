---
tags:
  - api
  - serveur
---

# API Serveur

Classes principales du serveur R-Type.

## Architecture

```mermaid
classDiagram
    class Application {
        -RoomManager roomManager_
        -TcpServer tcpServer_
        -UdpServer udpServer_
        +run()
        +shutdown()
    }

    class RoomManager {
        -map~RoomId, Room~ rooms_
        +createRoom(name) RoomId
        +joinRoom(roomId, client)
        +leaveRoom(roomId, clientId)
        +getRoom(roomId) Room*
    }

    class Room {
        -RoomId id_
        -string name_
        -GameWorld world_
        -vector~ClientSession*~ clients_
        +tick()
        +broadcast(packet)
    }

    class ClientSession {
        -PlayerId id_
        -TcpConnection tcp_
        -UdpEndpoint udp_
        +send(packet)
        +onPacket(packet)
    }

    class GameWorld {
        -vector~Player~ players_
        -vector~Enemy~ enemies_
        -vector~Missile~ missiles_
        +tick()
        +processInput(playerId, input)
        +getSnapshot() GameSnapshot
    }

    Application --> RoomManager
    RoomManager --> Room
    Room --> ClientSession
    Room --> GameWorld
```

---

## Flux Principal

```mermaid
sequenceDiagram
    participant Main
    participant App as Application
    participant RM as RoomManager
    participant Room
    participant GW as GameWorld

    Main->>App: run()
    App->>App: initServers()

    loop Game Loop
        App->>RM: tick()
        RM->>Room: tick() (pour chaque room)
        Room->>GW: tick()
        GW->>GW: processInputs()
        GW->>GW: updatePhysics()
        GW->>GW: checkCollisions()
        GW-->>Room: snapshot
        Room->>Room: broadcast(snapshot)
    end
```

---

## Classes

<div class="grid-cards">
  <div class="card">
    <h3><a href="application/">Application</a></h3>
    <p>Point d'entrée serveur</p>
  </div>
  <div class="card">
    <h3><a href="room-manager/">RoomManager</a></h3>
    <p>Gestion des salons</p>
  </div>
  <div class="card">
    <h3><a href="client-session/">ClientSession</a></h3>
    <p>Session client</p>
  </div>
  <div class="card">
    <h3><a href="game-world/">GameWorld</a></h3>
    <p>Monde de jeu</p>
  </div>
</div>
