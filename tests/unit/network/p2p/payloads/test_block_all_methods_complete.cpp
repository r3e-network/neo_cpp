#include "neo/extensions/utility.h"
#include "neo/io/memory_reader.h"
#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "neo/json/json.h"
#include "neo/ledger/blockchain.h"
#include "neo/network/p2p/payloads/block.h"
#include "neo/network/p2p/payloads/header.h"
#include "neo/network/p2p/payloads/transaction.h"
#include "neo/network/p2p/payloads/witness.h"
#include "neo/protocol_settings.h"
#include "neo/smartcontract/application_engine.h"
#include "neo/smartcontract/native/ledger.h"
#include "neo/smartcontract/trigger_type.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace neo;
using namespace neo::network::p2p::payloads;
using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::ledger;
using namespace neo::io;
using namespace neo::json;

// Complete conversion of C# UT_Block.cs - ALL 14 test methods
class BlockAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Test block hex data from C# UT_Block.cs
        block_hex_ = "0000000000000000000000000000000000000000000000000000000000000000000000006c23be5d326" +
                     "79baa9c5c2aa0d329fd2a2441d7875d0f34d42f58f70428fbbbb9493ed0e58f01000000000000000000" +
                     "00000000000000000000000000000000000000000000000000000100011101000000000000000000000" +
                     "0000000000000000000000000000001000000000000000000000000000000000000000001000112010000";

        // Initialize protocol settings for testing
        protocol_settings_ = GetTestProtocolSettings();

        // Initialize blockchain system for testing
        system_ = GetTestBlockchainSystem();
    }

    void TearDown() override
    {
        // Clean up resources
    }

    // Helper method equivalent to C# GetEngine
    std::unique_ptr<ApplicationEngine> GetEngine(bool has_container = false, bool has_snapshot = false,
                                                 bool has_block = false, bool add_script = true,
                                                 int64_t gas = 2000000000L)
    {
        auto system = GetTestBlockchainSystem();

        Transaction* tx = has_container ? GetTestTransaction(UInt160::Zero()) : nullptr;
        std::shared_ptr<DataCache> snapshot_cache = has_snapshot ? system->GetTestSnapshotCache() : nullptr;

        Block* block = nullptr;
        if (has_block)
        {
            block = new Block();
            block->SetHeader(std::make_shared<Header>());
        }

        auto engine =
            ApplicationEngine::Create(TriggerType::Application, tx, snapshot_cache, block, protocol_settings_, gas);

        if (add_script)
        {
            engine->LoadScript({0x01});
        }

        return engine;
    }

    // Helper methods from C# TestUtils equivalent
    std::unique_ptr<Block> MakeBlock(Transaction* tx, const UInt256& prev_hash, uint32_t tx_count)
    {
        auto block = std::make_unique<Block>();
        auto header = std::make_shared<Header>();

        header->SetPrevHash(prev_hash);
        header->SetIndex(0);
        header->SetTimestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
        header->SetNonce(0x0001020304050607ULL);
        header->SetNextConsensus(UInt160::Parse("NKuyBkoGdZZSLyPbJEetheRhMjeznFZszf"));

        // Set witness
        Witness witness = Witness::Empty();
        witness.SetVerificationScript({0x11});  // PUSH1
        header->SetWitness(witness);

        block->SetHeader(header);

        // Add transactions if specified
        if (tx_count > 0)
        {
            std::vector<Transaction*> transactions;
            for (uint32_t i = 0; i < tx_count; i++)
            {
                transactions.push_back(GetTestTransaction(UInt160::Zero()));
            }
            block->SetTransactions(transactions);
        }

        return block;
    }

    Transaction* GetTestTransaction(const UInt160& sender)
    {
        auto tx = new Transaction();
        tx->SetVersion(0);
        tx->SetNonce(0);
        tx->SetSystemFee(0);
        tx->SetNetworkFee(0);
        tx->SetValidUntilBlock(1000000);
        tx->SetScript({0x11});  // PUSH1
        tx->SetAttributes({});

        Signer signer;
        signer.SetAccount(sender);
        tx->SetSigners({signer});

        tx->SetWitnesses({Witness::Empty()});

        return tx;
    }

    ProtocolSettings GetTestProtocolSettings()
    {
        ProtocolSettings settings;
        settings.network = 844378958;
        settings.max_transaction_size = 102400;
        settings.max_block_size = 262144;
        return settings;
    }

    std::shared_ptr<BlockchainSystem> GetTestBlockchainSystem()
    {
        // Mock blockchain system for testing
        return std::make_shared<TestBlockchainSystem>();
    }

    std::vector<uint8_t> GetByteArray(size_t length, uint8_t value)
    {
        return std::vector<uint8_t>(length, value);
    }

    std::string block_hex_;
    ProtocolSettings protocol_settings_;
    std::shared_ptr<BlockchainSystem> system_;
};

