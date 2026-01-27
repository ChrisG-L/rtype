# Rapport Complet : Interfaces Utilisateur R-Type Client

**Competences couvertes:** 10 (interfaces GUI/TUI/CLI), 13 (code operationnel, conventions)

---

## 1. ABSTRACTION GRAPHIQUE : Pattern Multi-Backend

### 1.1 Architecture IWindow

Le systeme graphique utilise une **interface abstraite `IWindow`** implementee par deux backends :

**Fichier : `src/client/include/graphics/IWindow.hpp:20-62`**
```cpp
class IWindow {
    virtual Vec2u getSize() const = 0;
    virtual bool isOpen() = 0;
    virtual void close() = 0;
    virtual events::Event pollEvent() = 0;
    virtual void draw(const IDrawable& drawable) = 0;
    virtual void drawRect(float x, float y, float width, float height, rgba color) = 0;
    virtual void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) = 0;
    virtual bool loadTexture(const std::string& key, const std::string& filepath) = 0;
    virtual void drawSprite(const std::string& textureKey, float x, float y, float width, float height) = 0;
    virtual bool loadFont(const std::string& key, const std::string& filepath) = 0;
    virtual void drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color) = 0;
    // Post-processing shaders (SFML only)
    virtual bool loadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) = 0;
    virtual void setPostProcessShader(const std::string& key) = 0;
    virtual void clearPostProcessShader() = 0;
    // Fullscreen support
    virtual void setFullscreen(bool enabled) = 0;
    virtual void toggleFullscreen() = 0;
    virtual bool isFullscreen() const = 0;
};
```

### 1.2 Implementations Multi-Backend

**SFML Backend** : `src/client/lib/sfml/src/SFMLWindow.cpp`
- Support **shader post-processing** (colorblind filters)
- Gestion du **letterboxing 16:9** avec calcul de viewport
- Conversion de coordonnees pixel -> logique (1920x1080)

**Fichier exemple : `src/client/lib/sfml/src/SFMLWindow.cpp:336-384`**
```cpp
void SFMLWindow::setFullscreen(bool enabled) {
    if (enabled) {
        _window.create(sf::VideoMode::getDesktopMode(), _windowTitle, sf::Style::None);
        _window.setPosition({0, 0});
    } else {
        _window.create(sf::VideoMode({1920, 1080}), _windowTitle, sf::Style::Default);
    }
    auto size = _window.getSize();
    handleResize(size.x, size.y);
    _isFullscreen = enabled;
}

void SFMLWindow::handleResize(unsigned int newWidth, unsigned int newHeight) {
    constexpr float targetRatio = 1920.f / 1080.f;
    float windowRatio = static_cast<float>(newWidth) / static_cast<float>(newHeight);
    sf::View view(sf::FloatRect({0.f, 0.f}, {1920.f, 1080.f}));
    if (windowRatio > targetRatio) {
        float viewportWidth = targetRatio / windowRatio;
        view.setViewport(sf::FloatRect({(1.f - viewportWidth) / 2.f, 0.f}, {viewportWidth, 1.f}));
    }
    _window.setView(view);
}
```

**SDL2 Backend** : `src/client/lib/sdl2/src/SDL2Window.cpp`
- **Logical size 1920x1080** via `SDL_RenderSetLogicalSize()`
- Pas de support shader (retourne `false` pour `supportsShaders()`)
- Gestion DPI Windows (desactive le scaling)

**Fichier exemple : `src/client/lib/sdl2/src/SDL2Window.cpp:85-119`**
```cpp
SDL2Window::SDL2Window(Vec2u winSize, const std::string& name) {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");
    _window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(winSize.x), static_cast<int>(winSize.y),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(_renderer, 1920, 1080);  // Scaling automatique
}
```

### 1.3 Plugin Architecture

**Fichier : `src/client/include/graphics/IGraphicPlugin.hpp:19-39`**
```cpp
class IGraphicPlugin {
    virtual const char* getName() const = 0;
    virtual std::shared_ptr<IWindow> createWindow(Vec2u winSize, const std::string& name) = 0;
    virtual std::shared_ptr<core::IRenderer> createRenderer(std::shared_ptr<graphics::IWindow> window) = 0;
};
```

