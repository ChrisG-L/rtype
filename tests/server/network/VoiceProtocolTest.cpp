/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour le protocole Voice Chat
*/

#include <gtest/gtest.h>
#include "Protocol.hpp"
#include <cstring>
#include <vector>

class VoiceProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// VoiceJoin Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceJoinSerialization) {
    VoiceJoin join;
    std::memset(join.token.bytes, 0xAB, TOKEN_SIZE);
    std::memcpy(join.roomCode, "ABC123", ROOM_CODE_LEN);

    uint8_t buf[VoiceJoin::WIRE_SIZE];
    join.to_bytes(buf);

    auto parsed = VoiceJoin::from_bytes(buf, VoiceJoin::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(std::memcmp(parsed->token.bytes, join.token.bytes, TOKEN_SIZE), 0);
    EXPECT_EQ(std::memcmp(parsed->roomCode, "ABC123", ROOM_CODE_LEN), 0);
}

TEST_F(VoiceProtocolTest, VoiceJoinInvalidBuffer) {
    uint8_t buf[10] = {0};
    auto parsed = VoiceJoin::from_bytes(buf, 10);
    EXPECT_FALSE(parsed.has_value());
}

// ============================================================================
// VoiceJoinAck Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceJoinAckSerialization) {
    VoiceJoinAck ack{.player_id = 42};

    uint8_t buf[VoiceJoinAck::WIRE_SIZE];
    ack.to_bytes(buf);

    auto parsed = VoiceJoinAck::from_bytes(buf, VoiceJoinAck::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_id, 42);
}

// ============================================================================
// VoiceFrame Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceFrameMinimalSerialization) {
    VoiceFrame frame;
    frame.speaker_id = 1;
    frame.sequence = 1000;
    frame.opus_len = 0;

    uint8_t buf[VoiceFrame::HEADER_SIZE];
    frame.to_bytes(buf);

    auto parsed = VoiceFrame::from_bytes(buf, VoiceFrame::HEADER_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->speaker_id, 1);
    EXPECT_EQ(parsed->sequence, 1000);
    EXPECT_EQ(parsed->opus_len, 0);
}

TEST_F(VoiceProtocolTest, VoiceFrameWithOpusData) {
    VoiceFrame frame;
    frame.speaker_id = 3;
    frame.sequence = 65535;
    frame.opus_len = 100;
    std::memset(frame.opus_data, 0x42, 100);

    std::vector<uint8_t> buf(VoiceFrame::HEADER_SIZE + 100);
    frame.to_bytes(buf.data());

    auto parsed = VoiceFrame::from_bytes(buf.data(), buf.size());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->speaker_id, 3);
    EXPECT_EQ(parsed->sequence, 65535);
    EXPECT_EQ(parsed->opus_len, 100);
    EXPECT_EQ(parsed->opus_data[0], 0x42);
    EXPECT_EQ(parsed->opus_data[99], 0x42);
}

TEST_F(VoiceProtocolTest, VoiceFrameMaxOpusSize) {
    VoiceFrame frame;
    frame.speaker_id = 255;
    frame.sequence = 32000;
    frame.opus_len = MAX_OPUS_FRAME_SIZE;
    std::memset(frame.opus_data, 0xAA, MAX_OPUS_FRAME_SIZE);

    std::vector<uint8_t> buf(VoiceFrame::MAX_WIRE_SIZE);
    frame.to_bytes(buf.data());

    auto parsed = VoiceFrame::from_bytes(buf.data(), buf.size());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->opus_len, MAX_OPUS_FRAME_SIZE);
    EXPECT_EQ(parsed->opus_data[MAX_OPUS_FRAME_SIZE - 1], 0xAA);
}

TEST_F(VoiceProtocolTest, VoiceFrameSequenceWrap) {
    VoiceFrame frame1{.speaker_id = 1, .sequence = 65534, .opus_len = 0};
    VoiceFrame frame2{.speaker_id = 1, .sequence = 65535, .opus_len = 0};
    VoiceFrame frame3{.speaker_id = 1, .sequence = 0, .opus_len = 0};

    uint8_t buf[VoiceFrame::HEADER_SIZE];

    frame1.to_bytes(buf);
    auto p1 = VoiceFrame::from_bytes(buf, VoiceFrame::HEADER_SIZE);
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1->sequence, 65534);

    frame2.to_bytes(buf);
    auto p2 = VoiceFrame::from_bytes(buf, VoiceFrame::HEADER_SIZE);
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2->sequence, 65535);

    frame3.to_bytes(buf);
    auto p3 = VoiceFrame::from_bytes(buf, VoiceFrame::HEADER_SIZE);
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(p3->sequence, 0);
}

