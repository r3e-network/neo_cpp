#include <gtest/gtest.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_state.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/recovery_message.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <array>
#include <sstream>
#include <chrono>

using namespace neo::consensus;
using namespace neo::cryptography::ecc;
using namespace neo::io;

class ConsensusMessageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment
    }
};

TEST_F(ConsensusMessageTest, ConsensusMessage)
{
    // Create message
    ConsensusMessage message(ConsensusMessageType::ChangeView);
    message.SetViewNumber(1);
    message.SetValidatorIndex(2);
    message.SetBlockIndex(100);

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ConsensusMessage message2(ConsensusMessageType::ChangeView);
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::ChangeView);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 2u);
    EXPECT_EQ(message2.GetBlockIndex(), 100u);
}

TEST_F(ConsensusMessageTest, ViewChangeMessage)
{
    // Create message
    ViewChangeMessage message;
    message.SetViewNumber(1);
    message.SetNewViewNumber(2);
    message.SetValidatorIndex(3);
    message.SetBlockIndex(100);
    auto timestamp = std::chrono::system_clock::now();
    message.SetTimestamp(timestamp);
    message.SetReason(ChangeViewReason::TxInvalid);

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ViewChangeMessage message2;
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::ChangeView);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 3u);
    EXPECT_EQ(message2.GetNewViewNumber(), 2u);
    EXPECT_EQ(message2.GetReason(), ChangeViewReason::TxInvalid);
    // Note: Timestamp comparison would need tolerance due to serialization precision
}

TEST_F(ConsensusMessageTest, PrepareRequestMessage)
{
    // Create message
    PrepareRequestMessage message;
    message.SetViewNumber(1);
    message.SetValidatorIndex(3);
    message.SetBlockIndex(100);
    message.SetNonce(987654321);
    auto timestamp = std::chrono::system_clock::now();
    message.SetTimestamp(timestamp);
    message.SetTransactionHashes({UInt256::Zero(), UInt256::Zero()});

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareRequestMessage message2;
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 3u);
    EXPECT_EQ(message2.GetNonce(), 987654321u);
    EXPECT_EQ(message2.GetTransactionHashes().size(), 2u);
}

TEST_F(ConsensusMessageTest, PrepareResponseMessage)
{
    // Create message
    UInt256 preparationHash = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    PrepareResponseMessage message;
    message.SetViewNumber(1);
    message.SetValidatorIndex(3);
    message.SetBlockIndex(100);
    message.SetPrepareRequestHash(preparationHash);

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareResponseMessage message2;
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 3u);
    EXPECT_EQ(message2.GetPrepareRequestHash(), preparationHash);
}

TEST_F(ConsensusMessageTest, CommitMessage)
{
    // Create message
    std::vector<uint8_t> commitSignature{1, 2, 3, 4, 5};
    CommitMessage message;
    message.SetViewNumber(1);
    message.SetValidatorIndex(3);
    message.SetBlockIndex(100);
    message.SetSignature(commitSignature);

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    CommitMessage message2;
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::Commit);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 3u);
    EXPECT_EQ(message2.GetSignature(), commitSignature);
}

TEST_F(ConsensusMessageTest, RecoveryRequestMessage)
{
    // Create message
    RecoveryRequestMessage message;
    message.SetViewNumber(1);
    message.SetValidatorIndex(3);
    message.SetBlockIndex(100);

    // Serialize message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();

    // Deserialize message
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    RecoveryRequestMessage message2;
    message2.Deserialize(reader);

    // Check message
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::RecoveryRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1u);
    EXPECT_EQ(message2.GetValidatorIndex(), 3u);
}

