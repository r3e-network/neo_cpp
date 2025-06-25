#include <neo/cryptography/lz4.h>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace neo::cryptography
{
    io::ByteVector LZ4::Compress(const io::ByteSpan& data)
    {
        if (data.Size() == 0)
        {
            return io::ByteVector();
        }

        // Simple implementation for now - in a real scenario you'd use LZ4 library
        // For compatibility with existing code, we'll create a minimal implementation
        // that stores the original size and just copies the data
        
        io::ByteVector result;
        result.Reserve(data.Size() + 4); // 4 bytes for original size
        
        // Store original size (little endian)
        uint32_t originalSize = static_cast<uint32_t>(data.Size());
        result.Push(static_cast<uint8_t>(originalSize & 0xFF));
        result.Push(static_cast<uint8_t>((originalSize >> 8) & 0xFF));
        result.Push(static_cast<uint8_t>((originalSize >> 16) & 0xFF));
        result.Push(static_cast<uint8_t>((originalSize >> 24) & 0xFF));
        
        // For now, just copy the data (no actual compression)
        // In production, this would use LZ4_compress_default()
        result.Append(data);
        
        return result;
    }

    io::ByteVector LZ4::Decompress(const io::ByteSpan& compressedData, uint32_t maxDecompressedSize)
    {
        if (compressedData.Size() < 4)
        {
            throw std::invalid_argument("Invalid compressed data: too small");
        }

        // Read original size (little endian)
        uint32_t originalSize = static_cast<uint32_t>(compressedData.Data()[0]) |
                               (static_cast<uint32_t>(compressedData.Data()[1]) << 8) |
                               (static_cast<uint32_t>(compressedData.Data()[2]) << 16) |
                               (static_cast<uint32_t>(compressedData.Data()[3]) << 24);

        if (originalSize > maxDecompressedSize)
        {
            throw std::invalid_argument("Decompressed size exceeds maximum allowed");
        }

        if (compressedData.Size() < 4 + originalSize)
        {
            throw std::invalid_argument("Invalid compressed data: insufficient data");
        }

        // For now, just copy the data (matches our simple compression)
        // In production, this would use LZ4_decompress_safe()
        io::ByteVector result;
        result.Reserve(originalSize);
        
        const uint8_t* sourceData = compressedData.Data() + 4;
        for (uint32_t i = 0; i < originalSize; i++)
        {
            result.Push(sourceData[i]);
        }
        
        return result;
    }

    uint32_t LZ4::GetMaxCompressedSize(uint32_t inputSize)
    {
        // LZ4 worst case: input size + (input size / 255) + 16
        // For our simple implementation: original size + 4 bytes header
        return inputSize + 4;
    }

    uint32_t LZ4::GetDecompressedSize(const io::ByteSpan& compressedData)
    {
        if (compressedData.Size() < 4)
        {
            return 0;
        }

        // Read original size (little endian)
        return static_cast<uint32_t>(compressedData.Data()[0]) |
               (static_cast<uint32_t>(compressedData.Data()[1]) << 8) |
               (static_cast<uint32_t>(compressedData.Data()[2]) << 16) |
               (static_cast<uint32_t>(compressedData.Data()[3]) << 24);
    }
}