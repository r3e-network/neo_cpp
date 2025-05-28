#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;
using namespace neo::io;

TEST(StackItemTest, BooleanItem)
{
    // Constructor
    BooleanItem item1(true);
    BooleanItem item2(false);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Boolean);
    EXPECT_EQ(item2.GetType(), StackItemType::Boolean);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_FALSE(item2.GetBoolean());
    
    // GetInteger
    EXPECT_EQ(item1.GetInteger(), 1);
    EXPECT_EQ(item2.GetInteger(), 0);
    
    // GetByteArray
    EXPECT_EQ(item1.GetByteArray().Size(), sizeof(bool));
    EXPECT_EQ(item2.GetByteArray().Size(), sizeof(bool));
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item2.Equals(item1));
    
    // Equals with other types
    IntegerItem intItem1(1);
    IntegerItem intItem2(0);
    EXPECT_TRUE(item1.Equals(intItem1));
    EXPECT_TRUE(item2.Equals(intItem2));
    
    ByteVector bytes1 = ByteVector::Parse("01");
    ByteVector bytes2 = ByteVector::Parse("00");
    ByteStringItem byteItem1(bytes1);
    ByteStringItem byteItem2(bytes2);
    EXPECT_TRUE(item1.Equals(byteItem1));
    EXPECT_TRUE(item2.Equals(byteItem2));
}

TEST(StackItemTest, IntegerItem)
{
    // Constructor
    IntegerItem item1(123);
    IntegerItem item2(-456);
    IntegerItem item3(0);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Integer);
    EXPECT_EQ(item2.GetType(), StackItemType::Integer);
    EXPECT_EQ(item3.GetType(), StackItemType::Integer);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_FALSE(item3.GetBoolean());
    
    // GetInteger
    EXPECT_EQ(item1.GetInteger(), 123);
    EXPECT_EQ(item2.GetInteger(), -456);
    EXPECT_EQ(item3.GetInteger(), 0);
    
    // GetByteArray
    EXPECT_EQ(item1.GetByteArray()[0], 123);
    EXPECT_EQ(item3.GetByteArray()[0], 0);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
    
    // Equals with other types
    BooleanItem boolItem1(true);
    BooleanItem boolItem2(false);
    EXPECT_TRUE(item1.Equals(boolItem1));
    EXPECT_TRUE(item3.Equals(boolItem2));
    
    ByteVector bytes1 = ByteVector::Parse("7B"); // 123 in hex
    ByteVector bytes3 = ByteVector::Parse("00");
    ByteStringItem byteItem1(bytes1);
    ByteStringItem byteItem3(bytes3);
    EXPECT_TRUE(item1.Equals(byteItem1));
    EXPECT_TRUE(item3.Equals(byteItem3));
}

TEST(StackItemTest, ByteStringItem)
{
    // Constructor
    ByteVector bytes1 = ByteVector::Parse("0102030405");
    ByteVector bytes2 = ByteVector::Parse("0607080910");
    ByteVector bytes3 = ByteVector::Parse("");
    ByteStringItem item1(bytes1);
    ByteStringItem item2(bytes2);
    ByteStringItem item3(bytes3);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::ByteString);
    EXPECT_EQ(item2.GetType(), StackItemType::ByteString);
    EXPECT_EQ(item3.GetType(), StackItemType::ByteString);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_FALSE(item3.GetBoolean());
    
    // GetByteArray
    EXPECT_EQ(item1.GetByteArray(), bytes1);
    EXPECT_EQ(item2.GetByteArray(), bytes2);
    EXPECT_EQ(item3.GetByteArray(), bytes3);
    
    // GetString
    EXPECT_EQ(item1.GetString(), std::string(reinterpret_cast<const char*>(bytes1.Data()), bytes1.Size()));
    EXPECT_EQ(item2.GetString(), std::string(reinterpret_cast<const char*>(bytes2.Data()), bytes2.Size()));
    EXPECT_EQ(item3.GetString(), std::string(reinterpret_cast<const char*>(bytes3.Data()), bytes3.Size()));
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
    
    // Equals with other types
    ByteVector bytes4 = ByteVector::Parse("01");
    ByteVector bytes5 = ByteVector::Parse("00");
    ByteStringItem byteItem4(bytes4);
    ByteStringItem byteItem5(bytes5);
    BooleanItem boolItem1(true);
    BooleanItem boolItem2(false);
    EXPECT_TRUE(byteItem4.Equals(boolItem1));
    EXPECT_TRUE(byteItem5.Equals(boolItem2));
    
    ByteVector bytes6 = ByteVector::Parse("01");
    ByteStringItem byteItem6(bytes6);
    IntegerItem intItem1(1);
    EXPECT_TRUE(byteItem6.Equals(intItem1));
}