TEST_F(ConsensusMessageTest, RecoveryMessageRoundTrip)
{
    RecoveryMessage message(1);
    message.SetViewNumber(1);
    message.SetValidatorIndex(7);
    message.SetBlockIndex(512);

    RecoveryMessage::ChangeViewPayloadCompact changeView;
    changeView.validator_index = 2;
    changeView.original_view_number = 1;
    changeView.timestamp = 123u;
    changeView.invocation_script = ByteVector({0x10, 0x20});
    message.AddChangeViewPayload(changeView);

    auto prepareRequest = std::make_shared<PrepareRequest>();
    prepareRequest->SetViewNumber(1);
    prepareRequest->SetValidatorIndex(3);
    prepareRequest->SetBlockIndex(512);
    prepareRequest->SetNonce(42);
    UInt256 txHash{};
    txHash.Data()[0] = 0x01;
    std::vector<UInt256> hashes{txHash};
    prepareRequest->SetTransactionHashes(hashes);
    message.SetPrepareRequest(prepareRequest);

    RecoveryMessage::PreparationPayloadCompact prepareResponse;
    prepareResponse.validator_index = 4;
    prepareResponse.invocation_script = ByteVector({0x30, 0x31});
    message.AddPreparationPayload(prepareResponse);

    RecoveryMessage::CommitPayloadCompact commit;
    commit.view_number = 1;
    commit.validator_index = 5;
    commit.signature = ByteVector({0xAA, 0xBB});
    commit.invocation_script = ByteVector({0x40, 0x41});
    message.AddCommitPayload(commit);

    neo::network::p2p::payloads::Neo3Transaction transaction;
    transaction.SetVersion(1);
    transaction.SetNonce(123456);
    transaction.SetSystemFee(100);
    transaction.SetNetworkFee(50);
    transaction.SetValidUntilBlock(200);
    neo::io::ByteVector script({0x01, 0x02});
    transaction.SetScript(script);
    message.AddTransaction(transaction);

    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);

    std::istringstream serialized(stream.str());
    BinaryReader reader(serialized);
    RecoveryMessage decoded(0);
    decoded.Deserialize(reader);

    EXPECT_EQ(decoded.GetType(), ConsensusMessageType::RecoveryMessage);
    EXPECT_EQ(decoded.GetViewNumber(), 1u);
    EXPECT_EQ(decoded.GetValidatorIndex(), 7u);
    ASSERT_EQ(decoded.GetChangeViewPayloads().size(), 1u);
    EXPECT_EQ(decoded.GetChangeViewPayloads()[0].validator_index, 2u);
    EXPECT_EQ(decoded.GetChangeViewPayloads()[0].original_view_number, 1u);
    EXPECT_EQ(decoded.GetChangeViewPayloads()[0].invocation_script.Size(), 2u);
    ASSERT_NE(decoded.GetPrepareRequest(), nullptr);
    EXPECT_EQ(decoded.GetPrepareRequest()->GetNonce(), 42u);
    ASSERT_EQ(decoded.GetPreparationPayloads().size(), 1u);
    EXPECT_EQ(decoded.GetPreparationPayloads()[0].validator_index, 4u);
    EXPECT_EQ(decoded.GetPreparationPayloads()[0].invocation_script.Size(), 2u);
    ASSERT_EQ(decoded.GetCommitPayloads().size(), 1u);
    const auto& commitPayload = decoded.GetCommitPayloads()[0];
    EXPECT_EQ(commitPayload.signature.Size(), 2u);
    EXPECT_EQ(commitPayload.signature.Data()[0], 0xAA);
    EXPECT_EQ(commitPayload.signature.Data()[1], 0xBB);
    EXPECT_EQ(commitPayload.invocation_script.Size(), 2u);
    ASSERT_EQ(decoded.GetTransactions().size(), 1u);
    EXPECT_EQ(decoded.GetTransactions()[0].GetNonce(), 123456u);
    EXPECT_EQ(decoded.GetTransactions()[0].GetSystemFee(), 100);
    EXPECT_EQ(decoded.GetTransactions()[0].GetNetworkFee(), 50);
}

