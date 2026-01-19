/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Compression - LZ4 compression utilities for network packets
*/

#ifndef COMPRESSION_HPP_
#define COMPRESSION_HPP_

#include <cstdint>
#include <cstddef>
#include <vector>
#include <optional>
#include <lz4.h>

namespace compression {

/**
 * @brief Compresses data using LZ4 algorithm
 * @param src Source data to compress
 * @param srcSize Size of source data in bytes
 * @return Compressed data or empty vector if compression fails/not worth it
 *
 * Note: Returns empty if compressed size >= original size (not worth compressing)
 */
inline std::vector<uint8_t> compress(const uint8_t* src, size_t srcSize) {
    if (srcSize == 0 || src == nullptr) {
        return {};
    }

    // Get maximum compressed size
    int maxDstSize = LZ4_compressBound(static_cast<int>(srcSize));
    if (maxDstSize <= 0) {
        return {};
    }

    std::vector<uint8_t> compressed(static_cast<size_t>(maxDstSize));

    int compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(src),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(srcSize),
        maxDstSize
    );

    if (compressedSize <= 0) {
        return {};
    }

    // Only use compression if it actually reduces size
    if (static_cast<size_t>(compressedSize) >= srcSize) {
        return {};
    }

    compressed.resize(static_cast<size_t>(compressedSize));
    return compressed;
}

/**
 * @brief Decompresses LZ4-compressed data
 * @param src Compressed data
 * @param srcSize Size of compressed data
 * @param originalSize Original uncompressed size (must be known)
 * @return Decompressed data or nullopt if decompression fails
 */
inline std::optional<std::vector<uint8_t>> decompress(
    const uint8_t* src,
    size_t srcSize,
    size_t originalSize
) {
    if (srcSize == 0 || src == nullptr || originalSize == 0) {
        return std::nullopt;
    }

    std::vector<uint8_t> decompressed(originalSize);

    int decompressedSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(src),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(srcSize),
        static_cast<int>(originalSize)
    );

    if (decompressedSize < 0 || static_cast<size_t>(decompressedSize) != originalSize) {
        return std::nullopt;
    }

    return decompressed;
}

/**
 * @brief Minimum packet size to consider for compression
 * Small packets have compression overhead that isn't worth it
 */
constexpr size_t MIN_COMPRESS_SIZE = 128;

/**
 * @brief Maximum uncompressed size we support (64KB)
 * Larger packets should be fragmented at a higher level
 */
constexpr size_t MAX_UNCOMPRESSED_SIZE = 65535;

} // namespace compression

#endif /* !COMPRESSION_HPP_ */
