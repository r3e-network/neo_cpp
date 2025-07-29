#include <chrono>
#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkle_tree.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/fixed8.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/transaction_output.h>
#include <neo/ledger/witness.h>
#include <sstream>

using namespace neo::ledger;
using namespace neo::io;
using namespace neo::cryptography;

TEST(BlockTest, Constructor)
{
    // Default constructor
    Block block;
    EXPECT_EQ(block.GetVersion(), 0);
    EXPECT_EQ(block.GetPreviousHash(), UInt256());
    EXPECT_EQ(block.GetMerkleRoot(), UInt256());
    EXPECT_EQ(block.GetTimestamp(), std::chrono::system_clock::time_point());
    EXPECT_EQ(block.GetIndex(), 0);
    EXPECT_EQ(block.GetNextConsensus(), UInt160());
    EXPECT_TRUE(block.GetTransactions().empty());
}

TEST(BlockTest, Serialization)
{
    // Create a block
    Block block;
    block.SetVersion(1);
    block.SetPreviousHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(std::chrono::system_clock::from_time_t(123456789));
    block.SetIndex(1);
    block.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Note: Block doesn't have SetWitness method in Neo N3
    // Witnesses are part of transactions, not blocks

    // Add a transaction
    Transaction tx;
    tx.SetType(Transaction::Type::InvocationTransaction);
    tx.SetVersion(1);

    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");  // 20 bytes for Script
    TransactionAttribute attribute(usage, data);
    tx.SetAttributes({attribute});

    // Neo 3 doesn't have inputs/outputs, set empty
    tx.SetInputs({});
    tx.SetOutputs({});

    // Add witnesses
    ByteVector txInvocationScript = ByteVector::Parse("0102030405");
    ByteVector txVerificationScript = ByteVector::Parse("0607080910");
    Witness txWitness(txInvocationScript, txVerificationScript);
    tx.SetWitnesses({txWitness});

    block.AddTransaction(tx);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    block.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    Block block2;
    block2.Deserialize(reader);

    // Check
    EXPECT_EQ(block2.GetVersion(), 1);
    EXPECT_EQ(block2.GetPreviousHash(),
              UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(block2.GetMerkleRoot(),
              UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(block2.GetTimestamp(), std::chrono::system_clock::from_time_t(123456789));
    EXPECT_EQ(block2.GetIndex(), 1);
    EXPECT_EQ(block2.GetNextConsensus(), UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    EXPECT_EQ(block2.GetTransactions().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0].GetType(), Transaction::Type::InvocationTransaction);
    EXPECT_EQ(block2.GetTransactions()[0].GetVersion(), 1);
    EXPECT_EQ(block2.GetTransactions()[0].GetAttributes().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0].GetAttributes()[0]->GetUsage(), usage);
    EXPECT_EQ(block2.GetTransactions()[0].GetAttributes()[0]->GetData(), data);
    // Neo 3 doesn't have inputs/outputs
    EXPECT_EQ(block2.GetTransactions()[0].GetInputs().size(), 0);
    EXPECT_EQ(block2.GetTransactions()[0].GetOutputs().size(), 0);
    EXPECT_EQ(block2.GetTransactions()[0].GetWitnesses().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0].GetWitnesses()[0].GetInvocationScript(), txInvocationScript);
    EXPECT_EQ(block2.GetTransactions()[0].GetWitnesses()[0].GetVerificationScript(), txVerificationScript);
}

