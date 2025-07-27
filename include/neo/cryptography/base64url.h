#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <string>

namespace neo::cryptography
{
/**
 * @brief Provides Base64Url encoding and decoding functionality.
 */
class Base64Url
{
  public:
    /**
     * @brief Encodes a byte array to a Base64Url string.
     * @param data The data to encode.
     * @return The Base64Url encoded string.
     */
    static std::string Encode(const io::ByteSpan& data);

    /**
     * @brief Encodes a string to a Base64Url string.
     * @param data The string to encode.
     * @return The Base64Url encoded string.
     */
    static std::string Encode(const std::string& data);

    /**
     * @brief Decodes a Base64Url string to a byte array.
     * @param data The Base64Url string to decode.
     * @return The decoded byte array.
     */
    static io::ByteVector Decode(const std::string& data);

    /**
     * @brief Decodes a Base64Url string to a string.
     * @param data The Base64Url string to decode.
     * @return The decoded string.
     */
    static std::string DecodeToString(const std::string& data);
};
}  // namespace neo::cryptography
