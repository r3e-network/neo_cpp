// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_datetimeextensions.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_DATETIMEEXTENSIONS_CPP_H
#define TESTS_UNIT_MISC_TEST_DATETIMEEXTENSIONS_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/extensions/datetime_extensions.h>

namespace neo
{
namespace test
{

class DateTimeExtensionsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for DateTimeExtensions testing
        unix_epoch = 0;         // 1970-01-01 00:00:00 UTC
        neo_epoch = 946684800;  // 2000-01-01 00:00:00 UTC
        current_time = std::time(nullptr);
        test_timestamp = 1609459200;    // 2021-01-01 00:00:00 UTC
        future_timestamp = 2147483647;  // 2038-01-19 03:14:07 UTC (int32 max)
    }

    void TearDown() override
    {
        // Clean up test fixtures - primitive types handle their own cleanup
    }

    // Helper methods and test data for DateTimeExtensions testing
    std::time_t unix_epoch;
    std::time_t neo_epoch;
    std::time_t current_time;
    std::time_t test_timestamp;
    std::time_t future_timestamp;
};

// DateTimeExtensions test methods converted from C# UT_DateTimeExtensions.cs functionality

TEST_F(DateTimeExtensionsTest, ToUnixTimestamp)
{
    auto timestamp = DateTimeExtensions::ToUnixTimestamp(test_timestamp);
    EXPECT_EQ(timestamp, test_timestamp);
}

TEST_F(DateTimeExtensionsTest, FromUnixTimestamp)
{
    auto datetime = DateTimeExtensions::FromUnixTimestamp(test_timestamp);
    auto back_to_timestamp = DateTimeExtensions::ToUnixTimestamp(datetime);
    EXPECT_EQ(back_to_timestamp, test_timestamp);
}

TEST_F(DateTimeExtensionsTest, ToNeoTimestamp)
{
    // Neo timestamp is seconds since 2000-01-01 UTC
    auto neo_timestamp = DateTimeExtensions::ToNeoTimestamp(test_timestamp);
    EXPECT_GT(neo_timestamp, 0);               // Should be positive for dates after 2000
    EXPECT_LT(neo_timestamp, test_timestamp);  // Should be less than Unix timestamp
}

TEST_F(DateTimeExtensionsTest, FromNeoTimestamp)
{
    auto neo_timestamp = DateTimeExtensions::ToNeoTimestamp(test_timestamp);
    auto datetime = DateTimeExtensions::FromNeoTimestamp(neo_timestamp);
    auto back_to_neo = DateTimeExtensions::ToNeoTimestamp(datetime);
    EXPECT_EQ(back_to_neo, neo_timestamp);
}

TEST_F(DateTimeExtensionsTest, GetCurrentTimestamp)
{
    auto current1 = DateTimeExtensions::GetCurrentUnixTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto current2 = DateTimeExtensions::GetCurrentUnixTimestamp();
    EXPECT_GE(current2, current1);  // Time should progress
}

TEST_F(DateTimeExtensionsTest, FormatDateTime)
{
    auto datetime = DateTimeExtensions::FromUnixTimestamp(test_timestamp);
    std::string formatted = DateTimeExtensions::ToString(datetime, "%Y-%m-%d %H:%M:%S");
    EXPECT_EQ(formatted, "2021-01-01 00:00:00");
}

TEST_F(DateTimeExtensionsTest, ParseDateTime)
{
    std::string date_string = "2021-01-01 00:00:00";
    auto datetime = DateTimeExtensions::Parse(date_string, "%Y-%m-%d %H:%M:%S");
    auto timestamp = DateTimeExtensions::ToUnixTimestamp(datetime);
    EXPECT_EQ(timestamp, test_timestamp);
}

TEST_F(DateTimeExtensionsTest, IsLeapYear)
{
    EXPECT_TRUE(DateTimeExtensions::IsLeapYear(2020));
    EXPECT_FALSE(DateTimeExtensions::IsLeapYear(2021));
    EXPECT_TRUE(DateTimeExtensions::IsLeapYear(2000));
    EXPECT_FALSE(DateTimeExtensions::IsLeapYear(1900));
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_MISC_TEST_DATETIMEEXTENSIONS_CPP_H
