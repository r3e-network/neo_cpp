// Copyright (C) 2015-2025 The Neo Project.
//
// test_big_decimal.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/core/big_decimal.h"
#include <stdexcept>

namespace neo {
namespace test {

class BigDecimalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }

    void TearDown() override {
        // Clean up test fixtures
    }
};

// Test constructors
TEST_F(BigDecimalTest, TestDefaultConstructor) {
    BigDecimal bd;
    EXPECT_EQ(bd.value(), 0);
    EXPECT_EQ(bd.decimals(), 0);
    EXPECT_EQ(bd.sign(), 0);
}

TEST_F(BigDecimalTest, TestBigIntegerConstructor) {
    BigDecimal::BigInteger value(12345);
    BigDecimal bd(value, 2);
    EXPECT_EQ(bd.value(), 12345);
    EXPECT_EQ(bd.decimals(), 2);
    EXPECT_EQ(bd.sign(), 1);
}

TEST_F(BigDecimalTest, TestDoubleConstructor) {
    BigDecimal bd(123.45);
    EXPECT_EQ(bd.decimals(), 2);
    EXPECT_EQ(bd.to_string(), "123.45");
}

TEST_F(BigDecimalTest, TestIntegerConstructor) {
    BigDecimal bd(12345, 3);
    EXPECT_EQ(bd.value(), 12345);
    EXPECT_EQ(bd.decimals(), 3);
    EXPECT_EQ(bd.to_string(), "12.345");
}

// Test sign method
TEST_F(BigDecimalTest, TestSign) {
    BigDecimal positive(100, 0);
    BigDecimal negative(-100, 0);
    BigDecimal zero(0, 0);
    
    EXPECT_EQ(positive.sign(), 1);
    EXPECT_EQ(negative.sign(), -1);
    EXPECT_EQ(zero.sign(), 0);
}

// Test change_decimals method
TEST_F(BigDecimalTest, TestChangeDecimals) {
    BigDecimal bd(12345, 2); // 123.45
    
    // Increase decimals
    BigDecimal bd3 = bd.change_decimals(4); // 123.4500
    EXPECT_EQ(bd3.value(), 1234500);
    EXPECT_EQ(bd3.decimals(), 4);
    
    // Decrease decimals (exact division)
    BigDecimal bd1000(100000, 3); // 100.000
    BigDecimal bd1 = bd1000.change_decimals(0); // 100
    EXPECT_EQ(bd1.value(), 100);
    EXPECT_EQ(bd1.decimals(), 0);
    
    // Decrease decimals (would lose precision)
    EXPECT_THROW(bd.change_decimals(1), std::invalid_argument);
}

// Test to_string method
TEST_F(BigDecimalTest, TestToString) {
    EXPECT_EQ(BigDecimal(12345, 0).to_string(), "12345");
    EXPECT_EQ(BigDecimal(12345, 2).to_string(), "123.45");
    EXPECT_EQ(BigDecimal(12300, 2).to_string(), "123");
    EXPECT_EQ(BigDecimal(12340, 3).to_string(), "12.34");
    EXPECT_EQ(BigDecimal(-12345, 2).to_string(), "-123.45");
    EXPECT_EQ(BigDecimal(0, 2).to_string(), "0");
}

// Test parse method
TEST_F(BigDecimalTest, TestParse) {
    BigDecimal bd1 = BigDecimal::parse("123.45", 2);
    EXPECT_EQ(bd1.value(), 12345);
    EXPECT_EQ(bd1.decimals(), 2);
    
    BigDecimal bd2 = BigDecimal::parse("123", 2);
    EXPECT_EQ(bd2.value(), 12300);
    EXPECT_EQ(bd2.decimals(), 2);
    
    BigDecimal bd3 = BigDecimal::parse("-123.45", 2);
    EXPECT_EQ(bd3.value(), -12345);
    EXPECT_EQ(bd3.decimals(), 2);
    
    // Scientific notation
    BigDecimal bd4 = BigDecimal::parse("1.23e2", 2);
    EXPECT_EQ(bd4.value(), 12300);
    EXPECT_EQ(bd4.decimals(), 2);
    
    // Invalid format
    EXPECT_THROW(BigDecimal::parse("invalid", 2), std::invalid_argument);
}

// Test try_parse method
TEST_F(BigDecimalTest, TestTryParse) {
    auto result1 = BigDecimal::try_parse("123.45", 2);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1->value(), 12345);
    
    auto result2 = BigDecimal::try_parse("invalid", 2);
    EXPECT_FALSE(result2.has_value());
}

