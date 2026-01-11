/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IUserSettingsRepository - Interface for user settings persistence
*/

#ifndef IUSERSETTINGSREPOSITORY_HPP_
#define IUSERSETTINGSREPOSITORY_HPP_

#include <string>
#include <optional>
#include <array>
#include <cstdint>

namespace application::ports::out::persistence {

// Data transfer object for user settings
struct UserSettingsData {
    std::string colorBlindMode;     // "none", "protanopia", "deuteranopia", "tritanopia", "high_contrast"
    uint16_t gameSpeedPercent;      // 50-200 (represents 0.5x-2.0x)
    std::array<uint8_t, 12> keyBindings;  // 6 actions × 2 keys
    uint8_t shipSkin;               // Ship skin variant (1-6 for Ship1.png to Ship6.png)

    // Default constructor with default values
    UserSettingsData()
        : colorBlindMode("none")
        , gameSpeedPercent(100)
        , keyBindings{}
        , shipSkin(1) {}

    // Set default key bindings (matches AccessibilityConfig defaults)
    void setDefaultKeyBindings() {
        // Default bindings: Up/Z, Down/S, Left/Q, Right/D, Space/Enter, Escape/P
        // Key values are based on events::Key enum
        // MoveUp: Up=24, Z=25
        keyBindings[0] = 24;  // Up
        keyBindings[1] = 25;  // Z
        // MoveDown: Down=22, S=18
        keyBindings[2] = 22;  // Down
        keyBindings[3] = 18;  // S
        // MoveLeft: Left=23, Q=16
        keyBindings[4] = 23;  // Left
        keyBindings[5] = 16;  // Q
        // MoveRight: Right=21, D=3
        keyBindings[6] = 21;  // Right
        keyBindings[7] = 3;   // D
        // Shoot: Space=36, Enter=37
        keyBindings[8] = 36;  // Space
        keyBindings[9] = 37;  // Enter
        // Pause: Escape=38, P=15
        keyBindings[10] = 38; // Escape
        keyBindings[11] = 15; // P
    }
};

class IUserSettingsRepository {
public:
    virtual ~IUserSettingsRepository() = default;

    /**
     * Find user settings by email
     * @param email User's email address
     * @return Settings if found, nullopt otherwise
     */
    virtual std::optional<UserSettingsData> findByEmail(const std::string& email) = 0;

    /**
     * Save or update user settings
     * @param email User's email address
     * @param settings Settings to save
     */
    virtual void save(const std::string& email, const UserSettingsData& settings) = 0;

    /**
     * Delete user settings
     * @param email User's email address
     */
    virtual void remove(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IUSERSETTINGSREPOSITORY_HPP_ */
