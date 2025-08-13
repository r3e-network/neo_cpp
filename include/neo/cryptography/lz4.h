/**
 * @file lz4.h
 * @brief Lz4
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <cstdint>

namespace neo::cryptography
{
class LZ4
{
   public:
    // Compress data using LZ4 algorithm
    static io::ByteVector Compress(const io::ByteSpan& data);

    // Decompress LZ4 compressed data with size limit
    static io::ByteVector Decompress(const io::ByteSpan& compressedData, uint32_t maxDecompressedSize);

    // Get maximum compressed size for given input size
    static uint32_t GetMaxCompressedSize(uint32_t inputSize);

    // Get decompressed size from compressed data header
    static uint32_t GetDecompressedSize(const io::ByteSpan& compressedData);
};
}  // namespace neo::cryptography