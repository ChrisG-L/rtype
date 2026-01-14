/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AccessibilityConfig - Implementation
*/

#include "accessibility/AccessibilityConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace accessibility {

AccessibilityConfig& AccessibilityConfig::getInstance()
{
    static AccessibilityConfig instance;
    return instance;
}

AccessibilityConfig::AccessibilityConfig()
{
    resetKeyBindings();
    initDefaultColors();
}

void AccessibilityConfig::resetKeyBindings()
{
    _keyBindings[static_cast<size_t>(GameAction::MoveUp)]     = {events::Key::Up, events::Key::Z};
    _keyBindings[static_cast<size_t>(GameAction::MoveDown)]   = {events::Key::Down, events::Key::S};
    _keyBindings[static_cast<size_t>(GameAction::MoveLeft)]   = {events::Key::Left, events::Key::A};
    _keyBindings[static_cast<size_t>(GameAction::MoveRight)]  = {events::Key::Right, events::Key::D};
    _keyBindings[static_cast<size_t>(GameAction::Shoot)]      = {events::Key::Space, events::Key::Enter};
    _keyBindings[static_cast<size_t>(GameAction::Pause)]      = {events::Key::Escape, events::Key::P};
    _keyBindings[static_cast<size_t>(GameAction::PushToTalk)] = {events::Key::V, events::Key::Unknown};
    // Gameplay Phase 2/3 - New configurable keys
    _keyBindings[static_cast<size_t>(GameAction::WeaponPrev)]     = {events::Key::Q, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(GameAction::WeaponNext)]     = {events::Key::E, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(GameAction::OpenChat)]       = {events::Key::T, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(GameAction::ExpandChat)]     = {events::Key::O, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(GameAction::ForceToggle)]    = {events::Key::F, events::Key::Unknown};
    _keyBindings[static_cast<size_t>(GameAction::ToggleControls)] = {events::Key::H, events::Key::Unknown};
}

void AccessibilityConfig::setKeyBinding(GameAction action, events::Key primaryKey, events::Key secondaryKey)
{
    size_t idx = static_cast<size_t>(action);
    if (idx < static_cast<size_t>(GameAction::ActionCount)) {
        _keyBindings[idx][0] = primaryKey;
        _keyBindings[idx][1] = secondaryKey;
    }
}

events::Key AccessibilityConfig::getPrimaryKey(GameAction action) const
{
    size_t idx = static_cast<size_t>(action);
    if (idx < static_cast<size_t>(GameAction::ActionCount)) {
        return _keyBindings[idx][0];
    }
    return events::Key::Unknown;
}

events::Key AccessibilityConfig::getSecondaryKey(GameAction action) const
{
    size_t idx = static_cast<size_t>(action);
    if (idx < static_cast<size_t>(GameAction::ActionCount)) {
        return _keyBindings[idx][1];
    }
    return events::Key::Unknown;
}

bool AccessibilityConfig::isActionKey(GameAction action, events::Key key) const
{
    size_t idx = static_cast<size_t>(action);
    if (idx < static_cast<size_t>(GameAction::ActionCount)) {
        return _keyBindings[idx][0] == key || _keyBindings[idx][1] == key;
    }
    return false;
}

void AccessibilityConfig::setColorBlindMode(ColorBlindMode mode)
{
    _colorBlindMode = mode;
    updateColorsForMode();
}

ColorBlindMode AccessibilityConfig::getColorBlindMode() const
{
    return _colorBlindMode;
}

void AccessibilityConfig::initDefaultColors()
{
    _playerColor        = {0, 150, 255, 255};
    _otherPlayerColor   = {0, 200, 100, 255};
    _enemyColor         = {200, 50, 50, 255};
    _playerMissileColor = {255, 255, 0, 255};
    _enemyMissileColor  = {255, 100, 100, 255};
    _healthHigh         = {0, 200, 50, 255};
    _healthMedium       = {255, 165, 0, 255};
    _healthLow          = {200, 0, 0, 255};
}

