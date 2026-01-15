---
tags:
  - configuration
  - accessibilite
---

# Accessibilité

## Stockage

Les paramètres d'accessibilité sont stockés **côté serveur** (MongoDB) et synchronisés à la connexion.

| Paramètre | Champ MongoDB | Type |
|-----------|---------------|------|
| Mode couleur | `colorBlindMode` | string (16 chars) |
| Keybindings | `keyBindings` | uint8[14] |
| Ship skin | `shipSkin` | uint8 (1-6) |
| Game speed | `gameSpeedPercent` | uint16 (50-200) |

---

## Modes Daltonien (5 modes)

| Mode | Enum | Description |
|------|------|-------------|
| `none` | 0 | Couleurs d'origine |
| `protanopia` | 1 | Déficience rouge |
| `deuteranopia` | 2 | Déficience vert |
| `tritanopia` | 3 | Déficience bleu |
| `highcontrast` | 4 | Contraste élevé |

Source : `AccessibilityConfig.hpp`

### Implémentation selon le Backend

!!! warning "Différence SFML / SDL2"
    L'implémentation du mode daltonien **diffère selon le backend graphique**.

#### SFML : Shaders Post-Processing (vraie simulation)

SFML applique un **shader GLSL** qui transforme **toute l'image** via des matrices de daltonisme :

```cpp
// Activation du shader
_window->setPostProcessShader("colorblind");
_window->setShaderUniform("colorBlindMode", static_cast<int>(mode));

// Le shader applique une transformation matricielle sur chaque pixel
// Ex: mat3 DEUTERANOPIA transforme RGB pour simuler la vision
```

Les shaders sont dans `assets/shaders/colorblind.frag`.

#### SDL2 : Palette de Couleurs (approximation)

SDL2 **ne supporte pas les shaders**. À la place, `AccessibilityConfig` fournit des **couleurs distinctives** pour chaque élément du jeu :

```cpp
// AccessibilityConfig.cpp - Palette pour protanopia/deuteranopia
case ColorBlindMode::Deuteranopia:
    _playerColor        = {100, 100, 255, 255};  // Bleu
    _enemyColor         = {180, 100, 255, 255};  // Violet
    _playerMissileColor = {100, 255, 255, 255};  // Cyan
    _enemyMissileColor  = {255, 150, 255, 255};  // Rose
    break;
```

#### Comparaison

| Backend | Méthode | Scope | Qualité |
|---------|---------|-------|---------|
| **SFML** | Shader post-processing | Toute l'image | Simulation fidèle |
| **SDL2** | Palette `AccessibilityConfig` | Éléments UI/HUD | Couleurs distinctes |

---

## Rebinding (7 actions)

| Action | Enum | Touches par défaut |
|--------|------|-------------------|
| Haut | `MoveUp` | Z / ↑ |
| Bas | `MoveDown` | S / ↓ |
| Gauche | `MoveLeft` | Q / ← |
| Droite | `MoveRight` | D / → |
| Tir | `Shoot` | Space |
| Pause | `Pause` | Escape |
| Push-to-Talk | `PushToTalk` | V |

### Stockage

Chaque action a 2 touches (primaire + secondaire).
Format MongoDB : tableau de 14 bytes (7 actions × 2 touches).

```cpp
// AccessibilityConfig.hpp
std::array<std::array<events::Key, 2>, 7> _keyBindings;
```

### Modification

**In-Game** : Menu Options → Contrôles

---

## Ship Skins (6 variantes)

| Skin ID | Fichier |
|---------|---------|
| 1 | `Ship1.png` |
| 2 | `Ship2.png` |
| 3 | `Ship3.png` |
| 4 | `Ship4.png` |
| 5 | `Ship5.png` |
| 6 | `Ship6.png` |

Modifiable dans **Options → Personnalisation**.

---

## Vitesse de Jeu

Pour l'accessibilité cognitive, la vitesse du jeu peut être ajustée.

| Valeur | Multiplicateur |
|--------|----------------|
| 50 | 0.5× (lent) |
| 100 | 1.0× (normal) |
| 150 | 1.5× (rapide) |
| 200 | 2.0× (max) |

Configurable par le **host** de la room avant le lancement de la partie.
