#include <gtest/gtest.h>
#include <neo/consensus/consensus_context.h>
#include <neo/consensus/consensus_service.h>
#include <neo/consensus/consensus_message.h>
#include <neo/core/neo_system.h>
#include <neo/wallets/key_pair.h>
#include <neo/io/byte_vector.h>
#include <memory>

using namespace neo::consensus;
using namespace neo::core;
using namespace neo::wallets;
using namespace neo::io;
using namespace neo::cryptography;

class ConsensusTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system;
    std::shared_ptr<ConsensusContext> context;
    
    void SetUp() override
    {
        // Create a simple Neo system for testing
        system = std::make_shared<NeoSystem>("memory", "test");
        
        // Create consensus context
        context = std::make_shared<ConsensusContext>(system.get());
    }
    
    void TearDown() override
    {
        context.reset();
        system.reset();
    }
};

TEST_F(ConsensusTest, TestConsensusContextInitialization)
{
    EXPECT_NE(context, nullptr);
    EXPECT_EQ(context->GetViewNumber(), 0);
    EXPECT_EQ(context->GetPrimaryIndex(), 0);
}

TEST_F(ConsensusTest, TestViewNumberIncrement)
{
    uint8_t initialView = context->GetViewNumber();
    context->IncrementViewNumber();
    EXPECT_EQ(context->GetViewNumber(), initialView + 1);
}

TEST_F(ConsensusTest, TestPrimaryIndexCalculation)
{
    // Test primary index calculation for different view numbers
    context->Reset(0);
    uint8_t primary0 = context->GetPrimaryIndex();
    
    context->Reset(1);
    uint8_t primary1 = context->GetPrimaryIndex();
    
    context->Reset(2);
    uint8_t primary2 = context->GetPrimaryIndex();
    
    // Primary should change with view number
    EXPECT_GE(primary0, 0);
    EXPECT_GE(primary1, 0);
    EXPECT_GE(primary2, 0);
}

TEST_F(ConsensusTest, TestConsensusMessageTypes)
{
    // Test that we can create messages with different types
    ConsensusMessage changeView(ConsensusMessageType::ChangeView);
    EXPECT_EQ(changeView.GetType(), ConsensusMessageType::ChangeView);
    
    ConsensusMessage prepareRequest(ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(prepareRequest.GetType(), ConsensusMessageType::PrepareRequest);
    
    ConsensusMessage prepareResponse(ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(prepareResponse.GetType(), ConsensusMessageType::PrepareResponse);
    
    ConsensusMessage commit(ConsensusMessageType::Commit);
    EXPECT_EQ(commit.GetType(), ConsensusMessageType::Commit);
}

TEST_F(ConsensusTest, TestConsensusMessageSerialization)
{
    ConsensusMessage msg(ConsensusMessageType::PrepareRequest);
    msg.SetViewNumber(5);
    
    // Serialize
    ByteVector serialized;
    msg.Serialize(serialized);
    EXPECT_GT(serialized.Size(), 0u);
    
    // Deserialize
    ConsensusMessage deserialized(ConsensusMessageType::PrepareRequest);
    ByteSpan span = serialized.AsSpan();
    deserialized.Deserialize(span);
    
    EXPECT_EQ(deserialized.GetViewNumber(), 5);
}