#include <neo/ledger/blockchain.h>
#include <neo/ledger/neo_system.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/script_builder.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/vm_state.h>

#include <chrono>
#include <iostream>

namespace neo::ledger
{
void Blockchain::ProcessBlock(std::shared_ptr<Block> block)
{
    std::unique_lock<std::shared_mutex> lock(blockchain_mutex_);

    try
    {
        // Verify block is still valid for processing
        uint32_t current_height = system_->GetLedgerContract()->GetCurrentIndex(data_cache_);
        if (block->GetIndex() != current_height + 1)
        {
            std::cerr << "Block " << block->GetIndex() << " is no longer next in sequence" << std::endl;
            return;
        }

        // Process continuous blocks
        std::vector<std::shared_ptr<Block>> blocks_to_persist;
        auto current_block = block;

        // Collect all ready blocks
        while (current_block && current_block->GetIndex() == current_height + 1)
        {
            blocks_to_persist.push_back(current_block);

            // Check for next block
            uint32_t next_index = current_block->GetIndex() + 1;

            // Since header_cache is disabled, we can only process blocks we have in cache
            // Look for next block directly in block cache
            std::shared_ptr<Block> next_block = nullptr;
            for (const auto& [hash, cached_block] : block_cache_)
            {
                if (cached_block->GetIndex() == next_index && cached_block->GetPrevHash() == current_block->GetHash())
                {
                    next_block = cached_block;
                    break;
                }
            }

            if (!next_block)
            {
                break;
            }

            current_block = next_block;
            current_height = next_index - 1;
        }

        // Persist blocks
        for (const auto& block_to_persist : blocks_to_persist)
        {
            PersistBlock(block_to_persist);

            // Clean up caches
            block_cache_unverified_.erase(block_to_persist->GetIndex());

            // Check for unverified blocks that may now be ready
            ProcessUnverifiedBlocks(block_to_persist->GetIndex() + 1);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error processing block " << block->GetIndex() << ": " << e.what() << std::endl;
    }
}

void Blockchain::PersistBlock(std::shared_ptr<Block> block)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    try
    {
        auto snapshot = data_cache_->CreateSnapshot();

        // Execute block scripts and get application execution results
        std::vector<ApplicationExecuted> all_application_executed;

        // Execute OnPersist script
        {
            auto engine = smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::OnPersist,
                                                                   nullptr,  // No transaction for system calls
                                                                   snapshot, block, system_->GetSettings(),
                                                                   0  // No gas limit for system calls
            );

            engine->LoadScript(ON_PERSIST_SCRIPT);
            auto vm_state = engine->Execute();

            if (vm_state != smartcontract::VMState::HALT)
            {
                if (engine->GetFaultException())
                {
                    throw *engine->GetFaultException();
                }
                throw std::runtime_error("OnPersist script execution failed");
            }

            ApplicationExecuted app_executed;
            app_executed.transaction = nullptr;
            app_executed.engine = engine;
            app_executed.vm_state = vm_state;
            app_executed.gas_consumed = engine->GetGasConsumed();
            app_executed.logs = engine->GetLogs();
            app_executed.notifications = engine->GetNotifications();

            all_application_executed.push_back(app_executed);
        }

        // Process each transaction in the block
        auto cloned_snapshot = snapshot->CloneCache();

        for (const auto& tx : block->GetTransactions())
        {
            // Execute transaction script
            auto tx_engine =
                smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::Application, tx, cloned_snapshot,
                                                         block, system_->GetSettings(), tx->GetSystemFee());

            tx_engine->LoadScript(tx->GetScript());
            auto tx_vm_state = tx_engine->Execute();

            ApplicationExecuted tx_app_executed;
            tx_app_executed.transaction = tx;
            tx_app_executed.engine = tx_engine;
            tx_app_executed.vm_state = tx_vm_state;
            tx_app_executed.gas_consumed = tx_engine->GetGasConsumed();
            tx_app_executed.logs = tx_engine->GetLogs();
            tx_app_executed.notifications = tx_engine->GetNotifications();

            if (tx_vm_state == smartcontract::VMState::HALT)
            {
                cloned_snapshot->Commit();
                tx_app_executed.exception_message = "";
            }
            else
            {
                cloned_snapshot = snapshot->CloneCache();
                tx_app_executed.exception_message = tx_engine->GetFaultException()
                                                        ? tx_engine->GetFaultException()->what()
                                                        : "Transaction execution failed";
            }

            all_application_executed.push_back(tx_app_executed);
        }

