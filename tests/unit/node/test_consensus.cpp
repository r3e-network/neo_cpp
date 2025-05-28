#include <gtest/gtest.h>
#include <neo/node/consensus.h>
#include <neo/node/node.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <sstream>

using namespace neo::node;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::io;
using namespace neo::cryptography::ecc;

TEST(ConsensusMessageTest, Constructor)
{
    // Default constructor
    ConsensusMessage message1;
    EXPECT_EQ(message1.GetType(), ConsensusMessageType::ChangeView);
    EXPECT_EQ(message1.GetViewNumber(), 0);
    
    // Type constructor
    ConsensusMessage message2(ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message2.GetViewNumber(), 0);
}

TEST(ConsensusMessageTest, SettersAndGetters)
{
    ConsensusMessage message;
    
    // Type
    message.SetType(ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(message.GetType(), ConsensusMessageType::PrepareResponse);
    
    // View number
    message.SetViewNumber(1);
    EXPECT_EQ(message.GetViewNumber(), 1);
}

TEST(ConsensusMessageTest, Serialization)
{
    // Create message
    ConsensusMessage message;
    message.SetType(ConsensusMessageType::PrepareResponse);
    message.SetViewNumber(1);
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ConsensusMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(message2.GetViewNumber(), 1);
}

TEST(ChangeViewMessageTest, Constructor)
{
    ChangeViewMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::ChangeView);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_EQ(message.GetNewViewNumber(), 0);
    EXPECT_EQ(message.GetTimestamp(), 0);
}

TEST(ChangeViewMessageTest, SettersAndGetters)
{
    ChangeViewMessage message;
    
    // New view number
    message.SetNewViewNumber(2);
    EXPECT_EQ(message.GetNewViewNumber(), 2);
    
    // Timestamp
    message.SetTimestamp(123456789);
    EXPECT_EQ(message.GetTimestamp(), 123456789);
}

TEST(ChangeViewMessageTest, Serialization)
{
    // Create message
    ChangeViewMessage message;
    message.SetViewNumber(1);
    message.SetNewViewNumber(2);
    message.SetTimestamp(123456789);
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ChangeViewMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::ChangeView);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetNewViewNumber(), 2);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
}

TEST(PrepareRequestMessageTest, Constructor)
{
    PrepareRequestMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_EQ(message.GetTimestamp(), 0);
    EXPECT_EQ(message.GetNonce(), 0);
    EXPECT_EQ(message.GetNextConsensus(), UInt160());
    EXPECT_TRUE(message.GetTransactions().empty());
    EXPECT_TRUE(message.GetInvocationScript().IsEmpty());
}

TEST(PrepareRequestMessageTest, SettersAndGetters)
{
    PrepareRequestMessage message;
    
    // Timestamp
    message.SetTimestamp(123456789);
    EXPECT_EQ(message.GetTimestamp(), 123456789);
    
    // Nonce
    message.SetNonce(987654321);
    EXPECT_EQ(message.GetNonce(), 987654321);
    
    // Next consensus
    UInt160 nextConsensus = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    message.SetNextConsensus(nextConsensus);
    EXPECT_EQ(message.GetNextConsensus(), nextConsensus);
    
    // Transactions
    auto tx1 = std::make_shared<Transaction>();
    tx1->SetVersion(0);
    auto tx2 = std::make_shared<Transaction>();
    tx2->SetVersion(1);
    std::vector<std::shared_ptr<Transaction>> transactions = {tx1, tx2};
    message.SetTransactions(transactions);
    EXPECT_EQ(message.GetTransactions().size(), 2);
    EXPECT_EQ(message.GetTransactions()[0]->GetVersion(), 0);
    EXPECT_EQ(message.GetTransactions()[1]->GetVersion(), 1);
    
    // Invocation script
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    message.SetInvocationScript(invocationScript);
    EXPECT_EQ(message.GetInvocationScript(), invocationScript);
}

TEST(PrepareRequestMessageTest, Serialization)
{
    // Create message
    PrepareRequestMessage message;
    message.SetViewNumber(1);
    message.SetTimestamp(123456789);
    message.SetNonce(987654321);
    message.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    
    auto tx1 = std::make_shared<Transaction>();
    tx1->SetVersion(0);
    auto tx2 = std::make_shared<Transaction>();
    tx2->SetVersion(1);
    std::vector<std::shared_ptr<Transaction>> transactions = {tx1, tx2};
    message.SetTransactions(transactions);
    
    message.SetInvocationScript(ByteVector::Parse("0102030405"));
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareRequestMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
    EXPECT_EQ(message2.GetNonce(), 987654321);
    EXPECT_EQ(message2.GetNextConsensus(), UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    EXPECT_EQ(message2.GetTransactions().size(), 2);
    EXPECT_EQ(message2.GetTransactions()[0]->GetVersion(), 0);
    EXPECT_EQ(message2.GetTransactions()[1]->GetVersion(), 1);
    EXPECT_EQ(message2.GetInvocationScript(), ByteVector::Parse("0102030405"));
}

TEST(PrepareResponseMessageTest, Constructor)
{
    PrepareResponseMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_TRUE(message.GetInvocationScript().IsEmpty());
}

TEST(PrepareResponseMessageTest, SettersAndGetters)
{
    PrepareResponseMessage message;
    
    // Invocation script
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    message.SetInvocationScript(invocationScript);
    EXPECT_EQ(message.GetInvocationScript(), invocationScript);
}

TEST(PrepareResponseMessageTest, Serialization)
{
    // Create message
    PrepareResponseMessage message;
    message.SetViewNumber(1);
    message.SetInvocationScript(ByteVector::Parse("0102030405"));
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    PrepareResponseMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::PrepareResponse);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetInvocationScript(), ByteVector::Parse("0102030405"));
}

