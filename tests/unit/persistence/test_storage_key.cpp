#include <gtest/gtest.h>
#include <neo/persistence/storage_key.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
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
    // Create a storage key
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(scriptHash, key);
    
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
    EXPECT_EQ(storageKey2.GetScriptHash(), scriptHash);
    EXPECT_EQ(storageKey2.GetKey(), key);
}

TEST(StorageKeyTest, ToArray)
{
    // Create a storage key
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(scriptHash, key);
    
    // Serialize to array
    ByteVector array = storageKey.ToArray();
    
    // Deserialize from array
    StorageKey storageKey2;
    storageKey2.DeserializeFromArray(array.AsSpan());
    
    // Check
    EXPECT_EQ(storageKey2.GetScriptHash(), scriptHash);
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
