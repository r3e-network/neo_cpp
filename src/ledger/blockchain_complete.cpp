/**
 * @file blockchain_complete.cpp
 * @brief Core blockchain implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/blockchain.h>
#include <neo/persistence/store_factory.h>
#include <neo/smartcontract/native/native_contract_manager.h>

#include <chrono>

namespace neo::ledger
{
// Constructor
Blockchain::Blockchain(std::shared_ptr<NeoSystem> system)
    : system_(system), extensible_whitelist_cached_(false), running_(false)
{
    LOG_INFO("Initializing Blockchain...");

    // Initialize data cache from system's store
    if (system && system->GetStore())
    {
        data_cache_ = std::make_shared<persistence::StoreCache>(*system->GetStore());
    }
}

// Destructor
Blockchain::~Blockchain() { Stop(); }

// Initialize blockchain
void Blockchain::Initialize()
{
    LOG_INFO("Starting Blockchain initialization...");

    // Check if genesis block exists
    if (!IsGenesisBlockInitialized())
    {
        LOG_INFO("Genesis block not found, initializing...");
        InitializeGenesisBlock();
    }

    // Start processing thread
    running_ = true;
    processing_thread_ = std::thread(&Blockchain::ProcessingThreadFunction, this);

    LOG_INFO("Blockchain initialized successfully");
}

// Stop blockchain processing
void Blockchain::Stop()
{
    if (running_)
    {
        LOG_INFO("Stopping Blockchain...");

        running_ = false;
        processing_cv_.notify_all();

        if (processing_thread_.joinable())
        {
            processing_thread_.join();
        }

        LOG_INFO("Blockchain stopped");
    }
}

// GetCurrentBlockIndex() is already defined inline in header as GetHeight()

// Get current block hash
io::UInt256 Blockchain::GetCurrentBlockHash() const
{
    auto index = GetCurrentBlockIndex();
    return GetBlockHash(index);
}

// Get current block
std::shared_ptr<Block> Blockchain::GetCurrentBlock() const
{
    auto index = GetCurrentBlockIndex();
    return GetBlock(index);
}

// Get block by hash
std::shared_ptr<Block> Blockchain::GetBlock(const io::UInt256& hash) const
{
    // Check cache first
    auto it = block_cache_.find(hash);
    if (it != block_cache_.end())
    {
        return it->second;
    }

    // Load from storage
    if (!data_cache_) return nullptr;

    // Ledger contract stores full blocks under contract id -4 with prefix 0x05
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x05), hash);
    auto item = data_cache_->TryGet(key);
    if (!item) return nullptr;

    // Deserialize block
    try
    {
        auto block = std::make_shared<Block>();
        io::BinaryReader reader(item->GetValue().AsSpan());
        block->Deserialize(reader);

        // Cache the block
        const_cast<Blockchain*>(this)->block_cache_[hash] = block;

        return block;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to deserialize block {}: {}", hash.ToString(), e.what());
        return nullptr;
    }
}

// Get block by index
std::shared_ptr<Block> Blockchain::GetBlock(uint32_t index) const
{
    auto hash = GetBlockHash(index);
    if (hash == io::UInt256::Zero()) return nullptr;
    return GetBlock(hash);
}

// Get block hash by index
io::UInt256 Blockchain::GetBlockHash(uint32_t index) const
{
    if (!data_cache_) return io::UInt256::Zero();

    // First, try to find in block cache by index (useful at genesis)
    for (const auto& [h, blk] : block_cache_)
    {
        if (blk && blk->GetIndex() == index)
        {
            return h;
        }
    }

    // Ledger contract index -> hash mapping uses contract id -4, prefix 0x09
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x09), index);
    auto item = data_cache_->TryGet(key);
    if (!item || item->GetValue().Size() != 32) return io::UInt256::Zero();
    return io::UInt256(item->GetValue().AsSpan());
}

// Get block header by hash
std::shared_ptr<Header> Blockchain::GetBlockHeader(const io::UInt256& hash) const
{
    auto block = GetBlock(hash);
    if (!block) return nullptr;

    // Create header from block
    auto header = std::make_shared<BlockHeader>();
    header->SetVersion(block->GetVersion());
    header->SetPrevHash(block->GetPreviousHash());
    header->SetMerkleRoot(block->GetMerkleRoot());
    header->SetTimestamp(block->GetTimestamp());
    header->SetIndex(block->GetIndex());
    header->SetPrimaryIndex(block->GetPrimaryIndex());
    header->SetNextConsensus(block->GetNextConsensus());
    header->SetWitness(block->GetWitness());

    return header;
}

// Get block header by index
std::shared_ptr<Header> Blockchain::GetBlockHeader(uint32_t index) const
{
    auto hash = GetBlockHash(index);
    if (hash == io::UInt256::Zero()) return nullptr;
    return GetBlockHeader(hash);
}

// Get transaction by hash
std::shared_ptr<Transaction> Blockchain::GetTransaction(const io::UInt256& hash) const
{
    if (!data_cache_) return nullptr;

    // Transactions are stored under LedgerContract with prefix 0x01
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x01), hash);
    auto item = data_cache_->TryGet(key);
    if (!item) return nullptr;

    try
    {
        auto tx = std::make_shared<Transaction>();
        io::BinaryReader reader(item->GetValue().AsSpan());
        tx->Deserialize(reader);
        return tx;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to deserialize transaction {}: {}", hash.ToString(), e.what());
        return nullptr;
    }
}

// Get transaction height
int32_t Blockchain::GetTransactionHeight(const io::UInt256& hash) const
{
    if (!data_cache_) return -1;

    // Transaction height stored under LedgerContract (prefix 0x02)
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x02), hash);
    auto item = data_cache_->TryGet(key);
    if (!item || item->GetValue().Size() < 4) return -1;
    return *reinterpret_cast<const int32_t*>(item->GetValue().Data());
}

// Get contract state
std::shared_ptr<smartcontract::ContractState> Blockchain::GetContract(const io::UInt160& hash) const
{
    // This would query the ContractManagement native contract
    // Returns nullptr when contract is not found
    return nullptr;
}

// Check if block exists
bool Blockchain::ContainsBlock(const io::UInt256& hash) const
{
    // Check cache first
    if (block_cache_.find(hash) != block_cache_.end()) return true;

    // Check storage (LedgerContract block prefix 0x05)
    if (!data_cache_) return false;
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x05), hash);
    return data_cache_->TryGet(key) != nullptr;
}

// Check if transaction exists
bool Blockchain::ContainsTransaction(const io::UInt256& hash) const
{
    if (!data_cache_) return false;
    // Transaction content prefix 0x01
    auto key = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x01), hash);
    return data_cache_->TryGet(key) != nullptr;
}

// Process new block
VerifyResult Blockchain::OnNewBlock(std::shared_ptr<Block> block)
{
    if (!block) return VerifyResult::Invalid;

    LOG_INFO("Processing new block: {} at height {}", block->GetHash().ToString(), block->GetIndex());

    // Add to processing queue
    {
        std::lock_guard<std::mutex> lock(processing_mutex_);
        processing_queue_.push([this, block]() { ProcessBlock(block); });
    }
    processing_cv_.notify_one();

    return VerifyResult::Succeed;
}

// Process new headers
void Blockchain::OnNewHeaders(const std::vector<std::shared_ptr<Header>>& headers)
{
    // Process headers
    for (const auto& header : headers)
    {
        LOG_DEBUG("Processing header: {} at height {}", header->GetHash().ToString(), header->GetIndex());
    }
}

// Process new transaction
VerifyResult Blockchain::OnNewTransaction(std::shared_ptr<Transaction> transaction)
{
    if (!transaction) return VerifyResult::Invalid;

    // Forward to memory pool if available
    if (system_ && system_->GetMemPool())
    {
        // Neo N3 transaction processing
        LOG_DEBUG("OnNewTransaction called with Neo N3 transaction");
        return VerifyResult::UnableToVerify;
    }

    return VerifyResult::Succeed;
}

// Import blocks
bool Blockchain::ImportBlocks(const ImportData& import_data)
{
    LOG_INFO("Importing {} blocks...", import_data.blocks.size());

    for (const auto& block : import_data.blocks)
    {
        if (import_data.verify)
        {
            auto result = OnNewBlock(block);
            if (result != VerifyResult::Succeed)
            {
                LOG_ERROR("Failed to import block {}", block->GetHash().ToString());
                return false;
            }
        }
        else
        {
            ProcessBlock(block);
        }
    }

    return true;
}

// Fill memory pool
void Blockchain::FillMemoryPool(const std::vector<std::shared_ptr<Transaction>>& transactions)
{
    if (!system_ || !system_->GetMemPool()) return;

    // Neo N3 transaction processing for memory pool
    LOG_DEBUG("FillMemoryPool called with Neo N3 transactions");
}

// Register event handlers
void Blockchain::RegisterCommittingHandler(CommittingHandler handler) { committing_handlers_.push_back(handler); }

void Blockchain::RegisterCommittedHandler(CommittedHandler handler) { committed_handlers_.push_back(handler); }

void Blockchain::RegisterBlockPersistenceHandler(BlockPersistenceHandler handler)
{
    block_persistence_handlers_.push_back(handler);
}

void Blockchain::RegisterTransactionHandler(TransactionHandler handler) { transaction_handlers_.push_back(handler); }

// Private methods

void Blockchain::ProcessBlock(std::shared_ptr<Block> block)
{
    if (!block) return;

    // Verify block
    auto snapshot = data_cache_->CreateSnapshot();
    if (!VerifyBlock(block, std::dynamic_pointer_cast<persistence::DataCache>(snapshot)))
    {
        LOG_ERROR("Block verification failed: {}", block->GetHash().ToString());
        return;
    }

    // Persist block
    PersistBlock(block);
}

void Blockchain::PersistBlock(std::shared_ptr<Block> block)
{
    LOG_INFO("Persisting block: {} at height {}", block->GetHash().ToString(), block->GetIndex());

    // Create snapshot for block persistence
    auto snapshot = std::dynamic_pointer_cast<persistence::DataCache>(data_cache_->CreateSnapshot());

    // Execute block scripts
    auto appExecuted = ExecuteBlockScripts(block, snapshot);

    // Fire committing event
    FireCommittingEvent(block, snapshot, appExecuted);

    // Let native contracts (LedgerContract, etc.) write their storage via OnPersist (already executed)
    // Now execute PostPersist to update current block state (hash + height)
    try
    {
        auto nativeContracts = smartcontract::native::NativeContractManager::GetInstance().GetContracts();
        for (const auto& contract : nativeContracts)
        {
            try
            {
                auto engine = std::make_shared<smartcontract::ApplicationEngine>(
                    smartcontract::TriggerType::PostPersist, static_cast<const io::ISerializable*>(nullptr), snapshot,
                    block.get(), 0);
                engine->Execute();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Error executing PostPersist for contract {}: {}", contract->GetName(), e.what());
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("PostPersist execution failed: {}", e.what());
    }

    // Commit changes (includes native contract OnPersist/PostPersist storage updates)
    // Ensure essential LedgerContract keys exist even if native contracts are not fully wired
    try
    {
        // Use a write cache bound to the underlying store for direct writes
        persistence::StoreCache write_cache(*system_->GetStore());

        // Persist full block by hash
        {
            auto blockKey = persistence::StorageKey::Create(static_cast<int32_t>(-4),    // Ledger contract ID
                                                            static_cast<uint8_t>(0x05),  // Block prefix
                                                            block->GetHash());
            if (!write_cache.Contains(blockKey))
            {
                io::ByteVector blockData;
                io::BinaryWriter writer(blockData);
                block->Serialize(writer);
                persistence::StorageItem blockItem(blockData);
                write_cache.Add(blockKey, blockItem);
            }
        }

        // Persist index -> hash mapping
        {
            auto indexKey = persistence::StorageKey::Create(static_cast<int32_t>(-4),    // Ledger contract ID
                                                            static_cast<uint8_t>(0x09),  // Block index prefix
                                                            static_cast<uint32_t>(block->GetIndex()));
            if (!write_cache.Contains(indexKey))
            {
                io::ByteVector hashData(32);
                std::memcpy(hashData.Data(), block->GetHash().Data(), 32);
                persistence::StorageItem hashItem(hashData);
                write_cache.Add(indexKey, hashItem);
            }
        }

        // Update current block height
        {
            auto heightKey = persistence::StorageKey::Create(static_cast<int32_t>(-4),    // Ledger contract ID
                                                             static_cast<uint8_t>(0x0C)); // Current block prefix
            io::ByteVector heightData(sizeof(uint32_t));
            *reinterpret_cast<uint32_t*>(heightData.Data()) = block->GetIndex();
            persistence::StorageItem heightItem(heightData);
            if (write_cache.Contains(heightKey))
                write_cache.Update(heightKey, heightItem);
            else
                write_cache.Add(heightKey, heightItem);
        }

        // Persist transactions (content and height)
        for (const auto& tx : block->GetTransactions())
        {
            // Content under prefix 0x01
            auto txKey = persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x01),
                                                         tx.GetHash());
            if (!write_cache.Contains(txKey))
            {
                io::ByteVector txData;
                io::BinaryWriter txWriter(txData);
                tx.Serialize(txWriter);
                persistence::StorageItem txItem(txData);
                write_cache.Add(txKey, txItem);
            }

            // Height under prefix 0x02
            auto txHeightKey =
                persistence::StorageKey::Create(static_cast<int32_t>(-4), static_cast<uint8_t>(0x02), tx.GetHash());
            io::ByteVector hData(sizeof(int32_t));
            *reinterpret_cast<int32_t*>(hData.Data()) = static_cast<int32_t>(block->GetIndex());
            persistence::StorageItem hItem(hData);
            if (write_cache.Contains(txHeightKey))
                write_cache.Update(txHeightKey, hItem);
            else
                write_cache.Add(txHeightKey, hItem);
        }

        // Commit both native-contract writes and direct key writes
        snapshot->Commit();
        write_cache.Commit();
    }
    catch (const std::exception& e)
    {
        LOG_WARNING("Fallback key writes failed during PersistBlock: {}", e.what());
        // Still commit native-contract writes to avoid losing state
        snapshot->Commit();
    }

    // Fire committed event
    FireCommittedEvent(block);

    // Fire block persisted event
    FireBlockPersistedEvent(block);

    // Cache the block
    block_cache_[block->GetHash()] = block;
}

bool Blockchain::VerifyBlock(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot)
{
    // Basic verification
    if (!block) return false;

    // Check if block already exists
    if (ContainsBlock(block->GetHash()))
    {
        return false;
    }

    // Verify previous block exists (except for genesis)
    if (block->GetIndex() > 0)
    {
        if (!ContainsBlock(block->GetPreviousHash()))
        {
            return false;
        }
    }

    // More verification would be added here

    return true;
}

std::vector<ApplicationExecuted> Blockchain::ExecuteBlockScripts(std::shared_ptr<Block> block,
                                                                 std::shared_ptr<persistence::DataCache> snapshot)
{
    std::vector<ApplicationExecuted> results;

    // Execute native contract OnPersist methods
    auto nativeContracts = smartcontract::native::NativeContractManager::GetInstance().GetContracts();
    for (const auto& contract : nativeContracts)
    {
        try
        {
            // Create application engine for native contract execution
            auto engine = std::make_shared<smartcontract::ApplicationEngine>(
                smartcontract::TriggerType::OnPersist, static_cast<const io::ISerializable*>(nullptr), snapshot,
                block.get(), 0);

            // Execute OnPersist
            // Native contracts are loaded automatically by the engine
            auto state = engine->Execute();

            ApplicationExecuted executed;
            executed.engine = engine;
            executed.vm_state = state;
            executed.gas_consumed = engine->GetGasConsumed();

            results.push_back(executed);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error executing OnPersist for contract {}: {}", contract->GetName(), e.what());
        }
    }

    // Execute block transactions
    for (const auto& tx : block->GetTransactions())
    {
        try
        {
            auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application,
                                                                             static_cast<const io::ISerializable*>(&tx),
                                                                             snapshot, block.get(), 0);

            // Execute transaction script
            engine->LoadScript(tx.GetScript());
            auto state = engine->Execute();

            ApplicationExecuted executed;
            executed.transaction = std::make_shared<Transaction>(tx);
            executed.engine = engine;
            executed.vm_state = state;
            executed.gas_consumed = engine->GetGasConsumed();

            results.push_back(executed);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error executing transaction {}: {}", tx.GetHash().ToString(), e.what());
        }
    }

    return results;
}

void Blockchain::FireCommittingEvent(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot,
                                     const std::vector<ApplicationExecuted>& appExecuted)
{
    for (const auto& handler : committing_handlers_)
    {
        try
        {
            handler(system_, block, snapshot, appExecuted);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error in committing handler: {}", e.what());
        }
    }
}

void Blockchain::FireCommittedEvent(std::shared_ptr<Block> block)
{
    for (const auto& handler : committed_handlers_)
    {
        try
        {
            handler(system_, block);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error in committed handler: {}", e.what());
        }
    }
}

void Blockchain::FireBlockPersistedEvent(std::shared_ptr<Block> block)
{
    for (const auto& handler : block_persistence_handlers_)
    {
        try
        {
            handler(block);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error in block persistence handler: {}", e.what());
        }
    }
}

void Blockchain::FireTransactionEvent(std::shared_ptr<Transaction> transaction, VerifyResult result)
{
    for (const auto& handler : transaction_handlers_)
    {
        try
        {
            handler(transaction, result);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error in transaction handler: {}", e.what());
        }
    }
}

bool Blockchain::IsGenesisBlockInitialized() const { return GetBlockHash(0) != io::UInt256::Zero(); }

void Blockchain::InitializeGenesisBlock()
{
    LOG_INFO("Creating genesis block...");

    // Create genesis block
    auto genesis = std::make_shared<Block>();
    genesis->SetVersion(0);
    genesis->SetPreviousHash(io::UInt256::Zero());
    genesis->SetTimestamp(1468595301000);  // Neo genesis time (milliseconds since epoch)
    genesis->SetIndex(0);
    genesis->SetPrimaryIndex(0);
    genesis->SetNextConsensus(io::UInt160::Parse("0x0000000000000000000000000000000000000000"));

    // Persist genesis block
    PersistBlock(genesis);

    LOG_INFO("Genesis block created: {}", genesis->GetHash().ToString());
}

void Blockchain::ProcessingThreadFunction()
{
    LOG_INFO("Blockchain processing thread started");

    while (running_)
    {
        std::unique_lock<std::mutex> lock(processing_mutex_);
        processing_cv_.wait(lock, [this] { return !processing_queue_.empty() || !running_; });

        while (!processing_queue_.empty())
        {
            auto task = processing_queue_.front();
            processing_queue_.pop();
            lock.unlock();

            try
            {
                task();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Error in processing task: {}", e.what());
            }

            lock.lock();
        }
    }

    LOG_INFO("Blockchain processing thread stopped");
}

void Blockchain::IdleProcessingFunction()
{
    // Idle processing for maintenance tasks
}

std::unordered_set<io::UInt160> Blockchain::UpdateExtensibleWitnessWhiteList(
    std::shared_ptr<persistence::DataCache> snapshot)
{
    // Would query RoleManagement native contract for whitelist
    return std::unordered_set<io::UInt160>();
}

void Blockchain::Start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
    {
        return;
    }

    LOG_INFO("Blockchain started");

    // Start block processing threads if needed
    // Processing threads are started when blocks are received
}
}  // namespace neo::ledger