TEST(StackItemTest, BufferItem)
{
    // Constructor
    ByteVector bytes1 = ByteVector::Parse("0102030405");
    ByteVector bytes2 = ByteVector::Parse("0607080910");
    ByteVector bytes3 = ByteVector::Parse("");
    BufferItem item1(bytes1);
    BufferItem item2(bytes2);
    BufferItem item3(bytes3);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Buffer);
    EXPECT_EQ(item2.GetType(), StackItemType::Buffer);
    EXPECT_EQ(item3.GetType(), StackItemType::Buffer);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_FALSE(item3.GetBoolean());
    
    // GetByteArray
    EXPECT_EQ(item1.GetByteArray(), bytes1);
    EXPECT_EQ(item2.GetByteArray(), bytes2);
    EXPECT_EQ(item3.GetByteArray(), bytes3);
    
    // GetString
    EXPECT_EQ(item1.GetString(), std::string(reinterpret_cast<const char*>(bytes1.Data()), bytes1.Size()));
    EXPECT_EQ(item2.GetString(), std::string(reinterpret_cast<const char*>(bytes2.Data()), bytes2.Size()));
    EXPECT_EQ(item3.GetString(), std::string(reinterpret_cast<const char*>(bytes3.Data()), bytes3.Size()));
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
    
    // Equals with ByteStringItem
    ByteStringItem byteItem1(bytes1);
    ByteStringItem byteItem2(bytes2);
    ByteStringItem byteItem3(bytes3);
    EXPECT_TRUE(item1.Equals(byteItem1));
    EXPECT_TRUE(item2.Equals(byteItem2));
    EXPECT_TRUE(item3.Equals(byteItem3));
    EXPECT_TRUE(byteItem1.Equals(item1));
    EXPECT_TRUE(byteItem2.Equals(item2));
    EXPECT_TRUE(byteItem3.Equals(item3));
}

TEST(StackItemTest, ArrayItem)
{
    // Constructor
    std::vector<std::shared_ptr<StackItem>> items1 = {
        StackItem::Create(1),
        StackItem::Create(2),
        StackItem::Create(3)
    };
    std::vector<std::shared_ptr<StackItem>> items2 = {
        StackItem::Create(4),
        StackItem::Create(5),
        StackItem::Create(6)
    };
    std::vector<std::shared_ptr<StackItem>> items3 = {};
    ArrayItem item1(items1);
    ArrayItem item2(items2);
    ArrayItem item3(items3);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Array);
    EXPECT_EQ(item2.GetType(), StackItemType::Array);
    EXPECT_EQ(item3.GetType(), StackItemType::Array);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_TRUE(item3.GetBoolean());
    
    // GetArray
    EXPECT_EQ(item1.GetArray().size(), 3);
    EXPECT_EQ(item2.GetArray().size(), 3);
    EXPECT_EQ(item3.GetArray().size(), 0);
    
    // Get
    EXPECT_EQ(item1.Get(0)->GetInteger(), 1);
    EXPECT_EQ(item1.Get(1)->GetInteger(), 2);
    EXPECT_EQ(item1.Get(2)->GetInteger(), 3);
    EXPECT_EQ(item2.Get(0)->GetInteger(), 4);
    EXPECT_EQ(item2.Get(1)->GetInteger(), 5);
    EXPECT_EQ(item2.Get(2)->GetInteger(), 6);
    EXPECT_THROW(item3.Get(0), std::out_of_range);
    
    // Set
    item1.Set(0, StackItem::Create(10));
    EXPECT_EQ(item1.Get(0)->GetInteger(), 10);
    
    // Add
    item3.Add(StackItem::Create(7));
    EXPECT_EQ(item3.Size(), 1);
    EXPECT_EQ(item3.Get(0)->GetInteger(), 7);
    
    // Remove
    item1.Remove(0);
    EXPECT_EQ(item1.Size(), 2);
    EXPECT_EQ(item1.Get(0)->GetInteger(), 2);
    
    // Clear
    item1.Clear();
    EXPECT_EQ(item1.Size(), 0);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
}