void AccessibilityConfig::updateColorsForMode()
{
    switch (_colorBlindMode) {
        case ColorBlindMode::None:
            initDefaultColors();
            break;

        case ColorBlindMode::Protanopia:
        case ColorBlindMode::Deuteranopia:
            _playerColor        = {100, 100, 255, 255};
            _otherPlayerColor   = {255, 255, 100, 255};
            _enemyColor         = {180, 100, 255, 255};
            _playerMissileColor = {100, 255, 255, 255};
            _enemyMissileColor  = {255, 150, 255, 255};
            _healthHigh         = {100, 100, 255, 255};
            _healthMedium       = {255, 255, 100, 255};
            _healthLow          = {180, 100, 255, 255};
            break;

        case ColorBlindMode::Tritanopia:
            _playerColor        = {100, 255, 100, 255};
            _otherPlayerColor   = {255, 100, 100, 255};
            _enemyColor         = {255, 100, 255, 255};
            _playerMissileColor = {200, 255, 200, 255};
            _enemyMissileColor  = {255, 150, 200, 255};
            _healthHigh         = {100, 255, 100, 255};
            _healthMedium       = {255, 200, 100, 255};
            _healthLow          = {255, 100, 255, 255};
            break;

        case ColorBlindMode::HighContrast:
            _playerColor        = {0, 255, 255, 255};
            _otherPlayerColor   = {255, 255, 0, 255};
            _enemyColor         = {255, 0, 255, 255};
            _playerMissileColor = {255, 255, 255, 255};
            _enemyMissileColor  = {255, 128, 0, 255};
            _healthHigh         = {0, 255, 0, 255};
            _healthMedium       = {255, 255, 0, 255};
            _healthLow          = {255, 0, 0, 255};
            break;
    }
}

Color AccessibilityConfig::getPlayerColor() const { return _playerColor; }
Color AccessibilityConfig::getOtherPlayerColor() const { return _otherPlayerColor; }
Color AccessibilityConfig::getEnemyColor() const { return _enemyColor; }
Color AccessibilityConfig::getPlayerMissileColor() const { return _playerMissileColor; }
Color AccessibilityConfig::getEnemyMissileColor() const { return _enemyMissileColor; }
Color AccessibilityConfig::getHealthHighColor() const { return _healthHigh; }
Color AccessibilityConfig::getHealthMediumColor() const { return _healthMedium; }
Color AccessibilityConfig::getHealthLowColor() const { return _healthLow; }

void AccessibilityConfig::setGameSpeedMultiplier(float multiplier)
{
    _gameSpeedMultiplier = std::clamp(multiplier, 0.5f, 2.0f);
}

float AccessibilityConfig::getGameSpeedMultiplier() const
{
    return _gameSpeedMultiplier;
}

void AccessibilityConfig::setShipSkin(uint8_t skinId)
{
    _shipSkin = std::clamp(skinId, static_cast<uint8_t>(1), static_cast<uint8_t>(6));
}

uint8_t AccessibilityConfig::getShipSkin() const
{
    return _shipSkin;
}

std::string AccessibilityConfig::keyToString(events::Key key)
{
    static const std::unordered_map<events::Key, std::string> keyNames = {
        {events::Key::A, "A"}, {events::Key::B, "B"}, {events::Key::C, "C"},
        {events::Key::D, "D"}, {events::Key::E, "E"}, {events::Key::F, "F"},
        {events::Key::G, "G"}, {events::Key::H, "H"}, {events::Key::I, "I"},
        {events::Key::J, "J"}, {events::Key::K, "K"}, {events::Key::L, "L"},
        {events::Key::M, "M"}, {events::Key::N, "N"}, {events::Key::O, "O"},
        {events::Key::P, "P"}, {events::Key::Q, "Q"}, {events::Key::R, "R"},
        {events::Key::S, "S"}, {events::Key::T, "T"}, {events::Key::U, "U"},
        {events::Key::V, "V"}, {events::Key::W, "W"}, {events::Key::X, "X"},
        {events::Key::Y, "Y"}, {events::Key::Z, "Z"},
        {events::Key::Num0, "0"}, {events::Key::Num1, "1"}, {events::Key::Num2, "2"},
        {events::Key::Num3, "3"}, {events::Key::Num4, "4"}, {events::Key::Num5, "5"},
        {events::Key::Num6, "6"}, {events::Key::Num7, "7"}, {events::Key::Num8, "8"},
        {events::Key::Num9, "9"},
        {events::Key::Space, "Space"}, {events::Key::Enter, "Enter"},
        {events::Key::Escape, "Escape"}, {events::Key::Tab, "Tab"},
        {events::Key::Backspace, "Backspace"},
        {events::Key::Up, "Up"}, {events::Key::Down, "Down"},
        {events::Key::Left, "Left"}, {events::Key::Right, "Right"},
        {events::Key::LShift, "LShift"}, {events::Key::RShift, "RShift"},
        {events::Key::LCtrl, "LCtrl"}, {events::Key::RCtrl, "RCtrl"},
        {events::Key::LAlt, "LAlt"}, {events::Key::RAlt, "RAlt"},
        {events::Key::Unknown, "Unknown"}
    };

    auto it = keyNames.find(key);
    return it != keyNames.end() ? it->second : "Unknown";
}

