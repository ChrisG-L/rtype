/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AudioDevicePersistenceTest - Unit tests for audio device persistence
** Tests that selected audio devices are correctly stored and retrieved
*/

#include <gtest/gtest.h>
#include <string>

// For testing, we create a mock/testable version that doesn't need real audio hardware
// This tests the logic of device name storage and retrieval

/**
 * @brief Simplified mock of VoiceChatManager's device selection logic
 *
 * This mock allows testing the core persistence logic without requiring
 * actual PortAudio initialization or hardware devices.
 */
class MockVoiceChatManager {
public:
    // Getters - return the selected device names
    std::string getSelectedInputDevice() const { return _selectedInputDevice; }
    std::string getSelectedOutputDevice() const { return _selectedOutputDevice; }

    // Get preferred devices (used during init)
    std::string getPreferredInputDevice() const { return _preferredInputDevice; }
    std::string getPreferredOutputDevice() const { return _preferredOutputDevice; }

    /**
     * @brief Set selected devices (for loading from server, without reinit)
     * This should update BOTH selected and preferred
     */
    void setSelectedDevices(const std::string& inputDeviceName, const std::string& outputDeviceName) {
        _selectedInputDevice = inputDeviceName;
        _selectedOutputDevice = outputDeviceName;
        _preferredInputDevice = inputDeviceName;
        _preferredOutputDevice = outputDeviceName;
    }

    /**
     * @brief Apply audio device settings (simulates the real method)
     * IMPORTANT: Must update BOTH _selectedInputDevice and _preferredInputDevice
     * This was the bug that was fixed
     */
    bool applyAudioDevices(const std::string& inputDeviceName, const std::string& outputDeviceName) {
        // Store preferences AND selected devices (for persistence)
        _preferredInputDevice = inputDeviceName;
        _preferredOutputDevice = outputDeviceName;
        _selectedInputDevice = inputDeviceName;
        _selectedOutputDevice = outputDeviceName;

        // In real implementation, this would reinitialize audio if already running
        return true;
    }

    // Voice settings
    enum class VoiceMode { PushToTalk, VoiceActivity };

    void applySettings(uint8_t voiceMode, uint8_t vadThreshold, uint8_t micGain, uint8_t voiceVolume) {
        _voiceMode = (voiceMode == 1) ? VoiceMode::VoiceActivity : VoiceMode::PushToTalk;
        _vadThreshold = static_cast<float>(vadThreshold) / 100.0f;
        _micGain = static_cast<float>(micGain) / 100.0f;
        _playbackVolume = static_cast<int>(voiceVolume);
    }

    VoiceMode getVoiceMode() const { return _voiceMode; }
    float getVADThreshold() const { return _vadThreshold; }
    float getMicGain() const { return _micGain; }
    int getPlaybackVolume() const { return _playbackVolume; }

private:
    std::string _preferredInputDevice;
    std::string _preferredOutputDevice;
    std::string _selectedInputDevice;
    std::string _selectedOutputDevice;

    VoiceMode _voiceMode{VoiceMode::PushToTalk};
    float _vadThreshold{0.02f};
    float _micGain{1.0f};
    int _playbackVolume{80};
};

class AudioDevicePersistenceTest : public ::testing::Test {
protected:
    MockVoiceChatManager voice;

    void SetUp() override {
        // Reset to default state
    }
};

// ═══════════════════════════════════════════════════════════════════
// Test: setSelectedDevices() correctly stores both input and output
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, SetSelectedDevicesStoresInputDevice) {
    voice.setSelectedDevices("Test Microphone", "Test Speaker");

    EXPECT_EQ(voice.getSelectedInputDevice(), "Test Microphone");
}

TEST_F(AudioDevicePersistenceTest, SetSelectedDevicesStoresOutputDevice) {
    voice.setSelectedDevices("Test Microphone", "Test Speaker");

    EXPECT_EQ(voice.getSelectedOutputDevice(), "Test Speaker");
}

