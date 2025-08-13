/**
 * @file ripemd160.cpp
 * @brief Ripemd160
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/cryptography/ripemd160managed.h>

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
}  // namespace neo::cryptography