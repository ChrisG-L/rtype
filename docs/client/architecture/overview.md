# Architecture Globale du Client

## 📐 Vue d'Ensemble

L'architecture du client R-Type suit les principes SOLID avec une séparation claire entre les interfaces et les implémentations. Cette approche permet une grande flexibilité, facilitant les tests et les changements de technologies.

## 🏛️ Architecture en Couches

```mermaid
graph TD
    subgraph "Couche Application"
        A[main.cpp] --> B[Boot]
    end

    subgraph "Couche Orchestration"
        B --> C[Engine]
        B --> D[TCPClient]
    end

    subgraph "Couche Métier - Interfaces"
        C --> E[IEngine]
        C --> F[IGameLoop]
        C --> G[IRenderer]
        C --> H[IWindow]
    end

    subgraph "Couche Implémentation"
        E -.->|implements| I[Engine]
        F -.->|implements| J[GameLoop]
        G -.->|implements| K[SFMLRenderer]
        H -.->|implements| L[SFMLWindow]
    end

    subgraph "Couche Utilitaires"
        K --> M[AssetManager]
        M --> N[Textures Cache]
        M --> O[Sprite Pools]
    end

    subgraph "Couche Infrastructure"
        L --> P[SFML Library]
        D --> Q[Boost.Asio]
    end

    style E fill:#ffe1e1
    style F fill:#ffe1e1
    style G fill:#ffe1e1
    style H fill:#ffe1e1
    style I fill:#e1f5ff
    style J fill:#e1f5ff
    style K fill:#e1f5ff
    style L fill:#e1f5ff
```

## 🎯 Principes Architecturaux

### 1. Dependency Inversion Principle (DIP)

Les modules de haut niveau ne dépendent pas des modules de bas niveau. Les deux dépendent d'abstractions.

**Exemple concret** :

```cpp
// ❌ MAUVAIS : Engine dépend directement de SFML
class Engine {
    sf::RenderWindow _window;  // Couplage fort !
    sf::Texture _texture;      // Difficile à tester !
};

// ✅ BON : Engine dépend d'interfaces
class Engine {
    std::unique_ptr<IWindow> _window;      // Abstraction
    std::unique_ptr<IRenderer> _renderer;  // Flexible
};
```

**Avantages** :

- ✅ Tests unitaires faciles (mocks)
- ✅ Changement de bibliothèque graphique transparent
- ✅ Plusieurs implémentations possibles (SFML, SDL, OpenGL)

### 2. Interface Segregation Principle (ISP)

Les clients ne doivent pas dépendre d'interfaces qu'ils n'utilisent pas.

**Notre approche** :

```cpp
// Interfaces spécialisées plutôt qu'une grosse interface
class IWindow {
    virtual void draw(const sf::Sprite&) = 0;
    virtual void clear() = 0;
    virtual void display() = 0;
};

class IRenderer {
    virtual void initialize() = 0;
    virtual void update() = 0;
};

class IEngine {
    virtual void initialize() = 0;
    virtual void run() = 0;
};
```

### 3. Single Responsibility Principle (SRP)

Chaque classe a une seule raison de changer.

| Classe | Responsabilité Unique |
|--------|----------------------|
| `Boot` | Orchestration et initialisation |
| `Engine` | Gestion du cycle de vie du moteur |
| `GameLoop` | Boucle de rendu (clear/update/display) |
| `SFMLRenderer` | Rendu graphique avec SFML |
| `AssetManager` | Cache et gestion des ressources |
| `TCPClient` | Communication réseau |

### 4. RAII et Ownership Clair

Gestion automatique de la mémoire avec smart pointers :

```cpp
class Engine {
    // Engine OWNS ces ressources
    std::unique_ptr<SFMLWindow> _window;
    std::unique_ptr<SFMLRenderer> _renderer;
    std::unique_ptr<GameLoop> _gameLoop;

    // Destructeur automatique, pas de fuites !
    ~Engine() = default;
};
```

## 🔄 Flux de Données

```mermaid
sequenceDiagram
    autonumber
    participant M as main()
    participant B as Boot
    participant E as Engine
    participant GL as GameLoop
    participant R as SFMLRenderer
    participant AM as AssetManager
    participant W as SFMLWindow

    rect rgb(240, 248, 255)
        Note over M,B: Phase 1: Initialisation
        M->>B: Boot boot;
        M->>B: boot.core();
        B->>E: new Engine()
        B->>E: engine->initialize()
    end

    rect rgb(255, 250, 240)
        Note over E,W: Phase 2: Setup Components
        E->>W: new SFMLWindow(1200, 1200, "rtype")
        W-->>E: window ready
        E->>R: new SFMLRenderer(window)
        R->>AM: new AssetManager()
        R->>AM: registerTexture("bedroom.jpg")
        AM-->>R: texture loaded
        E->>GL: new GameLoop(window, renderer)
    end

    rect rgb(240, 255, 240)
        Note over E,W: Phase 3: Game Loop
        B->>E: engine->run()
        E->>GL: gameLoop->run()

        loop Tant que window.isOpen()
            GL->>W: clear()
            GL->>R: update()
            R->>AM: drawAll(window)
            AM->>W: draw(sprite1)
            AM->>W: draw(sprite2)
            GL->>W: display()
        end
    end
```