TEST(StackItemTest, StructItem)
{
    // Constructor
    std::vector<std::shared_ptr<StackItem>> items1 = {
        StackItem::Create(1),
        StackItem::Create(2),
        StackItem::Create(3)
    };
    std::vector<std::shared_ptr<StackItem>> items2 = {
        StackItem::Create(1),
        StackItem::Create(2),
        StackItem::Create(3)
    };
    StructItem item1(items1);
    StructItem item2(items2);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Struct);
    EXPECT_EQ(item2.GetType(), StackItemType::Struct);
    
    // Clone
    auto item3 = item1.Clone();
    EXPECT_EQ(item3->GetType(), StackItemType::Struct);
    EXPECT_EQ(item3->Size(), 3);
    EXPECT_EQ(item3->Get(0)->GetInteger(), 1);
    EXPECT_EQ(item3->Get(1)->GetInteger(), 2);
    EXPECT_EQ(item3->Get(2)->GetInteger(), 3);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item1.Equals(item2));
    EXPECT_TRUE(item2.Equals(item1));
    EXPECT_TRUE(item1.Equals(*item3));
    EXPECT_TRUE(item3->Equals(item1));
}

TEST(StackItemTest, MapItem)
{
    // Constructor
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> map1 = {
        { StackItem::Create(1), StackItem::Create("one") },
        { StackItem::Create(2), StackItem::Create("two") },
        { StackItem::Create(3), StackItem::Create("three") }
    };
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> map2 = {
        { StackItem::Create(4), StackItem::Create("four") },
        { StackItem::Create(5), StackItem::Create("five") },
        { StackItem::Create(6), StackItem::Create("six") }
    };
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> map3 = {};
    MapItem item1(map1);
    MapItem item2(map2);
    MapItem item3(map3);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Map);
    EXPECT_EQ(item2.GetType(), StackItemType::Map);
    EXPECT_EQ(item3.GetType(), StackItemType::Map);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_TRUE(item3.GetBoolean());
    
    // GetMap
    EXPECT_EQ(item1.GetMap().size(), 3);
    EXPECT_EQ(item2.GetMap().size(), 3);
    EXPECT_EQ(item3.GetMap().size(), 0);
    
    // Get
    auto value1 = item1.Get(StackItem::Create(1));
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ((*value1)->GetString(), "one");
    auto value2 = item1.Get(StackItem::Create(2));
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ((*value2)->GetString(), "two");
    auto value3 = item1.Get(StackItem::Create(3));
    EXPECT_TRUE(value3.has_value());
    EXPECT_EQ((*value3)->GetString(), "three");
    auto value4 = item1.Get(StackItem::Create(4));
    EXPECT_FALSE(value4.has_value());
    
    // Set
    item1.Set(StackItem::Create(1), StackItem::Create("ONE"));
    value1 = item1.Get(StackItem::Create(1));
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ((*value1)->GetString(), "ONE");
    
    // Remove
    item1.Remove(StackItem::Create(1));
    value1 = item1.Get(StackItem::Create(1));
    EXPECT_FALSE(value1.has_value());
    
    // Clear
    item1.Clear();
    EXPECT_EQ(item1.Size(), 0);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
}

