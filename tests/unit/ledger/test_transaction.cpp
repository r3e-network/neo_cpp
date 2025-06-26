#include <gtest/gtest.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/transaction_output.h>
#include <neo/core/fixed8.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <sstream>

using namespace neo::ledger;
using namespace neo::io;

TEST(WitnessTest, Constructor)
{
    // Default constructor
    Witness witness1;
    EXPECT_EQ(witness1.GetInvocationScript(), ByteVector());
    EXPECT_EQ(witness1.GetVerificationScript(), ByteVector());
    
    // Parameter constructor
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness2(invocationScript, verificationScript);
    EXPECT_EQ(witness2.GetInvocationScript(), invocationScript);
    EXPECT_EQ(witness2.GetVerificationScript(), verificationScript);
}

TEST(WitnessTest, GetScriptHash)
{
    ByteVector verificationScript = ByteVector::Parse("0102030405");
    Witness witness(ByteVector(), verificationScript);
    UInt160 scriptHash = witness.GetScriptHash();
    EXPECT_EQ(scriptHash, neo::cryptography::Hash::Hash160(verificationScript.AsSpan()));
}

TEST(WitnessTest, Serialization)
{
    // Create a witness
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    witness.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    Witness witness2;
    witness2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(witness2.GetInvocationScript(), invocationScript);
    EXPECT_EQ(witness2.GetVerificationScript(), verificationScript);
}

TEST(WitnessTest, Equality)
{
    ByteVector invocationScript1 = ByteVector::Parse("0102030405");
    ByteVector verificationScript1 = ByteVector::Parse("0607080910");
    Witness witness1(invocationScript1, verificationScript1);
    
    ByteVector invocationScript2 = ByteVector::Parse("0102030405");
    ByteVector verificationScript2 = ByteVector::Parse("0607080910");
    Witness witness2(invocationScript2, verificationScript2);
    
    ByteVector invocationScript3 = ByteVector::Parse("1112131415");
    ByteVector verificationScript3 = ByteVector::Parse("0607080910");
    Witness witness3(invocationScript3, verificationScript3);
    
    ByteVector invocationScript4 = ByteVector::Parse("0102030405");
    ByteVector verificationScript4 = ByteVector::Parse("1617181920");
    Witness witness4(invocationScript4, verificationScript4);
    
    EXPECT_TRUE(witness1 == witness2);
    EXPECT_FALSE(witness1 == witness3);
    EXPECT_FALSE(witness1 == witness4);
    
    EXPECT_FALSE(witness1 != witness2);
    EXPECT_TRUE(witness1 != witness3);
    EXPECT_TRUE(witness1 != witness4);
}

TEST(CoinReferenceTest, Constructor)
{
    // Default constructor
    CoinReference coinRef1;
    EXPECT_EQ(coinRef1.GetPrevHash(), UInt256());
    EXPECT_EQ(coinRef1.GetPrevIndex(), 0);
    
    // Parameter constructor
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference coinRef2(prevHash, prevIndex);
    EXPECT_EQ(coinRef2.GetPrevHash(), prevHash);
    EXPECT_EQ(coinRef2.GetPrevIndex(), prevIndex);
}

TEST(CoinReferenceTest, Serialization)
{
    // Create a coin reference
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference coinRef(prevHash, prevIndex);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    coinRef.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    CoinReference coinRef2;
    coinRef2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(coinRef2.GetPrevHash(), prevHash);
    EXPECT_EQ(coinRef2.GetPrevIndex(), prevIndex);
}

TEST(CoinReferenceTest, Equality)
{
    UInt256 prevHash1 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex1 = 123;
    CoinReference coinRef1(prevHash1, prevIndex1);
    
    UInt256 prevHash2 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex2 = 123;
    CoinReference coinRef2(prevHash2, prevIndex2);
    
    UInt256 prevHash3 = UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40");
    uint16_t prevIndex3 = 123;
    CoinReference coinRef3(prevHash3, prevIndex3);
    
    UInt256 prevHash4 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex4 = 456;
    CoinReference coinRef4(prevHash4, prevIndex4);
    
    EXPECT_TRUE(coinRef1 == coinRef2);
    EXPECT_FALSE(coinRef1 == coinRef3);
    EXPECT_FALSE(coinRef1 == coinRef4);
    
    EXPECT_FALSE(coinRef1 != coinRef2);
    EXPECT_TRUE(coinRef1 != coinRef3);
    EXPECT_TRUE(coinRef1 != coinRef4);
}

TEST(TransactionOutputTest, Constructor)
{
    // Default constructor
    TransactionOutput output1;
    EXPECT_EQ(output1.GetAssetId(), UInt256());
    EXPECT_EQ(output1.GetValue(), Fixed8(0));
    EXPECT_EQ(output1.GetScriptHash(), UInt160());
    
    // Parameter constructor
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output2(assetId, value, scriptHash);
    EXPECT_EQ(output2.GetAssetId(), assetId);
    EXPECT_EQ(output2.GetValue(), value);
    EXPECT_EQ(output2.GetScriptHash(), scriptHash);
}

