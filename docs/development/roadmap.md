# Roadmap R-Type - Guide d'Implémentation par Priorité

**Version:** 2.1
**Dernière mise à jour:** 25 novembre 2025
**Objectif:** Terminer le projet R-Type multijoueur

---

## 🎯 Vue d'Ensemble des Priorités

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ORDRE D'IMPLÉMENTATION                                │
│                                                                              │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌───────────┐ │
│  │  PRIORITÉ 0 │ ──▶ │  PRIORITÉ 1 │ ──▶ │  PRIORITÉ 2 │ ──▶ │PRIORITÉ 3 │ │
│  │   (Base)    │     │   (Core)    │     │  (Features) │     │ (Polish)  │ │
│  └─────────────┘     └─────────────┘     └─────────────┘     └───────────┘ │
│                                                                              │
│  • Libs Dynamiques   • ECS Complet       • Ennemis & IA     • Audio        │
│  • Delta Time        • Player            • Power-ups        • Particules   │
│  • InputManager      • Projectiles       • Waves            • Menus        │
│  • World basique     • Collisions        • Network UDP      • Polish       │
│                      • Rendu             • Multijoueur      • Niveaux      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### État Actuel du Projet

```
✅ COMPLÉTÉ                          🚧 EN COURS                    📋 À FAIRE
─────────────────────────────────────────────────────────────────────────────
• Architecture hexagonale serveur    • UI Components client         • Libs dyn.
• Client SFML (Boot, Engine, Loop)   • Intégration auth            • ECS
• Système de scènes                  • TextField                    • Gameplay
• TCPClient/TCPServer                                               • Multijoueur
• Authentification (Login/Register)                                 • Audio
• MongoDB (Users)                                                   • Polish
• Logging (12 loggers)
• AssetManager
• Tests (261 tests)
```

---

## 📋 Checklist Rapide - Par Ordre

Coche au fur et à mesure :

```
PRIORITÉ 0 - FONDATIONS (Faire en premier !)
├── □ Architecture Libs Dynamiques (.so/.dll)
│   ├── □ IGraphicsPlugin interface (init, createWindow, createRenderer)
│   ├── □ PluginLoader (dlopen/LoadLibrary)
│   ├── □ Plugin SFML (libsfml_plugin.so)
│   └── □ Retirer sf::Event de IScene
├── □ Delta Time dans GameLoop
├── □ InputManager (abstraction complète)
├── □ Entity + EntityManager basique
├── □ ComponentPool<T> template
└── □ World façade minimale

PRIORITÉ 1 - CORE GAMEPLAY (Un joueur jouable)
├── □ TransformComponent + VelocityComponent
├── □ SpriteComponent + RenderSystem
├── □ MovementSystem
├── □ ColliderComponent + CollisionSystem
├── □ PlayerComponent + PlayerInputSystem
├── □ ProjectileComponent + ProjectileFactory
├── □ HealthComponent + HealthSystem
└── □ Un joueur qui tire et peut mourir

PRIORITÉ 2 - GAMEPLAY COMPLET (Jeu solo jouable)
├── □ EnemyComponent + EnemyFactory
├── □ EnemyAISystem (patterns basiques)
├── □ WaveSystem (spawn ennemis)
├── □ PowerUpComponent + PowerUpSystem
├── □ AnimationSystem
├── □ Score et Game Over
└── □ 1 niveau complet jouable en solo

PRIORITÉ 3 - MULTIJOUEUR (Jeu multi jouable)
├── □ NetworkSyncComponent
├── □ UDPClient gameplay
├── □ Client-side prediction
├── □ Entity interpolation
├── □ Lobby TCP
└── □ 2-4 joueurs simultanés

PRIORITÉ 4 - POLISH (Jeu fini)
├── □ AudioManager + Sons
├── □ Système de particules
├── □ 3 niveaux + Boss
├── □ Menus complets
└── □ Polish et bugfix
```

---

# PRIORITÉ 0 : Fondations du Game Engine

**🎯 Objectif : Avoir les bases pour construire le reste**

> ⚠️ **NE PAS SAUTER CETTE ÉTAPE** - Tout le reste dépend de ces fondations !

---

## 0.1 Architecture Bibliothèques Dynamiques (Plugins)

### Pourquoi c'est critique

Actuellement le code a un **couplage fort avec SFML** :
- `sf::Event` directement dans `IScene::handleEvent()`
- Impossible de changer de bibliothèque graphique
- Impossible de tester sans SFML

**Solution : Les bibliothèques dynamiques (.so sur Linux, .dll sur Windows)**

Le core engine ne connaît que des **interfaces abstraites**. L'implémentation SFML est dans une lib dynamique séparée chargée au runtime.

### Architecture Cible

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ARCHITECTURE PLUGIN                                  │
│                                                                              │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                           CORE ENGINE                                  │  │
│  │                    (Ne connaît AUCUNE lib graphique)                   │  │
│  │                                                                        │  │
│  │   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                │  │
│  │   │  IWindow     │  │  IRenderer   │  │  IInput      │                │  │
│  │   │  (abstract)  │  │  (abstract)  │  │  (abstract)  │                │  │
│  │   └──────────────┘  └──────────────┘  └──────────────┘                │  │
│  │          ▲                 ▲                ▲                          │  │
│  │          │                 │                │                          │  │
│  │   ┌──────┴─────────────────┴────────────────┴──────┐                  │  │
│  │   │              IGraphicsPlugin                    │                  │  │
│  │   │   (Interface que chaque plugin implémente)      │                  │  │
│  │   └─────────────────────────────────────────────────┘                  │  │
│  │                            ▲                                           │  │
│  └────────────────────────────│───────────────────────────────────────────┘  │
│                               │                                              │
│                      ┌────────┴────────┐                                     │
│                      │  PluginLoader   │                                     │
│                      │  (dlopen/       │                                     │
│                      │   LoadLibrary)  │                                     │
│                      └────────┬────────┘                                     │
│                               │                                              │
│          ┌────────────────────┼────────────────────┐                         │
│          ▼                    ▼                    ▼                         │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │ SFML Plugin  │    │  SDL Plugin  │    │ Raylib Plugin│                   │
│  │ (.so/.dll)   │    │  (.so/.dll)  │    │  (.so/.dll)  │                   │
│  │              │    │              │    │              │                   │
│  │ SFMLWindow   │    │ SDLWindow    │    │ RaylibWindow │                   │
│  │ SFMLRenderer │    │ SDLRenderer  │    │RaylibRenderer│                   │
│  │ SFMLInput    │    │ SDLInput     │    │ RaylibInput  │                   │
│  └──────────────┘    └──────────────┘    └──────────────┘                   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Structure des Fichiers à Créer

```
src/client/
├── core/
│   ├── Engine.hpp/.cpp
│   ├── GameLoop.hpp/.cpp
│   └── PluginLoader.hpp/.cpp       # NOUVEAU
│
├── graphics/
│   ├── IWindow.hpp                 # Interface abstraite (déjà existe)
│   ├── IRenderer.hpp               # Interface abstraite (déjà existe)
│   └── IGraphicsPlugin.hpp         # NOUVEAU - Interface plugin
│
├── input/
│   ├── InputAction.hpp             # NOUVEAU - Enum des actions
│   ├── IInput.hpp                  # NOUVEAU - Interface abstraite
│   └── InputManager.hpp/.cpp       # NOUVEAU - Utilise IInput
│
└── plugins/
    └── sfml/                       # Compile en .so/.dll séparé
        ├── CMakeLists.txt
        ├── SFMLPlugin.hpp/.cpp     # Implémente IGraphicsPlugin
        ├── SFMLWindow.hpp/.cpp     # Implémente IWindow
        ├── SFMLRenderer.hpp/.cpp   # Implémente IRenderer
        └── SFMLInput.hpp/.cpp      # Implémente IInput
```

### Étape 1 : Créer les Interfaces Abstraites

**Fichier : `src/client/input/InputAction.hpp`**
```cpp
#pragma once

enum class InputAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Fire,
    SpecialFire,
    Pause,
    Confirm,
    Cancel,
    COUNT  // Pour itérer
};
```

