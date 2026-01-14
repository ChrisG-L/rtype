/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceChatManager - Voice chat system with PortAudio + Opus
*/

#ifndef VOICECHATMANAGER_HPP_
#define VOICECHATMANAGER_HPP_

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>

#include <boost/asio.hpp>
#include <portaudio.h>

#include "audio/OpusCodec.hpp"
#include "Protocol.hpp"

namespace audio {

/**
 * @brief Voice chat manager singleton for push-to-talk and voice activity detection
 *
 * Handles:
 * - Microphone capture via PortAudio
 * - Audio encoding/decoding via Opus
 * - Network communication with voice server (UDP port 4126)
 * - Playback of incoming voice from other players
 *
 * Modes:
 * - PushToTalk: Hold V key to transmit
 * - VoiceActivity: Auto-detect when user is speaking
 *
 * Usage:
 *   auto& voice = VoiceChatManager::getInstance();
 *   voice.init();
 *   voice.connect("localhost", 4126);
 *   voice.joinVoiceChannel(sessionToken, "ABCDEF");
 *
 *   // In game loop:
 *   if (keyPressed(V)) voice.startTalking();
 *   if (keyReleased(V)) voice.stopTalking();
 *
 *   // Process incoming audio (call every frame)
 *   voice.update();
 */
class VoiceChatManager {
public:
    static VoiceChatManager& getInstance();

    // Voice input mode
    enum class VoiceMode {
        PushToTalk,     // Transmit only when key held
        VoiceActivity   // Auto-detect voice
    };

    /**
     * @brief Initialize the voice chat system
     * @return true if initialization succeeded
     */
    bool init();

    /**
     * @brief Shutdown the voice chat system
     */
    void shutdown();

    /**
     * @brief Check if voice chat is initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief Connect to voice server
     * @param host Server hostname or IP
     * @param port Server port (default 4126)
     * @return true if connection succeeded
     */
    bool connect(const std::string& host, uint16_t port = VOICE_UDP_PORT);

    /**
     * @brief Disconnect from voice server
     */
    void disconnect();

    /**
     * @brief Check if connected to voice server
     */
    bool isConnected() const { return _connected; }

    /**
     * @brief Join a voice channel for a room
     * @param token Session token (from game authentication)
     * @param roomCode Room code to join
     */
    void joinVoiceChannel(const SessionToken& token, const std::string& roomCode);

    /**
     * @brief Leave the current voice channel
     */
    void leaveVoiceChannel();

    // ═══════════════════════════════════════════════════════════════════
    // Voice Mode Settings
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Set voice input mode
     */
    void setVoiceMode(VoiceMode mode) { _voiceMode = mode; }

    /**
     * @brief Get current voice mode
     */
    VoiceMode getVoiceMode() const { return _voiceMode; }

    /**
     * @brief Set VAD (Voice Activity Detection) threshold
     * @param threshold 0.0 to 1.0 (lower = more sensitive)
     */
    void setVADThreshold(float threshold) { _vadThreshold = threshold; }

    /**
     * @brief Get VAD threshold
     */
    float getVADThreshold() const { return _vadThreshold; }

    // ═══════════════════════════════════════════════════════════════════
    // Push-to-Talk Controls
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Start transmitting (call when PTT key pressed)
     */
    void startTalking();

    /**
     * @brief Stop transmitting (call when PTT key released)
     */
    void stopTalking();

    /**
     * @brief Check if currently transmitting
     */
    bool isTalking() const { return _talking; }

    // ═══════════════════════════════════════════════════════════════════
    // Volume Settings
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Set microphone input gain
     * @param gain 0.0 to 2.0 (1.0 = normal, 2.0 = double volume)
     */
    void setMicGain(float gain) { _micGain = gain; }

    /**
     * @brief Get microphone gain
     */
    float getMicGain() const { return _micGain; }

    /**
     * @brief Set voice chat playback volume
     * @param volume 0 to 100
     */
    void setPlaybackVolume(int volume) { _playbackVolume = volume; }

    /**
     * @brief Get playback volume
     */
    int getPlaybackVolume() const { return _playbackVolume; }

    /**
     * @brief Mute/unmute microphone
     */
    void setMuted(bool muted);

    /**
     * @brief Check if microphone is muted
     */
    bool isMuted() const { return _muted; }

    /**
     * @brief Apply settings from UserSettingsPayload (called after loading user settings)
     * @param voiceMode 0 = PushToTalk, 1 = VoiceActivity
     * @param vadThreshold 0-100 (converted to 0.0-1.0 internally)
     * @param micGain 0-200 (converted to 0.0-2.0 internally)
     * @param voiceVolume 0-100
     */
    void applySettings(uint8_t voiceMode, uint8_t vadThreshold, uint8_t micGain, uint8_t voiceVolume);

    // ═══════════════════════════════════════════════════════════════════
    // Audio Device Selection
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Audio device information
     */
    struct AudioDeviceInfo {
        int index;                   // PortAudio device index
        std::string name;            // Device name
        int maxInputChannels;        // Max input channels (0 = output only)
        int maxOutputChannels;       // Max output channels (0 = input only)
        double defaultSampleRate;    // Default sample rate
        bool isDefault;              // Is system default device
    };

    /**
     * @brief Get list of available input devices (microphones)
     * @return Vector of input device info, first element is "Auto (default)"
     */
    std::vector<AudioDeviceInfo> getInputDevices() const;

