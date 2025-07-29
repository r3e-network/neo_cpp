#include <gtest/gtest.h>
#include <memory>
#include <neo/config/protocol_settings.h>
#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/node/neo_system.h>
#include <neo/persistence/memory_store.h>

using namespace neo::node;
using namespace neo::persistence;
using namespace neo::config;
using namespace neo::network;
using namespace neo::network::p2p;

class UT_NeoSystem : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Create protocol settings
        settings = std::make_unique<ProtocolSettings>();
        settings->SetAddressVersion(0x35);
        settings->SetStandbyCommittee({"03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c"});
        settings->SetValidatorsCount(1);
        settings->SetSeedList({"localhost:20333"});
        settings->SetNetwork(0x4F454E);
        settings->SetMillisecondsPerBlock(15000);
        settings->SetMaxTransactionsPerBlock(512);
        settings->SetMemoryPoolMaxTransactions(50000);
        settings->SetMaxTraceableBlocks(2102400);
        settings->SetInitialGasDistribution(5200000000000000);
        settings->SetNativeUpdateHistory({});

        // Create store provider
        storeProvider = std::make_shared<MemoryStore>();

        // Create NeoSystem
        system = std::make_unique<NeoSystem>(*settings, storeProvider, "");
    }

    std::unique_ptr<ProtocolSettings> settings;
    std::shared_ptr<MemoryStore> storeProvider;
    std::unique_ptr<NeoSystem> system;
};

TEST_F(UT_NeoSystem, TestGetSettings)
{
    EXPECT_EQ(settings->GetAddressVersion(), system->GetSettings().GetAddressVersion());
    EXPECT_EQ(settings->GetStandbyCommittee(), system->GetSettings().GetStandbyCommittee());
    EXPECT_EQ(settings->GetValidatorsCount(), system->GetSettings().GetValidatorsCount());
    EXPECT_EQ(settings->GetSeedList(), system->GetSettings().GetSeedList());
    EXPECT_EQ(settings->GetNetwork(), system->GetSettings().GetNetwork());
    EXPECT_EQ(settings->GetMillisecondsPerBlock(), system->GetSettings().GetMillisecondsPerBlock());
    EXPECT_EQ(settings->GetMaxTransactionsPerBlock(), system->GetSettings().GetMaxTransactionsPerBlock());
    EXPECT_EQ(settings->GetMemoryPoolMaxTransactions(), system->GetSettings().GetMemoryPoolMaxTransactions());
    EXPECT_EQ(settings->GetMaxTraceableBlocks(), system->GetSettings().GetMaxTraceableBlocks());
    EXPECT_EQ(settings->GetInitialGasDistribution(), system->GetSettings().GetInitialGasDistribution());
    EXPECT_EQ(settings->GetNativeUpdateHistory(), system->GetSettings().GetNativeUpdateHistory());
}

TEST_F(UT_NeoSystem, TestGetGenesisBlock)
{
    EXPECT_NE(nullptr, system->GetGenesisBlock());
    EXPECT_EQ(0, system->GetGenesisBlock()->GetHeader()->GetIndex());
}

TEST_F(UT_NeoSystem, TestGetBlockchain)
{
    EXPECT_NE(nullptr, system->GetBlockchain());
}

TEST_F(UT_NeoSystem, TestGetLocalNode)
{
    EXPECT_NE(nullptr, system->GetLocalNode());
}

TEST_F(UT_NeoSystem, TestGetTaskManager)
{
    EXPECT_NE(nullptr, system->GetTaskManager());
}

TEST_F(UT_NeoSystem, TestGetTxRouter)
{
    EXPECT_NE(nullptr, system->GetTxRouter());
}

TEST_F(UT_NeoSystem, TestGetMemPool)
{
    EXPECT_NE(nullptr, system->GetMemPool());
}

TEST_F(UT_NeoSystem, TestGetSnapshot)
{
    EXPECT_NE(nullptr, system->GetSnapshot());
}

TEST_F(UT_NeoSystem, TestGetSnapshotCache)
{
    EXPECT_NE(nullptr, system->GetSnapshotCache());
}

TEST_F(UT_NeoSystem, TestAddService)
{
    // Add a service
    auto service = std::make_shared<int>(42);
    system->AddService(service);

    // Get the service
    auto retrievedService = system->GetService<int>();
    EXPECT_NE(nullptr, retrievedService);
    EXPECT_EQ(42, *retrievedService);
}

TEST_F(UT_NeoSystem, TestStartNode)
{
    // Create channels config
    ChannelsConfig config;
    config.SetTcp(IPEndPoint(IPAddress::Any(), 10333));
    config.SetMinDesiredConnections(10);
    config.SetMaxConnections(20);
    config.SetMaxConnectionsPerAddress(3);
    config.SetMaxKnownAddresses(1000);
    config.SetMaxKnownHashes(1000);
    config.SetSeedList({IPEndPoint(IPAddress::Parse("127.0.0.1"), 20333)});

    // Start the node
    system->StartNode(config);

    // Suspend node startup
    system->SuspendNodeStartup();

    // Resume node startup
    EXPECT_TRUE(system->ResumeNodeStartup());
}