events::Key AccessibilityConfig::stringToKey(const std::string& str)
{
    static const std::unordered_map<std::string, events::Key> keyMap = {
        {"A", events::Key::A}, {"B", events::Key::B}, {"C", events::Key::C},
        {"D", events::Key::D}, {"E", events::Key::E}, {"F", events::Key::F},
        {"G", events::Key::G}, {"H", events::Key::H}, {"I", events::Key::I},
        {"J", events::Key::J}, {"K", events::Key::K}, {"L", events::Key::L},
        {"M", events::Key::M}, {"N", events::Key::N}, {"O", events::Key::O},
        {"P", events::Key::P}, {"Q", events::Key::Q}, {"R", events::Key::R},
        {"S", events::Key::S}, {"T", events::Key::T}, {"U", events::Key::U},
        {"V", events::Key::V}, {"W", events::Key::W}, {"X", events::Key::X},
        {"Y", events::Key::Y}, {"Z", events::Key::Z},
        {"0", events::Key::Num0}, {"1", events::Key::Num1}, {"2", events::Key::Num2},
        {"3", events::Key::Num3}, {"4", events::Key::Num4}, {"5", events::Key::Num5},
        {"6", events::Key::Num6}, {"7", events::Key::Num7}, {"8", events::Key::Num8},
        {"9", events::Key::Num9},
        {"Space", events::Key::Space}, {"Enter", events::Key::Enter},
        {"Escape", events::Key::Escape}, {"Tab", events::Key::Tab},
        {"Backspace", events::Key::Backspace},
        {"Up", events::Key::Up}, {"Down", events::Key::Down},
        {"Left", events::Key::Left}, {"Right", events::Key::Right},
        {"LShift", events::Key::LShift}, {"RShift", events::Key::RShift},
        {"LCtrl", events::Key::LCtrl}, {"RCtrl", events::Key::RCtrl},
        {"LAlt", events::Key::LAlt}, {"RAlt", events::Key::RAlt}
    };

    auto it = keyMap.find(str);
    return it != keyMap.end() ? it->second : events::Key::Unknown;
}

std::string AccessibilityConfig::colorBlindModeToString(ColorBlindMode mode)
{
    switch (mode) {
        case ColorBlindMode::None: return "none";
        case ColorBlindMode::Protanopia: return "protanopia";
        case ColorBlindMode::Deuteranopia: return "deuteranopia";
        case ColorBlindMode::Tritanopia: return "tritanopia";
        case ColorBlindMode::HighContrast: return "high_contrast";
        default: return "none";
    }
}

ColorBlindMode AccessibilityConfig::stringToColorBlindMode(const std::string& str)
{
    if (str == "protanopia") return ColorBlindMode::Protanopia;
    if (str == "deuteranopia") return ColorBlindMode::Deuteranopia;
    if (str == "tritanopia") return ColorBlindMode::Tritanopia;
    if (str == "high_contrast") return ColorBlindMode::HighContrast;
    return ColorBlindMode::None;
}

