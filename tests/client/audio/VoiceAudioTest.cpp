/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceAudioTest - Automated test for voice chat audio pipeline
*/

#include <gtest/gtest.h>
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <vector>
#include <cstdio>

// We'll test the Opus codec directly
#include <opus/opus.h>

// Test constants matching VoiceChatManager
constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAME_SIZE = 960;  // 20ms @ 48kHz
constexpr int BITRATE = 32000;
constexpr int MAX_PACKET_SIZE = 4000;

class VoiceAudioTest : public ::testing::Test {
protected:
    OpusEncoder* encoder = nullptr;
    OpusDecoder* decoder = nullptr;

    void SetUp() override {
        int error;
        encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &error);
        ASSERT_EQ(error, OPUS_OK);
        ASSERT_NE(encoder, nullptr);

        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));
        opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
        opus_encoder_ctl(encoder, OPUS_SET_VBR(1));

        decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &error);
        ASSERT_EQ(error, OPUS_OK);
        ASSERT_NE(decoder, nullptr);
    }

    void TearDown() override {
        if (encoder) opus_encoder_destroy(encoder);
        if (decoder) opus_decoder_destroy(decoder);
    }

    // Generate a sine wave test signal
    std::vector<float> generateSineWave(float frequency, int samples) {
        std::vector<float> signal(samples);
        for (int i = 0; i < samples; ++i) {
            signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / SAMPLE_RATE);
        }
        return signal;
    }

    // Calculate RMS of a signal
    float calculateRMS(const std::vector<float>& signal) {
        float sum = 0.0f;
        for (float s : signal) {
            sum += s * s;
        }
        return std::sqrt(sum / signal.size());
    }

    // Find max absolute value
    float findMax(const std::vector<float>& signal) {
        float maxVal = 0.0f;
        for (float s : signal) {
            if (std::abs(s) > maxVal) maxVal = std::abs(s);
        }
        return maxVal;
    }
};

TEST_F(VoiceAudioTest, OpusEncodeDecodeRoundtrip) {
    // Generate a 440Hz sine wave (A4 note)
    auto input = generateSineWave(440.0f, FRAME_SIZE);

    float inputMax = findMax(input);
    float inputRMS = calculateRMS(input);
    printf("[TEST] Input: %d samples, max=%.4f, RMS=%.4f\n",
           FRAME_SIZE, inputMax, inputRMS);

    ASSERT_GT(inputMax, 0.1f) << "Input signal should have amplitude";

    // Encode
    std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
    int bytesWritten = opus_encode_float(
        encoder,
        input.data(),
        FRAME_SIZE,
        encoded.data(),
        MAX_PACKET_SIZE
    );

    printf("[TEST] Encoded: %d bytes\n", bytesWritten);
    ASSERT_GT(bytesWritten, 0) << "Opus encode should produce bytes";
    encoded.resize(bytesWritten);

    // Decode
    std::vector<float> decoded(FRAME_SIZE * CHANNELS);
    int samplesDecoded = opus_decode_float(
        decoder,
        encoded.data(),
        bytesWritten,
        decoded.data(),
        FRAME_SIZE,
        0  // No FEC
    );

    printf("[TEST] Decoded: %d samples\n", samplesDecoded);
    ASSERT_EQ(samplesDecoded, FRAME_SIZE) << "Should decode full frame";

    float decodedMax = findMax(decoded);
    float decodedRMS = calculateRMS(decoded);
    printf("[TEST] Decoded: max=%.4f, RMS=%.4f\n", decodedMax, decodedRMS);

    // Decoded signal should have similar amplitude (Opus is lossy but preserves energy)
    ASSERT_GT(decodedMax, 0.1f) << "Decoded signal should have amplitude";
    ASSERT_GT(decodedRMS, inputRMS * 0.5f) << "Decoded RMS should be at least 50% of input";
}

TEST_F(VoiceAudioTest, OpusEncodeSilence) {
    // Test with silence - should produce small encoded output
    std::vector<float> silence(FRAME_SIZE, 0.0f);

    std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
    int bytesWritten = opus_encode_float(
        encoder,
        silence.data(),
        FRAME_SIZE,
        encoded.data(),
        MAX_PACKET_SIZE
    );

    printf("[TEST] Silence encoded: %d bytes\n", bytesWritten);
    ASSERT_GT(bytesWritten, 0) << "Even silence should encode to something";

    // Decode silence
    std::vector<float> decoded(FRAME_SIZE);
    int samplesDecoded = opus_decode_float(
        decoder,
        encoded.data(),
        bytesWritten,
        decoded.data(),
        FRAME_SIZE,
        0
    );

    ASSERT_EQ(samplesDecoded, FRAME_SIZE);

    float decodedMax = findMax(decoded);
    printf("[TEST] Decoded silence max: %.6f\n", decodedMax);

    // Decoded silence should be very quiet
    ASSERT_LT(decodedMax, 0.01f) << "Decoded silence should be quiet";
}

