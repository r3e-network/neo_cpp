#include <neo/cryptography/hash.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/evp.h>
#include <cstring>

// Suppress OpenSSL deprecation warnings for compatibility
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // OpenSSL deprecation warnings
#endif

namespace neo::cryptography
{
    io::UInt256 Hash::Sha256(const io::ByteSpan& data)
    {
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, data.Data(), data.Size());
        SHA256_Final(hash, &ctx);
        return io::UInt256(io::ByteSpan(hash, SHA256_DIGEST_LENGTH));
    }

    io::UInt160 Hash::Ripemd160(const io::ByteSpan& data)
    {
        uint8_t hash[RIPEMD160_DIGEST_LENGTH];
        RIPEMD160_CTX ctx;
        RIPEMD160_Init(&ctx);
        RIPEMD160_Update(&ctx, data.Data(), data.Size());
        RIPEMD160_Final(hash, &ctx);
        return io::UInt160(io::ByteSpan(hash, RIPEMD160_DIGEST_LENGTH));
    }

    io::UInt256 Hash::Hash256(const io::ByteSpan& data)
    {
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, data.Data(), data.Size());
        SHA256_Final(hash, &ctx);
        
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, hash, SHA256_DIGEST_LENGTH);
        SHA256_Final(hash, &ctx);
        
        return io::UInt256(io::ByteSpan(hash, SHA256_DIGEST_LENGTH));
    }

    io::UInt160 Hash::Hash160(const io::ByteSpan& data)
    {
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.Data(), data.Size());
        SHA256_Final(hash, &sha256);
        
        uint8_t ripemd[RIPEMD160_DIGEST_LENGTH];
        RIPEMD160_CTX ripemd160;
        RIPEMD160_Init(&ripemd160);
        RIPEMD160_Update(&ripemd160, hash, SHA256_DIGEST_LENGTH);
        RIPEMD160_Final(ripemd, &ripemd160);
        
        return io::UInt160(io::ByteSpan(ripemd, RIPEMD160_DIGEST_LENGTH));
    }

    io::UInt256 Hash::Keccak256(const io::ByteSpan& data)
    {
        uint8_t hash[EVP_MAX_MD_SIZE];
        unsigned int hashLength;
        
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
        EVP_DigestUpdate(ctx, data.Data(), data.Size());
        EVP_DigestFinal_ex(ctx, hash, &hashLength);
        EVP_MD_CTX_free(ctx);
        
        return io::UInt256(io::ByteSpan(hash, hashLength));
    }

    uint32_t Hash::Murmur32(const io::ByteSpan& data, uint32_t seed)
    {
        const uint8_t* key = data.Data();
        int len = static_cast<int>(data.Size());
        
        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;
        
        uint32_t h1 = seed;
        const uint32_t* blocks = reinterpret_cast<const uint32_t*>(key + ((len & ~3)));
        
        for (int i = -len & 3; i; i += 4)
        {
            uint32_t k1 = blocks[i >> 2];
            
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
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