TEST(CommitMessageTest, Constructor)
{
    CommitMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::Commit);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_TRUE(message.GetSignature().IsEmpty());
}

TEST(CommitMessageTest, SettersAndGetters)
{
    CommitMessage message;
    
    // Signature
    ByteVector signature = ByteVector::Parse("0102030405");
    message.SetSignature(signature);
    EXPECT_EQ(message.GetSignature(), signature);
}

TEST(CommitMessageTest, Serialization)
{
    // Create message
    CommitMessage message;
    message.SetViewNumber(1);
    message.SetSignature(ByteVector::Parse("0102030405"));
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    CommitMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::Commit);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetSignature(), ByteVector::Parse("0102030405"));
}

TEST(RecoveryRequestMessageTest, Constructor)
{
    RecoveryRequestMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::RecoveryRequest);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_EQ(message.GetTimestamp(), 0);
}

TEST(RecoveryRequestMessageTest, SettersAndGetters)
{
    RecoveryRequestMessage message;
    
    // Timestamp
    message.SetTimestamp(123456789);
    EXPECT_EQ(message.GetTimestamp(), 123456789);
}

TEST(RecoveryRequestMessageTest, Serialization)
{
    // Create message
    RecoveryRequestMessage message;
    message.SetViewNumber(1);
    message.SetTimestamp(123456789);
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    RecoveryRequestMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::RecoveryRequest);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetTimestamp(), 123456789);
}

TEST(RecoveryMessageTest, Constructor)
{
    RecoveryMessage message;
    EXPECT_EQ(message.GetType(), ConsensusMessageType::RecoveryMessage);
    EXPECT_EQ(message.GetViewNumber(), 0);
    EXPECT_TRUE(message.GetChangeViewMessages().empty());
    EXPECT_EQ(message.GetPrepareRequestMessage(), nullptr);
    EXPECT_TRUE(message.GetPrepareResponseMessages().empty());
    EXPECT_TRUE(message.GetCommitMessages().empty());
}

TEST(RecoveryMessageTest, SettersAndGetters)
{
    RecoveryMessage message;
    
    // Change view messages
    auto changeViewMessage1 = std::make_shared<ChangeViewMessage>();
    changeViewMessage1->SetViewNumber(1);
    auto changeViewMessage2 = std::make_shared<ChangeViewMessage>();
    changeViewMessage2->SetViewNumber(2);
    std::vector<std::shared_ptr<ChangeViewMessage>> changeViewMessages = {changeViewMessage1, changeViewMessage2};
    message.SetChangeViewMessages(changeViewMessages);
    EXPECT_EQ(message.GetChangeViewMessages().size(), 2);
    EXPECT_EQ(message.GetChangeViewMessages()[0]->GetViewNumber(), 1);
    EXPECT_EQ(message.GetChangeViewMessages()[1]->GetViewNumber(), 2);
    
    // Prepare request message
    auto prepareRequestMessage = std::make_shared<PrepareRequestMessage>();
    prepareRequestMessage->SetViewNumber(3);
    message.SetPrepareRequestMessage(prepareRequestMessage);
    EXPECT_EQ(message.GetPrepareRequestMessage()->GetViewNumber(), 3);
    
    // Prepare response messages
    auto prepareResponseMessage1 = std::make_shared<PrepareResponseMessage>();
    prepareResponseMessage1->SetViewNumber(4);
    auto prepareResponseMessage2 = std::make_shared<PrepareResponseMessage>();
    prepareResponseMessage2->SetViewNumber(5);
    std::vector<std::shared_ptr<PrepareResponseMessage>> prepareResponseMessages = {prepareResponseMessage1, prepareResponseMessage2};
    message.SetPrepareResponseMessages(prepareResponseMessages);
    EXPECT_EQ(message.GetPrepareResponseMessages().size(), 2);
    EXPECT_EQ(message.GetPrepareResponseMessages()[0]->GetViewNumber(), 4);
    EXPECT_EQ(message.GetPrepareResponseMessages()[1]->GetViewNumber(), 5);
    
    // Commit messages
    auto commitMessage1 = std::make_shared<CommitMessage>();
    commitMessage1->SetViewNumber(6);
    auto commitMessage2 = std::make_shared<CommitMessage>();
    commitMessage2->SetViewNumber(7);
    std::vector<std::shared_ptr<CommitMessage>> commitMessages = {commitMessage1, commitMessage2};
    message.SetCommitMessages(commitMessages);
    EXPECT_EQ(message.GetCommitMessages().size(), 2);
    EXPECT_EQ(message.GetCommitMessages()[0]->GetViewNumber(), 6);
    EXPECT_EQ(message.GetCommitMessages()[1]->GetViewNumber(), 7);
}

