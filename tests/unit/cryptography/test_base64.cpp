#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "neo/cryptography/base64.h"
#include "neo/extensions/string_extensions.h"

using namespace neo::cryptography;
using namespace neo::extensions;

class Base64Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Known test vectors for Base64 encoding/decoding
        test_vectors_ = {// RFC 4648 test vectors
                         {"", ""},
                         {"f", "Zg=="},
                         {"fo", "Zm8="},
                         {"foo", "Zm9v"},
                         {"foob", "Zm9vYg=="},
                         {"fooba", "Zm9vYmE="},
                         {"foobar", "Zm9vYmFy"},

                         // Additional test cases
                         {"pleasure.", "cGxlYXN1cmUu"},
                         {"leasure.", "bGVhc3VyZS4="},
                         {"easure.", "ZWFzdXJlLg=="},
                         {"asure.", "YXN1cmUu"},
                         {"sure.", "c3VyZS4="},
                         {"ure.", "dXJlLg=="},
                         {"re.", "cmUu"},
                         {"e.", "ZS4="},
                         {".", "Lg=="},

                         // Binary data test cases
                         {"\x00", "AA=="},
                         {"\x00\x00", "AAA="},
                         {"\x00\x00\x00", "AAAA"},
                         {"\xFF", "/w=="},
                         {"\xFF\xFF", "//8="},
                         {"\xFF\xFF\xFF", "////"},

                         // Mixed ASCII and binary
                         {"Hello\x00World", "SGVsbG8AV29ybGQ="},
                         {"Neo\x01\x02\x03Blockchain", "TmVvAQIDQmxvY2tjaGFpbg=="},

                         // Long strings
                         {"The quick brown fox jumps over the lazy dog",
                          "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw=="},
                         {"Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
                          "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4="}};
    }

    struct Base64TestVector
    {
        std::string input;
        std::string expected_output;
    };

    std::vector<Base64TestVector> test_vectors_;
};

// Test basic Base64 encoding
TEST_F(Base64Test, BasicEncoding)
{
    for (const auto& tv : test_vectors_)
    {
        std::vector<uint8_t> input(tv.input.begin(), tv.input.end());
        std::string result = Base64::Encode(input);

        EXPECT_EQ(result, tv.expected_output) << "Failed encoding for input: '" << tv.input << "'";
    }
}

// Test basic Base64 decoding
TEST_F(Base64Test, BasicDecoding)
{
    for (const auto& tv : test_vectors_)
    {
        if (tv.expected_output.empty() && tv.input.empty())
        {
            continue;  // Skip empty test case for decoding
        }

        auto result = Base64::Decode(tv.expected_output);
        std::string result_string(result.begin(), result.end());

        EXPECT_EQ(result_string, tv.input) << "Failed decoding for input: '" << tv.expected_output << "'";
    }
}

// Test round-trip encoding and decoding
TEST_F(Base64Test, RoundTripEncodingDecoding)
{
    std::vector<std::string> test_strings = {"Hello, World!",
                                             "Neo blockchain",
                                             "1234567890",
                                             "!@#$%^&*()",
                                             "The quick brown fox jumps over the lazy dog",
                                             "",
                                             "a",
                                             "ab",
                                             "abc",
                                             "abcd",
                                             "abcde",
                                             "abcdef"};

    for (const auto& test_string : test_strings)
    {
        std::vector<uint8_t> original(test_string.begin(), test_string.end());

        // Encode
        std::string encoded = Base64::Encode(original);

        // Decode
        auto decoded = Base64::Decode(encoded);

        // Should match original
        EXPECT_EQ(original, decoded) << "Round-trip failed for: '" << test_string << "'";
    }
}

