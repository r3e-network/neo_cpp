#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "neo/cryptography/scrypt.h"
#include "neo/extensions/string_extensions.h"

using namespace neo::cryptography;
using namespace neo::extensions;

class ScryptTest : public ::testing::Test
{
  protected:
    // Test vectors from various sources including RFC 7914
    struct ScryptTestVector
    {
        std::string password;
        std::string salt;
        uint32_t N;      // CPU/memory cost parameter
        uint32_t r;      // Block size parameter
        uint32_t p;      // Parallelization parameter
        uint32_t dkLen;  // Desired key length
        std::string expected_hex;
    };

    std::vector<ScryptTestVector> GetTestVectors()
    {
        return {                // RFC 7914 test vectors
                {"",            // password
                 "",            // salt
                 16, 1, 1, 64,  // N, r, p, dkLen
                 "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e"
                 "0fb2e0d3628cf35e20c38d18906"},
                {"password",       // password
                 "NaCl",           // salt
                 1024, 8, 16, 64,  // N, r, p, dkLen
                 "fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b3731622eaf30d92e22a3886ff109279d9830dac727a"
                 "fb94a83ee6d8360cbdfa2cc0640"},
                {"pleaseletmein",   // password
                 "SodiumChloride",  // salt
                 32768, 8, 1, 64,   // N, r, p, dkLen
                 "7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2d5432955613f0fcf62d49705242a9af9e61e8"
                 "5dc0d651e40dfcf017b45575887"},
                // Additional test vectors for edge cases
                {"a",           // password
                 "salt",        // salt
                 16, 1, 1, 32,  // N, r, p, dkLen
                 "48b0d1aaff03cc5aa50a9e9dd60c80e3b26da0c58bb9d67b8b5b5b3b66ad8bfb"},
                {"test",          // password
                 "testsalt",      // salt
                 1024, 1, 1, 16,  // N, r, p, dkLen
                 "4f41b14b6060a3e3516da38a8a38adc5"}};
    }
};

// Test basic Scrypt functionality with known test vectors
TEST_F(ScryptTest, KnownTestVectors)
{
    auto test_vectors = GetTestVectors();

    for (const auto& tv : test_vectors)
    {
        std::vector<uint8_t> password(tv.password.begin(), tv.password.end());
        std::vector<uint8_t> salt(tv.salt.begin(), tv.salt.end());

        auto result = Scrypt::DeriveKey(password, salt, tv.N, tv.r, tv.p, tv.dkLen);

        EXPECT_EQ(result.size(), tv.dkLen) << "Incorrect output length for test vector";

        std::string result_hex = StringExtensions::ToHexString(result);
        EXPECT_EQ(result_hex, tv.expected_hex)
            << "Scrypt output mismatch for password: " << tv.password << ", salt: " << tv.salt << ", N: " << tv.N
            << ", r: " << tv.r << ", p: " << tv.p;
    }
}

// Test empty inputs
TEST_F(ScryptTest, EmptyInputs)
{
    std::vector<uint8_t> empty_password;
    std::vector<uint8_t> empty_salt;
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Empty password
    auto result1 = Scrypt::DeriveKey(empty_password, salt, 16, 1, 1, 32);
    EXPECT_EQ(result1.size(), 32);

    // Empty salt
    auto result2 = Scrypt::DeriveKey(password, empty_salt, 16, 1, 1, 32);
    EXPECT_EQ(result2.size(), 32);

    // Both empty
    auto result3 = Scrypt::DeriveKey(empty_password, empty_salt, 16, 1, 1, 32);
    EXPECT_EQ(result3.size(), 32);

    // Results should be different
    EXPECT_NE(result1, result2);
    EXPECT_NE(result1, result3);
    EXPECT_NE(result2, result3);
}

// Test different output lengths
TEST_F(ScryptTest, DifferentOutputLengths)
{
    std::vector<uint8_t> password = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    std::vector<uint32_t> lengths = {1, 16, 32, 64, 128, 256};

    for (auto len : lengths)
    {
        auto result = Scrypt::DeriveKey(password, salt, 16, 1, 1, len);
        EXPECT_EQ(result.size(), len) << "Incorrect output length: " << len;

        // Verify all bytes are set (not all zeros)
        bool all_zeros = std::all_of(result.begin(), result.end(), [](uint8_t b) { return b == 0; });
        EXPECT_FALSE(all_zeros) << "Output is all zeros for length: " << len;
    }
}

