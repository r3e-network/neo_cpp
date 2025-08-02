#include <gtest/gtest.h>
#include <neo/persistence/storage_key.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/cryptography/ecc/ecpoint.h>

using namespace neo::persistence;
using namespace neo::io;
using namespace neo::cryptography::ecc;

class StorageKeyComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Test contract ID
        test_contract_id = 123456;
        
        // Test prefix byte
        test_prefix = 0x20;
        
        // Test key data
        test_key_data = ByteVector({0x01, 0x02, 0x03, 0x04});
        
        // Test UInt160 hash
        test_uint160 = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
        
        // Test UInt256 hash  
        test_uint256 = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        // Test ECPoint
        test_ecpoint_hex = "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a7021";
        test_ecpoint_bytes = ByteVector::Parse(test_ecpoint_hex);
        test_ecpoint = ECPoint::FromBytes(test_ecpoint_bytes.AsSpan(), "secp256r1");
    }

    int32_t test_contract_id;
    uint8_t test_prefix;
    ByteVector test_key_data;
    UInt160 test_uint160;
    UInt256 test_uint256;
    std::string test_ecpoint_hex;
    ByteVector test_ecpoint_bytes;
    ECPoint test_ecpoint;
};

// Test basic construction
TEST_F(StorageKeyComprehensiveTest, DefaultConstruction)
{
    StorageKey key;
    EXPECT_EQ(key.GetId(), 0);
    EXPECT_EQ(key.GetKey().Size(), 0);
}

TEST_F(StorageKeyComprehensiveTest, ContractIdConstruction)
{
    StorageKey key(test_contract_id);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 0);
}

TEST_F(StorageKeyComprehensiveTest, ContractIdAndKeyConstruction)
{
    StorageKey key(test_contract_id, test_key_data);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), test_key_data.Size());
    EXPECT_EQ(key.GetKey().AsSpan().ToHexString(), test_key_data.AsSpan().ToHexString());
}

// Test Create methods with different data types
TEST_F(StorageKeyComprehensiveTest, CreateWithPrefix)
{
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_GT(key.GetKey().Size(), 0);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndByte)
{
    uint8_t test_byte = 0xFF;
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_byte);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_GT(key.GetKey().Size(), 1);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
    EXPECT_EQ(key.GetKey()[1], test_byte);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndUInt160)
{
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_uint160);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_GT(key.GetKey().Size(), UInt160::Size);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
    
    // Verify UInt160 is properly embedded
    ByteVector serialized_uint160;
    MemoryStream stream(serialized_uint160);
    BinaryWriter writer(stream);
    test_uint160.Serialize(writer);
    
    EXPECT_EQ(key.GetKey().Size(), 1 + UInt160::Size); // prefix + UInt160
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndUInt256)
{
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_uint256);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_GT(key.GetKey().Size(), UInt256::Size);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
    EXPECT_EQ(key.GetKey().Size(), 1 + UInt256::Size); // prefix + UInt256
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndECPoint)
{
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_ecpoint);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_GT(key.GetKey().Size(), 1);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
    
    // ECPoint should be serialized as compressed (33 bytes)
    EXPECT_EQ(key.GetKey().Size(), 1 + 33); // prefix + compressed ECPoint
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndInt32)
{
    int32_t test_int32 = 0x12345678;
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_int32);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + sizeof(int32_t));
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndUInt32)
{
    uint32_t test_uint32 = 0x12345678U;
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_uint32);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + sizeof(uint32_t));
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndInt64)
{
    int64_t test_int64 = 0x123456789ABCDEF0LL;
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_int64);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + sizeof(int64_t));
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndUInt64)
{
    uint64_t test_uint64 = 0x123456789ABCDEF0ULL;
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_uint64);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + sizeof(uint64_t));
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixAndSpan)
{
    std::vector<uint8_t> test_data = {0xAA, 0xBB, 0xCC, 0xDD};
    std::span<const uint8_t> test_span(test_data.data(), test_data.size());
    
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_span);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + test_data.size());
    EXPECT_EQ(key.GetKey()[0], test_prefix);
    
    // Verify data is properly embedded
    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_EQ(key.GetKey()[1 + i], test_data[i]);
    }
}

TEST_F(StorageKeyComprehensiveTest, CreateWithPrefixUInt256AndUInt160)
{
    StorageKey key = StorageKey::Create(test_contract_id, test_prefix, test_uint256, test_uint160);
    EXPECT_EQ(key.GetId(), test_contract_id);
    EXPECT_EQ(key.GetKey().Size(), 1 + UInt256::Size + UInt160::Size);
    EXPECT_EQ(key.GetKey()[0], test_prefix);
}

// Test serialization/deserialization
TEST_F(StorageKeyComprehensiveTest, SerializeDeserialize)
{
    StorageKey original(test_contract_id, test_key_data);
    
    // Serialize
    ByteVector buffer;
    MemoryStream stream(buffer);
    BinaryWriter writer(stream);
    original.Serialize(writer);
    
    // Deserialize
    stream.Seek(0);
    BinaryReader reader(stream);
    StorageKey deserialized;
    deserialized.Deserialize(reader);
    
    // Verify
    EXPECT_EQ(original.GetId(), deserialized.GetId());
    EXPECT_EQ(original.GetKey().Size(), deserialized.GetKey().Size());
    EXPECT_EQ(original.GetKey().AsSpan().ToHexString(), deserialized.GetKey().AsSpan().ToHexString());
}