TEST(StackItemTest, InteropInterfaceItem)
{
    // Constructor
    int data1 = 123;
    int data2 = 456;
    InteropInterfaceItem item1(&data1);
    InteropInterfaceItem item2(&data2);
    InteropInterfaceItem item3(nullptr);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::InteropInterface);
    EXPECT_EQ(item2.GetType(), StackItemType::InteropInterface);
    EXPECT_EQ(item3.GetType(), StackItemType::InteropInterface);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_FALSE(item3.GetBoolean());
    
    // GetInterface
    EXPECT_EQ(item1.GetInterface(), &data1);
    EXPECT_EQ(item2.GetInterface(), &data2);
    EXPECT_EQ(item3.GetInterface(), nullptr);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
}

TEST(StackItemTest, PointerItem)
{
    // Constructor
    PointerItem item1(123);
    PointerItem item2(456);
    PointerItem item3(0);
    
    // GetType
    EXPECT_EQ(item1.GetType(), StackItemType::Pointer);
    EXPECT_EQ(item2.GetType(), StackItemType::Pointer);
    EXPECT_EQ(item3.GetType(), StackItemType::Pointer);
    
    // GetBoolean
    EXPECT_TRUE(item1.GetBoolean());
    EXPECT_TRUE(item2.GetBoolean());
    EXPECT_TRUE(item3.GetBoolean());
    
    // GetPosition
    EXPECT_EQ(item1.GetPosition(), 123);
    EXPECT_EQ(item2.GetPosition(), 456);
    EXPECT_EQ(item3.GetPosition(), 0);
    
    // Equals
    EXPECT_TRUE(item1.Equals(item1));
    EXPECT_TRUE(item2.Equals(item2));
    EXPECT_TRUE(item3.Equals(item3));
    EXPECT_FALSE(item1.Equals(item2));
    EXPECT_FALSE(item1.Equals(item3));
    EXPECT_FALSE(item2.Equals(item3));
}

TEST(StackItemTest, Create)
{
    // Create(bool)
    auto item1 = StackItem::Create(true);
    auto item2 = StackItem::Create(false);
    EXPECT_EQ(item1->GetType(), StackItemType::Boolean);
    EXPECT_EQ(item2->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(item1->GetBoolean());
    EXPECT_FALSE(item2->GetBoolean());
    
    // Create(int64_t)
    auto item3 = StackItem::Create(123);
    auto item4 = StackItem::Create(-456);
    EXPECT_EQ(item3->GetType(), StackItemType::Integer);
    EXPECT_EQ(item4->GetType(), StackItemType::Integer);
    EXPECT_EQ(item3->GetInteger(), 123);
    EXPECT_EQ(item4->GetInteger(), -456);
    
    // Create(ByteVector)
    ByteVector bytes = ByteVector::Parse("0102030405");
    auto item5 = StackItem::Create(bytes);
    EXPECT_EQ(item5->GetType(), StackItemType::ByteString);
    EXPECT_EQ(item5->GetByteArray(), bytes);
    
    // Create(ByteSpan)
    auto item6 = StackItem::Create(bytes.AsSpan());
    EXPECT_EQ(item6->GetType(), StackItemType::ByteString);
    EXPECT_EQ(item6->GetByteArray(), bytes);
    
    // Create(std::string)
    std::string str = "Hello, world!";
    auto item7 = StackItem::Create(str);
    EXPECT_EQ(item7->GetType(), StackItemType::ByteString);
    EXPECT_EQ(item7->GetString(), str);
    
    // Create(UInt160)
    UInt160 uint160 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto item8 = StackItem::Create(uint160);
    EXPECT_EQ(item8->GetType(), StackItemType::ByteString);
    EXPECT_EQ(item8->GetByteArray(), ByteVector(ByteSpan(uint160.Data(), uint160.Size())));
    
    // Create(UInt256)
    UInt256 uint256 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto item9 = StackItem::Create(uint256);
    EXPECT_EQ(item9->GetType(), StackItemType::ByteString);
    EXPECT_EQ(item9->GetByteArray(), ByteVector(ByteSpan(uint256.Data(), uint256.Size())));
}
