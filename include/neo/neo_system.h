#pragma once

#include <neo/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/ledger/header_cache.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/task_manager.h>
#include <neo/network/p2p/transaction_router.h>
#include <neo/plugins/plugin_manager.h>
#include <neo/persistence/data_cache.h>
#include <neo/consensus/consensus_service.h>
#include <neo/io/uint256.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <atomic>
#include <thread>

namespace neo
{
    /**
     * @brief Cache for preventing message relay flooding
     */
    class RelayCache
    {
    public:
        explicit RelayCache(size_t capacity = 100);
        
        /**
         * @brief Try to add a hash to the cache
         * @param hash The hash to add
         * @return True if added (not already present), false if already exists
         */
        bool TryAdd(const io::UInt256& hash);
        
        /**
         * @brief Check if hash exists in cache
         * @param hash The hash to check
         * @return True if exists, false otherwise
         */
        bool Contains(const io::UInt256& hash) const;
        
        /**
         * @brief Clear the cache
         */
        void Clear();
        
    private:
        mutable std::mutex mutex_;
        std::unordered_set<io::UInt256> cache_;
        size_t capacity_;
        
        void RemoveOldest();
    };

    /**
     * @brief Service container for dependency injection
     */
    class ServiceContainer
    {
    public:
        /**
         * @brief Register a service
         * @tparam T Service type
         * @param service The service instance
         */
        template<typename T>
        void Register(std::shared_ptr<T> service)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            services_[std::type_index(typeid(T))] = service;
        }
        
