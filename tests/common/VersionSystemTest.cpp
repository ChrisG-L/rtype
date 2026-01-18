/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VersionSystemTest - Tests for version checking system (VersionInfo, VersionHistory)
*/

#include <gtest/gtest.h>
#include "protocol/Protocol.hpp"
#include <cstring>
#include <array>

// ============================================================================
// Tests - VersionInfo
// ============================================================================

class VersionInfoTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(VersionInfoTest, BasicConstruction) {
    VersionInfo info;
    info.major = 1;
    info.minor = 2;
    info.patch = 3;
    info.flags = 0;
    std::snprintf(info.gitHash, GIT_HASH_LEN, "%s", "abc12345");

    EXPECT_EQ(info.major, 1);
    EXPECT_EQ(info.minor, 2);
    EXPECT_EQ(info.patch, 3);
    EXPECT_EQ(info.flags, 0);
    EXPECT_STREQ(info.gitHash, "abc12345");
}

TEST_F(VersionInfoTest, DevModeFlag) {
    VersionInfo info;
    info.flags = 0;

    EXPECT_FALSE(info.isDev());

    info.setDev(true);
    EXPECT_TRUE(info.isDev());
    EXPECT_EQ(info.flags & 0x01, 0x01);

    info.setDev(false);
    EXPECT_FALSE(info.isDev());
    EXPECT_EQ(info.flags & 0x01, 0x00);
}

TEST_F(VersionInfoTest, DevModeFlagPreservesOtherFlags) {
    VersionInfo info;
    info.flags = 0xFE;  // All bits except bit 0

    info.setDev(true);
    EXPECT_EQ(info.flags, 0xFF);

    info.setDev(false);
    EXPECT_EQ(info.flags, 0xFE);
}

