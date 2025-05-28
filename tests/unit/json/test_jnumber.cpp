#include <gtest/gtest.h>
#include <neo/json/jnumber.h>
#include <cmath>
#include <limits>

namespace neo::json::tests
{
    class JNumberTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            integer_number = std::make_shared<JNumber>(42.0);
            float_number = std::make_shared<JNumber>(3.14159);
            zero_number = std::make_shared<JNumber>(0.0);
            negative_number = std::make_shared<JNumber>(-123.456);
            large_number = std::make_shared<JNumber>(1e20);
            small_number = std::make_shared<JNumber>(1e-20);
            nan_number = std::make_shared<JNumber>(std::numeric_limits<double>::quiet_NaN());
            inf_number = std::make_shared<JNumber>(std::numeric_limits<double>::infinity());
        }

        std::shared_ptr<JNumber> integer_number;
        std::shared_ptr<JNumber> float_number;
        std::shared_ptr<JNumber> zero_number;
        std::shared_ptr<JNumber> negative_number;
        std::shared_ptr<JNumber> large_number;
        std::shared_ptr<JNumber> small_number;
        std::shared_ptr<JNumber> nan_number;
        std::shared_ptr<JNumber> inf_number;
    };

    TEST_F(JNumberTest, TestGetType)
    {
        EXPECT_EQ(JTokenType::Number, integer_number->GetType());
    }

    TEST_F(JNumberTest, TestAsNumber)
    {
        EXPECT_EQ(42.0, integer_number->AsNumber());
        EXPECT_DOUBLE_EQ(3.14159, float_number->AsNumber());
        EXPECT_EQ(0.0, zero_number->AsNumber());
        EXPECT_DOUBLE_EQ(-123.456, negative_number->AsNumber());
    }

    TEST_F(JNumberTest, TestGetNumber)
    {
        EXPECT_EQ(42.0, integer_number->GetNumber());
        EXPECT_DOUBLE_EQ(3.14159, float_number->GetNumber());
    }

    TEST_F(JNumberTest, TestToString)
    {
        EXPECT_EQ("42", integer_number->ToString());
        EXPECT_EQ("0", zero_number->ToString());
        
        // Float numbers should preserve precision
        std::string float_str = float_number->ToString();
        EXPECT_NE(float_str.find("3.14159"), std::string::npos);
        
        // Negative numbers
        std::string neg_str = negative_number->ToString();
        EXPECT_EQ('-', neg_str[0]);
        
        // NaN and infinity should be converted to "null" in JSON
        EXPECT_EQ("null", nan_number->ToString());
        EXPECT_EQ("null", inf_number->ToString());
    }

    TEST_F(JNumberTest, TestClone)
    {
        auto cloned = std::dynamic_pointer_cast<JNumber>(integer_number->Clone());
        EXPECT_NE(integer_number.get(), cloned.get());
        EXPECT_EQ(integer_number->GetNumber(), cloned->GetNumber());
    }

    TEST_F(JNumberTest, TestEquals)
    {
        auto same_number = std::make_shared<JNumber>(42.0);
        auto different_number = std::make_shared<JNumber>(43.0);
        
        EXPECT_TRUE(integer_number->Equals(*same_number));
        EXPECT_FALSE(integer_number->Equals(*different_number));
        
        // Test NaN equality (NaN should equal NaN in our implementation)
        auto another_nan = std::make_shared<JNumber>(std::numeric_limits<double>::quiet_NaN());
        EXPECT_TRUE(nan_number->Equals(*another_nan));
        
        // NaN should not equal any regular number
        EXPECT_FALSE(nan_number->Equals(*integer_number));
    }

    TEST_F(JNumberTest, TestImplicitConversions)
    {
        double double_val = *integer_number;
        EXPECT_EQ(42.0, double_val);
        
        int int_val = *integer_number;
        EXPECT_EQ(42, int_val);
        
        // Test conversion of float to int (should truncate)
        int float_to_int = *float_number;
        EXPECT_EQ(3, float_to_int);
    }

    TEST_F(JNumberTest, TestGetValue)
    {
        EXPECT_EQ(42.0, integer_number->GetValue());
        EXPECT_DOUBLE_EQ(3.14159, float_number->GetValue());
    }

    TEST_F(JNumberTest, TestLargeNumbers)
    {
        EXPECT_EQ(1e20, large_number->GetNumber());
        EXPECT_EQ(1e-20, small_number->GetNumber());
        
        // Large numbers should be represented in scientific notation
        std::string large_str = large_number->ToString();
        EXPECT_FALSE(large_str.empty());
        
        std::string small_str = small_number->ToString();
        EXPECT_FALSE(small_str.empty());
    }

    TEST_F(JNumberTest, TestSpecialValues)
    {
        // Test NaN
        EXPECT_TRUE(std::isnan(nan_number->GetNumber()));
        
        // Test infinity
        EXPECT_TRUE(std::isinf(inf_number->GetNumber()));
        EXPECT_GT(inf_number->GetNumber(), 0);
        
        // Test negative infinity
        auto neg_inf = std::make_shared<JNumber>(-std::numeric_limits<double>::infinity());
        EXPECT_TRUE(std::isinf(neg_inf->GetNumber()));
        EXPECT_LT(neg_inf->GetNumber(), 0);
    }

    TEST_F(JNumberTest, TestIntegerDetection)
    {
        // Integer values should be formatted without decimal point
        auto int_val = std::make_shared<JNumber>(100.0);
        EXPECT_EQ("100", int_val->ToString());
        
        // Non-integer values should include decimal point
        auto float_val = std::make_shared<JNumber>(100.5);
        std::string float_str = float_val->ToString();
        EXPECT_NE(float_str.find('.'), std::string::npos);
    }
}