    /**
     * @brief Get list of available output devices (speakers)
     * @return Vector of output device info, first element is "Auto (default)"
     */
    std::vector<AudioDeviceInfo> getOutputDevices() const;

    /**
     * @brief Get currently selected input device name
     * @return Device name, or empty string if using auto selection
     */
    std::string getSelectedInputDevice() const { return _selectedInputDevice; }

    /**
     * @brief Get currently selected output device name
     * @return Device name, or empty string if using auto selection
     */
    std::string getSelectedOutputDevice() const { return _selectedOutputDevice; }

    /**
     * @brief Set preferred input device by name
     * @param deviceName Device name, or empty string for auto selection
     * Takes effect on next init() call
     */
    void setPreferredInputDevice(const std::string& deviceName);

    /**
     * @brief Set preferred output device by name
     * @param deviceName Device name, or empty string for auto selection
     * Takes effect on next init() call
     */
    void setPreferredOutputDevice(const std::string& deviceName);

    /**
     * @brief Apply audio device settings and reinitialize if needed
     * @param inputDeviceName Preferred input device name ("" = auto)
     * @param outputDeviceName Preferred output device name ("" = auto)
     * @return true if successfully applied (may require restart)
     */
    bool applyAudioDevices(const std::string& inputDeviceName, const std::string& outputDeviceName);

    /**
     * @brief Set selected devices (for loading from server, without reinit)
     * @param inputDeviceName Input device name ("" = auto)
     * @param outputDeviceName Output device name ("" = auto)
     * Just stores the names, actual device selection happens on init()
     */
    void setSelectedDevices(const std::string& inputDeviceName, const std::string& outputDeviceName);

    // ═══════════════════════════════════════════════════════════════════
    // State Information
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Get list of players currently speaking
     * @return Vector of player IDs who are transmitting
     */
    std::vector<uint8_t> getActiveSpeakers() const;

    /**
     * @brief Get our assigned player ID in voice channel
     */
    uint8_t getLocalPlayerId() const { return _localPlayerId; }

    /**
     * @brief Update voice chat (call every frame from game loop)
     * Processes incoming network data and audio playback
     */
    void update();

private:
    VoiceChatManager() = default;
    ~VoiceChatManager() noexcept;
    VoiceChatManager(const VoiceChatManager&) = delete;
    VoiceChatManager& operator=(const VoiceChatManager&) = delete;

    // PortAudio callback (static to be used as C callback)
    static int audioCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );

    // Network functions
    void startReceive();
    void handleReceive(const boost::system::error_code& error, std::size_t bytes);
    void sendVoiceJoin(const SessionToken& token, const std::string& roomCode);
    void sendVoiceLeave();
    void sendVoiceFrame(const std::vector<uint8_t>& opusData);
    void sendVoiceMute(bool muted);

    // Audio processing
    void processCapture(const float* inputBuffer, unsigned long frameCount);
    void processPlayback(float* outputBuffer, unsigned long frameCount);
    float calculateRMS(const float* buffer, size_t samples) const;
    bool detectVoiceActivity(const float* buffer, size_t samples) const;

    // JACK port connection (for PipeWire compatibility)
    void connectJackPorts();

    // State
    std::atomic<bool> _initialized{false};
    std::atomic<bool> _connected{false};
    std::atomic<bool> _talking{false};
    std::atomic<bool> _muted{false};

    VoiceMode _voiceMode{VoiceMode::PushToTalk};
    float _vadThreshold{0.05f};  // 5% default (higher to avoid keyboard noise)
    float _micGain{1.0f};
    int _playbackVolume{80};

    uint8_t _localPlayerId{0};
    uint16_t _sequenceNum{0};

    // Audio
    PaStream* _stream{nullptr};
    OpusCodec _codec;
    int _inputChannels{1};   // Actual channels opened (may differ from requested)
    int _outputChannels{2};  // Actual channels opened

    // Capture buffer (protected by mutex)
    std::vector<float> _captureBuffer;
    std::mutex _captureMutex;

    // Playback buffers per speaker (protected by mutex)
    struct SpeakerState {
        std::queue<std::vector<float>> frames;
        std::chrono::steady_clock::time_point lastActivity;
        bool speaking{false};
    };
    std::unordered_map<uint8_t, SpeakerState> _speakers;
    std::mutex _playbackMutex;

    // Mixed output buffer for playback
    std::vector<float> _mixBuffer;

    // Network
    boost::asio::io_context _ioContext;
    std::unique_ptr<boost::asio::ip::udp::socket> _socket;
    boost::asio::ip::udp::endpoint _serverEndpoint;
    boost::asio::ip::udp::endpoint _senderEndpoint;
    std::array<char, BUFFER_SIZE> _recvBuffer;
    std::jthread _networkThread;

    // Current room
    std::string _currentRoomCode;

    // Audio device selection (stored by name for persistence across sessions)
    std::string _preferredInputDevice;   // Preferred input device name ("" = auto)
    std::string _preferredOutputDevice;  // Preferred output device name ("" = auto)
    std::string _selectedInputDevice;    // Actually selected input device name
    std::string _selectedOutputDevice;   // Actually selected output device name

    // Find device by name (for applying preferences)
    PaDeviceIndex findDeviceByName(const std::string& name, bool isInput) const;
};

}

#endif /* !VOICECHATMANAGER_HPP_ */
