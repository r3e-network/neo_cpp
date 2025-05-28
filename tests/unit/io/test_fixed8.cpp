#include <gtest/gtest.h>
#include <neo/io/fixed8.h>
#include <limits>

using namespace neo::io;

TEST(Fixed8Test, Constructor)
{
    // Default constructor
    Fixed8 f1;
    EXPECT_EQ(f1.Value(), 0);

    // Value constructor
    Fixed8 f2(123456789);
    EXPECT_EQ(f2.Value(), 123456789);
}

TEST(Fixed8Test, ToDouble)
{
    Fixed8 f1(123456789);
    EXPECT_DOUBLE_EQ(f1.ToDouble(), 1.23456789);

    Fixed8 f2(-123456789);
    EXPECT_DOUBLE_EQ(f2.ToDouble(), -1.23456789);
}

TEST(Fixed8Test, ToString)
{
    Fixed8 f1(123456789);
    EXPECT_EQ(f1.ToString(), "1.23456789");

    Fixed8 f2(-123456789);
    EXPECT_EQ(f2.ToString(), "-1.23456789");

    Fixed8 f3(100000000);
    EXPECT_EQ(f3.ToString(), "1");

    Fixed8 f4(100000);
    EXPECT_EQ(f4.ToString(), "0.001");

    Fixed8 f5(0);
    EXPECT_EQ(f5.ToString(), "0");
}

TEST(Fixed8Test, Addition)
{
    Fixed8 f1(100000000); // 1.0
    Fixed8 f2(200000000); // 2.0
    
    Fixed8 sum = f1 + f2;
    EXPECT_EQ(sum.Value(), 300000000); // 3.0
    
    // Overflow
    Fixed8 max = Fixed8::MaxValue();
    EXPECT_THROW(max + Fixed8(1), std::overflow_error);
    
    Fixed8 min = Fixed8::MinValue();
    EXPECT_THROW(min + Fixed8(-1), std::overflow_error);
}

TEST(Fixed8Test, Subtraction)
{
    Fixed8 f1(300000000); // 3.0
    Fixed8 f2(100000000); // 1.0
    
    Fixed8 diff = f1 - f2;
    EXPECT_EQ(diff.Value(), 200000000); // 2.0
    
    // Overflow
    Fixed8 max = Fixed8::MaxValue();
    EXPECT_THROW(max - Fixed8(-1), std::overflow_error);
    
    Fixed8 min = Fixed8::MinValue();
    EXPECT_THROW(min - Fixed8(1), std::overflow_error);
}

TEST(Fixed8Test, Multiplication)
{
    Fixed8 f1(200000000); // 2.0
    Fixed8 f2(300000000); // 3.0
    
    Fixed8 product = f1 * f2;
    EXPECT_EQ(product.Value(), 600000000); // 6.0
    
    // Overflow
    Fixed8 max = Fixed8::MaxValue();
    EXPECT_THROW(max * Fixed8(2), std::overflow_error);
    
    Fixed8 min = Fixed8::MinValue();
    EXPECT_THROW(min * Fixed8(2), std::overflow_error);
}

TEST(Fixed8Test, Division)
{
    Fixed8 f1(600000000); // 6.0
    Fixed8 f2(200000000); // 2.0
    
    Fixed8 quotient = f1 / f2;
    EXPECT_EQ(quotient.Value(), 300000000); // 3.0
    
    // Division by zero
    EXPECT_THROW(f1 / Fixed8(0), std::invalid_argument);
    
    // Overflow
    Fixed8 min = Fixed8::MinValue();
    EXPECT_THROW(min / Fixed8(-1), std::overflow_error);
}

TEST(Fixed8Test, Comparison)
{
    Fixed8 f1(100000000); // 1.0
    Fixed8 f2(200000000); // 2.0
    Fixed8 f3(100000000); // 1.0
    
    EXPECT_TRUE(f1 == f3);
    EXPECT_FALSE(f1 == f2);
    
    EXPECT_TRUE(f1 != f2);
    EXPECT_FALSE(f1 != f3);
    
    EXPECT_TRUE(f1 < f2);
    EXPECT_FALSE(f2 < f1);
    EXPECT_FALSE(f1 < f3);
    
    EXPECT_TRUE(f1 <= f2);
    EXPECT_TRUE(f1 <= f3);
    EXPECT_FALSE(f2 <= f1);
    
    EXPECT_TRUE(f2 > f1);
    EXPECT_FALSE(f1 > f2);
    EXPECT_FALSE(f1 > f3);
    
    EXPECT_TRUE(f2 >= f1);
    EXPECT_TRUE(f1 >= f3);
    EXPECT_FALSE(f1 >= f2);
}

TEST(Fixed8Test, FromDecimal)
{
    Fixed8 f1 = Fixed8::FromDecimal(1.23456789);
    EXPECT_EQ(f1.Value(), 123456789);
    
    Fixed8 f2 = Fixed8::FromDecimal(-1.23456789);
    EXPECT_EQ(f2.Value(), -123456789);
    
    // Overflow
    EXPECT_THROW(Fixed8::FromDecimal(std::numeric_limits<double>::max()), std::overflow_error);
    EXPECT_THROW(Fixed8::FromDecimal(std::numeric_limits<double>::lowest()), std::overflow_error);
}

TEST(Fixed8Test, Constants)
{
    Fixed8 zero = Fixed8::Zero();
    EXPECT_EQ(zero.Value(), 0);
    
    Fixed8 one = Fixed8::One();
    EXPECT_EQ(one.Value(), Fixed8::Decimals);
    
    Fixed8 max = Fixed8::MaxValue();
    EXPECT_EQ(max.Value(), INT64_MAX);
    
    Fixed8 min = Fixed8::MinValue();
    EXPECT_EQ(min.Value(), INT64_MIN);
}