**Fichier : `src/client/input/IInput.hpp`**
```cpp
#pragma once

#include "InputAction.hpp"

/**
 * @brief Interface abstraite pour les inputs
 *
 * Implémentée par chaque plugin graphique (SFML, SDL, etc.)
 * Le core engine ne connaît QUE cette interface
 */
class IInput {
public:
    virtual ~IInput() = default;

    // Appelé au début de chaque frame pour traiter les events
    virtual void pollEvents() = 0;

    // Requêtes d'état
    virtual bool isPressed(InputAction action) const = 0;
    virtual bool isJustPressed(InputAction action) const = 0;
    virtual bool isJustReleased(InputAction action) const = 0;

    // Gestion fenêtre (détecté via events)
    virtual bool shouldClose() const = 0;

    // Optionnel : rebind
    virtual void bindKey(InputAction action, int keyCode) = 0;
};
```

**Fichier : `src/client/graphics/IGraphicsPlugin.hpp`**
```cpp
#pragma once

#include <memory>
#include <string>

class IWindow;
class IRenderer;
class IInput;

/**
 * @brief Interface pour les plugins graphiques
 *
 * Chaque plugin (SFML, SDL, Raylib...) implémente cette interface.
 * Le PluginLoader charge le .so/.dll et récupère cette interface.
 */
class IGraphicsPlugin {
public:
    virtual ~IGraphicsPlugin() = default;

    // Nom du plugin (pour logs)
    virtual std::string getName() const = 0;

    // Initialisation/Cleanup
    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Factory methods - crée les implémentations concrètes
    virtual std::unique_ptr<IWindow> createWindow(
        unsigned int width,
        unsigned int height,
        const std::string& title
    ) = 0;

    virtual std::unique_ptr<IRenderer> createRenderer(IWindow& window) = 0;
    virtual std::unique_ptr<IInput> createInput(IWindow& window) = 0;
};

// Point d'entrée exporté par chaque plugin
// Le PluginLoader appelle cette fonction après dlopen()
extern "C" {
    typedef IGraphicsPlugin* (*CreatePluginFn)();
    typedef void (*DestroyPluginFn)(IGraphicsPlugin*);
}

// Macro pour exporter le plugin (à mettre dans chaque plugin)
#define EXPORT_GRAPHICS_PLUGIN(PluginClass)                     \
    extern "C" {                                                \
        IGraphicsPlugin* createPlugin() {                       \
            return new PluginClass();                           \
        }                                                       \
        void destroyPlugin(IGraphicsPlugin* plugin) {           \
            delete plugin;                                      \
        }                                                       \
    }
```

### Étape 2 : Créer le PluginLoader

**Fichier : `src/client/core/PluginLoader.hpp`**
```cpp
#pragma once

#include <memory>
#include <string>
#include "graphics/IGraphicsPlugin.hpp"

/**
 * @brief Charge les plugins graphiques dynamiquement
 *
 * Utilise dlopen() sur Linux, LoadLibrary() sur Windows
 */
class PluginLoader {
public:
    PluginLoader() = default;
    ~PluginLoader();

    // Charge un plugin depuis un fichier .so/.dll
    // Retourne nullptr si échec
    IGraphicsPlugin* load(const std::string& path);

    // Décharge le plugin actuel
    void unload();

    // Vérifie si un plugin est chargé
    bool isLoaded() const;

private:
    void* _handle = nullptr;
    IGraphicsPlugin* _plugin = nullptr;
    DestroyPluginFn _destroyFn = nullptr;
};
```

**Fichier : `src/client/core/PluginLoader.cpp`**
```cpp
#include "PluginLoader.hpp"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) GetProcAddress((HMODULE)handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary((HMODULE)handle)
    #define PLUGIN_EXT ".dll"
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #define PLUGIN_EXT ".so"
#endif

PluginLoader::~PluginLoader() {
    unload();
}

IGraphicsPlugin* PluginLoader::load(const std::string& path) {
    // Décharger l'ancien plugin si présent
    unload();

    // Charger la bibliothèque
    _handle = LOAD_LIBRARY(path.c_str());
    if (!_handle) {
        #ifndef _WIN32
            std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
        #endif
        return nullptr;
    }

    // Récupérer les fonctions exportées
    auto createFn = reinterpret_cast<CreatePluginFn>(
        GET_SYMBOL(_handle, "createPlugin")
    );
    _destroyFn = reinterpret_cast<DestroyPluginFn>(
        GET_SYMBOL(_handle, "destroyPlugin")
    );

    if (!createFn || !_destroyFn) {
        std::cerr << "Plugin missing required symbols" << std::endl;
        CLOSE_LIBRARY(_handle);
        _handle = nullptr;
        return nullptr;
    }

    // Créer le plugin
    _plugin = createFn();
    if (!_plugin) {
        std::cerr << "Failed to create plugin instance" << std::endl;
        CLOSE_LIBRARY(_handle);
        _handle = nullptr;
        return nullptr;
    }

    return _plugin;
}

void PluginLoader::unload() {
    if (_plugin && _destroyFn) {
        _destroyFn(_plugin);
        _plugin = nullptr;
    }

    if (_handle) {
        CLOSE_LIBRARY(_handle);
        _handle = nullptr;
    }

    _destroyFn = nullptr;
}

bool PluginLoader::isLoaded() const {
    return _plugin != nullptr;
}
```

### Étape 3 : Créer le Plugin SFML

**Fichier : `src/client/plugins/sfml/SFMLInput.hpp`**
```cpp
#pragma once

#include "input/IInput.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <unordered_map>
#include <unordered_set>

class SFMLInput : public IInput {
public:
    explicit SFMLInput(sf::RenderWindow& window);

    void pollEvents() override;

    bool isPressed(InputAction action) const override;
    bool isJustPressed(InputAction action) const override;
    bool isJustReleased(InputAction action) const override;

    bool shouldClose() const override;

    void bindKey(InputAction action, int keyCode) override;

private:
    sf::RenderWindow& _window;
    bool _shouldClose = false;

    std::unordered_map<InputAction, sf::Keyboard::Key> _bindings;
    std::unordered_set<InputAction> _currentState;
    std::unordered_set<InputAction> _previousState;

    void setupDefaultBindings();
    void updateKeyState(sf::Keyboard::Key key, bool pressed);
};
```

**Fichier : `src/client/plugins/sfml/SFMLInput.cpp`**
```cpp
#include "SFMLInput.hpp"

SFMLInput::SFMLInput(sf::RenderWindow& window) : _window(window) {
    setupDefaultBindings();
}

void SFMLInput::setupDefaultBindings() {
    _bindings[InputAction::MoveUp] = sf::Keyboard::Key::Z;
    _bindings[InputAction::MoveDown] = sf::Keyboard::Key::S;
    _bindings[InputAction::MoveLeft] = sf::Keyboard::Key::Q;
    _bindings[InputAction::MoveRight] = sf::Keyboard::Key::D;
    _bindings[InputAction::Fire] = sf::Keyboard::Key::Space;
    _bindings[InputAction::SpecialFire] = sf::Keyboard::Key::LShift;
    _bindings[InputAction::Pause] = sf::Keyboard::Key::Escape;
    _bindings[InputAction::Confirm] = sf::Keyboard::Key::Enter;
    _bindings[InputAction::Cancel] = sf::Keyboard::Key::Escape;
}

void SFMLInput::pollEvents() {
    // Sauvegarder l'état précédent
    _previousState = _currentState;

    // Traiter les events
    while (auto event = _window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            _shouldClose = true;
        }
        else if (auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            updateKeyState(keyPressed->code, true);
        }
        else if (auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            updateKeyState(keyReleased->code, false);
        }
    }
}

void SFMLInput::updateKeyState(sf::Keyboard::Key key, bool pressed) {
    for (const auto& [action, boundKey] : _bindings) {
        if (boundKey == key) {
            if (pressed) {
                _currentState.insert(action);
            } else {
                _currentState.erase(action);
            }
        }
    }
}

bool SFMLInput::isPressed(InputAction action) const {
    return _currentState.count(action) > 0;
}

bool SFMLInput::isJustPressed(InputAction action) const {
    return _currentState.count(action) > 0 &&
           _previousState.count(action) == 0;
}

bool SFMLInput::isJustReleased(InputAction action) const {
    return _currentState.count(action) == 0 &&
           _previousState.count(action) > 0;
}

bool SFMLInput::shouldClose() const {
    return _shouldClose;
}

void SFMLInput::bindKey(InputAction action, int keyCode) {
    _bindings[action] = static_cast<sf::Keyboard::Key>(keyCode);
}
```