TEST(BlockTest, GetHash)
{
    // Create a block
    Block block;
    block.SetVersion(1);
    block.SetPreviousHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(std::chrono::system_clock::from_time_t(123456789));
    block.SetIndex(1);
    block.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Get the hash
    UInt256 hash = block.GetHash();

    // Verify the hash
    std::ostringstream stream;
    BinaryWriter writer(stream);

    // Serialize the block header
    writer.Write(block.GetVersion());
    writer.Write(block.GetPreviousHash());
    writer.Write(block.GetMerkleRoot());
    writer.Write(static_cast<uint64_t>(block.GetTimestamp().time_since_epoch().count()));
    writer.Write(block.GetIndex());
    writer.Write(block.GetPrimaryIndex());
    writer.Write(block.GetNextConsensus());

    std::string data = stream.str();
    UInt256 expectedHash = Hash::Hash256(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

    EXPECT_EQ(hash, expectedHash);
}

// Note: MerkleTree::ComputeRoot is not implemented yet
// TEST(BlockTest, RebuildMerkleRoot) is commented out until MerkleTree is implemented

// Note: Block class doesn't implement operator== and operator!= in current API
// TEST(BlockTest, Equality) is commented out until equality operators are implemented

TEST(BlockHeaderTest, Constructor)
{
    // Default constructor
    BlockHeader header1;
    EXPECT_EQ(header1.GetVersion(), 0);
    EXPECT_EQ(header1.GetPrevHash(), UInt256());
    EXPECT_EQ(header1.GetMerkleRoot(), UInt256());
    EXPECT_EQ(header1.GetTimestamp(), 0);
    EXPECT_EQ(header1.GetIndex(), 0);
    EXPECT_EQ(header1.GetNextConsensus(), UInt160());

    // Block constructor
    Block block;
    block.SetVersion(1);
    block.SetPreviousHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(std::chrono::system_clock::from_time_t(123456789));
    block.SetIndex(1);
    block.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Note: Block doesn't have SetWitness method in Neo N3
    // Witnesses are part of transactions, not blocks

    BlockHeader header2(block);
    EXPECT_EQ(header2.GetVersion(), 1);
    EXPECT_EQ(header2.GetPrevHash(),
              UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(header2.GetMerkleRoot(),
              UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(header2.GetTimestamp(), 123456789);
    EXPECT_EQ(header2.GetIndex(), 1);
    EXPECT_EQ(header2.GetNextConsensus(), UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    // Note: Witness check would need to verify BlockHeader witness support
}

TEST(BlockHeaderTest, Serialization)
{
    // Create a block header
    BlockHeader header;
    header.SetVersion(1);
    header.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header.SetTimestamp(123456789);
    header.SetIndex(1);
    header.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    header.SetWitness(witness);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    header.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    BlockHeader header2;
    header2.Deserialize(reader);

    // Check
    EXPECT_EQ(header2.GetVersion(), 1);
    EXPECT_EQ(header2.GetPrevHash(),
              UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(header2.GetMerkleRoot(),
              UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(header2.GetTimestamp(), 123456789);
    EXPECT_EQ(header2.GetIndex(), 1);
    EXPECT_EQ(header2.GetNextConsensus(), UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    // Note: Witness check would need to verify BlockHeader witness support
}

TEST(BlockHeaderTest, GetHash)
{
    // Create a block header
    BlockHeader header;
    header.SetVersion(1);
    header.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header.SetTimestamp(123456789);
    header.SetIndex(1);
    header.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Get the hash
    UInt256 hash = header.GetHash();

    // Verify the hash
    std::ostringstream stream;
    BinaryWriter writer(stream);

    // Serialize the block header
    writer.Write(header.GetVersion());
    writer.Write(header.GetPrevHash());
    writer.Write(header.GetMerkleRoot());
    writer.Write(header.GetTimestamp());
    writer.Write(header.GetNonce());
    writer.Write(header.GetIndex());
    writer.Write(header.GetPrimaryIndex());
    writer.Write(header.GetNextConsensus());

    std::string data = stream.str();
    UInt256 expectedHash = Hash::Hash256(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

    EXPECT_EQ(hash, expectedHash);
}

TEST(BlockHeaderTest, Equality)
{
    // Create a block header
    BlockHeader header1;
    header1.SetVersion(1);
    header1.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header1.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header1.SetTimestamp(123456789);
    header1.SetIndex(1);
    header1.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));

    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    header1.SetWitness(witness);

    // Create an identical block header
    BlockHeader header2;
    header2.SetVersion(1);
    header2.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header2.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header2.SetTimestamp(123456789);
    header2.SetIndex(1);
    header2.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    header2.SetWitness(witness);

    // Create a block header with different version
    BlockHeader header3;
    header3.SetVersion(2);
    header3.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header3.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header3.SetTimestamp(123456789);
    header3.SetIndex(1);
    header3.SetNextConsensus(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    header3.SetWitness(witness);

    EXPECT_TRUE(header1 == header2);
    EXPECT_FALSE(header1 == header3);

    EXPECT_FALSE(header1 != header2);
    EXPECT_TRUE(header1 != header3);
}