TEST_F(VoiceProtocolTest, VoiceFrameInvalidBuffer) {
    uint8_t buf[3] = {0};
    auto parsed = VoiceFrame::from_bytes(buf, 3);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceFrameTruncatedOpusData) {
    VoiceFrame frame;
    frame.speaker_id = 1;
    frame.sequence = 100;
    frame.opus_len = 200;
    std::memset(frame.opus_data, 0xFF, 200);

    std::vector<uint8_t> buf(VoiceFrame::HEADER_SIZE + 200);
    frame.to_bytes(buf.data());

    auto truncated = VoiceFrame::from_bytes(buf.data(), VoiceFrame::HEADER_SIZE + 100);
    EXPECT_FALSE(truncated.has_value());
}

// ============================================================================
// VoiceMute Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceMuteSerialization) {
    VoiceMute mute{.player_id = 5, .muted = 1};

    uint8_t buf[VoiceMute::WIRE_SIZE];
    mute.to_bytes(buf);

    auto parsed = VoiceMute::from_bytes(buf, VoiceMute::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_id, 5);
    EXPECT_EQ(parsed->muted, 1);
}

TEST_F(VoiceProtocolTest, VoiceMuteUnmute) {
    VoiceMute unmute{.player_id = 2, .muted = 0};

    uint8_t buf[VoiceMute::WIRE_SIZE];
    unmute.to_bytes(buf);

    auto parsed = VoiceMute::from_bytes(buf, VoiceMute::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_id, 2);
    EXPECT_EQ(parsed->muted, 0);
}

// ============================================================================
// VoiceLeave Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceLeaveSerialization) {
    VoiceLeave leave{.player_id = 10};

    uint8_t buf[VoiceLeave::WIRE_SIZE];
    leave.to_bytes(buf);

    auto parsed = VoiceLeave::from_bytes(buf, VoiceLeave::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->player_id, 10);
}

// ============================================================================
// UserSettingsPayload Voice Fields Tests
// ============================================================================

TEST_F(VoiceProtocolTest, UserSettingsVoiceFieldsSerialization) {
    UserSettingsPayload settings;
    std::strncpy(settings.colorBlindMode, "none", COLORBLIND_MODE_LEN - 1);
    settings.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
    settings.gameSpeedPercent = 100;
    std::memset(settings.keyBindings, 0, KEY_BINDINGS_COUNT);
    settings.shipSkin = 1;
    settings.voiceMode = 1;
    settings.vadThreshold = 50;
    settings.micGain = 150;
    settings.voiceVolume = 80;
    std::strncpy(settings.audioInputDevice, "Microphone", AUDIO_DEVICE_NAME_LEN - 1);
    settings.audioInputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';
    std::strncpy(settings.audioOutputDevice, "Speakers", AUDIO_DEVICE_NAME_LEN - 1);
    settings.audioOutputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';

    uint8_t buf[UserSettingsPayload::WIRE_SIZE];
    settings.to_bytes(buf);

    auto parsed = UserSettingsPayload::from_bytes(buf, UserSettingsPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->voiceMode, 1);
    EXPECT_EQ(parsed->vadThreshold, 50);
    EXPECT_EQ(parsed->micGain, 150);
    EXPECT_EQ(parsed->voiceVolume, 80);
    EXPECT_STREQ(parsed->audioInputDevice, "Microphone");
    EXPECT_STREQ(parsed->audioOutputDevice, "Speakers");
}

TEST_F(VoiceProtocolTest, UserSettingsVoiceDefaultValues) {
    UserSettingsPayload settings;
    std::strncpy(settings.colorBlindMode, "none", COLORBLIND_MODE_LEN - 1);
    settings.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
    settings.gameSpeedPercent = 100;
    std::memset(settings.keyBindings, 0, KEY_BINDINGS_COUNT);
    settings.shipSkin = 1;
    settings.voiceMode = 0;
    settings.vadThreshold = 2;
    settings.micGain = 100;
    settings.voiceVolume = 100;
    // Empty string = auto selection
    std::memset(settings.audioInputDevice, 0, AUDIO_DEVICE_NAME_LEN);
    std::memset(settings.audioOutputDevice, 0, AUDIO_DEVICE_NAME_LEN);

    uint8_t buf[UserSettingsPayload::WIRE_SIZE];
    settings.to_bytes(buf);

    auto parsed = UserSettingsPayload::from_bytes(buf, UserSettingsPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->voiceMode, 0);
    EXPECT_EQ(parsed->vadThreshold, 2);
    EXPECT_EQ(parsed->micGain, 100);
    EXPECT_EQ(parsed->voiceVolume, 100);
    EXPECT_STREQ(parsed->audioInputDevice, "");
    EXPECT_STREQ(parsed->audioOutputDevice, "");
}