**Fichier : `src/client/plugins/sfml/SFMLPlugin.hpp`**
```cpp
#pragma once

#include "graphics/IGraphicsPlugin.hpp"

class SFMLPlugin : public IGraphicsPlugin {
public:
    std::string getName() const override { return "SFML"; }

    bool init() override;
    void shutdown() override;

    std::unique_ptr<IWindow> createWindow(
        unsigned int width,
        unsigned int height,
        const std::string& title
    ) override;

    std::unique_ptr<IRenderer> createRenderer(IWindow& window) override;
    std::unique_ptr<IInput> createInput(IWindow& window) override;
};

// Export le plugin
EXPORT_GRAPHICS_PLUGIN(SFMLPlugin)
```

### Étape 4 : CMakeLists.txt pour le Plugin

**Fichier : `src/client/plugins/sfml/CMakeLists.txt`**
```cmake
# Plugin SFML - compile en bibliothèque dynamique

add_library(sfml_plugin SHARED
    SFMLPlugin.cpp
    SFMLWindow.cpp
    SFMLRenderer.cpp
    SFMLInput.cpp
)

# Le plugin link SFML, pas le core engine
target_link_libraries(sfml_plugin PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

# Headers du core (interfaces)
target_include_directories(sfml_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/client/include
)

# Output dans le dossier plugins
set_target_properties(sfml_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins  # Windows
    PREFIX "lib"  # libsfml_plugin.so
)
```

### Étape 5 : Modifier le Core Engine

**Fichier : `src/client/core/Engine.hpp` (modifié)**
```cpp
#pragma once

#include <memory>
#include <string>

class IWindow;
class IRenderer;
class IInput;
class IGraphicsPlugin;
class PluginLoader;
class SceneManager;

class Engine {
public:
    Engine();
    ~Engine();

    // Initialise avec un plugin graphique
    bool init(const std::string& pluginPath);
    void run();
    void shutdown();

    // Accès aux systèmes (pour les scènes)
    IWindow& getWindow() { return *_window; }
    IRenderer& getRenderer() { return *_renderer; }
    IInput& getInput() { return *_input; }

private:
    std::unique_ptr<PluginLoader> _pluginLoader;
    IGraphicsPlugin* _plugin = nullptr;

    std::unique_ptr<IWindow> _window;
    std::unique_ptr<IRenderer> _renderer;
    std::unique_ptr<IInput> _input;
    std::unique_ptr<SceneManager> _sceneManager;

    bool _running = false;
};
```

### Étape 6 : Modifier IScene (Plus de sf::Event !)

**Fichier : `src/client/include/scenes/IScene.hpp` (modifié)**
```cpp
#pragma once

class IRenderer;
class IInput;

/**
 * @brief Interface pour les scènes
 *
 * IMPORTANT: Plus aucune dépendance SFML !
 * Les scènes utilisent IInput pour les inputs
 */
class IScene {
public:
    virtual ~IScene() = default;

    virtual void init() = 0;
    virtual void cleanup() = 0;

    // Plus de handleEvent(sf::Event) !
    // Les scènes interrogent IInput directement

    virtual void update(float deltaTime) = 0;
    virtual void render(IRenderer& renderer) = 0;
};
```

**Exemple d'utilisation dans GameScene :**
```cpp
void GameScene::update(float deltaTime) {
    // Plus de handleEvent() ! On query IInput directement

    if (_input.isPressed(InputAction::MoveUp)) {
        // Bouger vers le haut
    }

    if (_input.isJustPressed(InputAction::Fire)) {
        // Tirer (une seule fois par appui)
    }

    if (_input.isJustPressed(InputAction::Pause)) {
        // Mettre en pause
    }

    // ... reste de la logique
}
```

### Étape 7 : Nouveau GameLoop

**Fichier : `src/client/core/GameLoop.cpp` (modifié)**
```cpp
void GameLoop::run() {
    while (_running && !_input->shouldClose()) {
        // 1. Delta time
        _deltaTime = _clock.restart().asSeconds();
        if (_deltaTime > 0.1f) _deltaTime = 0.1f;

        // 2. Poll events (via plugin, pas direct SFML)
        _input->pollEvents();

        // 3. Update
        _sceneManager->update(_deltaTime);

        // 4. Render
        _window->clear();
        _sceneManager->render(*_renderer);
        _window->display();
    }
}
```

### Checklist Architecture Plugin

- [ ] Créer `IInput` interface abstraite
- [ ] Créer `IGraphicsPlugin` interface
- [ ] Créer `PluginLoader` (dlopen/LoadLibrary)
- [ ] Créer `SFMLInput` implémente `IInput`
- [ ] Créer `SFMLPlugin` implémente `IGraphicsPlugin`
- [ ] Modifier `SFMLWindow` pour implémenter `IWindow`
- [ ] Modifier `SFMLRenderer` pour implémenter `IRenderer`
- [ ] CMakeLists pour compiler le plugin en .so/.dll
- [ ] Modifier `Engine` pour charger le plugin
- [ ] **Retirer `sf::Event` de `IScene::handleEvent()`**
- [ ] Modifier `IScene::handleEvent()` → les scènes query `IInput`
- [ ] Mettre à jour `LoginScene`, `GameScene` etc.
- [ ] Tester le chargement du plugin

### Avantages de cette Architecture

1. **Découplage total** : Le core engine ne connaît pas SFML
2. **Testabilité** : On peut créer un MockPlugin pour les tests
3. **Flexibilité** : Facile d'ajouter SDL, Raylib, etc.
4. **Propreté** : Les interfaces sont claires et documentées
5. **Portabilité** : Même core, plugins différents par plateforme

---

## 0.2 Delta Time dans GameLoop

### Pourquoi c'est critique

Sans delta time :
- PC 60 FPS : objets bougent à vitesse X
- PC 144 FPS : objets bougent 2.4x plus vite
- PC 30 FPS : objets bougent 2x plus lent

**Le jeu est injouable sur différentes machines.**

### Ce qu'il faut faire

**Fichier : `src/client/core/GameLoop.hpp`**

Ajouter :
```cpp
private:
    sf::Clock _clock;
    float _deltaTime = 0.0f;
```

**Fichier : `src/client/core/GameLoop.cpp`**

Modifier `run()` :
```cpp
void GameLoop::run() {
    while (_window->isOpen()) {
        // 1. Calculer delta time
        _deltaTime = _clock.restart().asSeconds();

        // 2. Limiter delta time (évite les sauts après pause)
        if (_deltaTime > 0.1f) _deltaTime = 0.1f;

        // 3. Process events
        while (auto event = _window->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window->close();
                return;
            }
            _sceneManager->handleEvent(*event);
        }

        // 4. Update avec delta time
        _sceneManager->update(_deltaTime);

        // 5. Render
        clear();
        _sceneManager->render(_window);
        display();
    }
}
```

**Fichier : `src/client/include/scenes/IScene.hpp`**

Modifier l'interface :
```cpp
virtual void update(float deltaTime) = 0;  // Ajouter deltaTime
```

### Checklist

- [ ] Ajouter `sf::Clock _clock` dans GameLoop
- [ ] Calculer `_deltaTime` au début de chaque frame
- [ ] Modifier `IScene::update()` pour prendre `float deltaTime`
- [ ] Mettre à jour toutes les scènes existantes

---

## 0.3 InputManager (Wrapper optionnel)

> **Note** : L'essentiel de la gestion des inputs est dans le plugin (section 0.1).
> Cette section décrit un wrapper optionnel pour des fonctionnalités avancées.

