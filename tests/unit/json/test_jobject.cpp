#include <gtest/gtest.h>
#include <neo/json/jboolean.h>
#include <neo/json/jnumber.h>
#include <neo/json/jobject.h>
#include <neo/json/jstring.h>

namespace neo::json::tests
{
class JObjectTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create alice object
        alice = std::make_shared<JObject>();
        alice->SetProperty("name", std::make_shared<JString>("alice"));
        alice->SetProperty("age", std::make_shared<JNumber>(30));
        alice->SetProperty("score", std::make_shared<JNumber>(100.001));
        alice->SetProperty("isMarried", std::make_shared<JBoolean>(true));

        auto pet1 = std::make_shared<JObject>();
        pet1->SetProperty("name", std::make_shared<JString>("Tom"));
        pet1->SetProperty("type", std::make_shared<JString>("cat"));
        alice->SetProperty("pet", pet1);

        // Create bob object
        bob = std::make_shared<JObject>();
        bob->SetProperty("name", std::make_shared<JString>("bob"));
        bob->SetProperty("age", std::make_shared<JNumber>(100000));
        bob->SetProperty("score", std::make_shared<JNumber>(0.001));
        bob->SetProperty("isMarried", std::make_shared<JBoolean>(false));

        auto pet2 = std::make_shared<JObject>();
        pet2->SetProperty("name", std::make_shared<JString>("Paul"));
        pet2->SetProperty("type", std::make_shared<JString>("dog"));
        bob->SetProperty("pet", pet2);
    }

    std::shared_ptr<JObject> alice;
    std::shared_ptr<JObject> bob;
};

TEST_F(JObjectTest, TestAsBoolean)
{
    EXPECT_TRUE(alice->AsBoolean());
}

TEST_F(JObjectTest, TestAsNumber)
{
    EXPECT_TRUE(std::isnan(alice->AsNumber()));
}

TEST_F(JObjectTest, TestParse)
{
    // Test invalid JSON
    EXPECT_THROW(JToken::Parse("aaa"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("hello world"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("100.a"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("\"a"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("{\"k1\":\"v1\""), std::runtime_error);

    // Test valid JSON
    auto null_token = JToken::Parse("null");
    EXPECT_EQ(null_token, nullptr);

    auto true_token = JToken::Parse("true");
    EXPECT_TRUE(true_token->AsBoolean());

    auto false_token = JToken::Parse("false");
    EXPECT_FALSE(false_token->AsBoolean());

    auto string_token = JToken::Parse("\"hello world\"");
    EXPECT_EQ("hello world", string_token->AsString());

    auto escaped_token = JToken::Parse("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"");
    EXPECT_EQ("\"\\/\b\f\n\r\t", escaped_token->AsString());

    auto object_token = JToken::Parse("{\"k1\":\"v1\"}");
    EXPECT_EQ("{\"k1\":\"v1\"}", object_token->ToString());
}

TEST_F(JObjectTest, TestGetNull)
{
    EXPECT_EQ(JToken::Null, nullptr);
}

TEST_F(JObjectTest, TestClone)
{
    auto bob_clone = std::dynamic_pointer_cast<JObject>(bob->Clone());
    EXPECT_NE(bob.get(), bob_clone.get());

    // Check that all properties are cloned correctly
    const auto& bob_props = bob->GetProperties();
    const auto& clone_props = bob_clone->GetProperties();

    EXPECT_EQ(bob_props.size(), clone_props.size());

    for (size_t i = 0; i < bob_props.size(); ++i)
    {
        const auto& key = bob_props.key_at(i);
        const auto& bob_value = bob_props.value_at(i);
        const auto& clone_value = clone_props.at(key);

        if (bob_value == nullptr)
        {
            EXPECT_EQ(clone_value, nullptr);
        }
        else
        {
            EXPECT_NE(bob_value.get(), clone_value.get());  // Different objects
            EXPECT_TRUE(bob_value->Equals(*clone_value));   // Same content
        }
    }
}

TEST_F(JObjectTest, TestPropertyAccess)
{
    // Test property access
    auto name = (*alice)["name"];
    EXPECT_NE(name, nullptr);
    EXPECT_EQ("alice", name->AsString());

    auto age = (*alice)["age"];
    EXPECT_NE(age, nullptr);
    EXPECT_EQ(30, age->AsNumber());

    // Test non-existent property
    auto nonexistent = (*alice)["nonexistent"];
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(JObjectTest, TestContainsProperty)
{
    EXPECT_TRUE(alice->ContainsProperty("name"));
    EXPECT_TRUE(alice->ContainsProperty("age"));
    EXPECT_FALSE(alice->ContainsProperty("nonexistent"));
}

TEST_F(JObjectTest, TestClear)
{
    EXPECT_GT(alice->Count(), 0);
    alice->Clear();
    EXPECT_EQ(alice->Count(), 0);
}

TEST_F(JObjectTest, TestEquals)
{
    auto alice_clone = std::dynamic_pointer_cast<JObject>(alice->Clone());
    EXPECT_TRUE(alice->Equals(*alice_clone));
    EXPECT_FALSE(alice->Equals(*bob));
}

TEST_F(JObjectTest, TestToString)
{
    std::string json_str = alice->ToString();
    EXPECT_FALSE(json_str.empty());
    EXPECT_NE(json_str.find("alice"), std::string::npos);
    EXPECT_NE(json_str.find("30"), std::string::npos);
}
}  // namespace neo::json::tests
