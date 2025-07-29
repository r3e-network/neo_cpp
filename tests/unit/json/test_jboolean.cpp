#include <gtest/gtest.h>
#include <neo/json/jboolean.h>

namespace neo::json::tests
{
class JBooleanTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        true_token = std::make_shared<JBoolean>(true);
        false_token = std::make_shared<JBoolean>(false);
    }

    std::shared_ptr<JBoolean> true_token;
    std::shared_ptr<JBoolean> false_token;
};

TEST_F(JBooleanTest, TestGetType)
{
    EXPECT_EQ(JTokenType::Boolean, true_token->GetType());
    EXPECT_EQ(JTokenType::Boolean, false_token->GetType());
}

TEST_F(JBooleanTest, TestAsBoolean)
{
    EXPECT_TRUE(true_token->AsBoolean());
    EXPECT_FALSE(false_token->AsBoolean());
}

TEST_F(JBooleanTest, TestGetBoolean)
{
    EXPECT_TRUE(true_token->GetBoolean());
    EXPECT_FALSE(false_token->GetBoolean());
}

TEST_F(JBooleanTest, TestToString)
{
    EXPECT_EQ("true", true_token->ToString());
    EXPECT_EQ("false", false_token->ToString());
}

TEST_F(JBooleanTest, TestClone)
{
    auto true_clone = std::dynamic_pointer_cast<JBoolean>(true_token->Clone());
    auto false_clone = std::dynamic_pointer_cast<JBoolean>(false_token->Clone());

    EXPECT_NE(true_token.get(), true_clone.get());
    EXPECT_NE(false_token.get(), false_clone.get());

    EXPECT_EQ(true_token->GetBoolean(), true_clone->GetBoolean());
    EXPECT_EQ(false_token->GetBoolean(), false_clone->GetBoolean());
}

TEST_F(JBooleanTest, TestEquals)
{
    auto another_true = std::make_shared<JBoolean>(true);
    auto another_false = std::make_shared<JBoolean>(false);

    EXPECT_TRUE(true_token->Equals(*another_true));
    EXPECT_TRUE(false_token->Equals(*another_false));
    EXPECT_FALSE(true_token->Equals(*false_token));
    EXPECT_FALSE(false_token->Equals(*true_token));
}

TEST_F(JBooleanTest, TestImplicitConversion)
{
    bool true_val = *true_token;
    bool false_val = *false_token;

    EXPECT_TRUE(true_val);
    EXPECT_FALSE(false_val);
}

TEST_F(JBooleanTest, TestGetValue)
{
    EXPECT_TRUE(true_token->GetValue());
    EXPECT_FALSE(false_token->GetValue());
}

TEST_F(JBooleanTest, TestWriteJson)
{
    std::string output;

    true_token->WriteJson(output, false, 0);
    EXPECT_EQ("true", output);

    output.clear();
    false_token->WriteJson(output, false, 0);
    EXPECT_EQ("false", output);

    // Test with indentation (should not affect boolean output)
    output.clear();
    true_token->WriteJson(output, true, 2);
    EXPECT_EQ("true", output);
}

TEST_F(JBooleanTest, TestConstructor)
{
    // Test explicit construction
    JBoolean explicit_true(true);
    JBoolean explicit_false(false);

    EXPECT_TRUE(explicit_true.GetBoolean());
    EXPECT_FALSE(explicit_false.GetBoolean());
}
}  // namespace neo::json::tests