TEST_F(ConsensusMessageTest, RecoveryMessagePreservesTransactionOrder)
{
    RecoveryMessage message(3);

    neo::network::p2p::payloads::Neo3Transaction first;
    first.SetNonce(111);
    first.SetSystemFee(10);
    first.SetNetworkFee(5);
    first.SetValidUntilBlock(500);
    first.SetScript(ByteVector({0x01, 0x02}));

    neo::network::p2p::payloads::Neo3Transaction second;
    second.SetNonce(222);
    second.SetSystemFee(20);
    second.SetNetworkFee(6);
    second.SetValidUntilBlock(600);
    second.SetScript(ByteVector({0x03, 0x04}));

    message.SetTransactions({first, second});

    const auto& direct = message.GetTransactions();
    ASSERT_EQ(direct.size(), 2u);
    EXPECT_EQ(direct[0].GetNonce(), first.GetNonce());
    EXPECT_EQ(direct[1].GetNonce(), second.GetNonce());

    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);

    std::istringstream serialized(stream.str());
    BinaryReader reader(serialized);
    RecoveryMessage decoded(0);
    decoded.Deserialize(reader);

    const auto& roundTrip = decoded.GetTransactions();
    ASSERT_EQ(roundTrip.size(), 2u);
    EXPECT_EQ(roundTrip[0].GetNonce(), first.GetNonce());
    EXPECT_EQ(roundTrip[1].GetNonce(), second.GetNonce());
}

TEST_F(ConsensusMessageTest, RecoveryMessagePreparationHashFallback)
{
    RecoveryMessage message(4);

    auto hash = UInt256::Parse("0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    message.SetPreparationHash(hash);

    RecoveryMessage::PreparationPayloadCompact prep;
    prep.validator_index = 1;
    prep.invocation_script = ByteVector({0xAA});
    message.AddPreparationPayload(prep);

    RecoveryMessage::CommitPayloadCompact commit;
    commit.view_number = 4;
    commit.validator_index = 2;
    commit.signature = ByteVector({0x10, 0x11, 0x12});
    commit.invocation_script = ByteVector({0xBB, 0xCC});
    message.AddCommitPayload(commit);

    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);

    std::istringstream serialized(stream.str());
    BinaryReader reader(serialized);
    RecoveryMessage decoded(0);
    decoded.Deserialize(reader);

    EXPECT_EQ(decoded.GetPrepareRequest(), nullptr);
    ASSERT_TRUE(decoded.GetPreparationHash().has_value());
    EXPECT_EQ(decoded.GetPreparationHash().value(), hash);
    ASSERT_EQ(decoded.GetPreparationPayloads().size(), 1u);
    EXPECT_EQ(decoded.GetPreparationPayloads()[0].validator_index, 1u);
    EXPECT_EQ(decoded.GetPreparationPayloads()[0].invocation_script.Size(), 1u);
    ASSERT_EQ(decoded.GetCommitPayloads().size(), 1u);
    EXPECT_EQ(decoded.GetCommitPayloads()[0].validator_index, 2u);
    EXPECT_EQ(decoded.GetCommitPayloads()[0].signature.Size(), 3u);
    EXPECT_EQ(decoded.GetCommitPayloads()[0].invocation_script.Size(), 2u);
}

TEST_F(ConsensusMessageTest, ConsensusStateCachesTransactions)
{
    ConsensusState state;

    neo::network::p2p::payloads::Neo3Transaction tx;
    tx.SetVersion(0);
    tx.SetNonce(4242);
    tx.SetSystemFee(10);
    tx.SetNetworkFee(5);
    tx.SetValidUntilBlock(1000);
    tx.SetScript(ByteVector({0x01, 0x02, 0x03}));

    EXPECT_TRUE(state.AddTransaction(tx));

    auto hash = tx.GetHash();
    auto cached = state.GetCachedTransaction(hash);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->GetNonce(), tx.GetNonce());

    // Duplicate additions should be ignored
    EXPECT_FALSE(state.AddTransaction(tx));

    state.RemoveTransaction(hash);
    EXPECT_FALSE(state.GetCachedTransaction(hash).has_value());
}
