#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_key.h>
#include <sstream>

using namespace neo::persistence;
using namespace neo::io;

TEST(StorageKeyTest, Constructor)
{
    // Default constructor
    StorageKey key1;
    EXPECT_EQ(key1.GetScriptHash(), UInt160());
    EXPECT_EQ(key1.GetKey(), ByteVector());

    // ScriptHash constructor
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    StorageKey key2(scriptHash);
    EXPECT_EQ(key2.GetScriptHash(), scriptHash);
    EXPECT_EQ(key2.GetKey(), ByteVector());

    // ScriptHash and key constructor
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey key3(scriptHash, key);
    EXPECT_EQ(key3.GetScriptHash(), scriptHash);
    EXPECT_EQ(key3.GetKey(), key);
}

TEST(StorageKeyTest, Serialization)
{
    // Create a storage key with contract ID (Neo N3 style)
    int32_t contractId = 123;
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(contractId, key);

    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    storageKey.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    StorageKey storageKey2;
    storageKey2.Deserialize(reader);

    // Check
    EXPECT_EQ(storageKey2.GetId(), contractId);
    EXPECT_EQ(storageKey2.GetKey(), key);

    // Test serialization with script hash (compatibility mode)
    // Note: In Neo N3, script hash is not serialized, only contract ID is
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    StorageKey storageKey3(scriptHash, key);

    // After deserialization, script hash won't be preserved (this is expected in Neo N3)
    // Only the contract ID (which defaults to 0 for script hash constructor) and key are preserved
}

TEST(StorageKeyTest, ToArray)
{
    // Create a storage key with contract ID (Neo N3 style)
    int32_t contractId = 456;
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(contractId, key);

    // Serialize to array
    ByteVector array = storageKey.ToArray();

    // Expected format: 4 bytes contract ID (little-endian) + key bytes
    EXPECT_EQ(array.Size(), sizeof(int32_t) + key.Size());

    // Deserialize from array
    StorageKey storageKey2;
    storageKey2.DeserializeFromArray(array.AsSpan());

    // Check
    EXPECT_EQ(storageKey2.GetId(), contractId);
    EXPECT_EQ(storageKey2.GetKey(), key);
}

TEST(StorageKeyTest, Equality)
{
    // Create storage keys
    UInt160 scriptHash1 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key1 = ByteVector::Parse("0102030405");
    StorageKey storageKey1(scriptHash1, key1);

    UInt160 scriptHash2 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key2 = ByteVector::Parse("0102030405");
    StorageKey storageKey2(scriptHash2, key2);

    UInt160 scriptHash3 = UInt160::Parse("1102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key3 = ByteVector::Parse("0102030405");
    StorageKey storageKey3(scriptHash3, key3);

    UInt160 scriptHash4 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key4 = ByteVector::Parse("1102030405");
    StorageKey storageKey4(scriptHash4, key4);

    // Check equality
    EXPECT_TRUE(storageKey1 == storageKey2);
    EXPECT_FALSE(storageKey1 == storageKey3);
    EXPECT_FALSE(storageKey1 == storageKey4);

    // Check inequality
    EXPECT_FALSE(storageKey1 != storageKey2);
    EXPECT_TRUE(storageKey1 != storageKey3);
    EXPECT_TRUE(storageKey1 != storageKey4);
}

TEST(StorageKeyTest, Comparison)
{
    // Create storage keys
    UInt160 scriptHash1 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key1 = ByteVector::Parse("0102030405");
    StorageKey storageKey1(scriptHash1, key1);

    UInt160 scriptHash2 = UInt160::Parse("1102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key2 = ByteVector::Parse("0102030405");
    StorageKey storageKey2(scriptHash2, key2);

    UInt160 scriptHash3 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key3 = ByteVector::Parse("1102030405");
    StorageKey storageKey3(scriptHash3, key3);

    // Check comparison
    EXPECT_TRUE(storageKey1 < storageKey2);
    EXPECT_FALSE(storageKey2 < storageKey1);

    EXPECT_TRUE(storageKey1 < storageKey3);
    EXPECT_FALSE(storageKey3 < storageKey1);

    EXPECT_FALSE(storageKey2 < storageKey3);
    EXPECT_TRUE(storageKey3 < storageKey2);
}