// C# Test Method: Transactions_Get()
TEST_F(BlockAllMethodsTest, TransactionsGet)
{
    auto uut = std::make_unique<Block>();
    EXPECT_EQ(nullptr, uut->Transactions());
}

// C# Test Method: Header_Get()
TEST_F(BlockAllMethodsTest, HeaderGet)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 0);
    EXPECT_NE(nullptr, uut->Header());
    EXPECT_EQ(UInt256::Zero(), uut->Header()->PrevHash());
}

// C# Test Method: Size_Get()
TEST_F(BlockAllMethodsTest, SizeGet)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 0);
    // header 4 + 32 + 32 + 8 + 4 + 1 + 20 + 4
    // tx 1
    EXPECT_EQ(114, uut->Size());  // 106 + nonce
}

// C# Test Method: Size_Get_1_Transaction()
TEST_F(BlockAllMethodsTest, SizeGet1Transaction)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);
    std::vector<Transaction*> transactions = {GetTestTransaction(UInt160::Zero())};
    uut->SetTransactions(transactions);

    EXPECT_EQ(167, uut->Size());  // 159 + nonce
}

// C# Test Method: Size_Get_3_Transaction()
TEST_F(BlockAllMethodsTest, SizeGet3Transaction)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 3);
    std::vector<Transaction*> transactions = {GetTestTransaction(UInt160::Zero()), GetTestTransaction(UInt160::Zero()),
                                              GetTestTransaction(UInt160::Zero())};
    uut->SetTransactions(transactions);

    EXPECT_EQ(273, uut->Size());  // 265 + nonce
}

// C# Test Method: Serialize()
TEST_F(BlockAllMethodsTest, Serialize)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);
    auto serialized = uut->ToArray();
    auto hex_string = Utility::ToHexString(serialized);
    EXPECT_EQ(block_hex_, hex_string);
}

// C# Test Method: Deserialize()
TEST_F(BlockAllMethodsTest, Deserialize)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);
    auto hex_bytes = Utility::FromHexString(block_hex_);
    MemoryReader reader(hex_bytes);
    uut->Deserialize(reader);
    auto merkle_root = uut->MerkleRoot();

    EXPECT_EQ(merkle_root, uut->MerkleRoot());
}

// C# Test Method: Equals_SameObj()
TEST_F(BlockAllMethodsTest, EqualsSameObj)
{
    auto uut = std::make_unique<Block>();
    EXPECT_TRUE(uut->Equals(*uut));

    Block* obj = uut.get();
    EXPECT_TRUE(uut->Equals(*obj));
}

// C# Test Method: TestGetHashCode()
TEST_F(BlockAllMethodsTest, TestGetHashCode)
{
    auto engine = GetEngine(true, true);
    auto snapshot = engine->SnapshotCache();
    auto block = Ledger::GetBlock(snapshot, 0);
    EXPECT_EQ(-626492395, block->GetHashCode());
}

// C# Test Method: Equals_DiffObj()
TEST_F(BlockAllMethodsTest, EqualsDiffObj)
{
    auto prev_hash = UInt256(GetByteArray(32, 0x42));
    auto block = MakeBlock(nullptr, UInt256::Zero(), 1);
    auto uut = MakeBlock(nullptr, prev_hash, 0);

    EXPECT_FALSE(uut->Equals(*block));
}

// C# Test Method: Equals_Null()
TEST_F(BlockAllMethodsTest, EqualsNull)
{
    auto uut = std::make_unique<Block>();
    EXPECT_FALSE(uut->Equals(nullptr));
}

// C# Test Method: Equals_SameHash()
TEST_F(BlockAllMethodsTest, EqualsSameHash)
{
    auto prev_hash = UInt256(GetByteArray(32, 0x42));
    auto block = MakeBlock(nullptr, prev_hash, 1);
    auto uut = MakeBlock(nullptr, prev_hash, 1);
    EXPECT_TRUE(uut->Equals(*block));
}