### Fonctionnalités Avancées (Optionnel)

Si tu veux des features supplémentaires au-delà de `IInput` :

```cpp
#pragma once

#include "IInput.hpp"
#include <memory>

/**
 * @brief Wrapper optionnel autour de IInput
 *
 * Ajoute des fonctionnalités comme :
 * - Input buffering (pour combos)
 * - Rebinding runtime
 * - Sauvegarde/chargement config
 */
class InputManager {
public:
    explicit InputManager(IInput& input);

    // Délègue à IInput
    bool isPressed(InputAction action) const;
    bool isJustPressed(InputAction action) const;
    bool isJustReleased(InputAction action) const;

    // Fonctionnalités avancées
    void enableBuffering(float windowMs);  // Input buffering
    bool wasRecentlyPressed(InputAction action) const;  // Pour combos

    void saveBindings(const std::string& path);
    void loadBindings(const std::string& path);

private:
    IInput& _input;
    // ... état pour buffering, etc.
};
```

### Checklist InputManager (Optionnel)

- [ ] Créer `InputManager` wrapper (si besoin)
- [ ] Input buffering (pour combos)
- [ ] Sauvegarde/chargement config

> **Pour l'instant, utilise directement `IInput` dans les scènes. C'est suffisant.**

---

## 0.4 ECS Basique - Entity et World

### Structure des fichiers à créer

```
src/client/ecs/
├── Types.hpp           # Entity = uint32_t
├── Entity.hpp          # Constantes et helpers
├── ComponentPool.hpp   # Stockage template
├── EntityManager.hpp   # Création/destruction
├── World.hpp           # Façade
└── World.cpp
```

### Ce qu'il faut faire

**Fichier : `src/client/ecs/Types.hpp`**
```cpp
#pragma once
#include <cstdint>

using Entity = uint32_t;
constexpr Entity NULL_ENTITY = 0;
constexpr uint32_t MAX_ENTITIES = 10000;
```

**Fichier : `src/client/ecs/EntityManager.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include <queue>
#include <vector>

class EntityManager {
public:
    EntityManager();

    Entity create();
    void destroy(Entity entity);
    bool isAlive(Entity entity) const;

private:
    std::queue<Entity> _available;
    std::vector<bool> _alive;
    Entity _nextEntity = 1;  // 0 est NULL_ENTITY
};
```

**Fichier : `src/client/ecs/ComponentPool.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include <vector>
#include <unordered_map>

template<typename T>
class ComponentPool {
public:
    void add(Entity entity, T component) {
        _entityToIndex[entity] = _components.size();
        _indexToEntity.push_back(entity);
        _components.push_back(std::move(component));
    }

    void remove(Entity entity) {
        if (!has(entity)) return;

        size_t index = _entityToIndex[entity];
        size_t lastIndex = _components.size() - 1;

        if (index != lastIndex) {
            _components[index] = std::move(_components[lastIndex]);
            Entity lastEntity = _indexToEntity[lastIndex];
            _entityToIndex[lastEntity] = index;
            _indexToEntity[index] = lastEntity;
        }

        _components.pop_back();
        _indexToEntity.pop_back();
        _entityToIndex.erase(entity);
    }

    T& get(Entity entity) {
        return _components[_entityToIndex[entity]];
    }

    bool has(Entity entity) const {
        return _entityToIndex.count(entity) > 0;
    }

    std::vector<T>& all() { return _components; }
    const std::vector<Entity>& entities() const { return _indexToEntity; }

private:
    std::vector<T> _components;
    std::vector<Entity> _indexToEntity;
    std::unordered_map<Entity, size_t> _entityToIndex;
};
```

**Fichier : `src/client/ecs/World.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include "EntityManager.hpp"
#include "ComponentPool.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>

class World {
public:
    // Entités
    Entity createEntity();
    void destroyEntity(Entity entity);
    bool isAlive(Entity entity) const;

    // Composants
    template<typename T, typename... Args>
    T& addComponent(Entity entity, Args&&... args);

    template<typename T>
    void removeComponent(Entity entity);

    template<typename T>
    T& getComponent(Entity entity);

    template<typename T>
    bool hasComponent(Entity entity) const;

    // Accès aux pools (pour les systems)
    template<typename T>
    ComponentPool<T>& getPool();

private:
    EntityManager _entities;
    std::unordered_map<std::type_index, std::shared_ptr<void>> _pools;

    template<typename T>
    ComponentPool<T>& getOrCreatePool();
};

// Implémentations template (dans le header)
template<typename T, typename... Args>
T& World::addComponent(Entity entity, Args&&... args) {
    auto& pool = getOrCreatePool<T>();
    pool.add(entity, T{std::forward<Args>(args)...});
    return pool.get(entity);
}

template<typename T>
void World::removeComponent(Entity entity) {
    if (auto it = _pools.find(std::type_index(typeid(T))); it != _pools.end()) {
        auto& pool = *static_cast<ComponentPool<T>*>(it->second.get());
        pool.remove(entity);
    }
}

template<typename T>
T& World::getComponent(Entity entity) {
    return getPool<T>().get(entity);
}

template<typename T>
bool World::hasComponent(Entity entity) const {
    auto it = _pools.find(std::type_index(typeid(T)));
    if (it == _pools.end()) return false;
    return static_cast<ComponentPool<T>*>(it->second.get())->has(entity);
}

template<typename T>
ComponentPool<T>& World::getPool() {
    return *static_cast<ComponentPool<T>*>(_pools.at(std::type_index(typeid(T))).get());
}

template<typename T>
ComponentPool<T>& World::getOrCreatePool() {
    auto typeIdx = std::type_index(typeid(T));
    if (_pools.find(typeIdx) == _pools.end()) {
        _pools[typeIdx] = std::make_shared<ComponentPool<T>>();
    }
    return *static_cast<ComponentPool<T>*>(_pools[typeIdx].get());
}
```

### Checklist

- [ ] Créer dossier `src/client/ecs/`
- [ ] Créer `Types.hpp`
- [ ] Créer `EntityManager.hpp/.cpp`
- [ ] Créer `ComponentPool.hpp` (template)
- [ ] Créer `World.hpp/.cpp`
- [ ] Tester : créer entité, ajouter composant, récupérer

---

## 0.5 Validation Priorité 0

Avant de passer à la Priorité 1, vérifie :

### Test Plugin Architecture
```cpp
// Dans main.cpp
int main() {
    PluginLoader loader;

    // Charger le plugin SFML
    auto* plugin = loader.load("plugins/libsfml_plugin.so");
    if (!plugin) {
        std::cerr << "Failed to load plugin" << std::endl;
        return 1;
    }

    plugin->init();

    // Créer les objets via le plugin
    auto window = plugin->createWindow(1280, 720, "R-Type");
    auto renderer = plugin->createRenderer(*window);
    auto input = plugin->createInput(*window);

    // Test input sans sf::Event !
    while (!input->shouldClose()) {
        input->pollEvents();

        if (input->isPressed(InputAction::Fire)) {
            std::cout << "Fire!" << std::endl;
        }

        // ... render ...
    }

    plugin->shutdown();
    return 0;
}
```

### Test Delta Time
```cpp
// Delta time fonctionne
// update() reçoit float deltaTime
void GameScene::update(float deltaTime) {
    // deltaTime est entre 0.016 (60fps) et 0.033 (30fps)
    // Mouvement indépendant du framerate
    position.x += velocity.x * deltaTime;
}
```

### Test ECS
```cpp
// ECS fonctionne
World world;
Entity e = world.createEntity();

struct TestComponent { float x, y; };
world.addComponent<TestComponent>(e, 10.0f, 20.0f);

auto& comp = world.getComponent<TestComponent>(e);
// comp.x == 10.0f ✓

world.destroyEntity(e);
// world.isAlive(e) == false ✓
```

### Checklist Finale Priorité 0

