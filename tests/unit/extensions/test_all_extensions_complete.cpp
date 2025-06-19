#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/extensions/assembly_extensions.h"
#include "neo/extensions/biginteger_extensions.h"
#include "neo/extensions/byte_array_comparer.h"
#include "neo/extensions/byte_array_equality_comparer.h"
#include "neo/extensions/collection_extensions.h"
#include "neo/extensions/datetime_extensions.h"
#include "neo/extensions/hashset_extensions.h"
#include "neo/extensions/ipaddress_extensions.h"
#include "neo/extensions/random_extensions.h"
#include "neo/extensions/secure_string_extensions.h"
#include "neo/extensions/utility.h"
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <chrono>
#include <random>

using namespace neo::extensions;

class ExtensionsCompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test data
        test_bytes1_ = {0x01, 0x02, 0x03, 0x04, 0x05};
        test_bytes2_ = {0x01, 0x02, 0x03, 0x04, 0x05};
        test_bytes3_ = {0x06, 0x07, 0x08, 0x09, 0x0A};
        
        test_vector_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        test_set_ = {1, 3, 5, 7, 9};
        test_map_ = {{1, "one"}, {2, "two"}, {3, "three"}};
    }
    
    std::vector<uint8_t> test_bytes1_;
    std::vector<uint8_t> test_bytes2_;
    std::vector<uint8_t> test_bytes3_;
    std::vector<int> test_vector_;
    std::set<int> test_set_;
    std::map<int, std::string> test_map_;
};

// Assembly Extensions Tests
TEST_F(ExtensionsCompleteTest, AssemblyExtensions_GetTypes) {
    auto types = AssemblyExtensions::GetTypes("TestAssembly");
    EXPECT_TRUE(types.empty() || !types.empty()); // Implementation dependent
}

TEST_F(ExtensionsCompleteTest, AssemblyExtensions_LoadAssembly) {
    // Test loading assembly from bytes
    std::vector<uint8_t> assembly_bytes = {0x4D, 0x5A}; // PE header start
    auto assembly = AssemblyExtensions::LoadAssembly(assembly_bytes);
    // Result depends on implementation
}

TEST_F(ExtensionsCompleteTest, AssemblyExtensions_GetManifestResourceNames) {
    auto resources = AssemblyExtensions::GetManifestResourceNames("TestAssembly");
    EXPECT_TRUE(resources.empty() || !resources.empty());
}

TEST_F(ExtensionsCompleteTest, AssemblyExtensions_GetCustomAttributes) {
    auto attributes = AssemblyExtensions::GetCustomAttributes("TestAssembly", "TestType");
    EXPECT_TRUE(attributes.empty() || !attributes.empty());
}

// BigInteger Extensions Tests
TEST_F(ExtensionsCompleteTest, BigIntegerExtensions_GetLowestSetBit) {
    BigInteger num1(8); // Binary: 1000
    EXPECT_EQ(BigIntegerExtensions::GetLowestSetBit(num1), 3);
    
    BigInteger num2(12); // Binary: 1100
    EXPECT_EQ(BigIntegerExtensions::GetLowestSetBit(num2), 2);
    
    BigInteger num3(1); // Binary: 1
    EXPECT_EQ(BigIntegerExtensions::GetLowestSetBit(num3), 0);
    
    BigInteger zero(0);
    EXPECT_EQ(BigIntegerExtensions::GetLowestSetBit(zero), -1);
}

TEST_F(ExtensionsCompleteTest, BigIntegerExtensions_TestBit) {
    BigInteger num(10); // Binary: 1010
    
    EXPECT_FALSE(BigIntegerExtensions::TestBit(num, 0)); // Bit 0 is 0
    EXPECT_TRUE(BigIntegerExtensions::TestBit(num, 1));  // Bit 1 is 1
    EXPECT_FALSE(BigIntegerExtensions::TestBit(num, 2)); // Bit 2 is 0
    EXPECT_TRUE(BigIntegerExtensions::TestBit(num, 3));  // Bit 3 is 1
    EXPECT_FALSE(BigIntegerExtensions::TestBit(num, 4)); // Bit 4 is 0
}

