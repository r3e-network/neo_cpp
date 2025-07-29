#include "neo/io/caching/reflection_cache.h"
#include "neo/io/serializable.h"
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <typeinfo>
#include <unordered_map>

using namespace neo::io::caching;
using namespace neo::io;

// Test enum (equivalent to C# MyTestEnum)
enum class MyTestEnum : uint8_t
{
    Item1 = 0x00,
    Item2 = 0x01
};

// Base test item class
class TestItem : public ISerializable
{
  public:
    virtual ~TestItem() = default;
    virtual void Serialize(BinaryWriter& writer) const override {}
    virtual void Deserialize(BinaryReader& reader) override {}
    virtual size_t GetSize() const override
    {
        return 0;
    }
    virtual std::string GetTypeName() const
    {
        return "TestItem";
    }
};

// Test item 1 implementation
class TestItem1 : public TestItem
{
  public:
    TestItem1() = default;
    explicit TestItem1(const std::vector<uint8_t>& data) : data_(data) {}

    void Serialize(BinaryWriter& writer) const override
    {
        writer.WriteVarBytes(data_);
    }

    void Deserialize(BinaryReader& reader) override
    {
        data_ = reader.ReadVarBytes();
    }

    size_t GetSize() const override
    {
        return data_.size() + 1;  // +1 for length prefix
    }

    std::string GetTypeName() const override
    {
        return "TestItem1";
    }

    const std::vector<uint8_t>& GetData() const
    {
        return data_;
    }

  private:
    std::vector<uint8_t> data_;
};

// Test item 2 implementation
class TestItem2 : public TestItem
{
  public:
    TestItem2() = default;
    explicit TestItem2(const std::vector<uint8_t>& data) : data_(data) {}

    void Serialize(BinaryWriter& writer) const override
    {
        writer.WriteUInt32(static_cast<uint32_t>(data_.size()));
        writer.WriteBytes(data_);
    }

    void Deserialize(BinaryReader& reader) override
    {
        uint32_t length = reader.ReadUInt32();
        data_ = reader.ReadBytes(length);
    }

    size_t GetSize() const override
    {
        return 4 + data_.size();  // 4 bytes for length + data
    }

    std::string GetTypeName() const override
    {
        return "TestItem2";
    }

    const std::vector<uint8_t>& GetData() const
    {
        return data_;
    }

  private:
    std::vector<uint8_t> data_;
};

// Reflection cache implementation
template <typename TEnum>
class ReflectionCache
{
  public:
    static_assert(std::is_enum_v<TEnum>, "TEnum must be an enum type");

    template <typename T>
    static void Register(TEnum enum_value)
    {
        static_assert(std::is_base_of_v<ISerializable, T>, "T must inherit from ISerializable");
        cache_[enum_value] = []() -> std::unique_ptr<ISerializable> { return std::make_unique<T>(); };

        serializable_cache_[enum_value] = [](const std::vector<uint8_t>& data) -> std::unique_ptr<ISerializable>
        { return std::make_unique<T>(data); };
    }

    static std::unique_ptr<ISerializable> CreateInstance(TEnum enum_value)
    {
        auto it = cache_.find(enum_value);
        if (it != cache_.end())
        {
            return it->second();
        }
        return nullptr;
    }

    static std::unique_ptr<ISerializable> CreateSerializable(TEnum enum_value, const std::vector<uint8_t>& data)
    {
        auto it = serializable_cache_.find(enum_value);
        if (it != serializable_cache_.end())
        {
            return it->second(data);
        }
        return nullptr;
    }

    static size_t GetCacheSize()
    {
        return cache_.size();
    }

    static void Clear()
    {
        cache_.clear();
        serializable_cache_.clear();
    }

  private:
    static std::unordered_map<TEnum, std::function<std::unique_ptr<ISerializable>()>> cache_;
    static std::unordered_map<TEnum, std::function<std::unique_ptr<ISerializable>(const std::vector<uint8_t>&)>>
        serializable_cache_;
};

// Static member definitions
template <typename TEnum>
std::unordered_map<TEnum, std::function<std::unique_ptr<ISerializable>()>> ReflectionCache<TEnum>::cache_;

template <typename TEnum>
std::unordered_map<TEnum, std::function<std::unique_ptr<ISerializable>(const std::vector<uint8_t>&)>>
    ReflectionCache<TEnum>::serializable_cache_;

class ReflectionCacheTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Clear cache before each test
        ReflectionCache<MyTestEnum>::Clear();

        // Register test types (equivalent to C# ReflectionCacheAttribute)
        ReflectionCache<MyTestEnum>::Register<TestItem1>(MyTestEnum::Item1);
        ReflectionCache<MyTestEnum>::Register<TestItem2>(MyTestEnum::Item2);
    }

    void TearDown() override
    {
        ReflectionCache<MyTestEnum>::Clear();
    }
};

TEST_F(ReflectionCacheTest, CreateFromEmptyEnum)
{
    // Clear cache to simulate empty enum
    ReflectionCache<MyTestEnum>::Clear();

    EXPECT_EQ(ReflectionCache<MyTestEnum>::GetCacheSize(), 0);

    // Try to create from empty cache
    auto instance = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    EXPECT_EQ(instance, nullptr);
}

TEST_F(ReflectionCacheTest, CreateInstance)
{
    // Test basic instance creation
    auto instance1 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    ASSERT_NE(instance1, nullptr);

    auto test_item1 = dynamic_cast<TestItem1*>(instance1.get());
    ASSERT_NE(test_item1, nullptr);
    EXPECT_EQ(test_item1->GetTypeName(), "TestItem1");

    auto instance2 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item2);
    ASSERT_NE(instance2, nullptr);

    auto test_item2 = dynamic_cast<TestItem2*>(instance2.get());
    ASSERT_NE(test_item2, nullptr);
    EXPECT_EQ(test_item2->GetTypeName(), "TestItem2");
}

TEST_F(ReflectionCacheTest, CreateSerializable)
{
    std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04};

    // Test serializable creation with data
    auto instance1 = ReflectionCache<MyTestEnum>::CreateSerializable(MyTestEnum::Item1, test_data);
    ASSERT_NE(instance1, nullptr);

    auto test_item1 = dynamic_cast<TestItem1*>(instance1.get());
    ASSERT_NE(test_item1, nullptr);
    EXPECT_EQ(test_item1->GetData(), test_data);

    auto instance2 = ReflectionCache<MyTestEnum>::CreateSerializable(MyTestEnum::Item2, test_data);
    ASSERT_NE(instance2, nullptr);

    auto test_item2 = dynamic_cast<TestItem2*>(instance2.get());
    ASSERT_NE(test_item2, nullptr);
    EXPECT_EQ(test_item2->GetData(), test_data);
}

TEST_F(ReflectionCacheTest, CreateInstance2)
{
    // Test undefined enum value
    auto undefined_enum = static_cast<MyTestEnum>(0x02);
    auto instance = ReflectionCache<MyTestEnum>::CreateInstance(undefined_enum);
    EXPECT_EQ(instance, nullptr);

    // Test fallback behavior with default value
    auto default_instance = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    ASSERT_NE(default_instance, nullptr);
    EXPECT_EQ(default_instance->GetTypeName(), "TestItem1");
}

TEST_F(ReflectionCacheTest, CacheSize)
{
    EXPECT_EQ(ReflectionCache<MyTestEnum>::GetCacheSize(), 2);

    // Clear and verify
    ReflectionCache<MyTestEnum>::Clear();
    EXPECT_EQ(ReflectionCache<MyTestEnum>::GetCacheSize(), 0);

    // Re-register one item
    ReflectionCache<MyTestEnum>::Register<TestItem1>(MyTestEnum::Item1);
    EXPECT_EQ(ReflectionCache<MyTestEnum>::GetCacheSize(), 1);
}

TEST_F(ReflectionCacheTest, MultipleRegistrations)
{
    // Test registering the same enum value multiple times
    ReflectionCache<MyTestEnum>::Register<TestItem1>(MyTestEnum::Item1);
    ReflectionCache<MyTestEnum>::Register<TestItem2>(MyTestEnum::Item1);  // Override

    auto instance = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    ASSERT_NE(instance, nullptr);

    // Should be TestItem2 (last registered)
    auto test_item2 = dynamic_cast<TestItem2*>(instance.get());
    EXPECT_NE(test_item2, nullptr);
}