Les plugins (SFML/SDL2) sont charges dynamiquement avec `dlopen()` et exposent des symboles C :
```cpp
extern "C" {
    graphics::IGraphicPlugin* create() { return new sdl2::SDL2Plugin(); }
    void destroy(graphics::IGraphicPlugin* graphPlugin) { delete graphPlugin; }
}
```

---

## 2. SYSTEME DE SCENES : Stack-Based Navigation

### 2.1 Architecture IScene

**Fichier : `src/client/include/scenes/IScene.hpp:20-42`**
```cpp
struct GameContext {
    std::shared_ptr<graphics::IWindow> window;
    std::shared_ptr<client::network::UDPClient> udpClient;
    std::shared_ptr<client::network::TCPClient> tcpClient;
    std::string sessionToken;
};

class IScene {
public:
    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltatime) = 0;
    virtual void render() = 0;
    void setSceneManager(SceneManager* manager) { _sceneManager = manager; }
    void setContext(const GameContext& ctx) { _context = ctx; }
protected:
    SceneManager* _sceneManager = nullptr;
    GameContext _context;
};
```

### 2.2 SceneManager : Stack Navigation

**Fichier : `src/client/include/scenes/SceneManager.hpp:17-72`**
```cpp
class SceneManager {
public:
    void changeScene(std::unique_ptr<IScene> newScene);  // Remplace toute la pile
    void pushScene(std::unique_ptr<IScene> scene);       // Overlay (pause, menu)
    void popScene();                                     // Ferme overlay
    bool hasOverlay() const;
    size_t sceneCount() const;
    void handleEvent(const events::Event& event);
    void update(float deltatime);
    void render();
private:
    std::stack<std::unique_ptr<IScene>> _sceneStack;
    enum class PendingAction { None, Change, Push, Pop };
    PendingAction _pendingAction = PendingAction::None;
};
```

### 2.3 Diagramme Navigation Scenes

```
SCENE FLOW DIAGRAM

  LoginScene ─────────┐
                      ├──-> MainMenuScene
  (Auth)             |       |
                     |       ├──-> LobbyScene
                     |       |    (Ready/Start)
                     |       |       |
                     |       ├──-> RoomBrowserScene
                     |       ├──-> LeaderboardScene
                     |       ├──-> FriendsScene
                     |       └──-> SettingsScene
                     |            (Accessibility)
                     |
                     └──-> GameScene (Gameplay)
                           ├──-> PauseOverlay
                           └──-> PrivateChatScene
```

### 2.4 Implementations des Scenes Principales

| Scene | Fichier Header | Description |
|-------|---|---|
| **LoginScene** | `include/scenes/LoginScene.hpp` | Auth, register, version check, server config |
| **MainMenuScene** | `include/scenes/MainMenuScene.hpp` | Room creation/join, leaderboard, friends, settings |
| **LobbyScene** | `include/scenes/LobbyScene.hpp` | Waiting room, chat, ship skins, game speed (host) |
| **GameScene** | `include/scenes/GameScene.hpp` | Gameplay complet, HUD, chat, voice |
| **SettingsScene** | `include/scenes/SettingsScene.hpp` | Key remapping, colorblind, audio devices |
| **LeaderboardScene** | `include/scenes/LeaderboardScene.hpp` | Leaderboards, stats, achievements |
| **FriendsScene** | `include/scenes/FriendsScene.hpp` | Friends list, requests, blocking |
| **PrivateChatScene** | `include/scenes/PrivateChatScene.hpp` | Private messaging |
| **RoomBrowserScene** | `include/scenes/RoomBrowserScene.hpp` | Room listing + filtering |
| **BreakoutScene** | `include/scenes/BreakoutScene.hpp` | Game bonus breakout |
| **ConnectionScene** | `include/scenes/ConnectionScene.hpp` | Loading/transition |

---

## 3. SYSTEME D'EVENEMENTS : Variant-Based Dispatch

### 3.1 Event Types

