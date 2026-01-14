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
    std::array<uint8_t, 26> keyBindings;  // 13 actions × 2 keys (matches KEY_BINDINGS_COUNT in Protocol.hpp)
    uint8_t shipSkin;               // Ship skin variant (1-6 for Ship1.png to Ship6.png)
    uint8_t voiceMode;              // 0 = PushToTalk, 1 = VoiceActivity
    uint8_t vadThreshold;           // 0-100 (scaled from 0.0-1.0)
    uint8_t micGain;                // 0-200 (scaled from 0.0-2.0)
    uint8_t voiceVolume;            // 0-100
    std::string audioInputDevice;   // Preferred input device name ("" = auto)
    std::string audioOutputDevice;  // Preferred output device name ("" = auto)
    bool keepChatOpenAfterSend;     // If true, chat input stays open after sending

    // Default constructor with default values
    UserSettingsData()
        : colorBlindMode("none")
        , gameSpeedPercent(100)
        , keyBindings{}
        , shipSkin(1)
        , voiceMode(0)
        , vadThreshold(2)
        , micGain(100)
        , voiceVolume(100)
        , audioInputDevice("")
        , audioOutputDevice("")
        , keepChatOpenAfterSend(false) {}

    // Set default key bindings (matches AccessibilityConfig defaults)
    // Key values based on events::Key enum:
    // A=0, B=1, C=2, D=3, E=4, F=5, G=6, H=7, I=8, J=9, K=10, L=11, M=12
    // N=13, O=14, P=15, Q=16, R=17, S=18, T=19, U=20, V=21, W=22, X=23, Y=24, Z=25
    // Space=36, Enter=37, Escape=38, Up=41, Down=42, Left=43, Right=44, Unknown=51
    void setDefaultKeyBindings() {
        // MoveUp: Up=41, Z=25
        keyBindings[0] = 41;  // Up
        keyBindings[1] = 25;  // Z
        // MoveDown: Down=42, S=18
        keyBindings[2] = 42;  // Down
        keyBindings[3] = 18;  // S
        // MoveLeft: Left=43, A=0 (changed from Q since Q is now WeaponPrev)
        keyBindings[4] = 43;  // Left
        keyBindings[5] = 0;   // A
        // MoveRight: Right=44, D=3
        keyBindings[6] = 44;  // Right
        keyBindings[7] = 3;   // D
        // Shoot: Space=36, Enter=37
        keyBindings[8] = 36;  // Space
        keyBindings[9] = 37;  // Enter
        // Pause: Escape=38, P=15
        keyBindings[10] = 38; // Escape
        keyBindings[11] = 15; // P
        // PushToTalk: V=21, Unknown=51
        keyBindings[12] = 21; // V
        keyBindings[13] = 51; // Unknown
        // WeaponPrev: Q=16, Unknown=51
        keyBindings[14] = 16; // Q
        keyBindings[15] = 51; // Unknown
        // WeaponNext: E=4, Unknown=51
        keyBindings[16] = 4;  // E
        keyBindings[17] = 51; // Unknown
        // OpenChat: T=19, Unknown=51
        keyBindings[18] = 19; // T
        keyBindings[19] = 51; // Unknown
        // ExpandChat: O=14, Unknown=51
        keyBindings[20] = 14; // O
        keyBindings[21] = 51; // Unknown
        // ForceToggle: F=5, Unknown=51
        keyBindings[22] = 5;  // F
        keyBindings[23] = 51; // Unknown
        // ToggleControls: H=7, Unknown=51
        keyBindings[24] = 7;  // H
        keyBindings[25] = 51; // Unknown
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
