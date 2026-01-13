/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** OpusCodec - Implementation
*/

#include "audio/OpusCodec.hpp"
#include "core/Logger.hpp"

namespace audio {

OpusCodec::~OpusCodec() {
    shutdown();
}

OpusCodec::OpusCodec(OpusCodec&& other) noexcept
    : _encoder(other._encoder),
      _decoder(other._decoder),
      _initialized(other._initialized),
      _sampleRate(other._sampleRate),
      _channels(other._channels),
      _bitrate(other._bitrate) {
    other._encoder = nullptr;
    other._decoder = nullptr;
    other._initialized = false;
}

OpusCodec& OpusCodec::operator=(OpusCodec&& other) noexcept {
    if (this != &other) {
        shutdown();
        _encoder = other._encoder;
        _decoder = other._decoder;
        _initialized = other._initialized;
        _sampleRate = other._sampleRate;
        _channels = other._channels;
        _bitrate = other._bitrate;

        other._encoder = nullptr;
        other._decoder = nullptr;
        other._initialized = false;
    }
    return *this;
}

bool OpusCodec::init() {
    return init(SAMPLE_RATE, CHANNELS, BITRATE);
}

bool OpusCodec::init(int sampleRate, int channels, int bitrate) {
    auto logger = client::logging::Logger::getAudioLogger();
    if (_initialized) {
        logger->warn("OpusCodec already initialized");
        return true;
    }

    _sampleRate = sampleRate;
    _channels = channels;
    _bitrate = bitrate;

    int error;

    // Create encoder
    _encoder = opus_encoder_create(_sampleRate, _channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK || !_encoder) {
        logger->error("Failed to create Opus encoder: {}", opus_strerror(error));
        return false;
    }

    // Configure encoder for voice
    opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(_bitrate));
    opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(_encoder, OPUS_SET_VBR(1));  // Variable bitrate
    opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(5));  // Medium complexity

    // Create decoder
    _decoder = opus_decoder_create(_sampleRate, _channels, &error);
    if (error != OPUS_OK || !_decoder) {
        logger->error("Failed to create Opus decoder: {}", opus_strerror(error));
        opus_encoder_destroy(_encoder);
        _encoder = nullptr;
        return false;
    }

    _initialized = true;
    logger->info("OpusCodec initialized ({}Hz, {} ch, {} bps)",
                 _sampleRate, _channels, _bitrate);
    return true;
}

void OpusCodec::shutdown() {
    if (_encoder) {
        opus_encoder_destroy(_encoder);
        _encoder = nullptr;
    }
    if (_decoder) {
        opus_decoder_destroy(_decoder);
        _decoder = nullptr;
    }
    _initialized = false;
}

std::vector<uint8_t> OpusCodec::encode(const float* pcmData, int frameSize) {
    if (!_initialized || !_encoder || !pcmData) {
        return {};
    }

    std::vector<uint8_t> encoded(MAX_PACKET_SIZE);

    int bytesWritten = opus_encode_float(
        _encoder,
        pcmData,
        frameSize,
        encoded.data(),
        static_cast<opus_int32>(encoded.size())
    );

    if (bytesWritten < 0) {
        client::logging::Logger::getAudioLogger()->error("Opus encode failed: {}", opus_strerror(bytesWritten));
        return {};
    }

    encoded.resize(static_cast<size_t>(bytesWritten));
    return encoded;
}

std::vector<float> OpusCodec::decode(const uint8_t* opusData, int opusSize, int frameSize) {
    if (!_initialized || !_decoder || !opusData || opusSize <= 0) {
        return {};
    }

    std::vector<float> decoded(static_cast<size_t>(frameSize * _channels));

    int samplesDecoded = opus_decode_float(
        _decoder,
        opusData,
        opusSize,
        decoded.data(),
        frameSize,
        0  // No FEC
    );

    if (samplesDecoded <= 0) {
        return {};
    }

    decoded.resize(static_cast<size_t>(samplesDecoded * _channels));
    return decoded;
}

std::vector<float> OpusCodec::decodePLC(int frameSize) {
    if (!_initialized || !_decoder) {
        return {};
    }

    std::vector<float> decoded(static_cast<size_t>(frameSize * _channels));

    // Pass nullptr to trigger packet loss concealment
    int samplesDecoded = opus_decode_float(
        _decoder,
        nullptr,
        0,
        decoded.data(),
        frameSize,
        0
    );

    if (samplesDecoded < 0) {
        client::logging::Logger::getAudioLogger()->error("Opus PLC failed: {}", opus_strerror(samplesDecoded));
        return {};
    }

    decoded.resize(static_cast<size_t>(samplesDecoded * _channels));
    return decoded;
}

void OpusCodec::setBitrate(int bitrate) {
    _bitrate = bitrate;
    if (_encoder) {
        opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(_bitrate));
    }
}

}