**Fichier : `src/client/include/events/Event.hpp:17-78`**
```cpp
namespace events {
    enum class Key {
        A, B, C, ..., Z,  // Lettres
        Num0-9,           // Chiffres
        F1-F12,           // Fonction
        Space, Enter, Escape, Tab, Backspace,
        Up, Down, Left, Right,
        LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
        Unknown
    };

    enum class MouseButton { Left, Right, Middle, Unknown };

    struct KeyPressed { Key key; };
    struct KeyReleased { Key key; };
    struct MouseMoved { int x, y; };
    struct MouseButtonPressed { MouseButton button; int x, y; };
    struct MouseButtonReleased { MouseButton button; int x, y; };
    struct TextEntered { uint32_t unicode; };
    struct WindowClosed {};
    struct None {};

    using Event = std::variant<None, KeyPressed, KeyReleased, MouseMoved,
                              MouseButtonPressed, MouseButtonReleased,
                              TextEntered, WindowClosed>;
}
```

### 3.2 Conversion SFML -> Event

**Fichier : `src/client/lib/sfml/src/SFMLWindow.cpp:25-93, 119-170`**
```cpp
static events::Key scancodeToKey(sf::Keyboard::Scancode scancode) {
    switch (scancode) {
        case sf::Keyboard::Scancode::A: return events::Key::A;
        // ... 90+ mapping lines
    }
}

events::Event SFMLWindow::pollEvent() {
    if (auto ev = _window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            return events::WindowClosed{};
        }
        if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>()) {
            return events::KeyPressed{scancodeToKey(keyPressed->scancode)};
        }
        if (const auto* mousePressed = ev->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f worldPos = _window.mapPixelToCoords(mousePressed->position);
            return events::MouseButtonPressed{
                sfmlButtonToMouseButton(mousePressed->button),
                static_cast<int>(worldPos.x), static_cast<int>(worldPos.y)
            };
        }
        if (const auto* resized = ev->getIf<sf::Event::Resized>()) {
            handleResize(resized->size.x, resized->size.y);
            return pollEvent();  // Continue polling
        }
    }
    return events::None{};
}
```

---

## 4. SYSTEME UI : Component-Based Architecture

### 4.1 Hierarchie Composants

**Fichier : `src/client/include/ui/IUIElement.hpp:20-52`**
```cpp
class IUIElement {
public:
    virtual ~IUIElement() = default;
    virtual Vec2f getPos() const = 0;
    virtual void setPos(const Vec2f& pos) = 0;
    virtual Vec2f getSize() const = 0;
    virtual void setSize(const Vec2f& size) = 0;
    virtual bool contains(float x, float y) const {
        Vec2f pos = getPos();
        Vec2f size = getSize();
        return x >= pos.x && x <= pos.x + size.x &&
               y >= pos.y && y <= pos.y + size.y;
    }
    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(graphics::IWindow& window) = 0;
    virtual bool isFocused() const = 0;
    virtual void setFocused(bool focused) = 0;
    virtual bool isVisible() const { return _visible; }
    virtual bool isEnabled() const { return _enabled; }
protected:
    bool _visible = true;
    bool _enabled = true;
};
```

### 4.2 Button Component

**Fichier : `src/client/include/ui/Button.hpp:20-78`**
```cpp
class Button : public IUIElement {
public:
    enum class State { Normal, Hovered, Pressed, Disabled };
    using ClickCallback = std::function<void()>;

    Button(const Vec2f& pos, const Vec2f& size, const std::string& text,
           const std::string& fontKey);

    void setOnClick(ClickCallback callback) { _onClick = std::move(callback); }
    void setText(const std::string& text) { _text = text; }
    void setNormalColor(rgba color) { _normalColor = color; }
    void setHoveredColor(rgba color) { _hoveredColor = color; }
    void setPressedColor(rgba color) { _pressedColor = color; }
private:
    Vec2f _pos, _size;
    State _state = State::Normal;
    ClickCallback _onClick;
    rgba _normalColor{60, 60, 80, 255};
    rgba _hoveredColor{80, 80, 110, 255};
    rgba _pressedColor{40, 40, 60, 255};
};
```

### 4.3 TextInput Component