## 🏗️ Composants Principaux

### Boot - Orchestrateur

**Rôle** : Point d'entrée et orchestration des systèmes.

```cpp
class Boot {
public:
    Boot();
    void core();  // Lance le jeu

private:
    boost::asio::io_context io_ctx;
    std::unique_ptr<TCPClient> tcpClient;  // Réseau (futur)
    std::unique_ptr<core::Engine> engine;  // Moteur graphique
};
```

**Responsabilités** :

- ✅ Initialiser `io_context` pour Boost.Asio
- ✅ Créer le `TCPClient` (actuellement désactivé)
- ✅ Créer et lancer l'`Engine`
- ⏳ Future : Coordonner thread réseau + thread graphique

### Engine - Cœur du Moteur

**Rôle** : Gestionnaire principal du cycle de vie du jeu.

```cpp
class Engine : public IEngine {
public:
    void initialize() override;
    void run() override;

private:
    std::unique_ptr<SFMLWindow> _window;
    std::unique_ptr<SFMLRenderer> _renderer;
    std::unique_ptr<GameLoop> _gameLoop;
};
```

**Cycle de vie** :

```mermaid
stateDiagram-v2
    [*] --> Créé: new Engine()
    Créé --> Initialisé: initialize()
    Initialisé --> EnMarche: run()
    EnMarche --> EnMarche: GameLoop tourne
    EnMarche --> Terminé: window.close()
    Terminé --> [*]
```

**Responsabilités** :

1. **initialize()** :
   - Créer la fenêtre SFML (1200x1200)
   - Créer le renderer avec la fenêtre
   - Créer la GameLoop avec window et renderer

2. **run()** :
   - Déléguer à `gameLoop->run()`
   - Bloquer jusqu'à fermeture fenêtre

### GameLoop - Boucle de Rendu

**Rôle** : Orchestrer le cycle clear → update → display.

```cpp
class GameLoop : public IGameLoop {
public:
    GameLoop(IWindow* window, IRenderer* renderer);

    void run() override;
    void clear() override;
    void display() override;

private:
    IWindow* _window;      // Pas d'ownership
    IRenderer* _renderer;  // Pas d'ownership
};
```

**Pattern** :

```cpp
void GameLoop::run() {
    while (_window->isOpen()) {
        clear();           // Nettoie la frame
        _renderer->update();  // Rendu des objets
        display();         // Affiche à l'écran
    }
}
```

!!! warning "Ownership"
    `GameLoop` utilise des **pointeurs nus** car elle ne possède pas les objets. L'ownership appartient à `Engine`.

### SFMLRenderer - Moteur de Rendu

**Rôle** : Gérer le rendu graphique et les assets.

```cpp
class SFMLRenderer : public IRenderer {
public:
    SFMLRenderer(IWindow* window);

    void initialize() override;
    void update() override;

private:
    IWindow* _window;
    std::unique_ptr<AssetManager> mAsset;
};
```

**Workflow** :

```mermaid
graph LR
    A[initialize] -->|registerTexture| B[AssetManager]
    B -->|load texture| C[Cache]
    B -->|create sprite| D[Sprite Pool]

    E[update] -->|drawAll| B
    B -->|pour chaque sprite| F[window->draw]

    style A fill:#e1f5ff
    style E fill:#ffe8e1
```

### AssetManager - Gestionnaire de Ressources

**Rôle** : Cache intelligent de textures et sprites.

```cpp
class AssetManager {
public:
    void registerTexture(const std::string& file);
    sf::Texture& getTexture(const std::string& key);
    void addSprite(const std::string& key, const sf::Sprite& sprite);
    void drawAll(IWindow* window);

private:
    std::unordered_map<std::string, sf::Texture> textures;
    std::unordered_map<std::string, std::vector<sf::Sprite>> spritePools;
};
```

**Optimisations** :

- **Cache de textures** : Évite les rechargements
- **Sprite pools** : Regroupe sprites par texture
- **Détection doublons** : `if (textures.count(file)) return;`

## 🌐 Système Réseau (TCPClient)

**Rôle** : Communication asynchrone avec le serveur.

```cpp
class TCPClient {
public:
    TCPClient(boost::asio::io_context& io_ctx);
    void run();  // À implémenter

private:
    boost::asio::io_context& _io_ctx;
    tcp::socket _socket;
};
```

