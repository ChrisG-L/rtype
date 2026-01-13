/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceChatManager - Implementation
*/

#include "audio/VoiceChatManager.hpp"
#include "core/Logger.hpp"
#include <cstring>
#include <cmath>
#include <thread>
#include <vector>
#include <cstdio>
#include <string>
#include <algorithm>

namespace audio {

using boost::asio::ip::udp;

// Speaker timeout - remove speaker state after 2 seconds of inactivity
static constexpr auto SPEAKER_TIMEOUT = std::chrono::seconds(2);

VoiceChatManager& VoiceChatManager::getInstance() {
    static VoiceChatManager instance;
    return instance;
}

VoiceChatManager::~VoiceChatManager() noexcept {
    try {
        shutdown();
    } catch (...) {
        // Destructors must not throw exceptions (MISRA C++ rule)
        // Silently ignore any exceptions during shutdown
    }
}

bool VoiceChatManager::init() {
    auto logger = client::logging::Logger::getAudioLogger();
    if (_initialized) {
        logger->warn("VoiceChatManager already initialized");
        return true;
    }

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        logger->error("PortAudio init failed: {}", Pa_GetErrorText(err));
        return false;
    }

    // Initialize Opus codec
    if (!_codec.init()) {
        logger->error("Opus codec init failed");
        Pa_Terminate();
        return false;
    }

    // Allocate buffers
    _captureBuffer.reserve(OpusCodec::FRAME_SIZE * 4);
    _mixBuffer.resize(OpusCodec::FRAME_SIZE);

    // Find suitable input and output devices
    // Priority: 1) User-selected device, 2) Auto-select hardware, 3) System default
    // Cross-platform compatible (Linux/Windows/macOS)
    PaDeviceIndex inputDevice = paNoDevice;
    PaDeviceIndex outputDevice = paNoDevice;
    PaDeviceIndex fallbackOutput = paNoDevice;
    PaDeviceIndex fallbackInput = paNoDevice;

    int numDevices = Pa_GetDeviceCount();
    logger->debug("Found {} audio devices", numDevices);

