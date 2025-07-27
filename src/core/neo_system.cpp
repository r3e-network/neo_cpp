// Copyright (C) 2015-2025 The Neo Project.
//
// neo_system.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include "neo/core/neo_system.h"
#include "neo/common/contains_transaction_type.h"
#include "neo/core/logging.h"
#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "neo/ledger/block.h"
#include "neo/ledger/blockchain.h"
#include "neo/ledger/header_cache.h"
#include "neo/ledger/memory_pool.h"
#include "neo/network/p2p/channels_config.h"
#include "neo/network/p2p/local_node.h"
#include "neo/network/p2p/task_manager.h"
#include "neo/persistence/data_cache.h"
#include "neo/persistence/istore.h"
#include "neo/persistence/store_factory.h"
#include "neo/plugins/plugin.h"
#include "neo/protocol_settings.h"
#include "neo/smartcontract/contract.h"
#include "neo/smartcontract/native/ledger_contract.h"
#include "neo/smartcontract/native/native_contract.h"
#include "neo/vm/opcode.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace neo
{

// Production-ready LRU relay cache implementation consistent with C# version
class RelayCache
{
  private:
    struct CacheEntry
    {
        std::string hash;
        std::chrono::steady_clock::time_point access_time;

        CacheEntry(const std::string& h) : hash(h), access_time(std::chrono::steady_clock::now()) {}
    };

    std::unordered_map<std::string, std::list<CacheEntry>::iterator> cache_map_;
    std::list<CacheEntry> lru_list_;
    std::mutex mutex_;
    size_t max_size_;

  public:
    explicit RelayCache(size_t max_size) : max_size_(max_size) {}

    bool contains(const std::string& hash_str)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_map_.find(hash_str);
        if (it != cache_map_.end())
        {
            // Move to front (most recently used)
            auto list_it = it->second;
            list_it->access_time = std::chrono::steady_clock::now();
            lru_list_.splice(lru_list_.begin(), lru_list_, list_it);
            return true;
        }
        return false;
    }

    void add(const std::string& hash_str)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already exists
        auto it = cache_map_.find(hash_str);
        if (it != cache_map_.end())
        {
            // Move to front and update access time
            auto list_it = it->second;
            list_it->access_time = std::chrono::steady_clock::now();
            lru_list_.splice(lru_list_.begin(), lru_list_, list_it);
            return;
        }

        // Implement proper LRU eviction consistent with Neo C# RelayCache
        if (cache_map_.size() >= max_size_)
        {
            // Remove least recently used item (back of list)
            auto lru_entry = lru_list_.back();
            cache_map_.erase(lru_entry.hash);
            lru_list_.pop_back();
        }

        // Add new entry at front (most recently used)
        lru_list_.emplace_front(hash_str);
        cache_map_[hash_str] = lru_list_.begin();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_map_.clear();
        lru_list_.clear();
    }
};

NeoSystem::NeoSystem(std::unique_ptr<ProtocolSettings> settings, const std::string& storage_provider_name,
                     const std::string& storage_path)
    : NeoSystem(std::move(settings), persistence::StoreFactory::get_store_provider(storage_provider_name), storage_path)
{
}

NeoSystem::NeoSystem(std::unique_ptr<ProtocolSettings> settings,
                     std::shared_ptr<persistence::IStoreProvider> storage_provider, const std::string& storage_path)
    : settings_(std::move(settings)), storage_provider_(std::move(storage_provider)),
      relay_cache_(std::make_unique<RelayCache>(100))
{
    if (!settings_)
    {
        throw std::invalid_argument("Settings cannot be null");
    }

    if (!storage_provider_)
    {
        throw std::invalid_argument("Storage provider cannot be null");
    }

    try
    {
        // Initialize core components
        genesis_block_ = create_genesis_block(*settings_);
        store_ = storage_provider_->GetStore(storage_path);
        header_cache_ = std::make_unique<ledger::HeaderCache>();
        mem_pool_ = std::make_unique<ledger::MemoryPool>();

        // Initialize blockchain and network components
        initialize_components();

        // Load plugins and notify them of system initialization
        // TODO: Fix plugin system integration
        // for (auto& plugin : plugins::Plugin::GetPlugins()) {
        //     plugin->on_system_loaded(*this);
        // }

        // Initialize blockchain
        blockchain_->Initialize();

        LOG_INFO("NeoSystem initialized successfully");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to initialize NeoSystem: {}", e.what());
        throw;
    }
}

NeoSystem::~NeoSystem()
{
    stop();
}

void NeoSystem::initialize_components()
{
    // Create blockchain component
    blockchain_ = std::make_unique<ledger::Blockchain>(shared_from_this());

    // LocalNode is singleton - we'll initialize it but not store in unique_ptr
    // The LocalNode instance is managed by the singleton pattern

    // TaskManager needs shared pointers
    // Create a shared_ptr that doesn't own the blockchain (owned by unique_ptr)
    auto blockchain_ptr = std::shared_ptr<ledger::Blockchain>(blockchain_.get(), [](ledger::Blockchain*) {});
    // Create a shared_ptr that doesn't own the mempool (owned by unique_ptr)
    auto mempool_ptr = std::shared_ptr<ledger::MemoryPool>(mem_pool_.get(), [](ledger::MemoryPool*) {});

    task_manager_ = std::make_unique<network::p2p::TaskManager>(blockchain_ptr, mempool_ptr);

    // Start worker threads
    start_worker_threads();
}

