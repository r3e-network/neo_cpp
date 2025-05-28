#include <gtest/gtest.h>
#include <neo/ledger/block.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <sstream>

using namespace neo::ledger;
using namespace neo::io;

TEST(BlockTest, Constructor)
{
    // Default constructor
    Block block;
    EXPECT_EQ(block.GetVersion(), 0);
    EXPECT_EQ(block.GetPrevHash(), UInt256());
    EXPECT_EQ(block.GetMerkleRoot(), UInt256());
    EXPECT_EQ(block.GetTimestamp(), 0);
    EXPECT_EQ(block.GetIndex(), 0);
    EXPECT_EQ(block.GetNextConsensus(), 0);
    EXPECT_TRUE(block.GetTransactions().empty());
}

TEST(BlockTest, Serialization)
{
    // Create a block
    Block block;
    block.SetVersion(1);
    block.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(123456789);
    block.SetIndex(1);
    block.SetNextConsensus(987654321);
    
    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    block.SetWitness(witness);
    
    // Add a transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetType(Transaction::Type::InvocationTransaction);
    tx->SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute(usage, data);
    tx->SetAttributes({attribute});
    
    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference input(prevHash, prevIndex);
    tx->SetInputs({input});
    
    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    tx->SetOutputs({output});
    
    // Add witnesses
    ByteVector txInvocationScript = ByteVector::Parse("0102030405");
    ByteVector txVerificationScript = ByteVector::Parse("0607080910");
    Witness txWitness(txInvocationScript, txVerificationScript);
    tx->SetWitnesses({txWitness});
    
    block.SetTransactions({tx});
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    block.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    Block block2;
    block2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(block2.GetVersion(), 1);
    EXPECT_EQ(block2.GetPrevHash(), UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(block2.GetMerkleRoot(), UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(block2.GetTimestamp(), 123456789);
    EXPECT_EQ(block2.GetIndex(), 1);
    EXPECT_EQ(block2.GetNextConsensus(), 987654321);
    EXPECT_EQ(block2.GetWitness().GetInvocationScript(), invocationScript);
    EXPECT_EQ(block2.GetWitness().GetVerificationScript(), verificationScript);
    EXPECT_EQ(block2.GetTransactions().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetType(), Transaction::Type::InvocationTransaction);
    EXPECT_EQ(block2.GetTransactions()[0]->GetVersion(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetAttributes().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetAttributes()[0].GetUsage(), usage);
    EXPECT_EQ(block2.GetTransactions()[0]->GetAttributes()[0].GetData(), data);
    EXPECT_EQ(block2.GetTransactions()[0]->GetInputs().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetInputs()[0].GetPrevHash(), prevHash);
    EXPECT_EQ(block2.GetTransactions()[0]->GetInputs()[0].GetPrevIndex(), prevIndex);
    EXPECT_EQ(block2.GetTransactions()[0]->GetOutputs().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetOutputs()[0].GetAssetId(), assetId);
    EXPECT_EQ(block2.GetTransactions()[0]->GetOutputs()[0].GetValue(), value);
    EXPECT_EQ(block2.GetTransactions()[0]->GetOutputs()[0].GetScriptHash(), scriptHash);
    EXPECT_EQ(block2.GetTransactions()[0]->GetWitnesses().size(), 1);
    EXPECT_EQ(block2.GetTransactions()[0]->GetWitnesses()[0].GetInvocationScript(), txInvocationScript);
    EXPECT_EQ(block2.GetTransactions()[0]->GetWitnesses()[0].GetVerificationScript(), txVerificationScript);
}

TEST(BlockTest, GetHash)
{
    // Create a block
    Block block;
    block.SetVersion(1);
    block.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(123456789);
    block.SetIndex(1);
    block.SetNextConsensus(987654321);
    
    // Get the hash
    UInt256 hash = block.GetHash();
    
    // Verify the hash
    std::ostringstream stream;
    BinaryWriter writer(stream);
    
    // Serialize the block header
    writer.Write(block.GetVersion());
    writer.Write(block.GetPrevHash());
    writer.Write(block.GetMerkleRoot());
    writer.Write(block.GetTimestamp());
    writer.Write(block.GetIndex());
    writer.Write(block.GetNextConsensus());
    
    std::string data = stream.str();
    UInt256 expectedHash = neo::cryptography::Hash::Sha256(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    
    EXPECT_EQ(hash, expectedHash);
}

TEST(BlockTest, RebuildMerkleRoot)
{
    // Create a block
    Block block;
    
    // Add a transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetType(Transaction::Type::InvocationTransaction);
    tx->SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute(usage, data);
    tx->SetAttributes({attribute});
    
    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference input(prevHash, prevIndex);
    tx->SetInputs({input});
    
    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    tx->SetOutputs({output});
    
    // Add witnesses
    ByteVector txInvocationScript = ByteVector::Parse("0102030405");
    ByteVector txVerificationScript = ByteVector::Parse("0607080910");
    Witness txWitness(txInvocationScript, txVerificationScript);
    tx->SetWitnesses({txWitness});
    
    block.SetTransactions({tx});
    
    // Rebuild the merkle root
    block.RebuildMerkleRoot();
    
    // Verify the merkle root
    std::vector<UInt256> hashes = {tx->GetHash()};
    auto expectedRoot = neo::cryptography::MerkleTree::ComputeRoot(hashes);
    
    EXPECT_TRUE(expectedRoot.has_value());
    EXPECT_EQ(block.GetMerkleRoot(), *expectedRoot);
}

TEST(BlockTest, Equality)
{
    // Create a block
    Block block1;
    block1.SetVersion(1);
    block1.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block1.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block1.SetTimestamp(123456789);
    block1.SetIndex(1);
    block1.SetNextConsensus(987654321);
    
    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    block1.SetWitness(witness);
    
    // Add a transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetType(Transaction::Type::InvocationTransaction);
    tx->SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute(usage, data);
    tx->SetAttributes({attribute});
    
    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference input(prevHash, prevIndex);
    tx->SetInputs({input});
    
    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    tx->SetOutputs({output});
    
    // Add witnesses
    ByteVector txInvocationScript = ByteVector::Parse("0102030405");
    ByteVector txVerificationScript = ByteVector::Parse("0607080910");
    Witness txWitness(txInvocationScript, txVerificationScript);
    tx->SetWitnesses({txWitness});
    
    block1.SetTransactions({tx});
    
    // Create an identical block
    Block block2;
    block2.SetVersion(1);
    block2.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block2.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block2.SetTimestamp(123456789);
    block2.SetIndex(1);
    block2.SetNextConsensus(987654321);
    block2.SetWitness(witness);
    block2.SetTransactions({tx});
    
    // Create a block with different version
    Block block3;
    block3.SetVersion(2);
    block3.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block3.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block3.SetTimestamp(123456789);
    block3.SetIndex(1);
    block3.SetNextConsensus(987654321);
    block3.SetWitness(witness);
    block3.SetTransactions({tx});
    
    EXPECT_TRUE(block1 == block2);
    EXPECT_FALSE(block1 == block3);
    
    EXPECT_FALSE(block1 != block2);
    EXPECT_TRUE(block1 != block3);
}

TEST(BlockHeaderTest, Constructor)
{
    // Default constructor
    BlockHeader header1;
    EXPECT_EQ(header1.GetVersion(), 0);
    EXPECT_EQ(header1.GetPrevHash(), UInt256());
    EXPECT_EQ(header1.GetMerkleRoot(), UInt256());
    EXPECT_EQ(header1.GetTimestamp(), 0);
    EXPECT_EQ(header1.GetIndex(), 0);
    EXPECT_EQ(header1.GetNextConsensus(), 0);
    
    // Block constructor
    Block block;
    block.SetVersion(1);
    block.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    block.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    block.SetTimestamp(123456789);
    block.SetIndex(1);
    block.SetNextConsensus(987654321);
    
    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    block.SetWitness(witness);
    
    BlockHeader header2(block);
    EXPECT_EQ(header2.GetVersion(), 1);
    EXPECT_EQ(header2.GetPrevHash(), UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(header2.GetMerkleRoot(), UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(header2.GetTimestamp(), 123456789);
    EXPECT_EQ(header2.GetIndex(), 1);
    EXPECT_EQ(header2.GetNextConsensus(), 987654321);
    EXPECT_EQ(header2.GetWitness().GetInvocationScript(), invocationScript);
    EXPECT_EQ(header2.GetWitness().GetVerificationScript(), verificationScript);
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
    header.SetNextConsensus(987654321);
    
    // Add a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    header.SetWitness(witness);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    header.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    BlockHeader header2;
    header2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(header2.GetVersion(), 1);
    EXPECT_EQ(header2.GetPrevHash(), UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    EXPECT_EQ(header2.GetMerkleRoot(), UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    EXPECT_EQ(header2.GetTimestamp(), 123456789);
    EXPECT_EQ(header2.GetIndex(), 1);
    EXPECT_EQ(header2.GetNextConsensus(), 987654321);
    EXPECT_EQ(header2.GetWitness().GetInvocationScript(), invocationScript);
    EXPECT_EQ(header2.GetWitness().GetVerificationScript(), verificationScript);
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
    header.SetNextConsensus(987654321);
    
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
    writer.Write(header.GetIndex());
    writer.Write(header.GetNextConsensus());
    
    std::string data = stream.str();
    UInt256 expectedHash = neo::cryptography::Hash::Sha256(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    
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
    header1.SetNextConsensus(987654321);
    
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
    header2.SetNextConsensus(987654321);
    header2.SetWitness(witness);
    
    // Create a block header with different version
    BlockHeader header3;
    header3.SetVersion(2);
    header3.SetPrevHash(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    header3.SetMerkleRoot(UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40"));
    header3.SetTimestamp(123456789);
    header3.SetIndex(1);
    header3.SetNextConsensus(987654321);
    header3.SetWitness(witness);
    
    EXPECT_TRUE(header1 == header2);
    EXPECT_FALSE(header1 == header3);
    
    EXPECT_FALSE(header1 != header2);
    EXPECT_TRUE(header1 != header3);
}
