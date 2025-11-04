/**
 * @file test_consensus_manual_control.cpp
 * @brief Integration coverage for manual consensus lifecycle controls.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <neo/consensus/consensus_service.h>
#include <neo/core/configuration_manager.h>
#include <neo/core/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/neo_system.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/network/p2p/local_node.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/rpc/error_codes.h>
#include <memory>
#include <string>

namespace neo::tests::integration
{
class ManualConsensusControlTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        nodeProtocolSettings_ = std::make_shared<neo::ProtocolSettings>();
        ledgerSystem_ = std::make_shared<neo::ledger::NeoSystem>(nodeProtocolSettings_);
        ledgerSystem_->Start();

        blockchain_ = ledgerSystem_->GetBlockchain();
        memoryPool_ = ledgerSystem_->GetMemoryPool();

        coreSettings_ = std::make_shared<core::ProtocolSettings>();

        consensusService_ = std::make_shared<consensus::ConsensusService>(coreSettings_, blockchain_, memoryPool_);
        consensusService_->SetAutoStartEnabled(false);

        auto& config = core::ConfigurationManager::GetInstance().GetConsensusConfig();
        config.auto_start = false;

        auto& localNode = network::p2p::LocalNode::GetInstance();
        localNode.SetConsensusService(consensusService_);
        localNode.ForceRunningStateForTesting(true);
    }

    void TearDown() override
    {
        auto& localNode = network::p2p::LocalNode::GetInstance();
        localNode.ForceRunningStateForTesting(false);
        localNode.SetConsensusService(nullptr);

        if (consensusService_)
        {
            consensusService_->Stop();
            consensusService_.reset();
        }

        if (ledgerSystem_)
        {
            ledgerSystem_->Stop();
            ledgerSystem_->Dispose();
            ledgerSystem_.reset();
        }

        blockchain_.reset();
        memoryPool_.reset();
        nodeProtocolSettings_.reset();
        coreSettings_.reset();

        auto& config = core::ConfigurationManager::GetInstance().GetConsensusConfig();
        config.auto_start = false;
    }

    std::shared_ptr<neo::ProtocolSettings> nodeProtocolSettings_;
    std::shared_ptr<neo::ledger::NeoSystem> ledgerSystem_;
    std::shared_ptr<neo::ledger::Blockchain> blockchain_;
    std::shared_ptr<neo::ledger::MemoryPool> memoryPool_;
    std::shared_ptr<core::ProtocolSettings> coreSettings_;
    std::shared_ptr<consensus::ConsensusService> consensusService_;
};

TEST_F(ManualConsensusControlTest, RpcStartStopRestartLifecycle)
{
    auto params = nlohmann::json::array();

    auto startResult = rpc::RPCMethods::StartConsensus(nullptr, params);
    ASSERT_TRUE(startResult.is_boolean());
    EXPECT_TRUE(startResult.get<bool>());
    EXPECT_TRUE(consensusService_->IsRunning());

    auto stopResult = rpc::RPCMethods::StopConsensus(nullptr, params);
    ASSERT_TRUE(stopResult.is_boolean());
    EXPECT_TRUE(stopResult.get<bool>());
    EXPECT_FALSE(consensusService_->IsRunning());

    auto restartResult = rpc::RPCMethods::RestartConsensus(nullptr, params);
    ASSERT_TRUE(restartResult.is_boolean());
    EXPECT_TRUE(restartResult.get<bool>());
    EXPECT_TRUE(consensusService_->IsRunning());
}

TEST_F(ManualConsensusControlTest, RpcStartConsensusFailsWhenLocalNodeOffline)
{
    auto& localNode = network::p2p::LocalNode::GetInstance();
    localNode.ForceRunningStateForTesting(false);
    consensusService_->Stop();

    EXPECT_THROW(rpc::RPCMethods::StartConsensus(nullptr, nlohmann::json::array()), rpc::RpcException);

    localNode.ForceRunningStateForTesting(true);
}

TEST_F(ManualConsensusControlTest, AutoStartFlagReflectsConfiguration)
{
    auto& config = core::ConfigurationManager::GetInstance().GetConsensusConfig();

    config.auto_start = true;
    consensusService_->SetAutoStartEnabled(true);

    EXPECT_TRUE(config.auto_start);
    EXPECT_TRUE(consensusService_->IsAutoStartEnabled());

    config.auto_start = false;
    consensusService_->SetAutoStartEnabled(false);

    EXPECT_FALSE(config.auto_start);
    EXPECT_FALSE(consensusService_->IsAutoStartEnabled());
}
}  // namespace neo::tests::integration
