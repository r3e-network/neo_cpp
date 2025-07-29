#include <gtest/gtest.h>
#include <memory>
#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>
#include <string>
#include <vector>

using namespace neo::network::p2p;
using namespace neo::network;

/**
 * @brief Test fixture for ChannelsConfig
 */
class ChannelsConfigTest : public testing::Test
{
  protected:
    IPEndPoint testTcpEndpoint;
    IPEndPoint testWsEndpoint;
    std::vector<IPEndPoint> testSeedList;

    void SetUp() override
    {
        // Initialize test data
        testTcpEndpoint = IPEndPoint(IPAddress::Parse("127.0.0.1"), 10333);
        testWsEndpoint = IPEndPoint(IPAddress::Parse("127.0.0.1"), 10334);

        // Create test seed list
        testSeedList.push_back(IPEndPoint(IPAddress::Parse("10.0.0.1"), 10333));
        testSeedList.push_back(IPEndPoint(IPAddress::Parse("10.0.0.2"), 10333));
        testSeedList.push_back(IPEndPoint(IPAddress::Parse("10.0.0.3"), 10333));
    }
};

TEST_F(ChannelsConfigTest, DefaultConstructor)
{
    ChannelsConfig config;

    // Check default values
    EXPECT_EQ(0u, config.GetTcp().GetPort());
    EXPECT_EQ(0u, config.GetWebSocket().GetPort());
    EXPECT_GT(config.GetMinDesiredConnections(), 0u);  // Should have a default > 0
    EXPECT_GT(config.GetMaxConnections(), 0u);
    EXPECT_GE(config.GetMaxConnections(), config.GetMinDesiredConnections());
    EXPECT_GT(config.GetMaxConnectionsPerAddress(), 0u);
    EXPECT_GT(config.GetMaxKnownAddresses(), 0u);
    EXPECT_GT(config.GetMaxKnownHashes(), 0u);
    EXPECT_TRUE(config.GetSeedList().empty());
}

TEST_F(ChannelsConfigTest, GettersAndSetters_Endpoints)
{
    ChannelsConfig config;

    // Test TCP endpoint
    config.SetTcp(testTcpEndpoint);
    EXPECT_EQ(testTcpEndpoint.GetAddress().ToString(), config.GetTcp().GetAddress().ToString());
    EXPECT_EQ(testTcpEndpoint.GetPort(), config.GetTcp().GetPort());

    // Test WebSocket endpoint
    config.SetWebSocket(testWsEndpoint);
    EXPECT_EQ(testWsEndpoint.GetAddress().ToString(), config.GetWebSocket().GetAddress().ToString());
    EXPECT_EQ(testWsEndpoint.GetPort(), config.GetWebSocket().GetPort());
}

TEST_F(ChannelsConfigTest, GettersAndSetters_Connections)
{
    ChannelsConfig config;

    // Test MinDesiredConnections
    uint32_t minDesired = 10;
    config.SetMinDesiredConnections(minDesired);
    EXPECT_EQ(minDesired, config.GetMinDesiredConnections());

    // Test MaxConnections
    uint32_t maxConnections = 50;
    config.SetMaxConnections(maxConnections);
    EXPECT_EQ(maxConnections, config.GetMaxConnections());

    // Test MaxConnectionsPerAddress
    uint32_t maxPerAddress = 3;
    config.SetMaxConnectionsPerAddress(maxPerAddress);
    EXPECT_EQ(maxPerAddress, config.GetMaxConnectionsPerAddress());
}

TEST_F(ChannelsConfigTest, GettersAndSetters_Limits)
{
    ChannelsConfig config;

    // Test MaxKnownAddresses
    uint32_t maxAddresses = 5000;
    config.SetMaxKnownAddresses(maxAddresses);
    EXPECT_EQ(maxAddresses, config.GetMaxKnownAddresses());

    // Test MaxKnownHashes
    uint32_t maxHashes = 10000;
    config.SetMaxKnownHashes(maxHashes);
    EXPECT_EQ(maxHashes, config.GetMaxKnownHashes());
}

