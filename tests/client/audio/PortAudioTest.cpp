/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PortAudioTest - Test PortAudio output directly
*/

#include <gtest/gtest.h>
#include <portaudio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>

constexpr int SAMPLE_RATE = 48000;
constexpr int FRAME_SIZE = 960;

class PortAudioTest : public ::testing::Test {
protected:
    void SetUp() override {
        PaError err = Pa_Initialize();
        ASSERT_EQ(err, paNoError) << "Pa_Initialize failed: " << Pa_GetErrorText(err);
    }

    void TearDown() override {
        Pa_Terminate();
    }
};

// Callback data for sine wave test
struct SineTestData {
    float frequency = 440.0f;
    float phase = 0.0f;
    int sampleRate = SAMPLE_RATE;
    int channels = 2;
    std::atomic<int> callbackCount{0};
    std::atomic<float> maxOutput{0.0f};
};

static int sineCallback(
    [[maybe_unused]] const void* input,
    void* output,
    unsigned long frameCount,
    [[maybe_unused]] const PaStreamCallbackTimeInfo* timeInfo,
    [[maybe_unused]] PaStreamCallbackFlags statusFlags,
    void* userData)
{
    auto* data = static_cast<SineTestData*>(userData);
    auto* out = static_cast<float*>(output);

    float phaseIncrement = 2.0f * M_PI * data->frequency / data->sampleRate;

    for (unsigned long i = 0; i < frameCount; ++i) {
        float sample = 0.3f * std::sin(data->phase);
        data->phase += phaseIncrement;
        if (data->phase > 2.0f * M_PI) {
            data->phase -= 2.0f * M_PI;
        }

        if (std::abs(sample) > data->maxOutput) {
            data->maxOutput = std::abs(sample);
        }

        if (data->channels == 2) {
            out[i * 2] = sample;      // Left
            out[i * 2 + 1] = sample;  // Right
        } else {
            out[i] = sample;
        }
    }

    data->callbackCount++;
    return paContinue;
}



// This test plays an actual tone - enable it to verify audio output
TEST_F(PortAudioTest, DISABLED_PlayTone) {
    PaStream* stream = nullptr;
    SineTestData data;
    data.frequency = 440.0f;  // A4 note

    PaError err = Pa_OpenDefaultStream(
        &stream, 0, 2, paFloat32,
        SAMPLE_RATE, FRAME_SIZE,
        sineCallback, &data
    );

    if (err != paNoError) {
        err = Pa_OpenDefaultStream(
            &stream, 0, 1, paFloat32,
            SAMPLE_RATE, FRAME_SIZE,
            sineCallback, &data
        );
        data.channels = 1;
    }
    ASSERT_EQ(err, paNoError);

    err = Pa_StartStream(stream);
    ASSERT_EQ(err, paNoError) << "Failed to start stream: " << Pa_GetErrorText(err);

    printf("[PA] Playing 440Hz tone for 1 second...\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    printf("[PA] Callback called %d times, maxOutput=%.4f\n",
           data.callbackCount.load(), data.maxOutput.load());

    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    ASSERT_GT(data.callbackCount, 0) << "Callback was never called!";
    ASSERT_GT(data.maxOutput, 0.1f) << "No audio was generated!";
}


// Test with JACK API specifically (for PipeWire compatibility)
TEST_F(PortAudioTest, CheckJackApi) {
    printf("\n[PA] Checking for JACK host API...\n");

    PaHostApiIndex apiCount = Pa_GetHostApiCount();
    printf("[PA] Found %d host APIs:\n", apiCount);

    PaHostApiIndex jackApiIndex = -1;
    for (PaHostApiIndex i = 0; i < apiCount; ++i) {
        const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(i);
        if (apiInfo) {
            printf("[PA]   [%d] %s (type=%d, devices=%d)\n",
                   i, apiInfo->name, apiInfo->type, apiInfo->deviceCount);
            if (apiInfo->type == paJACK) {
                jackApiIndex = i;
            }
        }
    }

    if (jackApiIndex >= 0) {
        printf("[PA] JACK API found at index %d\n", jackApiIndex);
        const PaHostApiInfo* jackInfo = Pa_GetHostApiInfo(jackApiIndex);
        printf("[PA] JACK has %d devices\n", jackInfo->deviceCount);

        // List JACK devices
        for (int d = 0; d < jackInfo->deviceCount; ++d) {
            PaDeviceIndex devIndex = Pa_HostApiDeviceIndexToDeviceIndex(jackApiIndex, d);
            const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(devIndex);
            if (devInfo) {
                printf("[PA]   JACK device [%d]: %s (in=%d, out=%d)\n",
                       devIndex, devInfo->name,
                       devInfo->maxInputChannels, devInfo->maxOutputChannels);
            }
        }
    } else {
        printf("[PA] WARNING: JACK API not found! PipeWire voice may not work.\n");
        printf("[PA] Make sure to run with: pw-jack ./rtype_client\n");
    }
}

// ============================================================================
// Cross-Platform Device Selection Tests
// ============================================================================