    // Step 1: Try to use user-selected devices if configured
    if (!_preferredInputDevice.empty()) {
        inputDevice = findDeviceByName(_preferredInputDevice, true);
        if (inputDevice != paNoDevice) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(inputDevice);
            logger->info("Using user-selected input device: {} [{}]", info->name, inputDevice);
            _selectedInputDevice = info->name;
        } else {
            logger->warn("User-selected input device '{}' not found, using auto-selection", _preferredInputDevice);
        }
    }

    if (!_preferredOutputDevice.empty()) {
        outputDevice = findDeviceByName(_preferredOutputDevice, false);
        if (outputDevice != paNoDevice) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(outputDevice);
            logger->info("Using user-selected output device: {} [{}]", info->name, outputDevice);
            _selectedOutputDevice = info->name;
        } else {
            logger->warn("User-selected output device '{}' not found, using auto-selection", _preferredOutputDevice);
        }
    }

    // Step 2: Auto-select devices if not user-specified

    // Virtual device patterns to skip (browser/application virtual devices)
    // These are common across platforms
    auto isVirtualDevice = [](const std::string& name) -> bool {
        // Browser/application virtual devices (Linux/Windows)
        if (name.find("WEBRTC") != std::string::npos) return true;
        if (name.find("speech-dispatcher") != std::string::npos) return true;
        if (name.find("Firefox") != std::string::npos) return true;
        if (name.find("Chrome") != std::string::npos) return true;
        if (name.find("Chromium") != std::string::npos) return true;
        if (name.find("Discord") != std::string::npos) return true;
        if (name.find("Skype") != std::string::npos) return true;
        if (name.find("Teams") != std::string::npos) return true;
        if (name.find("Zoom") != std::string::npos) return true;
        // Windows virtual devices
        if (name.find("Microsoft Sound Mapper") != std::string::npos) return true;
        if (name.find("VB-Audio") != std::string::npos) return true;
        if (name.find("Virtual Cable") != std::string::npos) return true;
        if (name.find("CABLE") != std::string::npos) return true;
        // Linux loopback
        if (name.find("Loopback") != std::string::npos) return true;
        if (name.find("Monitor") != std::string::npos && name.find("playback") == std::string::npos) return true;
        return false;
    };

    // Check if device name suggests it's a preferred hardware device
    auto isPreferredOutput = [](const std::string& name) -> bool {
        // Common hardware output indicators (case-insensitive check would be better)
        if (name.find("Speaker") != std::string::npos) return true;
        if (name.find("speaker") != std::string::npos) return true;
        if (name.find("Headphone") != std::string::npos) return true;
        if (name.find("headphone") != std::string::npos) return true;
        if (name.find("Speakers") != std::string::npos) return true;
        if (name.find("Output") != std::string::npos) return true;
        if (name.find("playback") != std::string::npos) return true;
        // Windows common names
        if (name.find("Realtek") != std::string::npos) return true;
        if (name.find("High Definition Audio") != std::string::npos) return true;
        // Linux Intel/AMD sound
        if (name.find("cAVS") != std::string::npos) return true;
        if (name.find("HDA") != std::string::npos) return true;
        if (name.find("HDMI") != std::string::npos) return true;
        if (name.find("ALC") != std::string::npos) return true;  // Realtek ALSA
        return false;
    };

    auto isPreferredInput = [](const std::string& name) -> bool {
        if (name.find("Microphone") != std::string::npos) return true;
        if (name.find("microphone") != std::string::npos) return true;
        if (name.find("Mic") != std::string::npos) return true;
        if (name.find("mic") != std::string::npos) return true;
        if (name.find("capture") != std::string::npos) return true;
        if (name.find("Input") != std::string::npos) return true;
        // Windows
        if (name.find("Realtek") != std::string::npos) return true;
        if (name.find("High Definition Audio") != std::string::npos) return true;
        // Linux
        if (name.find("cAVS") != std::string::npos) return true;
        if (name.find("HDA") != std::string::npos) return true;
        if (name.find("ALC") != std::string::npos) return true;
        return false;
    };

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        std::string name(info->name);
        logger->debug("Device [{}]: {} (in={}, out={})", i, name,
                     info->maxInputChannels, info->maxOutputChannels);

        bool isVirtual = isVirtualDevice(name);

        // Output device selection
        if (info->maxOutputChannels >= 1 && !isVirtual) {
            // First non-virtual device with output as fallback
            if (fallbackOutput == paNoDevice) {
                fallbackOutput = i;
            }
            // Prefer devices that look like hardware speakers/headphones
            if (outputDevice == paNoDevice && isPreferredOutput(name)) {
                outputDevice = i;
                logger->info("Selected output device: {} [{}]", name, i);
            }
        }

        // Input device selection
        if (info->maxInputChannels >= 1 && !isVirtual) {
            // First non-virtual device with input as fallback
            if (fallbackInput == paNoDevice) {
                fallbackInput = i;
            }
            // Prefer devices that look like microphones
            if (inputDevice == paNoDevice && isPreferredInput(name)) {
                inputDevice = i;
                logger->info("Selected input device: {} [{}]", name, i);
            }
        }
    }

    // Use fallback non-virtual devices if no preferred device found
    if (outputDevice == paNoDevice && fallbackOutput != paNoDevice) {
        outputDevice = fallbackOutput;
        const PaDeviceInfo* info = Pa_GetDeviceInfo(outputDevice);
        logger->info("Using fallback output device: {} [{}]", info ? info->name : "unknown", outputDevice);
    }
    if (inputDevice == paNoDevice && fallbackInput != paNoDevice) {
        inputDevice = fallbackInput;
        const PaDeviceInfo* info = Pa_GetDeviceInfo(inputDevice);
        logger->info("Using fallback input device: {} [{}]", info ? info->name : "unknown", inputDevice);
    }

    // Last resort: use system defaults (may include virtual devices)
    if (outputDevice == paNoDevice) {
        outputDevice = Pa_GetDefaultOutputDevice();
        if (outputDevice != paNoDevice) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(outputDevice);
            logger->warn("Using system default output device: {}", info ? info->name : "unknown");
        }
    }
    if (inputDevice == paNoDevice) {
        inputDevice = Pa_GetDefaultInputDevice();
        if (inputDevice != paNoDevice) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(inputDevice);
            logger->warn("Using system default input device: {}", info ? info->name : "unknown");
        }
    }

    // Store selected device names for API access
    if (inputDevice != paNoDevice && _selectedInputDevice.empty()) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(inputDevice);
        if (info) _selectedInputDevice = info->name;
    }
    if (outputDevice != paNoDevice && _selectedOutputDevice.empty()) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(outputDevice);
        if (info) _selectedOutputDevice = info->name;
    }

    // Setup stream parameters
    PaStreamParameters inputParams, outputParams;
    PaStreamParameters* inputParamsPtr = nullptr;
    PaStreamParameters* outputParamsPtr = nullptr;

    if (inputDevice != paNoDevice) {
        const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(inputDevice);
        inputParams.device = inputDevice;
        inputParams.channelCount = 1;  // Mono input
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = inputInfo->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;
        inputParamsPtr = &inputParams;
    }

    if (outputDevice != paNoDevice) {
        const PaDeviceInfo* outputInfo = Pa_GetDeviceInfo(outputDevice);
        outputParams.device = outputDevice;
        outputParams.channelCount = std::min(2, outputInfo->maxOutputChannels);  // Stereo if available
        outputParams.sampleFormat = paFloat32;
        outputParams.suggestedLatency = outputInfo->defaultLowOutputLatency;
        outputParams.hostApiSpecificStreamInfo = nullptr;
        outputParamsPtr = &outputParams;
    }

    if (!inputParamsPtr && !outputParamsPtr) {
        logger->error("No audio devices available");
        _codec.shutdown();
        Pa_Terminate();
        return false;
    }

    // Open the stream with explicit device selection
    err = Pa_OpenStream(
        &_stream,
        inputParamsPtr,
        outputParamsPtr,
        OpusCodec::SAMPLE_RATE,
        OpusCodec::FRAME_SIZE,
        paNoFlag,
        audioCallback,
        this
    );

    if (err != paNoError) {
        logger->error("Failed to open audio stream: {}", Pa_GetErrorText(err));
        _codec.shutdown();
        Pa_Terminate();
        return false;
    }

    _inputChannels = inputParamsPtr ? inputParams.channelCount : 0;
    _outputChannels = outputParamsPtr ? outputParams.channelCount : 0;
    logger->info("Audio stream opened: {} input, {} output channels",
                 _inputChannels, _outputChannels);

    // Start the audio stream
    err = Pa_StartStream(_stream);
    if (err != paNoError) {
        logger->error("PortAudio stream start failed: {}", Pa_GetErrorText(err));
        Pa_CloseStream(_stream);
        _stream = nullptr;
        _codec.shutdown();
        Pa_Terminate();
        return false;
    }

    // Connecter les ports JACK manuellement (nécessaire pour PipeWire)
    connectJackPorts();

    _initialized = true;
    logger->info("VoiceChatManager initialized");
    return true;
}

