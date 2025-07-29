#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/consensus_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/merkle_block_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
// #include <neo/network/payload_factory.h>  // PayloadFactory not implemented
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/network/payload_type.h>
#include <sstream>
#include <vector>

using namespace neo::network;
using namespace neo::network::payloads;
using namespace neo::io;

TEST(NetworkPayloadsTest, TransactionPayload_Serialize_Deserialize)
{
    // Create a transaction payload with test data
    TransactionPayload payload;
    payload.SetTransaction(ledger::Transaction());

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    TransactionPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetPayloadType(), PayloadType::Transaction);

    // Create a payload via the factory
    std::istringstream factoryStream(data);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::Transaction);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::Transaction);
    auto castedPayload = dynamic_cast<TransactionPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
}

TEST(NetworkPayloadsTest, BlockPayload_Serialize_Deserialize)
{
    // Create a block payload with test data
    BlockPayload payload;
    payload.SetBlock(ledger::Block());

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    BlockPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetPayloadType(), PayloadType::Block);

    // Create a payload via the factory
    std::istringstream factoryStream(data);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::Block);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::Block);
    auto castedPayload = dynamic_cast<BlockPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
}

TEST(NetworkPayloadsTest, ConsensusPayload_Serialize_Deserialize)
{
    // Create a consensus payload with test data
    ConsensusPayload payload;
    ByteVector testData = ByteVector::Parse("0102030405");
    payload.SetConsensusData(testData);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    ConsensusPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetPayloadType(), PayloadType::Consensus);
    EXPECT_EQ(deserializedPayload.GetConsensusData(), testData);

    // Create a payload via the factory
    std::istringstream factoryStream(data);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::Consensus);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::Consensus);
    auto castedPayload = dynamic_cast<ConsensusPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
    EXPECT_EQ(castedPayload->GetConsensusData(), testData);
}

TEST(NetworkPayloadsTest, FilterLoadPayload_Serialize_Deserialize)
{
    // Create a filter load payload with test data
    FilterLoadPayload payload;
    ByteVector filter = ByteVector::Parse("0102030405");
    payload.SetFilter(filter);
    payload.SetK(10);
    payload.SetTweak(12345);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    FilterLoadPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetPayloadType(), PayloadType::FilterLoad);
    EXPECT_EQ(deserializedPayload.GetFilter(), filter);
    EXPECT_EQ(deserializedPayload.GetK(), 10);
    EXPECT_EQ(deserializedPayload.GetTweak(), 12345);

    // Create a payload via the factory
    std::istringstream factoryStream(data);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::FilterLoad);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::FilterLoad);
    auto castedPayload = dynamic_cast<FilterLoadPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
    EXPECT_EQ(castedPayload->GetFilter(), filter);
    EXPECT_EQ(castedPayload->GetK(), 10);
    EXPECT_EQ(castedPayload->GetTweak(), 12345);
}

TEST(NetworkPayloadsTest, FilterAddPayload_Serialize_Deserialize)
{
    // Create a filter add payload with test data
    FilterAddPayload payload;
    ByteVector data = ByteVector::Parse("0102030405");
    payload.SetData(data);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string serializedData = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(serializedData);
    BinaryReader reader(inputStream);
    FilterAddPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetPayloadType(), PayloadType::FilterAdd);
    EXPECT_EQ(deserializedPayload.GetData(), data);

    // Create a payload via the factory
    std::istringstream factoryStream(serializedData);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::FilterAdd);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::FilterAdd);
    auto castedPayload = dynamic_cast<FilterAddPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
    EXPECT_EQ(castedPayload->GetData(), data);
}