        // Execute PostPersist script
        {
            auto engine = smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::PostPersist, nullptr,
                                                                   snapshot, block, system_->GetSettings(), 0);

            engine->LoadScript(POST_PERSIST_SCRIPT);
            auto vm_state = engine->Execute();

            if (vm_state != smartcontract::VMState::HALT)
            {
                if (engine->GetFaultException())
                {
                    throw *engine->GetFaultException();
                }
                throw std::runtime_error("PostPersist script execution failed");
            }

            ApplicationExecuted app_executed;
            app_executed.transaction = nullptr;
            app_executed.engine = engine;
            app_executed.vm_state = vm_state;
            app_executed.gas_consumed = engine->GetGasConsumed();
            app_executed.logs = engine->GetLogs();
            app_executed.notifications = engine->GetNotifications();

            all_application_executed.push_back(app_executed);
        }

        // Fire committing event
        FireCommittingEvent(block, snapshot, all_application_executed);

        // Commit the snapshot
        snapshot->Commit();

        // Update memory pool
        system_->GetMemoryPool()->UpdatePoolForBlockPersisted(block, data_cache_);

        // Clear extensible witness whitelist cache
        extensible_whitelist_cached_ = false;

        // Remove previous block from cache
        if (block->GetIndex() > 0)
        {
            block_cache_.erase(block->GetPrevHash());
        }

        // Header cache disabled - no need to remove headers

        // Fire events
        FireCommittedEvent(block);
        FireBlockPersistedEvent(block);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "Block " << block->GetIndex() << " persisted in " << duration.count() << "ms" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error persisting block " << block->GetIndex() << ": " << e.what() << std::endl;
        throw;
    }
}

bool Blockchain::VerifyBlock(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot)
{
    try
    {
        // Basic block verification
        if (!block->Verify(system_->GetSettings(), snapshot))
        {
            return false;
        }

        // Header cache disabled - verification done above

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Block verification failed: " << e.what() << std::endl;
        return false;
    }
}

void Blockchain::AddUnverifiedBlockToCache(std::shared_ptr<Block> block, const std::string& node_id)
{
    // Limit cache size
    if (block_cache_unverified_.size() >= MaxUnverifiedBlocks)
    {
        // Remove oldest entries
        auto oldest_it = std::min_element(block_cache_unverified_.begin(), block_cache_unverified_.end());
        if (oldest_it != block_cache_unverified_.end())
        {
            block_cache_unverified_.erase(oldest_it);
        }
    }

    uint32_t block_index = block->GetIndex();
    auto& unverified_list = block_cache_unverified_[block_index];

    if (!unverified_list)
    {
        unverified_list = std::make_shared<UnverifiedBlocksList>();
    }

    // Check if block with same hash already exists
    for (const auto& existing_block : unverified_list->blocks)
    {
        if (existing_block->GetHash() == block->GetHash())
        {
            return;  // Already cached
        }
    }

    // Check node conflicts
    if (unverified_list->nodes.find(node_id) != unverified_list->nodes.end())
    {
        // Same node sending different block for same height - suspicious
        std::cerr << "Node " << node_id << " sent conflicting blocks for height " << block_index << std::endl;
        return;
    }

    unverified_list->blocks.push_back(block);
    unverified_list->nodes.insert(node_id);
}

void Blockchain::ProcessUnverifiedBlocks(uint32_t height)
{
    auto it = block_cache_unverified_.find(height);
    if (it == block_cache_unverified_.end())
    {
        return;
    }

    auto unverified_list = it->second;
    block_cache_unverified_.erase(it);

    // Process all unverified blocks for this height
    for (const auto& unverified_block : unverified_list->blocks)
    {
        // Queue for processing without relaying
        std::unique_lock<std::mutex> proc_lock(processing_mutex_);
        processing_queue_.push([this, unverified_block]() { OnNewBlock(unverified_block); });
        processing_cv_.notify_one();
    }
}

