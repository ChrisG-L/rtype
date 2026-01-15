---
tags:
  - technique
  - api
---

# API Reference

Documentation des interfaces publiques de R-Type.

## Modules

<div class="grid-cards">
  <div class="card">
    <div class="card-icon">🎨</div>
    <h3>Graphics</h3>
    <p><code>rtype::graphics</code></p>
    <p>Système de rendu multi-backend.</p>
  </div>

  <div class="card">
    <div class="card-icon">🌐</div>
    <h3>Network</h3>
    <p><code>rtype::network</code></p>
    <p>Communication UDP client-serveur.</p>
  </div>

  <div class="card">
    <div class="card-icon">🎮</div>
    <h3>ECS</h3>
    <p><code>rtype::ecs</code></p>
    <p>Entity Component System.</p>
  </div>

  <div class="card">
    <div class="card-icon">⚙️</div>
    <h3>Core</h3>
    <p><code>rtype::core</code></p>
    <p>Utilitaires et types de base.</p>
  </div>
</div>

---

## Namespace rtype::graphics

### IGraphicsBackend

Interface principale du système graphique.

```cpp
namespace rtype::graphics {

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;

    // Lifecycle
    virtual bool initialize(const WindowConfig& config) = 0;
    virtual void shutdown() = 0;

    // Window
    virtual IWindow& getWindow() = 0;

    // Resources
    virtual std::unique_ptr<ITexture> loadTexture(
        const std::filesystem::path& path) = 0;
    virtual std::unique_ptr<IFont> loadFont(
        const std::filesystem::path& path, int size) = 0;

    // Rendering
    virtual void clear(Color color = Color::Black) = 0;
    virtual void draw(const IDrawable& drawable) = 0;
    virtual void present() = 0;

    // Info
    virtual std::string getName() const = 0;
};

} // namespace rtype::graphics
```

### Types Graphiques

```cpp
// Vector types
using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned>;

// Rectangle
struct IntRect {
    int x, y, width, height;
};

// Color
struct Color {
    uint8_t r, g, b, a;

    static const Color Black;
    static const Color White;
    static const Color Red;
    // ...
};

// Window configuration
struct WindowConfig {
    std::string title;
    uint32_t width;
    uint32_t height;
    bool fullscreen;
    bool vsync;
};
```

---

## Namespace rtype::network

### Packet

Classe de sérialisation des données réseau.

```cpp
namespace rtype::network {

class Packet {
public:
    explicit Packet(PacketType type);

    // Writing
    template<typename T> void write(const T& value);
    void write(const std::string& str);

    // Reading
    template<typename T> T read();
    std::string readString();

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Packet deserialize(const std::vector<uint8_t>& data);

    // Properties
    PacketType getType() const;
    uint32_t getSequence() const;
};

} // namespace rtype::network
```

### PacketType

```cpp
enum class PacketType : uint8_t {
    CONNECT     = 0x01,
    ACCEPT      = 0x02,
    REJECT      = 0x03,
    DISCONNECT  = 0x04,
    INPUT       = 0x10,
    STATE       = 0x20,
    DELTA       = 0x21,
    EVENT       = 0x30,
    PING        = 0x40,
    PONG        = 0x41
};
```

---

## Namespace rtype::ecs

### Registry

Gestionnaire principal des entités et composants.

```cpp
namespace rtype::ecs {

class Registry {
public:
    // Entity management
    Entity create();
    void destroy(Entity entity);
    bool valid(Entity entity) const;

    // Component management
    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args);

    template<typename T>
    T& get(Entity entity);

    template<typename T>
    const T& get(Entity entity) const;

    template<typename T>
    T* tryGet(Entity entity);

    template<typename T>
    bool has(Entity entity) const;

    template<typename T>
    void remove(Entity entity);

    // Views
    template<typename... Components>
    auto view();

    template<typename... Components>
    auto view() const;

    // Systems
    template<typename System, typename... Args>
    System& addSystem(Args&&... args);

    void update(float dt);
};

} // namespace rtype::ecs
```

### Components Core

```cpp
// Transform
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Rotation { float angle; };

// Rendering
struct Sprite {
    std::string texturePath;
    IntRect sourceRect;
    int zOrder = 0;
};

// Gameplay
struct Health {
    int current;
    int max;
};

struct Weapon {
    WeaponType type;
    float cooldown;
    float currentCooldown = 0;
};

struct Hitbox {
    float width;
    float height;
    float offsetX = 0;
    float offsetY = 0;
};

// Network
struct NetworkId {
    uint32_t id;
    bool isLocal;
};
```

---

## Namespace rtype::core

### Logger

Wrapper spdlog pour le logging.

```cpp
namespace rtype::core {

class Logger {
public:
    static void init(const std::string& name, LogLevel level);

    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args&&... args);

    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args);

    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args);

    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args);

    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args);
};

} // namespace rtype::core
```

### Config

Gestionnaire de configuration JSON.

```cpp
namespace rtype::core {

class Config {
public:
    static Config& instance();

    bool load(const std::filesystem::path& path);

    template<typename T>
    T get(const std::string& key, T defaultValue = T{}) const;

    template<typename T>
    void set(const std::string& key, const T& value);

    bool save(const std::filesystem::path& path) const;
};

} // namespace rtype::core
```

---

## Exemples d'Utilisation

### Initialisation du Client

```cpp
#include <rtype/graphics/BackendRegistry.hpp>
#include <rtype/network/NetworkClient.hpp>
#include <rtype/ecs/Registry.hpp>

int main() {
    using namespace rtype;

    // Initialize graphics
    auto backend = graphics::BackendRegistry::instance()
        .create("sdl2");

    backend->initialize({
        .title = "R-Type",
        .width = 1920,
        .height = 1080
    });

    // Initialize network
    network::NetworkClient client;
    client.connect("127.0.0.1", 4242);

    // Initialize ECS
    ecs::Registry registry;
    registry.addSystem<MovementSystem>();
    registry.addSystem<RenderSystem>(*backend);

    // Game loop
    while (backend->getWindow().isOpen()) {
        // ...
    }

    return 0;
}
```

### Création d'une Entité

```cpp
// Create player entity
auto player = registry.create();
registry.emplace<Position>(player, 100.f, 300.f);
registry.emplace<Velocity>(player, 0.f, 0.f);
registry.emplace<Sprite>(player, "assets/player.png", IntRect{0, 0, 64, 64});
registry.emplace<Health>(player, 100, 100);
registry.emplace<Hitbox>(player, 64.f, 64.f);
```