// Test virtual device detection
TEST_F(PortAudioTest, VirtualDeviceDetection) {
    printf("\n[PA] === Virtual Device Detection Test ===\n");

    // Test patterns that should be detected as virtual
    auto isVirtualDevice = [](const std::string& name) -> bool {
        if (name.find("WEBRTC") != std::string::npos) return true;
        if (name.find("speech-dispatcher") != std::string::npos) return true;
        if (name.find("Firefox") != std::string::npos) return true;
        if (name.find("Chrome") != std::string::npos) return true;
        if (name.find("Chromium") != std::string::npos) return true;
        if (name.find("Discord") != std::string::npos) return true;
        if (name.find("Skype") != std::string::npos) return true;
        if (name.find("Teams") != std::string::npos) return true;
        if (name.find("Zoom") != std::string::npos) return true;
        if (name.find("Microsoft Sound Mapper") != std::string::npos) return true;
        if (name.find("VB-Audio") != std::string::npos) return true;
        if (name.find("Virtual Cable") != std::string::npos) return true;
        if (name.find("CABLE") != std::string::npos) return true;
        if (name.find("Loopback") != std::string::npos) return true;
        if (name.find("Monitor") != std::string::npos && name.find("playback") == std::string::npos) return true;
        return false;
    };

    // Test known virtual device names
    EXPECT_TRUE(isVirtualDevice("WEBRTC VoiceEngine"));
    EXPECT_TRUE(isVirtualDevice("Firefox"));
    EXPECT_TRUE(isVirtualDevice("Chrome Input"));
    EXPECT_TRUE(isVirtualDevice("speech-dispatcher-dummy"));
    EXPECT_TRUE(isVirtualDevice("VB-Audio Virtual Cable"));
    EXPECT_TRUE(isVirtualDevice("CABLE Input"));
    EXPECT_TRUE(isVirtualDevice("Discord"));
    EXPECT_TRUE(isVirtualDevice("Microsoft Sound Mapper - Input"));

    // Test known hardware device names that should NOT be virtual
    EXPECT_FALSE(isVirtualDevice("Realtek High Definition Audio"));
    EXPECT_FALSE(isVirtualDevice("Speaker + Headphones"));
    EXPECT_FALSE(isVirtualDevice("Raptor Lake-P/U/H cAVS"));
    EXPECT_FALSE(isVirtualDevice("Microphone Array"));
    EXPECT_FALSE(isVirtualDevice("HDMI Output"));
    EXPECT_FALSE(isVirtualDevice("ALC256 Analog"));
    EXPECT_FALSE(isVirtualDevice("HDA Intel PCH"));

    printf("[PA] Virtual device detection: PASSED\n");
}

// Test preferred output device detection
TEST_F(PortAudioTest, PreferredOutputDetection) {
    printf("\n[PA] === Preferred Output Device Detection Test ===\n");

    auto isPreferredOutput = [](const std::string& name) -> bool {
        if (name.find("Speaker") != std::string::npos) return true;
        if (name.find("speaker") != std::string::npos) return true;
        if (name.find("Headphone") != std::string::npos) return true;
        if (name.find("headphone") != std::string::npos) return true;
        if (name.find("Speakers") != std::string::npos) return true;
        if (name.find("Output") != std::string::npos) return true;
        if (name.find("playback") != std::string::npos) return true;
        if (name.find("Realtek") != std::string::npos) return true;
        if (name.find("High Definition Audio") != std::string::npos) return true;
        if (name.find("cAVS") != std::string::npos) return true;
        if (name.find("HDA") != std::string::npos) return true;
        if (name.find("HDMI") != std::string::npos) return true;
        if (name.find("ALC") != std::string::npos) return true;
        return false;
    };

    // Windows typical names
    EXPECT_TRUE(isPreferredOutput("Speakers (Realtek High Definition Audio)"));
    EXPECT_TRUE(isPreferredOutput("Realtek Digital Output"));
    EXPECT_TRUE(isPreferredOutput("HDMI Output (AMD High Definition Audio)"));
    EXPECT_TRUE(isPreferredOutput("Headphones (High Definition Audio)"));

    // Linux typical names
    EXPECT_TRUE(isPreferredOutput("Raptor Lake-P/U/H cAVS Speaker + Headphones"));
    EXPECT_TRUE(isPreferredOutput("HDA Intel PCH Speaker"));
    EXPECT_TRUE(isPreferredOutput("ALC256 Analog Output"));
    EXPECT_TRUE(isPreferredOutput("Built-in Audio Analog Stereo playback"));

    // macOS typical names
    EXPECT_TRUE(isPreferredOutput("Built-in Output"));
    EXPECT_TRUE(isPreferredOutput("MacBook Pro Speakers"));

    printf("[PA] Preferred output detection: PASSED\n");
}

