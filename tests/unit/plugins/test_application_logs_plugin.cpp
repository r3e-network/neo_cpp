#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/istore.h>
#include <neo/plugins/application_logs_plugin.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/vm_types.h>
#include <neo/vm/stack_item.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::plugins
{
class ApplicationLogsPluginTestHelper
{
   public:
    static void Handle(ApplicationLogsPlugin& plugin, const std::shared_ptr<neo::ledger::Block>& block,
                       const std::vector<neo::ledger::ApplicationExecuted>& executions)
    {
        plugin.HandleCommitting(block, executions);
    }
};
}  // namespace neo::plugins

class ApplicationLogsPluginTest : public ::testing::Test
{
  protected:
    std::shared_ptr<neo::node::NeoSystem> neoSystem_;
    std::shared_ptr<neo::rpc::RpcServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;
    std::string tempDir_;

    void SetUp() override
    {
        // Create temporary directory
        tempDir_ = std::filesystem::temp_directory_path().string() + "/neo_test_logs";
        std::filesystem::create_directories(tempDir_);

        // Create neo system
        auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
        protocolSettings->SetNetwork(0x334F454E);
        neoSystem_ = std::make_shared<neo::node::NeoSystem>(protocolSettings, "memory");

        // Create RPC server with config
        neo::rpc::RpcConfig config;
        config.port = 10332;
        config.enable_cors = true;
        rpcServer_ = std::make_shared<neo::rpc::RpcServer>(config);
    }

    void TearDown() override
    {
        rpcServer_.reset();
        if (neoSystem_)
        {
            neoSystem_->Stop();
        }
        neoSystem_.reset();

        // Remove temporary directory
        std::filesystem::remove_all(tempDir_);
    }
};

TEST_F(ApplicationLogsPluginTest, Constructor)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    EXPECT_EQ(plugin->GetName(), "ApplicationLogs");
    EXPECT_EQ(plugin->GetDescription(), "Provides application logs functionality");
    EXPECT_EQ(plugin->GetVersion(), "1.0");
    EXPECT_EQ(plugin->GetAuthor(), "Neo C++ Team");
    EXPECT_FALSE(plugin->IsRunning());
}

TEST_F(ApplicationLogsPluginTest, Initialize)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();

    // Initialize plugin
    bool result = plugin->Initialize(neoSystem_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin->IsRunning());
}

TEST_F(ApplicationLogsPluginTest, InitializeWithSettings)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

    // Initialize plugin
    bool result = plugin->Initialize(neoSystem_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin->IsRunning());
}

TEST_F(ApplicationLogsPluginTest, StartStop)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

    // Initialize plugin
    plugin->Initialize(neoSystem_, settings);

    ASSERT_TRUE(neoSystem_->Start());
    // Start plugin
    bool result1 = plugin->Start();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(plugin->IsRunning());

    // Stop plugin
    bool result2 = plugin->Stop();
    EXPECT_TRUE(result2);
    EXPECT_FALSE(plugin->IsRunning());
}

TEST_F(ApplicationLogsPluginTest, GetApplicationLog)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

    // Initialize plugin
    plugin->Initialize(neoSystem_, settings);

    // Add and retrieve application log
    neo::io::UInt256 txHash =
        neo::io::UInt256::Parse("0x2146ce05715f5006b88c68b715fdd4d4a96b24508afc297b256760c2f4b3d6c1");
    auto logEntry = std::make_shared<neo::plugins::ApplicationLog>();
    logEntry->TxHash = txHash;
    neo::plugins::ApplicationLog::Execution execution;
    execution.Trigger = neo::smartcontract::TriggerType::Application;
    execution.VmState = neo::vm::VMState::Halt;
    execution.GasConsumed = 123;
    execution.Exception.clear();
    logEntry->Executions.push_back(execution);
    plugin->AddLog(logEntry);

    auto log = plugin->GetApplicationLog(txHash);
    ASSERT_NE(log, nullptr);
    ASSERT_TRUE(log->TxHash.has_value());
    EXPECT_EQ(*log->TxHash, txHash);
    ASSERT_EQ(log->Executions.size(), 1u);
    EXPECT_EQ(log->Executions.front().GasConsumed, 123);

    // No start/stop required for manual cache operations in this test
}

