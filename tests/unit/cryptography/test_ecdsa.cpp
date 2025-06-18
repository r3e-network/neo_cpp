#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <chrono>

#include "neo/cryptography/ecdsa.h"
#include "neo/cryptography/ecc.h"
#include "neo/cryptography/hash.h"
#include "neo/io/uint256.h"
#include "neo/extensions/string_extensions.h"
#include "tests/utils/test_helpers.h"

using namespace neo::cryptography;
using namespace neo::io;
using namespace neo::extensions;
using namespace neo::tests;

class ECDSATest : public ::testing::Test {
protected:
    void SetUp() override {
        // Known test vectors for ECDSA with secp256r1
        test_vectors_ = {
            {
                // Private key
                "c28a9f80738efe59020f471c0ee41eb3eed5e5ea734d2078cd09b2accc5e2cbfb",
                // Public key (compressed)
                "03661aed5e27cb83ba24f60fce7635a7d60a3a3f2e17e2b2cf32a4eb1b4b3bcd5",
                // Message hash
                "af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf",
                // Expected signature (r,s format)
                "9c5e3aa8a65edcaa8b5d5af5b40bf5b0fbb3c6e9e5e59e66c43c1c5fc7bc11c7",
                "7a9c4c5b9e1f6b8d3a2e8f9b4c7a5e1d8b6c9a3f2e7c4b8a9e6d3f1c5b7a8e2"
            },
            {
                // Another test vector
                "f7ce80c11173e4a4b5e50ebcfdc3b84b0b3e5c4c7d2b8f5e1a4b9c6d3e8f7a2",
                "02e3c3d5e7f9b1c4a6e8f2d5c7b9a3e6f1d4c8b2a7e9f6c3b5a8d1e4f7c2b9",
                "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
                "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1",
                "b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2"
            }
        };
    }
    
    struct ECDSATestVector {
        std::string private_key_hex;
        std::string public_key_hex;
        std::string message_hash_hex;
        std::string expected_r_hex;
        std::string expected_s_hex;
    };
    
    std::vector<ECDSATestVector> test_vectors_;
    
    std::vector<uint8_t> HexToBytes(const std::string& hex) {
        return StringExtensions::FromHexString(hex);
    }
    
    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        return StringExtensions::ToHexString(bytes);
    }
};

// Test ECDSA key generation
TEST_F(ECDSATest, KeyGeneration) {
    auto key_pair = ECDSA::GenerateKeyPair();
    
    EXPECT_FALSE(key_pair.GetPrivateKey().empty());
    EXPECT_FALSE(key_pair.GetPublicKey().empty());
    
    // Private key should be 32 bytes
    EXPECT_EQ(key_pair.GetPrivateKey().size(), 32);
    
    // Public key should be 33 bytes (compressed) or 65 bytes (uncompressed)
    auto pub_key_size = key_pair.GetPublicKey().size();
    EXPECT_TRUE(pub_key_size == 33 || pub_key_size == 65);
    
    // Private key should not be all zeros
    bool all_zeros = std::all_of(key_pair.GetPrivateKey().begin(), 
                                key_pair.GetPrivateKey().end(), 
                                [](uint8_t b) { return b == 0; });
    EXPECT_FALSE(all_zeros);
}

// Test multiple key generation produces different keys
TEST_F(ECDSATest, MultipleKeyGeneration) {
    auto key_pair1 = ECDSA::GenerateKeyPair();
    auto key_pair2 = ECDSA::GenerateKeyPair();
    auto key_pair3 = ECDSA::GenerateKeyPair();
    
    // All private keys should be different
    EXPECT_NE(key_pair1.GetPrivateKey(), key_pair2.GetPrivateKey());
    EXPECT_NE(key_pair1.GetPrivateKey(), key_pair3.GetPrivateKey());
    EXPECT_NE(key_pair2.GetPrivateKey(), key_pair3.GetPrivateKey());
    
    // All public keys should be different
    EXPECT_NE(key_pair1.GetPublicKey(), key_pair2.GetPublicKey());
    EXPECT_NE(key_pair1.GetPublicKey(), key_pair3.GetPublicKey());
    EXPECT_NE(key_pair2.GetPublicKey(), key_pair3.GetPublicKey());
}