std::vector<ApplicationExecuted> Blockchain::ExecuteBlockScripts(std::shared_ptr<Block> block,
                                                                 std::shared_ptr<persistence::DataCache> snapshot)
{
    std::vector<ApplicationExecuted> results;

    try
    {
        // This method is used for testing and verification
        // The actual execution is done in PersistBlock

        // Execute OnPersist
        auto on_persist_engine = smartcontract::ApplicationEngine::Create(
            smartcontract::TriggerType::OnPersist, nullptr, snapshot, block, system_->GetSettings(), 0);

        on_persist_engine->LoadScript(ON_PERSIST_SCRIPT);
        auto on_persist_state = on_persist_engine->Execute();

        ApplicationExecuted on_persist_result;
        on_persist_result.transaction = nullptr;
        on_persist_result.engine = on_persist_engine;
        on_persist_result.vm_state = on_persist_state;
        on_persist_result.gas_consumed = on_persist_engine->GetGasConsumed();
        on_persist_result.logs = on_persist_engine->GetLogs();
        on_persist_result.notifications = on_persist_engine->GetNotifications();

        results.push_back(on_persist_result);

        // Execute transactions
        for (const auto& tx : block->GetTransactions())
        {
            auto tx_engine =
                smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::Application, tx, snapshot, block,
                                                         system_->GetSettings(), tx->GetSystemFee());

            tx_engine->LoadScript(tx->GetScript());
            auto tx_state = tx_engine->Execute();

            ApplicationExecuted tx_result;
            tx_result.transaction = tx;
            tx_result.engine = tx_engine;
            tx_result.vm_state = tx_state;
            tx_result.gas_consumed = tx_engine->GetGasConsumed();
            tx_result.logs = tx_engine->GetLogs();
            tx_result.notifications = tx_engine->GetNotifications();

            results.push_back(tx_result);
        }

        // Execute PostPersist
        auto post_persist_engine = smartcontract::ApplicationEngine::Create(
            smartcontract::TriggerType::PostPersist, nullptr, snapshot, block, system_->GetSettings(), 0);

        post_persist_engine->LoadScript(POST_PERSIST_SCRIPT);
        auto post_persist_state = post_persist_engine->Execute();

        ApplicationExecuted post_persist_result;
        post_persist_result.transaction = nullptr;
        post_persist_result.engine = post_persist_engine;
        post_persist_result.vm_state = post_persist_state;
        post_persist_result.gas_consumed = post_persist_engine->GetGasConsumed();
        post_persist_result.logs = post_persist_engine->GetLogs();
        post_persist_result.notifications = post_persist_engine->GetNotifications();

        results.push_back(post_persist_result);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error executing block scripts: " << e.what() << std::endl;
    }

    return results;
}

void Blockchain::FireCommittingEvent(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot,
                                     const std::vector<ApplicationExecuted>& app_executed)
{
    std::lock_guard<std::mutex> lock(event_mutex_);

    for (const auto& handler : committing_handlers_)
    {
        try
        {
            handler(system_, block, snapshot, app_executed);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error in committing handler: " << e.what() << std::endl;
        }
    }
}

void Blockchain::FireCommittedEvent(std::shared_ptr<Block> block)
{
    std::lock_guard<std::mutex> lock(event_mutex_);

    for (const auto& handler : committed_handlers_)
    {
        try
        {
            handler(system_, block);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error in committed handler: " << e.what() << std::endl;
        }
    }
}

void Blockchain::FireBlockPersistedEvent(std::shared_ptr<Block> block)
{
    std::lock_guard<std::mutex> lock(event_mutex_);

    for (const auto& handler : block_persistence_handlers_)
    {
        try
        {
            handler(block);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error in block persistence handler: " << e.what() << std::endl;
        }
    }
}

void Blockchain::FireTransactionEvent(std::shared_ptr<Transaction> transaction, VerifyResult result)
{
    std::lock_guard<std::mutex> lock(event_mutex_);

    for (const auto& handler : transaction_handlers_)
    {
        try
        {
            handler(transaction, result);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error in transaction handler: " << e.what() << std::endl;
        }
    }
}

// FireInventoryEvent removed - network module is disabled

}  // namespace neo::ledger