TEST(RecoveryMessageTest, Serialization)
{
    // Create message
    RecoveryMessage message;
    message.SetViewNumber(1);
    
    auto changeViewMessage1 = std::make_shared<ChangeViewMessage>();
    changeViewMessage1->SetViewNumber(1);
    auto changeViewMessage2 = std::make_shared<ChangeViewMessage>();
    changeViewMessage2->SetViewNumber(2);
    std::vector<std::shared_ptr<ChangeViewMessage>> changeViewMessages = {changeViewMessage1, changeViewMessage2};
    message.SetChangeViewMessages(changeViewMessages);
    
    auto prepareRequestMessage = std::make_shared<PrepareRequestMessage>();
    prepareRequestMessage->SetViewNumber(3);
    message.SetPrepareRequestMessage(prepareRequestMessage);
    
    auto prepareResponseMessage1 = std::make_shared<PrepareResponseMessage>();
    prepareResponseMessage1->SetViewNumber(4);
    auto prepareResponseMessage2 = std::make_shared<PrepareResponseMessage>();
    prepareResponseMessage2->SetViewNumber(5);
    std::vector<std::shared_ptr<PrepareResponseMessage>> prepareResponseMessages = {prepareResponseMessage1, prepareResponseMessage2};
    message.SetPrepareResponseMessages(prepareResponseMessages);
    
    auto commitMessage1 = std::make_shared<CommitMessage>();
    commitMessage1->SetViewNumber(6);
    auto commitMessage2 = std::make_shared<CommitMessage>();
    commitMessage2->SetViewNumber(7);
    std::vector<std::shared_ptr<CommitMessage>> commitMessages = {commitMessage1, commitMessage2};
    message.SetCommitMessages(commitMessages);
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    RecoveryMessage message2;
    message2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(message2.GetType(), ConsensusMessageType::RecoveryMessage);
    EXPECT_EQ(message2.GetViewNumber(), 1);
    EXPECT_EQ(message2.GetChangeViewMessages().size(), 2);
    EXPECT_EQ(message2.GetChangeViewMessages()[0]->GetViewNumber(), 1);
    EXPECT_EQ(message2.GetChangeViewMessages()[1]->GetViewNumber(), 2);
    EXPECT_NE(message2.GetPrepareRequestMessage(), nullptr);
    EXPECT_EQ(message2.GetPrepareRequestMessage()->GetViewNumber(), 3);
    EXPECT_EQ(message2.GetPrepareResponseMessages().size(), 2);
    EXPECT_EQ(message2.GetPrepareResponseMessages()[0]->GetViewNumber(), 4);
    EXPECT_EQ(message2.GetPrepareResponseMessages()[1]->GetViewNumber(), 5);
    EXPECT_EQ(message2.GetCommitMessages().size(), 2);
    EXPECT_EQ(message2.GetCommitMessages()[0]->GetViewNumber(), 6);
    EXPECT_EQ(message2.GetCommitMessages()[1]->GetViewNumber(), 7);
}

class ConsensusServiceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create store provider
        auto store = std::make_shared<MemoryStore>();
        storeProvider_ = std::make_shared<StoreProvider>(store);
        
        // Create settings
        std::unordered_map<std::string, std::string> settings;
        settings["P2PPort"] = "10333";
        settings["RPCPort"] = "10332";
        settings["MemoryPoolCapacity"] = "50000";
        
        // Create node
        node_ = std::make_shared<Node>(storeProvider_, settings);
        
        // Create key pair
        keyPair_ = Secp256r1::GenerateKeyPair();
        
        // Create consensus service
        consensusService_ = std::make_shared<ConsensusService>(node_, keyPair_);
    }

    std::shared_ptr<StoreProvider> storeProvider_;
    std::shared_ptr<Node> node_;
    KeyPair keyPair_;
    std::shared_ptr<ConsensusService> consensusService_;
};

TEST_F(ConsensusServiceTest, Constructor)
{
    EXPECT_EQ(consensusService_->GetNode(), node_);
    EXPECT_EQ(consensusService_->GetKeyPair().PrivateKey, keyPair_.PrivateKey);
    EXPECT_EQ(consensusService_->GetKeyPair().PublicKey, keyPair_.PublicKey);
    EXPECT_FALSE(consensusService_->IsRunning());
}

TEST_F(ConsensusServiceTest, StartStop)
{
    // Start consensus service
    consensusService_->Start();
    EXPECT_TRUE(consensusService_->IsRunning());
    
    // Stop consensus service
    consensusService_->Stop();
    EXPECT_FALSE(consensusService_->IsRunning());
}