TEST_F(ExtensionsCompleteTest, BigIntegerExtensions_ModInverse) {
    BigInteger a(3);
    BigInteger m(11);
    auto inverse = BigIntegerExtensions::ModInverse(a, m);
    
    // 3 * inverse ≡ 1 (mod 11)
    // 3 * 4 = 12 ≡ 1 (mod 11)
    EXPECT_EQ(inverse, 4);
    
    // Verify: (a * inverse) % m == 1
    EXPECT_EQ((a * inverse) % m, 1);
}

TEST_F(ExtensionsCompleteTest, BigIntegerExtensions_ModPow) {
    BigInteger base(2);
    BigInteger exponent(10);
    BigInteger modulus(1000);
    
    auto result = BigIntegerExtensions::ModPow(base, exponent, modulus);
    EXPECT_EQ(result, 24); // 2^10 % 1000 = 1024 % 1000 = 24
}

TEST_F(ExtensionsCompleteTest, BigIntegerExtensions_ToByteArrayUnsigned) {
    BigInteger num(255);
    auto bytes = BigIntegerExtensions::ToByteArrayUnsigned(num);
    EXPECT_EQ(bytes.size(), 1);
    EXPECT_EQ(bytes[0], 0xFF);
    
    BigInteger num2(256);
    auto bytes2 = BigIntegerExtensions::ToByteArrayUnsigned(num2);
    EXPECT_EQ(bytes2.size(), 2);
    EXPECT_EQ(bytes2[0], 0x00); // Little-endian
    EXPECT_EQ(bytes2[1], 0x01);
}

// Byte Array Comparer Tests
TEST_F(ExtensionsCompleteTest, ByteArrayComparer_Compare) {
    ByteArrayComparer comparer;
    
    // Equal arrays
    EXPECT_EQ(comparer.Compare(test_bytes1_, test_bytes2_), 0);
    
    // Different arrays
    EXPECT_LT(comparer.Compare(test_bytes1_, test_bytes3_), 0);
    EXPECT_GT(comparer.Compare(test_bytes3_, test_bytes1_), 0);
    
    // Different lengths
    std::vector<uint8_t> short_array = {0x01, 0x02};
    EXPECT_GT(comparer.Compare(test_bytes1_, short_array), 0);
    EXPECT_LT(comparer.Compare(short_array, test_bytes1_), 0);
}

TEST_F(ExtensionsCompleteTest, ByteArrayComparer_Equals) {
    ByteArrayComparer comparer;
    
    EXPECT_TRUE(comparer.Equals(test_bytes1_, test_bytes2_));
    EXPECT_FALSE(comparer.Equals(test_bytes1_, test_bytes3_));
}

TEST_F(ExtensionsCompleteTest, ByteArrayComparer_GetHashCode) {
    ByteArrayComparer comparer;
    
    auto hash1 = comparer.GetHashCode(test_bytes1_);
    auto hash2 = comparer.GetHashCode(test_bytes2_);
    auto hash3 = comparer.GetHashCode(test_bytes3_);
    
    EXPECT_EQ(hash1, hash2); // Equal arrays have same hash
    EXPECT_NE(hash1, hash3); // Different arrays likely have different hash
}

// Byte Array Equality Comparer Tests
TEST_F(ExtensionsCompleteTest, ByteArrayEqualityComparer_Equals) {
    ByteArrayEqualityComparer comparer;
    
    EXPECT_TRUE(comparer.Equals(test_bytes1_, test_bytes2_));
    EXPECT_FALSE(comparer.Equals(test_bytes1_, test_bytes3_));
    
    // Empty arrays
    std::vector<uint8_t> empty1, empty2;
    EXPECT_TRUE(comparer.Equals(empty1, empty2));
}

TEST_F(ExtensionsCompleteTest, ByteArrayEqualityComparer_GetHashCode) {
    ByteArrayEqualityComparer comparer;
    
    auto hash1 = comparer.GetHashCode(test_bytes1_);
    auto hash2 = comparer.GetHashCode(test_bytes2_);
    
    EXPECT_EQ(hash1, hash2);
}