// Test parameter validation
TEST_F(ScryptTest, ParameterValidation)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Test invalid N values (must be power of 2 and > 1)
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 0, 1, 1, 32), std::invalid_argument);
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 1, 1, 1, 32), std::invalid_argument);
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 3, 1, 1, 32), std::invalid_argument);
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 15, 1, 1, 32), std::invalid_argument);

    // Test invalid r values (must be > 0)
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 16, 0, 1, 32), std::invalid_argument);

    // Test invalid p values (must be > 0)
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 16, 1, 0, 32), std::invalid_argument);

    // Test invalid dkLen (must be > 0)
    EXPECT_THROW(Scrypt::DeriveKey(password, salt, 16, 1, 1, 0), std::invalid_argument);

    // Test very large dkLen (should handle gracefully)
    EXPECT_NO_THROW(Scrypt::DeriveKey(password, salt, 16, 1, 1, 1024));
}

// Test deterministic output
TEST_F(ScryptTest, DeterministicOutput)
{
    std::vector<uint8_t> password = {'m', 'y', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    std::vector<uint8_t> salt = {'m', 'y', 's', 'a', 'l', 't'};

    // Run same derivation multiple times
    auto result1 = Scrypt::DeriveKey(password, salt, 1024, 1, 1, 64);
    auto result2 = Scrypt::DeriveKey(password, salt, 1024, 1, 1, 64);
    auto result3 = Scrypt::DeriveKey(password, salt, 1024, 1, 1, 64);

    // All results should be identical
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
    EXPECT_EQ(result1, result3);
}

// Test different parameters produce different outputs
TEST_F(ScryptTest, DifferentParametersDifferentOutputs)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    auto result_base = Scrypt::DeriveKey(password, salt, 16, 1, 1, 32);

    // Different N
    auto result_n = Scrypt::DeriveKey(password, salt, 32, 1, 1, 32);
    EXPECT_NE(result_base, result_n);

    // Different r
    auto result_r = Scrypt::DeriveKey(password, salt, 16, 2, 1, 32);
    EXPECT_NE(result_base, result_r);

    // Different p
    auto result_p = Scrypt::DeriveKey(password, salt, 16, 1, 2, 32);
    EXPECT_NE(result_base, result_p);

    // All should be different from each other
    EXPECT_NE(result_n, result_r);
    EXPECT_NE(result_n, result_p);
    EXPECT_NE(result_r, result_p);
}

// Test password sensitivity
TEST_F(ScryptTest, PasswordSensitivity)
{
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    auto result1 = Scrypt::DeriveKey({'p', 'a', 's', 's', 'w', 'o', 'r', 'd'}, salt, 16, 1, 1, 32);
    auto result2 = Scrypt::DeriveKey({'P', 'a', 's', 's', 'w', 'o', 'r', 'd'}, salt, 16, 1, 1, 32);  // Different case
    auto result3 = Scrypt::DeriveKey({'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1'}, salt, 16, 1, 1, 32);  // Extra char
    auto result4 = Scrypt::DeriveKey({'p', 'a', 's', 's', 'w', 'o', 'r'}, salt, 16, 1, 1, 32);  // Missing char

    // All should be different
    EXPECT_NE(result1, result2);
    EXPECT_NE(result1, result3);
    EXPECT_NE(result1, result4);
    EXPECT_NE(result2, result3);
    EXPECT_NE(result2, result4);
    EXPECT_NE(result3, result4);
}

// Test salt sensitivity
TEST_F(ScryptTest, SaltSensitivity)
{
    std::vector<uint8_t> password = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};

    auto result1 = Scrypt::DeriveKey(password, {'s', 'a', 'l', 't'}, 16, 1, 1, 32);
    auto result2 = Scrypt::DeriveKey(password, {'S', 'a', 'l', 't'}, 16, 1, 1, 32);       // Different case
    auto result3 = Scrypt::DeriveKey(password, {'s', 'a', 'l', 't', '1'}, 16, 1, 1, 32);  // Extra char
    auto result4 = Scrypt::DeriveKey(password, {'s', 'a', 'l'}, 16, 1, 1, 32);            // Missing char

    // All should be different
    EXPECT_NE(result1, result2);
    EXPECT_NE(result1, result3);
    EXPECT_NE(result1, result4);
    EXPECT_NE(result2, result3);
    EXPECT_NE(result2, result4);
    EXPECT_NE(result3, result4);
}

// Test large input handling
TEST_F(ScryptTest, LargeInputs)
{
    // Large password
    std::vector<uint8_t> large_password(10000, 'a');
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    auto result1 = Scrypt::DeriveKey(large_password, salt, 16, 1, 1, 32);
    EXPECT_EQ(result1.size(), 32);

    // Large salt
    std::vector<uint8_t> password = {'p', 'a', 's', 's'};
    std::vector<uint8_t> large_salt(10000, 's');

    auto result2 = Scrypt::DeriveKey(password, large_salt, 16, 1, 1, 32);
    EXPECT_EQ(result2.size(), 32);

    // Both large
    auto result3 = Scrypt::DeriveKey(large_password, large_salt, 16, 1, 1, 32);
    EXPECT_EQ(result3.size(), 32);

    // All should be different
    EXPECT_NE(result1, result2);
    EXPECT_NE(result1, result3);
    EXPECT_NE(result2, result3);
}

