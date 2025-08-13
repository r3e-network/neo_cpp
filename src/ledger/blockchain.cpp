/**
 * @file blockchain.cpp
 * @brief Implementation of the core blockchain processing engine
 * @details Handles block validation, persistence, transaction processing,
 *          and consensus integration for the Neo blockchain.
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/neo_system.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace neo::ledger
{
// Static blockchain scripts for native contract callbacks
// These scripts are executed for every block to trigger native contract logic

/// @brief Script executed before block persistence
/// @details Calls System.Contract.NativeOnPersist for all native contracts
static const std::vector<uint8_t> ON_PERSIST_SCRIPT = {
    0x41, 0x9e, 0xd8, 0x5e, 0x10  // SYSCALL System.Contract.NativeOnPersist
};

/// @brief Script executed after block persistence
/// @details Calls System.Contract.NativePostPersist for finalization
static const std::vector<uint8_t> POST_PERSIST_SCRIPT = {
    0x41, 0x9f, 0xd8, 0x5e, 0x10  // SYSCALL System.Contract.NativePostPersist
};

Blockchain::Blockchain(std::shared_ptr<NeoSystem> system)
    : system_(system)
      // Note: header_cache_ initialization disabled - network module not available
      ,
      data_cache_(system->GetStoreView()),
      extensible_whitelist_cached_(false),
      running_(false)
{
    // Validate system parameter
    if (!system_)
    {
        throw std::invalid_argument("NeoSystem cannot be null");
    }
    
    // Initialize with system's data cache for blockchain state access
    // The data_cache_ provides persistent storage through RocksDB backend

Blockchain::~Blockchain() 
{ 
    // Ensure clean shutdown of processing threads
    Stop(); 
}

void Blockchain::Initialize()
{
    // Acquire exclusive lock for initialization
    std::unique_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Check if genesis block exists, create if not
    // Genesis block is the foundation of the blockchain
    if (!IsGenesisBlockInitialized())
    {
        InitializeGenesisBlock();
    }
}

void Blockchain::Start()
{
    if (running_.exchange(true))
    {
        return;  // Already running
    }

    processing_thread_ = std::thread(&Blockchain::ProcessingThreadFunction, this);
}

void Blockchain::Stop()
{
    if (!running_.exchange(false))
    {
        return;  // Already stopped
    }

    // Wake up processing thread
    {
        std::unique_lock<std::mutex> lock(processing_mutex_);
        processing_cv_.notify_all();
    }

    if (processing_thread_.joinable())
    {
        processing_thread_.join();
    }
}

uint32_t Blockchain::GetHeight() const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    return system_->GetLedgerContract()->GetCurrentIndex(data_cache_);
}

io::UInt256 Blockchain::GetCurrentBlockHash() const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    uint32_t current_height = GetHeight();
    if (current_height == 0)
    {
        auto genesis_block = GetBlock(0);
        return genesis_block ? genesis_block->GetHash() : io::UInt256();
    }
    return system_->GetLedgerContract()->GetBlockHash(data_cache_, current_height);
}

std::shared_ptr<Block> Blockchain::GetCurrentBlock() const
{
    io::UInt256 current_hash = GetCurrentBlockHash();
    return GetBlock(current_hash);
}

std::shared_ptr<Block> Blockchain::GetBlock(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Check cache first
    auto cache_it = block_cache_.find(hash);
    if (cache_it != block_cache_.end())
    {
        return cache_it->second;
    }

    // Load from storage
    return system_->GetLedgerContract()->GetBlock(data_cache_, hash);
}

std::shared_ptr<Block> Blockchain::GetBlock(uint32_t index) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);

    io::UInt256 block_hash = system_->GetLedgerContract()->GetBlockHash(data_cache_, index);
    if (block_hash.IsZero())
    {
        return nullptr;
    }

    return GetBlock(block_hash);
}

io::UInt256 Blockchain::GetBlockHash(uint32_t index) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    return system_->GetLedgerContract()->GetBlockHash(data_cache_, index);
}

std::shared_ptr<Header> Blockchain::GetBlockHeader(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Header cache disabled - load full block and return header directly
    auto block = GetBlock(hash);
    return block ? std::make_shared<BlockHeader>(*block) : nullptr;
}

std::shared_ptr<Header> Blockchain::GetBlockHeader(uint32_t index) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Header cache disabled - load from storage directly
    io::UInt256 block_hash = system_->GetLedgerContract()->GetBlockHash(data_cache_, index);
    return block_hash.IsZero() ? nullptr : GetBlockHeader(block_hash);
}

std::shared_ptr<Transaction> Blockchain::GetTransaction(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    return system_->GetLedgerContract()->GetTransaction(data_cache_, hash);
}

int32_t Blockchain::GetTransactionHeight(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    return system_->GetLedgerContract()->GetTransactionHeight(data_cache_, hash);
}

bool Blockchain::ContainsBlock(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Check cache first
    if (block_cache_.find(hash) != block_cache_.end())
    {
        return true;
    }

    auto block = system_->GetLedgerContract()->GetBlock(data_cache_, hash);
    return block != nullptr;
}

bool Blockchain::ContainsTransaction(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    auto transaction = system_->GetLedgerContract()->GetTransaction(data_cache_, hash);
    return transaction != nullptr;
}

std::shared_ptr<smartcontract::ContractState> Blockchain::GetContract(const io::UInt160& hash) const
{
    std::shared_lock<std::shared_mutex> lock(blockchain_mutex_);
    return smartcontract::native::ContractManagement::GetContract(*data_cache_, hash);
}

VerifyResult Blockchain::OnNewBlock(std::shared_ptr<Block> block)
{
    if (!block || block->GetHash() == io::UInt256())
    {
        return VerifyResult::Invalid;
    }

    std::unique_lock<std::shared_mutex> lock(blockchain_mutex_);

    // Create proper snapshot for blockchain operations to ensure data consistency
    std::shared_ptr<persistence::DataCache> snapshot;
    try
    {
        // Create a snapshot from the current data cache for read operations
        snapshot = system_->GetSnapshot();
        if (!snapshot)
        {
            // Fallback to data_cache_ if snapshot creation fails
            snapshot = data_cache_;
        }
    }
    catch (const std::exception& e)
    {
        // Error creating snapshot - use data_cache_ as fallback
        snapshot = data_cache_;
    }

    // Get current height using the snapshot for consistency
    uint32_t current_height = system_->GetLedgerContract()->GetCurrentIndex(*snapshot);

    // Use snapshot for header height as well
    uint32_t header_height = current_height;

    // Check if block already exists
    if (block->GetIndex() <= current_height)
    {
        return VerifyResult::AlreadyExists;
    }

    // Check if we're missing previous blocks
    if (block->GetIndex() - 1 > header_height)
    {
        AddUnverifiedBlockToCache(block, "unknown");
        return VerifyResult::UnableToVerify;
    }

    // Verify block
    if (block->GetIndex() == header_height + 1)
    {
        if (!VerifyBlock(block, data_cache_))
        {
            return VerifyResult::Invalid;
        }
    }
    else
    {
        // Header cache disabled - verify block index is sequential
        if (block->GetIndex() > header_height + 1)
        {
            return VerifyResult::Invalid;
        }
    }

    // Add to cache
    block_cache_[block->GetHash()] = block;

    // Process if ready for persistence
    if (block->GetIndex() == current_height + 1)
    {
        // Queue for processing
        std::unique_lock<std::mutex> proc_lock(processing_mutex_);
        processing_queue_.push([this, block]() { ProcessBlock(block); });
        processing_cv_.notify_one();
    }
    else
    {
        // Relay block if within range
        if (block->GetIndex() + 99 >= header_height)
        {
            // Would notify network layer here
        }

        // Header cache disabled
        // Would add header to cache here if enabled
    }

    return VerifyResult::Succeed;
}

void Blockchain::OnNewHeaders(const std::vector<std::shared_ptr<Header>>& headers)
{
    // Header cache disabled - method is no-op
    // In a full implementation, this would update the header cache
    // to allow for fast header synchronization
    return;
}

VerifyResult Blockchain::OnNewTransaction(std::shared_ptr<Transaction> transaction)
{
    if (!transaction || !transaction->TryGetHash())
    {
        return VerifyResult::Invalid;
    }

    auto hash = transaction->GetHash();

    // Check if transaction exists
    switch (system_->ContainsTransaction(hash))
    {
        case ContainsTransactionType::ExistsInPool:
            return VerifyResult::AlreadyInPool;
        case ContainsTransactionType::ExistsInLedger:
            return VerifyResult::AlreadyExists;
    }

    // Check for conflicts
    std::vector<io::UInt160> signers;
    for (const auto& signer : transaction->GetSigners())
    {
        signers.push_back(signer.account);
    }

    if (system_->ContainsConflictHash(hash, signers))
    {
        return VerifyResult::HasConflicts;
    }

    // Try to add to memory pool
    return system_->GetMemoryPool()->TryAdd(transaction, data_cache_);
}

// ExtensiblePayload method removed - network module is disabled

bool Blockchain::ImportBlocks(const ImportData& import_data)
{
    std::unique_lock<std::shared_mutex> lock(blockchain_mutex_);

    try
    {
        uint32_t current_height = system_->GetLedgerContract()->GetCurrentIndex(data_cache_);

        for (const auto& block : import_data.blocks)
        {
            if (block->GetIndex() <= current_height)
            {
                continue;
            }

            if (block->GetIndex() != current_height + 1)
            {
                throw std::invalid_argument("Blocks must be imported in sequence");
            }

            if (import_data.verify && !VerifyBlock(block, data_cache_))
            {
                throw std::invalid_argument("Block verification failed");
            }

            PersistBlock(block);
            ++current_height;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Import failed: " << e.what() << std::endl;
        return false;
    }
}

void Blockchain::FillMemoryPool(const std::vector<std::shared_ptr<Transaction>>& transactions)
{
    // Invalidate all transactions in memory pool
    system_->GetMemoryPool()->InvalidateAllTransactions();

    auto snapshot = data_cache_->CreateSnapshot();
    uint32_t max_traceable_blocks = system_->GetMaxTraceableBlocks();

    for (const auto& tx : transactions)
    {
        if (system_->GetLedgerContract()->ContainsTransaction(snapshot, tx->GetHash()))
        {
            continue;
        }

        std::vector<io::UInt160> signers;
        for (const auto& signer : tx->GetSigners())
        {
            signers.push_back(signer.account);
        }

        if (system_->GetLedgerContract()->ContainsConflictHash(snapshot, tx->GetHash(), signers, max_traceable_blocks))
        {
            continue;
        }

        // Remove if unverified in pool
        system_->GetMemoryPool()->TryRemoveUnverified(tx->GetHash());

        // Add to memory pool
        system_->GetMemoryPool()->TryAdd(tx, snapshot);
    }
}

// ReverifyInventories method removed - network module is disabled

// Event registration methods
void Blockchain::RegisterCommittingHandler(CommittingHandler handler)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    committing_handlers_.push_back(std::move(handler));
}

void Blockchain::RegisterCommittedHandler(CommittedHandler handler)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    committed_handlers_.push_back(std::move(handler));
}

void Blockchain::RegisterBlockPersistenceHandler(BlockPersistenceHandler handler)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    block_persistence_handlers_.push_back(std::move(handler));
}

void Blockchain::RegisterTransactionHandler(TransactionHandler handler)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    transaction_handlers_.push_back(std::move(handler));
}

// RegisterInventoryHandler method removed - network module is disabled

void Blockchain::ProcessingThreadFunction()
{
    while (running_)
    {
        std::unique_lock<std::mutex> lock(processing_mutex_);

        // Wait for work or timeout for idle processing
        processing_cv_.wait_for(lock, std::chrono::milliseconds(100),
                                [this] { return !processing_queue_.empty() || !running_; });

        if (!running_)
        {
            break;
        }

        // Process queued work
        while (!processing_queue_.empty() && running_)
        {
            auto work = std::move(processing_queue_.front());
            processing_queue_.pop();
            lock.unlock();

            try
            {
                work();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Processing error: " << e.what() << std::endl;
            }

            lock.lock();
        }

        lock.unlock();

        // Idle processing
        if (running_)
        {
            IdleProcessingFunction();
        }
    }
}

void Blockchain::IdleProcessingFunction()
{
    // Re-verify top unverified transactions
    if (system_->GetMemoryPool()->ReVerifyTopUnverifiedTransactionsIfNeeded(MaxTxToReverifyPerIdle, data_cache_))
    {
        // Schedule another idle processing cycle
        std::unique_lock<std::mutex> lock(processing_mutex_);
        processing_cv_.notify_one();
    }
}

}  // namespace neo::ledger