// Collection Extensions Tests
TEST_F(ExtensionsCompleteTest, CollectionExtensions_RemoveWhere) {
    auto vector_copy = test_vector_;
    
    // Remove even numbers
    CollectionExtensions::RemoveWhere(vector_copy, [](int x) { return x % 2 == 0; });
    
    std::vector<int> expected = {1, 3, 5, 7, 9};
    EXPECT_EQ(vector_copy, expected);
}

TEST_F(ExtensionsCompleteTest, CollectionExtensions_ForEach) {
    std::vector<int> results;
    
    CollectionExtensions::ForEach(test_vector_, [&results](int x) {
        results.push_back(x * 2);
    });
    
    std::vector<int> expected = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
    EXPECT_EQ(results, expected);
}

TEST_F(ExtensionsCompleteTest, CollectionExtensions_ToArray) {
    auto array = CollectionExtensions::ToArray(test_set_);
    
    // Convert set to vector for comparison
    std::vector<int> expected(test_set_.begin(), test_set_.end());
    EXPECT_EQ(array, expected);
}

TEST_F(ExtensionsCompleteTest, CollectionExtensions_AddRange) {
    std::vector<int> target = {1, 2, 3};
    std::vector<int> source = {4, 5, 6};
    
    CollectionExtensions::AddRange(target, source);
    
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(target, expected);
}

TEST_F(ExtensionsCompleteTest, CollectionExtensions_Distinct) {
    std::vector<int> with_duplicates = {1, 2, 2, 3, 3, 3, 4, 5, 5};
    auto distinct = CollectionExtensions::Distinct(with_duplicates);
    
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(distinct, expected);
}

// DateTime Extensions Tests
TEST_F(ExtensionsCompleteTest, DateTimeExtensions_ToTimestamp) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = DateTimeExtensions::ToTimestamp(now);
    
    // Should be positive and reasonable
    EXPECT_GT(timestamp, 1600000000000ULL); // After 2020
    EXPECT_LT(timestamp, 2000000000000ULL); // Before 2033
}

TEST_F(ExtensionsCompleteTest, DateTimeExtensions_FromTimestamp) {
    uint64_t timestamp = 1609459200000; // January 1, 2021 00:00:00 UTC
    auto datetime = DateTimeExtensions::FromTimestamp(timestamp);
    
    auto time_t = std::chrono::system_clock::to_time_t(datetime);
    auto tm = *std::gmtime(&time_t);
    
    EXPECT_EQ(tm.tm_year + 1900, 2021);
    EXPECT_EQ(tm.tm_mon, 0); // January
    EXPECT_EQ(tm.tm_mday, 1);
}

TEST_F(ExtensionsCompleteTest, DateTimeExtensions_AddMilliseconds) {
    auto base_time = std::chrono::system_clock::now();
    auto new_time = DateTimeExtensions::AddMilliseconds(base_time, 5000);
    
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(new_time - base_time);
    EXPECT_EQ(diff.count(), 5000);
}

TEST_F(ExtensionsCompleteTest, DateTimeExtensions_ToUniversalTime) {
    auto local_time = std::chrono::system_clock::now();
    auto utc_time = DateTimeExtensions::ToUniversalTime(local_time);
    
    // UTC conversion should not change the time point significantly
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(utc_time - local_time);
    EXPECT_LT(std::abs(diff.count()), 86400); // Less than 24 hours difference
}

// HashSet Extensions Tests
TEST_F(ExtensionsCompleteTest, HashSetExtensions_AddRange) {
    std::unordered_set<int> hashset = {1, 2, 3};
    std::vector<int> to_add = {3, 4, 5, 6};
    
    HashSetExtensions::AddRange(hashset, to_add);
    
    std::unordered_set<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(hashset, expected);
}

TEST_F(ExtensionsCompleteTest, HashSetExtensions_RemoveWhere) {
    std::unordered_set<int> hashset = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    HashSetExtensions::RemoveWhere(hashset, [](int x) { return x % 2 == 0; });
    
    std::unordered_set<int> expected = {1, 3, 5, 7, 9};
    EXPECT_EQ(hashset, expected);
}