TEST_F(ApplicationLogsPluginTest, HandleCommittingStoresLogs)
{
    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}, {"MaxCachedLogs", "2"}};
    plugin->Initialize(neoSystem_, settings);
    ASSERT_TRUE(neoSystem_->Start());
    plugin->Start();

    auto block = std::make_shared<neo::ledger::Block>();
    block->SetVersion(0);
    block->SetIndex(1);
    block->SetTimestamp(123456789);
    block->SetPreviousHash(neo::io::UInt256::Zero());
    block->SetMerkleRoot(neo::io::UInt256::Zero());
    block->SetNonce(42);
    block->SetNextConsensus(neo::io::UInt160::Parse("0x0000000000000000000000000000000000000000"));

    auto tx = std::make_shared<neo::ledger::Transaction>();
    tx->SetVersion(0);
    tx->SetNonce(999);
    tx->SetSystemFee(1);
    tx->SetNetworkFee(1);
    tx->SetValidUntilBlock(100);
    neo::io::ByteVector script;
    script.Push(0x01);
    tx->SetScript(script);
    block->AddTransaction(*tx);

    auto snapshot = neoSystem_->GetDataCache();

    auto onPersistEngineUnique = neo::smartcontract::ApplicationEngine::Create(
        neo::smartcontract::TriggerType::OnPersist, nullptr, snapshot, block.get(), 0);
    auto onPersistEngine = std::shared_ptr<neo::smartcontract::ApplicationEngine>(std::move(onPersistEngineUnique));

    neo::ledger::ApplicationExecuted onPersistExec;
    onPersistExec.transaction = nullptr;
    onPersistExec.engine = onPersistEngine;
    onPersistExec.vm_state = neo::vm::VMState::Halt;
    onPersistExec.gas_consumed = 0;
    onPersistExec.exception_message = "";
    neo::smartcontract::NotifyEntry onPersistNotify;
    onPersistNotify.script_hash = neo::io::UInt160::Parse("0x0102030405060708090a0b0c0d0e0f1011121314");
    onPersistNotify.event_name = "OnPersistEvent";
    onPersistNotify.state = {neo::vm::StackItem::Create(true)};
    onPersistExec.notifications.push_back(onPersistNotify);

    auto txEngineUnique = neo::smartcontract::ApplicationEngine::Create(
        neo::smartcontract::TriggerType::Application, tx.get(), snapshot, block.get(), tx->GetSystemFee());
    auto txEngine = std::shared_ptr<neo::smartcontract::ApplicationEngine>(std::move(txEngineUnique));

    neo::ledger::ApplicationExecuted txExec;
    txExec.transaction = tx;
    txExec.engine = txEngine;
    txExec.vm_state = neo::vm::VMState::Halt;
    txExec.gas_consumed = 5;
    txExec.exception_message = "";
    neo::smartcontract::NotifyEntry txNotify;
    txNotify.script_hash = neo::io::UInt160::Parse("0x02030405060708090a0b0c0d0e0f101112131415");
    txNotify.event_name = "TxEvent";
    txNotify.state = {neo::vm::StackItem::Create(static_cast<int64_t>(7))};
    txExec.notifications.push_back(txNotify);

    auto postPersistEngineUnique = neo::smartcontract::ApplicationEngine::Create(
        neo::smartcontract::TriggerType::PostPersist, nullptr, snapshot, block.get(), 0);
    auto postPersistEngine = std::shared_ptr<neo::smartcontract::ApplicationEngine>(std::move(postPersistEngineUnique));

    neo::ledger::ApplicationExecuted postPersistExec;
    postPersistExec.transaction = nullptr;
    postPersistExec.engine = postPersistEngine;
    postPersistExec.vm_state = neo::vm::VMState::Halt;
    postPersistExec.gas_consumed = 0;
    postPersistExec.exception_message = "";

    std::vector<neo::ledger::ApplicationExecuted> executions = {onPersistExec, txExec, postPersistExec};
    neo::plugins::ApplicationLogsPluginTestHelper::Handle(*plugin, block, executions);

    const auto txHash = tx->GetHash();
    auto txLog = plugin->GetApplicationLog(txHash);
    ASSERT_NE(txLog, nullptr);
    ASSERT_TRUE(txLog->TxHash.has_value());
    EXPECT_EQ(*txLog->TxHash, txHash);
    ASSERT_TRUE(txLog->BlockHash.has_value());
    EXPECT_EQ(*txLog->BlockHash, block->GetHash());
    ASSERT_EQ(txLog->Executions.size(), 1u);
    EXPECT_EQ(txLog->Executions[0].Trigger, neo::smartcontract::TriggerType::Application);
    ASSERT_EQ(txLog->Executions[0].Notifications.size(), 1u);
    const auto& notifState = txLog->Executions[0].Notifications[0].State;
    ASSERT_TRUE(notifState.is_object());
    EXPECT_EQ(notifState.at("type"), "Array");
    ASSERT_TRUE(notifState.at("value").is_array());
    ASSERT_EQ(notifState.at("value").size(), 1u);
    EXPECT_EQ(notifState.at("value")[0].at("type"), "Integer");
    EXPECT_EQ(notifState.at("value")[0].at("value"), "7");

    auto blockLog = plugin->GetApplicationLog(block->GetHash());
    ASSERT_NE(blockLog, nullptr);
    EXPECT_FALSE(blockLog->TxHash.has_value());
    ASSERT_TRUE(blockLog->BlockHash.has_value());
    EXPECT_EQ(*blockLog->BlockHash, block->GetHash());
    ASSERT_EQ(blockLog->Executions.size(), 2u);
    EXPECT_EQ(blockLog->Executions[0].Trigger, neo::smartcontract::TriggerType::OnPersist);
    EXPECT_EQ(blockLog->Executions[1].Trigger, neo::smartcontract::TriggerType::PostPersist);

    // Adding an extra transaction log should prune the oldest cached entry (the original tx)
    const auto txHash2 = neo::io::UInt256::Parse("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    auto extraLog = std::make_shared<neo::plugins::ApplicationLog>();
    extraLog->TxHash = txHash2;
    plugin->AddLog(extraLog);

    EXPECT_EQ(plugin->GetApplicationLog(txHash), nullptr);
    EXPECT_NE(plugin->GetApplicationLog(txHash2), nullptr);
    EXPECT_NE(plugin->GetApplicationLog(block->GetHash()), nullptr);

    plugin->Stop();
}

// Factory test removed - ApplicationLogsPluginFactory is not yet implemented