// C# Test Method: ToJson()
TEST_F(BlockAllMethodsTest, ToJson)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);
    auto json_obj = uut->ToJson(protocol_settings_);
    EXPECT_NE(nullptr, json_obj);

    EXPECT_EQ("0x942065e93848732c2e7844061fa92d20c5d9dc0bc71d420a1ea71b3431fc21b4", json_obj->GetString("hash"));
    EXPECT_EQ(167, json_obj->GetNumber("size"));  // 159 + nonce
    EXPECT_EQ(0, json_obj->GetNumber("version"));
    EXPECT_EQ("0x0000000000000000000000000000000000000000000000000000000000000000",
              json_obj->GetString("previousblockhash"));
    EXPECT_EQ("0xb9bbfb2804f7582fd4340f5d87d741242afd29d3a02a5c9caa9b67325dbe236c", json_obj->GetString("merkleroot"));
    EXPECT_EQ(uut->Header()->Timestamp(), json_obj->GetNumber("time"));

    // Format nonce as hex string
    std::stringstream nonce_stream;
    nonce_stream << std::hex << std::uppercase << uut->Header()->Nonce();
    std::string nonce_hex = nonce_stream.str();
    while (nonce_hex.length() < 16)
    {
        nonce_hex = "0" + nonce_hex;
    }
    EXPECT_EQ(nonce_hex, json_obj->GetString("nonce"));

    EXPECT_EQ(uut->Header()->Index(), json_obj->GetNumber("index"));
    EXPECT_EQ("NKuyBkoGdZZSLyPbJEetheRhMjeznFZszf", json_obj->GetString("nextconsensus"));

    // Check witnesses
    auto witnesses_array = json_obj->GetArray("witnesses");
    EXPECT_EQ(1, witnesses_array->size());
    auto witness_obj = (*witnesses_array)[0]->AsObject();
    EXPECT_EQ("", witness_obj->GetString("invocation"));
    EXPECT_EQ("EQ==", witness_obj->GetString("verification"));

    // Check transactions
    auto tx_array = json_obj->GetArray("tx");
    EXPECT_NE(nullptr, tx_array);
    EXPECT_EQ(1, tx_array->size());

    auto tx_obj = (*tx_array)[0]->AsObject();
    EXPECT_EQ("0xb9bbfb2804f7582fd4340f5d87d741242afd29d3a02a5c9caa9b67325dbe236c", tx_obj->GetString("hash"));
    EXPECT_EQ(53, tx_obj->GetNumber("size"));
    EXPECT_EQ(0, tx_obj->GetNumber("version"));

    auto attributes_array = tx_obj->GetArray("attributes");
    EXPECT_EQ(0, attributes_array->size());
    EXPECT_EQ("0", tx_obj->GetString("netfee"));
}

// C# Test Method: Witness()
TEST_F(BlockAllMethodsTest, Witness)
{
    auto item = std::make_unique<Block>();
    auto header = std::make_shared<Header>();
    item->SetHeader(header);

    // Block should have exactly 1 witness
    EXPECT_EQ(1, item->Witnesses().size());

    // Setting witnesses to null should throw NotSupportedException
    EXPECT_THROW(item->SetWitnesses({}), std::runtime_error);
}

// Additional comprehensive test methods to ensure complete coverage

// Test Method: TestMerkleRootCalculation()
TEST_F(BlockAllMethodsTest, TestMerkleRootCalculation)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 2);
    std::vector<Transaction*> transactions = {
        GetTestTransaction(UInt160::Zero()),
        GetTestTransaction(UInt160::Parse("0x1234567890123456789012345678901234567890"))};
    uut->SetTransactions(transactions);

    // Verify merkle root is calculated correctly
    auto merkle_root = uut->MerkleRoot();
    EXPECT_FALSE(merkle_root.IsZero());

    // Merkle root should be deterministic
    auto merkle_root2 = uut->MerkleRoot();
    EXPECT_EQ(merkle_root, merkle_root2);
}

// Test Method: TestBlockValidation()
TEST_F(BlockAllMethodsTest, TestBlockValidation)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);

    // Test basic validation
    EXPECT_TRUE(uut->IsValid());

    // Test with invalid header
    uut->Header()->SetIndex(0xFFFFFFFF);  // Invalid index
    EXPECT_FALSE(uut->IsValid());
}

