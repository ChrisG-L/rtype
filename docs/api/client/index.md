---
tags:
  - api
  - client
---

# API Client

Classes principales du client R-Type.

## Architecture

```mermaid
classDiagram
    class Engine {
        -SceneManager sceneManager_
        -NetworkClient network_
        -AudioManager audio_
        -Renderer renderer_
        +run()
        +quit()
    }

    class SceneManager {
        -stack~Scene~ scenes_
        +push(scene)
        +pop()
        +replace(scene)
        +current() Scene*
    }

    class NetworkClient {
        -TcpClient tcp_
        -UdpClient udp_
        +connect(host, port)
        +send(packet)
        +poll() vector~Packet~
    }

    class AudioManager {
        -map~string, Sound~ sounds_
        -VoiceChat voice_
        +playSound(name)
        +playMusic(name)
        +setVolume(type, value)
    }

    class Renderer {
        <<interface>>
        +clear()
        +draw(sprite)
        +present()
    }

    Engine --> SceneManager
    Engine --> NetworkClient
    Engine --> AudioManager
    Engine --> Renderer
```

---

## Flux Principal

```mermaid
sequenceDiagram
    participant Main
    participant Engine
    participant SM as SceneManager
    participant Scene
    participant Renderer

    Main->>Engine: run()
    Engine->>SM: push(MenuScene)

    loop Game Loop
        Engine->>Engine: pollEvents()
        Engine->>SM: current()
        SM-->>Engine: scene
        Engine->>Scene: update(dt)
        Engine->>Scene: render(renderer)
        Engine->>Renderer: present()
    end

    Note over Main: Quit requested
    Engine-->>Main: return 0
```

---

## Classes

<div class="grid-cards">
  <div class="card">
    <h3><a href="engine/">Engine</a></h3>
    <p>Moteur de jeu principal</p>
  </div>
  <div class="card">
    <h3><a href="scene-manager/">SceneManager</a></h3>
    <p>Gestionnaire de scènes</p>
  </div>
  <div class="card">
    <h3><a href="network-client/">NetworkClient</a></h3>
    <p>Client réseau TCP/UDP</p>
  </div>
  <div class="card">
    <h3><a href="audio-manager/">AudioManager</a></h3>
    <p>Gestionnaire audio</p>
  </div>
</div>

---

## Scènes Disponibles

```mermaid
stateDiagram-v2
    [*] --> SplashScene

    SplashScene --> MenuScene: timeout

    MenuScene --> SettingsScene: settings
    MenuScene --> LobbyScene: play
    MenuScene --> [*]: quit

    SettingsScene --> MenuScene: back

    LobbyScene --> RoomScene: join room
    LobbyScene --> MenuScene: back

    RoomScene --> GameScene: start
    RoomScene --> LobbyScene: leave

    GameScene --> ScoreScene: game over
    GameScene --> RoomScene: quit

    ScoreScene --> RoomScene: continue
    ScoreScene --> MenuScene: quit
```

---

## Plugins

Le client supporte des plugins pour étendre ses fonctionnalités.

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string name() const = 0;
    virtual void init(Engine& engine) = 0;
    virtual void shutdown() = 0;
};

// Exemple: Plugin controller
class ControllerPlugin : public IPlugin {
public:
    std::string name() const override { return "controller"; }
    void init(Engine& engine) override {
        // Init SDL GameController
    }
    void shutdown() override {
        // Cleanup
    }
};
```