**État actuel** :

- ✅ Connexion asynchrone à `127.0.0.1:4125`
- ✅ Gestion erreurs de connexion
- ⏳ Lecture/écriture à implémenter
- ⏳ Intégration avec Engine (multi-thread)

**Future architecture** :

```mermaid
graph TB
    B[Boot] --> E[Engine Thread]
    B --> T[Network Thread]

    E --> GL[GameLoop]
    E --> R[Renderer]

    T --> TC[TCPClient]
    TC --> S[Serveur]

    TC -.->|État Jeu| E
    E -.->|Inputs| TC

    style E fill:#e1f5ff
    style T fill:#ffe1e1
```

## 📊 Diagramme de Classes Complet

```mermaid
classDiagram
    class IEngine {
        <<interface>>
        +initialize()
        +run()
    }

    class IGameLoop {
        <<interface>>
        +run()
        +clear()
        +display()
    }

    class IRenderer {
        <<interface>>
        +initialize()
        +update()
    }

    class IWindow {
        <<interface>>
        +initialize(Vec2u, string)
        +isOpen() bool
        +draw(Sprite)
        +clear()
        +display()
    }

    class Engine {
        -unique_ptr~SFMLWindow~ _window
        -unique_ptr~SFMLRenderer~ _renderer
        -unique_ptr~GameLoop~ _gameLoop
        +initialize()
        +run()
    }

    class GameLoop {
        -IWindow* _window
        -IRenderer* _renderer
        +run()
        +clear()
        +display()
    }

    class SFMLRenderer {
        -IWindow* _window
        -unique_ptr~AssetManager~ mAsset
        +initialize()
        +update()
    }

    class SFMLWindow {
        -sf::RenderWindow _window
        +initialize(Vec2u, string)
        +isOpen() bool
        +draw(Sprite)
        +clear()
        +display()
    }

    class AssetManager {
        -map~string, Texture~ textures
        -map~string, vector~Sprite~~ spritePools
        +registerTexture(file)
        +getTexture(key) Texture
        +addSprite(key, sprite)
        +drawAll(window)
    }

    class Boot {
        -io_context io_ctx
        -unique_ptr~TCPClient~ tcpClient
        -unique_ptr~Engine~ engine
        +core()
    }

    class TCPClient {
        -io_context& _io_ctx
        -tcp::socket _socket
        +run()
    }

    IEngine <|.. Engine : implements
    IGameLoop <|.. GameLoop : implements
    IRenderer <|.. SFMLRenderer : implements
    IWindow <|.. SFMLWindow : implements

    Boot --> Engine : owns
    Boot --> TCPClient : owns
    Engine --> SFMLWindow : owns
    Engine --> SFMLRenderer : owns
    Engine --> GameLoop : owns
    GameLoop --> IWindow : uses
    GameLoop --> IRenderer : uses
    SFMLRenderer --> AssetManager : owns
    SFMLRenderer --> IWindow : uses
```

## 🎨 Patterns de Conception Utilisés

### Adapter Pattern

Encapsuler SFML derrière nos interfaces.

```cpp
// Notre interface
class IWindow { /*...*/ };

// Adapter SFML
class SFMLWindow : public IWindow {
    sf::RenderWindow _window;  // Wrapped

    void draw(const sf::Sprite& s) override {
        _window.draw(s);  // Délégation
    }
};
```

### Dependency Injection

Injection via constructeurs :

```cpp
// GameLoop ne crée pas ses dépendances
GameLoop::GameLoop(IWindow* window, IRenderer* renderer)
    : _window(window), _renderer(renderer) {}

// C'est Engine qui injecte
_gameLoop = std::make_unique<GameLoop>(_window.get(), _renderer.get());
```

### Strategy Pattern

AssetManager peut changer de stratégie de cache :

```cpp
// Stratégie actuelle : std::unordered_map
std::unordered_map<std::string, sf::Texture> textures;

// Facile à remplacer par LRU cache, etc.
```

## 🚀 Points d'Extension Futurs

| Extension | Difficulté | Impact |
|-----------|------------|--------|
| **Intégration ECS** | Haute | Architecture complète |
| **Gestion Événements** | Moyenne | Inputs joueur |
| **Multi-threading réseau** | Haute | Performance |
| **Delta Time** | Basse | Gameplay fluide |
| **Audio System** | Moyenne | Ambiance sonore |
| **UI/HUD** | Moyenne | Interface utilisateur |

## 📚 Ressources

- [Patterns de Conception](design-patterns.md)
- [Système Graphique](../graphics/overview.md)
- [Moteur de Jeu](../core/engine.md)
- [Système Réseau](../network/tcp-client.md)

---

!!! success "Architecture Solide"
    Cette architecture modulaire et découplée permet d'ajouter facilement de nouvelles fonctionnalités sans impacter le code existant !
