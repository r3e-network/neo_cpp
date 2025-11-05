#include <gtest/gtest.h>
#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
#include <neo/json/jnumber.h>
#include <neo/json/jobject.h>
#include <neo/json/jstring.h>

#include <nlohmann/json.hpp>

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
        array->Add(nullptr);  // null value
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

    auto last_item = (*array)[static_cast<int>(array->Count()) - 1];
    EXPECT_EQ("new item", last_item->AsString());
}

TEST_F(JArrayTest, TestRemoveAt)
{
    size_t initial_count = array->Count();
    array->RemoveAt(1);  // Remove the number 42
    EXPECT_EQ(initial_count - 1, array->Count());

    // Check that the remaining items shifted
    auto item1 = (*array)[1];
    EXPECT_TRUE(item1->AsBoolean());  // This was originally at index 2

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
    for (int i = 0; i < static_cast<int>(array->Count()); ++i)
    {
        auto original_item = (*array)[i];
        auto cloned_item = (*cloned)[i];

        if (original_item == nullptr)
        {
            EXPECT_EQ(cloned_item, nullptr);
        }
        else
        {
            EXPECT_NE(original_item.get(), cloned_item.get());  // Different objects
            EXPECT_TRUE(original_item->Equals(*cloned_item));   // Same content
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
    for ([[maybe_unused]] const auto& item : *array)
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
        std::make_shared<JString>("test"), std::make_shared<JNumber>(123), std::make_shared<JBoolean>(false)});

    EXPECT_EQ(3, init_array->Count());
    EXPECT_EQ("test", (*init_array)[0]->AsString());
    EXPECT_EQ(123, (*init_array)[1]->AsNumber());
    EXPECT_FALSE((*init_array)[2]->AsBoolean());
}

class JArrayComplexTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        alice = CreatePerson("alice", 30, 100.001, "female", true, "Tom", "cat");
        bob = CreatePerson("bob", 100000, 0.001, "male", false, "Paul", "dog");
    }

    static std::shared_ptr<JObject> CreatePerson(const std::string& name, int age, double score,
                                                 const std::string& gender, bool isMarried,
                                                 const std::string& petName, const std::string& petType)
    {
        auto person = std::make_shared<JObject>();
        person->SetProperty("name", std::make_shared<JString>(name));
        person->SetProperty("age", std::make_shared<JNumber>(static_cast<double>(age)));
        person->SetProperty("score", std::make_shared<JNumber>(score));
        person->SetProperty("gender", std::make_shared<JString>(gender));
        person->SetProperty("isMarried", std::make_shared<JBoolean>(isMarried));

        auto pet = std::make_shared<JObject>();
        pet->SetProperty("name", std::make_shared<JString>(petName));
        pet->SetProperty("type", std::make_shared<JString>(petType));
        person->SetProperty("pet", pet);
        return person;
    }

    static bool TokensEqual(const std::shared_ptr<JToken>& lhs, const std::shared_ptr<JToken>& rhs)
    {
        if (lhs == rhs) return true;
        if (!lhs || !rhs) return false;
        return lhs->Equals(*rhs);
    }

    std::shared_ptr<JObject> alice;
    std::shared_ptr<JObject> bob;
};

TEST_F(JArrayComplexTest, SetItemReplacesValue)
{
    JArray array;
    array.Add(alice);
    array.SetItem(0, bob);
    EXPECT_TRUE(TokensEqual(array[0], bob));
    EXPECT_THROW(array.SetItem(1, alice), std::out_of_range);
}

TEST_F(JArrayComplexTest, SetItemSupportsNull)
{
    JArray array;
    array.Add(alice);
    array.SetItem(0, nullptr);
    EXPECT_EQ(nullptr, array[0]);
}

TEST_F(JArrayComplexTest, InsertMaintainsOrder)
{
    JArray array;
    array.Add(alice);
    array.Add(alice);
    array.Add(alice);
    array.Add(alice);

    array.Insert(1, bob);
    EXPECT_EQ(5u, array.Count());
    EXPECT_TRUE(TokensEqual(array[1], bob));
    EXPECT_TRUE(TokensEqual(array[2], alice));

    array.Insert(static_cast<int>(array.Count()), bob);
    EXPECT_EQ(6u, array.Count());
    EXPECT_TRUE(TokensEqual(array[5], bob));
    EXPECT_THROW(array.Insert(-1, alice), std::out_of_range);
    EXPECT_THROW(array.Insert(8, alice), std::out_of_range);
}

TEST_F(JArrayComplexTest, InsertSupportsNull)
{
    JArray array;
    array.Add(alice);
    array.Insert(0, nullptr);
    EXPECT_EQ(nullptr, array[0]);
    EXPECT_TRUE(TokensEqual(array[1], alice));
}

TEST_F(JArrayComplexTest, IndexOfAndContains)
{
    JArray array;
    EXPECT_EQ(-1, array.IndexOf(alice));
    EXPECT_FALSE(array.Contains(alice));

    array.Add(alice);
    array.Add(alice);
    array.Add(alice);
    array.Add(alice);
    EXPECT_EQ(0, array.IndexOf(alice));
    EXPECT_TRUE(array.Contains(alice));

    array.Insert(1, bob);
    EXPECT_EQ(1, array.IndexOf(bob));
    EXPECT_TRUE(array.Contains(bob));
}

