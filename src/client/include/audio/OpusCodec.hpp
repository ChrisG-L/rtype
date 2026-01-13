/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** OpusCodec - Opus audio encoder/decoder for voice chat
*/

#ifndef OPUSCODEC_HPP_
#define OPUSCODEC_HPP_

#include <vector>
#include <cstdint>
#include <opus/opus.h>

namespace audio {

/**
 * @brief Opus codec wrapper for voice chat encoding/decoding
 *
 * Opus is a highly efficient audio codec designed for real-time communication.
 * This class wraps the Opus encoder and decoder for use in voice chat.
 *
 * Default configuration:
 * - Sample rate: 48000 Hz (optimal for Opus)
 * - Channels: 1 (mono, sufficient for voice)
 * - Frame size: 960 samples (20ms @ 48kHz)
 * - Bitrate: 32000 bps (good quality/bandwidth trade-off)
 *
 * Usage:
 *   OpusCodec codec;
 *   if (codec.init()) {
 *       auto encoded = codec.encode(pcmData, FRAME_SIZE);
 *       auto decoded = codec.decode(encoded.data(), encoded.size());
 *   }
 */
class OpusCodec {
public:
    // Default audio parameters for voice
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CHANNELS = 1;
    static constexpr int FRAME_SIZE = 960;  // 20ms @ 48kHz
    static constexpr int BITRATE = 32000;   // 32 kbps

    // Maximum encoded frame size (enough for any Opus frame)
    static constexpr int MAX_PACKET_SIZE = 480;

    OpusCodec() = default;
    ~OpusCodec();

    // Non-copyable (Opus state is not trivially copyable)
    OpusCodec(const OpusCodec&) = delete;
    OpusCodec& operator=(const OpusCodec&) = delete;

    // Movable
    OpusCodec(OpusCodec&& other) noexcept;
    OpusCodec& operator=(OpusCodec&& other) noexcept;

    /**
     * @brief Initialize the codec with default parameters
     * @return true if initialization succeeded
     */
    bool init();

    /**
     * @brief Initialize the codec with custom parameters
     * @param sampleRate Audio sample rate (8000, 12000, 16000, 24000, 48000)
     * @param channels Number of channels (1 for mono, 2 for stereo)
     * @param bitrate Target bitrate in bits/second
     * @return true if initialization succeeded
     */
    bool init(int sampleRate, int channels, int bitrate);

    /**
     * @brief Shutdown the codec and free resources
     */
    void shutdown();

    /**
     * @brief Check if codec is initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief Encode PCM audio data to Opus
     * @param pcmData Input PCM samples (float, -1.0 to 1.0)
     * @param frameSize Number of samples per channel (e.g., 960 for 20ms @ 48kHz)
     * @return Encoded Opus data, empty on error
     */
    std::vector<uint8_t> encode(const float* pcmData, int frameSize);

    /**
     * @brief Decode Opus data to PCM audio
     * @param opusData Encoded Opus data
     * @param opusSize Size of encoded data in bytes
     * @param frameSize Expected number of samples per channel
     * @return Decoded PCM samples (float, -1.0 to 1.0), empty on error
     */
    std::vector<float> decode(const uint8_t* opusData, int opusSize, int frameSize);

    /**
     * @brief Decode with packet loss concealment (when packet is lost)
     * @param frameSize Expected number of samples per channel
     * @return Concealed PCM samples, empty on error
     */
    std::vector<float> decodePLC(int frameSize);

    /**
     * @brief Get configured sample rate
     */
    int getSampleRate() const { return _sampleRate; }

    /**
     * @brief Get configured number of channels
     */
    int getChannels() const { return _channels; }

    /**
     * @brief Get default frame size (samples per channel)
     */
    int getFrameSize() const { return FRAME_SIZE; }

    /**
     * @brief Set encoder bitrate
     * @param bitrate Target bitrate in bits/second (500 to 512000)
     */
    void setBitrate(int bitrate);

    /**
     * @brief Get current encoder bitrate
     */
    int getBitrate() const { return _bitrate; }

private:
    OpusEncoder* _encoder = nullptr;
    OpusDecoder* _decoder = nullptr;

    bool _initialized = false;
    int _sampleRate = SAMPLE_RATE;
    int _channels = CHANNELS;
    int _bitrate = BITRATE;
};

}

#endif /* !OPUSCODEC_HPP_ */
