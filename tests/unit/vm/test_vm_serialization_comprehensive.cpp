#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <sstream>

using namespace neo::vm;
using namespace neo::io;

class VMSerializationComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup test data
    }
    
    void TearDown() override
    {
        // Cleanup
    }
    
    // Helper method to serialize and deserialize a stack item
    std::shared_ptr<StackItem> RoundTripSerialize(std::shared_ptr<StackItem> item)
    {
        // Serialize
        std::ostringstream oss;
        BinaryWriter writer(oss);
        StackItem::Serialize(item, writer);
        
        // Deserialize
        std::string serialized = oss.str();
        std::istringstream iss(serialized);
        BinaryReader reader(iss);
        return StackItem::Deserialize(reader);
    }
};

TEST_F(VMSerializationComprehensiveTest, SerializeBoolean)
{
    // Test true
    auto true_item = StackItem::CreateBoolean(true);
    auto deserialized_true = RoundTripSerialize(true_item);
    
    EXPECT_EQ(deserialized_true->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(deserialized_true->GetBoolean());
    
    // Test false
    auto false_item = StackItem::CreateBoolean(false);
    auto deserialized_false = RoundTripSerialize(false_item);
    
    EXPECT_EQ(deserialized_false->GetType(), StackItemType::Boolean);
    EXPECT_FALSE(deserialized_false->GetBoolean());
}

TEST_F(VMSerializationComprehensiveTest, SerializeInteger)
{
    // Test positive integer
    auto pos_item = StackItem::Create(int64_t(12345));
    auto deserialized_pos = RoundTripSerialize(pos_item);
    
    EXPECT_EQ(deserialized_pos->GetType(), StackItemType::Integer);
    EXPECT_EQ(deserialized_pos->GetInteger(), 12345);
    
    // Test negative integer
    auto neg_item = StackItem::Create(int64_t(-9876));
    auto deserialized_neg = RoundTripSerialize(neg_item);
    
    EXPECT_EQ(deserialized_neg->GetType(), StackItemType::Integer);
    EXPECT_EQ(deserialized_neg->GetInteger(), -9876);
    
    // Test zero
    auto zero_item = StackItem::Create(int64_t(0));
    auto deserialized_zero = RoundTripSerialize(zero_item);
    
    EXPECT_EQ(deserialized_zero->GetType(), StackItemType::Integer);
    EXPECT_EQ(deserialized_zero->GetInteger(), 0);
    
    // Test maximum value
    auto max_item = StackItem::Create(std::numeric_limits<int64_t>::max());
    auto deserialized_max = RoundTripSerialize(max_item);
    
    EXPECT_EQ(deserialized_max->GetType(), StackItemType::Integer);
    EXPECT_EQ(deserialized_max->GetInteger(), std::numeric_limits<int64_t>::max());
}

TEST_F(VMSerializationComprehensiveTest, SerializeByteString)
{
    // Test empty byte string
    ByteVector empty_data;
    auto empty_item = StackItem::Create(empty_data);
    auto deserialized_empty = RoundTripSerialize(empty_item);
    
    EXPECT_EQ(deserialized_empty->GetType(), StackItemType::ByteString);
    EXPECT_EQ(deserialized_empty->GetByteArray().Size(), 0);
    
    // Test non-empty byte string
    std::vector<uint8_t> test_data = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    ByteVector data(ByteSpan(test_data.data(), test_data.size()));
    auto item = StackItem::Create(data);
    auto deserialized = RoundTripSerialize(item);
    
    EXPECT_EQ(deserialized->GetType(), StackItemType::ByteString);
    auto result_array = deserialized->GetByteArray();
    EXPECT_EQ(result_array.Size(), test_data.size());
    
    for (size_t i = 0; i < test_data.size(); ++i)
    {
        EXPECT_EQ(result_array[i], test_data[i]);
    }
    
    // Test large byte string
    std::vector<uint8_t> large_data(1000, 0xAB);
    ByteVector large_bv(ByteSpan(large_data.data(), large_data.size()));
    auto large_item = StackItem::Create(large_bv);
    auto deserialized_large = RoundTripSerialize(large_item);
    
    EXPECT_EQ(deserialized_large->GetType(), StackItemType::ByteString);
    EXPECT_EQ(deserialized_large->GetByteArray().Size(), 1000);
}

TEST_F(VMSerializationComprehensiveTest, SerializeBuffer)
{
    // Test buffer serialization
    std::vector<uint8_t> buffer_data = {0x01, 0x02, 0x03, 0x04};
    ByteVector data(ByteSpan(buffer_data.data(), buffer_data.size()));
    auto buffer_item = StackItem::CreateBuffer(data);
    auto deserialized_buffer = RoundTripSerialize(buffer_item);
    
    EXPECT_EQ(deserialized_buffer->GetType(), StackItemType::Buffer);
    auto result_array = deserialized_buffer->GetByteArray();
    EXPECT_EQ(result_array.Size(), buffer_data.size());
    
    for (size_t i = 0; i < buffer_data.size(); ++i)
    {
        EXPECT_EQ(result_array[i], buffer_data[i]);
    }
}

TEST_F(VMSerializationComprehensiveTest, SerializeArray)
{
    // Test empty array
    std::vector<std::shared_ptr<StackItem>> empty_items;
    auto empty_array = StackItem::CreateArray(empty_items);
    auto deserialized_empty = RoundTripSerialize(empty_array);
    
    EXPECT_EQ(deserialized_empty->GetType(), StackItemType::Array);
    EXPECT_EQ(deserialized_empty->GetArray().size(), 0);
    
    // Test array with various items
    std::vector<std::shared_ptr<StackItem>> items;
    items.push_back(StackItem::CreateBoolean(true));
    items.push_back(StackItem::Create(int64_t(42)));
    
    std::vector<uint8_t> test_data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    ByteVector data(ByteSpan(test_data.data(), test_data.size()));
    items.push_back(StackItem::Create(data));
    
    auto array_item = StackItem::CreateArray(items);
    auto deserialized_array = RoundTripSerialize(array_item);
    
    EXPECT_EQ(deserialized_array->GetType(), StackItemType::Array);
    auto result_array = deserialized_array->GetArray();
    EXPECT_EQ(result_array.size(), 3);
    
    EXPECT_EQ(result_array[0]->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(result_array[0]->GetBoolean());
    
    EXPECT_EQ(result_array[1]->GetType(), StackItemType::Integer);
    EXPECT_EQ(result_array[1]->GetInteger(), 42);
    
    EXPECT_EQ(result_array[2]->GetType(), StackItemType::ByteString);
    EXPECT_EQ(result_array[2]->GetByteArray().Size(), test_data.size());
}

TEST_F(VMSerializationComprehensiveTest, SerializeNestedArray)
{
    // Test nested arrays
    std::vector<std::shared_ptr<StackItem>> inner_items;
    inner_items.push_back(StackItem::Create(int64_t(1)));
    inner_items.push_back(StackItem::Create(int64_t(2)));
    auto inner_array = StackItem::CreateArray(inner_items);
    
    std::vector<std::shared_ptr<StackItem>> outer_items;
    outer_items.push_back(inner_array);
    outer_items.push_back(StackItem::Create(int64_t(3)));
    auto outer_array = StackItem::CreateArray(outer_items);
    
    auto deserialized = RoundTripSerialize(outer_array);
    
    EXPECT_EQ(deserialized->GetType(), StackItemType::Array);
    auto result_array = deserialized->GetArray();
    EXPECT_EQ(result_array.size(), 2);
    
    // Check inner array
    EXPECT_EQ(result_array[0]->GetType(), StackItemType::Array);
    auto inner_result = result_array[0]->GetArray();
    EXPECT_EQ(inner_result.size(), 2);
    EXPECT_EQ(inner_result[0]->GetInteger(), 1);
    EXPECT_EQ(inner_result[1]->GetInteger(), 2);
    
    // Check outer element
    EXPECT_EQ(result_array[1]->GetType(), StackItemType::Integer);
    EXPECT_EQ(result_array[1]->GetInteger(), 3);
}

TEST_F(VMSerializationComprehensiveTest, SerializeNull)
{
    // Test null serialization
    auto null_item = StackItem::Null();
    auto deserialized_null = RoundTripSerialize(null_item);
    
    EXPECT_EQ(deserialized_null->GetType(), StackItemType::Any);
    EXPECT_TRUE(deserialized_null->IsNull());
}

TEST_F(VMSerializationComprehensiveTest, SerializationEdgeCases)
{
    // Test maximum size constraints
    // This ensures the serializer handles edge cases properly
    
    // Test very large integer (boundary case)
    auto large_int = StackItem::Create(int64_t(9223372036854775807LL)); // max int64
    auto deserialized_large_int = RoundTripSerialize(large_int);
    EXPECT_EQ(deserialized_large_int->GetInteger(), 9223372036854775807LL);
    
    // Test minimum integer (boundary case)
    auto min_int = StackItem::Create(int64_t(-9223372036854775807LL - 1)); // min int64
    auto deserialized_min_int = RoundTripSerialize(min_int);
    EXPECT_EQ(deserialized_min_int->GetInteger(), -9223372036854775807LL - 1);
}

TEST_F(VMSerializationComprehensiveTest, SerializationErrors)
{
    // Test that InteropInterface items cannot be serialized
    auto interop_item = StackItem::CreateInteropInterface(nullptr);
    
    std::ostringstream oss;
    BinaryWriter writer(oss);
    
    EXPECT_THROW(StackItem::Serialize(interop_item, writer), std::runtime_error);
}

TEST_F(VMSerializationComprehensiveTest, DeserializationErrors)
{
    // Test that InteropInterface type cannot be deserialized
    std::ostringstream oss;
    BinaryWriter writer(oss);
    writer.WriteByte(static_cast<uint8_t>(StackItemType::InteropInterface));
    
    std::string serialized = oss.str();
    std::istringstream iss(serialized);
    BinaryReader reader(iss);
    
    EXPECT_THROW(StackItem::Deserialize(reader), std::runtime_error);
    
    // Test unknown type
    std::ostringstream oss2;
    BinaryWriter writer2(oss2);
    writer2.WriteByte(0xFF); // Invalid type
    
    std::string serialized2 = oss2.str();
    std::istringstream iss2(serialized2);
    BinaryReader reader2(iss2);
    
    EXPECT_THROW(StackItem::Deserialize(reader2), std::runtime_error);
}

TEST_F(VMSerializationComprehensiveTest, SerializationPerformance)
{
    // Performance test with large arrays
    const size_t large_count = 1000;
    std::vector<std::shared_ptr<StackItem>> large_items;
    large_items.reserve(large_count);
    
    for (size_t i = 0; i < large_count; ++i)
    {
        large_items.push_back(StackItem::Create(static_cast<int64_t>(i)));
    }
    
    auto large_array = StackItem::CreateArray(large_items);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto deserialized_large = RoundTripSerialize(large_array);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(deserialized_large->GetType(), StackItemType::Array);
    EXPECT_EQ(deserialized_large->GetArray().size(), large_count);
    
    // Should complete in reasonable time (less than 1 second)
    EXPECT_LT(duration.count(), 1000);
    
    // Verify correctness of large array
    auto result_items = deserialized_large->GetArray();
    for (size_t i = 0; i < 10; ++i) // Check first 10 items
    {
        EXPECT_EQ(result_items[i]->GetType(), StackItemType::Integer);
        EXPECT_EQ(result_items[i]->GetInteger(), static_cast<int64_t>(i));
    }
}

TEST_F(VMSerializationComprehensiveTest, SerializationSizeConstraints)
{
    // Test serialization size limits and constraints
    
    // Test reasonable size byte string
    std::vector<uint8_t> reasonable_data(1024, 0x42); // 1KB
    ByteVector reasonable_bv(ByteSpan(reasonable_data.data(), reasonable_data.size()));
    auto reasonable_item = StackItem::Create(reasonable_bv);
    auto deserialized_reasonable = RoundTripSerialize(reasonable_item);
    
    EXPECT_EQ(deserialized_reasonable->GetType(), StackItemType::ByteString);
    EXPECT_EQ(deserialized_reasonable->GetByteArray().Size(), 1024);
    
    // Verify all bytes are correct
    auto result_data = deserialized_reasonable->GetByteArray();
    for (size_t i = 0; i < 1024; ++i)
    {
        EXPECT_EQ(result_data[i], 0x42);
    }
}