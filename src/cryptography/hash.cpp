/**
 * @file hash.cpp
 * @brief Hashing algorithms
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <openssl/evp.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

#include <cstring>

// Suppress OpenSSL deprecation warnings for compatibility
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // OpenSSL deprecation warnings
#endif

namespace neo::cryptography
{
io::UInt256 Hash::Sha256(const io::ByteSpan& data)
{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.Data(), data.Size());
    unsigned int hashLength;
    EVP_DigestFinal_ex(ctx, hash, &hashLength);
    EVP_MD_CTX_free(ctx);
    return io::UInt256(io::ByteSpan(hash, SHA256_DIGEST_LENGTH));
}

io::UInt160 Hash::Ripemd160(const io::ByteSpan& data)
{
    uint8_t hash[RIPEMD160_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_ripemd160(), nullptr);
    EVP_DigestUpdate(ctx, data.Data(), data.Size());
    unsigned int hashLength;
    EVP_DigestFinal_ex(ctx, hash, &hashLength);
    EVP_MD_CTX_free(ctx);
    return io::UInt160(io::ByteSpan(hash, RIPEMD160_DIGEST_LENGTH));
}

io::UInt256 Hash::Hash256(const io::ByteSpan& data)
{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    // First SHA256
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.Data(), data.Size());
    unsigned int hashLength;
    EVP_DigestFinal_ex(ctx, hash, &hashLength);

    // Second SHA256
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, hash, SHA256_DIGEST_LENGTH);
    EVP_DigestFinal_ex(ctx, hash, &hashLength);

    EVP_MD_CTX_free(ctx);
    return io::UInt256(io::ByteSpan(hash, SHA256_DIGEST_LENGTH));
}

io::UInt160 Hash::Hash160(const io::ByteSpan& data)
{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    // First SHA256
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.Data(), data.Size());
    unsigned int hashLength;
    EVP_DigestFinal_ex(ctx, hash, &hashLength);

    // Then RIPEMD160
    uint8_t ripemd[RIPEMD160_DIGEST_LENGTH];
    EVP_DigestInit_ex(ctx, EVP_ripemd160(), nullptr);
    EVP_DigestUpdate(ctx, hash, SHA256_DIGEST_LENGTH);
    EVP_DigestFinal_ex(ctx, ripemd, &hashLength);

    EVP_MD_CTX_free(ctx);
    return io::UInt160(io::ByteSpan(ripemd, RIPEMD160_DIGEST_LENGTH));
}

io::UInt256 Hash::Keccak256(const io::ByteSpan& data)
{
    // Use the simple verified implementation
    return Keccak256Simple(data);
}

uint32_t Hash::Murmur32(const io::ByteSpan& data, uint32_t seed)
{
    const uint8_t* key = data.Data();
    int len = static_cast<int>(data.Size());

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    uint32_t h1 = seed;
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(key);
    int nblocks = len / 4;

    for (int i = 0; i < nblocks; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t* tail = reinterpret_cast<const uint8_t*>(key + (len & ~3));
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
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;
            h1 ^= k1;
    }

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}
}  // namespace neo::cryptography

#ifdef _MSC_VER
#pragma warning(pop)
#endif