- [ ] Plugin SFML compile en `.so`/`.dll` séparé
- [ ] `PluginLoader` charge le plugin correctement
- [ ] `IInput` fonctionne (plus de `sf::Event` dans les scènes !)
- [ ] Delta time calculé dans GameLoop
- [ ] `IScene::update()` reçoit `float deltaTime`
- [ ] ECS : création/destruction entités
- [ ] ECS : ajout/suppression composants
- [ ] ECS : query composants

---

# PRIORITÉ 1 : Core Gameplay

**⏱️ Durée estimée : 1 semaine**
**🎯 Objectif : Un joueur qui se déplace, tire, et peut mourir**

> Prérequis : Priorité 0 complète

---

## 1.1 Components de Base

### Fichiers à créer

```
src/client/components/
├── TransformComponent.hpp
├── VelocityComponent.hpp
├── SpriteComponent.hpp
├── ColliderComponent.hpp
├── HealthComponent.hpp
├── PlayerComponent.hpp
└── ProjectileComponent.hpp
```

### TransformComponent

```cpp
#pragma once

struct TransformComponent {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};
```

### VelocityComponent

```cpp
#pragma once

struct VelocityComponent {
    float vx = 0.0f;
    float vy = 0.0f;
    float maxSpeed = 300.0f;
};
```

### SpriteComponent

```cpp
#pragma once
#include <string>

struct SpriteComponent {
    std::string textureKey;
    int srcX = 0, srcY = 0;
    int srcWidth = 32, srcHeight = 32;
    int zOrder = 0;
    bool flipX = false;
    bool flipY = false;
};
```

### ColliderComponent

```cpp
#pragma once
#include <cstdint>

// Layers (bitmask)
namespace CollisionLayer {
    constexpr uint32_t NONE          = 0;
    constexpr uint32_t PLAYER        = 1 << 0;  // 1
    constexpr uint32_t ENEMY         = 1 << 1;  // 2
    constexpr uint32_t PLAYER_BULLET = 1 << 2;  // 4
    constexpr uint32_t ENEMY_BULLET  = 1 << 3;  // 8
    constexpr uint32_t POWERUP       = 1 << 4;  // 16
}

struct ColliderComponent {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float width = 32.0f;
    float height = 32.0f;
    uint32_t layer = CollisionLayer::NONE;
    uint32_t mask = CollisionLayer::NONE;  // Avec quoi on collide
    bool isTrigger = false;
};
```

### HealthComponent

```cpp
#pragma once

struct HealthComponent {
    float current = 100.0f;
    float max = 100.0f;
    float invincibilityTimer = 0.0f;
    float invincibilityDuration = 2.0f;
    bool isDead = false;
};
```

### PlayerComponent

```cpp
#pragma once

struct PlayerComponent {
    int playerId = 1;
    int score = 0;
    int lives = 3;
    float speed = 300.0f;
    float fireCooldown = 0.0f;
    float fireRate = 0.15f;  // Secondes entre chaque tir
};
```

### ProjectileComponent

```cpp
#pragma once
#include "../ecs/Types.hpp"

struct ProjectileComponent {
    float damage = 10.0f;
    Entity owner = NULL_ENTITY;
    float lifetime = 5.0f;
    bool isPlayerBullet = true;
};
```

### Checklist Components

- [ ] Créer dossier `src/client/components/`
- [ ] TransformComponent
- [ ] VelocityComponent
- [ ] SpriteComponent
- [ ] ColliderComponent (avec les layers)
- [ ] HealthComponent
- [ ] PlayerComponent
- [ ] ProjectileComponent

---

## 1.2 Systems de Base

### Fichiers à créer

```
src/client/systems/
├── ISystem.hpp
├── MovementSystem.hpp/.cpp
├── RenderSystem.hpp/.cpp
├── CollisionSystem.hpp/.cpp
├── PlayerInputSystem.hpp/.cpp
├── HealthSystem.hpp/.cpp
├── LifetimeSystem.hpp/.cpp
└── CleanupSystem.hpp/.cpp
```

### ISystem Interface

```cpp
#pragma once

class World;

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(World& world, float deltaTime) = 0;
};
```

### MovementSystem

```cpp
#pragma once
#include "ISystem.hpp"

class MovementSystem : public ISystem {
public:
    void update(World& world, float deltaTime) override;
};

// Dans .cpp
void MovementSystem::update(World& world, float deltaTime) {
    auto& transforms = world.getPool<TransformComponent>();
    auto& velocities = world.getPool<VelocityComponent>();

    for (Entity entity : velocities.entities()) {
        if (!transforms.has(entity)) continue;

        auto& t = transforms.get(entity);
        auto& v = velocities.get(entity);

        t.x += v.vx * deltaTime;
        t.y += v.vy * deltaTime;
    }
}
```

### RenderSystem

```cpp
#pragma once
#include "ISystem.hpp"
#include <memory>

class IRenderer;

class RenderSystem {
public:
    RenderSystem(std::shared_ptr<IRenderer> renderer);
    void render(World& world);  // Pas update, c'est render

private:
    std::shared_ptr<IRenderer> _renderer;
};

// Dans .cpp
void RenderSystem::render(World& world) {
    auto& transforms = world.getPool<TransformComponent>();
    auto& sprites = world.getPool<SpriteComponent>();

    // Collecter et trier par zOrder
    std::vector<Entity> renderList;
    for (Entity e : sprites.entities()) {
        if (transforms.has(e)) {
            renderList.push_back(e);
        }
    }

    std::sort(renderList.begin(), renderList.end(), [&](Entity a, Entity b) {
        return sprites.get(a).zOrder < sprites.get(b).zOrder;
    });

    // Dessiner
    for (Entity e : renderList) {
        auto& t = transforms.get(e);
        auto& s = sprites.get(e);
        _renderer->drawSprite(s.textureKey, t.x, t.y, s.srcX, s.srcY,
                              s.srcWidth, s.srcHeight, t.rotation,
                              t.scaleX, t.scaleY);
    }
}
```

### PlayerInputSystem

```cpp
#pragma once
#include "ISystem.hpp"

class InputManager;
class ProjectileFactory;

class PlayerInputSystem : public ISystem {
public:
    PlayerInputSystem(InputManager& input, ProjectileFactory& projectiles);
    void update(World& world, float deltaTime) override;

private:
    InputManager& _input;
    ProjectileFactory& _projectiles;
};

// Dans .cpp
void PlayerInputSystem::update(World& world, float deltaTime) {
    auto& players = world.getPool<PlayerComponent>();
    auto& velocities = world.getPool<VelocityComponent>();
    auto& transforms = world.getPool<TransformComponent>();

    for (Entity entity : players.entities()) {
        if (!velocities.has(entity)) continue;

        auto& player = players.get(entity);
        auto& vel = velocities.get(entity);

        // Reset velocity
        vel.vx = 0;
        vel.vy = 0;

        // Mouvement
        if (_input.isPressed(InputAction::MoveUp))
            vel.vy = -player.speed;
        if (_input.isPressed(InputAction::MoveDown))
            vel.vy = player.speed;
        if (_input.isPressed(InputAction::MoveLeft))
            vel.vx = -player.speed;
        if (_input.isPressed(InputAction::MoveRight))
            vel.vx = player.speed;

        // Tir
        player.fireCooldown -= deltaTime;
        if (_input.isPressed(InputAction::Fire) && player.fireCooldown <= 0) {
            auto& t = transforms.get(entity);
            _projectiles.createPlayerBullet(world, t.x + 32, t.y + 16);
            player.fireCooldown = player.fireRate;
        }
    }
}
```

### CollisionSystem