void VoiceChatManager::shutdown() {
    if (!_initialized) {
        return;
    }

    disconnect();

    if (_stream) {
        Pa_StopStream(_stream);
        Pa_CloseStream(_stream);
        _stream = nullptr;
    }

    _codec.shutdown();
    Pa_Terminate();

    _initialized = false;
    auto shutdownLogger = client::logging::Logger::getAudioLogger();
    if (shutdownLogger) {
        shutdownLogger->info("VoiceChatManager shutdown");
    }
}

bool VoiceChatManager::connect(const std::string& host, uint16_t port) {
    auto logger = client::logging::Logger::getAudioLogger();
    if (_connected) {
        logger->warn("Already connected to voice server");
        return true;
    }

    if (!_initialized) {
        logger->error("VoiceChatManager not initialized");
        return false;
    }

    try {
        // Resolve server address
        udp::resolver resolver(_ioContext);
        auto endpoints = resolver.resolve(udp::v4(), host, std::to_string(port));
        _serverEndpoint = *endpoints.begin();

        // Create socket
        _socket = std::make_unique<udp::socket>(_ioContext, udp::endpoint(udp::v4(), 0));

        // Start network thread
        _networkThread = std::jthread([this](std::stop_token stopToken) {
            // Keep io_context running until stopped
            auto work = boost::asio::make_work_guard(_ioContext);

            while (!stopToken.stop_requested()) {
                try {
                    _ioContext.run_for(std::chrono::milliseconds(100));
                } catch (const std::exception& e) {
                    auto threadLogger = client::logging::Logger::getAudioLogger();
                    if (threadLogger) {
                        threadLogger->error("Voice network error: {}", e.what());
                    }
                }
            }

            work.reset();
        });

        // Start receiving
        startReceive();

        _connected = true;
        logger->info("Connected to voice server {}:{}", host, port);
        return true;

    } catch (const std::exception& e) {
        logger->error("Failed to connect to voice server: {}", e.what());
        return false;
    }
}

void VoiceChatManager::disconnect() {
    if (!_connected) {
        return;
    }

    // Leave voice channel first
    if (!_currentRoomCode.empty()) {
        leaveVoiceChannel();
    }

    _connected = false;

    // Stop network thread
    if (_networkThread.joinable()) {
        _networkThread.request_stop();
        _ioContext.stop();
        _networkThread.join();
    }

    // Close socket
    if (_socket) {
        boost::system::error_code ec;
        _socket->close(ec);
        _socket.reset();
    }

    // Reset io_context for potential reconnection
    _ioContext.restart();

    auto disconnectLogger = client::logging::Logger::getAudioLogger();
    if (disconnectLogger) {
        disconnectLogger->info("Disconnected from voice server");
    }
}