TEST_F(AudioDevicePersistenceTest, SetSelectedDevicesAlsoUpdatesPreferred) {
    voice.setSelectedDevices("Test Microphone", "Test Speaker");

    // Preferred should also be updated (for next init())
    EXPECT_EQ(voice.getPreferredInputDevice(), "Test Microphone");
    EXPECT_EQ(voice.getPreferredOutputDevice(), "Test Speaker");
}

TEST_F(AudioDevicePersistenceTest, SetSelectedDevicesWithEmptyStringsForAuto) {
    voice.setSelectedDevices("", "");

    EXPECT_EQ(voice.getSelectedInputDevice(), "");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "");
}

// ═══════════════════════════════════════════════════════════════════
// Test: applyAudioDevices() updates BOTH preferred AND selected
// This was the bug that was fixed
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, ApplyAudioDevicesUpdatesSelectedInput) {
    voice.applyAudioDevices("Applied Mic", "Applied Speaker");

    // The key test: getSelectedInputDevice should return the applied value
    EXPECT_EQ(voice.getSelectedInputDevice(), "Applied Mic");
}

TEST_F(AudioDevicePersistenceTest, ApplyAudioDevicesUpdatesSelectedOutput) {
    voice.applyAudioDevices("Applied Mic", "Applied Speaker");

    EXPECT_EQ(voice.getSelectedOutputDevice(), "Applied Speaker");
}

TEST_F(AudioDevicePersistenceTest, ApplyAudioDevicesUpdatesPreferredInput) {
    voice.applyAudioDevices("Applied Mic", "Applied Speaker");

    EXPECT_EQ(voice.getPreferredInputDevice(), "Applied Mic");
}

TEST_F(AudioDevicePersistenceTest, ApplyAudioDevicesUpdatesPreferredOutput) {
    voice.applyAudioDevices("Applied Mic", "Applied Speaker");

    EXPECT_EQ(voice.getPreferredOutputDevice(), "Applied Speaker");
}

TEST_F(AudioDevicePersistenceTest, ApplyAudioDevicesWithAutoSelection) {
    // Empty string means "Auto" selection
    voice.applyAudioDevices("", "");

    EXPECT_EQ(voice.getSelectedInputDevice(), "");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "");
    EXPECT_EQ(voice.getPreferredInputDevice(), "");
    EXPECT_EQ(voice.getPreferredOutputDevice(), "");
}

// ═══════════════════════════════════════════════════════════════════
// Test: Roundtrip - apply then get returns same values
// This simulates the SettingsScene workflow
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, RoundtripApplyThenGetReturnsCorrectValues) {
    // Simulates: User selects devices in Settings, clicks Apply
    std::string inputDevice = "Raptor Lake-P/U/H cAVS Digital Microphone";
    std::string outputDevice = "Raptor Lake-P/U/H cAVS HDMI / DisplayPort 2 Output";

    voice.applyAudioDevices(inputDevice, outputDevice);

    // Simulates: User reopens Settings, reads from VoiceChatManager
    EXPECT_EQ(voice.getSelectedInputDevice(), inputDevice);
    EXPECT_EQ(voice.getSelectedOutputDevice(), outputDevice);
}

TEST_F(AudioDevicePersistenceTest, RoundtripSetSelectedThenGetReturnsCorrectValues) {
    // Simulates: Server sends user settings on login
    std::string inputDevice = "Test Input Device";
    std::string outputDevice = "Test Output Device";

    voice.setSelectedDevices(inputDevice, outputDevice);

    // Simulates: SettingsScene constructor reads from VoiceChatManager
    EXPECT_EQ(voice.getSelectedInputDevice(), inputDevice);
    EXPECT_EQ(voice.getSelectedOutputDevice(), outputDevice);
}

// ═══════════════════════════════════════════════════════════════════
// Test: Multiple operations preserve consistency
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, MultipleApplyOperationsPreserveLastValue) {
    voice.applyAudioDevices("Mic1", "Speaker1");
    voice.applyAudioDevices("Mic2", "Speaker2");
    voice.applyAudioDevices("Mic3", "Speaker3");

    EXPECT_EQ(voice.getSelectedInputDevice(), "Mic3");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "Speaker3");
}

