---
tags:
  - technique
  - architecture
---

# Architecture Système

Vision détaillée de l'architecture R-Type.

## Architecture Hexagonale

R-Type suit les principes de l'**architecture hexagonale** (Ports & Adapters) pour une séparation claire des responsabilités.

```mermaid
graph TB
    subgraph "Ports (Interfaces)"
        P1[IGraphicsPort]
        P2[INetworkPort]
        P3[IAudioPort]
        P4[IInputPort]
    end

    subgraph "Domain (Core)"
        D1[Game Engine]
        D2[ECS Registry]
        D3[Physics Engine]
        D4[Game Rules]
    end

    subgraph "Adapters (Implementations)"
        A1[SDL2 Adapter]
        A2[SFML Adapter]
        A3[Boost.ASIO Adapter]
        A4[OpenAL Adapter]
    end

    P1 --> D1
    P2 --> D1
    P3 --> D1
    P4 --> D1

    A1 -.-> P1
    A2 -.-> P1
    A3 -.-> P2
    A4 -.-> P3

    style D1 fill:#7c3aed,color:#fff
    style D2 fill:#7c3aed,color:#fff
```

### Avantages

- **Testabilité** : Le domaine peut être testé sans infrastructure
- **Flexibilité** : Changement de backend sans toucher au cœur
- **Maintenabilité** : Responsabilités clairement définies

---

## Client Architecture

```mermaid
flowchart TB
    subgraph Client
        subgraph "Presentation Layer"
            UI[UI Manager]
            Renderer[Render System]
        end

        subgraph "Application Layer"
            GL[Game Loop]
            Input[Input Handler]
            State[State Machine]
        end

        subgraph "Domain Layer"
            ECS[ECS Registry]
            Physics[Physics]
            Collision[Collision]
        end

        subgraph "Infrastructure Layer"
            GFX[Graphics Backend]
            NET[Network Client]
            Audio[Audio System]
        end
    end

    GL --> State
    State --> ECS
    Input --> ECS
    ECS --> Physics
    ECS --> Collision
    ECS --> Renderer
    Renderer --> GFX
    GL --> NET
    GL --> Audio

    style ECS fill:#7c3aed,color:#fff
    style GFX fill:#f59e0b,color:#000
```

### Game Loop

```cpp
void GameLoop::run() {
    while (running_) {
        auto dt = clock_.restart();

        // 1. Input
        input_handler_.poll();

        // 2. Network
        network_client_.receive();

        // 3. Update
        ecs_.update(dt);

        // 4. Render
        renderer_.render(ecs_);

        // 5. Present
        graphics_backend_.present();
    }
}
```

---

## Server Architecture

```mermaid
flowchart TB
    subgraph Server
        subgraph "Network Layer"
            UDP[UDP Server]
            Sessions[Session Manager]
            Protocol[Protocol Handler]
        end

        subgraph "Game Layer"
            Tick[Tick System]
            World[World State]
            Sync[State Synchronizer]
        end

        subgraph "Domain Layer"
            ECS2[ECS Registry]
            Physics2[Physics]
            GameRules[Game Rules]
        end
    end

    UDP --> Sessions
    Sessions --> Protocol
    Protocol --> Tick
    Tick --> ECS2
    ECS2 --> Physics2
    ECS2 --> GameRules
    ECS2 --> Sync
    Sync --> World
    World --> UDP

    style ECS2 fill:#7c3aed,color:#fff
    style UDP fill:#10b981,color:#fff
```

### Tick System

Le serveur fonctionne en tick fixe (60 Hz par défaut) :

```cpp
void Server::run() {
    constexpr auto tick_duration = 16.67ms;  // 60 Hz

    while (running_) {
        auto start = Clock::now();

        // 1. Process incoming packets
        network_.process_incoming();

        // 2. Update game state
        game_world_.tick();

        // 3. Broadcast state to clients
        synchronizer_.broadcast();

        // 4. Sleep until next tick
        auto elapsed = Clock::now() - start;
        if (elapsed < tick_duration) {
            std::this_thread::sleep_for(tick_duration - elapsed);
        }
    }
}
```

---

## Communication Client-Server

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server

    Note over C,S: Connection Phase
    C->>S: CONNECT (player_name)
    S->>C: ACCEPT (player_id, initial_state)

    Note over C,S: Game Loop
    loop Every Frame
        C->>S: INPUT (position, actions)
    end

    loop Every Tick (16ms)
        S->>C: STATE (entities, positions)
    end

    Note over C,S: Disconnection
    C->>S: DISCONNECT
    S->>C: ACK
```

### Protocole UDP

| Type | Direction | Description |
|------|-----------|-------------|
| `CONNECT` | C→S | Demande de connexion |
| `ACCEPT` | S→C | Connexion acceptée |
| `REJECT` | S→C | Connexion refusée |
| `INPUT` | C→S | Actions du joueur |
| `STATE` | S→C | État du monde |
| `EVENT` | S→C | Événements (mort, spawn) |
| `DISCONNECT` | C↔S | Déconnexion propre |

---

## Entity Component System

### Composants Core

```cpp
// Identité
struct EntityInfo {
    EntityType type;
    uint32_t owner_id;
};

// Transform
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Rotation { float angle; };

// Rendering
struct Sprite {
    TextureId texture;
    IntRect source_rect;
    int z_order;
};

// Gameplay
struct Health { int current, max; };
struct Weapon { WeaponType type; float cooldown; };
struct Hitbox { float width, height; };
```

### Systèmes

```mermaid
flowchart LR
    Input[Input System] --> Movement[Movement System]
    Movement --> Physics[Physics System]
    Physics --> Collision[Collision System]
    Collision --> Combat[Combat System]
    Combat --> Render[Render System]

    style Input fill:#7c3aed,color:#fff
    style Render fill:#f59e0b,color:#000
```

| Système | Responsabilité |
|---------|----------------|
| `InputSystem` | Convertit les inputs en intentions |
| `MovementSystem` | Applique la vélocité aux positions |
| `PhysicsSystem` | Gère la gravité, friction |
| `CollisionSystem` | Détecte et résout les collisions |
| `CombatSystem` | Gère les dégâts et la mort |
| `RenderSystem` | Prépare les entités pour le rendu |

---

## State Machine

Les états du jeu sont gérés par une machine à états :

```mermaid
stateDiagram-v2
    [*] --> Menu
    Menu --> Connecting: Play
    Connecting --> Lobby: Success
    Connecting --> Menu: Failure
    Lobby --> Playing: Start
    Playing --> Paused: Escape
    Paused --> Playing: Resume
    Paused --> Menu: Quit
    Playing --> GameOver: Defeat
    Playing --> Victory: Win
    GameOver --> Menu: Continue
    Victory --> Menu: Continue
```

```cpp
class StateMachine {
    std::stack<std::unique_ptr<IState>> states_;

public:
    void push(std::unique_ptr<IState> state);
    void pop();
    void update(float dt);
    void render();
};
```
