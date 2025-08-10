#include <neo/cryptography/murmur32.h>

#include <cstring>

namespace neo::cryptography
{
uint32_t Murmur32::Hash(const io::ByteSpan& data, uint32_t seed) { return Hash(data.Data(), data.Size(), seed); }

uint32_t Murmur32::Hash(const uint8_t* data, size_t len, uint32_t seed)
{
    uint32_t hash = seed;
    const size_t nblocks = len / 4;

    // Process 4-byte blocks
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
    for (size_t i = 0; i < nblocks; i++)
    {
        uint32_t k = blocks[i];

        k *= c1;
        k = RotateLeft(k, r1);
        k *= c2;

        hash ^= k;
        hash = RotateLeft(hash, r2);
        hash = hash * m + n;
    }

    // Process remaining bytes
    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3)
    {
        case 3:
            k1 ^= tail[2] << 16;
            [[fallthrough]];
        case 2:
            k1 ^= tail[1] << 8;
            [[fallthrough]];
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = RotateLeft(k1, r1);
            k1 *= c2;
            hash ^= k1;
    }

    return FinalizeHash(hash, len);
}

uint32_t Murmur32::RotateLeft(uint32_t value, int shift) { return (value << shift) | (value >> (32 - shift)); }

uint32_t Murmur32::FinalizeHash(uint32_t hash, size_t len)
{
    hash ^= static_cast<uint32_t>(len);
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    return hash;
}
}  // namespace neo::cryptography