TEST_F(VoiceProtocolTest, UserSettingsAudioDeviceLongName) {
    UserSettingsPayload settings;
    std::strncpy(settings.colorBlindMode, "none", COLORBLIND_MODE_LEN - 1);
    settings.colorBlindMode[COLORBLIND_MODE_LEN - 1] = '\0';
    settings.gameSpeedPercent = 100;
    std::memset(settings.keyBindings, 0, KEY_BINDINGS_COUNT);
    settings.shipSkin = 1;
    settings.voiceMode = 0;
    settings.vadThreshold = 2;
    settings.micGain = 100;
    settings.voiceVolume = 100;
    // Test long device name (should truncate at 63 chars + null)
    std::strncpy(settings.audioInputDevice,
        "Raptor Lake-P/U/H cAVS Digital Microphone with Very Long Name Extension",
        AUDIO_DEVICE_NAME_LEN - 1);
    settings.audioInputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';
    std::strncpy(settings.audioOutputDevice,
        "Raptor Lake-P/U/H cAVS Speaker + Headphones with Extra Description",
        AUDIO_DEVICE_NAME_LEN - 1);
    settings.audioOutputDevice[AUDIO_DEVICE_NAME_LEN - 1] = '\0';

    uint8_t buf[UserSettingsPayload::WIRE_SIZE];
    settings.to_bytes(buf);

    auto parsed = UserSettingsPayload::from_bytes(buf, UserSettingsPayload::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    // Should be truncated to 63 characters
    EXPECT_EQ(strlen(parsed->audioInputDevice), AUDIO_DEVICE_NAME_LEN - 1);
    EXPECT_EQ(strlen(parsed->audioOutputDevice), AUDIO_DEVICE_NAME_LEN - 1);
}

// ============================================================================
// Edge Cases & Boundary Tests
// ============================================================================

TEST_F(VoiceProtocolTest, VoiceJoinAckInvalidBuffer) {
    auto parsed = VoiceJoinAck::from_bytes(nullptr, 0);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceLeaveInvalidBuffer) {
    auto parsed = VoiceLeave::from_bytes(nullptr, 0);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceMuteInvalidBuffer) {
    uint8_t buf[1] = {0};
    auto parsed = VoiceMute::from_bytes(buf, 1);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceFrameOpusLenExceedsMax) {
    uint8_t buf[VoiceFrame::HEADER_SIZE] = {0};
    buf[0] = 1;  // speaker_id
    // Set opus_len to exceed MAX_OPUS_FRAME_SIZE (big-endian)
    uint16_t invalid_len = MAX_OPUS_FRAME_SIZE + 100;
    buf[3] = (invalid_len >> 8) & 0xFF;
    buf[4] = invalid_len & 0xFF;

    auto parsed = VoiceFrame::from_bytes(buf, VoiceFrame::HEADER_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceJoinNullptrBuffer) {
    auto parsed = VoiceJoin::from_bytes(nullptr, VoiceJoin::WIRE_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceFrameNullptrBuffer) {
    auto parsed = VoiceFrame::from_bytes(nullptr, VoiceFrame::HEADER_SIZE);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, UserSettingsPayloadInvalidBuffer) {
    uint8_t buf[10] = {0};
    auto parsed = UserSettingsPayload::from_bytes(buf, 10);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(VoiceProtocolTest, VoiceFrameWireSizeCalculation) {
    VoiceFrame frame;
    frame.opus_len = 0;
    EXPECT_EQ(frame.wire_size(), VoiceFrame::HEADER_SIZE);

    frame.opus_len = 100;
    EXPECT_EQ(frame.wire_size(), VoiceFrame::HEADER_SIZE + 100);

    frame.opus_len = MAX_OPUS_FRAME_SIZE;
    EXPECT_EQ(frame.wire_size(), VoiceFrame::MAX_WIRE_SIZE);
}
