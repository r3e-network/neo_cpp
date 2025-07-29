#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <sstream>

using namespace neo::persistence;
using namespace neo::io;

TEST(StorageItemTest, Constructor)
{
    // Default constructor
    StorageItem item1;
    EXPECT_EQ(item1.GetValue(), ByteVector());

    // Value constructor
    ByteVector value = ByteVector::Parse("0102030405");
    StorageItem item2(value);
    EXPECT_EQ(item2.GetValue(), value);
}

TEST(StorageItemTest, SetValue)
{
    // Create a storage item
    StorageItem item;
    EXPECT_EQ(item.GetValue(), ByteVector());

    // Set the value
    ByteVector value = ByteVector::Parse("0102030405");
    item.SetValue(value);
    EXPECT_EQ(item.GetValue(), value);

    // Set a different value
    ByteVector value2 = ByteVector::Parse("0607080910");
    item.SetValue(value2);
    EXPECT_EQ(item.GetValue(), value2);
}

TEST(StorageItemTest, Serialization)
{
    // Create a storage item
    ByteVector value = ByteVector::Parse("0102030405");
    StorageItem item(value);

    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    item.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    StorageItem item2;
    item2.Deserialize(reader);

    // Check
    EXPECT_EQ(item2.GetValue(), value);
}

TEST(StorageItemTest, ToArray)
{
    // Create a storage item
    ByteVector value = ByteVector::Parse("0102030405");
    StorageItem item(value);

    // Serialize to array
    ByteVector array = item.ToArray();

    // Deserialize from array
    StorageItem item2;
    item2.DeserializeFromArray(array.AsSpan());

    // Check
    EXPECT_EQ(item2.GetValue(), value);
}

TEST(StorageItemTest, Equality)
{
    // Create storage items
    ByteVector value1 = ByteVector::Parse("0102030405");
    StorageItem item1(value1);

    ByteVector value2 = ByteVector::Parse("0102030405");
    StorageItem item2(value2);

    ByteVector value3 = ByteVector::Parse("0607080910");
    StorageItem item3(value3);

    // Check equality
    EXPECT_TRUE(item1 == item2);
    EXPECT_FALSE(item1 == item3);

    // Check inequality
    EXPECT_FALSE(item1 != item2);
    EXPECT_TRUE(item1 != item3);
}