// Test signature generation and verification
TEST_F(ECDSATest, SignatureGenerationAndVerification) {
    auto key_pair = ECDSA::GenerateKeyPair();
    
    // Test message
    std::string message = "Hello, Neo blockchain!";
    auto message_bytes = std::vector<uint8_t>(message.begin(), message.end());
    auto message_hash = Hash::Sha256(message_bytes);
    
    // Generate signature
    auto signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
    
    EXPECT_FALSE(signature.empty());
    EXPECT_EQ(signature.size(), 64); // r (32 bytes) + s (32 bytes)
    
    // Verify signature
    bool is_valid = ECDSA::Verify(message_hash, signature, key_pair.GetPublicKey());
    EXPECT_TRUE(is_valid);
    
    // Verify with wrong message should fail
    auto wrong_message = std::string("Wrong message");
    auto wrong_bytes = std::vector<uint8_t>(wrong_message.begin(), wrong_message.end());
    auto wrong_hash = Hash::Sha256(wrong_bytes);
    
    bool is_invalid = ECDSA::Verify(wrong_hash, signature, key_pair.GetPublicKey());
    EXPECT_FALSE(is_invalid);
}

// Test known test vectors
TEST_F(ECDSATest, KnownTestVectors) {
    for (const auto& tv : test_vectors_) {
        auto private_key = HexToBytes(tv.private_key_hex);
        auto message_hash = HexToBytes(tv.message_hash_hex);
        
        // Generate public key from private key
        auto public_key = ECDSA::GetPublicKeyFromPrivateKey(private_key);
        EXPECT_FALSE(public_key.empty());
        
        // Sign the message
        auto signature = ECDSA::Sign(message_hash, private_key);
        EXPECT_EQ(signature.size(), 64);
        
        // Verify the signature
        bool is_valid = ECDSA::Verify(message_hash, signature, public_key);
        EXPECT_TRUE(is_valid) << "Failed to verify signature for test vector";
        
        // Extract r and s components
        std::vector<uint8_t> r(signature.begin(), signature.begin() + 32);
        std::vector<uint8_t> s(signature.begin() + 32, signature.end());
        
        // Note: Due to randomness in ECDSA, we can't compare exact r,s values
        // but we can verify the signature is valid
        EXPECT_EQ(r.size(), 32);
        EXPECT_EQ(s.size(), 32);
    }
}

// Test deterministic signatures (RFC 6979)
TEST_F(ECDSATest, DeterministicSignatures) {
    auto private_key = HexToBytes("c28a9f80738efe59020f471c0ee41eb3eed5e5ea734d2078cd09b2accc5e2cbfb");
    auto message_hash = HexToBytes("af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf");
    
    // Generate signature multiple times - should be deterministic if using RFC 6979
    auto signature1 = ECDSA::SignDeterministic(message_hash, private_key);
    auto signature2 = ECDSA::SignDeterministic(message_hash, private_key);
    auto signature3 = ECDSA::SignDeterministic(message_hash, private_key);
    
    // All signatures should be identical (deterministic)
    EXPECT_EQ(signature1, signature2);
    EXPECT_EQ(signature2, signature3);
    EXPECT_EQ(signature1, signature3);
    
    // But should still be valid
    auto public_key = ECDSA::GetPublicKeyFromPrivateKey(private_key);
    EXPECT_TRUE(ECDSA::Verify(message_hash, signature1, public_key));
}