```cpp
#pragma once
#include "ISystem.hpp"
#include <functional>
#include <vector>

struct CollisionEvent {
    Entity a;
    Entity b;
};

class CollisionSystem : public ISystem {
public:
    void update(World& world, float deltaTime) override;

    // Callbacks pour réagir aux collisions
    std::vector<CollisionEvent>& getCollisions() { return _collisions; }

private:
    std::vector<CollisionEvent> _collisions;

    bool intersects(float ax, float ay, float aw, float ah,
                    float bx, float by, float bw, float bh);
};

// Dans .cpp
void CollisionSystem::update(World& world, float deltaTime) {
    _collisions.clear();

    auto& transforms = world.getPool<TransformComponent>();
    auto& colliders = world.getPool<ColliderComponent>();

    auto entities = colliders.entities();

    for (size_t i = 0; i < entities.size(); ++i) {
        Entity a = entities[i];
        if (!transforms.has(a)) continue;

        auto& ta = transforms.get(a);
        auto& ca = colliders.get(a);

        for (size_t j = i + 1; j < entities.size(); ++j) {
            Entity b = entities[j];
            if (!transforms.has(b)) continue;

            auto& tb = transforms.get(b);
            auto& cb = colliders.get(b);

            // Check layer/mask
            bool aCanHitB = (ca.layer & cb.mask) != 0;
            bool bCanHitA = (cb.layer & ca.mask) != 0;
            if (!aCanHitB && !bCanHitA) continue;

            // AABB test
            if (intersects(
                ta.x + ca.offsetX, ta.y + ca.offsetY, ca.width, ca.height,
                tb.x + cb.offsetX, tb.y + cb.offsetY, cb.width, cb.height
            )) {
                _collisions.push_back({a, b});
            }
        }
    }
}

bool CollisionSystem::intersects(float ax, float ay, float aw, float ah,
                                  float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}
```

### Checklist Systems

- [ ] Créer dossier `src/client/systems/`
- [ ] ISystem interface
- [ ] MovementSystem
- [ ] RenderSystem
- [ ] PlayerInputSystem
- [ ] CollisionSystem
- [ ] HealthSystem
- [ ] LifetimeSystem (pour projectiles)
- [ ] CleanupSystem (supprime entités mortes)

---

## 1.3 Factories

### Fichiers à créer

```
src/client/factories/
├── PlayerFactory.hpp/.cpp
└── ProjectileFactory.hpp/.cpp
```

### PlayerFactory

```cpp
#pragma once
#include "../ecs/Types.hpp"

class World;

class PlayerFactory {
public:
    Entity create(World& world, int playerId, float x, float y);
};

// Dans .cpp
Entity PlayerFactory::create(World& world, int playerId, float x, float y) {
    Entity e = world.createEntity();

    world.addComponent<TransformComponent>(e, x, y, 0.0f, 1.0f, 1.0f);
    world.addComponent<VelocityComponent>(e, 0.0f, 0.0f, 300.0f);
    world.addComponent<SpriteComponent>(e, "player", 0, 0, 32, 32, 10);
    world.addComponent<ColliderComponent>(e, 4.0f, 4.0f, 24.0f, 24.0f,
        CollisionLayer::PLAYER,
        CollisionLayer::ENEMY | CollisionLayer::ENEMY_BULLET | CollisionLayer::POWERUP);
    world.addComponent<HealthComponent>(e, 100.0f, 100.0f, 0.0f, 2.0f, false);
    world.addComponent<PlayerComponent>(e, playerId, 0, 3, 300.0f, 0.0f, 0.15f);

    return e;
}
```

### ProjectileFactory

```cpp
#pragma once
#include "../ecs/Types.hpp"

class World;

class ProjectileFactory {
public:
    Entity createPlayerBullet(World& world, float x, float y);
    Entity createEnemyBullet(World& world, float x, float y, float vx, float vy);
};

// Dans .cpp
Entity ProjectileFactory::createPlayerBullet(World& world, float x, float y) {
    Entity e = world.createEntity();

    world.addComponent<TransformComponent>(e, x, y);
    world.addComponent<VelocityComponent>(e, 500.0f, 0.0f);  // Va vers la droite
    world.addComponent<SpriteComponent>(e, "bullet_player", 0, 0, 16, 8, 5);
    world.addComponent<ColliderComponent>(e, 0, 0, 16, 8,
        CollisionLayer::PLAYER_BULLET,
        CollisionLayer::ENEMY);
    world.addComponent<ProjectileComponent>(e, 10.0f, NULL_ENTITY, 3.0f, true);

    return e;
}
```

### Checklist Factories

- [ ] Créer dossier `src/client/factories/`
- [ ] PlayerFactory
- [ ] ProjectileFactory

---

## 1.4 Intégration dans GameScene

Modifier `GameScene` pour utiliser l'ECS :

```cpp
class GameScene : public IScene {
public:
    GameScene(std::shared_ptr<IRenderer> renderer, InputManager& input);

    void init();
    void update(float deltaTime) override;
    void render() override;

private:
    World _world;

    // Systems
    std::unique_ptr<PlayerInputSystem> _playerInput;
    std::unique_ptr<MovementSystem> _movement;
    std::unique_ptr<CollisionSystem> _collision;
    std::unique_ptr<HealthSystem> _health;
    std::unique_ptr<LifetimeSystem> _lifetime;
    std::unique_ptr<CleanupSystem> _cleanup;
    std::unique_ptr<RenderSystem> _render;

    // Factories
    PlayerFactory _playerFactory;
    ProjectileFactory _projectileFactory;

    // Autres
    Entity _player;
};

void GameScene::init() {
    // Créer le joueur
    _player = _playerFactory.create(_world, 1, 100, 300);

    // Initialiser systems
    _playerInput = std::make_unique<PlayerInputSystem>(_input, _projectileFactory);
    _movement = std::make_unique<MovementSystem>();
    _collision = std::make_unique<CollisionSystem>();
    // etc...
}

void GameScene::update(float deltaTime) {
    // Ordre critique !
    _playerInput->update(_world, deltaTime);
    _movement->update(_world, deltaTime);
    _collision->update(_world, deltaTime);
    _health->update(_world, deltaTime);
    _lifetime->update(_world, deltaTime);
    _cleanup->update(_world, deltaTime);
}

void GameScene::render() {
    _render->render(_world);
}
```

### Checklist Intégration

- [ ] Modifier GameScene pour avoir World
- [ ] Instancier tous les systems
- [ ] Créer le joueur au init
- [ ] Appeler systems dans update() (bon ordre !)
- [ ] Appeler render

---

## 1.5 Validation Priorité 1

Avant de passer à la Priorité 2 :

- [ ] Le joueur se déplace avec ZQSD
- [ ] Le joueur tire avec Espace
- [ ] Les projectiles avancent et disparaissent hors écran
- [ ] Le joueur reste dans les limites de l'écran
- [ ] Le système de collision détecte les collisions

---

# PRIORITÉ 2 : Gameplay Complet

**⏱️ Durée estimée : 1-2 semaines**
**🎯 Objectif : Un jeu solo complet avec ennemis, vagues, power-ups**

> Prérequis : Priorité 1 complète

---

## 2.1 Ennemis

### Nouveaux Components

**EnemyComponent.hpp**
```cpp
#pragma once

enum class EnemyType {
    Basic,      // Vol horizontal
    Wave,       // Mouvement sinusoïdal
    Shooter,    // Tire vers le joueur
    Charger     // Fonce vers le joueur
};

enum class AIState {
    Idle,
    Patrol,
    Chase,
    Attack
};

struct EnemyComponent {
    EnemyType type = EnemyType::Basic;
    AIState state = AIState::Patrol;
    float stateTimer = 0.0f;
    int pointsValue = 100;
    float fireRate = 1.0f;
    float fireCooldown = 0.0f;
};
```

### EnemyFactory

```cpp
class EnemyFactory {
public:
    Entity createBasic(World& world, float x, float y);
    Entity createWave(World& world, float x, float y);
    Entity createShooter(World& world, float x, float y);
    Entity createCharger(World& world, float x, float y);
};
```

### EnemyAISystem

Gère les patterns de mouvement et le comportement :
- **Basic** : Déplacement horizontal vers la gauche
- **Wave** : Mouvement sinusoïdal
- **Shooter** : Tire périodiquement vers le joueur
- **Charger** : Accélère vers le joueur

### Checklist Ennemis

- [ ] EnemyComponent avec types et états
- [ ] EnemyFactory avec les 4 types
- [ ] EnemyAISystem
- [ ] Ennemis tirent (pour Shooter)
- [ ] Collision enemy/player bullet → enemy meurt
- [ ] Collision enemy/player → player prend des dégâts

