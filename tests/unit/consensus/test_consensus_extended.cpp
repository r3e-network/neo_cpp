#include <gtest/gtest.h>
#include <neo/consensus/consensus_service.h>
#include <neo/consensus/consensus_context.h>
#include <neo/network/message.h>
#include <memory>

using namespace neo::consensus;
using namespace neo::network;

class ConsensusServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<ConsensusService> service;
    std::unique_ptr<ConsensusContext> context;
    
    void SetUp() override
    {
        context = std::make_unique<ConsensusContext>();
        service = std::make_unique<ConsensusService>(context.get());
    }
};

TEST_F(ConsensusServiceTest, TestConsensusServiceCreation)
{
    EXPECT_NE(service, nullptr);
    EXPECT_NE(context, nullptr);
    EXPECT_EQ(service->GetState(), ConsensusState::Initial);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceStart)
{
    EXPECT_TRUE(service->Start());
    EXPECT_EQ(service->GetState(), ConsensusState::Running);
    
    // Should not start twice
    EXPECT_FALSE(service->Start());
}

TEST_F(ConsensusServiceTest, TestConsensusServiceReceivesBlockchainMessages)
{
    service->Start();
    
    // Create a mock blockchain message
    auto message = std::make_shared<Message>(MessageType::Block);
    EXPECT_TRUE(service->ProcessMessage(message));
    
    // Verify message was processed
    EXPECT_GT(service->GetProcessedMessageCount(), 0);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceHandlesExtensiblePayload)
{
    service->Start();
    
    // Create extensible payload
    ExtensiblePayload payload;
    payload.Category = "dBFT";
    payload.ValidBlockStart = 0;
    payload.ValidBlockEnd = 100;
    
    EXPECT_TRUE(service->ProcessExtensiblePayload(payload));
}

TEST_F(ConsensusServiceTest, TestConsensusServiceHandlesValidConsensusMessage)
{
    service->Start();
    
    // Create valid consensus message
    ConsensusMessage msg;
    msg.Type = ConsensusMessageType::PrepareRequest;
    msg.ViewNumber = 0;
    
    EXPECT_TRUE(service->ProcessConsensusMessage(msg));
    EXPECT_EQ(context->GetViewNumber(), msg.ViewNumber);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceRejectsInvalidPayload)
{
    service->Start();
    
    // Create invalid payload (wrong category)
    ExtensiblePayload payload;
    payload.Category = "Invalid";
    
    EXPECT_FALSE(service->ProcessExtensiblePayload(payload));
    
    // Create expired payload
    ExtensiblePayload expiredPayload;
    expiredPayload.Category = "dBFT";
    expiredPayload.ValidBlockEnd = 0; // Expired
    
    EXPECT_FALSE(service->ProcessExtensiblePayload(expiredPayload));
}
