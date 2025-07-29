#include <gtest/gtest.h>
#include <memory>
#include <neo/cryptography/hash.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>

using namespace neo::ledger;
using namespace neo::io;
using namespace neo::cryptography;

TEST(LedgerJsonSerializationTest, WitnessSerializeDeserialize)
{
    // Create a witness
    Witness witness;
    witness.SetInvocationScript(ByteVector::Parse("0123456789ABCDEF"));
    witness.SetVerificationScript(ByteVector::Parse("FEDCBA9876543210"));

    // Serialize to JSON
    nlohmann::json json = witness.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["invocation"], "0123456789ABCDEF");
    EXPECT_EQ(json["verification"], "FEDCBA9876543210");

    // Deserialize from JSON
    Witness deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetInvocationScript().AsSpan().ToHexString(), "0123456789ABCDEF");
    EXPECT_EQ(deserialized.GetVerificationScript().AsSpan().ToHexString(), "FEDCBA9876543210");
}

TEST(LedgerJsonSerializationTest, CoinReferenceSerializeDeserialize)
{
    // Create a coin reference
    CoinReference coinRef;
    coinRef.SetPrevHash(UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    coinRef.SetPrevIndex(123);

    // Serialize to JSON
    nlohmann::json json = coinRef.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["txid"], "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(json["vout"], 123);

    // Deserialize from JSON
    CoinReference deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetPrevHash().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserialized.GetPrevIndex(), 123);
}

TEST(LedgerJsonSerializationTest, TransactionOutputSerializeDeserialize)
{
    // Create a transaction output
    TransactionOutput output;
    output.SetAssetId(UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    output.SetValue(Fixed8::FromDouble(123.45));
    output.SetScriptHash(UInt160::Parse("0123456789ABCDEF0123456789ABCDEF01234567"));

    // Serialize to JSON
    nlohmann::json json = output.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["asset"], "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(json["value"], "123.45");
    EXPECT_EQ(json["address"], "0123456789ABCDEF0123456789ABCDEF01234567");

    // Deserialize from JSON
    TransactionOutput deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetAssetId().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserialized.GetValue().ToString(), "123.45");
    EXPECT_EQ(deserialized.GetScriptHash().ToHexString(), "0123456789ABCDEF0123456789ABCDEF01234567");
}

TEST(LedgerJsonSerializationTest, TransactionAttributeSerializeDeserialize)
{
    // Create a transaction attribute
    TransactionAttribute attr;
    attr.SetUsage(TransactionAttribute::Usage::Script);
    attr.SetData(ByteVector::Parse("0123456789ABCDEF"));

    // Serialize to JSON
    nlohmann::json json = attr.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["usage"], static_cast<uint8_t>(TransactionAttribute::Usage::Script));
    EXPECT_EQ(json["data"], "0123456789ABCDEF");

    // Deserialize from JSON
    TransactionAttribute deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetUsage(), TransactionAttribute::Usage::Script);
    EXPECT_EQ(deserialized.GetData().AsSpan().ToHexString(), "0123456789ABCDEF");
}

TEST(LedgerJsonSerializationTest, TransactionSerializeDeserialize)
{
    // Create a transaction
    Transaction tx;
    tx.SetType(Transaction::Type::ContractTransaction);
    tx.SetVersion(0);

    // Add attributes
    TransactionAttribute attr;
    attr.SetUsage(TransactionAttribute::Usage::Script);
    attr.SetData(ByteVector::Parse("0123456789ABCDEF"));
    tx.SetAttributes({attr});

    // Add inputs
    CoinReference input;
    input.SetPrevHash(UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    input.SetPrevIndex(123);
    tx.SetInputs({input});

    // Add outputs
    TransactionOutput output;
    output.SetAssetId(UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    output.SetValue(Fixed8::FromDouble(123.45));
    output.SetScriptHash(UInt160::Parse("0123456789ABCDEF0123456789ABCDEF01234567"));
    tx.SetOutputs({output});

    // Add witnesses
    Witness witness;
    witness.SetInvocationScript(ByteVector::Parse("0123456789ABCDEF"));
    witness.SetVerificationScript(ByteVector::Parse("FEDCBA9876543210"));
    tx.SetWitnesses({witness});

    // Serialize to JSON
    nlohmann::json json = tx.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["type"], static_cast<uint8_t>(Transaction::Type::ContractTransaction));
    EXPECT_EQ(json["version"], 0);
    EXPECT_EQ(json["attributes"].size(), 1);
    EXPECT_EQ(json["vin"].size(), 1);
    EXPECT_EQ(json["vout"].size(), 1);
    EXPECT_EQ(json["witnesses"].size(), 1);

    // Deserialize from JSON
    Transaction deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetType(), Transaction::Type::ContractTransaction);
    EXPECT_EQ(deserialized.GetVersion(), 0);
    EXPECT_EQ(deserialized.GetAttributes().size(), 1);
    EXPECT_EQ(deserialized.GetInputs().size(), 1);
    EXPECT_EQ(deserialized.GetOutputs().size(), 1);
    EXPECT_EQ(deserialized.GetWitnesses().size(), 1);
}

TEST(LedgerJsonSerializationTest, BlockSerializeDeserialize)
{
    // Create a block
    Block block;
    block.SetVersion(0);
    block.SetPrevHash(UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    block.SetMerkleRoot(UInt256::Parse("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210"));
    block.SetTimestamp(1234567890);
    block.SetIndex(123);
    block.SetNextConsensus(123456789);

    // Add witness
    Witness witness;
    witness.SetInvocationScript(ByteVector::Parse("0123456789ABCDEF"));
    witness.SetVerificationScript(ByteVector::Parse("FEDCBA9876543210"));
    block.SetWitness(witness);

    // Add transactions
    auto tx = std::make_shared<Transaction>();
    tx->SetType(Transaction::Type::ContractTransaction);
    tx->SetVersion(0);
    block.SetTransactions({tx});

    // Rebuild merkle root
    block.RebuildMerkleRoot();

    // Serialize to JSON
    nlohmann::json json = block.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["version"], 0);
    EXPECT_EQ(json["previousblockhash"], "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(json["time"], 1234567890);
    EXPECT_EQ(json["index"], 123);
    EXPECT_EQ(json["tx"].size(), 1);

    // Deserialize from JSON
    Block deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.GetVersion(), 0);
    EXPECT_EQ(deserialized.GetPrevHash().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserialized.GetTimestamp(), 1234567890);
    EXPECT_EQ(deserialized.GetIndex(), 123);
    EXPECT_EQ(deserialized.GetTransactions().size(), 1);
}
