/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AccessibilityConfig - Game accessibility settings
*/

#ifndef ACCESSIBILITYCONFIG_HPP_
#define ACCESSIBILITYCONFIG_HPP_

#include "events/Event.hpp"
#include <string>
#include <unordered_map>
#include <array>
#include <cstdint>

namespace accessibility {

/**
 * @brief Color vision deficiency modes
 */
enum class ColorBlindMode {
    None,
    Protanopia,
    Deuteranopia,
    Tritanopia,
    HighContrast
};

/**
 * @brief Game actions that can be remapped
 */
enum class GameAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    Pause,
    ActionCount
};

/**
 * @brief RGBA color structure for accessibility colors
 */
struct Color {
    uint8_t r, g, b, a;
};

/**
 * @brief Accessibility configuration manager
 *
 * Provides:
 * - Key remapping for all game actions
 * - Colorblind-friendly color palettes
 * - High contrast mode
 * - Configurable game speed (for cognitive accessibility)
 */
class AccessibilityConfig {
public:
    static AccessibilityConfig& getInstance();

    void setKeyBinding(GameAction action, events::Key primaryKey, events::Key secondaryKey = events::Key::Unknown);
    events::Key getPrimaryKey(GameAction action) const;
    events::Key getSecondaryKey(GameAction action) const;
    bool isActionKey(GameAction action, events::Key key) const;
    void resetKeyBindings();

    void setColorBlindMode(ColorBlindMode mode);
    ColorBlindMode getColorBlindMode() const;

    Color getPlayerColor() const;
    Color getOtherPlayerColor() const;
    Color getEnemyColor() const;
    Color getPlayerMissileColor() const;
    Color getEnemyMissileColor() const;
    Color getHealthHighColor() const;
    Color getHealthMediumColor() const;
    Color getHealthLowColor() const;

    void setGameSpeedMultiplier(float multiplier);
    float getGameSpeedMultiplier() const;

    void setShipSkin(uint8_t skinId);
    uint8_t getShipSkin() const;

    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

    static std::string keyToString(events::Key key);
    static events::Key stringToKey(const std::string& str);
    static std::string colorBlindModeToString(ColorBlindMode mode);
    static ColorBlindMode stringToColorBlindMode(const std::string& str);

private:
    AccessibilityConfig();
    ~AccessibilityConfig() = default;
    AccessibilityConfig(const AccessibilityConfig&) = delete;
    AccessibilityConfig& operator=(const AccessibilityConfig&) = delete;

    void initDefaultColors();
    void updateColorsForMode();

    std::array<std::array<events::Key, 2>, static_cast<size_t>(GameAction::ActionCount)> _keyBindings;

    ColorBlindMode _colorBlindMode = ColorBlindMode::None;
    float _gameSpeedMultiplier = 1.0f;
    uint8_t _shipSkin = 1;  // Ship skin variant (1-6 for Ship1.png to Ship6.png)

    Color _playerColor;
    Color _otherPlayerColor;
    Color _enemyColor;
    Color _playerMissileColor;
    Color _enemyMissileColor;
    Color _healthHigh;
    Color _healthMedium;
    Color _healthLow;
};

}

#endif /* !ACCESSIBILITYCONFIG_HPP_ */