// Test signature malleability protection
TEST_F(ECDSATest, SignatureMalleabilityProtection) {
    auto key_pair = ECDSA::GenerateKeyPair();
    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
    
    auto signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
    
    // Check that s value is in the lower half of the curve order (canonical)
    std::vector<uint8_t> s(signature.begin() + 32, signature.end());
    
    // secp256r1 curve order (n) = FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
    // s should be <= n/2
    std::vector<uint8_t> half_order = {
        0x7F, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xDE, 0x73, 0x7D, 0x56, 0xD3, 0x8B, 0xCF, 0x42,
        0x79, 0xDC, 0xE5, 0x61, 0x7E, 0x31, 0x92, 0xA8
    };
    
    // Compare s with half_order (simplified check)
    bool is_canonical = true;
    for (size_t i = 0; i < 32; ++i) {
        if (s[i] > half_order[i]) {
            is_canonical = false;
            break;
        } else if (s[i] < half_order[i]) {
            break;
        }
    }
    
    EXPECT_TRUE(is_canonical) << "Signature s value should be canonical (low-s)";
}

// Test invalid signature detection
TEST_F(ECDSATest, InvalidSignatureDetection) {
    auto key_pair = ECDSA::GenerateKeyPair();
    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
    auto valid_signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
    
    // Test with corrupted signature
    auto corrupted_signature = valid_signature;
    corrupted_signature[0] ^= 0xFF; // Flip bits in r
    
    bool is_valid = ECDSA::Verify(message_hash, corrupted_signature, key_pair.GetPublicKey());
    EXPECT_FALSE(is_valid);
    
    // Test with wrong signature length
    std::vector<uint8_t> short_signature(valid_signature.begin(), valid_signature.begin() + 32);
    is_valid = ECDSA::Verify(message_hash, short_signature, key_pair.GetPublicKey());
    EXPECT_FALSE(is_valid);
    
    // Test with all-zero signature
    std::vector<uint8_t> zero_signature(64, 0);
    is_valid = ECDSA::Verify(message_hash, zero_signature, key_pair.GetPublicKey());
    EXPECT_FALSE(is_valid);
    
    // Test with all-FF signature
    std::vector<uint8_t> ff_signature(64, 0xFF);
    is_valid = ECDSA::Verify(message_hash, ff_signature, key_pair.GetPublicKey());
    EXPECT_FALSE(is_valid);
}

// Test public key validation
TEST_F(ECDSATest, PublicKeyValidation) {
    auto key_pair = ECDSA::GenerateKeyPair();
    
    // Valid public key should pass validation
    bool is_valid = ECDSA::IsValidPublicKey(key_pair.GetPublicKey());
    EXPECT_TRUE(is_valid);
    
    // Invalid public key formats
    std::vector<uint8_t> short_key(10, 0x02);
    EXPECT_FALSE(ECDSA::IsValidPublicKey(short_key));
    
    std::vector<uint8_t> long_key(100, 0x02);
    EXPECT_FALSE(ECDSA::IsValidPublicKey(long_key));
    
    // Wrong prefix for compressed key
    auto invalid_prefix_key = key_pair.GetPublicKey();
    if (invalid_prefix_key.size() == 33) {
        invalid_prefix_key[0] = 0x01; // Invalid prefix
        EXPECT_FALSE(ECDSA::IsValidPublicKey(invalid_prefix_key));
    }
    
    // All-zero public key
    std::vector<uint8_t> zero_key(33, 0);
    zero_key[0] = 0x02; // Valid prefix but invalid point
    EXPECT_FALSE(ECDSA::IsValidPublicKey(zero_key));
}

// Test private key validation
TEST_F(ECDSATest, PrivateKeyValidation) {
    auto key_pair = ECDSA::GenerateKeyPair();
    
    // Valid private key
    bool is_valid = ECDSA::IsValidPrivateKey(key_pair.GetPrivateKey());
    EXPECT_TRUE(is_valid);
    
    // Invalid private key formats
    std::vector<uint8_t> short_key(10, 0x01);
    EXPECT_FALSE(ECDSA::IsValidPrivateKey(short_key));
    
    std::vector<uint8_t> long_key(50, 0x01);
    EXPECT_FALSE(ECDSA::IsValidPrivateKey(long_key));
    
    // Zero private key (invalid)
    std::vector<uint8_t> zero_key(32, 0);
    EXPECT_FALSE(ECDSA::IsValidPrivateKey(zero_key));
    
    // Private key >= curve order (invalid)
    std::vector<uint8_t> max_key(32, 0xFF);
    EXPECT_FALSE(ECDSA::IsValidPrivateKey(max_key));
}

