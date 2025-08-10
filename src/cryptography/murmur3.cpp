#include <neo/cryptography/murmur3.h>

#include <cstring>

namespace neo::cryptography
{

uint32_t MurmurHash3::Hash32(const uint8_t* data, size_t len, uint32_t seed)
{
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    const uint32_t r1 = 15;
    const uint32_t r2 = 13;
    const uint32_t m = 5;
    const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;

    const int nblocks = len / 4;
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);

    // Process 4-byte blocks
    for (int i = 0; i < nblocks; i++)
    {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    // Process remaining bytes
    const uint8_t* tail = reinterpret_cast<const uint8_t*>(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3)
    {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << r1) | (k1 >> (32 - r1));
            k1 *= c2;
            hash ^= k1;
    }

    // Finalization
    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

uint32_t MurmurHash3::Hash32(const io::ByteSpan& data, uint32_t seed) { return Hash32(data.Data(), data.Size(), seed); }

uint32_t MurmurHash3::Hash32(const io::ByteVector& data, uint32_t seed)
{
    return Hash32(data.Data(), data.Size(), seed);
}

uint32_t MurmurHash3::Hash32(const std::string& data, uint32_t seed)
{
    return Hash32(reinterpret_cast<const uint8_t*>(data.data()), data.size(), seed);
}

}  // namespace neo::cryptography