void VoiceChatManager::joinVoiceChannel(const SessionToken& token, const std::string& roomCode) {
    if (!_connected) {
        client::logging::Logger::getAudioLogger()->error("Cannot join voice channel: not connected");
        return;
    }

    // Leave previous channel if any
    if (!_currentRoomCode.empty()) {
        leaveVoiceChannel();
    }

    _currentRoomCode = roomCode;
    sendVoiceJoin(token, roomCode);
    client::logging::Logger::getAudioLogger()->info("Joining voice channel for room '{}'", roomCode);
}

void VoiceChatManager::leaveVoiceChannel() {
    if (_currentRoomCode.empty()) {
        return;
    }

    sendVoiceLeave();

    // Clear speaker states
    {
        std::lock_guard<std::mutex> lock(_playbackMutex);
        _speakers.clear();
    }

    auto logger = client::logging::Logger::getAudioLogger();
    if (logger) {
        logger->info("Left voice channel for room '{}'", _currentRoomCode);
    }
    _currentRoomCode.clear();
    _localPlayerId = 0;
}

void VoiceChatManager::startTalking() {
    if (_muted || !_connected) {
        return;
    }
    _talking = true;
}

void VoiceChatManager::stopTalking() {
    _talking = false;
}

void VoiceChatManager::setMuted(bool muted) {
    _muted = muted;
    if (muted) {
        _talking = false;
    }

    // Notify server of mute state
    if (_connected && !_currentRoomCode.empty()) {
        sendVoiceMute(muted);
    }
}

void VoiceChatManager::applySettings(uint8_t voiceMode, uint8_t vadThreshold, uint8_t micGain, uint8_t voiceVolume) {
    _voiceMode = (voiceMode == 1) ? VoiceMode::VoiceActivity : VoiceMode::PushToTalk;
    _vadThreshold = static_cast<float>(vadThreshold) / 100.0f;
    _micGain = static_cast<float>(micGain) / 100.0f;
    _playbackVolume = static_cast<int>(voiceVolume);

    client::logging::Logger::getAudioLogger()->debug("Voice settings applied: mode={}, vadThreshold={:.2f}, micGain={:.2f}, volume={}",
        voiceMode == 1 ? "VAD" : "PTT", _vadThreshold, _micGain, _playbackVolume);
}

std::vector<uint8_t> VoiceChatManager::getActiveSpeakers() const {
    std::vector<uint8_t> active;
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(_playbackMutex));

    for (const auto& [playerId, state] : _speakers) {
        if (state.speaking) {
            active.push_back(playerId);
        }
    }

    return active;
}