**Fichier : `src/client/include/ui/TextInput.hpp:20-88`**
```cpp
class TextInput : public IUIElement {
public:
    using OnChangeCallback = std::function<void(const std::string&)>;
    using OnSubmitCallback = std::function<void(const std::string&)>;

    TextInput(const Vec2f& pos, const Vec2f& size, const std::string& placeholder,
              const std::string& fontKey, bool isPassword = false);

    const std::string& getText() const { return _text; }
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder) { _placeholder = placeholder; }
    void setMaxLength(size_t len) { _maxLength = len; }
    void setPassword(bool isPassword) { _isPassword = isPassword; }
    void setOnChange(OnChangeCallback callback) { _onChange = std::move(callback); }
    void setOnSubmit(OnSubmitCallback callback) { _onSubmit = std::move(callback); }

private:
    std::string _text;
    std::string _placeholder;
    size_t _cursorPos = 0;
    float _cursorBlinkTimer = 0.0f;
    bool _showCursor = true;
    static constexpr float CURSOR_BLINK_RATE = 0.5f;
};
```

---

## 5. ACCESSIBILITE : Multi-Modal Support

### 5.1 AccessibilityConfig

**Fichier : `src/client/include/accessibility/AccessibilityConfig.hpp:22-131`**

```cpp
enum class ColorBlindMode {
    None,
    Protanopia,      // Red-blind
    Deuteranopia,    // Green-blind
    Tritanopia,      // Blue-yellow blind
    HighContrast
};

enum class GameAction {
    MoveUp, MoveDown, MoveLeft, MoveRight,
    Shoot, Pause, PushToTalk,
    WeaponPrev, WeaponNext,  // Q, E
    OpenChat,                 // T
    ExpandChat,              // O
    ForceToggle,             // F
    ToggleControls,          // H
    ActionCount
};

class AccessibilityConfig {
public:
    static AccessibilityConfig& getInstance();

    void setKeyBinding(GameAction action, events::Key primaryKey,
                       events::Key secondaryKey = events::Key::Unknown);
    events::Key getPrimaryKey(GameAction action) const;
    events::Key getSecondaryKey(GameAction action) const;
    bool isActionKey(GameAction action, events::Key key) const;
    void resetKeyBindings();

    void setColorBlindMode(ColorBlindMode mode);
    ColorBlindMode getColorBlindMode() const;

    void setGameSpeedMultiplier(float multiplier);
    float getGameSpeedMultiplier() const;

    void setShipSkin(uint8_t skinId);
    uint8_t getShipSkin() const;

    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

private:
    std::array<std::array<events::Key, 2>, static_cast<size_t>(GameAction::ActionCount)> _keyBindings;
    ColorBlindMode _colorBlindMode = ColorBlindMode::None;
    float _gameSpeedMultiplier = 1.0f;
    uint8_t _shipSkin = 1;
    bool _keepChatOpenAfterSend = false;
};
```

### 5.2 ColorblindShaderManager

**Fichier : `src/client/include/accessibility/ColorblindShaderManager.hpp:17-60`**

```cpp
class ColorblindShaderManager {
public:
    static ColorblindShaderManager& getInstance();

    bool initialize(std::shared_ptr<graphics::IWindow> window);
    void updateFromConfig();
    bool isAvailable() const;
    void shutdown();

private:
    std::shared_ptr<graphics::IWindow> _window;
    bool _initialized = false;
    bool _shadersAvailable = false;
    ColorBlindMode _currentMode = ColorBlindMode::None;

    static constexpr const char* SHADER_KEY = "colorblind";
    static constexpr const char* VERTEX_SHADER_PATH = "assets/shaders/colorblind.vert";
    static constexpr const char* FRAGMENT_SHADER_PATH = "assets/shaders/colorblind.frag";
};
```

---

## 6. CONVENTIONS DE CODE C++

### 6.1 Nommage