TEST_F(ChannelsConfigTest, GettersAndSetters_SeedList)
{
    ChannelsConfig config;

    // Initially empty
    EXPECT_TRUE(config.GetSeedList().empty());

    // Set seed list
    config.SetSeedList(testSeedList);
    EXPECT_EQ(testSeedList.size(), config.GetSeedList().size());

    // Verify each seed
    for (size_t i = 0; i < testSeedList.size(); ++i)
    {
        EXPECT_EQ(testSeedList[i].GetAddress().ToString(), config.GetSeedList()[i].GetAddress().ToString());
        EXPECT_EQ(testSeedList[i].GetPort(), config.GetSeedList()[i].GetPort());
    }
}

TEST_F(ChannelsConfigTest, UpdateValues)
{
    ChannelsConfig config;

    // Set initial values
    config.SetMinDesiredConnections(10);
    config.SetMaxConnections(50);
    config.SetMaxConnectionsPerAddress(3);

    // Update values
    config.SetMinDesiredConnections(20);
    config.SetMaxConnections(100);
    config.SetMaxConnectionsPerAddress(5);

    // Verify updates
    EXPECT_EQ(20u, config.GetMinDesiredConnections());
    EXPECT_EQ(100u, config.GetMaxConnections());
    EXPECT_EQ(5u, config.GetMaxConnectionsPerAddress());
}

TEST_F(ChannelsConfigTest, DifferentPorts)
{
    ChannelsConfig config;

    // Test with different ports
    IPEndPoint tcp1(IPAddress::Parse("192.168.1.1"), 10333);
    IPEndPoint ws1(IPAddress::Parse("192.168.1.1"), 10334);

    config.SetTcp(tcp1);
    config.SetWebSocket(ws1);

    EXPECT_NE(config.GetTcp().GetPort(), config.GetWebSocket().GetPort());
    EXPECT_EQ(config.GetTcp().GetAddress().ToString(), config.GetWebSocket().GetAddress().ToString());
}

TEST_F(ChannelsConfigTest, ZeroValues)
{
    ChannelsConfig config;

    // Test setting zero values (edge case)
    config.SetMinDesiredConnections(0);
    config.SetMaxConnections(0);
    config.SetMaxConnectionsPerAddress(0);
    config.SetMaxKnownAddresses(0);
    config.SetMaxKnownHashes(0);

    EXPECT_EQ(0u, config.GetMinDesiredConnections());
    EXPECT_EQ(0u, config.GetMaxConnections());
    EXPECT_EQ(0u, config.GetMaxConnectionsPerAddress());
    EXPECT_EQ(0u, config.GetMaxKnownAddresses());
    EXPECT_EQ(0u, config.GetMaxKnownHashes());
}

TEST_F(ChannelsConfigTest, MaxValues)
{
    ChannelsConfig config;

    // Test setting maximum values
    uint32_t maxValue = UINT32_MAX;

    config.SetMinDesiredConnections(maxValue);
    config.SetMaxConnections(maxValue);
    config.SetMaxConnectionsPerAddress(maxValue);
    config.SetMaxKnownAddresses(maxValue);
    config.SetMaxKnownHashes(maxValue);

    EXPECT_EQ(maxValue, config.GetMinDesiredConnections());
    EXPECT_EQ(maxValue, config.GetMaxConnections());
    EXPECT_EQ(maxValue, config.GetMaxConnectionsPerAddress());
    EXPECT_EQ(maxValue, config.GetMaxKnownAddresses());
    EXPECT_EQ(maxValue, config.GetMaxKnownHashes());
}

TEST_F(ChannelsConfigTest, EmptySeedList)
{
    ChannelsConfig config;
    std::vector<IPEndPoint> emptyList;

    config.SetSeedList(emptyList);
    EXPECT_TRUE(config.GetSeedList().empty());
}

TEST_F(ChannelsConfigTest, LargeSeedList)
{
    ChannelsConfig config;
    std::vector<IPEndPoint> largeSeedList;

    // Create a large seed list
    for (int i = 0; i < 1000; ++i)
    {
        std::string ip = "10.0." + std::to_string(i / 256) + "." + std::to_string(i % 256);
        largeSeedList.push_back(IPEndPoint(IPAddress::Parse(ip), 10333));
    }

    config.SetSeedList(largeSeedList);
    EXPECT_EQ(1000u, config.GetSeedList().size());
}