TEST_F(VoiceAudioTest, OpusEncodeVoiceLikeSignal) {
    // Simulate voice: multiple frequencies mixed (fundamental + harmonics)
    std::vector<float> voice(FRAME_SIZE, 0.0f);

    // Male voice fundamental ~120Hz with harmonics
    for (int i = 0; i < FRAME_SIZE; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        voice[i] = 0.3f * std::sin(2.0f * M_PI * 120.0f * t);   // Fundamental
        voice[i] += 0.2f * std::sin(2.0f * M_PI * 240.0f * t);  // 2nd harmonic
        voice[i] += 0.1f * std::sin(2.0f * M_PI * 360.0f * t);  // 3rd harmonic
        voice[i] += 0.05f * std::sin(2.0f * M_PI * 480.0f * t); // 4th harmonic
    }

    float inputMax = findMax(voice);
    printf("[TEST] Voice-like input: max=%.4f\n", inputMax);

    // Encode
    std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
    int bytesWritten = opus_encode_float(encoder, voice.data(), FRAME_SIZE,
                                         encoded.data(), MAX_PACKET_SIZE);
    printf("[TEST] Voice encoded: %d bytes\n", bytesWritten);
    ASSERT_GT(bytesWritten, 0);

    // Decode
    std::vector<float> decoded(FRAME_SIZE);
    int samplesDecoded = opus_decode_float(decoder, encoded.data(), bytesWritten,
                                           decoded.data(), FRAME_SIZE, 0);
    ASSERT_EQ(samplesDecoded, FRAME_SIZE);

    float decodedMax = findMax(decoded);
    printf("[TEST] Voice decoded: max=%.4f\n", decodedMax);

    ASSERT_GT(decodedMax, 0.1f) << "Decoded voice should have amplitude";
}

TEST_F(VoiceAudioTest, MultipleFramesSequential) {
    // Test encoding/decoding multiple frames in sequence (like real streaming)
    const int NUM_FRAMES = 10;

    printf("[TEST] Testing %d sequential frames...\n", NUM_FRAMES);

    for (int f = 0; f < NUM_FRAMES; ++f) {
        // Generate different frequency for each frame
        float freq = 200.0f + f * 50.0f;
        auto input = generateSineWave(freq, FRAME_SIZE);

        // Encode
        std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
        int bytesWritten = opus_encode_float(encoder, input.data(), FRAME_SIZE,
                                             encoded.data(), MAX_PACKET_SIZE);
        ASSERT_GT(bytesWritten, 0) << "Frame " << f << " encode failed";

        // Decode
        std::vector<float> decoded(FRAME_SIZE);
        int samplesDecoded = opus_decode_float(decoder, encoded.data(), bytesWritten,
                                               decoded.data(), FRAME_SIZE, 0);
        ASSERT_EQ(samplesDecoded, FRAME_SIZE) << "Frame " << f << " decode failed";

        float decodedMax = findMax(decoded);
        ASSERT_GT(decodedMax, 0.1f) << "Frame " << f << " has no amplitude";
    }

    printf("[TEST] All %d frames OK\n", NUM_FRAMES);
}

TEST_F(VoiceAudioTest, VolumeScaling) {
    // Test that volume scaling works correctly (simulating processPlayback)
    auto input = generateSineWave(440.0f, FRAME_SIZE);

    // Encode/decode
    std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
    int bytesWritten = opus_encode_float(encoder, input.data(), FRAME_SIZE,
                                         encoded.data(), MAX_PACKET_SIZE);
    std::vector<float> decoded(FRAME_SIZE);
    opus_decode_float(decoder, encoded.data(), bytesWritten, decoded.data(), FRAME_SIZE, 0);

    // Apply volume scaling like in processPlayback
    int playbackVolume = 80;  // 80%
    float volumeScale = static_cast<float>(playbackVolume) / 100.0f;

    std::vector<float> scaled(FRAME_SIZE);
    for (int i = 0; i < FRAME_SIZE; ++i) {
        scaled[i] = decoded[i] * volumeScale;
    }

    float decodedMax = findMax(decoded);
    float scaledMax = findMax(scaled);

    printf("[TEST] Volume scaling: decoded=%.4f, scaled(80%%)=%.4f\n",
           decodedMax, scaledMax);

    ASSERT_NEAR(scaledMax, decodedMax * 0.8f, 0.01f)
        << "Volume scaling should reduce by 20%";
}