TEST(NetworkPayloadsTest, MerkleBlockPayload_Serialize_Deserialize)
{
    // Create a merkle block payload with test data
    MerkleBlockPayload payload;
    payload.SetBlock(ledger::Block());

    std::vector<uint8_t> flags = {1, 0, 1, 0, 1};
    ByteVector flagsVector(flags.data(), flags.size());
    payload.SetFlags(flagsVector);

    std::vector<neo::io::UInt256> hashes;
    hashes.push_back(neo::io::UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000001"));
    hashes.push_back(neo::io::UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000002"));
    payload.SetHashes(hashes);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string serializedData = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(serializedData);
    BinaryReader reader(inputStream);
    MerkleBlockPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    // Note: GetPayloadType() is not available in the current API
    EXPECT_EQ(deserializedPayload.GetFlags(), flagsVector);
    EXPECT_EQ(deserializedPayload.GetHashes().size(), 2);
    EXPECT_EQ(deserializedPayload.GetHashes()[0], hashes[0]);
    EXPECT_EQ(deserializedPayload.GetHashes()[1], hashes[1]);

    /*
    // Create a payload via the factory
    std::istringstream factoryStream(serializedData);
    BinaryReader factoryReader(factoryStream);
    auto factoryPayload = PayloadFactory::Create(PayloadType::MerkleBlock);
    factoryPayload->Deserialize(factoryReader);

    // Verify the factory created payload is of the correct type
    EXPECT_EQ(factoryPayload->GetPayloadType(), PayloadType::MerkleBlock);
    auto castedPayload = dynamic_cast<MerkleBlockPayload*>(factoryPayload.get());
    EXPECT_NE(castedPayload, nullptr);
    EXPECT_EQ(castedPayload->GetFlags(), flagsVector);
    EXPECT_EQ(castedPayload->GetHashes().size(), 2);
    EXPECT_EQ(castedPayload->GetHashes()[0], hashes[0]);
    EXPECT_EQ(castedPayload->GetHashes()[1], hashes[1]);
    */
}

/*
TEST(NetworkPayloadsTest, PayloadFactory_Create_All_Types)
{
    // Test creation of all payload types
    auto versionPayload = PayloadFactory::Create(PayloadType::Version);
    EXPECT_NE(versionPayload, nullptr);
    EXPECT_EQ(versionPayload->GetPayloadType(), PayloadType::Version);

    auto addrPayload = PayloadFactory::Create(PayloadType::Addr);
    EXPECT_NE(addrPayload, nullptr);
    EXPECT_EQ(addrPayload->GetPayloadType(), PayloadType::Addr);

    auto invPayload = PayloadFactory::Create(PayloadType::Inventory);
    EXPECT_NE(invPayload, nullptr);
    EXPECT_EQ(invPayload->GetPayloadType(), PayloadType::Inventory);

    auto getDataPayload = PayloadFactory::Create(PayloadType::GetData);
    EXPECT_NE(getDataPayload, nullptr);
    EXPECT_EQ(getDataPayload->GetPayloadType(), PayloadType::GetData);

    auto getBlocksPayload = PayloadFactory::Create(PayloadType::GetBlocks);
    EXPECT_NE(getBlocksPayload, nullptr);
    EXPECT_EQ(getBlocksPayload->GetPayloadType(), PayloadType::GetBlocks);

    auto headersPayload = PayloadFactory::Create(PayloadType::Headers);
    EXPECT_NE(headersPayload, nullptr);
    EXPECT_EQ(headersPayload->GetPayloadType(), PayloadType::Headers);

    auto pingPayload = PayloadFactory::Create(PayloadType::Ping);
    EXPECT_NE(pingPayload, nullptr);
    EXPECT_EQ(pingPayload->GetPayloadType(), PayloadType::Ping);

    auto pongPayload = PayloadFactory::Create(PayloadType::Pong);
    EXPECT_NE(pongPayload, nullptr);
    EXPECT_EQ(pongPayload->GetPayloadType(), PayloadType::Pong);

    auto getAddrPayload = PayloadFactory::Create(PayloadType::GetAddr);
    EXPECT_NE(getAddrPayload, nullptr);
    EXPECT_EQ(getAddrPayload->GetPayloadType(), PayloadType::GetAddr);

    auto blockPayload = PayloadFactory::Create(PayloadType::Block);
    EXPECT_NE(blockPayload, nullptr);
    EXPECT_EQ(blockPayload->GetPayloadType(), PayloadType::Block);

    auto transactionPayload = PayloadFactory::Create(PayloadType::Transaction);
    EXPECT_NE(transactionPayload, nullptr);
    EXPECT_EQ(transactionPayload->GetPayloadType(), PayloadType::Transaction);

    auto consensusPayload = PayloadFactory::Create(PayloadType::Consensus);
    EXPECT_NE(consensusPayload, nullptr);
    EXPECT_EQ(consensusPayload->GetPayloadType(), PayloadType::Consensus);

    auto filterLoadPayload = PayloadFactory::Create(PayloadType::FilterLoad);
    EXPECT_NE(filterLoadPayload, nullptr);
    EXPECT_EQ(filterLoadPayload->GetPayloadType(), PayloadType::FilterLoad);

    auto filterAddPayload = PayloadFactory::Create(PayloadType::FilterAdd);
    EXPECT_NE(filterAddPayload, nullptr);
    EXPECT_EQ(filterAddPayload->GetPayloadType(), PayloadType::FilterAdd);

    auto filterClearPayload = PayloadFactory::Create(PayloadType::FilterClear);
    EXPECT_NE(filterClearPayload, nullptr);
    EXPECT_EQ(filterClearPayload->GetPayloadType(), PayloadType::FilterClear);

    auto merkleBlockPayload = PayloadFactory::Create(PayloadType::MerkleBlock);
    EXPECT_NE(merkleBlockPayload, nullptr);
    EXPECT_EQ(merkleBlockPayload->GetPayloadType(), PayloadType::MerkleBlock);

    // Test invalid payload type
    EXPECT_THROW(PayloadFactory::Create(static_cast<PayloadType>(999)), std::invalid_argument);
}
*/
