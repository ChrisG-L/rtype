# Accessibility Features

R-Type includes accessibility features to ensure the game can be enjoyed by players with various needs. This guide explains how to configure and use these features.

## Overview

| Feature | Disability Type | Description |
|---------|-----------------|-------------|
| Key Remapping | Motor/Physical | Customize all control keys |
| Colorblind Modes | Visual | Alternative color palettes |
| High Contrast Mode | Visual (Low Vision) | Maximum visibility colors |
| Game Speed Control | Cognitive | Slow down or speed up gameplay |

## Configuration File

All accessibility settings are stored in `assets/accessibility.cfg`. Edit this file with any text editor to customize your experience.

### Location

```
rtype/
└── assets/
    └── accessibility.cfg
```

## Key Remapping

### Motor/Physical Accessibility

Players with limited mobility can remap all game controls to keys that are more comfortable to reach.

### Default Key Bindings

| Action | Primary Key | Alternative Key |
|--------|-------------|-----------------|
| Move Up | Arrow Up | Z |
| Move Down | Arrow Down | S |
| Move Left | Arrow Left | Q |
| Move Right | Arrow Right | D |
| Shoot | Space | Enter |

### Customizing Key Bindings

Edit `accessibility.cfg` and change the key names:

```ini
# Example: Use IJKL for movement (one-handed play)
move_up = I
move_up_alt = Up
move_down = K
move_down_alt = Down
move_left = J
move_left_alt = Left
move_right = L
move_right_alt = Right

# Example: Use left mouse area keys for shooting
shoot = A
shoot_alt = Space
```

### Available Key Names

```
Letters: A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
Numbers: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
Arrows: Up, Down, Left, Right
Special: Space, Enter, Escape, Tab, Backspace
Modifiers: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt
```

## Colorblind Modes

### Visual Accessibility

R-Type supports multiple colorblind-friendly palettes to ensure all game elements remain distinguishable.

### Available Modes

| Mode | Affects | Color Palette |
|------|---------|---------------|
| `none` | Default | Standard game colors |
| `protanopia` | Red-blind | Blue, Yellow, Purple, Cyan |
| `deuteranopia` | Green-blind | Blue, Yellow, Purple, Cyan |
| `tritanopia` | Blue-blind | Red, Green, Magenta |
| `high_contrast` | Low vision | Maximum contrast colors |

### Configuration

Edit `accessibility.cfg`:

```ini
# For red-green colorblindness
colorblind_mode = protanopia

# For blue-yellow colorblindness
colorblind_mode = tritanopia

# For maximum visibility
colorblind_mode = high_contrast
```

### Color Comparison

#### Normal Mode (none)
- Player: Blue
- Allies: Green
- Enemies: Red
- Player Missiles: Yellow
- Enemy Missiles: Light Red
- Health Bar: Green → Orange → Red

#### Protanopia/Deuteranopia Mode
- Player: Blue
- Allies: Yellow
- Enemies: Purple
- Player Missiles: Cyan
- Enemy Missiles: Pink
- Health Bar: Blue → Yellow → Purple

#### Tritanopia Mode
- Player: Green
- Allies: Red
- Enemies: Magenta
- Player Missiles: Light Green
- Enemy Missiles: Pink
- Health Bar: Green → Orange → Magenta

#### High Contrast Mode
- Player: Cyan
- Allies: Yellow
- Enemies: Magenta
- Player Missiles: White
- Enemy Missiles: Orange
- Health Bar: Bright Green → Bright Yellow → Bright Red

## Game Speed Control

### Cognitive Accessibility

Adjust the game speed to make gameplay easier to follow or more challenging.

### Configuration

```ini
# Slower gameplay (50% speed) - easier to react
game_speed = 0.5

# Normal speed
game_speed = 1.0

# Faster gameplay (150% speed) - more challenging
game_speed = 1.5
```

### Speed Range

| Value | Effect |
|-------|--------|
| 0.5 | Half speed (easier) |
| 0.75 | 75% speed |
| 1.0 | Normal speed |
| 1.25 | 125% speed |
| 1.5 | 150% speed |
| 2.0 | Double speed (harder) |

## Example Configurations

### One-Handed Play (Right Hand)

```ini
colorblind_mode = none
game_speed = 0.75

move_up = I
move_up_alt = Num8
move_down = K
move_down_alt = Num2
move_left = J
move_left_alt = Num4
move_right = L
move_right_alt = Num6
shoot = Space
shoot_alt = Num0
```

### Low Vision Setup

```ini
colorblind_mode = high_contrast
game_speed = 0.75

# Default keys (comfortable for most users)
move_up = Up
move_up_alt = W
move_down = Down
move_down_alt = S
move_left = Left
move_left_alt = A
move_right = Right
move_right_alt = D
shoot = Space
shoot_alt = Enter
```

### Red-Green Colorblind Gamer

```ini
colorblind_mode = deuteranopia
game_speed = 1.0

move_up = Up
move_up_alt = Z
move_down = Down
move_down_alt = S
move_left = Left
move_left_alt = Q
move_right = Right
move_right_alt = D
shoot = Space
shoot_alt = Enter
```

## Programmatic Access

### C++ API

For developers, the accessibility system is available through the `AccessibilityConfig` singleton:

```cpp
#include "accessibility/AccessibilityConfig.hpp"

// Get the singleton instance
auto& config = accessibility::AccessibilityConfig::getInstance();

// Load custom configuration
config.loadFromFile("assets/accessibility.cfg");

// Check if a key triggers an action
if (config.isActionKey(accessibility::GameAction::Shoot, key)) {
    // Handle shoot action
}

// Get colorblind-friendly colors
auto playerColor = config.getPlayerColor();
auto enemyColor = config.getEnemyColor();

// Set colorblind mode programmatically
config.setColorBlindMode(accessibility::ColorBlindMode::Protanopia);

// Adjust game speed
config.setGameSpeedMultiplier(0.75f);
```

### Available Game Actions

```cpp
enum class GameAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    Pause
};
```

### Available Color Methods

```cpp
Color getPlayerColor();        // Local player
Color getOtherPlayerColor();   // Allied players
Color getEnemyColor();         // Enemy ships
Color getPlayerMissileColor(); // Player projectiles
Color getEnemyMissileColor();  // Enemy projectiles
Color getHealthHighColor();    // Health > 60%
Color getHealthMediumColor();  // Health 30-60%
Color getHealthLowColor();     // Health < 30%
```

## Future Improvements

Planned accessibility features for future releases:

- [ ] Audio cues for visual events (screen reader support)
- [ ] Subtitles for audio events
- [ ] Auto-aim assist for motor impairments
- [ ] UI scaling options
- [ ] Custom color picker for individual elements
- [ ] Vibration/haptic feedback options
- [ ] Save/load multiple accessibility profiles

## Support

If you need additional accessibility features not currently supported, please open an issue on our GitHub repository describing your needs.

## References

- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/)
- [Game Accessibility Guidelines](https://gameaccessibilityguidelines.com/)
- [Xbox Accessibility Guidelines](https://docs.microsoft.com/gaming/accessibility)