TEST(TransactionOutputTest, Serialization)
{
    // Create a transaction output
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    output.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    TransactionOutput output2;
    output2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(output2.GetAssetId(), assetId);
    EXPECT_EQ(output2.GetValue(), value);
    EXPECT_EQ(output2.GetScriptHash(), scriptHash);
}

TEST(TransactionOutputTest, Equality)
{
    UInt256 assetId1 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value1(123);
    UInt160 scriptHash1 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output1(assetId1, value1, scriptHash1);
    
    UInt256 assetId2 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value2(123);
    UInt160 scriptHash2 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output2(assetId2, value2, scriptHash2);
    
    UInt256 assetId3 = UInt256::Parse("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40");
    Fixed8 value3(123);
    UInt160 scriptHash3 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output3(assetId3, value3, scriptHash3);
    
    UInt256 assetId4 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value4(456);
    UInt160 scriptHash4 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output4(assetId4, value4, scriptHash4);
    
    UInt256 assetId5 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value5(123);
    UInt160 scriptHash5 = UInt160::Parse("2122232425262728292a2b2c2d2e2f3031323334");
    TransactionOutput output5(assetId5, value5, scriptHash5);
    
    EXPECT_TRUE(output1 == output2);
    EXPECT_FALSE(output1 == output3);
    EXPECT_FALSE(output1 == output4);
    EXPECT_FALSE(output1 == output5);
    
    EXPECT_FALSE(output1 != output2);
    EXPECT_TRUE(output1 != output3);
    EXPECT_TRUE(output1 != output4);
    EXPECT_TRUE(output1 != output5);
}

TEST(TransactionAttributeTest, Constructor)
{
    // Default constructor
    TransactionAttribute attribute1;
    EXPECT_EQ(attribute1.GetUsage(), TransactionAttribute::Usage::ContractHash);
    EXPECT_EQ(attribute1.GetData(), ByteVector());
    
    // Parameter constructor
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute2(usage, data);
    EXPECT_EQ(attribute2.GetUsage(), usage);
    EXPECT_EQ(attribute2.GetData(), data);
}

TEST(TransactionAttributeTest, Serialization)
{
    // Create a transaction attribute
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionAttribute attribute(usage, data);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    attribute.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    TransactionAttribute attribute2;
    attribute2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(attribute2.GetUsage(), usage);
    EXPECT_EQ(attribute2.GetData(), data);
}

TEST(TransactionAttributeTest, Equality)
{
    TransactionAttribute::Usage usage1 = TransactionAttribute::Usage::Script;
    ByteVector data1 = ByteVector::Parse("0102030405");
    TransactionAttribute attribute1(usage1, data1);
    
    TransactionAttribute::Usage usage2 = TransactionAttribute::Usage::Script;
    ByteVector data2 = ByteVector::Parse("0102030405");
    TransactionAttribute attribute2(usage2, data2);
    
    TransactionAttribute::Usage usage3 = TransactionAttribute::Usage::Vote;
    ByteVector data3 = ByteVector::Parse("0102030405");
    TransactionAttribute attribute3(usage3, data3);
    
    TransactionAttribute::Usage usage4 = TransactionAttribute::Usage::Script;
    ByteVector data4 = ByteVector::Parse("0607080910");
    TransactionAttribute attribute4(usage4, data4);
    
    EXPECT_TRUE(attribute1 == attribute2);
    EXPECT_FALSE(attribute1 == attribute3);
    EXPECT_FALSE(attribute1 == attribute4);
    
    EXPECT_FALSE(attribute1 != attribute2);
    EXPECT_TRUE(attribute1 != attribute3);
    EXPECT_TRUE(attribute1 != attribute4);
}

TEST(TransactionTest, Constructor)
{
    // Default constructor
    Transaction tx;
    EXPECT_EQ(tx.GetType(), Transaction::Type::ContractTransaction);
    EXPECT_EQ(tx.GetVersion(), 0);
    EXPECT_TRUE(tx.GetAttributes().empty());
    EXPECT_TRUE(tx.GetInputs().empty());
    EXPECT_TRUE(tx.GetOutputs().empty());
    EXPECT_TRUE(tx.GetWitnesses().empty());
}

TEST(TransactionTest, Serialization)
{
    // Create a transaction
    Transaction tx;
    tx.SetType(Transaction::Type::InvocationTransaction);
    tx.SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute(usage, data);
    tx.SetAttributes({attribute});
    
    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference input(prevHash, prevIndex);
    tx.SetInputs({input});
    
    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    tx.SetOutputs({output});
    
    // Add witnesses
    ByteVector invocationScript = ByteVector::Parse("0102030405");
    ByteVector verificationScript = ByteVector::Parse("0607080910");
    Witness witness(invocationScript, verificationScript);
    tx.SetWitnesses({witness});
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    tx.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    Transaction tx2;
    tx2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(tx2.GetType(), Transaction::Type::InvocationTransaction);
    EXPECT_EQ(tx2.GetVersion(), 1);
    EXPECT_EQ(tx2.GetAttributes().size(), 1);
    EXPECT_EQ(tx2.GetAttributes()[0].GetUsage(), usage);
    EXPECT_EQ(tx2.GetAttributes()[0].GetData(), data);
    EXPECT_EQ(tx2.GetInputs().size(), 1);
    EXPECT_EQ(tx2.GetInputs()[0].GetPrevHash(), prevHash);
    EXPECT_EQ(tx2.GetInputs()[0].GetPrevIndex(), prevIndex);
    EXPECT_EQ(tx2.GetOutputs().size(), 1);
    EXPECT_EQ(tx2.GetOutputs()[0].GetAssetId(), assetId);
    EXPECT_EQ(tx2.GetOutputs()[0].GetValue(), value);
    EXPECT_EQ(tx2.GetOutputs()[0].GetScriptHash(), scriptHash);
    EXPECT_EQ(tx2.GetWitnesses().size(), 1);
    EXPECT_EQ(tx2.GetWitnesses()[0].GetInvocationScript(), invocationScript);
    EXPECT_EQ(tx2.GetWitnesses()[0].GetVerificationScript(), verificationScript);
}