---

## 2.2 Wave System

### WaveManager

```cpp
struct EnemySpawn {
    EnemyType type;
    float x, y;
    float delay;  // Secondes après début de vague
};

struct Wave {
    std::vector<EnemySpawn> spawns;
    float duration;
};

class WaveManager {
public:
    void loadWaves(const std::string& filename);  // Ou hardcodé
    void update(World& world, EnemyFactory& factory, float deltaTime);

    int getCurrentWave() const;
    bool isComplete() const;

private:
    std::vector<Wave> _waves;
    int _currentWave = 0;
    float _waveTimer = 0.0f;
    int _spawnIndex = 0;
    int _enemiesAlive = 0;
};
```

### Checklist Waves

- [ ] Structure Wave et EnemySpawn
- [ ] WaveManager
- [ ] Définir 3-5 vagues de test
- [ ] Transition entre vagues
- [ ] Afficher numéro de vague

---

## 2.3 Power-ups

### PowerUpComponent

```cpp
enum class PowerUpType {
    SpeedBoost,   // +50% vitesse
    RapidFire,    // -50% cooldown
    Shield,       // Absorbe 1 hit
    MultiShot,    // 3 projectiles
    Health        // +25% HP
};

struct PowerUpComponent {
    PowerUpType type;
    float duration = 10.0f;  // -1 = permanent/instant
};
```

### PowerUpSystem

- Détecte collision player/powerup
- Applique l'effet
- Gère la durée des effets temporaires

### Checklist Power-ups

- [ ] PowerUpComponent
- [ ] PowerUpFactory
- [ ] PowerUpSystem
- [ ] 3 types minimum fonctionnels
- [ ] Ennemis drop power-ups (aléatoire)

---

## 2.4 Animation

### AnimationComponent

```cpp
struct AnimationFrame {
    int srcX, srcY, srcW, srcH;
};

struct AnimationComponent {
    std::vector<AnimationFrame> frames;
    int currentFrame = 0;
    float frameTime = 0.1f;  // Secondes par frame
    float timer = 0.0f;
    bool loop = true;
    bool playing = true;
};
```

### AnimationSystem

```cpp
void AnimationSystem::update(World& world, float deltaTime) {
    auto& animations = world.getPool<AnimationComponent>();
    auto& sprites = world.getPool<SpriteComponent>();

    for (Entity e : animations.entities()) {
        if (!sprites.has(e)) continue;

        auto& anim = animations.get(e);
        if (!anim.playing || anim.frames.empty()) continue;

        anim.timer += deltaTime;
        if (anim.timer >= anim.frameTime) {
            anim.timer = 0;
            anim.currentFrame++;

            if (anim.currentFrame >= anim.frames.size()) {
                if (anim.loop) anim.currentFrame = 0;
                else {
                    anim.currentFrame = anim.frames.size() - 1;
                    anim.playing = false;
                }
            }

            // Update sprite
            auto& frame = anim.frames[anim.currentFrame];
            auto& sprite = sprites.get(e);
            sprite.srcX = frame.srcX;
            sprite.srcY = frame.srcY;
            sprite.srcWidth = frame.srcW;
            sprite.srcHeight = frame.srcH;
        }
    }
}
```

### Checklist Animation

- [ ] AnimationComponent
- [ ] AnimationSystem
- [ ] Animation joueur (idle, haut, bas)
- [ ] Animation ennemis
- [ ] Animation explosions

---

## 2.5 Score et Game Over

### ScoreManager

```cpp
class ScoreManager {
public:
    void addScore(int points);
    int getScore() const;
    int getHighScore() const;
    void reset();

private:
    int _score = 0;
    int _highScore = 0;
};
```

### Game Over Logic

Dans GameScene :
```cpp
void GameScene::update(float deltaTime) {
    // ... systems ...

    // Check game over
    if (!_world.hasComponent<PlayerComponent>(_player) ||
        _world.getComponent<HealthComponent>(_player).isDead) {
        if (_world.getComponent<PlayerComponent>(_player).lives <= 0) {
            _sceneManager->changeScene<GameOverScene>(_scoreManager.getScore());
        } else {
            // Respawn
            respawnPlayer();
        }
    }
}
```

### Checklist Score/Game Over

- [ ] ScoreManager
- [ ] HUD affiche score et vies
- [ ] Game Over quand vies = 0
- [ ] GameOverScene avec score final
- [ ] Option restart

---

## 2.6 Validation Priorité 2

Avant de passer à la Priorité 3 :

- [ ] Ennemis spawn en vagues
- [ ] 4 types d'ennemis différents
- [ ] Ennemis tireurs fonctionnent
- [ ] Power-ups drop et fonctionnent
- [ ] Animations fluides
- [ ] Score augmente quand ennemi tué
- [ ] Game over quand plus de vies
- [ ] **Le jeu est jouable en solo !**

---

# PRIORITÉ 3 : Multijoueur

**⏱️ Durée estimée : 2 semaines**
**🎯 Objectif : 2-4 joueurs simultanés**

> Prérequis : Priorité 2 complète

---

## 3.1 Architecture Réseau

```
┌─────────────────────────────────────────────────────────────────┐
│                      ARCHITECTURE                                │
│                                                                  │
│  CLIENT                              SERVEUR                     │
│  ┌─────────────┐                    ┌─────────────┐             │
│  │   World     │                    │   World     │             │
│  │  (prédit)   │◀──── UDP ────────▶│(authoritative)│             │
│  └─────────────┘                    └─────────────┘             │
│        │                                   │                     │
│        │ TCP                         TCP   │                     │
│        │ (Auth, Lobby)              (Auth, Lobby)                │
│        ▼                                   ▼                     │
│  ┌─────────────┐                    ┌─────────────┐             │
│  │  Lobby UI   │◀──────────────────▶│LobbyManager │             │
│  └─────────────┘                    └─────────────┘             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Protocole

- **TCP** : Authentification, Lobby, Chat
- **UDP** : Gameplay temps réel (inputs, world snapshots)

### Messages Protobuf (déjà créés)

- `proto/auth.proto` : Authentification
- `proto/game.proto` : Gameplay + Lobby

---

## 3.2 NetworkSyncComponent

```cpp
struct NetworkSyncComponent {
    uint32_t networkId = 0;     // ID unique réseau
    bool isLocalPlayer = false; // Ce client contrôle cette entité
    bool isDirty = false;       // A changé depuis dernier envoi
    uint32_t lastUpdateTick = 0;
};
```

---

## 3.3 Client-Side Prediction

Le client prédit le résultat de ses inputs sans attendre le serveur :

1. Client envoie input au serveur
2. Client applique input localement immédiatement
3. Client sauvegarde l'état dans un historique
4. Serveur renvoie l'état confirmé
5. Si différence, client corrige et rejoue les inputs non confirmés

---

## 3.4 Entity Interpolation

Pour les autres joueurs/entités (non locales) :

1. Client reçoit snapshots du serveur
2. Client stocke les derniers snapshots dans un buffer
3. Client affiche avec un délai (ex: 100ms)
4. Client interpole entre deux snapshots pour fluidité

---

## 3.5 Checklist Multijoueur

- [ ] NetworkSyncComponent
- [ ] UDPClient pour gameplay
- [ ] Sérialisation Protobuf
- [ ] Client envoie inputs
- [ ] Client reçoit world snapshots
- [ ] Client-side prediction
- [ ] Entity interpolation
- [ ] Lobby (TCP)
- [ ] 2-4 joueurs simultanés

---

# PRIORITÉ 4 : Polish

**⏱️ Durée estimée : 1-2 semaines**
**🎯 Objectif : Jeu fini et présentable**

> Prérequis : Priorité 3 complète (ou 2 si solo uniquement)

---

## 4.1 Audio

### AudioManager

```cpp
class AudioManager {
public:
    void loadSound(const std::string& key, const std::string& file);
    void loadMusic(const std::string& key, const std::string& file);

    void playSound(const std::string& key);
    void playMusic(const std::string& key, bool loop = true);
    void stopMusic();