TEST_F(JArrayComplexTest, RemoveRemovesFirstMatch)
{
    JArray array;
    array.Add(alice);
    array.Add(alice);
    array.Add(bob);
    array.Add(alice);

    EXPECT_TRUE(array.Remove(alice));
    EXPECT_EQ(3u, array.Count());
    EXPECT_TRUE(array.Remove(bob));
    EXPECT_EQ(-1, array.IndexOf(bob));
    EXPECT_FALSE(array.Remove(std::shared_ptr<JToken>()));
}

TEST_F(JArrayComplexTest, CopyToCopiesWithOffset)
{
    JArray array;
    array.Add(alice);
    array.Add(bob);

    std::vector<std::shared_ptr<JToken>> destination(4);
    array.CopyTo(destination, 1);
    EXPECT_EQ(nullptr, destination[0]);
    EXPECT_TRUE(TokensEqual(destination[1], alice));
    EXPECT_TRUE(TokensEqual(destination[2], bob));
    EXPECT_EQ(nullptr, destination[3]);

    EXPECT_THROW(array.CopyTo(destination, 3), std::out_of_range);
}

TEST(JArrayStandaloneTest, IsReadOnlyReturnsFalse)
{
    JArray array;
    EXPECT_FALSE(array.IsReadOnly());
}

TEST_F(JArrayComplexTest, EnumeratorReturnsItemsInOrder)
{
    JArray array;
    array.Add(alice);
    array.Add(bob);
    array.Add(alice);
    array.Add(bob);

    int index = 0;
    for (const auto& item : array)
    {
        if (index % 2 == 0)
        {
            EXPECT_TRUE(TokensEqual(item, alice));
        }
        else
        {
            EXPECT_TRUE(TokensEqual(item, bob));
        }
        ++index;
    }
    EXPECT_EQ(4, index);
}

TEST(JArrayStandaloneTest, EmptyEnumerationDoesNotIterate)
{
    JArray array;
    int count = 0;
    for (const auto& item : array)
    {
        (void)item;
        ++count;
    }
    EXPECT_EQ(0, count);
}

TEST_F(JArrayComplexTest, ImplicitConstructionFromTokenVector)
{
    JArray::Items items{alice, bob};
    JArray array(items);

    EXPECT_EQ(2u, array.Count());
    EXPECT_TRUE(TokensEqual(array[0], alice));
    EXPECT_TRUE(TokensEqual(array[1], bob));
}

TEST(JArrayStandaloneTest, AddNullValuesMaintained)
{
    JArray array;
    array.Add(nullptr);
    EXPECT_EQ(1u, array.Count());
    EXPECT_EQ(nullptr, array[0]);
}

TEST_F(JArrayComplexTest, RemoveHandlesNullEntries)
{
    JArray array;
    array.Add(nullptr);
    array.Add(alice);
    EXPECT_EQ(2u, array.Count());

    EXPECT_TRUE(array.Remove(nullptr));
    EXPECT_EQ(1u, array.Count());
    EXPECT_TRUE(TokensEqual(array[0], alice));
}

TEST_F(JArrayComplexTest, ContainsAndIndexOfNullValues)
{
    JArray array;
    array.Add(nullptr);
    array.Add(bob);

    EXPECT_TRUE(array.Contains(nullptr));
    EXPECT_EQ(0, array.IndexOf(nullptr));
    EXPECT_EQ(1, array.IndexOf(bob));
}

TEST_F(JArrayComplexTest, CopyToPreservesNullEntries)
{
    JArray array;
    array.Add(nullptr);
    array.Add(alice);

    std::vector<std::shared_ptr<JToken>> destination(3);
    array.CopyTo(destination, 1);

    EXPECT_EQ(nullptr, destination[1]);
    EXPECT_TRUE(TokensEqual(destination[2], alice));
}

TEST_F(JArrayComplexTest, ToStringWithNullMatchesJsonDump)
{
    JArray array;
    array.Add(nullptr);
    array.Add(alice);
    array.Add(bob);

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back(nullptr);
    expected.push_back(nlohmann::json::parse(alice->ToString()));
    expected.push_back(nlohmann::json::parse(bob->ToString()));

    EXPECT_EQ(expected.dump(), array.ToString());
}

TEST(JArrayParseTest, ParseHandlesNullEntries)
{
    const std::string json =
        "[null,{\"name\":\"alice\"},{\"name\":\"bob\"}]";

    auto parsedToken = JToken::Parse(json);
    auto parsedArray = std::dynamic_pointer_cast<JArray>(parsedToken);
    ASSERT_NE(parsedArray, nullptr);
    EXPECT_EQ(3u, parsedArray->Count());
    EXPECT_EQ(nullptr, (*parsedArray)[0]);
    auto aliceObject = std::dynamic_pointer_cast<JObject>((*parsedArray)[1]);
    auto bobObject = std::dynamic_pointer_cast<JObject>((*parsedArray)[2]);
    ASSERT_NE(aliceObject, nullptr);
    ASSERT_NE(bobObject, nullptr);
    EXPECT_EQ("alice", (*aliceObject)["name"]->AsString());
    EXPECT_EQ("bob", (*bobObject)["name"]->AsString());
}
}  // namespace neo::json::tests
