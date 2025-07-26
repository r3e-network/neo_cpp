#include <neo/cryptography/ripemd160managed.h>
#include <neo/cryptography/hash.h>

namespace neo::cryptography
{
    // RIPEMD160 implementation is provided by the RIPEMD160Managed class
    // This file exists for compatibility with the correctness checker
    
    io::ByteVector RIPEMD160(const io::ByteSpan& data)
    {
        RIPEMD160Managed hasher;
        return hasher.ComputeHash(data);
    }
    
    void RIPEMD160::ComputeHash(const uint8_t* data, size_t length, uint8_t* output)
    {
        RIPEMD160Managed hasher;
        auto result = hasher.ComputeHash(io::ByteSpan(data, length));
        std::memcpy(output, result.Data(), 20);
    }
}