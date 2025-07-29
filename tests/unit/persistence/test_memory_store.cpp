#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/persistence/memory_store.h>

using namespace neo::persistence;
using namespace neo::io;

TEST(MemoryStoreTest, TryGet)
{
    MemoryStore store;

    // Empty store
    ByteVector key = ByteVector::Parse("0102030405");
    auto value = store.TryGet(key);
    EXPECT_FALSE(value.has_value());

    // Add a value
    ByteVector value1 = ByteVector::Parse("0607080910");
    store.Put(key, value1);

    auto value2 = store.TryGet(key);
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(*value2, value1);

    // Get a non-existent key
    ByteVector key2 = ByteVector::Parse("1112131415");
    auto value3 = store.TryGet(key2);
    EXPECT_FALSE(value3.has_value());
}

TEST(MemoryStoreTest, Contains)
{
    MemoryStore store;

    // Empty store
    ByteVector key = ByteVector::Parse("0102030405");
    EXPECT_FALSE(store.Contains(key));

    // Add a value
    ByteVector value = ByteVector::Parse("0607080910");
    store.Put(key, value);

    EXPECT_TRUE(store.Contains(key));

    // Check a non-existent key
    ByteVector key2 = ByteVector::Parse("1112131415");
    EXPECT_FALSE(store.Contains(key2));
}

TEST(MemoryStoreTest, Find)
{
    MemoryStore store;

    // Add some values
    ByteVector key1 = ByteVector::Parse("0102030405");
    ByteVector value1 = ByteVector::Parse("0607080910");
    store.Put(key1, value1);

    ByteVector key2 = ByteVector::Parse("0102031415");
    ByteVector value2 = ByteVector::Parse("1617181920");
    store.Put(key2, value2);

    ByteVector key3 = ByteVector::Parse("0103030405");
    ByteVector value3 = ByteVector::Parse("2627282930");
    store.Put(key3, value3);

    // Find all
    auto result1 = store.Find();
    EXPECT_EQ(result1.size(), 3);

    // Find with prefix
    ByteVector prefix = ByteVector::Parse("0102");
    auto result2 = store.Find(&prefix);
    EXPECT_EQ(result2.size(), 2);

    // Find with non-existent prefix
    ByteVector prefix2 = ByteVector::Parse("0104");
    auto result3 = store.Find(&prefix2);
    EXPECT_EQ(result3.size(), 0);

    // Find with direction
    auto result4 = store.Find(nullptr, SeekDirection::Backward);
    EXPECT_EQ(result4.size(), 3);

    // Find with prefix and direction
    auto result5 = store.Find(&prefix, SeekDirection::Backward);
    EXPECT_EQ(result5.size(), 2);
}

TEST(MemoryStoreTest, Put)
{
    MemoryStore store;

    // Add a value
    ByteVector key = ByteVector::Parse("0102030405");
    ByteVector value1 = ByteVector::Parse("0607080910");
    store.Put(key, value1);

    auto value2 = store.TryGet(key);
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(*value2, value1);

    // Update the value
    ByteVector value3 = ByteVector::Parse("1112131415");
    store.Put(key, value3);

    auto value4 = store.TryGet(key);
    EXPECT_TRUE(value4.has_value());
    EXPECT_EQ(*value4, value3);
}

TEST(MemoryStoreTest, Delete)
{
    MemoryStore store;

    // Add a value
    ByteVector key = ByteVector::Parse("0102030405");
    ByteVector value = ByteVector::Parse("0607080910");
    store.Put(key, value);

    EXPECT_TRUE(store.Contains(key));

    // Delete the value
    store.Delete(key);

    EXPECT_FALSE(store.Contains(key));

    // Delete a non-existent key
    ByteVector key2 = ByteVector::Parse("1112131415");
    store.Delete(key2);

    EXPECT_FALSE(store.Contains(key2));
}

TEST(MemoryStoreTest, CreateSnapshot)
{
    MemoryStore store;

    // Add a value
    ByteVector key = ByteVector::Parse("0102030405");
    ByteVector value1 = ByteVector::Parse("0607080910");
    store.Put(key, value1);

    // Create a snapshot
    auto snapshot = store.GetSnapshot();

    // Check that the snapshot contains the value
    auto value2 = snapshot->TryGet(key);
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(*value2, value1);

    // Update the value in the snapshot
    ByteVector value3 = ByteVector::Parse("1112131415");
    snapshot->Put(key, value3);

    // Check that the snapshot contains the updated value
    auto value4 = snapshot->TryGet(key);
    EXPECT_TRUE(value4.has_value());
    EXPECT_EQ(*value4, value3);

    // Check that the store still contains the original value
    auto value5 = store.TryGet(key);
    EXPECT_TRUE(value5.has_value());
    EXPECT_EQ(*value5, value1);

    // Commit the snapshot
    snapshot->Commit();

    // Check that the store now contains the updated value
    auto value6 = store.TryGet(key);
    EXPECT_TRUE(value6.has_value());
    EXPECT_EQ(*value6, value3);
}

TEST(MemoryStoreTest, SnapshotDelete)
{
    MemoryStore store;

    // Add a value
    ByteVector key = ByteVector::Parse("0102030405");
    ByteVector value = ByteVector::Parse("0607080910");
    store.Put(key, value);

    // Create a snapshot
    auto snapshot = store.GetSnapshot();

    // Delete the value in the snapshot
    snapshot->Delete(key);

    // Check that the snapshot does not contain the value
    auto value2 = snapshot->TryGet(key);
    EXPECT_FALSE(value2.has_value());

    // Check that the store still contains the value
    auto value3 = store.TryGet(key);
    EXPECT_TRUE(value3.has_value());
    EXPECT_EQ(*value3, value);

    // Commit the snapshot
    snapshot->Commit();

    // Check that the store no longer contains the value
    auto value4 = store.TryGet(key);
    EXPECT_FALSE(value4.has_value());
}

TEST(MemoryStoreTest, SnapshotFind)
{
    MemoryStore store;

    // Add some values
    ByteVector key1 = ByteVector::Parse("0102030405");
    ByteVector value1 = ByteVector::Parse("0607080910");
    store.Put(key1, value1);

    ByteVector key2 = ByteVector::Parse("0102031415");
    ByteVector value2 = ByteVector::Parse("1617181920");
    store.Put(key2, value2);

    // Create a snapshot
    auto snapshot = store.GetSnapshot();

    // Add a value to the snapshot
    ByteVector key3 = ByteVector::Parse("0103030405");
    ByteVector value3 = ByteVector::Parse("2627282930");
    snapshot->Put(key3, value3);

    // Find all in the snapshot
    auto result1 = snapshot->Find();
    EXPECT_EQ(result1.size(), 3);

    // Find with prefix in the snapshot
    ByteVector prefix = ByteVector::Parse("0102");
    auto result2 = snapshot->Find(&prefix);
    EXPECT_EQ(result2.size(), 2);

    // Find all in the store
    auto result3 = store.Find();
    EXPECT_EQ(result3.size(), 2);

    // Commit the snapshot
    snapshot->Commit();

    // Find all in the store
    auto result4 = store.Find();
    EXPECT_EQ(result4.size(), 3);
}