// Test arithmetic operators
TEST_F(BigDecimalTest, TestAddition) {
    BigDecimal a(12345, 2); // 123.45
    BigDecimal b(6789, 2);  // 67.89
    BigDecimal result = a + b;
    
    EXPECT_EQ(result.value(), 19134); // 191.34
    EXPECT_EQ(result.decimals(), 2);
}

TEST_F(BigDecimalTest, TestSubtraction) {
    BigDecimal a(12345, 2); // 123.45
    BigDecimal b(6789, 2);  // 67.89
    BigDecimal result = a - b;
    
    EXPECT_EQ(result.value(), 5556); // 55.56
    EXPECT_EQ(result.decimals(), 2);
}

TEST_F(BigDecimalTest, TestMultiplication) {
    BigDecimal a(123, 1); // 12.3
    BigDecimal b(45, 1);  // 4.5
    BigDecimal result = a * b;
    
    EXPECT_EQ(result.value(), 5535); // 55.35
    EXPECT_EQ(result.decimals(), 2);
}

TEST_F(BigDecimalTest, TestDivision) {
    BigDecimal a(1000, 2); // 10.00
    BigDecimal b(200, 2);  // 2.00
    BigDecimal result = a / b;
    
    EXPECT_EQ(result.value(), 500); // 5.00
    EXPECT_EQ(result.decimals(), 2);
    
    // Division by zero
    BigDecimal zero(0, 2);
    EXPECT_THROW(a / zero, std::runtime_error);
}

TEST_F(BigDecimalTest, TestModulo) {
    BigDecimal a(1050, 2); // 10.50
    BigDecimal b(300, 2);  // 3.00
    BigDecimal result = a % b;
    
    EXPECT_EQ(result.value(), 150); // 1.50
    EXPECT_EQ(result.decimals(), 2);
}

TEST_F(BigDecimalTest, TestUnaryMinus) {
    BigDecimal a(12345, 2); // 123.45
    BigDecimal result = -a;
    
    EXPECT_EQ(result.value(), -12345); // -123.45
    EXPECT_EQ(result.decimals(), 2);
}

// Test comparison operators
TEST_F(BigDecimalTest, TestComparison) {
    BigDecimal a(12345, 2); // 123.45
    BigDecimal b(6789, 2);  // 67.89
    BigDecimal c(12345, 2); // 123.45
    
    EXPECT_TRUE(a > b);
    EXPECT_TRUE(b < a);
    EXPECT_TRUE(a >= c);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
}

TEST_F(BigDecimalTest, TestComparisonDifferentDecimals) {
    BigDecimal a(1234, 2);  // 12.34
    BigDecimal b(12340, 3); // 12.340
    
    EXPECT_TRUE(a == b); // Should be equal when normalized
}

// Test assignment operators
TEST_F(BigDecimalTest, TestAssignmentOperators) {
    BigDecimal a(1000, 2); // 10.00
    BigDecimal b(200, 2);  // 2.00
    
    a += b;
    EXPECT_EQ(a.value(), 1200); // 12.00
    
    a -= b;
    EXPECT_EQ(a.value(), 1000); // 10.00
    
    a *= b;
    EXPECT_EQ(a.value(), 200000); // 20.00 (with 4 decimals)
    EXPECT_EQ(a.decimals(), 4);
    
    BigDecimal c(400, 2); // 4.00
    c /= BigDecimal(200, 2); // 2.00
    EXPECT_EQ(c.value(), 200); // 2.00
}

// Test edge cases
TEST_F(BigDecimalTest, TestZeroHandling) {
    BigDecimal zero(0, 2);
    BigDecimal nonZero(100, 2);
    
    EXPECT_EQ((zero + nonZero).value(), 100);
    EXPECT_EQ((nonZero - nonZero).value(), 0);
    EXPECT_EQ((zero * nonZero).value(), 0);
    EXPECT_EQ((zero % nonZero).value(), 0);
}

TEST_F(BigDecimalTest, TestLargeNumbers) {
    BigDecimal::BigInteger large("123456789012345678901234567890");
    BigDecimal bd(large, 10);
    
    EXPECT_EQ(bd.value(), large);
    EXPECT_EQ(bd.decimals(), 10);
    
    // Test that operations work with large numbers
    BigDecimal doubled = bd + bd;
    EXPECT_EQ(doubled.value(), large * 2);
}

} // namespace test
} // namespace neo