        /**
         * @brief Resolve a service
         * @tparam T Service type
         * @param filter Optional filter function
         * @return The service instance or nullptr if not found
         */
        template<typename T>
        std::shared_ptr<T> Resolve(std::function<bool(std::shared_ptr<T>)> filter = nullptr)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = services_.find(std::type_index(typeid(T)));
            if (it != services_.end())
            {
                auto service = std::static_pointer_cast<T>(it->second);
                if (!filter || filter(service))
                    return service;
            }
            return nullptr;
        }
        
        /**
         * @brief Check if service is registered
         * @tparam T Service type
         * @return True if registered, false otherwise
         */
        template<typename T>
        bool Contains() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return services_.find(std::type_index(typeid(T))) != services_.end();
        }
        
    private:
        mutable std::mutex mutex_;
        std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
    };

    /**
     * @brief Network configuration for starting the node
     */
    struct NetworkConfig
    {
        uint16_t port = 10333;
        std::vector<std::string> seedNodes;
        uint32_t maxConnections = 10;
        bool upnpEnabled = true;
        std::string bindAddress = "0.0.0.0";
    };

    /**
     * @brief Event handler for system events
     */
    class SystemEventHandler
    {
    public:
        virtual ~SystemEventHandler() = default;
        virtual void OnServiceAdded(std::shared_ptr<void> service, const std::type_info& type) {}
        virtual void OnBlockPersisted(std::shared_ptr<ledger::Block> block) {}
        virtual void OnTransactionConfirmed(std::shared_ptr<ledger::Transaction> transaction) {}
    };

    /**
     * @brief Main Neo system orchestrator - equivalent to C# NeoSystem
     * 
     * This class coordinates all major components of the Neo node and provides
     * a unified interface for system-wide operations.
     */
    class NeoSystem
    {
    public:
        /**
         * @brief Event fired when a service is added to the system
         */
        std::function<void(std::shared_ptr<void>, const std::type_info&)> ServiceAdded;

        /**
         * @brief Construct a new NeoSystem
         * @param settings Protocol settings
         * @param storageProvider Storage provider for blockchain data
         * @param storagePath Path for storage files
         */
        NeoSystem(std::shared_ptr<ProtocolSettings> settings,
                 std::shared_ptr<persistence::IStoreProvider> storageProvider = nullptr,
                 const std::string& storagePath = "");

        /**
         * @brief Destructor - ensures clean shutdown
         */
        ~NeoSystem();

        // Disable copy and move
        NeoSystem(const NeoSystem&) = delete;
        NeoSystem& operator=(const NeoSystem&) = delete;
        NeoSystem(NeoSystem&&) = delete;
        NeoSystem& operator=(NeoSystem&&) = delete;

        /**
         * @brief Get protocol settings
         * @return Protocol settings
         */
        std::shared_ptr<ProtocolSettings> GetSettings() const { return settings_; }

        /**
         * @brief Get genesis block
         * @return Genesis block
         */
        std::shared_ptr<ledger::Block> GetGenesisBlock() const { return genesisBlock_; }

        /**
         * @brief Get blockchain instance
         * @return Blockchain
         */
        std::shared_ptr<ledger::Blockchain> GetBlockchain() const { return blockchain_; }

        /**
         * @brief Get memory pool
         * @return Memory pool
         */
        std::shared_ptr<ledger::MemoryPool> GetMemoryPool() const { return memoryPool_; }

        /**
         * @brief Get header cache
         * @return Header cache
         */
        std::shared_ptr<ledger::HeaderCache> GetHeaderCache() const { return headerCache_; }

        /**
         * @brief Get local node
         * @return Local node
         */
        std::shared_ptr<network::p2p::LocalNode> GetLocalNode() const { return localNode_; }

        /**
         * @brief Get task manager
         * @return Task manager
         */
        std::shared_ptr<network::p2p::TaskManager> GetTaskManager() const { return taskManager_; }

        /**
         * @brief Get transaction router
         * @return Transaction router
         */
        std::shared_ptr<network::p2p::TransactionRouter> GetTransactionRouter() const { return transactionRouter_; }

        /**
         * @brief Get relay cache
         * @return Relay cache
         */
        std::shared_ptr<RelayCache> GetRelayCache() const { return relayCache_; }

        /**
         * @brief Get plugin manager
         * @return Plugin manager
         */
        std::shared_ptr<plugins::PluginManager> GetPluginManager() const { return pluginManager_; }

        /**
         * @brief Get a readonly view of the blockchain state
         * @return Store view
         */
        std::shared_ptr<persistence::DataCache> GetStoreView() const;

        /**
         * @brief Get a snapshot of the blockchain state for execution
         * @return Store snapshot
         */
        std::shared_ptr<persistence::DataCache> GetSnapshotCache() const;

        /**
         * @brief Initialize the system with genesis block
         * @param genesisBlock The genesis block (optional, will use default if null)
         */
        void Initialize(std::shared_ptr<ledger::Block> genesisBlock = nullptr);

        /**
         * @brief Start the node with network configuration
         * @param config Network configuration
         */
        void Start(const NetworkConfig& config);

        /**
         * @brief Stop the node gracefully
         */
        void Stop();

        /**
         * @brief Dispose of all resources
         */
        void Dispose();

        /**
         * @brief Add a service to the system
         * @tparam T Service type
         * @param service Service instance
         */
        template<typename T>
        void AddService(std::shared_ptr<T> service)
        {
            serviceContainer_.Register<T>(service);
            if (ServiceAdded)
                ServiceAdded(service, typeid(T));
        }

        /**
         * @brief Get a service from the system
         * @tparam T Service type
         * @param filter Optional filter function
         * @return Service instance or nullptr
         */
        template<typename T>
        std::shared_ptr<T> GetService(std::function<bool(std::shared_ptr<T>)> filter = nullptr)
        {
            return serviceContainer_.Resolve<T>(filter);
        }

        /**
         * @brief Check if a transaction exists in memory pool or blockchain
         * @param hash Transaction hash
         * @return Transaction existence status
         */
        ledger::ContainsTransactionType ContainsTransaction(const io::UInt256& hash) const;

        /**
         * @brief Check if a transaction conflicts with on-chain transactions
         * @param hash Transaction hash
         * @param signers Transaction signers
         * @return True if conflicts exist
         */
        bool ContainsConflictHash(const io::UInt256& hash, 
                                 const std::vector<io::UInt160>& signers) const;

        /**
         * @brief Suspend node startup (for plugins)
         */
        void SuspendNodeStartup();

        /**
         * @brief Resume node startup
         * @return True if startup was resumed
         */
        bool ResumeNodeStartup();

        /**
         * @brief Check if the system is running
         * @return True if running
         */
        bool IsRunning() const { return isRunning_.load(); }

        /**
         * @brief Add event handler
         * @param handler Event handler
         */
        void AddEventHandler(std::shared_ptr<SystemEventHandler> handler);

        /**
         * @brief Remove event handler
         * @param handler Event handler
         */
        void RemoveEventHandler(std::shared_ptr<SystemEventHandler> handler);

    private:
        // Core settings and configuration
        std::shared_ptr<ProtocolSettings> settings_;
        std::shared_ptr<ledger::Block> genesisBlock_;
        
        // Storage and persistence
        std::shared_ptr<persistence::IStoreProvider> storeProvider_;
        std::shared_ptr<persistence::IStore> store_;
        std::string storagePath_;
        
        // Core components
        std::shared_ptr<ledger::Blockchain> blockchain_;
        std::shared_ptr<ledger::MemoryPool> memoryPool_;
        std::shared_ptr<ledger::HeaderCache> headerCache_;
        std::shared_ptr<RelayCache> relayCache_;
        
        // Network components
        std::shared_ptr<network::p2p::LocalNode> localNode_;
        std::shared_ptr<network::p2p::TaskManager> taskManager_;
        std::shared_ptr<network::p2p::TransactionRouter> transactionRouter_;
        
        // System management
        std::shared_ptr<plugins::PluginManager> pluginManager_;
        ServiceContainer serviceContainer_;
        
        // Event handling
        std::vector<std::shared_ptr<SystemEventHandler>> eventHandlers_;
        mutable std::mutex eventHandlersMutex_;
        
        // State management
        std::atomic<bool> isRunning_{false};
        std::atomic<bool> isInitialized_{false};
        std::atomic<int32_t> suspendCount_{0};
        NetworkConfig pendingStartConfig_;
        bool hasPendingStart_{false};
        mutable std::mutex startMutex_;
        
        // Background threads
        std::vector<std::thread> backgroundThreads_;
        std::atomic<bool> shouldStop_{false};
        
        // Private methods
        void CreateGenesisBlock();
        void InitializeComponents();
        void StartComponents(const NetworkConfig& config);
        void StopComponents();
        void RunBackgroundTasks();
        void NotifyServiceAdded(std::shared_ptr<void> service, const std::type_info& type);
        void NotifyBlockPersisted(std::shared_ptr<ledger::Block> block);
        void NotifyTransactionConfirmed(std::shared_ptr<ledger::Transaction> transaction);
    };
}