#include <gtest/gtest.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::consensus;
using namespace neo::cryptography::ecc;
using namespace neo::io;

class ConsensusMessageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        keyPair_ = KeyPair::Create();
    }

    KeyPair keyPair_;
};

TEST_F(ConsensusMessageTest, ConsensusMessage)
{
    // Create message
    ConsensusMessage message(MessageType::ChangeView, 1);
    message.SetValidatorIndex(2);
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ConsensusMessage message2(MessageType::ChangeView, 0);
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::ChangeView);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 2);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, ChangeViewMessage)
{
    // Create message
    ChangeViewMessage message(1, 2, 123456789);
    message.SetValidatorIndex(3);
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ChangeViewMessage message2(0, 0, 0);
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::ChangeView);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetNewViewNumber(), 2);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, PrepareRequest)
{
    // Create message
    UInt160 nextConsensus;
    nextConsensus.FromString("0x1234567890abcdef1234567890abcdef12345678");
    PrepareRequest message(1, 123456789, 987654321, nextConsensus);
    message.SetValidatorIndex(3);
    message.SetTransactionHashes({UInt256(), UInt256()});
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareRequest message2(0, 0, 0, UInt160());
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::PrepareRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
    EXPECT_EQ(message2.GetNonce(), 987654321);
    EXPECT_EQ(message2.GetNextConsensus(), nextConsensus);
    EXPECT_EQ(message2.GetTransactionHashes().size(), 2);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, PrepareResponse)
{
    // Create message
    UInt256 preparationHash;
    preparationHash.FromString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    PrepareResponse message(1, preparationHash);
    message.SetValidatorIndex(3);
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareResponse message2(0, UInt256());
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::PrepareResponse);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetPreparationHash(), preparationHash);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, CommitMessage)
{
    // Create message
    UInt256 commitHash;
    commitHash.FromString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    ByteVector commitSignature{1, 2, 3, 4, 5};
    CommitMessage message(1, commitHash, commitSignature);
    message.SetValidatorIndex(3);
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    CommitMessage message2(0, UInt256(), ByteVector());
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::Commit);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetCommitHash(), commitHash);
    EXPECT_EQ(message2.GetCommitSignature(), commitSignature);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, RecoveryRequest)
{
    // Create message
    RecoveryRequest message(1, 123456789);
    message.SetValidatorIndex(3);
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    RecoveryRequest message2(0, 0);
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::RecoveryRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));
}

TEST_F(ConsensusMessageTest, RecoveryMessage)
{
    // Create message
    RecoveryMessage message(1);
    message.SetValidatorIndex(3);

    // Add change view message
    auto changeViewMessage = std::make_shared<ChangeViewMessage>(1, 2, 123456789);
    changeViewMessage->SetValidatorIndex(4);
    changeViewMessage->Sign(keyPair_);
    message.AddChangeViewMessage(changeViewMessage);

    // Add prepare request
    UInt160 nextConsensus;
    nextConsensus.FromString("0x1234567890abcdef1234567890abcdef12345678");
    auto prepareRequest = std::make_shared<PrepareRequest>(1, 123456789, 987654321, nextConsensus);
    prepareRequest->SetValidatorIndex(5);
    prepareRequest->SetTransactionHashes({UInt256(), UInt256()});
    prepareRequest->Sign(keyPair_);
    message.SetPrepareRequest(prepareRequest);

    // Add prepare response
    UInt256 preparationHash;
    preparationHash.FromString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    auto prepareResponse = std::make_shared<PrepareResponse>(1, preparationHash);
    prepareResponse->SetValidatorIndex(6);
    prepareResponse->Sign(keyPair_);
    message.AddPrepareResponse(prepareResponse);

    // Add commit message
    UInt256 commitHash;
    commitHash.FromString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    ByteVector commitSignature{1, 2, 3, 4, 5};
    auto commitMessage = std::make_shared<CommitMessage>(1, commitHash, commitSignature);
    commitMessage->SetValidatorIndex(7);
    commitMessage->Sign(keyPair_);
    message.AddCommitMessage(commitMessage);

    // Sign message
    message.Sign(keyPair_);

    // Verify signature
    EXPECT_TRUE(message.VerifySignature(keyPair_.GetPublicKey()));

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    RecoveryMessage message2(0);
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), MessageType::RecoveryMessage);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetValidatorIndex(), 3);
    EXPECT_EQ(message2.GetSignature(), message.GetSignature());
    EXPECT_TRUE(message2.VerifySignature(keyPair_.GetPublicKey()));

    // Check change view messages
    EXPECT_EQ(message2.GetChangeViewMessages().size(), 1);
    EXPECT_EQ(message2.GetChangeViewMessages()[0]->GetValidatorIndex(), 4);
    EXPECT_EQ(message2.GetChangeViewMessages()[0]->GetNewViewNumber(), 2);
    EXPECT_EQ(message2.GetChangeViewMessages()[0]->GetTimestamp(), 123456789);
    EXPECT_TRUE(message2.GetChangeViewMessages()[0]->VerifySignature(keyPair_.GetPublicKey()));

    // Check prepare request
    EXPECT_NE(message2.GetPrepareRequest(), nullptr);
    EXPECT_EQ(message2.GetPrepareRequest()->GetValidatorIndex(), 5);
    EXPECT_EQ(message2.GetPrepareRequest()->GetTimestamp(), 123456789);
    EXPECT_EQ(message2.GetPrepareRequest()->GetNonce(), 987654321);
    EXPECT_EQ(message2.GetPrepareRequest()->GetNextConsensus(), nextConsensus);
    EXPECT_EQ(message2.GetPrepareRequest()->GetTransactionHashes().size(), 2);
    EXPECT_TRUE(message2.GetPrepareRequest()->VerifySignature(keyPair_.GetPublicKey()));

    // Check prepare responses
    EXPECT_EQ(message2.GetPrepareResponses().size(), 1);
    EXPECT_EQ(message2.GetPrepareResponses()[0]->GetValidatorIndex(), 6);
    EXPECT_EQ(message2.GetPrepareResponses()[0]->GetPreparationHash(), preparationHash);
    EXPECT_TRUE(message2.GetPrepareResponses()[0]->VerifySignature(keyPair_.GetPublicKey()));

    // Check commit messages
    EXPECT_EQ(message2.GetCommitMessages().size(), 1);
    EXPECT_EQ(message2.GetCommitMessages()[0]->GetValidatorIndex(), 7);
    EXPECT_EQ(message2.GetCommitMessages()[0]->GetCommitHash(), commitHash);
    EXPECT_EQ(message2.GetCommitMessages()[0]->GetCommitSignature(), commitSignature);
    EXPECT_TRUE(message2.GetCommitMessages()[0]->VerifySignature(keyPair_.GetPublicKey()));
}
