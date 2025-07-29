// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class LedgerContractTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<LedgerContract> ledgerContract;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        ledgerContract = LedgerContract::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }

    void CreateTestBlock(uint32_t index, const UInt256& prevHash)
    {
        // Create a block
        auto block = std::make_shared<Block>();
        block->SetVersion(0);
        block->SetPrevHash(prevHash);
        block->SetMerkleRoot(UInt256());
        block->SetTimestamp(1234567890);
        block->SetIndex(index);
        block->SetNextConsensus(UInt160());

        // Add the block to the snapshot
        ledgerContract->OnPersist(*engine);
        ledgerContract->PostPersist(*engine);
    }

    void CreateTestTransaction(const UInt256& hash, uint32_t blockIndex)
    {
        // Create a transaction
        auto tx = std::make_shared<Transaction>();
        tx->SetVersion(0);
        tx->SetNonce(1234);
        tx->SetSender(UInt160());
        tx->SetSystemFee(0);
        tx->SetNetworkFee(0);
        tx->SetValidUntilBlock(blockIndex + 100);
        tx->SetScript(ByteVector{1, 2, 3});

        // Add the transaction to the snapshot
        auto txKey = ledgerContract->CreateStorageKey(LedgerContract::PREFIX_TRANSACTION,
                                                      ByteVector(ByteSpan(hash.Data(), hash.Size())));

        std::ostringstream txStream;
        BinaryWriter txWriter(txStream);
        tx->Serialize(txWriter);
        txWriter.Write(blockIndex);
        std::string txData = txStream.str();

        auto txItem = StorageItem(ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>(txData.data()), txData.size())));
        snapshot->Add(txKey, txItem);
    }
};

TEST_F(LedgerContractTest, TestGetCurrentIndexAndHash)
{
    // Create a test block
    UInt256 prevHash;
    std::memset(prevHash.Data(), 0, prevHash.Size());
    CreateTestBlock(1, prevHash);

    // Call the getCurrentIndex method
    auto indexResult = ledgerContract->Call(*engine, "getCurrentIndex", {});

    // Check the result
    ASSERT_TRUE(indexResult->IsInteger());
    ASSERT_EQ(indexResult->GetInteger(), 1);

    // Call the getCurrentHash method
    auto hashResult = ledgerContract->Call(*engine, "getCurrentHash", {});

    // Check the result
    ASSERT_TRUE(hashResult->IsBuffer());
    ASSERT_EQ(hashResult->GetByteArray().Size(), 32);
}

TEST_F(LedgerContractTest, TestGetHash)
{
    // Create test blocks
    UInt256 prevHash;
    std::memset(prevHash.Data(), 0, prevHash.Size());
    CreateTestBlock(1, prevHash);

    // Get the hash of the first block
    auto hash = ledgerContract->GetCurrentHash(snapshot);

    // Create another block
    CreateTestBlock(2, hash);

    // Call the getHash method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(1));
    auto result = ledgerContract->Call(*engine, "getHash", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    ASSERT_EQ(result->GetByteArray().Size(), 32);

    // Call the getHash method with an invalid index
    args.clear();
    args.push_back(StackItem::Create(100));
    result = ledgerContract->Call(*engine, "getHash", args);

    // Check the result
    ASSERT_TRUE(result->IsNull());
}

TEST_F(LedgerContractTest, TestGetBlock)
{
    // Create test blocks
    UInt256 prevHash;
    std::memset(prevHash.Data(), 0, prevHash.Size());
    CreateTestBlock(1, prevHash);

    // Get the hash of the first block
    auto hash = ledgerContract->GetCurrentHash(snapshot);

    // Call the getBlock method with index
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(1));
    auto result = ledgerContract->Call(*engine, "getBlock", args);

    // Check the result
    ASSERT_TRUE(result->IsArray());
    auto blockArray = result->GetArray();
    ASSERT_EQ(blockArray.size(), 8);

    // Call the getBlock method with hash
    args.clear();
    args.push_back(StackItem::Create(ByteVector(ByteSpan(hash.Data(), hash.Size()))));
    result = ledgerContract->Call(*engine, "getBlock", args);

    // Check the result
    ASSERT_TRUE(result->IsArray());
    blockArray = result->GetArray();
    ASSERT_EQ(blockArray.size(), 8);

    // Call the getBlock method with an invalid index
    args.clear();
    args.push_back(StackItem::Create(100));
    result = ledgerContract->Call(*engine, "getBlock", args);

    // Check the result
    ASSERT_TRUE(result->IsNull());
}

TEST_F(LedgerContractTest, TestGetTransactionAndHeight)
{
    // Create a test block
    UInt256 prevHash;
    std::memset(prevHash.Data(), 0, prevHash.Size());
    CreateTestBlock(1, prevHash);

    // Create a test transaction
    UInt256 txHash;
    std::memset(txHash.Data(), 1, txHash.Size());
    CreateTestTransaction(txHash, 1);

    // Call the getTransaction method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(ByteVector(ByteSpan(txHash.Data(), txHash.Size()))));
    auto result = ledgerContract->Call(*engine, "getTransaction", args);

    // Check the result
    ASSERT_TRUE(result->IsArray());
    auto txArray = result->GetArray();
    ASSERT_EQ(txArray.size(), 9);

    // Call the getTransactionHeight method
    args.clear();
    args.push_back(StackItem::Create(ByteVector(ByteSpan(txHash.Data(), txHash.Size()))));
    result = ledgerContract->Call(*engine, "getTransactionHeight", args);

    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 1);

    // Call the getTransaction method with an invalid hash
    UInt256 invalidHash;
    std::memset(invalidHash.Data(), 2, invalidHash.Size());
    args.clear();
    args.push_back(StackItem::Create(ByteVector(ByteSpan(invalidHash.Data(), invalidHash.Size()))));
    result = ledgerContract->Call(*engine, "getTransaction", args);

    // Check the result
    ASSERT_TRUE(result->IsNull());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
