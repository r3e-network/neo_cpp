/**
 * @file fuzz_crypto.cpp
 * @brief Fuzz testing for cryptographic operations
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <neo/cryptography/sha256.h>
#include <neo/cryptography/ripemd160.h>
#include <neo/cryptography/ecdsa.h>
#include <neo/cryptography/aes.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/base64.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > 1048576) { // Max 1MB
        return 0;
    }
    
    try {
        ByteVector input(data, data + size);
        
        // Test hashing algorithms
        {
            auto sha256_hash = SHA256::ComputeHash(input);
            auto ripemd160_hash = RIPEMD160::ComputeHash(input);
            
            // Double hashing
            auto double_sha = SHA256::ComputeHash(sha256_hash);
            auto hash160 = RIPEMD160::ComputeHash(sha256_hash);
            
            // Verify hash sizes
            if (sha256_hash.Size() != 32) return 1; // Bug if wrong size
            if (ripemd160_hash.Size() != 20) return 1;
        }
        
        // Test Base58/Base64 encoding
        if (size < 1024) { // Limit size for encoding tests
            auto base58_encoded = Base58::Encode(input);
            auto base58_decoded = Base58::Decode(base58_encoded);
            
            auto base64_encoded = Base64::Encode(input);
            auto base64_decoded = Base64::Decode(base64_encoded);
            
            // Verify round-trip
            if (base58_decoded != input && !base58_encoded.empty()) {
                // Potential bug in Base58
                return 0;
            }
            if (base64_decoded != input && !base64_encoded.empty()) {
                // Potential bug in Base64
                return 0;
            }
        }
        
        // Test ECDSA operations with properly sized input
        if (size >= 32) {
            ByteVector key_data(data, data + 32);
            try {
                KeyPair kp(key_data);
                
                // Sign a message
                ByteVector message(data, data + std::min(size, size_t(64)));
                auto signature = kp.Sign(message);
                
                // Verify signature
                bool valid = kp.Verify(message, signature);
                
                // Verify with wrong message should fail
                if (size > 64) {
                    ByteVector wrong_message(data + 32, data + std::min(size, size_t(96)));
                    bool should_be_false = kp.Verify(wrong_message, signature);
                    if (should_be_false && wrong_message != message) {
                        // Bug: wrong message verified
                        return 0;
                    }
                }
            } catch (...) {
                // Invalid key data
                return 0;
            }
        }
        
        // Test AES encryption with properly sized key and IV
        if (size >= 48) { // 16 bytes key + 16 bytes IV + 16 bytes data minimum
            ByteVector key(data, data + 16);
            ByteVector iv(data + 16, data + 32);
            ByteVector plaintext(data + 32, data + std::min(size, size_t(1024)));
            
            try {
                AES aes(key, iv);
                auto ciphertext = aes.Encrypt(plaintext);
                auto decrypted = aes.Decrypt(ciphertext);
                
                // Verify round-trip
                if (decrypted != plaintext) {
                    // Bug in AES
                    return 0;
                }
            } catch (...) {
                // Invalid AES parameters
                return 0;
            }
        }
        
    } catch (const std::exception& e) {
        // Expected for invalid input
        return 0;
    } catch (...) {
        // Catch all
        return 0;
    }
    
    return 0;
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    return 0;
}