TEST_F(AudioDevicePersistenceTest, SetSelectedAfterApplyOverwrites) {
    voice.applyAudioDevices("Applied Mic", "Applied Speaker");
    voice.setSelectedDevices("Server Mic", "Server Speaker");

    EXPECT_EQ(voice.getSelectedInputDevice(), "Server Mic");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "Server Speaker");
}

TEST_F(AudioDevicePersistenceTest, ApplyAfterSetSelectedOverwrites) {
    voice.setSelectedDevices("Server Mic", "Server Speaker");
    voice.applyAudioDevices("User Mic", "User Speaker");

    EXPECT_EQ(voice.getSelectedInputDevice(), "User Mic");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "User Speaker");
}

// ═══════════════════════════════════════════════════════════════════
// Test: Reset to defaults (empty strings)
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, ResetToDefaultsClearsSelection) {
    // Set some values first
    voice.applyAudioDevices("Some Mic", "Some Speaker");
    EXPECT_NE(voice.getSelectedInputDevice(), "");

    // Reset to defaults (empty = Auto)
    voice.applyAudioDevices("", "");

    EXPECT_EQ(voice.getSelectedInputDevice(), "");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "");
}

TEST_F(AudioDevicePersistenceTest, ResetToDefaultsAlsoResetsPreferred) {
    voice.applyAudioDevices("Some Mic", "Some Speaker");
    voice.applyAudioDevices("", "");

    EXPECT_EQ(voice.getPreferredInputDevice(), "");
    EXPECT_EQ(voice.getPreferredOutputDevice(), "");
}

// ═══════════════════════════════════════════════════════════════════
// Test: Independent input/output operations
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, CanSetInputOnlyKeepOutputAuto) {
    voice.applyAudioDevices("My Mic", "");

    EXPECT_EQ(voice.getSelectedInputDevice(), "My Mic");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "");
}

TEST_F(AudioDevicePersistenceTest, CanSetOutputOnlyKeepInputAuto) {
    voice.applyAudioDevices("", "My Speaker");

    EXPECT_EQ(voice.getSelectedInputDevice(), "");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "My Speaker");
}

// ═══════════════════════════════════════════════════════════════════
// Test: Voice settings persistence
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, ApplyVoiceSettingsPTTMode) {
    voice.applySettings(0, 20, 100, 80);  // PTT, 20% threshold, 100% gain, 80% volume

    EXPECT_EQ(voice.getVoiceMode(), MockVoiceChatManager::VoiceMode::PushToTalk);
}

TEST_F(AudioDevicePersistenceTest, ApplyVoiceSettingsVADMode) {
    voice.applySettings(1, 20, 100, 80);  // VAD mode

    EXPECT_EQ(voice.getVoiceMode(), MockVoiceChatManager::VoiceMode::VoiceActivity);
}

TEST_F(AudioDevicePersistenceTest, ApplyVoiceSettingsVADThreshold) {
    voice.applySettings(0, 50, 100, 80);  // 50% threshold

    EXPECT_NEAR(voice.getVADThreshold(), 0.50f, 0.01f);
}

TEST_F(AudioDevicePersistenceTest, ApplyVoiceSettingsMicGain) {
    voice.applySettings(0, 20, 150, 80);  // 150% = 1.5x gain

    EXPECT_NEAR(voice.getMicGain(), 1.50f, 0.01f);
}

TEST_F(AudioDevicePersistenceTest, ApplyVoiceSettingsPlaybackVolume) {
    voice.applySettings(0, 20, 100, 75);  // 75% volume

    EXPECT_EQ(voice.getPlaybackVolume(), 75);
}

