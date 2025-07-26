// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_ipaddressextensions.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_IPADDRESSEXTENSIONS_CPP_H
#define TESTS_UNIT_MISC_TEST_IPADDRESSEXTENSIONS_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/extensions/ipaddress_extensions.h>

namespace neo {
namespace test {

class IpAddressExtensionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for IPAddressExtensions testing
        ipv4_address = "192.168.1.1";
        ipv6_address = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
        localhost_ipv4 = "127.0.0.1";
        localhost_ipv6 = "::1";
        invalid_address = "not.an.ip.address";
        private_network = "10.0.0.1";
    }

    void TearDown() override {
        // Clean up test fixtures - strings handle their own memory
    }

    // Helper methods and test data for IPAddressExtensions testing
    std::string ipv4_address;
    std::string ipv6_address;
    std::string localhost_ipv4;
    std::string localhost_ipv6;
    std::string invalid_address;
    std::string private_network;
};

// IPAddressExtensions test methods converted from C# UT_IpAddressExtensions.cs functionality

TEST_F(IpAddressExtensionsTest, IsValidIPv4Address) {
    EXPECT_TRUE(IPAddressExtensions::IsValidIPv4(ipv4_address));
    EXPECT_TRUE(IPAddressExtensions::IsValidIPv4(localhost_ipv4));
    EXPECT_TRUE(IPAddressExtensions::IsValidIPv4(private_network));
    EXPECT_FALSE(IPAddressExtensions::IsValidIPv4(invalid_address));
    EXPECT_FALSE(IPAddressExtensions::IsValidIPv4(ipv6_address));
}

TEST_F(IpAddressExtensionsTest, IsValidIPv6Address) {
    EXPECT_TRUE(IPAddressExtensions::IsValidIPv6(ipv6_address));
    EXPECT_TRUE(IPAddressExtensions::IsValidIPv6(localhost_ipv6));
    EXPECT_FALSE(IPAddressExtensions::IsValidIPv6(invalid_address));
    EXPECT_FALSE(IPAddressExtensions::IsValidIPv6(ipv4_address));
}

TEST_F(IpAddressExtensionsTest, IsLocalhost) {
    EXPECT_TRUE(IPAddressExtensions::IsLocalhost(localhost_ipv4));
    EXPECT_TRUE(IPAddressExtensions::IsLocalhost(localhost_ipv6));
    EXPECT_FALSE(IPAddressExtensions::IsLocalhost(ipv4_address));
    EXPECT_FALSE(IPAddressExtensions::IsLocalhost(private_network));
}

TEST_F(IpAddressExtensionsTest, IsPrivateNetwork) {
    EXPECT_TRUE(IPAddressExtensions::IsPrivateNetwork(private_network));
    EXPECT_TRUE(IPAddressExtensions::IsPrivateNetwork("192.168.1.1"));
    EXPECT_TRUE(IPAddressExtensions::IsPrivateNetwork("172.16.0.1"));
    EXPECT_FALSE(IPAddressExtensions::IsPrivateNetwork("8.8.8.8")); // Public DNS
}

TEST_F(IpAddressExtensionsTest, ParseIPAddress) {
    auto ipv4 = IPAddressExtensions::Parse(ipv4_address);
    EXPECT_NE(ipv4, nullptr);
    EXPECT_EQ(ipv4->ToString(), ipv4_address);
    
    auto ipv6 = IPAddressExtensions::Parse(ipv6_address);
    EXPECT_NE(ipv6, nullptr);
}

TEST_F(IpAddressExtensionsTest, ParseInvalidAddress) {
    EXPECT_THROW(IPAddressExtensions::Parse(invalid_address), std::exception);
}

TEST_F(IpAddressExtensionsTest, GetAddressBytes) {
    auto ipv4 = IPAddressExtensions::Parse(ipv4_address);
    ASSERT_NE(ipv4, nullptr);
    
    auto bytes = IPAddressExtensions::GetBytes(ipv4);
    EXPECT_EQ(bytes.size(), 4); // IPv4 is 4 bytes
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_IPADDRESSEXTENSIONS_CPP_H