| Categorie | Convention | Exemple | Fichier:Ligne |
|-----------|-----------|---------|---------------|
| **Classes** | PascalCase | `class GameScene`, `class Button` | `GameScene.hpp:26`, `Button.hpp:20` |
| **Interfaces** | Prefixe I + PascalCase | `class IWindow`, `class IScene`, `class IUIElement` | `IWindow.hpp:20`, `IScene.hpp:27`, `IUIElement.hpp:20` |
| **Enums** | PascalCase (Type) + PascalCase (Value) | `enum class State { Normal, Hovered }` | `Button.hpp:22-27` |
| **Fonctions** | camelCase | `void handleEvent()`, `void renderBackground()` | `GameScene.hpp:43-44` |
| **Methodes privees** | camelCase | `void loadAssets()`, `void initUI()` | `GameScene.hpp:74` |
| **Variables membres** | _prefixWithUnderscore | `Vec2f _pos`, `rgba _normalColor` | `Button.hpp:59-70` |
| **Constantes** | SCREAMING_SNAKE_CASE | `static constexpr float MOVE_SPEED = 200.0f` | `GameScene.hpp:121` |
| **Namespaces** | snake_case | `namespace graphics`, `namespace ui`, `namespace events` | Multiple |

### 6.2 Formatage et Style

**Exemple 1 : Class Constructor Initializer List**
**Fichier : `src/client/src/ui/Button.cpp:13-22`**
```cpp
Button::Button(const Vec2f& pos, const Vec2f& size, const std::string& text,
               const std::string& fontKey)
    : _pos(pos)
    , _size(size)
    , _text(text)
    , _fontKey(fontKey)
    , _state(State::Normal)
    , _focused(false)
{
}
```
- Initialiseurs alignes verticalement
- Chaque initialiseur sur sa ligne avec virgule en tete

**Exemple 2 : Method Implementation avec Variant**
**Fichier : `src/client/src/ui/Button.cpp:24-73`**
```cpp
void Button::handleEvent(const events::Event& event) {
    if (!_enabled) {
        _state = State::Disabled;
        return;
    }

    if (auto* moved = std::get_if<events::MouseMoved>(&event)) {
        bool isHovered = contains(static_cast<float>(moved->x),
                                 static_cast<float>(moved->y));
        if (_state == State::Pressed) {
            // Keep pressed state while mouse button is down
        } else if (isHovered) {
            _state = State::Hovered;
        } else {
            _state = State::Normal;
        }
    }

    if (auto* pressed = std::get_if<events::MouseButtonPressed>(&event)) {
        if (pressed->button == events::MouseButton::Left &&
            contains(static_cast<float>(pressed->x), static_cast<float>(pressed->y))) {
            _state = State::Pressed;
        }
    }
}
```
- Utilise `std::get_if<>()` pour variant dispatch
- Cast explicites avec `static_cast<>`
- Commentaires pour les branches logiques critiques

**Exemple 3 : constexpr et Constants**
**Fichier : `src/client/include/scenes/GameScene.hpp:121-174`**
```cpp
static constexpr float MOVE_SPEED = 200.0f;
static constexpr float SHIP_WIDTH = 64.0f;
static constexpr float SHIP_HEIGHT = 30.0f;
static constexpr float HUD_HEALTH_BAR_WIDTH = 200.0f;
static constexpr float HUD_MARGIN = 20.0f;
static constexpr uint8_t MAX_HEALTH = 100;
static constexpr float SHOOT_COOLDOWN_TIME = 0.3f;
static constexpr const char* SHIP_TEXTURE_KEY = "ship";
static constexpr const char* FONT_KEY = "main";
static constexpr unsigned int LABEL_FONT_SIZE = 14;
static constexpr float SCREEN_WIDTH = 1920.0f;
static constexpr float SCREEN_HEIGHT = 1080.0f;
```

### 6.3 Headers Structure

**Pattern Standard : `src/client/include/ui/Button.hpp`**
```cpp
/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Button
*/

#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include "IUIElement.hpp"
#include "../graphics/IWindow.hpp"
#include "../events/Event.hpp"
#include "../utils/Vecs.hpp"
#include <string>
#include <functional>

namespace ui {

class Button : public IUIElement {
    // ... declaration
};

} // namespace ui

#endif /* !BUTTON_HPP_ */
```

---

## 7. USER EXPERIENCE : Feedback Visuel et Navigation

### 7.1 GameScene HUD System