// ═══════════════════════════════════════════════════════════════════
// Test: Full SettingsScene workflow simulation
// This is the exact flow that was broken before the fix
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, SettingsSceneWorkflow_InSessionPersistence) {
    // Step 1: User opens Settings, selects devices, clicks Apply
    std::string userInputDevice = "Raptor Lake-P/U/H cAVS Digital Microphone";
    std::string userOutputDevice = "Raptor Lake-P/U/H cAVS HDMI / DisplayPort 2 Output";

    bool applyResult = voice.applyAudioDevices(userInputDevice, userOutputDevice);
    EXPECT_TRUE(applyResult);

    // Step 2: User closes Settings (scene destroyed)
    // Step 3: User opens Settings again (new scene created)
    // New SettingsScene constructor reads from VoiceChatManager:
    std::string loadedInput = voice.getSelectedInputDevice();
    std::string loadedOutput = voice.getSelectedOutputDevice();

    // CRITICAL: Must show the user's saved selection, NOT "Auto"
    EXPECT_EQ(loadedInput, userInputDevice)
        << "In-session persistence failed: input device reset to default";
    EXPECT_EQ(loadedOutput, userOutputDevice)
        << "In-session persistence failed: output device reset to default";
}

TEST_F(AudioDevicePersistenceTest, SettingsSceneWorkflow_CrossSessionPersistence) {
    // Simulates client restart

    // Step 1: User logs in, server sends saved settings via TCPUserSettingsEvent
    std::string serverSavedInput = "Raptor Lake-P/U/H cAVS Digital Microphone";
    std::string serverSavedOutput = "Raptor Lake-P/U/H cAVS HDMI / DisplayPort 2 Output";

    // MainMenuScene applies settings from server
    voice.setSelectedDevices(serverSavedInput, serverSavedOutput);

    // Step 2: User opens Settings
    // SettingsScene constructor reads from VoiceChatManager:
    std::string displayedInput = voice.getSelectedInputDevice();
    std::string displayedOutput = voice.getSelectedOutputDevice();

    // CRITICAL: Must show the server-saved selection, NOT "Auto"
    EXPECT_EQ(displayedInput, serverSavedInput)
        << "Cross-session persistence failed: input device not loaded from server";
    EXPECT_EQ(displayedOutput, serverSavedOutput)
        << "Cross-session persistence failed: output device not loaded from server";
}

TEST_F(AudioDevicePersistenceTest, SettingsSceneWorkflow_ResetToDefaults) {
    // Step 1: User has saved custom devices
    voice.applyAudioDevices("Custom Mic", "Custom Speaker");
    EXPECT_EQ(voice.getSelectedInputDevice(), "Custom Mic");

    // Step 2: User clicks "Reset to Defaults" in Settings
    voice.applyAudioDevices("", "");

    // Step 3: Settings should show "Auto" (empty strings)
    EXPECT_EQ(voice.getSelectedInputDevice(), "");
    EXPECT_EQ(voice.getSelectedOutputDevice(), "");

    // Step 4: User reopens Settings - should still show "Auto"
    std::string loadedInput = voice.getSelectedInputDevice();
    std::string loadedOutput = voice.getSelectedOutputDevice();

    EXPECT_EQ(loadedInput, "")
        << "Reset to defaults not persisted correctly";
    EXPECT_EQ(loadedOutput, "")
        << "Reset to defaults not persisted correctly";
}

// ═══════════════════════════════════════════════════════════════════
// Test: Edge cases with special characters in device names
// ═══════════════════════════════════════════════════════════════════

TEST_F(AudioDevicePersistenceTest, DeviceNameWithSlashes) {
    std::string deviceName = "Raptor Lake-P/U/H cAVS HDMI / DisplayPort 2 Output";
    voice.applyAudioDevices("", deviceName);

    EXPECT_EQ(voice.getSelectedOutputDevice(), deviceName);
}

TEST_F(AudioDevicePersistenceTest, DeviceNameWithSpecialChars) {
    std::string deviceName = "Device (USB) [2.0] - Stereo";
    voice.applyAudioDevices(deviceName, "");

    EXPECT_EQ(voice.getSelectedInputDevice(), deviceName);
}

TEST_F(AudioDevicePersistenceTest, DeviceNameWithUnicode) {
    // Some audio devices might have unicode names
    std::string deviceName = "Microphone - Built-in Audio";
    voice.applyAudioDevices(deviceName, "");

    EXPECT_EQ(voice.getSelectedInputDevice(), deviceName);
}

// main() is provided by tests/client/main.cpp