// Test preferred input device detection
TEST_F(PortAudioTest, PreferredInputDetection) {
    printf("\n[PA] === Preferred Input Device Detection Test ===\n");

    auto isPreferredInput = [](const std::string& name) -> bool {
        if (name.find("Microphone") != std::string::npos) return true;
        if (name.find("microphone") != std::string::npos) return true;
        if (name.find("Mic") != std::string::npos) return true;
        if (name.find("mic") != std::string::npos) return true;
        if (name.find("capture") != std::string::npos) return true;
        if (name.find("Input") != std::string::npos) return true;
        if (name.find("Realtek") != std::string::npos) return true;
        if (name.find("High Definition Audio") != std::string::npos) return true;
        if (name.find("cAVS") != std::string::npos) return true;
        if (name.find("HDA") != std::string::npos) return true;
        if (name.find("ALC") != std::string::npos) return true;
        return false;
    };

    // Windows typical names
    EXPECT_TRUE(isPreferredInput("Microphone (Realtek High Definition Audio)"));
    EXPECT_TRUE(isPreferredInput("Microphone Array (Intel Smart Sound)"));
    EXPECT_TRUE(isPreferredInput("Line In (High Definition Audio)"));

    // Linux typical names
    EXPECT_TRUE(isPreferredInput("Raptor Lake-P/U/H cAVS Digital Microphone"));
    EXPECT_TRUE(isPreferredInput("HDA Intel Mic"));
    EXPECT_TRUE(isPreferredInput("ALC256 Analog capture"));
    EXPECT_TRUE(isPreferredInput("Built-in Audio Analog Stereo Input"));

    // macOS typical names
    EXPECT_TRUE(isPreferredInput("Built-in Microphone"));
    EXPECT_TRUE(isPreferredInput("MacBook Pro Microphone"));

    printf("[PA] Preferred input detection: PASSED\n");
}

// Test device selection priority with simulated device list
TEST_F(PortAudioTest, DeviceSelectionPriority) {
    printf("\n[PA] === Device Selection Priority Test ===\n");

    // Simulate a device list like on Linux with PipeWire
    struct SimDevice {
        const char* name;
        int inputChannels;
        int outputChannels;
        bool expectedVirtual;
        bool expectedPreferredOutput;
        bool expectedPreferredInput;
    };

    SimDevice devices[] = {
        {"Raptor Lake-P/U/H cAVS Headphones Stereo Microphone", 2, 0, false, false, true},
        {"Raptor Lake-P/U/H cAVS Digital Microphone", 2, 0, false, false, true},
        {"speech-dispatcher-dummy", 2, 0, true, false, false},
        {"Firefox", 2, 0, true, false, false},
        {"WEBRTC VoiceEngine", 4, 2, true, false, false},
        {"Raptor Lake-P/U/H cAVS Speaker + Headphones", 2, 2, false, true, true},
        {"Raptor Lake-P/U/H cAVS HDMI / DisplayPort 1 Output", 2, 2, false, true, false},
    };

    auto isVirtual = [](const std::string& name) -> bool {
        if (name.find("WEBRTC") != std::string::npos) return true;
        if (name.find("speech-dispatcher") != std::string::npos) return true;
        if (name.find("Firefox") != std::string::npos) return true;
        return false;
    };

    auto isPreferredOutput = [](const std::string& name) -> bool {
        if (name.find("Speaker") != std::string::npos) return true;
        if (name.find("Headphone") != std::string::npos) return true;
        if (name.find("cAVS") != std::string::npos) return true;
        if (name.find("HDMI") != std::string::npos) return true;
        return false;
    };

    auto isPreferredInput = [](const std::string& name) -> bool {
        if (name.find("Microphone") != std::string::npos) return true;
        if (name.find("cAVS") != std::string::npos && name.find("HDMI") == std::string::npos) return true;
        return false;
    };

    // Simulate device selection algorithm
    int selectedInput = -1;
    int selectedOutput = -1;
    int fallbackInput = -1;
    int fallbackOutput = -1;

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
        std::string name(devices[i].name);
        bool isVirt = isVirtual(name);

        // Output selection
        if (devices[i].outputChannels >= 1 && !isVirt) {
            if (fallbackOutput < 0) fallbackOutput = i;
            if (selectedOutput < 0 && isPreferredOutput(name)) {
                selectedOutput = i;
            }
        }

        // Input selection
        if (devices[i].inputChannels >= 1 && !isVirt) {
            if (fallbackInput < 0) fallbackInput = i;
            if (selectedInput < 0 && isPreferredInput(name)) {
                selectedInput = i;
            }
        }
    }

    printf("[PA] Selected output: %d (%s)\n", selectedOutput,
           selectedOutput >= 0 ? devices[selectedOutput].name : "none");
    printf("[PA] Selected input: %d (%s)\n", selectedInput,
           selectedInput >= 0 ? devices[selectedInput].name : "none");

    // Should select "Speaker + Headphones" for output (index 5)
    EXPECT_EQ(selectedOutput, 5);
    // Should select "Headphones Stereo Microphone" for input (index 0)
    EXPECT_EQ(selectedInput, 0);

    // Verify WEBRTC VoiceEngine was NOT selected (even though it has output)
    EXPECT_NE(selectedOutput, 4);

    printf("[PA] Device selection priority: PASSED\n");
}

