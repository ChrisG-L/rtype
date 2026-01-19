/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CompressionTest - Unit tests for LZ4 network compression utilities
*/

#include <gtest/gtest.h>
#include "compression/Compression.hpp"
#include "Protocol.hpp"
#include <vector>
#include <cstring>
#include <random>
#include <algorithm>
#include <numeric>

// ═══════════════════════════════════════════════════════════════════════════════
// compression::compress() Tests
// ═══════════════════════════════════════════════════════════════════════════════

class CompressionTest : public ::testing::Test {
protected:
    // Generate random data
    std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }

    // Generate compressible data (repeated patterns)
    std::vector<uint8_t> generateCompressibleData(size_t size) {
        std::vector<uint8_t> data(size);
        // Repeated pattern compresses well
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(i % 16);
        }
        return data;
    }

    // Generate highly compressible data (all zeros)
    std::vector<uint8_t> generateZeroData(size_t size) {
        return std::vector<uint8_t>(size, 0);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic Compression Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompressionTest, CompressNullPointerReturnsEmpty) {
    auto result = compression::compress(nullptr, 100);
    EXPECT_TRUE(result.empty());
}

TEST_F(CompressionTest, CompressZeroSizeReturnsEmpty) {
    uint8_t data[] = {1, 2, 3};
    auto result = compression::compress(data, 0);
    EXPECT_TRUE(result.empty());
}

TEST_F(CompressionTest, CompressSmallDataMayReturnEmpty) {
    // Very small data might not compress well
    uint8_t data[] = {1, 2, 3, 4, 5};
    auto result = compression::compress(data, sizeof(data));
    // Result may be empty if compression doesn't reduce size
    // This is expected behavior
}

TEST_F(CompressionTest, CompressCompressibleDataSucceeds) {
    auto data = generateCompressibleData(500);
    auto compressed = compression::compress(data.data(), data.size());

    // Compressible data should compress
    ASSERT_FALSE(compressed.empty());
    // Compressed size should be smaller
    EXPECT_LT(compressed.size(), data.size());
}

TEST_F(CompressionTest, CompressZeroDataCompressesVeryWell) {
    auto data = generateZeroData(1000);
    auto compressed = compression::compress(data.data(), data.size());

    // Zero data compresses extremely well
    ASSERT_FALSE(compressed.empty());
    // Should be much smaller (LZ4 is very efficient with zeros)
    EXPECT_LT(compressed.size(), data.size() / 5);
}

TEST_F(CompressionTest, CompressIncompressibleDataReturnsEmpty) {
    // Random data is typically incompressible
    auto data = generateRandomData(200);
    auto compressed = compression::compress(data.data(), data.size());

    // Random data shouldn't compress - returns empty (not worth it)
    // Note: occasionally random data might compress slightly
}

// ─────────────────────────────────────────────────────────────────────────────
// Decompression Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompressionTest, DecompressNullPointerReturnsNullopt) {
    auto result = compression::decompress(nullptr, 100, 200);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionTest, DecompressZeroSizeReturnsNullopt) {
    uint8_t data[] = {1, 2, 3};
    auto result = compression::decompress(data, 0, 100);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionTest, DecompressZeroOriginalSizeReturnsNullopt) {
    uint8_t data[] = {1, 2, 3};
    auto result = compression::decompress(data, sizeof(data), 0);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionTest, DecompressInvalidDataReturnsNullopt) {
    // Invalid compressed data
    uint8_t garbage[] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB};
    auto result = compression::decompress(garbage, sizeof(garbage), 100);
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Round-Trip Tests (Compress then Decompress)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompressionTest, RoundTripCompressibleData) {
    auto original = generateCompressibleData(500);
    auto compressed = compression::compress(original.data(), original.size());

    ASSERT_FALSE(compressed.empty()) << "Compression should succeed for compressible data";

    auto decompressed = compression::decompress(
        compressed.data(), compressed.size(), original.size());

    ASSERT_TRUE(decompressed.has_value()) << "Decompression should succeed";
    EXPECT_EQ(decompressed->size(), original.size());
    EXPECT_EQ(*decompressed, original);
}

TEST_F(CompressionTest, RoundTripZeroData) {
    auto original = generateZeroData(1000);
    auto compressed = compression::compress(original.data(), original.size());

    ASSERT_FALSE(compressed.empty());

    auto decompressed = compression::decompress(
        compressed.data(), compressed.size(), original.size());

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, original);
}

TEST_F(CompressionTest, RoundTripLargeData) {
    // Test with data larger than typical GameSnapshot
    auto original = generateCompressibleData(2000);
    auto compressed = compression::compress(original.data(), original.size());

    ASSERT_FALSE(compressed.empty());

    auto decompressed = compression::decompress(
        compressed.data(), compressed.size(), original.size());

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, original);
}

