// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_stringextensions.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_STRINGEXTENSIONS_CPP_H
#define TESTS_UNIT_MISC_TEST_STRINGEXTENSIONS_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/extensions/string_extensions.h>

namespace neo {
namespace test {

class StringExtensionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for StringExtensions testing
        test_string = "Hello World";
        empty_string = "";
        hex_string = "48656c6c6f20576f726c64"; // "Hello World" in hex
        whitespace_string = "  Hello World  ";
        number_string = "12345";
    }

    void TearDown() override {
        // Clean up test fixtures - strings handle their own memory
    }

    // Helper methods and test data for StringExtensions testing
    std::string test_string;
    std::string empty_string;
    std::string hex_string;
    std::string whitespace_string;
    std::string number_string;
};

// StringExtensions test methods converted from C# UT_StringExtensions.cs functionality

TEST_F(StringExtensionsTest, ToLowerCase) {
    std::string upper = "HELLO WORLD";
    std::string result = StringExtensions::ToLower(upper);
    EXPECT_EQ(result, "hello world");
}

TEST_F(StringExtensionsTest, ToUpperCase) {
    std::string lower = "hello world";
    std::string result = StringExtensions::ToUpper(lower);
    EXPECT_EQ(result, "HELLO WORLD");
}

TEST_F(StringExtensionsTest, TrimWhitespace) {
    std::string result = StringExtensions::Trim(whitespace_string);
    EXPECT_EQ(result, "Hello World");
}

TEST_F(StringExtensionsTest, TrimEmptyString) {
    std::string result = StringExtensions::Trim(empty_string);
    EXPECT_EQ(result, "");
}

TEST_F(StringExtensionsTest, HexStringToByteArray) {
    auto result = StringExtensions::HexToBytes(hex_string);
    EXPECT_GT(result.size(), 0);
    // Convert back to verify roundtrip
    std::string back = StringExtensions::BytesToHex(result);
    EXPECT_EQ(back, hex_string);
}

TEST_F(StringExtensionsTest, IsNumeric) {
    EXPECT_TRUE(StringExtensions::IsNumeric(number_string));
    EXPECT_FALSE(StringExtensions::IsNumeric(test_string));
    EXPECT_FALSE(StringExtensions::IsNumeric(empty_string));
}

TEST_F(StringExtensionsTest, SplitString) {
    std::string csv = "apple,banana,cherry";
    auto result = StringExtensions::Split(csv, ",");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "apple");
    EXPECT_EQ(result[1], "banana");
    EXPECT_EQ(result[2], "cherry");
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_STRINGEXTENSIONS_CPP_H