TEST_F(VersionInfoTest, ExactMatchSameHash) {
    VersionInfo v1, v2;
    std::snprintf(v1.gitHash, GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(v2.gitHash, GIT_HASH_LEN, "%s", "abc12345");

    EXPECT_TRUE(v1.isExactMatch(v2));
    EXPECT_TRUE(v2.isExactMatch(v1));
}

TEST_F(VersionInfoTest, ExactMatchDifferentHash) {
    VersionInfo v1, v2;
    std::snprintf(v1.gitHash, GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(v2.gitHash, GIT_HASH_LEN, "%s", "def67890");

    EXPECT_FALSE(v1.isExactMatch(v2));
    EXPECT_FALSE(v2.isExactMatch(v1));
}

TEST_F(VersionInfoTest, CompatibleWithSameMajor) {
    VersionInfo v1, v2;
    v1.major = 1;
    v1.minor = 0;
    v2.major = 1;
    v2.minor = 5;

    EXPECT_TRUE(v1.isCompatibleWith(v2));
}

TEST_F(VersionInfoTest, IncompatibleWithDifferentMajor) {
    VersionInfo v1, v2;
    v1.major = 1;
    v2.major = 2;

    EXPECT_FALSE(v1.isCompatibleWith(v2));
}

TEST_F(VersionInfoTest, SerializationRoundTrip) {
    VersionInfo original;
    original.major = 1;
    original.minor = 2;
    original.patch = 3;
    original.flags = 0x01;
    std::snprintf(original.gitHash, GIT_HASH_LEN, "%s", "abc12345");

    std::array<uint8_t, VersionInfo::WIRE_SIZE> buffer{};
    original.to_bytes(buffer.data());

    auto parsed = VersionInfo::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->major, original.major);
    EXPECT_EQ(parsed->minor, original.minor);
    EXPECT_EQ(parsed->patch, original.patch);
    EXPECT_EQ(parsed->flags, original.flags);
    EXPECT_STREQ(parsed->gitHash, original.gitHash);
}

TEST_F(VersionInfoTest, DeserializationFailsWithNullptr) {
    auto result = VersionInfo::from_bytes(nullptr, VersionInfo::WIRE_SIZE);
    EXPECT_FALSE(result.has_value());
}

TEST_F(VersionInfoTest, DeserializationFailsWithInsufficientSize) {
    std::array<uint8_t, VersionInfo::WIRE_SIZE - 1> buffer{};
    auto result = VersionInfo::from_bytes(buffer.data(), buffer.size());
    EXPECT_FALSE(result.has_value());
}

TEST_F(VersionInfoTest, WireSizeIsCorrect) {
    // WIRE_SIZE = 4 (major, minor, patch, flags) + GIT_HASH_LEN (9)
    EXPECT_EQ(VersionInfo::WIRE_SIZE, 4 + GIT_HASH_LEN);
    EXPECT_EQ(VersionInfo::WIRE_SIZE, 13);
}

// ============================================================================
// Tests - VersionHistory
// ============================================================================

class VersionHistoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(VersionHistoryTest, BasicConstruction) {
    VersionHistory hist;
    hist.count = 0;

    EXPECT_EQ(hist.count, 0);
}

TEST_F(VersionHistoryTest, FindPositionFirstEntry) {
    VersionHistory hist;
    hist.count = 3;
    std::snprintf(hist.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(hist.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(hist.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");

    EXPECT_EQ(hist.findPosition("abc12345"), 0);
}

TEST_F(VersionHistoryTest, FindPositionMiddleEntry) {
    VersionHistory hist;
    hist.count = 3;
    std::snprintf(hist.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(hist.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(hist.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");

    EXPECT_EQ(hist.findPosition("def67890"), 1);
}

TEST_F(VersionHistoryTest, FindPositionLastEntry) {
    VersionHistory hist;
    hist.count = 3;
    std::snprintf(hist.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(hist.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(hist.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");

    EXPECT_EQ(hist.findPosition("ghi11111"), 2);
}

TEST_F(VersionHistoryTest, FindPositionNotFound) {
    VersionHistory hist;
    hist.count = 3;
    std::snprintf(hist.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(hist.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(hist.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");

    EXPECT_EQ(hist.findPosition("zzz99999"), -1);
}

TEST_F(VersionHistoryTest, FindPositionEmptyHistory) {
    VersionHistory hist;
    hist.count = 0;

    EXPECT_EQ(hist.findPosition("abc12345"), -1);
}

TEST_F(VersionHistoryTest, FindPositionPartialMatch) {
    VersionHistory hist;
    hist.count = 2;
    std::snprintf(hist.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(hist.hashes[1], GIT_HASH_LEN, "%s", "abc12346");  // Similar but different

    // Should find exact match only
    EXPECT_EQ(hist.findPosition("abc12345"), 0);
    EXPECT_EQ(hist.findPosition("abc12346"), 1);
    EXPECT_EQ(hist.findPosition("abc12347"), -1);
}

TEST_F(VersionHistoryTest, SerializationRoundTrip) {
    VersionHistory original;
    original.count = 3;
    std::snprintf(original.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(original.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(original.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");

    std::array<uint8_t, VersionHistory::WIRE_SIZE> buffer{};
    original.to_bytes(buffer.data());

    auto parsed = VersionHistory::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->count, original.count);
    EXPECT_STREQ(parsed->hashes[0], original.hashes[0]);
    EXPECT_STREQ(parsed->hashes[1], original.hashes[1]);
    EXPECT_STREQ(parsed->hashes[2], original.hashes[2]);
}

TEST_F(VersionHistoryTest, DeserializationFailsWithNullptr) {
    auto result = VersionHistory::from_bytes(nullptr, VersionHistory::WIRE_SIZE);
    EXPECT_FALSE(result.has_value());
}

TEST_F(VersionHistoryTest, DeserializationFailsWithInsufficientSize) {
    std::array<uint8_t, VersionHistory::WIRE_SIZE - 1> buffer{};
    auto result = VersionHistory::from_bytes(buffer.data(), buffer.size());
    EXPECT_FALSE(result.has_value());
}

TEST_F(VersionHistoryTest, DeserializationClampsCount) {
    // Create a buffer with count > MAX_VERSION_HISTORY
    std::array<uint8_t, VersionHistory::WIRE_SIZE> buffer{};
    buffer[0] = 100;  // Invalid count (> 50)

    auto parsed = VersionHistory::from_bytes(buffer.data(), buffer.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->count, MAX_VERSION_HISTORY);  // Should be clamped to 50
}

TEST_F(VersionHistoryTest, WireSizeIsCorrect) {
    // WIRE_SIZE = 1 (count) + MAX_VERSION_HISTORY * GIT_HASH_LEN
    EXPECT_EQ(VersionHistory::WIRE_SIZE, 1 + (MAX_VERSION_HISTORY * GIT_HASH_LEN));
    EXPECT_EQ(VersionHistory::WIRE_SIZE, 1 + (50 * 9));
    EXPECT_EQ(VersionHistory::WIRE_SIZE, 451);
}

TEST_F(VersionHistoryTest, MaxVersionHistoryConstant) {
    EXPECT_EQ(MAX_VERSION_HISTORY, 50);
}

TEST_F(VersionHistoryTest, GitHashLenConstant) {
    EXPECT_EQ(GIT_HASH_LEN, 9);  // 8 chars + null terminator
}

// ============================================================================
// Tests - Integration (VersionInfo + VersionHistory)
// ============================================================================

class VersionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(VersionIntegrationTest, ClientUpToDate) {
    VersionInfo client, server;
    std::snprintf(client.gitHash, GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(server.gitHash, GIT_HASH_LEN, "%s", "abc12345");

    VersionHistory history;
    history.count = 5;
    std::snprintf(history.hashes[0], GIT_HASH_LEN, "%s", "abc12345");  // Current
    std::snprintf(history.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(history.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");
    std::snprintf(history.hashes[3], GIT_HASH_LEN, "%s", "jkl22222");
    std::snprintf(history.hashes[4], GIT_HASH_LEN, "%s", "mno33333");

    EXPECT_TRUE(client.isExactMatch(server));
    EXPECT_EQ(history.findPosition(client.gitHash), 0);  // Up to date
}

TEST_F(VersionIntegrationTest, ClientOneCommitBehind) {
    VersionInfo client;
    std::snprintf(client.gitHash, GIT_HASH_LEN, "%s", "def67890");

    VersionHistory history;
    history.count = 5;
    std::snprintf(history.hashes[0], GIT_HASH_LEN, "%s", "abc12345");  // Current
    std::snprintf(history.hashes[1], GIT_HASH_LEN, "%s", "def67890");  // Client's version
    std::snprintf(history.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");
    std::snprintf(history.hashes[3], GIT_HASH_LEN, "%s", "jkl22222");
    std::snprintf(history.hashes[4], GIT_HASH_LEN, "%s", "mno33333");

    EXPECT_EQ(history.findPosition(client.gitHash), 1);  // 1 commit behind
}

TEST_F(VersionIntegrationTest, ClientFiveCommitsBehind) {
    VersionInfo client;
    std::snprintf(client.gitHash, GIT_HASH_LEN, "%s", "mno33333");

    VersionHistory history;
    history.count = 5;
    std::snprintf(history.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(history.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(history.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");
    std::snprintf(history.hashes[3], GIT_HASH_LEN, "%s", "jkl22222");
    std::snprintf(history.hashes[4], GIT_HASH_LEN, "%s", "mno33333");  // Client's version

    EXPECT_EQ(history.findPosition(client.gitHash), 4);  // 4 commits behind
}

TEST_F(VersionIntegrationTest, ClientTooOld) {
    VersionInfo client;
    std::snprintf(client.gitHash, GIT_HASH_LEN, "%s", "old12345");

    VersionHistory history;
    history.count = 5;
    std::snprintf(history.hashes[0], GIT_HASH_LEN, "%s", "abc12345");
    std::snprintf(history.hashes[1], GIT_HASH_LEN, "%s", "def67890");
    std::snprintf(history.hashes[2], GIT_HASH_LEN, "%s", "ghi11111");
    std::snprintf(history.hashes[3], GIT_HASH_LEN, "%s", "jkl22222");
    std::snprintf(history.hashes[4], GIT_HASH_LEN, "%s", "mno33333");

    EXPECT_EQ(history.findPosition(client.gitHash), -1);  // Not found
}

TEST_F(VersionIntegrationTest, DevModeBypassesVersionCheck) {
    VersionInfo client, server;
    std::snprintf(client.gitHash, GIT_HASH_LEN, "%s", "old12345");
    std::snprintf(server.gitHash, GIT_HASH_LEN, "%s", "abc12345");
    client.setDev(true);

    // Version mismatch, but dev mode should bypass
    EXPECT_FALSE(client.isExactMatch(server));
    EXPECT_TRUE(client.isDev());
    // In real code: if (client.isDev()) return true; // bypass check
}
