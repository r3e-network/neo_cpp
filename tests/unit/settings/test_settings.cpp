#include <gtest/gtest.h>

#include <neo/settings.h>

#include <string>

namespace
{
std::string BuildConfigWithApplicationSection()
{
    return R"JSON(
    {
        "ApplicationConfiguration": {
            "DataPath": "./custom-data",
            "P2P": {
                "Port": 20333,
                "BindAddress": "127.0.0.1",
                "MinDesiredConnections": 8,
                "MaxConnections": 32,
                "MaxConnectionsPerAddress": 2,
                "EnableCompression": false,
                "SeedNodes": [
                    "node1.neo.org:20333",
                    "seed-without-port"
                ]
            },
            "RPC": {
                "Enabled": true,
                "Port": 20332,
                "BindAddress": "0.0.0.0",
                "MaxConcurrentConnections": 64,
                "EnableCors": true
            },
            "Storage": {
                "Engine": "RocksDB",
                "Path": "Data_LevelDB_{0}"
            },
            "Logging": {
                "Level": "debug",
                "Path": "Logs_{0}",
                "ConsoleOutput": false
            }
        },
        "ProtocolConfiguration": {
            "Network": 894710606,
            "AddressVersion": 53,
            "SeedList": [
                "proto1.neo.org:20333",
                "proto2.neo.org:20333"
            ]
        }
    }
    )JSON";
}

std::string BuildConfigWithLegacySections()
{
    return R"JSON(
    {
        "Storage": {
            "Engine": "Memory",
            "Path": "./legacy-data"
        },
        "RPC": {
            "Enabled": true,
            "Port": 40000,
            "BindAddress": "127.0.0.1"
        },
        "P2P": {
            "Port": 40001,
            "Seeds": ["legacy.seed:40001"],
            "EnableCompression": false
        }
    }
    )JSON";
}

std::string BuildCompactNetworkConfiguration()
{
    return R"JSON(
    {
        "network": {
            "p2p": {
                "Port": 45001,
                "BindAddress": "::",
                "MinDesiredConnections": 5,
                "MaxConnections": 10
            },
            "rpc": {
                "Enabled": false,
                "Port": 45002
            }
        },
        "ProtocolConfiguration": {
            "Network": 777
        }
    }
    )JSON";
}
}  // namespace

TEST(SettingsLoadTest, ParsesApplicationConfigurationOverrides)
{
    auto settings = neo::Settings::LoadFromJson(BuildConfigWithApplicationSection());

    EXPECT_EQ(settings.P2P.Port, 20333);
    EXPECT_EQ(settings.P2P.BindAddress, "127.0.0.1");
    EXPECT_EQ(settings.P2P.MinDesiredConnections, 8);
    EXPECT_EQ(settings.P2P.MaxConnections, 32);
    EXPECT_EQ(settings.P2P.MaxConnectionsPerAddress, 2);
    EXPECT_FALSE(settings.P2P.EnableCompression);
    ASSERT_EQ(settings.P2P.Seeds.size(), 2u);
    EXPECT_EQ(settings.P2P.Seeds[0], "node1.neo.org:20333");
    EXPECT_EQ(settings.P2P.Seeds[1], "seed-without-port");

    EXPECT_TRUE(settings.RPC.Enabled);
    EXPECT_EQ(settings.RPC.Port, 20332);
    EXPECT_EQ(settings.RPC.BindAddress, "0.0.0.0");
    EXPECT_EQ(settings.RPC.MaxConnections, 64);
    EXPECT_TRUE(settings.RPC.EnableCors);

    EXPECT_EQ(settings.Storage.Engine, "RocksDB");
    EXPECT_EQ(settings.Storage.Path, "Data_LevelDB_3554334E");
    EXPECT_EQ(settings.Application.LogPath, "Logs_3554334E");
    EXPECT_FALSE(settings.Application.LogToConsole);
    EXPECT_EQ(settings.Application.DataPath, "./custom-data");

    ASSERT_NE(settings.Protocol, nullptr);
    EXPECT_EQ(settings.Protocol->GetNetwork(), 894710606U);
    EXPECT_EQ(settings.Protocol->GetAddressVersion(), 53);
    const auto& protocolSeeds = settings.Protocol->GetSeedList();
    ASSERT_EQ(protocolSeeds.size(), 2u);
    EXPECT_EQ(protocolSeeds[0], "proto1.neo.org:20333");
}

TEST(SettingsLoadTest, ParsesLegacySections)
{
    auto settings = neo::Settings::LoadFromJson(BuildConfigWithLegacySections());

    EXPECT_EQ(settings.Storage.Engine, "Memory");
    EXPECT_EQ(settings.Storage.Path, "./legacy-data");
    EXPECT_TRUE(settings.RPC.Enabled);
    EXPECT_EQ(settings.RPC.Port, 40000);
    EXPECT_EQ(settings.RPC.BindAddress, "127.0.0.1");
    EXPECT_EQ(settings.P2P.Port, 40001);
    EXPECT_FALSE(settings.P2P.EnableCompression);
    ASSERT_EQ(settings.P2P.Seeds.size(), 1u);
    EXPECT_EQ(settings.P2P.Seeds[0], "legacy.seed:40001");
}

TEST(SettingsLoadTest, ParsesCompactNetworkConfiguration)
{
    auto settings = neo::Settings::LoadFromJson(BuildCompactNetworkConfiguration());

    EXPECT_EQ(settings.P2P.Port, 45001);
    EXPECT_EQ(settings.P2P.BindAddress, "::");
    EXPECT_EQ(settings.P2P.MinDesiredConnections, 5);
    EXPECT_EQ(settings.P2P.MaxConnections, 10);
    EXPECT_FALSE(settings.RPC.Enabled);
    EXPECT_EQ(settings.RPC.Port, 45002);
    ASSERT_NE(settings.Protocol, nullptr);
    EXPECT_EQ(settings.Protocol->GetNetwork(), 777U);
}
