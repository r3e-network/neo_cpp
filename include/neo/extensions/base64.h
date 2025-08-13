/**
 * @file base64.h
 * @brief Base64
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <string>

namespace neo::extensions
{
/**
 * @brief Base64 encoding and decoding utilities.
 */
class Base64
{
   public:
    /**
     * @brief Encodes a byte span to a base64 string.
     * @param data The data to encode.
     * @return The base64 encoded string.
     */
    static std::string Encode(const io::ByteSpan& data);

    /**
     * @brief Decodes a base64 string to a byte vector.
     * @param base64 The base64 string to decode.
     * @return The decoded byte vector.
     */
    static io::ByteVector Decode(const std::string& base64);

   private:
    static const std::string base64_chars;
    static bool IsBase64(unsigned char c);
};
}  // namespace neo::extensions