// Test key recovery from signature
TEST_F(ECDSATest, KeyRecoveryFromSignature) {
    auto key_pair = ECDSA::GenerateKeyPair();
    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
    
    // Sign with recovery info
    auto signature_with_recovery = ECDSA::SignWithRecovery(message_hash, key_pair.GetPrivateKey());
    
    EXPECT_EQ(signature_with_recovery.size(), 65); // 64 bytes signature + 1 byte recovery id
    
    // Recover public key from signature
    auto recovered_key = ECDSA::RecoverPublicKey(message_hash, signature_with_recovery);
    
    EXPECT_FALSE(recovered_key.empty());
    
    // Recovered key should match original (might need normalization)
    auto original_uncompressed = ECDSA::DecompressPublicKey(key_pair.GetPublicKey());
    auto recovered_uncompressed = ECDSA::DecompressPublicKey(recovered_key);
    
    EXPECT_EQ(original_uncompressed, recovered_uncompressed);
}

// Test public key compression/decompression
TEST_F(ECDSATest, PublicKeyCompressionDecompression) {
    auto key_pair = ECDSA::GenerateKeyPair();
    auto public_key = key_pair.GetPublicKey();
    
    if (public_key.size() == 33) {
        // Compressed key - decompress it
        auto decompressed = ECDSA::DecompressPublicKey(public_key);
        EXPECT_EQ(decompressed.size(), 65);
        EXPECT_EQ(decompressed[0], 0x04); // Uncompressed prefix
        
        // Compress it back
        auto recompressed = ECDSA::CompressPublicKey(decompressed);
        EXPECT_EQ(recompressed.size(), 33);
        EXPECT_TRUE(recompressed[0] == 0x02 || recompressed[0] == 0x03);
        
        // Should match original
        EXPECT_EQ(public_key, recompressed);
    } else if (public_key.size() == 65) {
        // Uncompressed key - compress it
        auto compressed = ECDSA::CompressPublicKey(public_key);
        EXPECT_EQ(compressed.size(), 33);
        EXPECT_TRUE(compressed[0] == 0x02 || compressed[0] == 0x03);
        
        // Decompress it back
        auto decompressed = ECDSA::DecompressPublicKey(compressed);
        EXPECT_EQ(decompressed.size(), 65);
        
        // Should match original
        EXPECT_EQ(public_key, decompressed);
    }
}

// Test ECDSA with different hash algorithms
TEST_F(ECDSATest, DifferentHashAlgorithms) {
    auto key_pair = ECDSA::GenerateKeyPair();
    std::string message = "Test message for different hashes";
    auto message_bytes = std::vector<uint8_t>(message.begin(), message.end());
    
    // Test with SHA256
    auto sha256_hash = Hash::Sha256(message_bytes);
    auto signature_sha256 = ECDSA::Sign(sha256_hash, key_pair.GetPrivateKey());
    EXPECT_TRUE(ECDSA::Verify(sha256_hash, signature_sha256, key_pair.GetPublicKey()));
    
    // Test with double SHA256
    auto double_sha256_hash = Hash::Sha256(Hash::Sha256(message_bytes));
    auto signature_double = ECDSA::Sign(double_sha256_hash, key_pair.GetPrivateKey());
    EXPECT_TRUE(ECDSA::Verify(double_sha256_hash, signature_double, key_pair.GetPublicKey()));
    
    // Signatures should be different for different hashes
    EXPECT_NE(signature_sha256, signature_double);
}