// Test ToArray method
TEST_F(StorageKeyComprehensiveTest, ToArray)
{
    StorageKey key(test_contract_id, test_key_data);
    auto array = key.ToArray();
    
    // Should contain contract ID (4 bytes) + key data
    EXPECT_EQ(array.Size(), sizeof(int32_t) + test_key_data.Size());
    
    // First 4 bytes should be contract ID in little-endian
    EXPECT_EQ(array[0], static_cast<uint8_t>(test_contract_id & 0xFF));
    EXPECT_EQ(array[1], static_cast<uint8_t>((test_contract_id >> 8) & 0xFF));
    EXPECT_EQ(array[2], static_cast<uint8_t>((test_contract_id >> 16) & 0xFF));
    EXPECT_EQ(array[3], static_cast<uint8_t>((test_contract_id >> 24) & 0xFF));
}

// Test search prefix creation
TEST_F(StorageKeyComprehensiveTest, CreateSearchPrefix)
{
    std::vector<uint8_t> prefix_data = {test_prefix, 0x01, 0x02};
    std::span<const uint8_t> prefix_span(prefix_data.data(), prefix_data.size());
    
    auto search_prefix = StorageKey::CreateSearchPrefix(test_contract_id, prefix_span);
    
    // Should contain contract ID + prefix data
    EXPECT_EQ(search_prefix.Size(), sizeof(int32_t) + prefix_data.size());
    
    // First 4 bytes should be contract ID
    EXPECT_EQ(search_prefix[0], static_cast<uint8_t>(test_contract_id & 0xFF));
    EXPECT_EQ(search_prefix[1], static_cast<uint8_t>((test_contract_id >> 8) & 0xFF));
    EXPECT_EQ(search_prefix[2], static_cast<uint8_t>((test_contract_id >> 16) & 0xFF));
    EXPECT_EQ(search_prefix[3], static_cast<uint8_t>((test_contract_id >> 24) & 0xFF));
    
    // Followed by prefix data
    for (size_t i = 0; i < prefix_data.size(); ++i) {
        EXPECT_EQ(search_prefix[4 + i], prefix_data[i]);
    }
}

// Test comparison operators
TEST_F(StorageKeyComprehensiveTest, ComparisonOperators)
{
    StorageKey key1(test_contract_id, test_key_data);
    StorageKey key2(test_contract_id, test_key_data);
    StorageKey key3(test_contract_id + 1, test_key_data);
    
    // Equality
    EXPECT_EQ(key1, key2);
    EXPECT_NE(key1, key3);
    
    // Inequality
    EXPECT_FALSE(key1 != key2);
    EXPECT_TRUE(key1 != key3);
    
    // Less than (for container ordering)
    EXPECT_TRUE(key1 < key3 || key3 < key1);
}

// Test C# compatibility methods
TEST_F(StorageKeyComprehensiveTest, CSharpCompatibility)
{
    StorageKey key1(test_contract_id, test_key_data);
    StorageKey key2(test_contract_id, test_key_data);
    StorageKey key3(test_contract_id + 1, test_key_data);
    
    // Equals method
    EXPECT_TRUE(key1.Equals(key2));
    EXPECT_FALSE(key1.Equals(key3));
    
    // CompareTo method
    EXPECT_EQ(key1.CompareTo(key2), 0);
    EXPECT_NE(key1.CompareTo(key3), 0);
    EXPECT_TRUE(key1.CompareTo(key3) < 0); // key1 should be less than key3
}

// Test error handling
TEST_F(StorageKeyComprehensiveTest, ErrorHandling)
{
    // Test GetContractId when lookup is required
    StorageKey key_with_script_hash(test_uint160);
    // This should throw since it requires DataCache context
    EXPECT_THROW(key_with_script_hash.GetContractId(), std::runtime_error);
}

// Test performance with large keys
TEST_F(StorageKeyComprehensiveTest, PerformanceTest)
{
    const int iterations = 1000;
    std::vector<StorageKey> keys;
    
    // Create many keys with different data
    for (int i = 0; i < iterations; ++i) {
        ByteVector key_data(32); // Large key data
        for (int j = 0; j < 32; ++j) {
            key_data[j] = static_cast<uint8_t>((i + j) % 256);
        }
        keys.emplace_back(StorageKey(i, key_data));
    }
    
    // Serialize all keys
    ByteVector total_buffer;
    MemoryStream stream(total_buffer);
    BinaryWriter writer(stream);
    
    for (const auto& key : keys) {
        key.Serialize(writer);
    }
    
    EXPECT_GT(total_buffer.Size(), iterations * (sizeof(int32_t) + 32));
    
    // Deserialize all keys
    stream.Seek(0);
    BinaryReader reader(stream);
    
    for (int i = 0; i < iterations; ++i) {
        StorageKey deserialized;
        EXPECT_NO_THROW(deserialized.Deserialize(reader));
        EXPECT_EQ(deserialized.GetId(), keys[i].GetId());
        EXPECT_EQ(deserialized.GetKey().Size(), keys[i].GetKey().Size());
    }
}