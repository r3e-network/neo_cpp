#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <openssl/ripemd.h>
#include <array>

namespace neo {
namespace cryptography {

/**
 * @brief RIPEMD-160 hash function wrapper
 */
class Ripemd160 {
public:
    static constexpr size_t HASH_SIZE = 20;  // 160 bits / 8
    
    /**
     * @brief Compute RIPEMD-160 hash of data
     * @param data Input data
     * @return 160-bit hash
     */
    static io::UInt160 Hash(const io::ByteSpan& data) {
        std::array<uint8_t, HASH_SIZE> hash;
        RIPEMD160(data.Data(), data.Size(), hash.data());
        return io::UInt160(hash.data());
    }
    
    /**
     * @brief Compute RIPEMD-160 hash of byte vector
     * @param data Input byte vector
     * @return 160-bit hash
     */
    static io::UInt160 Hash(const io::ByteVector& data) {
        return Hash(data.AsSpan());
    }
    
    /**
     * @brief Compute RIPEMD-160 hash of string
     * @param str Input string
     * @return 160-bit hash
     */
    static io::UInt160 Hash(const std::string& str) {
        return Hash(io::ByteSpan(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
    }
    
    /**
     * @brief Compute double RIPEMD-160 hash (hash of hash)
     * @param data Input data
     * @return 160-bit hash of hash
     */
    static io::UInt160 DoubleHash(const io::ByteSpan& data) {
        auto firstHash = Hash(data);
        return Hash(io::ByteSpan(firstHash.Data(), HASH_SIZE));
    }
};

} // namespace cryptography
} // namespace neo