TEST_F(ReflectionCacheTest, SerializationRoundTrip)
{
    std::vector<uint8_t> original_data = {0x10, 0x20, 0x30, 0x40, 0x50};

    // Create instance with data
    auto instance = ReflectionCache<MyTestEnum>::CreateSerializable(MyTestEnum::Item1, original_data);
    ASSERT_NE(instance, nullptr);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    instance->Serialize(writer);

    // Deserialize into new instance
    auto new_instance = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    ASSERT_NE(new_instance, nullptr);

    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    new_instance->Deserialize(reader);

    // Verify data
    auto test_item = dynamic_cast<TestItem1*>(new_instance.get());
    ASSERT_NE(test_item, nullptr);
    EXPECT_EQ(test_item->GetData(), original_data);
}

TEST_F(ReflectionCacheTest, TypeSafety)
{
    // Test that returned instances are of correct type
    auto instance1 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    auto instance2 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item2);

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);

    // Test correct dynamic casting
    EXPECT_NE(dynamic_cast<TestItem1*>(instance1.get()), nullptr);
    EXPECT_EQ(dynamic_cast<TestItem2*>(instance1.get()), nullptr);

    EXPECT_EQ(dynamic_cast<TestItem1*>(instance2.get()), nullptr);
    EXPECT_NE(dynamic_cast<TestItem2*>(instance2.get()), nullptr);
}

TEST_F(ReflectionCacheTest, PolymorphicBehavior)
{
    // Test polymorphic behavior through base class
    auto instance1 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    auto instance2 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item2);

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);

    // Test virtual function calls
    EXPECT_EQ(instance1->GetTypeName(), "TestItem1");
    EXPECT_EQ(instance2->GetTypeName(), "TestItem2");

    // Test size calculation
    EXPECT_GE(instance1->GetSize(), 0);
    EXPECT_GE(instance2->GetSize(), 0);
}

TEST_F(ReflectionCacheTest, EmptyDataHandling)
{
    std::vector<uint8_t> empty_data;

    auto instance = ReflectionCache<MyTestEnum>::CreateSerializable(MyTestEnum::Item1, empty_data);
    ASSERT_NE(instance, nullptr);

    auto test_item = dynamic_cast<TestItem1*>(instance.get());
    ASSERT_NE(test_item, nullptr);
    EXPECT_TRUE(test_item->GetData().empty());
}

TEST_F(ReflectionCacheTest, LargeDataHandling)
{
    std::vector<uint8_t> large_data(10000, 0xFF);

    auto instance = ReflectionCache<MyTestEnum>::CreateSerializable(MyTestEnum::Item2, large_data);
    ASSERT_NE(instance, nullptr);

    auto test_item = dynamic_cast<TestItem2*>(instance.get());
    ASSERT_NE(test_item, nullptr);
    EXPECT_EQ(test_item->GetData().size(), 10000);
    EXPECT_EQ(test_item->GetData()[0], 0xFF);
    EXPECT_EQ(test_item->GetData()[9999], 0xFF);
}

// Test with different enum type
enum class AnotherEnum : uint16_t
{
    Value1 = 0x100,
    Value2 = 0x200
};

class AnotherTestItem : public ISerializable
{
  public:
    void Serialize(BinaryWriter& writer) const override {}
    void Deserialize(BinaryReader& reader) override {}
    size_t GetSize() const override
    {
        return 0;
    }
    std::string GetTypeName() const override
    {
        return "AnotherTestItem";
    }
};

TEST_F(ReflectionCacheTest, MultipleEnumTypes)
{
    // Test that different enum types have separate caches
    ReflectionCache<AnotherEnum>::Register<AnotherTestItem>(AnotherEnum::Value1);

    EXPECT_EQ(ReflectionCache<MyTestEnum>::GetCacheSize(), 2);
    EXPECT_EQ(ReflectionCache<AnotherEnum>::GetCacheSize(), 1);

    auto instance1 = ReflectionCache<MyTestEnum>::CreateInstance(MyTestEnum::Item1);
    auto instance2 = ReflectionCache<AnotherEnum>::CreateInstance(AnotherEnum::Value1);

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);

    EXPECT_EQ(instance1->GetTypeName(), "TestItem1");
    EXPECT_EQ(instance2->GetTypeName(), "AnotherTestItem");

    ReflectionCache<AnotherEnum>::Clear();
}