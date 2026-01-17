# Audit Complet : Fullscreen et Resize

## Date : 15 Janvier 2026
## Auteur : Alexandre
## Branche : fullscreen-resize
## Assigné : Erwan

---

> **Note pour Erwan** : Ce document explique le problème et les solutions. La Phase 1 (fullscreen desktop) est prioritaire et devrait prendre ~2-4h. Le code à modifier est détaillé en section 6.

---

## 1. Problème Identifié

### 1.1 Description du Bug

Sur **Ubuntu** (et potentiellement d'autres distributions Linux avec barre système/dock), la fenêtre de jeu 1920×1080 est trop haute pour l'écran disponible. Le bas de la fenêtre passe "sous" l'écran :

- Les premiers messages du chat sont coupés/invisibles
- Le bas des contrôles est masqué
- L'input de chat peut être inaccessible

### 1.2 Hack Actuel

Un `UBUNTU_OFFSET` a été introduit comme contournement temporaire :

```cpp
// GameScene.cpp - Lignes 1518-1524
constexpr float UBUNTU_OFFSET = 30.0f;  // Pour les contrôles
constexpr float UBUNTU_OFFSET = 70.0f;  // Pour le chat (différentes valeurs !)

float hudY = SCREEN_HEIGHT - hudHeight - margin - UBUNTU_OFFSET;
```

**Problèmes avec cette approche :**
- Valeurs hardcodées différentes (30px, 70px)
- Décale l'UI sur Windows où ce n'est pas nécessaire
- Ne résout pas le problème fondamental
- Non maintenable

### 1.3 Environnements Affectés

| OS | Problème | Cause |
|----|----------|-------|
| Ubuntu (GNOME) | Oui | Barre système haut + dock bas |
| Ubuntu (KDE) | Potentiel | Panneau configurable |
| Windows 10/11 | Non | Taskbar ne chevauche pas |
| macOS | Potentiel | Menu bar + dock |

---

## 2. Analyse du Code Actuel

### 2.1 Architecture de Fenêtrage

```
IWindow (interface)
├── SFMLWindow
│   └── sf::RenderWindow
└── SDL2Window
    └── SDL_Window + SDL_Renderer
```

### 2.2 Création de Fenêtre

**SFML (SFMLWindow.cpp:83-85)**
```cpp
SFMLWindow::SFMLWindow(Vec2u winSize, const std::string& name) {
    _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
    // Pas de style spécifié = Default (titlebar + resize + close)
    // Pas de gestion fullscreen
}
```

**SDL2 (SDL2Window.cpp:84-91)**
```cpp
_window = SDL_CreateWindow(
    name.c_str(),
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    static_cast<int>(winSize.x),
    static_cast<int>(winSize.y),
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE  // Resize activé mais non géré !
);
```

### 2.3 Problèmes Identifiés

| Problème | Fichier | Impact |
|----------|---------|--------|
| Pas d'événement `WindowResized` | Event.hpp | Impossible de réagir au resize |
| `SCREEN_WIDTH/HEIGHT` hardcodés | GameScene.hpp:163-164 | UI non responsive |
| Positions absolues partout | Toutes les scènes | Pas de scaling |
| SDL2 accepte resize mais l'ignore | SDL2Window.cpp:90 | Comportement incohérent |
| SFML ignore événement Resized | SFMLWindow.cpp:138 | Passe en récursion sur pollEvent |

### 2.4 Constantes Hardcodées

```cpp
// GameScene.hpp
static constexpr float SCREEN_WIDTH = 1920.0f;
static constexpr float SCREEN_HEIGHT = 1080.0f;

// Toutes les scènes utilisent ces constantes
float centerX = SCREEN_WIDTH / 2.0f;
float boxX = SCREEN_WIDTH / 2 - 400;
// etc.
```

**Comptage des références :**
- `SCREEN_WIDTH` : ~150+ utilisations
- `SCREEN_HEIGHT` : ~120+ utilisations
- Positions absolues : ~500+ lignes affectées

---

## 3. Solutions Proposées

### 3.1 Architecture Cible

```
┌─────────────────────────────────────────────────────────┐
│                     Application                          │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────────────────┐   │
│  │  WindowManager  │  │      UIScaler               │   │
│  │  - fullscreen   │  │  - reference: 1920x1080     │   │
│  │  - resize       │  │  - current: dynamic         │   │
│  │  - getViewport  │  │  - getScale(): Vec2f        │   │
│  └────────┬────────┘  │  - scalePos(): Vec2f        │   │
│           │           │  - scaleSize(): Vec2f       │   │
│           ▼           └─────────────────────────────┘   │
│  ┌─────────────────┐                                    │
│  │    IWindow      │◄─── WindowResized event            │
│  │  + setFullscreen│                                    │
│  │  + toggleFull.. │                                    │
│  └─────────────────┘                                    │
└─────────────────────────────────────────────────────────┘
```

### 3.2 Option A : Fullscreen Desktop (Recommandé)

**Principe :** Utiliser le mode "fullscreen desktop" (borderless fullscreen) qui prend toute la résolution de l'écran sans changer le mode vidéo.

**Avantages :**
- Multitâche fluide (Alt+Tab rapide)
- Pas de changement de résolution système
- Compatible avec tous les moniteurs
- Scaling automatique possible

**SFML :**
```cpp
// Fullscreen desktop
_window.create(sf::VideoMode::getDesktopMode(), name, sf::Style::None);
_window.setPosition({0, 0});

// Ou vrai fullscreen
auto modes = sf::VideoMode::getFullscreenModes();
_window.create(modes.front(), name, sf::Style::Fullscreen);
```

**SDL2 :**
```cpp
// Fullscreen desktop (recommandé)
SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);

// Ou vrai fullscreen
SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN);
```

### 3.3 Option B : Fenêtre Redimensionnable avec Scaling

**Principe :** Garder une résolution de référence (1920×1080) et scaler tout le rendu.

**Implémentation :**

1. **Ajouter événement WindowResized**
```cpp
// Event.hpp
struct WindowResized {
    unsigned int width;
    unsigned int height;
};

using Event = std::variant<
    // ...
    WindowResized,
    WindowClosed
>;
```

2. **Gérer l'événement dans les backends**
```cpp
// SFMLWindow.cpp
if (const auto* resized = ev->getIf<sf::Event::Resized>()) {
    // Mettre à jour la view pour maintenir le ratio
    sf::FloatRect visibleArea({0, 0}, {1920, 1080});
    _window.setView(sf::View(visibleArea));
    return events::WindowResized{resized->size.x, resized->size.y};
}
```

3. **UIScaler pour coordonnées**
```cpp
class UIScaler {
public:
    static constexpr float REF_WIDTH = 1920.0f;
    static constexpr float REF_HEIGHT = 1080.0f;

    void updateSize(unsigned int width, unsigned int height) {
        _scaleX = width / REF_WIDTH;
        _scaleY = height / REF_HEIGHT;
        _scale = std::min(_scaleX, _scaleY);  // Letterbox
    }

    float scaleX(float x) const { return x * _scale + _offsetX; }
    float scaleY(float y) const { return y * _scale + _offsetY; }
    float scaleW(float w) const { return w * _scale; }
    float scaleH(float h) const { return h * _scale; }

private:
    float _scaleX = 1.0f, _scaleY = 1.0f, _scale = 1.0f;
    float _offsetX = 0.0f, _offsetY = 0.0f;  // Pour letterbox
};
```

### 3.4 Option C : Ancrage Relatif (UI Responsive)

**Principe :** Positionner les éléments UI relativement aux bords de l'écran.

```cpp
enum class Anchor { TopLeft, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };

struct UIElement {
    Anchor anchor;
    Vec2f offset;  // Depuis l'ancre
    Vec2f size;
};

// Exemple : Score en haut à droite
UIElement scoreBox {
    .anchor = Anchor::TopRight,
    .offset = {-280.0f, 15.0f},
    .size = {270.0f, 75.0f}
};
```

---

## 4. Comparaison des Solutions

| Critère | Option A (Fullscreen) | Option B (Scaling) | Option C (Responsive) |
|---------|----------------------|--------------------|-----------------------|
| Complexité | Faible | Moyenne | Élevée |
| Temps dev | ~2-4h | ~8-16h | ~20-40h |
| Fichiers modifiés | ~5 | ~15 | ~30+ |
| Résout Ubuntu | ✅ Oui | ✅ Oui | ✅ Oui |
| Multi-résolution | ❌ Non | ✅ Oui | ✅ Oui |
| Ratio d'aspect | Fixe | Letterbox | Flexible |
| Breaking changes | Minimes | Modérés | Importants |

---

## 5. Recommandation

### Phase 1 : Fullscreen Desktop (Priorité Haute)

Implémenter le mode fullscreen desktop pour résoudre immédiatement le problème Ubuntu :

1. **Ajouter à IWindow :**
   - `setFullscreen(bool enabled)`
   - `toggleFullscreen()`
   - `isFullscreen() const`

2. **Implémenter dans SFMLWindow et SDL2Window**

3. **Ajouter touche F11** pour toggle fullscreen

4. **Option dans Settings** pour choisir le mode au démarrage

### Phase 2 : View Scaling (Priorité Moyenne)

Utiliser la View SFML / Logical Size SDL pour scaler automatiquement :

```cpp
// SFML - Garder la même view peu importe la taille fenêtre
sf::View view({0, 0, 1920, 1080});
_window.setView(view);

// SDL2 - Logical size
SDL_RenderSetLogicalSize(_renderer, 1920, 1080);
```

### Phase 3 : UI Responsive (Optionnel)

Si besoin de supporter des ratios d'aspect très différents (16:9, 21:9, 4:3), implémenter l'ancrage relatif.

---

## 6. Plan d'Implémentation Détaillé

### 6.1 Fichiers à Modifier

| Fichier | Modifications |
|---------|---------------|
| `IWindow.hpp` | +3 méthodes fullscreen |
| `SFMLWindow.hpp/cpp` | Implémentation SFML |
| `SDL2Window.hpp/cpp` | Implémentation SDL2 |
| `Event.hpp` | +WindowResized event (optionnel) |
| `GameScene.cpp` | Supprimer UBUNTU_OFFSET, ajouter F11 |
| `SettingsScene.cpp` | Option fullscreen |

### 6.2 Méthodes à Ajouter à IWindow

```cpp
class IWindow {
public:
    // ... existant ...

    // Fullscreen support
    virtual void setFullscreen(bool enabled) = 0;
    virtual void toggleFullscreen() = 0;
    virtual bool isFullscreen() const = 0;

    // Optionnel : View/Logical size
    virtual void setLogicalSize(unsigned int width, unsigned int height) = 0;
};
```

### 6.3 Implémentation SFML

```cpp
void SFMLWindow::setFullscreen(bool enabled) {
    auto size = _window.getSize();
    std::string title = "R-Type";  // Ou stocker le titre

    if (enabled) {
        _window.create(sf::VideoMode::getDesktopMode(), title, sf::Style::None);
    } else {
        _window.create(sf::VideoMode({1920, 1080}), title, sf::Style::Default);
    }

    // Maintenir la view de référence
    sf::View view({0, 0, 1920, 1080});
    _window.setView(view);

    _isFullscreen = enabled;
}

void SFMLWindow::toggleFullscreen() {
    setFullscreen(!_isFullscreen);
}

bool SFMLWindow::isFullscreen() const {
    return _isFullscreen;
}
```

### 6.4 Implémentation SDL2

```cpp
void SDL2Window::setFullscreen(bool enabled) {
    if (enabled) {
        SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(_window, 0);
        SDL_SetWindowSize(_window, 1920, 1080);
        SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // Maintenir le logical size
    SDL_RenderSetLogicalSize(_renderer, 1920, 1080);

    _isFullscreen = enabled;
}
```

---

## 7. Tests à Effectuer

### 7.1 Tests Manuels

| Test | Ubuntu | Windows | macOS |
|------|--------|---------|-------|
| Démarrage en fenêtré | ⬜ | ⬜ | ⬜ |
| F11 → Fullscreen | ⬜ | ⬜ | ⬜ |
| F11 → Fenêtré | ⬜ | ⬜ | ⬜ |
| Alt+Tab en fullscreen | ⬜ | ⬜ | ⬜ |
| UI lisible en fullscreen | ⬜ | ⬜ | ⬜ |
| Chat visible | ⬜ | ⬜ | ⬜ |
| Contrôles accessibles | ⬜ | ⬜ | ⬜ |
| Clics souris corrects | ⬜ | ⬜ | ⬜ |

### 7.2 Tests Automatisés

```cpp
TEST(WindowTest, FullscreenToggle) {
    auto window = createWindow();
    EXPECT_FALSE(window->isFullscreen());

    window->setFullscreen(true);
    EXPECT_TRUE(window->isFullscreen());

    window->toggleFullscreen();
    EXPECT_FALSE(window->isFullscreen());
}
```

---

## 8. Risques et Mitigations

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Regression clics souris | Moyenne | Élevé | Tests intensifs multi-OS |
| Performance fullscreen | Faible | Moyen | Utiliser desktop fullscreen |
| Textures/shaders cassés | Faible | Élevé | Réinitialiser RenderTexture |
| Multi-écran | Moyenne | Faible | Documenter limitation |

---

## 9. Références

### Documentation Officielle
- [SFML Window Tutorial](https://www.sfml-dev.org/tutorials/3.0/window/window/)
- [SDL2 Fullscreen Tutorial](https://www.studyplan.dev/sdl2/sdl2-fullscreen)
- [SDL2 SetWindowFullscreen](https://wiki.libsdl.org/SDL2/SDL_SetWindowFullscreen)

### Articles et Discussions
- [Unity UI Multi-Resolution](https://docs.unity3d.com/Packages/com.unity.ugui@1.0/manual/HOWTO-UIMultiResolution.html)
- [UI Scaling In Games (UX Collective)](https://uxdesign.cc/how-to-handle-ui-scaling-in-games-3bdfe70bc7ff)
- [SFML Fullscreen Issues (GitHub)](https://github.com/SFML/SFML/issues/893)

### Issues Connues
- SFML : Pas d'événement resize en fullscreen
- SDL2 : DPI scaling sur Windows peut causer des problèmes
- Linux : Comportement variable selon le Window Manager

---

## 10. Conclusion

Le problème de fenêtre coupée sur Ubuntu est un symptôme d'un manque de support fullscreen/resize dans le client R-Type. La solution recommandée est d'implémenter le **fullscreen desktop** en Phase 1, ce qui :

1. Résout immédiatement le problème Ubuntu
2. Offre une meilleure expérience utilisateur
3. Nécessite peu de modifications
4. N'introduit pas de breaking changes majeurs

Le hack `UBUNTU_OFFSET` pourra ensuite être supprimé une fois le fullscreen implémenté.

---

*Document généré pour la branche fullscreen-resize du projet R-Type*