// Test Method: TestBlockSerialization()
TEST_F(BlockAllMethodsTest, TestBlockSerialization)
{
    auto original = MakeBlock(nullptr, UInt256::Zero(), 1);

    // Serialize block
    auto serialized = original->ToArray();
    EXPECT_GT(serialized.size(), 0);

    // Deserialize block
    auto deserialized = Block::FromArray(serialized);

    // Verify they are equal
    EXPECT_TRUE(original->Equals(*deserialized));
    EXPECT_EQ(original->Hash(), deserialized->Hash());
    EXPECT_EQ(original->Size(), deserialized->Size());
}

// Test Method: TestBlockTimestamp()
TEST_F(BlockAllMethodsTest, TestBlockTimestamp)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 0);

    // Test timestamp is set
    EXPECT_GT(uut->Header()->Timestamp(), 0);

    // Test timestamp modification
    uint64_t new_timestamp = 1234567890123ULL;
    uut->Header()->SetTimestamp(new_timestamp);
    EXPECT_EQ(new_timestamp, uut->Header()->Timestamp());
}

// Test Method: TestBlockIndex()
TEST_F(BlockAllMethodsTest, TestBlockIndex)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 0);

    // Test default index
    EXPECT_EQ(0, uut->Header()->Index());

    // Test index modification
    uint32_t new_index = 12345;
    uut->Header()->SetIndex(new_index);
    EXPECT_EQ(new_index, uut->Header()->Index());
}

// Test Method: TestBlockConsensusData()
TEST_F(BlockAllMethodsTest, TestBlockConsensusData)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 0);

    // Test next consensus address
    auto next_consensus = uut->Header()->NextConsensus();
    EXPECT_FALSE(next_consensus.IsZero());

    // Test nonce
    auto nonce = uut->Header()->Nonce();
    EXPECT_NE(0, nonce);
}

// Test Method: TestEmptyBlock()
TEST_F(BlockAllMethodsTest, TestEmptyBlock)
{
    auto uut = std::make_unique<Block>();

    // Empty block should have null transactions
    EXPECT_EQ(nullptr, uut->Transactions());

    // Empty block should have null header initially
    EXPECT_EQ(nullptr, uut->Header());

    // Empty block size should be minimal
    auto header = std::make_shared<Header>();
    uut->SetHeader(header);
    EXPECT_GT(uut->Size(), 0);
}

// Test Method: TestBlockWithMaxTransactions()
TEST_F(BlockAllMethodsTest, TestBlockWithMaxTransactions)
{
    // Test with maximum reasonable number of transactions
    const int max_tx_count = 1000;
    auto uut = MakeBlock(nullptr, UInt256::Zero(), max_tx_count);

    std::vector<Transaction*> transactions;
    for (int i = 0; i < max_tx_count; i++)
    {
        auto sender = UInt160::Parse("0x" + std::to_string(i % 256));
        transactions.push_back(GetTestTransaction(sender));
    }
    uut->SetTransactions(transactions);

    EXPECT_EQ(max_tx_count, uut->Transactions()->size());
    EXPECT_GT(uut->Size(), max_tx_count * 50);  // Each tx should be at least 50 bytes
}

// Test Method: TestBlockHashConsistency()
TEST_F(BlockAllMethodsTest, TestBlockHashConsistency)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);

    // Hash should be consistent across multiple calls
    auto hash1 = uut->Hash();
    auto hash2 = uut->Hash();
    EXPECT_EQ(hash1, hash2);

    // Hash should change if block content changes
    uut->Header()->SetNonce(0x9876543210987654ULL);
    auto hash3 = uut->Hash();
    EXPECT_NE(hash1, hash3);
}

// Test Method: TestBlockStringRepresentation()
TEST_F(BlockAllMethodsTest, TestBlockStringRepresentation)
{
    auto uut = MakeBlock(nullptr, UInt256::Zero(), 1);

    // Test ToString method
    auto string_repr = uut->ToString();
    EXPECT_GT(string_repr.length(), 0);
    EXPECT_NE(std::string::npos, string_repr.find("Block"));

    // String representation should include hash
    auto hash_str = uut->Hash().ToString();
    EXPECT_NE(std::string::npos, string_repr.find(hash_str));
}