void VoiceChatManager::update() {
    if (!_initialized || !_connected) {
        return;
    }

    // Clean up inactive speakers
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(_playbackMutex);
    for (auto it = _speakers.begin(); it != _speakers.end(); ) {
        if (now - it->second.lastActivity > SPEAKER_TIMEOUT) {
            it = _speakers.erase(it);
        } else {
            // Update speaking state based on whether they have audio to play
            it->second.speaking = !it->second.frames.empty();
            ++it;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// PortAudio Callback
// ═══════════════════════════════════════════════════════════════════

int VoiceChatManager::audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    [[maybe_unused]] const PaStreamCallbackTimeInfo* timeInfo,
    [[maybe_unused]] PaStreamCallbackFlags statusFlags,
    void* userData)
{
    auto* self = static_cast<VoiceChatManager*>(userData);

    // Process microphone input
    if (inputBuffer) {
        self->processCapture(static_cast<const float*>(inputBuffer), framesPerBuffer);
    }

    // Process speaker output
    if (outputBuffer) {
        self->processPlayback(static_cast<float*>(outputBuffer), framesPerBuffer);
    }

    return paContinue;
}

void VoiceChatManager::processCapture(const float* inputBuffer, unsigned long frameCount) {
    // No input channels available
    if (_inputChannels == 0 || inputBuffer == nullptr) {
        return;
    }

    // Check if we should be transmitting
    bool shouldTransmit = false;

    if (_voiceMode == VoiceMode::PushToTalk) {
        shouldTransmit = _talking && !_muted;
    } else {  // VoiceActivity
        shouldTransmit = !_muted && detectVoiceActivity(inputBuffer, frameCount);
    }

    if (!shouldTransmit || !_connected || _currentRoomCode.empty()) {
        return;
    }

    // Apply microphone gain
    std::vector<float> gainAdjusted(frameCount);
    for (unsigned long i = 0; i < frameCount; ++i) {
        gainAdjusted[i] = inputBuffer[i] * _micGain;
        // Clamp to prevent clipping
        if (gainAdjusted[i] > 1.0f) gainAdjusted[i] = 1.0f;
        if (gainAdjusted[i] < -1.0f) gainAdjusted[i] = -1.0f;
    }

    // Add to capture buffer
    {
        std::lock_guard<std::mutex> lock(_captureMutex);
        _captureBuffer.insert(_captureBuffer.end(), gainAdjusted.begin(), gainAdjusted.end());

        // Encode and send when we have enough samples
        while (_captureBuffer.size() >= OpusCodec::FRAME_SIZE) {
            auto encoded = _codec.encode(_captureBuffer.data(), OpusCodec::FRAME_SIZE);
            if (!encoded.empty()) {
                sendVoiceFrame(encoded);
            }

            // Remove processed samples
            _captureBuffer.erase(
                _captureBuffer.begin(),
                _captureBuffer.begin() + OpusCodec::FRAME_SIZE
            );
        }
    }
}

void VoiceChatManager::processPlayback(float* outputBuffer, unsigned long frameCount) {
    // Clear output buffer first
    if (_outputChannels > 0) {
        std::memset(outputBuffer, 0, frameCount * static_cast<size_t>(_outputChannels) * sizeof(float));
    }

    if (!_connected || _outputChannels == 0) {
        return;  // No output channels available
    }

    std::lock_guard<std::mutex> lock(_playbackMutex);

    // Mix all speakers together into mono first
    std::vector<float> monoMix(frameCount, 0.0f);
    float volumeScale = static_cast<float>(_playbackVolume) / 100.0f;

    for (auto& [playerId, state] : _speakers) {
        size_t outputIndex = 0;

        // Process frames until we fill the output buffer
        while (outputIndex < frameCount && !state.frames.empty()) {
            auto& frame = state.frames.front();

            // How many samples to take from this frame
            size_t samplesNeeded = frameCount - outputIndex;
            size_t samplesAvailable = frame.size();
            size_t samplesToMix = std::min(samplesNeeded, samplesAvailable);

            // Mix into mono buffer
            for (size_t i = 0; i < samplesToMix; ++i) {
                monoMix[outputIndex + i] += frame[i] * volumeScale;
            }
            outputIndex += samplesToMix;

            // If we used the entire frame, remove it
            if (samplesToMix >= samplesAvailable) {
                state.frames.pop();
            } else {
                // Partial consumption - remove used samples from front
                // Since std::queue doesn't support partial removal, we need to copy remaining
                std::vector<float> remaining(frame.begin() + samplesToMix, frame.end());
                state.frames.pop();
                if (!remaining.empty()) {
                    state.frames.push(std::move(remaining));
                }
            }
        }
    }

    // Output based on channel count
    for (unsigned long i = 0; i < frameCount; ++i) {
        float sample = monoMix[i];
        // Clamp to prevent clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        if (_outputChannels == 2) {
            // Stereo: duplicate to both channels (interleaved: L R L R)
            outputBuffer[i * 2] = sample;      // Left
            outputBuffer[i * 2 + 1] = sample;  // Right
        } else if (_outputChannels == 1) {
            // Mono output
            outputBuffer[i] = sample;
        }
    }
}

float VoiceChatManager::calculateRMS(const float* buffer, size_t samples) const {
    float sum = 0.0f;
    for (size_t i = 0; i < samples; ++i) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / static_cast<float>(samples));
}

bool VoiceChatManager::detectVoiceActivity(const float* buffer, size_t samples) const {
    float rms = calculateRMS(buffer, samples);
    return rms > _vadThreshold;
}

// ═══════════════════════════════════════════════════════════════════
// Network Functions
// ═══════════════════════════════════════════════════════════════════

void VoiceChatManager::startReceive() {
    if (!_socket) {
        return;
    }

    _socket->async_receive_from(
        boost::asio::buffer(_recvBuffer),
        _senderEndpoint,
        [this](const boost::system::error_code& error, std::size_t bytes) {
            handleReceive(error, bytes);
        }
    );
}