    void setMasterVolume(float volume);
    void setSoundVolume(float volume);
    void setMusicVolume(float volume);
};
```

### Sons à créer/trouver

| Catégorie | Sons |
|-----------|------|
| Joueur | tir, hit, mort, power-up |
| Ennemis | tir, explosion |
| UI | click, hover |
| Musique | menu, gameplay, boss, game over |

### Checklist Audio

- [ ] AudioManager
- [ ] Sons gameplay
- [ ] Musique de fond
- [ ] Options volume

---

## 4.2 Particules

### ParticleSystem

```cpp
struct Particle {
    float x, y;
    float vx, vy;
    float lifetime;
    float maxLifetime;
    float size;
    uint8_t r, g, b, a;
};

class ParticleSystem {
public:
    void emit(float x, float y, const ParticleConfig& config, int count);
    void update(float deltaTime);
    void render(IRenderer& renderer);

private:
    std::vector<Particle> _particles;
};
```

### Effets à créer

- Explosion ennemi
- Explosion joueur
- Collecte power-up
- Trail projectile

### Checklist Particules

- [ ] ParticleSystem basique
- [ ] Effet explosion
- [ ] Effet power-up

---

## 4.3 Menus et UI

### Scènes à créer

- MainMenuScene
- PauseScene (overlay)
- SettingsScene
- LobbyScene (si multi)
- GameOverScene (améliorer)
- VictoryScene

### Checklist Menus

- [ ] Menu principal
- [ ] Menu pause
- [ ] Paramètres (volume, contrôles)
- [ ] Transitions fluides

---

## 4.4 Contenu

### Niveaux

- 3 niveaux minimum
- Backgrounds parallax
- Boss de fin de niveau

### Checklist Contenu

- [ ] 3 niveaux
- [ ] Backgrounds
- [ ] 3 boss
- [ ] Progression difficulté

---

## 4.5 Polish Final

- [ ] Bugfix général
- [ ] Performance
- [ ] Équilibrage difficulté
- [ ] Feedback utilisateur (screen shake, flash)
- [ ] Instructions/tutoriel

---

# Annexes

## A. Structure Complète des Fichiers (avec Plugins)

```
src/client/
├── main.cpp
├── main.hpp
│
├── core/
│   ├── Engine.hpp/.cpp           # Charge le plugin, gère le cycle de vie
│   ├── GameLoop.hpp/.cpp         # Boucle principale avec delta time
│   ├── PluginLoader.hpp/.cpp     # dlopen/LoadLibrary
│   └── Logger.hpp/.cpp
│
├── graphics/
│   ├── IWindow.hpp               # Interface abstraite (AUCUN SFML)
│   ├── IRenderer.hpp             # Interface abstraite (AUCUN SFML)
│   └── IGraphicsPlugin.hpp       # Interface plugin
│
├── input/
│   ├── InputAction.hpp           # Enum des actions
│   ├── IInput.hpp                # Interface abstraite (AUCUN SFML)
│   └── InputManager.hpp/.cpp     # Wrapper optionnel
│
├── ecs/
│   ├── Types.hpp
│   ├── Entity.hpp
│   ├── ComponentPool.hpp
│   ├── EntityManager.hpp/.cpp
│   └── World.hpp/.cpp
│
├── components/
│   ├── TransformComponent.hpp
│   ├── VelocityComponent.hpp
│   ├── SpriteComponent.hpp
│   ├── AnimationComponent.hpp
│   ├── ColliderComponent.hpp
│   ├── HealthComponent.hpp
│   ├── PlayerComponent.hpp
│   ├── EnemyComponent.hpp
│   ├── ProjectileComponent.hpp
│   ├── PowerUpComponent.hpp
│   └── NetworkSyncComponent.hpp
│
├── systems/
│   ├── ISystem.hpp
│   ├── MovementSystem.hpp/.cpp
│   ├── RenderSystem.hpp/.cpp
│   ├── AnimationSystem.hpp/.cpp
│   ├── CollisionSystem.hpp/.cpp
│   ├── PlayerInputSystem.hpp/.cpp
│   ├── EnemyAISystem.hpp/.cpp
│   ├── HealthSystem.hpp/.cpp
│   ├── PowerUpSystem.hpp/.cpp
│   ├── LifetimeSystem.hpp/.cpp
│   ├── CleanupSystem.hpp/.cpp
│   └── NetworkSyncSystem.hpp/.cpp
│
├── factories/
│   ├── PlayerFactory.hpp/.cpp
│   ├── EnemyFactory.hpp/.cpp
│   ├── ProjectileFactory.hpp/.cpp
│   └── PowerUpFactory.hpp/.cpp
│
├── scenes/
│   ├── IScene.hpp                # PLUS de sf::Event ! Utilise IInput
│   ├── SceneManager.hpp/.cpp
│   ├── MainMenuScene.hpp/.cpp
│   ├── LoginScene.hpp/.cpp
│   ├── LobbyScene.hpp/.cpp
│   ├── GameScene.hpp/.cpp
│   ├── PauseScene.hpp/.cpp
│   └── GameOverScene.hpp/.cpp
│
├── ui/
│   └── ...
│
├── network/
│   ├── TCPClient.hpp/.cpp
│   ├── UDPClient.hpp/.cpp
│   └── NetworkManager.hpp/.cpp
│
├── audio/
│   └── AudioManager.hpp/.cpp
│
├── utils/
│   ├── Vecs.hpp
│   └── ...
│
└── plugins/                      # PLUGINS (libs dynamiques)
    └── sfml/                     # Plugin SFML
        ├── CMakeLists.txt        # Compile en libsfml_plugin.so/.dll
        ├── SFMLPlugin.hpp/.cpp   # Implémente IGraphicsPlugin
        ├── SFMLWindow.hpp/.cpp   # Implémente IWindow
        ├── SFMLRenderer.hpp/.cpp # Implémente IRenderer
        ├── SFMLInput.hpp/.cpp    # Implémente IInput
        └── SFMLAssetManager.hpp/.cpp

# Output après compilation
artifacts/
├── r-type_client               # Exécutable principal
└── plugins/
    └── libsfml_plugin.so       # Plugin SFML (.dll sur Windows)
```

### Séparation des Responsabilités

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CE QUI VA OÙ                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  CORE ENGINE (src/client/)          PLUGINS (src/client/plugins/)   │
│  ────────────────────────           ─────────────────────────────   │
│  • Interfaces abstraites             • Implémentations concrètes    │
│    (IWindow, IRenderer, IInput)        (SFMLWindow, SFMLRenderer)   │
│  • ECS (World, Components)           • Dépendances graphiques       │
│  • Scènes (IScene, GameScene)          (SFML, SDL, Raylib...)       │
│  • Systems (Movement, Collision)     • Code spécifique plateforme   │
│  • Factories                                                         │
│  • Network (TCP, UDP)                                               │
│  • Audio (interface)                                                │
│                                                                      │
│  ⚠️  AUCUN #include <SFML/...>       ✅ #include <SFML/...> OK      │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## B. Ordre d'Exécution des Systems

```
1. PlayerInputSystem     (lit inputs → modifie velocity)
2. EnemyAISystem         (IA → modifie velocity)
3. MovementSystem        (velocity → position)
4. BoundsSystem          (clamp aux bords)
5. CollisionSystem       (détecte collisions)
6. HealthSystem          (applique dégâts)
7. PowerUpSystem         (applique effets)
8. LifetimeSystem        (décrémente lifetime)
9. CleanupSystem         (supprime entités mortes)
10. AnimationSystem      (update frames)
11. RenderSystem         (dessine - séparé de update)
```

## C. Ressources

- [Game Programming Patterns](https://gameprogrammingpatterns.com/) - Livre gratuit
- [Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/) - Delta time
- [ECS FAQ](https://github.com/SanderMertens/ecs-faq) - Guide ECS
- [Networked Physics](https://gafferongames.com/categories/networked-physics/) - Réseau jeu

---

**Bon courage !**

*Document mis à jour le 25/11/2025 - v2.1 (ajout architecture plugins)*
