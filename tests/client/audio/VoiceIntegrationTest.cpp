/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceIntegrationTest - Integration test for voice pipeline with real audio
*/

#include <gtest/gtest.h>
#include <portaudio.h>
#include <opus/opus.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>

// Simulate the VoiceChatManager audio pipeline

constexpr int SAMPLE_RATE = 48000;
constexpr int FRAME_SIZE = 960;  // 20ms @ 48kHz
constexpr int BITRATE = 32000;

class VoiceIntegrationTest : public ::testing::Test {
protected:
    OpusEncoder* encoder = nullptr;
    OpusDecoder* decoder = nullptr;

    void SetUp() override {
        int error;
        encoder = opus_encoder_create(SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP, &error);
        ASSERT_EQ(error, OPUS_OK);
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));

        decoder = opus_decoder_create(SAMPLE_RATE, 1, &error);
        ASSERT_EQ(error, OPUS_OK);

        PaError err = Pa_Initialize();
        ASSERT_EQ(err, paNoError);
    }

    void TearDown() override {
        if (encoder) opus_encoder_destroy(encoder);
        if (decoder) opus_decoder_destroy(decoder);
        Pa_Terminate();
    }
};

// Simulate sending audio through the pipeline
struct SimulatedPipeline {
    std::queue<std::vector<float>> playbackQueue;
    std::mutex queueMutex;
    std::atomic<int> framesPlayed{0};
    std::atomic<float> maxOutputLevel{0.0f};
    int playbackVolume = 80;
    int outputChannels = 2;

    OpusEncoder* encoder;
    OpusDecoder* decoder;

    // Stats
    std::atomic<int> framesEncoded{0};
    std::atomic<int> framesDecoded{0};
    std::atomic<int> callbackCalls{0};