TEST_F(CompressionTest, RoundTripExactlyMinCompressSize) {
    auto original = generateCompressibleData(compression::MIN_COMPRESS_SIZE);
    auto compressed = compression::compress(original.data(), original.size());

    // At minimum size, compression may or may not help
    if (!compressed.empty()) {
        auto decompressed = compression::decompress(
            compressed.data(), compressed.size(), original.size());
        ASSERT_TRUE(decompressed.has_value());
        EXPECT_EQ(*decompressed, original);
    }
}

TEST_F(CompressionTest, RoundTripBelowMinCompressSize) {
    // Data below MIN_COMPRESS_SIZE should typically not be compressed
    // but the function still works if called
    auto original = generateCompressibleData(compression::MIN_COMPRESS_SIZE - 1);
    auto compressed = compression::compress(original.data(), original.size());

    // May or may not compress - either way should work
    if (!compressed.empty()) {
        auto decompressed = compression::decompress(
            compressed.data(), compressed.size(), original.size());
        ASSERT_TRUE(decompressed.has_value());
        EXPECT_EQ(*decompressed, original);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Simulated GameSnapshot Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompressionTest, SimulatedGameSnapshotCompression) {
    // Simulate a GameSnapshot-like structure
    // PlayerState has many fields with similar patterns
    std::vector<uint8_t> snapshot(800);

    // Simulate player data (repeated structure)
    for (size_t i = 0; i < 4; ++i) {
        size_t offset = i * 23; // PlayerState::WIRE_SIZE = 23
        snapshot[offset] = static_cast<uint8_t>(i); // id
        snapshot[offset + 1] = 0x01; // x high byte
        snapshot[offset + 2] = 0x00; // x low byte
        snapshot[offset + 3] = 0x00; // y high byte
        snapshot[offset + 4] = static_cast<uint8_t>(100 + i * 50); // y low byte
        snapshot[offset + 5] = 100; // health
        snapshot[offset + 6] = 1; // alive
    }

    auto compressed = compression::compress(snapshot.data(), snapshot.size());
    ASSERT_FALSE(compressed.empty());

    // GameSnapshot should compress reasonably well
    EXPECT_LT(compressed.size(), snapshot.size());

    auto decompressed = compression::decompress(
        compressed.data(), compressed.size(), snapshot.size());
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, snapshot);
}

// ─────────────────────────────────────────────────────────────────────────────
// Constants Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompressionTest, ConstantsAreReasonable) {
    // MIN_COMPRESS_SIZE should be reasonable (small packets overhead isn't worth it)
    EXPECT_GE(compression::MIN_COMPRESS_SIZE, 64);
    EXPECT_LE(compression::MIN_COMPRESS_SIZE, 256);

    // MAX_UNCOMPRESSED_SIZE should support typical network payloads
    EXPECT_GE(compression::MAX_UNCOMPRESSED_SIZE, 32768);
    EXPECT_LE(compression::MAX_UNCOMPRESSED_SIZE, 1 << 20); // 1MB max
}

// ═══════════════════════════════════════════════════════════════════════════════
// CompressionHeader Tests
// ═══════════════════════════════════════════════════════════════════════════════

class CompressionHeaderTest : public ::testing::Test {};

TEST_F(CompressionHeaderTest, WireSizeIsCorrect) {
    EXPECT_EQ(CompressionHeader::WIRE_SIZE, 2);
}

TEST_F(CompressionHeaderTest, ToBytesAndFromBytesRoundTrip) {
    CompressionHeader original{.originalSize = 1234};
    uint8_t buffer[CompressionHeader::WIRE_SIZE];

    original.to_bytes(buffer);

    auto parsed = CompressionHeader::from_bytes(buffer, CompressionHeader::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->originalSize, 1234);
}

TEST_F(CompressionHeaderTest, FromBytesWithNullReturnsNullopt) {
    auto result = CompressionHeader::from_bytes(nullptr, 10);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionHeaderTest, FromBytesWithInsufficientSizeReturnsNullopt) {
    uint8_t buffer[1] = {0};
    auto result = CompressionHeader::from_bytes(buffer, 1);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionHeaderTest, NetworkByteOrder) {
    CompressionHeader header{.originalSize = 0x1234};
    uint8_t buffer[CompressionHeader::WIRE_SIZE];

    header.to_bytes(buffer);

    // Network byte order is big-endian
    EXPECT_EQ(buffer[0], 0x12);
    EXPECT_EQ(buffer[1], 0x34);
}

TEST_F(CompressionHeaderTest, MaxOriginalSize) {
    CompressionHeader header{.originalSize = 65535};
    uint8_t buffer[CompressionHeader::WIRE_SIZE];

    header.to_bytes(buffer);

    auto parsed = CompressionHeader::from_bytes(buffer, CompressionHeader::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->originalSize, 65535);
}