// Test simulating the full pipeline: capture -> encode -> network -> decode -> playback
TEST_F(VoiceAudioTest, FullPipelineSimulation) {
    printf("\n[TEST] === Full Pipeline Simulation ===\n");

    // Step 1: Simulate microphone capture (generate audio)
    printf("[TEST] Step 1: Generating mic input (440Hz sine)\n");
    auto micInput = generateSineWave(440.0f, FRAME_SIZE);
    float micMax = findMax(micInput);
    printf("[TEST]   Mic input max: %.4f\n", micMax);
    ASSERT_GT(micMax, 0.1f);

    // Step 2: Apply mic gain (like processCapture)
    printf("[TEST] Step 2: Applying mic gain (1.0x)\n");
    float micGain = 1.0f;
    std::vector<float> gainAdjusted(FRAME_SIZE);
    for (int i = 0; i < FRAME_SIZE; ++i) {
        gainAdjusted[i] = micInput[i] * micGain;
        if (gainAdjusted[i] > 1.0f) gainAdjusted[i] = 1.0f;
        if (gainAdjusted[i] < -1.0f) gainAdjusted[i] = -1.0f;
    }

    // Step 3: Opus encode
    printf("[TEST] Step 3: Opus encoding\n");
    std::vector<uint8_t> opusData(MAX_PACKET_SIZE);
    int opusBytes = opus_encode_float(encoder, gainAdjusted.data(), FRAME_SIZE,
                                      opusData.data(), MAX_PACKET_SIZE);
    printf("[TEST]   Opus bytes: %d\n", opusBytes);
    ASSERT_GT(opusBytes, 0);
    opusData.resize(opusBytes);

    // Step 4: Simulate network transfer (just copy the bytes)
    printf("[TEST] Step 4: Simulating network transfer (%zu bytes)\n", opusData.size());
    std::vector<uint8_t> receivedData = opusData;

    // Step 5: Opus decode (receiver side)
    printf("[TEST] Step 5: Opus decoding\n");
    std::vector<float> decoded(FRAME_SIZE);
    int decodedSamples = opus_decode_float(decoder, receivedData.data(),
                                           receivedData.size(), decoded.data(),
                                           FRAME_SIZE, 0);
    printf("[TEST]   Decoded samples: %d\n", decodedSamples);
    ASSERT_EQ(decodedSamples, FRAME_SIZE);

    float decodedMax = findMax(decoded);
    printf("[TEST]   Decoded max: %.4f\n", decodedMax);
    ASSERT_GT(decodedMax, 0.1f) << "Decoded audio should have amplitude!";

    // Step 6: Apply playback volume (like processPlayback)
    printf("[TEST] Step 6: Applying playback volume (80%%)\n");
    int playbackVolume = 80;
    float volumeScale = static_cast<float>(playbackVolume) / 100.0f;

    std::vector<float> monoMix(FRAME_SIZE, 0.0f);
    for (int i = 0; i < FRAME_SIZE; ++i) {
        monoMix[i] = decoded[i] * volumeScale;
    }

    float mixMax = findMax(monoMix);
    printf("[TEST]   Mix max: %.4f\n", mixMax);
    ASSERT_GT(mixMax, 0.05f) << "Mixed audio should have amplitude!";

    // Step 7: Output to stereo buffer (like processPlayback output)
    printf("[TEST] Step 7: Writing to stereo output buffer\n");
    int outputChannels = 2;
    std::vector<float> outputBuffer(FRAME_SIZE * outputChannels, 0.0f);

    for (int i = 0; i < FRAME_SIZE; ++i) {
        float sample = monoMix[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        outputBuffer[i * 2] = sample;      // Left
        outputBuffer[i * 2 + 1] = sample;  // Right
    }

    float outputMax = findMax(outputBuffer);
    printf("[TEST]   Output buffer max: %.4f\n", outputMax);
    ASSERT_GT(outputMax, 0.05f) << "Output buffer should have audio!";

    printf("[TEST] === Pipeline OK! Audio flows correctly ===\n\n");
}

// main() is provided by tests/client/main.cpp
