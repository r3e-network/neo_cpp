#include <gtest/gtest.h>
#include <neo/json/jarray.h>
#include <neo/json/jstring.h>
#include <neo/json/jnumber.h>
#include <neo/json/jboolean.h>

namespace neo::json::tests
{
    class JArrayTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            array = std::make_shared<JArray>();
            array->Add(std::make_shared<JString>("hello"));
            array->Add(std::make_shared<JNumber>(42));
            array->Add(std::make_shared<JBoolean>(true));
            array->Add(nullptr); // null value
        }

        std::shared_ptr<JArray> array;
    };

    TEST_F(JArrayTest, TestGetType)
    {
        EXPECT_EQ(JTokenType::Array, array->GetType());
    }

    TEST_F(JArrayTest, TestIndexAccess)
    {
        auto item0 = (*array)[0];
        EXPECT_NE(item0, nullptr);
        EXPECT_EQ("hello", item0->AsString());
        
        auto item1 = (*array)[1];
        EXPECT_NE(item1, nullptr);
        EXPECT_EQ(42, item1->AsNumber());
        
        auto item2 = (*array)[2];
        EXPECT_NE(item2, nullptr);
        EXPECT_TRUE(item2->AsBoolean());
        
        auto item3 = (*array)[3];
        EXPECT_EQ(item3, nullptr);
        
        // Test out of range
        EXPECT_THROW((*array)[10], std::out_of_range);
        EXPECT_THROW((*array)[-1], std::out_of_range);
    }

    TEST_F(JArrayTest, TestCount)
    {
        EXPECT_EQ(4, array->Count());
        EXPECT_FALSE(array->IsEmpty());
        
        auto empty_array = std::make_shared<JArray>();
        EXPECT_EQ(0, empty_array->Count());
        EXPECT_TRUE(empty_array->IsEmpty());
    }

    TEST_F(JArrayTest, TestAdd)
    {
        size_t initial_count = array->Count();
        array->Add(std::make_shared<JString>("new item"));
        EXPECT_EQ(initial_count + 1, array->Count());
        
        auto last_item = (*array)[array->Count() - 1];
        EXPECT_EQ("new item", last_item->AsString());
    }

    TEST_F(JArrayTest, TestRemoveAt)
    {
        size_t initial_count = array->Count();
        array->RemoveAt(1); // Remove the number 42
        EXPECT_EQ(initial_count - 1, array->Count());
        
        // Check that the remaining items shifted
        auto item1 = (*array)[1];
        EXPECT_TRUE(item1->AsBoolean()); // This was originally at index 2
        
        // Test out of range
        EXPECT_THROW(array->RemoveAt(10), std::out_of_range);
    }

    TEST_F(JArrayTest, TestClear)
    {
        EXPECT_GT(array->Count(), 0);
        array->Clear();
        EXPECT_EQ(0, array->Count());
        EXPECT_TRUE(array->IsEmpty());
    }

    TEST_F(JArrayTest, TestClone)
    {
        auto cloned = std::dynamic_pointer_cast<JArray>(array->Clone());
        EXPECT_NE(array.get(), cloned.get());
        EXPECT_EQ(array->Count(), cloned->Count());
        
        // Check that all items are cloned correctly
        for (size_t i = 0; i < array->Count(); ++i)
        {
            auto original_item = (*array)[i];
            auto cloned_item = (*cloned)[i];
            
            if (original_item == nullptr)
            {
                EXPECT_EQ(cloned_item, nullptr);
            }
            else
            {
                EXPECT_NE(original_item.get(), cloned_item.get()); // Different objects
                EXPECT_TRUE(original_item->Equals(*cloned_item)); // Same content
            }
        }
    }

    TEST_F(JArrayTest, TestEquals)
    {
        auto cloned = std::dynamic_pointer_cast<JArray>(array->Clone());
        EXPECT_TRUE(array->Equals(*cloned));
        
        auto different_array = std::make_shared<JArray>();
        different_array->Add(std::make_shared<JString>("different"));
        EXPECT_FALSE(array->Equals(*different_array));
    }

    TEST_F(JArrayTest, TestToString)
    {
        std::string json_str = array->ToString();
        EXPECT_FALSE(json_str.empty());
        EXPECT_EQ('[', json_str[0]);
        EXPECT_EQ(']', json_str[json_str.length() - 1]);
        EXPECT_NE(json_str.find("hello"), std::string::npos);
        EXPECT_NE(json_str.find("42"), std::string::npos);
        EXPECT_NE(json_str.find("true"), std::string::npos);
        EXPECT_NE(json_str.find("null"), std::string::npos);
    }

    TEST_F(JArrayTest, TestIterators)
    {
        size_t count = 0;
        for (const auto& item : *array)
        {
            count++;
        }
        EXPECT_EQ(array->Count(), count);
        
        // Test const iterators
        count = 0;
        for (auto it = array->cbegin(); it != array->cend(); ++it)
        {
            count++;
        }
        EXPECT_EQ(array->Count(), count);
    }

    TEST_F(JArrayTest, TestInitializerList)
    {
        auto init_array = std::make_shared<JArray>(std::initializer_list<std::shared_ptr<JToken>>{
            std::make_shared<JString>("test"),
            std::make_shared<JNumber>(123),
            std::make_shared<JBoolean>(false)
        });
        
        EXPECT_EQ(3, init_array->Count());
        EXPECT_EQ("test", (*init_array)[0]->AsString());
        EXPECT_EQ(123, (*init_array)[1]->AsNumber());
        EXPECT_FALSE((*init_array)[2]->AsBoolean());
    }
}
