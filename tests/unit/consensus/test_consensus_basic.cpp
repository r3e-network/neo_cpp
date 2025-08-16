#include <gtest/gtest.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/change_view.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/commit.h>
#include <neo/io/byte_vector.h>

using namespace neo::consensus;
using namespace neo::io;

class ConsensusBasicTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

TEST_F(ConsensusBasicTest, TestConsensusMessageTypes)
{
    // Test different consensus message types
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::ChangeView), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::PrepareRequest), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::PrepareResponse), 0x21);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::Commit), 0x30);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::RecoveryRequest), 0x40);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::RecoveryMessage), 0x41);
}

TEST_F(ConsensusBasicTest, TestChangeViewCreation)
{
    ChangeView cv;
    cv.SetViewNumber(1);
    cv.SetTimestamp(1000);
    
    EXPECT_EQ(cv.GetViewNumber(), 1);
    EXPECT_EQ(cv.GetTimestamp(), 1000);
}

TEST_F(ConsensusBasicTest, TestPrepareRequestCreation)
{
    PrepareRequest pr;
    pr.SetViewNumber(2);
    pr.SetTimestamp(2000);
    pr.SetNonce(12345);
    
    EXPECT_EQ(pr.GetViewNumber(), 2);
    EXPECT_EQ(pr.GetTimestamp(), 2000);
    EXPECT_EQ(pr.GetNonce(), 12345);
}

TEST_F(ConsensusBasicTest, TestPrepareResponseCreation)
{
    PrepareResponse pr;
    pr.SetViewNumber(3);
    
    UInt256 hash;
    hash.fill(0xFF);
    pr.SetPreparationHash(hash);
    
    EXPECT_EQ(pr.GetViewNumber(), 3);
    EXPECT_EQ(pr.GetPreparationHash(), hash);
}

TEST_F(ConsensusBasicTest, TestCommitCreation)
{
    Commit commit;
    commit.SetViewNumber(4);
    
    ByteVector signature(64, 0xAB);
    commit.SetSignature(signature);
    
    EXPECT_EQ(commit.GetViewNumber(), 4);
    EXPECT_EQ(commit.GetSignature().Size(), 64);
}

TEST_F(ConsensusBasicTest, TestMessageSerialization)
{
    // Test that messages can be serialized
    ChangeView cv;
    cv.SetViewNumber(5);
    cv.SetTimestamp(5000);
    
    ByteVector data;
    cv.Serialize(data);
    
    EXPECT_GT(data.Size(), 0u);
    
    // Basic size check - ChangeView should have view number + timestamp
    EXPECT_GE(data.Size(), sizeof(uint8_t) + sizeof(uint64_t));
}

TEST_F(ConsensusBasicTest, TestViewNumberBounds)
{
    ChangeView cv;
    
    // Test min and max view numbers
    cv.SetViewNumber(0);
    EXPECT_EQ(cv.GetViewNumber(), 0);
    
    cv.SetViewNumber(255);
    EXPECT_EQ(cv.GetViewNumber(), 255);
}

TEST_F(ConsensusBasicTest, TestConsensusConstants)
{
    // Test consensus-related constants
    EXPECT_GT(ConsensusContext::MaxValidators, 0u);
    EXPECT_LE(ConsensusContext::MaxValidators, 1024u);
}