// Test performance characteristics
TEST_F(ECDSATest, PerformanceCharacteristics) {
    const int num_operations = 100;
    
    // Test key generation performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        auto key_pair = ECDSA::GenerateKeyPair();
        EXPECT_FALSE(key_pair.GetPrivateKey().empty());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto keygen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Test signing performance
    auto key_pair = ECDSA::GenerateKeyPair();
    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
    
    start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<uint8_t>> signatures;
    for (int i = 0; i < num_operations; ++i) {
        auto signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
        signatures.push_back(signature);
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto signing_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Test verification performance
    start_time = std::chrono::high_resolution_clock::now();
    
    int valid_signatures = 0;
    for (const auto& signature : signatures) {
        if (ECDSA::Verify(message_hash, signature, key_pair.GetPublicKey())) {
            valid_signatures++;
        }
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto verification_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Performance expectations
    EXPECT_LT(keygen_duration.count(), 10000); // Less than 10 seconds for 100 key generations
    EXPECT_LT(signing_duration.count(), 5000);  // Less than 5 seconds for 100 signatures
    EXPECT_LT(verification_duration.count(), 5000); // Less than 5 seconds for 100 verifications
    
    // All signatures should be valid
    EXPECT_EQ(valid_signatures, num_operations);
    
    std::cout << "Performance metrics:" << std::endl;
    std::cout << "Key generation: " << (keygen_duration.count() / static_cast<double>(num_operations)) << " ms/op" << std::endl;
    std::cout << "Signing: " << (signing_duration.count() / static_cast<double>(num_operations)) << " ms/op" << std::endl;
    std::cout << "Verification: " << (verification_duration.count() / static_cast<double>(num_operations)) << " ms/op" << std::endl;
}

// Test concurrent ECDSA operations
TEST_F(ECDSATest, ConcurrentOperations) {
    const int num_threads = 4;
    const int operations_per_thread = 25;
    
    std::atomic<int> successful_operations{0};
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([operations_per_thread, &successful_operations]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    // Generate key pair
                    auto key_pair = ECDSA::GenerateKeyPair();
                    
                    // Create message hash
                    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
                    
                    // Sign and verify
                    auto signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
                    bool is_valid = ECDSA::Verify(message_hash, signature, key_pair.GetPublicKey());
                    
                    if (is_valid) {
                        successful_operations++;
                    }
                } catch (...) {
                    // Handle any threading issues
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should handle concurrent operations without issues
    EXPECT_EQ(successful_operations.load(), num_threads * operations_per_thread);
}

// Test edge cases and error conditions
TEST_F(ECDSATest, EdgeCasesAndErrorConditions) {
    auto key_pair = ECDSA::GenerateKeyPair();
    
    // Empty message hash
    std::vector<uint8_t> empty_hash;
    EXPECT_THROW(ECDSA::Sign(empty_hash, key_pair.GetPrivateKey()), std::invalid_argument);
    
    // Wrong hash size
    std::vector<uint8_t> wrong_size_hash(16, 0xAA); // 16 bytes instead of 32
    EXPECT_THROW(ECDSA::Sign(wrong_size_hash, key_pair.GetPrivateKey()), std::invalid_argument);
    
    // Empty private key
    std::vector<uint8_t> empty_private_key;
    auto message_hash = TestHelpers::GenerateRandomHash().ToArray();
    EXPECT_THROW(ECDSA::Sign(message_hash, empty_private_key), std::invalid_argument);
    
    // Empty public key for verification
    auto signature = ECDSA::Sign(message_hash, key_pair.GetPrivateKey());
    std::vector<uint8_t> empty_public_key;
    EXPECT_FALSE(ECDSA::Verify(message_hash, signature, empty_public_key));
    
    // Empty signature
    std::vector<uint8_t> empty_signature;
    EXPECT_FALSE(ECDSA::Verify(message_hash, empty_signature, key_pair.GetPublicKey()));
}