void NeoSystem::start_worker_threads()
{
    // Start background processing threads
    worker_threads_.emplace_back(
        [this]()
        {
            while (!shutdown_requested_.load())
            {
                try
                {
                    // Process blockchain operations
                    // blockchain_->process_pending_operations();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Blockchain worker thread error: {}", e.what());
                }
            }
        });

    worker_threads_.emplace_back(
        [this]()
        {
            while (!shutdown_requested_.load())
            {
                try
                {
                    // Process network operations
                    // local_node_->process_pending_operations();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Network worker thread error: {}", e.what());
                }
            }
        });
}

void NeoSystem::stop_worker_threads()
{
    shutdown_requested_.store(true);

    for (auto& thread : worker_threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    worker_threads_.clear();
}

std::unique_ptr<persistence::StoreCache> NeoSystem::store_view() const
{
    return std::make_unique<persistence::StoreCache>(*store_);
}

void NeoSystem::add_service(std::shared_ptr<void> service)
{
    if (!service)
    {
        throw std::invalid_argument("Service cannot be null");
    }

    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        services_.push_back(service);
    }

    // Notify handlers
    for (const auto& handler : service_added_handlers_)
    {
        try
        {
            handler(service);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Service added handler error: {}", e.what());
        }
    }
}

void NeoSystem::on_service_added(ServiceAddedHandler handler)
{
    if (handler)
    {
        service_added_handlers_.push_back(std::move(handler));
    }
}

void NeoSystem::start_node(std::unique_ptr<network::p2p::ChannelsConfig> config)
{
    std::lock_guard<std::mutex> lock(start_message_mutex_);
    start_message_ = std::move(config);

    if (suspend_count_.load() == 0)
    {
        local_node_->Start(*start_message_);
        start_message_.reset();
    }
}

void NeoSystem::suspend_node_startup()
{
    suspend_count_.fetch_add(1);
}

bool NeoSystem::resume_node_startup()
{
    if (suspend_count_.fetch_sub(1) != 1)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(start_message_mutex_);
    if (start_message_)
    {
        local_node_->Start(*start_message_);
        start_message_.reset();
        return true;
    }

    return false;
}

void NeoSystem::stop()
{
    if (shutdown_requested_.load())
    {
        return;  // Already stopping
    }

    LOG_INFO("Stopping NeoSystem...");

    // Stop worker threads first
    stop_worker_threads();

    // Stop components in reverse order of initialization
    ensure_stopped("LocalNode",
                   [this]()
                   {
                       if (local_node_)
                           local_node_->Stop();
                   });

    ensure_stopped("Blockchain",
                   [this]()
                   {
                       if (blockchain_)
                           blockchain_->Stop();
                   });

    // Dispose plugins
    // TODO: Fix plugin system
    // for (auto& plugin : plugins::Plugin::GetPlugins()) {
    //     try {
    //         plugin->dispose();
    //     } catch (const std::exception& e) {
    //         LOG_ERROR("Plugin disposal error: {}", e.what());
    //     }
    // }

    // Clean up caches
    // TODO: Fix HeaderCache incomplete type
    // if (header_cache_) {
    //     header_cache_->clear();
    // }

    if (relay_cache_)
    {
        relay_cache_->clear();
    }

    // Close store
    if (store_)
    {
        // TODO: Check if IStore has a close method
        // store_->close();
    }

    LOG_INFO("NeoSystem stopped");
}

std::unique_ptr<persistence::IStore> NeoSystem::load_store(const std::string& path)
{
    return storage_provider_->GetStore(path);
}

std::unique_ptr<persistence::StoreCache> NeoSystem::get_snapshot_cache()
{
    // IStore doesn't have GetSnapshot, but concrete implementations do
    // For now, create a StoreCache directly from the store
    return std::make_unique<persistence::StoreCache>(*store_);
}

ContainsTransactionType NeoSystem::contains_transaction(const io::UInt256& hash) const
{
    // Check memory pool first
    if (mem_pool_->Contains(hash))
    {
        return ContainsTransactionType::ExistsInPool;
    }

    // Check ledger
    auto store_view = this->store_view();
    // TODO: Implement ledger transaction checking
    // For now, assume transaction doesn't exist in ledger

    return ContainsTransactionType::NotExist;
}

bool NeoSystem::contains_conflict_hash(const io::UInt256& hash, const std::vector<io::UInt160>& signers) const
{
    auto store_view = this->store_view();
    // TODO: Implement proper conflict hash checking
    // For now, return false
    return false;
}

ledger::Block* NeoSystem::create_genesis_block(const ProtocolSettings& settings)
{
    auto block = new ledger::Block();

    // Set header fields
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());

    // Genesis block timestamp: July 15, 2016 15:08:21 UTC
    auto genesis_time = std::chrono::system_clock::from_time_t(1468594101);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(genesis_time.time_since_epoch()).count();
    block->SetTimestamp(genesis_time);

    // Note: Block class doesn't have nonce property
    block->SetIndex(0);
    block->SetPrimaryIndex(0);

    // Set next consensus address
    // TODO: Set proper next consensus address
    // For now, use a placeholder
    block->SetNextConsensus(io::UInt160::Zero());

    // Note: Block class doesn't have witness property directly
    // Witness is handled separately in Neo3

    // Empty transactions array - Block already has empty transactions by default

    return block;
}

void NeoSystem::initialize_plugins()
{
    // TODO: Fix plugin system
    // plugins::Plugin::load_plugins();
}

void NeoSystem::ensure_stopped(const std::string& component_name, std::function<void()> stop_function)
{
    try
    {
        LOG_DEBUG("Stopping {}", component_name);
        stop_function();
        LOG_DEBUG("{} stopped", component_name);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error stopping {}: {}", component_name, e.what());
    }
}

void NeoSystem::handle_unhandled_exception(const std::exception& exception)
{
    LOG_ERROR("Unhandled exception: {}", exception.what());
}

}  // namespace neo