// Test binary data encoding/decoding
TEST_F(Base64Test, BinaryDataHandling)
{
    // Test with various binary patterns
    std::vector<std::vector<uint8_t>> binary_data = {
        {0x00},
        {0xFF},
        {0x00, 0xFF},
        {0xFF, 0x00},
        {0x00, 0x00, 0x00},
        {0xFF, 0xFF, 0xFF},
        {0xAA, 0xBB, 0xCC, 0xDD},
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
        // Random binary data
        {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44}};

    for (const auto& data : binary_data)
    {
        std::string encoded = Base64::Encode(data);
        auto decoded = Base64::Decode(encoded);

        EXPECT_EQ(data, decoded) << "Binary data round-trip failed";

        // Encoded string should only contain valid Base64 characters
        for (char c : encoded)
        {
            EXPECT_TRUE((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' ||
                        c == '/' || c == '=')
                << "Invalid Base64 character: " << c;
        }
    }
}

// Test empty input handling
TEST_F(Base64Test, EmptyInputHandling)
{
    // Empty encoding
    std::vector<uint8_t> empty_input;
    std::string encoded = Base64::Encode(empty_input);
    EXPECT_EQ(encoded, "");

    // Empty decoding
    auto decoded = Base64::Decode("");
    EXPECT_TRUE(decoded.empty());

    // Round trip with empty data
    auto round_trip = Base64::Decode(Base64::Encode(empty_input));
    EXPECT_EQ(empty_input, round_trip);
}

// Test invalid Base64 input handling
TEST_F(Base64Test, InvalidInputHandling)
{
    std::vector<std::string> invalid_inputs = {
        "A",      // Invalid length (not multiple of 4 without padding)
        "AB",     // Invalid length
        "ABC",    // Invalid length
        "A===",   // Too much padding
        "AB==A",  // Padding in wrong position
        "A=BC",   // Padding in wrong position
        "ABCD@",  // Invalid character
        "ABCD#",  // Invalid character
        "ABCD$",  // Invalid character
        "ABC\n",  // Newline character
        "ABC ",   // Space character
        "AB\tC",  // Tab character
    };

    for (const auto& invalid_input : invalid_inputs)
    {
        EXPECT_THROW(Base64::Decode(invalid_input), std::invalid_argument)
            << "Should throw for invalid input: '" << invalid_input << "'";
    }
}

// Test padding validation
TEST_F(Base64Test, PaddingValidation)
{
    // Valid padding cases
    std::vector<std::string> valid_padded = {
        "QQ==",  // 1 byte input
        "QUI=",  // 2 byte input
        "QUJD",  // 3 byte input (no padding needed)
    };

    for (const auto& input : valid_padded)
    {
        EXPECT_NO_THROW(Base64::Decode(input)) << "Should not throw for valid padded input: " << input;
    }

    // Invalid padding cases
    std::vector<std::string> invalid_padded = {
        "Q===",  // Too much padding
        "QQ=A",  // Padding followed by data
        "Q=Q=",  // Padding in middle
    };

    for (const auto& input : invalid_padded)
    {
        EXPECT_THROW(Base64::Decode(input), std::invalid_argument)
            << "Should throw for invalid padded input: " << input;
    }
}

// Test case sensitivity
TEST_F(Base64Test, CaseSensitivity)
{
    std::string test_input = "Hello World";
    std::vector<uint8_t> input_bytes(test_input.begin(), test_input.end());

    std::string encoded = Base64::Encode(input_bytes);
    auto decoded = Base64::Decode(encoded);

    EXPECT_EQ(input_bytes, decoded);

    // Base64 is case sensitive, so changing case should result in different decoding
    std::string encoded_upper = encoded;
    std::transform(encoded_upper.begin(), encoded_upper.end(), encoded_upper.begin(), ::toupper);

    if (encoded != encoded_upper)
    {
        // Only test if there was actually a case change
        EXPECT_THROW(Base64::Decode(encoded_upper), std::invalid_argument);
    }
}

// Test URL-safe Base64 encoding (if supported)
TEST_F(Base64Test, URLSafeEncoding)
{
    // Test data that would contain + and / in standard Base64
    std::vector<uint8_t> test_data = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9};

    std::string standard_encoded = Base64::Encode(test_data);

    // URL-safe encoding replaces + with - and / with _
    std::string url_safe_encoded = Base64::EncodeUrlSafe(test_data);

    // Should not contain + or /
    EXPECT_EQ(url_safe_encoded.find('+'), std::string::npos);
    EXPECT_EQ(url_safe_encoded.find('/'), std::string::npos);

    // Should be decodable
    auto decoded = Base64::DecodeUrlSafe(url_safe_encoded);
    EXPECT_EQ(test_data, decoded);

    // Standard and URL-safe should produce different results (if + or / present)
    if (standard_encoded.find('+') != std::string::npos || standard_encoded.find('/') != std::string::npos)
    {
        EXPECT_NE(standard_encoded, url_safe_encoded);
    }
}