**Fichier : `src/client/include/scenes/GameScene.hpp:47-73`**
```cpp
private:
    void renderBackground();
    void renderHUD();
    void renderPlayers();
    void renderMissiles();
    void renderEnemies();
    void renderEnemyMissiles();
    void renderDeathScreen();
    void renderKickedScreen();
    void renderChatOverlay();
    void renderVoiceIndicator();
    void renderScoreHUD();              // Score, combo, wave info
    void renderTeamScoreboard();        // All players' scores (multiplayer)
    void renderWeaponHUD();             // Current weapon indicator
    void renderBoss();                  // Boss sprite
    void renderBossHealthBar();         // Boss HP bar
    void renderWaveCannons();           // Wave Cannon beams
    void renderPowerUps();              // Power-up items
    void renderForcePods();             // Force pods
    void renderBitDevices();            // Bit devices (orbiting)
    void renderChargeGauge();           // Wave Cannon charge indicator
    void renderSpeedIndicator();        // Speed upgrade level
    void renderControlsHUD();           // Controls help (bottom-right)
    void renderPauseOverlay();          // Pause menu overlay
    void renderGlobalRank();            // Rank badge and best score
```

### 7.2 Chat Overlay System

**Fichier : `src/client/include/scenes/GameScene.hpp:176-191`**
```cpp
struct ChatDisplayMessage {
    std::string displayName;
    std::string message;
    float displayTime;      // Time remaining (0 = expired/archived)
    bool expired = false;   // True when timer reached 0
};
std::vector<ChatDisplayMessage> _chatDisplayMessages;
std::unique_ptr<ui::TextInput> _chatInput;
bool _chatInputOpen = false;
bool _chatExpanded = false;  // Show all messages when expanded

static constexpr float CHAT_MESSAGE_DISPLAY_TIME = 8.0f;
static constexpr size_t MAX_CHAT_DISPLAY_MESSAGES = 20;
static constexpr size_t ALWAYS_VISIBLE_MESSAGES = 3;  // Last N never expire
```

### 7.3 Fullscreen & Letterboxing

**Implementation SFML** - Maintient aspect ratio 16:9 avec barres noires :
```cpp
// Viewport adjustment pour letterboxing
if (windowRatio > targetRatio) {
    // Fenetre plus large -> barres noires sur les cotes
    float viewportWidth = targetRatio / windowRatio;
    view.setViewport(sf::FloatRect({(1.f - viewportWidth) / 2.f, 0.f},
                                   {viewportWidth, 1.f}));
} else {
    // Fenetre plus haute -> barres noires haut/bas
    float viewportHeight = windowRatio / targetRatio;
    view.setViewport(sf::FloatRect({0.f, (1.f - viewportHeight) / 2.f},
                                   {1.f, viewportHeight}));
}
```

---

## RESUME : Interfaces Utilisateur R-Type

| Aspect | Implementation | Fichier Cle |
|--------|---|---|
| **Abstraction graphique** | Plugin architecture + IWindow | `IGraphicPlugin.hpp`, `IWindow.hpp` |
| **Backends** | SFML + SDL2 | `SFMLWindow.cpp`, `SDL2Window.cpp` |
| **Navigation scenes** | Stack-based scene manager | `SceneManager.hpp/cpp` |
| **Evenements** | Variant-based dispatch | `Event.hpp` |
| **UI Components** | IUIElement (Button, TextInput) | `IUIElement.hpp`, `Button.hpp` |
| **Accessibilite** | Key remapping, colorblind, game speed | `AccessibilityConfig.hpp` |
| **Fullscreen** | Letterboxing 16:9 | `SFMLWindow.cpp:336-384` |
| **Chat System** | In-game overlay avec timer | `GameScene.hpp:176-191` |
| **State Machines** | Enum-based (Button states, etc) | `Button.hpp:22-27` |
| **Persistence** | Settings files + MongoDB | `AccessibilityConfig.hpp` |

---

## CONCLUSION

Le client R-Type implemente des interfaces utilisateur **professionnelles et accessibles** avec:
- Abstraction multi-backend (SFML/SDL2) via plugin system
- Navigation par pile de scenes
- Systeme d'evenements type-safe via std::variant
- Composants UI reutilisables (Button, TextInput)
- Support accessibilite complet (colorblind, key remapping)
- Conventions de code C++ coherentes et lisibles