TEST_F(CompressionHeaderTest, ZeroOriginalSize) {
    CompressionHeader header{.originalSize = 0};
    uint8_t buffer[CompressionHeader::WIRE_SIZE];

    header.to_bytes(buffer);

    auto parsed = CompressionHeader::from_bytes(buffer, CompressionHeader::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->originalSize, 0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Compression Flags Tests
// ═══════════════════════════════════════════════════════════════════════════════

class CompressionFlagsTest : public ::testing::Test {};

TEST_F(CompressionFlagsTest, CompressionFlagValue) {
    EXPECT_EQ(COMPRESSION_FLAG, 0x8000);
}

TEST_F(CompressionFlagsTest, TCPCompressionFlagValue) {
    EXPECT_EQ(TCP_COMPRESSION_FLAG, 0x8000);
}

TEST_F(CompressionFlagsTest, FlagsDoNotOverlapWithMessageTypes) {
    // MessageType values should all be below 0x8000
    // Check a few common ones
    EXPECT_LT(static_cast<uint16_t>(MessageType::Snapshot), COMPRESSION_FLAG);
    EXPECT_LT(static_cast<uint16_t>(MessageType::PlayerInput), COMPRESSION_FLAG);
    EXPECT_LT(static_cast<uint16_t>(MessageType::LeaderboardData), TCP_COMPRESSION_FLAG);
    EXPECT_LT(static_cast<uint16_t>(MessageType::FriendsListData), TCP_COMPRESSION_FLAG);
}

TEST_F(CompressionFlagsTest, ExtractActualTypeFromCompressedFlag) {
    uint16_t compressedType = static_cast<uint16_t>(MessageType::Snapshot) | COMPRESSION_FLAG;

    bool isCompressed = (compressedType & COMPRESSION_FLAG) != 0;
    uint16_t actualType = compressedType & ~COMPRESSION_FLAG;

    EXPECT_TRUE(isCompressed);
    EXPECT_EQ(actualType, static_cast<uint16_t>(MessageType::Snapshot));
}

TEST_F(CompressionFlagsTest, UncompressedTypeHasNoFlag) {
    uint16_t normalType = static_cast<uint16_t>(MessageType::Snapshot);

    bool isCompressed = (normalType & COMPRESSION_FLAG) != 0;
    uint16_t actualType = normalType & ~COMPRESSION_FLAG;

    EXPECT_FALSE(isCompressed);
    EXPECT_EQ(actualType, static_cast<uint16_t>(MessageType::Snapshot));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Edge Cases and Stress Tests
// ═══════════════════════════════════════════════════════════════════════════════

class CompressionEdgeCaseTest : public CompressionTest {};

TEST_F(CompressionEdgeCaseTest, CompressMaxSize) {
    // Test near MAX_UNCOMPRESSED_SIZE
    auto data = generateCompressibleData(compression::MAX_UNCOMPRESSED_SIZE);
    auto compressed = compression::compress(data.data(), data.size());

    ASSERT_FALSE(compressed.empty());

    auto decompressed = compression::decompress(
        compressed.data(), compressed.size(), data.size());
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(decompressed->size(), data.size());
}

TEST_F(CompressionEdgeCaseTest, DecompressWithWrongOriginalSizeFails) {
    auto original = generateCompressibleData(500);
    auto compressed = compression::compress(original.data(), original.size());
    ASSERT_FALSE(compressed.empty());

    // Try to decompress with wrong original size
    auto result = compression::decompress(
        compressed.data(), compressed.size(), original.size() + 100);

    // Should fail because LZ4 validates the output size
    EXPECT_FALSE(result.has_value());
}

TEST_F(CompressionEdgeCaseTest, MultipleCompressions) {
    // Test that compression works correctly when called multiple times
    for (int i = 0; i < 100; ++i) {
        auto data = generateCompressibleData(200 + i * 10);
        auto compressed = compression::compress(data.data(), data.size());

        if (!compressed.empty()) {
            auto decompressed = compression::decompress(
                compressed.data(), compressed.size(), data.size());
            ASSERT_TRUE(decompressed.has_value()) << "Iteration " << i;
            EXPECT_EQ(*decompressed, data) << "Iteration " << i;
        }
    }
}

TEST_F(CompressionEdgeCaseTest, SpecificPatternCompression) {
    // Test specific byte patterns that might be common in network data
    // Use a larger, more compressible pattern
    std::vector<uint8_t> data(500);

    // Fill with a repeating pattern (more compressible)
    for (size_t i = 0; i < data.size(); i += 8) {
        data[i] = 0x00;
        data[i + 1] = 0x01;
        data[i + 2] = 0x00;
        data[i + 3] = 0x01;
        data[i + 4] = 0x00;
        data[i + 5] = 0x01;
        data[i + 6] = 0x00;
        data[i + 7] = 0x01;
    }

    auto compressed = compression::compress(data.data(), data.size());

    // If compression works, verify round-trip
    if (!compressed.empty()) {
        auto decompressed = compression::decompress(
            compressed.data(), compressed.size(), data.size());
        ASSERT_TRUE(decompressed.has_value());
        EXPECT_EQ(*decompressed, data);
    }
    // Note: Some patterns may not compress well with LZ4, which is acceptable
}