// Test line breaking (if supported)
TEST_F(Base64Test, LineBreaking)
{
    // Long input that would benefit from line breaking
    std::string long_input = std::string(100, 'A');  // 100 'A' characters
    std::vector<uint8_t> input_bytes(long_input.begin(), long_input.end());

    // Standard encoding (no line breaks)
    std::string encoded = Base64::Encode(input_bytes);
    EXPECT_EQ(encoded.find('\n'), std::string::npos);
    EXPECT_EQ(encoded.find('\r'), std::string::npos);

    // Line-broken encoding (76 characters per line)
    std::string encoded_with_breaks = Base64::EncodeWithLineBreaks(input_bytes, 76);

    // Should contain line breaks for long input
    if (encoded.length() > 76)
    {
        EXPECT_NE(encoded_with_breaks.find('\n'), std::string::npos);
    }

    // Should decode to same result
    auto decoded = Base64::DecodeIgnoreWhitespace(encoded_with_breaks);
    EXPECT_EQ(input_bytes, decoded);
}

// Test performance with large data
TEST_F(Base64Test, PerformanceWithLargeData)
{
    // Create large test data (1MB)
    const size_t data_size = 1024 * 1024;
    std::vector<uint8_t> large_data(data_size);

    // Fill with pattern
    for (size_t i = 0; i < data_size; ++i)
    {
        large_data[i] = static_cast<uint8_t>(i & 0xFF);
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Encode
    std::string encoded = Base64::Encode(large_data);

    auto encode_time = std::chrono::high_resolution_clock::now();

    // Decode
    auto decoded = Base64::Decode(encoded);

    auto end_time = std::chrono::high_resolution_clock::now();

    // Verify correctness
    EXPECT_EQ(large_data, decoded);

    // Performance metrics
    auto encode_duration = std::chrono::duration_cast<std::chrono::milliseconds>(encode_time - start_time);
    auto decode_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - encode_time);

    // Should handle 1MB within reasonable time (less than 1 second each)
    EXPECT_LT(encode_duration.count(), 1000);
    EXPECT_LT(decode_duration.count(), 1000);

    std::cout << "Performance metrics for 1MB data:" << std::endl;
    std::cout << "Encoding: " << encode_duration.count() << " ms" << std::endl;
    std::cout << "Decoding: " << decode_duration.count() << " ms" << std::endl;
}