// Test Unicode password handling
TEST_F(ScryptTest, UnicodePasswords)
{
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // ASCII password
    std::string ascii_password = "password";
    std::vector<uint8_t> ascii_bytes(ascii_password.begin(), ascii_password.end());
    auto result_ascii = Scrypt::DeriveKey(ascii_bytes, salt, 16, 1, 1, 32);

    // UTF-8 password (same visual representation but different bytes)
    std::string utf8_password = "pässwörd";  // Contains non-ASCII characters
    std::vector<uint8_t> utf8_bytes(utf8_password.begin(), utf8_password.end());
    auto result_utf8 = Scrypt::DeriveKey(utf8_bytes, salt, 16, 1, 1, 32);

    // Should produce different results
    EXPECT_NE(result_ascii, result_utf8);

    // Both should be valid 32-byte outputs
    EXPECT_EQ(result_ascii.size(), 32);
    EXPECT_EQ(result_utf8.size(), 32);
}

// Test performance characteristics
TEST_F(ScryptTest, PerformanceCharacteristics)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Test different N values and measure time
    std::vector<uint32_t> n_values = {16, 64, 256, 1024};

    for (auto n : n_values)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = Scrypt::DeriveKey(password, salt, n, 1, 1, 32);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        EXPECT_EQ(result.size(), 32);

        // Higher N should generally take longer (allow some variance)
        if (n >= 1024)
        {
            EXPECT_GE(duration.count(), 1);  // At least 1ms for N=1024
        }

        // Should complete within reasonable time (less than 10 seconds even for N=1024)
        EXPECT_LT(duration.count(), 10000);
    }
}

// Test memory usage scaling
TEST_F(ScryptTest, MemoryUsageScaling)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Test different r values (affects memory usage)
    std::vector<uint32_t> r_values = {1, 2, 4, 8};

    for (auto r : r_values)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = Scrypt::DeriveKey(password, salt, 16, r, 1, 32);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        EXPECT_EQ(result.size(), 32);

        // Should complete within reasonable time
        EXPECT_LT(duration.count(), 1000);  // Less than 1 second
    }
}

// Test parallelization parameter
TEST_F(ScryptTest, ParallelizationParameter)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Test different p values
    auto result_p1 = Scrypt::DeriveKey(password, salt, 16, 1, 1, 32);
    auto result_p2 = Scrypt::DeriveKey(password, salt, 16, 1, 2, 32);
    auto result_p4 = Scrypt::DeriveKey(password, salt, 16, 1, 4, 32);

    // Different p values should produce different results
    EXPECT_NE(result_p1, result_p2);
    EXPECT_NE(result_p1, result_p4);
    EXPECT_NE(result_p2, result_p4);

    // All should be valid 32-byte outputs
    EXPECT_EQ(result_p1.size(), 32);
    EXPECT_EQ(result_p2.size(), 32);
    EXPECT_EQ(result_p4.size(), 32);
}

// Test edge case: maximum reasonable parameters
TEST_F(ScryptTest, MaximumParameters)
{
    std::vector<uint8_t> password = {'t', 'e', 's', 't'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};

    // Test with large but reasonable parameters
    // Note: This test might be slow and should have appropriate timeout
    auto start = std::chrono::high_resolution_clock::now();
    auto result = Scrypt::DeriveKey(password, salt, 16384, 1, 1, 32);  // N=16384
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    EXPECT_EQ(result.size(), 32);

    // Should complete within reasonable time (less than 30 seconds)
    EXPECT_LT(duration.count(), 30);

    // Result should not be all zeros
    bool all_zeros = std::all_of(result.begin(), result.end(), [](uint8_t b) { return b == 0; });
    EXPECT_FALSE(all_zeros);
}

// Test consistency with Neo C# implementation (if test vectors are available)
TEST_F(ScryptTest, NeoConsistency)
{
    // Test with parameters commonly used in Neo
    std::vector<uint8_t> password = {'n', 'e', 'o', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    std::vector<uint8_t> salt = {'n', 'e', 'o', 's', 'a', 'l', 't'};

    // Neo typically uses these parameters for wallet key derivation
    auto result = Scrypt::DeriveKey(password, salt, 16384, 8, 8, 64);

    EXPECT_EQ(result.size(), 64);

    // Result should be deterministic
    auto result2 = Scrypt::DeriveKey(password, salt, 16384, 8, 8, 64);
    EXPECT_EQ(result, result2);
}