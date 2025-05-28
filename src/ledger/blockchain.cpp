#include <neo/ledger/blockchain.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/cryptography/hash.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <stdexcept>
#include <iostream>
#include <sstream>

namespace neo::ledger
{
    // Storage prefixes
    static const uint8_t HeightPrefix = 0x08;
    static const uint8_t CurrentBlockHashPrefix = 0x09;

    Blockchain::Blockchain(std::shared_ptr<persistence::DataCache> dataCache)
        : dataCache_(dataCache),
          blockStorage_(std::make_shared<BlockStorage>(dataCache)),
          transactionStorage_(std::make_shared<TransactionStorage>(dataCache)),
          callbacks_(std::make_shared<BlockchainCallbacks>()),
          execution_(std::make_shared<BlockchainExecution>(callbacks_)),
          height_(0)
    {
        // Load the current height
        persistence::StorageKey heightKey(io::UInt160(), io::ByteVector{HeightPrefix});
        auto heightItem = dataCache_->TryGet(heightKey);
        if (heightItem)
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(heightItem->GetValue().Data()), heightItem->GetValue().Size()));
            io::BinaryReader reader(stream);
            height_ = reader.ReadUInt32();
        }

        // Load the current block hash
        persistence::StorageKey currentBlockHashKey(io::UInt160(), io::ByteVector{CurrentBlockHashPrefix});
        auto currentBlockHashItem = dataCache_->TryGet(currentBlockHashKey);
        if (currentBlockHashItem)
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(currentBlockHashItem->GetValue().Data()), currentBlockHashItem->GetValue().Size()));
            io::BinaryReader reader(stream);
            currentBlockHash_ = reader.ReadUInt256();
        }
    }

    uint32_t Blockchain::GetHeight() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return height_;
    }

    io::UInt256 Blockchain::GetCurrentBlockHash() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentBlockHash_;
    }

    std::shared_ptr<BlockHeader> Blockchain::GetCurrentBlockHeader() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return blockStorage_->GetBlockHeader(currentBlockHash_);
    }

    std::shared_ptr<Block> Blockchain::GetCurrentBlock() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return blockStorage_->GetBlock(currentBlockHash_);
    }

    std::shared_ptr<Block> Blockchain::GetBlock(const io::UInt256& hash) const
    {
        return blockStorage_->GetBlock(hash);
    }

    std::shared_ptr<Block> Blockchain::GetBlock(uint32_t index) const
    {
        return blockStorage_->GetBlock(index);
    }

    std::shared_ptr<BlockHeader> Blockchain::GetBlockHeader(const io::UInt256& hash) const
    {
        return blockStorage_->GetBlockHeader(hash);
    }

    std::shared_ptr<BlockHeader> Blockchain::GetBlockHeader(uint32_t index) const
    {
        return blockStorage_->GetBlockHeader(index);
    }

    std::shared_ptr<Transaction> Blockchain::GetTransaction(const io::UInt256& hash) const
    {
        return transactionStorage_->GetTransaction(hash);
    }

    VerifyResult Blockchain::OnNewBlock(const Block& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Verify the block
        if (!block.Verify())
            return VerifyResult::Invalid;

        // Check if the block already exists
        if (blockStorage_->ContainsBlock(block.GetHash()))
            return VerifyResult::AlreadyExists;

        // Check if the previous block exists
        if (block.GetIndex() > 0 && !blockStorage_->ContainsBlock(block.GetPrevHash()))
            return VerifyResult::UnableToVerify;

        // Check if the block is valid
        if (block.GetIndex() != height_ + 1)
            return VerifyResult::Invalid;

        // Add the block
        if (!AddBlock(block))
            return VerifyResult::Invalid;

        return VerifyResult::Succeed;
    }

    VerifyResult Blockchain::OnNewTransaction(const Transaction& transaction)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Verify the transaction
        if (!transaction.Verify())
            return VerifyResult::Invalid;

        // Check if the transaction already exists
        if (transactionStorage_->ContainsTransaction(transaction.GetHash()))
            return VerifyResult::AlreadyExists;

        // Notify transaction execution
        auto txPtr = std::make_shared<Transaction>(transaction);
        callbacks_->NotifyTransactionExecution(txPtr);

        return VerifyResult::Succeed;
    }

    bool Blockchain::AddBlock(const Block& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the block already exists
        if (blockStorage_->ContainsBlock(block.GetHash()))
            return false;

        // Check if the block is valid
        if (block.GetIndex() != height_ + 1)
            return false;

        // Create a snapshot
        auto snapshot = dataCache_->CreateSnapshot();
        auto dataSnapshot = std::dynamic_pointer_cast<persistence::DataCache>(snapshot);

        // Add the block to storage
        if (!blockStorage_->AddBlock(block, dataSnapshot))
            return false;

        // Add the transactions to storage
        for (const auto& tx : block.GetTransactions())
        {
            if (!transactionStorage_->AddTransaction(*tx, dataSnapshot))
                return false;
        }

        // Execute the block
        if (!execution_->ExecuteBlock(block, dataSnapshot))
            return false;

        // Update the current height
        if (block.GetIndex() > height_)
        {
            height_ = block.GetIndex();
            std::ostringstream heightStream;
            io::BinaryWriter heightWriter(heightStream);
            heightWriter.Write(height_);
            std::string heightData = heightStream.str();

            persistence::StorageKey heightKey(io::UInt160(), io::ByteVector{HeightPrefix});
            persistence::StorageItem heightItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(heightData.data()), heightData.size())));
            dataSnapshot->Add(heightKey, heightItem);

            // Update the current block hash
            currentBlockHash_ = block.GetHash();
            std::ostringstream currentBlockHashStream;
            io::BinaryWriter currentBlockHashWriter(currentBlockHashStream);
            currentBlockHashWriter.Write(currentBlockHash_);
            std::string currentBlockHashData = currentBlockHashStream.str();

            persistence::StorageKey currentBlockHashKey(io::UInt160(), io::ByteVector{CurrentBlockHashPrefix});
            persistence::StorageItem currentBlockHashItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(currentBlockHashData.data()), currentBlockHashData.size())));
            dataSnapshot->Add(currentBlockHashKey, currentBlockHashItem);
        }

        // Commit the changes
        dataSnapshot->Commit();

        // Notify block persistence
        callbacks_->NotifyBlockPersistence(std::make_shared<Block>(block));

        return true;
    }

    bool Blockchain::AddBlockHeader(const BlockHeader& header)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Verify the block header
        if (!header.Verify())
            return false;

        // Check if the block header already exists
        if (blockStorage_->ContainsBlock(header.GetHash()))
            return false;

        // Create a snapshot
        auto snapshot = dataCache_->CreateSnapshot();
        auto dataSnapshot = std::dynamic_pointer_cast<persistence::DataCache>(snapshot);

        // Add the block header to storage
        if (!blockStorage_->AddBlockHeader(header, dataSnapshot))
            return false;

        // Commit the changes
        dataSnapshot->Commit();

        return true;
    }

    bool Blockchain::AddTransaction(const Transaction& transaction)
    {
        // Transactions are only added as part of blocks
        return false;
    }

    bool Blockchain::ContainsBlock(const io::UInt256& hash) const
    {
        return blockStorage_->ContainsBlock(hash);
    }

    bool Blockchain::ContainsTransaction(const io::UInt256& hash) const
    {
        return transactionStorage_->ContainsTransaction(hash);
    }

    std::optional<io::UInt256> Blockchain::GetBlockHash(uint32_t index) const
    {
        return blockStorage_->GetBlockHash(index);
    }

    std::optional<io::UInt256> Blockchain::GetNextBlockHash(const io::UInt256& hash) const
    {
        return blockStorage_->GetNextBlockHash(hash);
    }

    std::vector<TransactionOutput> Blockchain::GetUnspentOutputs(const io::UInt256& hash) const
    {
        return transactionStorage_->GetUnspentOutputs(hash);
    }

    std::vector<TransactionOutput> Blockchain::GetUnspentOutputs(const io::UInt160& scriptHash) const
    {
        return transactionStorage_->GetUnspentOutputs(scriptHash);
    }

    io::Fixed8 Blockchain::GetBalance(const io::UInt160& scriptHash, const io::UInt256& assetId) const
    {
        return transactionStorage_->GetBalance(scriptHash, assetId);
    }

    int32_t Blockchain::RegisterBlockPersistenceCallback(BlockPersistenceCallback callback)
    {
        return callbacks_->RegisterBlockPersistenceCallback(std::move(callback));
    }

    void Blockchain::UnregisterBlockPersistenceCallback(int32_t id)
    {
        callbacks_->UnregisterBlockPersistenceCallback(id);
    }

    int32_t Blockchain::RegisterTransactionExecutionCallback(TransactionExecutionCallback callback)
    {
        return callbacks_->RegisterTransactionExecutionCallback(std::move(callback));
    }

    void Blockchain::UnregisterTransactionExecutionCallback(int32_t id)
    {
        callbacks_->UnregisterTransactionExecutionCallback(id);
    }

    bool Blockchain::ExecuteBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot)
    {
        return execution_->ExecuteBlock(block, snapshot);
    }

    void Blockchain::Initialize(const Block& genesisBlock)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the blockchain is already initialized
        if (height_ > 0)
            return;

        // Add the genesis block
        if (!AddBlock(genesisBlock))
            throw std::runtime_error("Failed to add genesis block");

        // Initialize the blockchain execution
        auto snapshot = dataCache_->CreateSnapshot();
        auto dataSnapshot = std::dynamic_pointer_cast<persistence::DataCache>(snapshot);
        execution_->Initialize(dataSnapshot);
        dataSnapshot->Commit();
    }
}
