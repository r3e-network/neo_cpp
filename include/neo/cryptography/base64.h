#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <string>

namespace neo::cryptography
{
/**
 * @brief Provides Base64 encoding and decoding.
 */
class Base64
{
   public:
    /**
     * @brief Encodes data to Base64.
     * @param data The data to encode.
     * @return The Base64 encoded string.
     */
    static std::string Encode(const io::ByteSpan& data);

    /**
     * @brief Encodes a string to Base64.
     * @param data The string to encode.
     * @return The Base64 encoded string.
     */
    static std::string Encode(const std::string& data);

    /**
     * @brief Decodes a Base64 string to bytes.
     * @param data The Base64 string to decode.
     * @return The decoded bytes.
     * @throws std::runtime_error if the string is not valid Base64.
     */
    static io::ByteVector Decode(const std::string& data);

    /**
     * @brief Decodes a Base64 string to a string.
     * @param data The Base64 string to decode.
     * @return The decoded string.
     * @throws std::runtime_error if the string is not valid Base64.
     */
    static std::string DecodeToString(const std::string& data);
};
}  // namespace neo::cryptography