void VoiceChatManager::handleReceive(const boost::system::error_code& error, std::size_t bytes) {
    if (error) {
        if (error != boost::asio::error::operation_aborted) {
            auto recvLogger = client::logging::Logger::getAudioLogger();
            if (recvLogger) {
                recvLogger->error("Voice receive error: {}", error.message());
            }
            startReceive();
        }
        return;
    }

    if (bytes < UDPHeader::WIRE_SIZE) {
        startReceive();
        return;
    }

    auto headerOpt = UDPHeader::from_bytes(_recvBuffer.data(), bytes);
    if (!headerOpt) {
        startReceive();
        return;
    }

    const auto& header = *headerOpt;
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(_recvBuffer.data()) + UDPHeader::WIRE_SIZE;
    size_t payloadSize = bytes - UDPHeader::WIRE_SIZE;

    switch (static_cast<MessageType>(header.type)) {
        case MessageType::VoiceJoinAck: {
            if (payloadSize >= VoiceJoinAck::WIRE_SIZE) {
                auto ackOpt = VoiceJoinAck::from_bytes(payload, payloadSize);
                if (ackOpt) {
                    _localPlayerId = ackOpt->player_id;
                    client::logging::Logger::getAudioLogger()->info("Joined voice channel as player {}",
                                static_cast<int>(_localPlayerId));
                }
            }
            break;
        }

        case MessageType::VoiceFrame: {
            if (payloadSize >= VoiceFrame::HEADER_SIZE) {
                auto frameOpt = VoiceFrame::from_bytes(payload, payloadSize);
                if (frameOpt) {
                    // Log receipt of voice frame
                    static int frameCounter = 0;
                    if (++frameCounter % 50 == 1) {  // Log every 50 frames
                        client::logging::Logger::getAudioLogger()->debug(
                            "VoiceFrame received: speaker={}, seq={}, opus_len={}, localId={}",
                            static_cast<int>(frameOpt->speaker_id),
                            frameOpt->sequence,
                            frameOpt->opus_len,
                            static_cast<int>(_localPlayerId));
                    }

                    if (frameOpt->speaker_id != _localPlayerId) {
                        // Decode and queue for playback
                        auto decoded = _codec.decode(
                            frameOpt->opus_data,
                            frameOpt->opus_len,
                            OpusCodec::FRAME_SIZE
                        );

                        if (!decoded.empty()) {
                            std::lock_guard<std::mutex> lock(_playbackMutex);
                            auto& speaker = _speakers[frameOpt->speaker_id];
                            speaker.frames.push(std::move(decoded));
                            speaker.lastActivity = std::chrono::steady_clock::now();
                            speaker.speaking = true;
                        }
                    }
                }
            }
            break;
        }

        case MessageType::VoiceMute: {
            if (payloadSize >= VoiceMute::WIRE_SIZE) {
                auto muteOpt = VoiceMute::from_bytes(payload, payloadSize);
                if (muteOpt) {
                    std::lock_guard<std::mutex> lock(_playbackMutex);
                    // Clear speaker's buffer if they muted
                    if (muteOpt->muted && _speakers.contains(muteOpt->player_id)) {
                        while (!_speakers[muteOpt->player_id].frames.empty()) {
                            _speakers[muteOpt->player_id].frames.pop();
                        }
                        _speakers[muteOpt->player_id].speaking = false;
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    startReceive();
}

void VoiceChatManager::sendVoiceJoin(const SessionToken& token, const std::string& roomCode) {
    const size_t totalSize = UDPHeader::WIRE_SIZE + VoiceJoin::WIRE_SIZE;
    std::vector<uint8_t> buf(totalSize);

    UDPHeader header{
        .type = static_cast<uint16_t>(MessageType::VoiceJoin),
        .sequence_num = 0,
        .timestamp = UDPHeader::getTimestamp()
    };
    header.to_bytes(buf.data());

    VoiceJoin join;
    join.token = token;
    std::memset(join.roomCode, 0, ROOM_CODE_LEN);
    std::strncpy(join.roomCode, roomCode.c_str(), ROOM_CODE_LEN - 1);
    join.roomCode[ROOM_CODE_LEN - 1] = '\0';
    join.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

    boost::system::error_code ec;
    _socket->send_to(boost::asio::buffer(buf), _serverEndpoint, 0, ec);
    if (ec) {
        client::logging::Logger::getAudioLogger()->error("Failed to send VoiceJoin: {}", ec.message());
    }
}

void VoiceChatManager::sendVoiceLeave() {
    const size_t totalSize = UDPHeader::WIRE_SIZE + VoiceLeave::WIRE_SIZE;
    std::vector<uint8_t> buf(totalSize);

    UDPHeader header{
        .type = static_cast<uint16_t>(MessageType::VoiceLeave),
        .sequence_num = 0,
        .timestamp = UDPHeader::getTimestamp()
    };
    header.to_bytes(buf.data());

    VoiceLeave leave{.player_id = _localPlayerId};
    leave.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

    boost::system::error_code ec;
    _socket->send_to(boost::asio::buffer(buf), _serverEndpoint, 0, ec);
}

void VoiceChatManager::sendVoiceFrame(const std::vector<uint8_t>& opusData) {
    if (opusData.size() > MAX_OPUS_FRAME_SIZE) {
        client::logging::Logger::getAudioLogger()->warn("Opus frame too large: {}", opusData.size());
        return;
    }

    VoiceFrame frame;
    frame.speaker_id = _localPlayerId;
    frame.sequence = _sequenceNum++;
    frame.opus_len = static_cast<uint16_t>(opusData.size());
    std::memcpy(frame.opus_data, opusData.data(), opusData.size());

    const size_t totalSize = UDPHeader::WIRE_SIZE + frame.wire_size();
    std::vector<uint8_t> buf(totalSize);

    UDPHeader header{
        .type = static_cast<uint16_t>(MessageType::VoiceFrame),
        .sequence_num = _sequenceNum,
        .timestamp = UDPHeader::getTimestamp()
    };
    header.to_bytes(buf.data());
    frame.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

    boost::system::error_code ec;
    _socket->send_to(boost::asio::buffer(buf), _serverEndpoint, 0, ec);
}

void VoiceChatManager::sendVoiceMute(bool muted) {
    const size_t totalSize = UDPHeader::WIRE_SIZE + VoiceMute::WIRE_SIZE;
    std::vector<uint8_t> buf(totalSize);

    UDPHeader header{
        .type = static_cast<uint16_t>(MessageType::VoiceMute),
        .sequence_num = 0,
        .timestamp = UDPHeader::getTimestamp()
    };
    header.to_bytes(buf.data());

    VoiceMute vm{.player_id = _localPlayerId, .muted = static_cast<uint8_t>(muted ? 1 : 0)};
    vm.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

    boost::system::error_code ec;
    _socket->send_to(boost::asio::buffer(buf), _serverEndpoint, 0, ec);
}

void VoiceChatManager::connectJackPorts() {
    auto logger = client::logging::Logger::getAudioLogger();

    if (_outputChannels == 0) {
        logger->debug("Voice output disabled (no output channels)");
        return;
    }

    logger->info("Audio stream ready: {} input channel(s), {} output channel(s)",
                 _inputChannels, _outputChannels);
}

// ═══════════════════════════════════════════════════════════════════
// Audio Device Selection API
// ═══════════════════════════════════════════════════════════════════

std::vector<VoiceChatManager::AudioDeviceInfo> VoiceChatManager::getInputDevices() const {
    std::vector<AudioDeviceInfo> devices;

    // Add "Auto (default)" as first option
    devices.push_back({
        .index = -1,
        .name = "Auto (default)",
        .maxInputChannels = 0,
        .maxOutputChannels = 0,
        .defaultSampleRate = 48000,
        .isDefault = true
    });

    // PortAudio must be initialized to enumerate devices
    bool needsTerminate = false;
    if (!_initialized) {
        if (Pa_Initialize() != paNoError) {
            return devices;
        }
        needsTerminate = true;
    }

    PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
    int numDevices = Pa_GetDeviceCount();

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info || info->maxInputChannels < 1) continue;

        std::string name(info->name);

        // Skip virtual/browser devices
        if (name.find("WEBRTC") != std::string::npos ||
            name.find("speech-dispatcher") != std::string::npos ||
            name.find("Firefox") != std::string::npos ||
            name.find("Chrome") != std::string::npos ||
            name.find("Discord") != std::string::npos) {
            continue;
        }

        devices.push_back({
            .index = i,
            .name = name,
            .maxInputChannels = info->maxInputChannels,
            .maxOutputChannels = info->maxOutputChannels,
            .defaultSampleRate = info->defaultSampleRate,
            .isDefault = (i == defaultInput)
        });
    }

    if (needsTerminate) {
        Pa_Terminate();
    }

    return devices;
}

std::vector<VoiceChatManager::AudioDeviceInfo> VoiceChatManager::getOutputDevices() const {
    std::vector<AudioDeviceInfo> devices;

    // Add "Auto (default)" as first option
    devices.push_back({
        .index = -1,
        .name = "Auto (default)",
        .maxInputChannels = 0,
        .maxOutputChannels = 0,
        .defaultSampleRate = 48000,
        .isDefault = true
    });

    // PortAudio must be initialized to enumerate devices
    bool needsTerminate = false;
    if (!_initialized) {
        if (Pa_Initialize() != paNoError) {
            return devices;
        }
        needsTerminate = true;
    }

    PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();
    int numDevices = Pa_GetDeviceCount();

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info || info->maxOutputChannels < 1) continue;

        std::string name(info->name);

        // Skip virtual/browser devices
        if (name.find("WEBRTC") != std::string::npos ||
            name.find("speech-dispatcher") != std::string::npos ||
            name.find("Firefox") != std::string::npos ||
            name.find("Chrome") != std::string::npos ||
            name.find("Discord") != std::string::npos) {
            continue;
        }

        devices.push_back({
            .index = i,
            .name = name,
            .maxInputChannels = info->maxInputChannels,
            .maxOutputChannels = info->maxOutputChannels,
            .defaultSampleRate = info->defaultSampleRate,
            .isDefault = (i == defaultOutput)
        });
    }

    if (needsTerminate) {
        Pa_Terminate();
    }

    return devices;
}

void VoiceChatManager::setPreferredInputDevice(const std::string& deviceName) {
    _preferredInputDevice = deviceName;
    client::logging::Logger::getAudioLogger()->info("Preferred input device set to: {}",
        deviceName.empty() ? "Auto" : deviceName);
}

void VoiceChatManager::setPreferredOutputDevice(const std::string& deviceName) {
    _preferredOutputDevice = deviceName;
    client::logging::Logger::getAudioLogger()->info("Preferred output device set to: {}",
        deviceName.empty() ? "Auto" : deviceName);
}

void VoiceChatManager::setSelectedDevices(const std::string& inputDeviceName, const std::string& outputDeviceName) {
    // Store the selected device names (for persistence/loading from server)
    // These will be used when SettingsScene opens to display the correct selection
    _selectedInputDevice = inputDeviceName;
    _selectedOutputDevice = outputDeviceName;
    // Also set as preferred so they'll be used on next init()
    _preferredInputDevice = inputDeviceName;
    _preferredOutputDevice = outputDeviceName;

    auto logger = client::logging::Logger::getAudioLogger();
    logger->info("Selected devices set - input='{}', output='{}'",
        inputDeviceName.empty() ? "Auto" : inputDeviceName,
        outputDeviceName.empty() ? "Auto" : outputDeviceName);
}

PaDeviceIndex VoiceChatManager::findDeviceByName(const std::string& name, bool isInput) const {
    if (name.empty()) {
        return paNoDevice;  // Use auto-selection
    }

    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        // Check if device has required channels
        if (isInput && info->maxInputChannels < 1) continue;
        if (!isInput && info->maxOutputChannels < 1) continue;

        // Exact name match
        if (std::string(info->name) == name) {
            return i;
        }
    }

    // Try partial match (device names may vary slightly)
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        if (isInput && info->maxInputChannels < 1) continue;
        if (!isInput && info->maxOutputChannels < 1) continue;

        std::string deviceName(info->name);
        // Check if the stored name is a substring or vice versa
        if (deviceName.find(name) != std::string::npos ||
            name.find(deviceName) != std::string::npos) {
            return i;
        }
    }

    return paNoDevice;  // Not found, will use auto-selection
}

