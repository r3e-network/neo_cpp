#include <gtest/gtest.h>
#include <neo/json/ordered_dictionary.h>
#include <string>

using namespace neo::json;

/**
 * @brief Test fixture for OrderedDictionary
 */
class OrderedDictionaryTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize test environment
        dict.clear();
    }

    void TearDown() override
    {
        // Clean up test environment
    }

    OrderedDictionary<std::string, int> dict;
};

TEST_F(OrderedDictionaryTest, BasicInsertion)
{
    dict["key1"] = 10;
    dict["key2"] = 20;
    dict["key3"] = 30;

    EXPECT_EQ(dict.size(), 3u);
    EXPECT_EQ(dict["key1"], 10);
    EXPECT_EQ(dict["key2"], 20);
    EXPECT_EQ(dict["key3"], 30);
}

TEST_F(OrderedDictionaryTest, ContainsKey)
{
    dict["test"] = 42;

    EXPECT_TRUE(dict.contains("test"));
    EXPECT_FALSE(dict.contains("nonexistent"));
}

TEST_F(OrderedDictionaryTest, OrderPreservation)
{
    dict["first"] = 1;
    dict["second"] = 2;
    dict["third"] = 3;

    auto it = dict.begin();
    EXPECT_EQ(it->key, "first");
    EXPECT_EQ(it->value, 1);

    ++it;
    EXPECT_EQ(it->key, "second");
    EXPECT_EQ(it->value, 2);

    ++it;
    EXPECT_EQ(it->key, "third");
    EXPECT_EQ(it->value, 3);
}

TEST_F(OrderedDictionaryTest, UpdateValue)
{
    dict["key"] = 100;
    EXPECT_EQ(dict["key"], 100);

    dict["key"] = 200;
    EXPECT_EQ(dict["key"], 200);
    EXPECT_EQ(dict.size(), 1u);
}

TEST_F(OrderedDictionaryTest, Clear)
{
    dict["a"] = 1;
    dict["b"] = 2;
    EXPECT_EQ(dict.size(), 2u);

    dict.clear();
    EXPECT_EQ(dict.size(), 0u);
    EXPECT_TRUE(dict.empty());
}

TEST_F(OrderedDictionaryTest, Remove)
{
    dict["keep"] = 1;
    dict["remove"] = 2;
    dict["keep2"] = 3;

    EXPECT_TRUE(dict.remove("remove"));
    EXPECT_FALSE(dict.remove("nonexistent"));

    EXPECT_EQ(dict.size(), 2u);
    EXPECT_TRUE(dict.contains("keep"));
    EXPECT_TRUE(dict.contains("keep2"));
    EXPECT_FALSE(dict.contains("remove"));
}

TEST_F(OrderedDictionaryTest, CopyConstructor)
{
    dict["a"] = 1;
    dict["b"] = 2;

    OrderedDictionary<std::string, int> copy(dict);

    EXPECT_EQ(copy.size(), 2u);
    EXPECT_EQ(copy["a"], 1);
    EXPECT_EQ(copy["b"], 2);

    // Test independence
    copy["c"] = 3;
    EXPECT_FALSE(dict.contains("c"));
}

TEST_F(OrderedDictionaryTest, MoveConstructor)
{
    dict["a"] = 1;
    dict["b"] = 2;

    OrderedDictionary<std::string, int> moved(std::move(dict));

    EXPECT_EQ(moved.size(), 2u);
    EXPECT_EQ(moved["a"], 1);
    EXPECT_EQ(moved["b"], 2);
}

TEST_F(OrderedDictionaryTest, Assignment)
{
    dict["original"] = 99;

    OrderedDictionary<std::string, int> other;
    other["other"] = 88;

    other = dict;

    EXPECT_EQ(other.size(), 1u);
    EXPECT_EQ(other["original"], 99);
    EXPECT_FALSE(other.contains("other"));
}