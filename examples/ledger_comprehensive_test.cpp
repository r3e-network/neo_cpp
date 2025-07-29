#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/ledger/transaction.h>
#include <sstream>
#include <vector>

using namespace neo::ledger;
using namespace neo::io;
using namespace neo::cryptography;

// Test Witness
TEST(LedgerComprehensiveTest, Witness)
{
    // Create a Witness
    ByteVector invocationScript = {0x01, 0x02, 0x03};
    ByteVector verificationScript = {0x04, 0x05, 0x06};
    Witness witness(invocationScript, verificationScript);

    // Check the properties
    EXPECT_EQ(witness.GetInvocationScript(), invocationScript);
    EXPECT_EQ(witness.GetVerificationScript(), verificationScript);

    // Get the script hash
    UInt160 scriptHash = witness.GetScriptHash();
    EXPECT_EQ(scriptHash, Hash::Hash160(verificationScript.AsSpan()));

    // Serialize and deserialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    witness.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    Witness deserializedWitness;
    deserializedWitness.Deserialize(reader);

    // Check the deserialized witness
    EXPECT_EQ(deserializedWitness.GetInvocationScript(), invocationScript);
    EXPECT_EQ(deserializedWitness.GetVerificationScript(), verificationScript);

    // Test equality operators
    EXPECT_TRUE(witness == deserializedWitness);
    EXPECT_FALSE(witness != deserializedWitness);

    // Test with different invocation script
    ByteVector differentInvocationScript = {0x01, 0x02, 0x04};
    Witness differentWitness1(differentInvocationScript, verificationScript);
    EXPECT_FALSE(witness == differentWitness1);
    EXPECT_TRUE(witness != differentWitness1);

    // Test with different verification script
    ByteVector differentVerificationScript = {0x04, 0x05, 0x07};
    Witness differentWitness2(invocationScript, differentVerificationScript);
    EXPECT_FALSE(witness == differentWitness2);
    EXPECT_TRUE(witness != differentWitness2);
}

// Test CoinReference
TEST(LedgerComprehensiveTest, CoinReference)
{
    // Create a CoinReference
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference coinRef(prevHash, prevIndex);

    // Check the properties
    EXPECT_EQ(coinRef.GetPrevHash(), prevHash);
    EXPECT_EQ(coinRef.GetPrevIndex(), prevIndex);

    // Serialize and deserialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    coinRef.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    CoinReference deserializedCoinRef;
    deserializedCoinRef.Deserialize(reader);

    // Check the deserialized coin reference
    EXPECT_EQ(deserializedCoinRef.GetPrevHash(), prevHash);
    EXPECT_EQ(deserializedCoinRef.GetPrevIndex(), prevIndex);

    // Test equality operators
    EXPECT_TRUE(coinRef == deserializedCoinRef);
    EXPECT_FALSE(coinRef != deserializedCoinRef);

    // Test with different prev hash
    UInt256 differentPrevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f21");
    CoinReference differentCoinRef1(differentPrevHash, prevIndex);
    EXPECT_FALSE(coinRef == differentCoinRef1);
    EXPECT_TRUE(coinRef != differentCoinRef1);

    // Test with different prev index
    CoinReference differentCoinRef2(prevHash, prevIndex + 1);
    EXPECT_FALSE(coinRef == differentCoinRef2);
    EXPECT_TRUE(coinRef != differentCoinRef2);
}

// Test TransactionOutput
TEST(LedgerComprehensiveTest, TransactionOutput)
{
    // Create a TransactionOutput
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123000000);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);

    // Check the properties
    EXPECT_EQ(output.GetAssetId(), assetId);
    EXPECT_EQ(output.GetValue(), value);
    EXPECT_EQ(output.GetScriptHash(), scriptHash);

    // Serialize and deserialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    output.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    TransactionOutput deserializedOutput;
    deserializedOutput.Deserialize(reader);

    // Check the deserialized output
    EXPECT_EQ(deserializedOutput.GetAssetId(), assetId);
    EXPECT_EQ(deserializedOutput.GetValue(), value);
    EXPECT_EQ(deserializedOutput.GetScriptHash(), scriptHash);

    // Test equality operators
    EXPECT_TRUE(output == deserializedOutput);
    EXPECT_FALSE(output != deserializedOutput);

    // Test with different asset ID
    UInt256 differentAssetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f21");
    TransactionOutput differentOutput1(differentAssetId, value, scriptHash);
    EXPECT_FALSE(output == differentOutput1);
    EXPECT_TRUE(output != differentOutput1);

    // Test with different value
    Fixed8 differentValue(456000000);
    TransactionOutput differentOutput2(assetId, differentValue, scriptHash);
    EXPECT_FALSE(output == differentOutput2);
    EXPECT_TRUE(output != differentOutput2);

    // Test with different script hash
    UInt160 differentScriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121315");
    TransactionOutput differentOutput3(assetId, value, differentScriptHash);
    EXPECT_FALSE(output == differentOutput3);
    EXPECT_TRUE(output != differentOutput3);
}

// Test Transaction
TEST(LedgerComprehensiveTest, Transaction)
{
    // Create a Transaction
    Transaction tx;
    tx.SetType(Transaction::Type::ContractTransaction);
    tx.SetVersion(0);

    // Add attributes
    TransactionAttribute attribute(TransactionAttribute::Usage::Script,
                                   ByteVector{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                              0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14});
    std::vector<TransactionAttribute> attributes = {attribute};
    tx.SetAttributes(attributes);

    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    CoinReference input(prevHash, 0);
    std::vector<CoinReference> inputs = {input};
    tx.SetInputs(inputs);

    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123000000);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    std::vector<TransactionOutput> outputs = {output};
    tx.SetOutputs(outputs);

    // Add witnesses
    ByteVector invocationScript = {0x01, 0x02, 0x03};
    ByteVector verificationScript = {0x04, 0x05, 0x06};
    Witness witness(invocationScript, verificationScript);
    std::vector<Witness> witnesses = {witness};
    tx.SetWitnesses(witnesses);

    // Check the properties
    EXPECT_EQ(tx.GetType(), Transaction::Type::ContractTransaction);
    EXPECT_EQ(tx.GetVersion(), 0);
    EXPECT_EQ(tx.GetAttributes().size(), 1);
    EXPECT_EQ(tx.GetInputs().size(), 1);
    EXPECT_EQ(tx.GetOutputs().size(), 1);
    EXPECT_EQ(tx.GetWitnesses().size(), 1);

    // Get the hash
    UInt256 hash = tx.GetHash();

    // Serialize and deserialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    tx.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    Transaction deserializedTx;
    deserializedTx.Deserialize(reader);

    // Check the deserialized transaction
    EXPECT_EQ(deserializedTx.GetType(), Transaction::Type::ContractTransaction);
    EXPECT_EQ(deserializedTx.GetVersion(), 0);
    EXPECT_EQ(deserializedTx.GetAttributes().size(), 1);
    EXPECT_EQ(deserializedTx.GetInputs().size(), 1);
    EXPECT_EQ(deserializedTx.GetOutputs().size(), 1);
    EXPECT_EQ(deserializedTx.GetWitnesses().size(), 1);

    // Check the hash
    EXPECT_EQ(deserializedTx.GetHash(), hash);

    // Test equality operators
    EXPECT_TRUE(tx == deserializedTx);
    EXPECT_FALSE(tx != deserializedTx);
}

int main(int argc, char** argv)
{
    std::cout << "Running Ledger comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