bool AccessibilityConfig::loadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "colorblind_mode") {
            setColorBlindMode(stringToColorBlindMode(value));
        } else if (key == "game_speed") {
            setGameSpeedMultiplier(std::stof(value));
        } else if (key == "move_up") {
            setKeyBinding(GameAction::MoveUp, stringToKey(value), getSecondaryKey(GameAction::MoveUp));
        } else if (key == "move_up_alt") {
            setKeyBinding(GameAction::MoveUp, getPrimaryKey(GameAction::MoveUp), stringToKey(value));
        } else if (key == "move_down") {
            setKeyBinding(GameAction::MoveDown, stringToKey(value), getSecondaryKey(GameAction::MoveDown));
        } else if (key == "move_down_alt") {
            setKeyBinding(GameAction::MoveDown, getPrimaryKey(GameAction::MoveDown), stringToKey(value));
        } else if (key == "move_left") {
            setKeyBinding(GameAction::MoveLeft, stringToKey(value), getSecondaryKey(GameAction::MoveLeft));
        } else if (key == "move_left_alt") {
            setKeyBinding(GameAction::MoveLeft, getPrimaryKey(GameAction::MoveLeft), stringToKey(value));
        } else if (key == "move_right") {
            setKeyBinding(GameAction::MoveRight, stringToKey(value), getSecondaryKey(GameAction::MoveRight));
        } else if (key == "move_right_alt") {
            setKeyBinding(GameAction::MoveRight, getPrimaryKey(GameAction::MoveRight), stringToKey(value));
        } else if (key == "shoot") {
            setKeyBinding(GameAction::Shoot, stringToKey(value), getSecondaryKey(GameAction::Shoot));
        } else if (key == "shoot_alt") {
            setKeyBinding(GameAction::Shoot, getPrimaryKey(GameAction::Shoot), stringToKey(value));
        } else if (key == "open_chat") {
            setKeyBinding(GameAction::OpenChat, stringToKey(value), getSecondaryKey(GameAction::OpenChat));
        } else if (key == "expand_chat") {
            setKeyBinding(GameAction::ExpandChat, stringToKey(value), getSecondaryKey(GameAction::ExpandChat));
        } else if (key == "keep_chat_open") {
            setKeepChatOpenAfterSend(value == "true" || value == "1");
        }
    }

    return true;
}

bool AccessibilityConfig::saveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "# R-Type Accessibility Configuration\n";
    file << "# Edit this file to customize your game experience\n\n";

    file << "# Color blind mode: none, protanopia, deuteranopia, tritanopia, high_contrast\n";
    file << "colorblind_mode = " << colorBlindModeToString(_colorBlindMode) << "\n\n";

    file << "# Game speed multiplier (0.5 to 2.0, 1.0 = normal)\n";
    file << "game_speed = " << _gameSpeedMultiplier << "\n\n";

    file << "# Key bindings (primary and alternative)\n";
    file << "move_up = " << keyToString(getPrimaryKey(GameAction::MoveUp)) << "\n";
    file << "move_up_alt = " << keyToString(getSecondaryKey(GameAction::MoveUp)) << "\n";
    file << "move_down = " << keyToString(getPrimaryKey(GameAction::MoveDown)) << "\n";
    file << "move_down_alt = " << keyToString(getSecondaryKey(GameAction::MoveDown)) << "\n";
    file << "move_left = " << keyToString(getPrimaryKey(GameAction::MoveLeft)) << "\n";
    file << "move_left_alt = " << keyToString(getSecondaryKey(GameAction::MoveLeft)) << "\n";
    file << "move_right = " << keyToString(getPrimaryKey(GameAction::MoveRight)) << "\n";
    file << "move_right_alt = " << keyToString(getSecondaryKey(GameAction::MoveRight)) << "\n";
    file << "shoot = " << keyToString(getPrimaryKey(GameAction::Shoot)) << "\n";
    file << "shoot_alt = " << keyToString(getSecondaryKey(GameAction::Shoot)) << "\n\n";

    file << "# Chat settings\n";
    file << "open_chat = " << keyToString(getPrimaryKey(GameAction::OpenChat)) << "\n";
    file << "expand_chat = " << keyToString(getPrimaryKey(GameAction::ExpandChat)) << "\n";
    file << "# Keep chat input open after sending a message (true/false)\n";
    file << "keep_chat_open = " << (_keepChatOpenAfterSend ? "true" : "false") << "\n";

    return true;
}

}