TEST_F(ChannelsConfigTest, IPv6Support)
{
    ChannelsConfig config;

    // Test with IPv6 addresses
    IPEndPoint tcpIPv6(IPAddress::Parse("::1"), 10333);
    IPEndPoint wsIPv6(IPAddress::Parse("2001:db8::1"), 10334);

    config.SetTcp(tcpIPv6);
    config.SetWebSocket(wsIPv6);

    EXPECT_EQ("::1", config.GetTcp().GetAddress().ToString());
    EXPECT_EQ("2001:db8::1", config.GetWebSocket().GetAddress().ToString());
}

TEST_F(ChannelsConfigTest, ValidConfiguration)
{
    ChannelsConfig config;

    // Set up a valid configuration
    config.SetTcp(IPEndPoint(IPAddress::Parse("0.0.0.0"), 10333));
    config.SetWebSocket(IPEndPoint(IPAddress::Parse("0.0.0.0"), 10334));
    config.SetMinDesiredConnections(10);
    config.SetMaxConnections(50);
    config.SetMaxConnectionsPerAddress(3);
    config.SetMaxKnownAddresses(5000);
    config.SetMaxKnownHashes(10000);
    config.SetSeedList(testSeedList);

    // Verify configuration is valid
    EXPECT_LE(config.GetMinDesiredConnections(), config.GetMaxConnections());
    EXPECT_GT(config.GetMaxConnectionsPerAddress(), 0u);
    EXPECT_GT(config.GetMaxKnownAddresses(), 0u);
    EXPECT_GT(config.GetMaxKnownHashes(), 0u);
    EXPECT_FALSE(config.GetSeedList().empty());
}

TEST_F(ChannelsConfigTest, ConfigurationScenarios)
{
    // Test typical configuration scenarios

    // Scenario 1: Main net configuration
    {
        ChannelsConfig mainnet;
        mainnet.SetTcp(IPEndPoint(IPAddress::Parse("0.0.0.0"), 10333));
        mainnet.SetWebSocket(IPEndPoint(IPAddress::Parse("0.0.0.0"), 10334));
        mainnet.SetMinDesiredConnections(10);
        mainnet.SetMaxConnections(40);
        mainnet.SetMaxConnectionsPerAddress(3);

        EXPECT_EQ(10333u, mainnet.GetTcp().GetPort());
        EXPECT_EQ(10334u, mainnet.GetWebSocket().GetPort());
    }

    // Scenario 2: Test net configuration
    {
        ChannelsConfig testnet;
        testnet.SetTcp(IPEndPoint(IPAddress::Parse("0.0.0.0"), 20333));
        testnet.SetWebSocket(IPEndPoint(IPAddress::Parse("0.0.0.0"), 20334));
        testnet.SetMinDesiredConnections(5);
        testnet.SetMaxConnections(20);
        testnet.SetMaxConnectionsPerAddress(2);

        EXPECT_EQ(20333u, testnet.GetTcp().GetPort());
        EXPECT_EQ(20334u, testnet.GetWebSocket().GetPort());
    }

    // Scenario 3: Private net configuration
    {
        ChannelsConfig privatenet;
        privatenet.SetTcp(IPEndPoint(IPAddress::Parse("127.0.0.1"), 30333));
        privatenet.SetWebSocket(IPEndPoint(IPAddress::Parse("127.0.0.1"), 30334));
        privatenet.SetMinDesiredConnections(1);
        privatenet.SetMaxConnections(5);
        privatenet.SetMaxConnectionsPerAddress(1);

        EXPECT_EQ("127.0.0.1", privatenet.GetTcp().GetAddress().ToString());
        EXPECT_EQ(1u, privatenet.GetMinDesiredConnections());
    }
}

TEST_F(ChannelsConfigTest, UpdateSeedList)
{
    ChannelsConfig config;

    // Set initial seed list
    config.SetSeedList(testSeedList);
    EXPECT_EQ(3u, config.GetSeedList().size());

    // Update with new seed list
    std::vector<IPEndPoint> newSeedList;
    newSeedList.push_back(IPEndPoint(IPAddress::Parse("192.168.1.1"), 10333));
    newSeedList.push_back(IPEndPoint(IPAddress::Parse("192.168.1.2"), 10333));

    config.SetSeedList(newSeedList);
    EXPECT_EQ(2u, config.GetSeedList().size());
    EXPECT_EQ("192.168.1.1", config.GetSeedList()[0].GetAddress().ToString());
}