    static int audioCallback(
        const void* input,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData)
    {
        (void)input; (void)timeInfo; (void)statusFlags;
        auto* self = static_cast<SimulatedPipeline*>(userData);
        auto* out = static_cast<float*>(output);

        self->callbackCalls++;

        // Clear output buffer
        std::memset(out, 0, frameCount * self->outputChannels * sizeof(float));

        std::lock_guard<std::mutex> lock(self->queueMutex);

        if (self->playbackQueue.empty()) {
            return paContinue;
        }

        // Mix from queue
        float volumeScale = static_cast<float>(self->playbackVolume) / 100.0f;
        std::vector<float> monoMix(frameCount, 0.0f);

        size_t outputIndex = 0;
        while (outputIndex < frameCount && !self->playbackQueue.empty()) {
            auto& frame = self->playbackQueue.front();
            size_t samplesNeeded = frameCount - outputIndex;
            size_t samplesToMix = std::min(samplesNeeded, frame.size());

            for (size_t i = 0; i < samplesToMix; ++i) {
                monoMix[outputIndex + i] += frame[i] * volumeScale;
            }
            outputIndex += samplesToMix;

            if (samplesToMix >= frame.size()) {
                self->playbackQueue.pop();
                self->framesPlayed++;
            } else {
                std::vector<float> remaining(frame.begin() + samplesToMix, frame.end());
                self->playbackQueue.pop();
                self->playbackQueue.push(std::move(remaining));
            }
        }

        // Write to output buffer
        float maxSample = 0.0f;
        for (unsigned long i = 0; i < frameCount; ++i) {
            float sample = monoMix[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            if (std::abs(sample) > maxSample) maxSample = std::abs(sample);

            if (self->outputChannels == 2) {
                out[i * 2] = sample;
                out[i * 2 + 1] = sample;
            } else {
                out[i] = sample;
            }
        }

        if (maxSample > self->maxOutputLevel) {
            self->maxOutputLevel = maxSample;
        }

        return paContinue;
    }

    void encodeAndQueue(const std::vector<float>& audio) {
        std::vector<uint8_t> encoded(4000);
        int bytesWritten = opus_encode_float(encoder, audio.data(), FRAME_SIZE,
                                             encoded.data(), encoded.size());
        if (bytesWritten > 0) {
            framesEncoded++;
            encoded.resize(bytesWritten);

            // Decode
            std::vector<float> decoded(FRAME_SIZE);
            int samples = opus_decode_float(decoder, encoded.data(), bytesWritten,
                                            decoded.data(), FRAME_SIZE, 0);
            if (samples > 0) {
                framesDecoded++;
                std::lock_guard<std::mutex> lock(queueMutex);
                playbackQueue.push(std::move(decoded));
            }
        }
    }
};

// This test simulates the full voice pipeline
TEST_F(VoiceIntegrationTest, FullPipelineWithRealAudio) {
    printf("\n[INT] === Full Voice Pipeline Integration Test ===\n");

    SimulatedPipeline pipeline;
    pipeline.encoder = encoder;
    pipeline.decoder = decoder;

    // Open audio stream
    PaStream* stream = nullptr;
    PaError err = Pa_OpenDefaultStream(
        &stream, 0, 2, paFloat32,
        SAMPLE_RATE, FRAME_SIZE,
        SimulatedPipeline::audioCallback, &pipeline
    );

    if (err != paNoError) {
        err = Pa_OpenDefaultStream(
            &stream, 0, 1, paFloat32,
            SAMPLE_RATE, FRAME_SIZE,
            SimulatedPipeline::audioCallback, &pipeline
        );
        pipeline.outputChannels = 1;
    }
    ASSERT_EQ(err, paNoError) << "Failed to open stream: " << Pa_GetErrorText(err);

    printf("[INT] Stream opened with %d channels\n", pipeline.outputChannels);

    // Start stream
    err = Pa_StartStream(stream);
    ASSERT_EQ(err, paNoError) << "Failed to start stream: " << Pa_GetErrorText(err);

    // Generate and queue some audio frames (simulate receiving voice)
    printf("[INT] Generating and queuing 50 audio frames...\n");
    for (int f = 0; f < 50; ++f) {
        std::vector<float> audio(FRAME_SIZE);
        float freq = 200.0f + (f % 10) * 50.0f;  // Varying frequency
        for (int i = 0; i < FRAME_SIZE; ++i) {
            audio[i] = 0.3f * std::sin(2.0f * M_PI * freq * i / SAMPLE_RATE);
        }
        pipeline.encodeAndQueue(audio);
    }

    printf("[INT] Frames encoded: %d, decoded: %d\n",
           pipeline.framesEncoded.load(), pipeline.framesDecoded.load());

    // Let audio play
    printf("[INT] Playing audio for 500ms...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check results
    printf("[INT] Results:\n");
    printf("[INT]   Callback calls: %d\n", pipeline.callbackCalls.load());
    printf("[INT]   Frames played: %d\n", pipeline.framesPlayed.load());
    printf("[INT]   Max output level: %.4f\n", pipeline.maxOutputLevel.load());
    printf("[INT]   Queue remaining: %zu\n", pipeline.playbackQueue.size());

    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    // Verify pipeline worked
    ASSERT_GT(pipeline.callbackCalls, 0) << "Audio callback was never called!";
    ASSERT_GT(pipeline.framesPlayed, 0) << "No frames were played!";
    ASSERT_GT(pipeline.maxOutputLevel, 0.1f) << "No audio was output!";

    printf("[INT] === Pipeline Test PASSED ===\n\n");
}

// Test that queue draining works correctly
TEST_F(VoiceIntegrationTest, QueueDraining) {
    printf("\n[INT] === Queue Draining Test ===\n");

    SimulatedPipeline pipeline;
    pipeline.encoder = encoder;
    pipeline.decoder = decoder;

    // Queue some frames
    for (int f = 0; f < 10; ++f) {
        std::vector<float> audio(FRAME_SIZE);
        for (int i = 0; i < FRAME_SIZE; ++i) {
            audio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
        }
        pipeline.encodeAndQueue(audio);
    }

    printf("[INT] Queued 10 frames, queue size: %zu\n", pipeline.playbackQueue.size());
    ASSERT_EQ(pipeline.playbackQueue.size(), 10u);

    // Simulate callback consuming frames
    std::vector<float> outputBuffer(FRAME_SIZE * 2);  // Stereo

    // Consume 5 frames (5 callback calls with frameCount=960)
    for (int i = 0; i < 5; ++i) {
        SimulatedPipeline::audioCallback(
            nullptr, outputBuffer.data(), FRAME_SIZE,
            nullptr, 0, &pipeline
        );
    }

    printf("[INT] After 5 callbacks, queue size: %zu, frames played: %d\n",
           pipeline.playbackQueue.size(), pipeline.framesPlayed.load());

    ASSERT_EQ(pipeline.framesPlayed, 5) << "Should have played 5 frames";
    ASSERT_EQ(pipeline.playbackQueue.size(), 5u) << "Should have 5 frames remaining";

    // Consume remaining
    for (int i = 0; i < 5; ++i) {
        SimulatedPipeline::audioCallback(
            nullptr, outputBuffer.data(), FRAME_SIZE,
            nullptr, 0, &pipeline
        );
    }

    printf("[INT] After 10 callbacks, queue size: %zu, frames played: %d\n",
           pipeline.playbackQueue.size(), pipeline.framesPlayed.load());

    ASSERT_EQ(pipeline.framesPlayed, 10) << "Should have played all 10 frames";
    ASSERT_EQ(pipeline.playbackQueue.size(), 0u) << "Queue should be empty";

    printf("[INT] === Queue Draining Test PASSED ===\n\n");
}