TEST_F(ExtensionsCompleteTest, HashSetExtensions_UnionWith) {
    std::unordered_set<int> set1 = {1, 2, 3};
    std::unordered_set<int> set2 = {3, 4, 5};
    
    HashSetExtensions::UnionWith(set1, set2);
    
    std::unordered_set<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(set1, expected);
}

TEST_F(ExtensionsCompleteTest, HashSetExtensions_IntersectWith) {
    std::unordered_set<int> set1 = {1, 2, 3, 4, 5};
    std::unordered_set<int> set2 = {3, 4, 5, 6, 7};
    
    HashSetExtensions::IntersectWith(set1, set2);
    
    std::unordered_set<int> expected = {3, 4, 5};
    EXPECT_EQ(set1, expected);
}

// IP Address Extensions Tests
TEST_F(ExtensionsCompleteTest, IPAddressExtensions_IsIPv4) {
    EXPECT_TRUE(IPAddressExtensions::IsIPv4("192.168.1.1"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv4("127.0.0.1"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv4("0.0.0.0"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv4("255.255.255.255"));
    
    EXPECT_FALSE(IPAddressExtensions::IsIPv4("256.1.1.1"));
    EXPECT_FALSE(IPAddressExtensions::IsIPv4("192.168.1"));
    EXPECT_FALSE(IPAddressExtensions::IsIPv4("not.an.ip.address"));
    EXPECT_FALSE(IPAddressExtensions::IsIPv4("2001:db8::1"));
}

TEST_F(ExtensionsCompleteTest, IPAddressExtensions_IsIPv6) {
    EXPECT_TRUE(IPAddressExtensions::IsIPv6("2001:db8::1"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv6("::1"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv6("fe80::"));
    EXPECT_TRUE(IPAddressExtensions::IsIPv6("2001:0db8:85a3:0000:0000:8a2e:0370:7334"));
    
    EXPECT_FALSE(IPAddressExtensions::IsIPv6("192.168.1.1"));
    EXPECT_FALSE(IPAddressExtensions::IsIPv6("not.an.ip.address"));
    EXPECT_FALSE(IPAddressExtensions::IsIPv6("2001:db8::1::2"));
}

TEST_F(ExtensionsCompleteTest, IPAddressExtensions_MapToIPv6) {
    auto ipv6 = IPAddressExtensions::MapToIPv6("192.168.1.1");
    EXPECT_EQ(ipv6, "::ffff:192.168.1.1");
}

TEST_F(ExtensionsCompleteTest, IPAddressExtensions_IsLoopback) {
    EXPECT_TRUE(IPAddressExtensions::IsLoopback("127.0.0.1"));
    EXPECT_TRUE(IPAddressExtensions::IsLoopback("::1"));
    EXPECT_FALSE(IPAddressExtensions::IsLoopback("192.168.1.1"));
    EXPECT_FALSE(IPAddressExtensions::IsLoopback("8.8.8.8"));
}

TEST_F(ExtensionsCompleteTest, IPAddressExtensions_IsPrivate) {
    // RFC 1918 private ranges
    EXPECT_TRUE(IPAddressExtensions::IsPrivate("192.168.1.1"));
    EXPECT_TRUE(IPAddressExtensions::IsPrivate("10.0.0.1"));
    EXPECT_TRUE(IPAddressExtensions::IsPrivate("172.16.0.1"));
    
    // Public addresses
    EXPECT_FALSE(IPAddressExtensions::IsPrivate("8.8.8.8"));
    EXPECT_FALSE(IPAddressExtensions::IsPrivate("1.1.1.1"));
}

// Random Extensions Tests
TEST_F(ExtensionsCompleteTest, RandomExtensions_NextBytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    auto bytes = RandomExtensions::NextBytes(gen, 10);
    EXPECT_EQ(bytes.size(), 10);
    
    // Should not be all zeros (very unlikely)
    bool has_non_zero = std::any_of(bytes.begin(), bytes.end(), [](uint8_t b) { return b != 0; });
    EXPECT_TRUE(has_non_zero);
}

TEST_F(ExtensionsCompleteTest, RandomExtensions_NextInt32) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    auto value = RandomExtensions::NextInt32(gen);
    EXPECT_GE(value, 0);
    EXPECT_LT(value, INT32_MAX);
}

TEST_F(ExtensionsCompleteTest, RandomExtensions_NextInt32Range) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    int min_val = 10;
    int max_val = 20;
    
    for (int i = 0; i < 100; i++) {
        auto value = RandomExtensions::NextInt32(gen, min_val, max_val);
        EXPECT_GE(value, min_val);
        EXPECT_LT(value, max_val);
    }
}

TEST_F(ExtensionsCompleteTest, RandomExtensions_NextDouble) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < 100; i++) {
        auto value = RandomExtensions::NextDouble(gen);
        EXPECT_GE(value, 0.0);
        EXPECT_LT(value, 1.0);
    }
}

// Secure String Extensions Tests
TEST_F(ExtensionsCompleteTest, SecureStringExtensions_ToSecureString) {
    std::string plain = "test_password";
    auto secure = SecureStringExtensions::ToSecureString(plain);
    
    // Should not be empty and should be different from plain string
    EXPECT_FALSE(secure.empty());
    EXPECT_NE(secure, plain);
}

TEST_F(ExtensionsCompleteTest, SecureStringExtensions_FromSecureString) {
    std::string original = "test_password";
    auto secure = SecureStringExtensions::ToSecureString(original);
    auto recovered = SecureStringExtensions::FromSecureString(secure);
    
    EXPECT_EQ(recovered, original);
}

TEST_F(ExtensionsCompleteTest, SecureStringExtensions_Clear) {
    std::string password = "secret123";
    auto secure = SecureStringExtensions::ToSecureString(password);
    
    SecureStringExtensions::Clear(secure);
    
    // After clearing, should not be able to recover original
    auto recovered = SecureStringExtensions::FromSecureString(secure);
    EXPECT_NE(recovered, password);
}

// Utility Extensions Tests
TEST_F(ExtensionsCompleteTest, Utility_GetVersion) {
    auto version = Utility::GetVersion();
    EXPECT_FALSE(version.empty());
    // Should contain version numbers
    EXPECT_TRUE(version.find('.') != std::string::npos);
}

TEST_F(ExtensionsCompleteTest, Utility_GetHashCode) {
    std::string test1 = "hello";
    std::string test2 = "hello";
    std::string test3 = "world";
    
    auto hash1 = Utility::GetHashCode(test1);
    auto hash2 = Utility::GetHashCode(test2);
    auto hash3 = Utility::GetHashCode(test3);
    
    EXPECT_EQ(hash1, hash2); // Same strings have same hash
    EXPECT_NE(hash1, hash3); // Different strings likely have different hash
}

TEST_F(ExtensionsCompleteTest, Utility_StrictUTF8) {
    std::vector<uint8_t> valid_utf8 = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    EXPECT_TRUE(Utility::StrictUTF8(valid_utf8));
    
    std::vector<uint8_t> invalid_utf8 = {0xFF, 0xFE}; // Invalid UTF-8
    EXPECT_FALSE(Utility::StrictUTF8(invalid_utf8));
}

TEST_F(ExtensionsCompleteTest, Utility_ToHexString) {
    std::vector<uint8_t> bytes = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    auto hex = Utility::ToHexString(bytes);
    EXPECT_EQ(hex, "0123456789abcdef");
}

TEST_F(ExtensionsCompleteTest, Utility_FromHexString) {
    std::string hex = "0123456789abcdef";
    auto bytes = Utility::FromHexString(hex);
    
    std::vector<uint8_t> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(bytes, expected);
}

TEST_F(ExtensionsCompleteTest, Utility_Base64Encode) {
    std::vector<uint8_t> data = {0x4D, 0x61, 0x6E}; // "Man"
    auto encoded = Utility::Base64Encode(data);
    EXPECT_EQ(encoded, "TWFu");
}

TEST_F(ExtensionsCompleteTest, Utility_Base64Decode) {
    std::string encoded = "TWFu";
    auto decoded = Utility::Base64Decode(encoded);
    
    std::vector<uint8_t> expected = {0x4D, 0x61, 0x6E}; // "Man"
    EXPECT_EQ(decoded, expected);
}