// Test concurrent Base64 operations
TEST_F(Base64Test, ConcurrentOperations)
{
    const int num_threads = 4;
    const int operations_per_thread = 100;

    std::atomic<int> successful_operations{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [operations_per_thread, &successful_operations]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    try
                    {
                        // Create test data
                        std::string test_data = "Thread" + std::to_string(i) + "Data";
                        std::vector<uint8_t> input(test_data.begin(), test_data.end());

                        // Encode and decode
                        std::string encoded = Base64::Encode(input);
                        auto decoded = Base64::Decode(encoded);

                        if (input == decoded)
                        {
                            successful_operations++;
                        }
                    }
                    catch (...)
                    {
                        // Handle any threading issues
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Should handle concurrent operations without issues
    EXPECT_EQ(successful_operations.load(), num_threads * operations_per_thread);
}

// Test streaming operations (if supported)
TEST_F(Base64Test, StreamingOperations)
{
    std::string large_input = std::string(10000, 'X');  // 10KB of 'X'
    std::vector<uint8_t> input_bytes(large_input.begin(), large_input.end());

    // Create encoder
    Base64Encoder encoder;
    std::string encoded_result;

    // Feed data in chunks
    size_t chunk_size = 1000;
    for (size_t i = 0; i < input_bytes.size(); i += chunk_size)
    {
        size_t current_chunk_size = std::min(chunk_size, input_bytes.size() - i);
        std::vector<uint8_t> chunk(input_bytes.begin() + i, input_bytes.begin() + i + current_chunk_size);

        encoded_result += encoder.EncodeChunk(chunk);
    }
    encoded_result += encoder.Finalize();

    // Verify streaming result matches batch result
    std::string batch_encoded = Base64::Encode(input_bytes);
    EXPECT_EQ(encoded_result, batch_encoded);

    // Test streaming decoder
    Base64Decoder decoder;
    std::vector<uint8_t> decoded_result;

    // Feed encoded data in chunks
    for (size_t i = 0; i < encoded_result.size(); i += chunk_size)
    {
        size_t current_chunk_size = std::min(chunk_size, encoded_result.size() - i);
        std::string chunk = encoded_result.substr(i, current_chunk_size);

        auto chunk_decoded = decoder.DecodeChunk(chunk);
        decoded_result.insert(decoded_result.end(), chunk_decoded.begin(), chunk_decoded.end());
    }
    auto final_decoded = decoder.Finalize();
    decoded_result.insert(decoded_result.end(), final_decoded.begin(), final_decoded.end());

    // Should match original
    EXPECT_EQ(input_bytes, decoded_result);
}

// Test edge cases with special characters
TEST_F(Base64Test, SpecialCharacterHandling)
{
    std::vector<std::string> special_inputs = {
        "\x00\x01\x02\x03",      // Low bytes
        "\xFC\xFD\xFE\xFF",      // High bytes
        "\x7F\x80\x81\x82",      // Around ASCII boundary
        "Hello\x00\x00World",    // Embedded nulls
        "\n\r\t",                // Whitespace characters
        "Unicode: ñ á é í ó ú",  // Unicode characters (UTF-8)
    };

    for (const auto& input : special_inputs)
    {
        std::vector<uint8_t> input_bytes(input.begin(), input.end());

        std::string encoded = Base64::Encode(input_bytes);
        auto decoded = Base64::Decode(encoded);

        EXPECT_EQ(input_bytes, decoded) << "Failed for special input with length " << input.length();
    }
}

// Test compliance with RFC 4648
TEST_F(Base64Test, RFC4648Compliance)
{
    // Test vectors from RFC 4648
    struct RFC4648TestVector
    {
        std::string input;
        std::string base64;
        std::string base64url;  // URL-safe variant
    };

    std::vector<RFC4648TestVector> rfc_vectors = {
        {"", "", ""},
        {"f", "Zg==", "Zg=="},
        {"fo", "Zm8=", "Zm8="},
        {"foo", "Zm9v", "Zm9v"},
        {"foob", "Zm9vYg==", "Zm9vYg=="},
        {"fooba", "Zm9vYmE=", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy", "Zm9vYmFy"},
    };

    for (const auto& tv : rfc_vectors)
    {
        std::vector<uint8_t> input(tv.input.begin(), tv.input.end());

        // Test standard Base64
        std::string encoded = Base64::Encode(input);
        EXPECT_EQ(encoded, tv.base64) << "RFC 4648 standard encoding failed for: " << tv.input;

        auto decoded = Base64::Decode(tv.base64);
        EXPECT_EQ(decoded, input) << "RFC 4648 standard decoding failed for: " << tv.base64;

        // Test URL-safe Base64 (if different)
        if (tv.base64url != tv.base64)
        {
            std::string url_encoded = Base64::EncodeUrlSafe(input);
            EXPECT_EQ(url_encoded, tv.base64url) << "RFC 4648 URL-safe encoding failed for: " << tv.input;

            auto url_decoded = Base64::DecodeUrlSafe(tv.base64url);
            EXPECT_EQ(url_decoded, input) << "RFC 4648 URL-safe decoding failed for: " << tv.base64url;
        }
    }
}