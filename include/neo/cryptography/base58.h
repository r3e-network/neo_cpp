#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <neo/io/byte_vector.h>

namespace neo::cryptography
{
    /**
     * @brief Base58 encoding and decoding functionality for Neo.
     * 
     * Provides Base58 and Base58Check encoding/decoding used by Neo
     * for addresses and other human-readable representations.
     */
    class Base58
    {
    public:
        /**
         * @brief Encode bytes to Base58 string.
         * @param data The bytes to encode
         * @return Base58 encoded string
         */
        static std::string Encode(const std::vector<uint8_t>& data);
        static std::string Encode(const neo::io::ByteVector& data);
        static std::string Encode(const neo::io::ByteSpan& data);
        
        /**
         * @brief Decode Base58 string to bytes.
         * @param encoded The Base58 encoded string
         * @return Decoded bytes
         */
        static std::vector<uint8_t> Decode(const std::string& encoded);
        static neo::io::ByteVector DecodeToByteVector(const std::string& encoded);
        
        /**
         * @brief Encode bytes to Base58Check string (with checksum).
         * @param data The bytes to encode
         * @return Base58Check encoded string
         */
        static std::string EncodeCheck(const std::vector<uint8_t>& data);
        static std::string EncodeCheck(const neo::io::ByteVector& data);
        static std::string EncodeCheck(const neo::io::ByteSpan& data);
        
        // Legacy aliases for compatibility
        static std::string EncodeWithChecksum(const neo::io::ByteSpan& data) { return EncodeCheck(data); }
        
        /**
         * @brief Decode Base58Check string to bytes (verifies checksum).
         * @param encoded The Base58Check encoded string
         * @return Decoded bytes (without checksum)
         * @throws std::runtime_error if checksum is invalid
         */
        static std::vector<uint8_t> DecodeCheck(const std::string& encoded);
        static neo::io::ByteVector DecodeCheckToByteVector(const std::string& encoded);
        
        // Legacy aliases for compatibility
        static neo::io::ByteVector DecodeWithChecksum(const std::string& encoded) { return DecodeCheckToByteVector(encoded); }
        
        /**
         * @brief Verify if a string is valid Base58.
         * @param str The string to check
         * @return true if valid Base58, false otherwise
         */
        static bool IsValid(const std::string& str);
        
        /**
         * @brief Verify if a string is valid Base58Check.
         * @param str The string to check
         * @return true if valid Base58Check, false otherwise
         */
        static bool IsValidCheck(const std::string& str);
        
    private:
        // Base58 alphabet used by Bitcoin/Neo
        static constexpr const char* ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        /**
         * @brief Calculate SHA256 double hash for checksum.
         * @param data Input data
         * @return First 4 bytes of SHA256(SHA256(data))
         */
        static std::vector<uint8_t> CalculateChecksum(const std::vector<uint8_t>& data);
    };
    
    // Convenience functions
    namespace base58
    {
        inline std::string encode(const std::vector<uint8_t>& data) 
        { 
            return Base58::Encode(data); 
        }
        
        inline std::string encode(const neo::io::ByteVector& data) 
        { 
            return Base58::Encode(data); 
        }
        
        inline std::vector<uint8_t> decode(const std::string& encoded) 
        { 
            return Base58::Decode(encoded); 
        }
        
        inline std::string encode_check(const std::vector<uint8_t>& data) 
        { 
            return Base58::EncodeCheck(data); 
        }
        
        inline std::string encode_check(const neo::io::ByteVector& data) 
        { 
            return Base58::EncodeCheck(data); 
        }
        
        inline std::vector<uint8_t> decode_check(const std::string& encoded) 
        { 
            return Base58::DecodeCheck(encoded); 
        }
        
        inline bool is_valid(const std::string& str) 
        { 
            return Base58::IsValid(str); 
        }
        
        inline bool is_valid_check(const std::string& str) 
        { 
            return Base58::IsValidCheck(str); 
        }
    }
} 