bool VoiceChatManager::applyAudioDevices(const std::string& inputDeviceName,
                                          const std::string& outputDeviceName) {
    auto logger = client::logging::Logger::getAudioLogger();

    // Store preferences AND selected devices (for persistence)
    _preferredInputDevice = inputDeviceName;
    _preferredOutputDevice = outputDeviceName;
    _selectedInputDevice = inputDeviceName;
    _selectedOutputDevice = outputDeviceName;

    // If already initialized, restart to apply changes immediately
    if (_initialized) {
        logger->info("Audio device change requested - restarting audio system");

        // Save connection state
        bool wasConnected = _connected;
        std::string savedHost;
        std::string savedRoomCode = _currentRoomCode;

        if (wasConnected && _socket) {
            savedHost = _serverEndpoint.address().to_string();
        }

        // Shutdown audio system (but keep preferences)
        shutdown();

        // Reinitialize with new devices
        if (!init()) {
            logger->error("Failed to reinitialize audio with new devices");
            return false;
        }

        // Note: Cannot auto-reconnect to voice channel because we don't store the session token.
        // The user will need to rejoin manually if they were in a game.
        if (wasConnected && !savedHost.empty()) {
            logger->info("Audio devices changed. Voice connection was reset - please rejoin if in game.");
        } else {
            logger->info("Audio devices changed successfully");
        }

        return true;
    }

    return true;
}

}