TEST(TransactionTest, GetHash)
{
    // Create a transaction
    Transaction tx;
    tx.SetType(Transaction::Type::InvocationTransaction);
    tx.SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405");
    TransactionAttribute attribute(usage, data);
    tx.SetAttributes({attribute});
    
    // Add inputs
    UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex = 123;
    CoinReference input(prevHash, prevIndex);
    tx.SetInputs({input});
    
    // Add outputs
    UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value(123);
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output(assetId, value, scriptHash);
    tx.SetOutputs({output});
    
    // Get the hash
    UInt256 hash = tx.GetHash();
    
    // Verify the hash
    std::ostringstream stream;
    BinaryWriter writer(stream);
    
    // Serialize the transaction without witnesses
    writer.Write(static_cast<uint8_t>(tx.GetType()));
    writer.Write(tx.GetVersion());
    
    // Serialize attributes
    writer.WriteVarInt(tx.GetAttributes().size());
    for (const auto& attr : tx.GetAttributes())
    {
        attr.Serialize(writer);
    }
    
    // Serialize inputs
    writer.WriteVarInt(tx.GetInputs().size());
    for (const auto& input : tx.GetInputs())
    {
        input.Serialize(writer);
    }
    
    // Serialize outputs
    writer.WriteVarInt(tx.GetOutputs().size());
    for (const auto& output : tx.GetOutputs())
    {
        output.Serialize(writer);
    }
    
    std::string data2 = stream.str();
    UInt256 expectedHash = neo::cryptography::Hash::Sha256(ByteSpan(reinterpret_cast<const uint8_t*>(data2.data()), data2.size()));
    
    EXPECT_EQ(hash, expectedHash);
}

TEST(TransactionTest, Equality)
{
    // Create a transaction
    Transaction tx1;
    tx1.SetType(Transaction::Type::InvocationTransaction);
    tx1.SetVersion(1);
    
    // Add attributes
    TransactionAttribute::Usage usage1 = TransactionAttribute::Usage::Script;
    ByteVector data1 = ByteVector::Parse("0102030405");
    TransactionAttribute attribute1(usage1, data1);
    tx1.SetAttributes({attribute1});
    
    // Add inputs
    UInt256 prevHash1 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint16_t prevIndex1 = 123;
    CoinReference input1(prevHash1, prevIndex1);
    tx1.SetInputs({input1});
    
    // Add outputs
    UInt256 assetId1 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 value1(123);
    UInt160 scriptHash1 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    TransactionOutput output1(assetId1, value1, scriptHash1);
    tx1.SetOutputs({output1});
    
    // Add witnesses
    ByteVector invocationScript1 = ByteVector::Parse("0102030405");
    ByteVector verificationScript1 = ByteVector::Parse("0607080910");
    Witness witness1(invocationScript1, verificationScript1);
    tx1.SetWitnesses({witness1});
    
    // Create an identical transaction
    Transaction tx2;
    tx2.SetType(Transaction::Type::InvocationTransaction);
    tx2.SetVersion(1);
    tx2.SetAttributes({attribute1});
    tx2.SetInputs({input1});
    tx2.SetOutputs({output1});
    tx2.SetWitnesses({witness1});
    
    // Create a transaction with different type
    Transaction tx3;
    tx3.SetType(Transaction::Type::ContractTransaction);
    tx3.SetVersion(1);
    tx3.SetAttributes({attribute1});
    tx3.SetInputs({input1});
    tx3.SetOutputs({output1});
    tx3.SetWitnesses({witness1});
    
    // Create a transaction with different version
    Transaction tx4;
    tx4.SetType(Transaction::Type::InvocationTransaction);
    tx4.SetVersion(2);
    tx4.SetAttributes({attribute1});
    tx4.SetInputs({input1});
    tx4.SetOutputs({output1});
    tx4.SetWitnesses({witness1});
    
    EXPECT_TRUE(tx1 == tx2);
    EXPECT_FALSE(tx1 == tx3);
    EXPECT_FALSE(tx1 == tx4);
    
    EXPECT_FALSE(tx1 != tx2);
    EXPECT_TRUE(tx1 != tx3);
    EXPECT_TRUE(tx1 != tx4);
}
