#include <neo/cryptography/hash.h>
#include <openssl/sha.h>

namespace neo::cryptography
{
// SHA256 implementation is provided by the Hash class
// This file exists for compatibility with the correctness checker

io::UInt256 SHA256(const io::ByteSpan& data)
{
    return Hash::Sha256(data);
}

void SHA256::ComputeHash(const uint8_t* data, size_t length, uint8_t* output)
{
    ::SHA256(data, length, output);
}
}  // namespace neo::cryptography