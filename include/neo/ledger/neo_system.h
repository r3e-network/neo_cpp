#pragma once

#include <neo/config/protocol_settings.h>
#include <neo/persistence/data_cache.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p/local_node.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <memory>
#include <string>

namespace neo::ledger
{
    // Forward declarations
    class MemoryPool;
    class Blockchain;
}

namespace neo::network::p2p
{
    class LocalNode;
}

namespace neo::smartcontract::native
{
    class LedgerContract;
}

namespace neo::ledger
{
    /**
     * @brief Core Neo system class that manages blockchain, mempool, and network components.
     * 
     * ## Overview
     * The NeoSystem class is the central coordinator for all Neo blockchain operations.
     * It manages the blockchain state, transaction pool, network connections, and
     * provides access to native contracts and protocol settings.
     * 
     * ## API Reference
     * - **Blockchain Access**: GetBlockchain(), GetStoreView()
     * - **Memory Pool**: GetMemoryPool()
     * - **Network**: GetLocalNode()
     * - **Settings**: GetSettings()
     * - **Native Contracts**: GetLedgerContract()
     * 
     * ## Usage Examples
     * ```cpp
     * auto system = std::make_shared<NeoSystem>(settings);
     * auto blockchain = system->GetBlockchain();
     * auto mempool = system->GetMemoryPool();
     * auto snapshot = system->GetStoreView();
     * ```
     */
    class NeoSystem
    {
    public:
        /**
         * @brief Constructs a new NeoSystem with the specified settings.
         * @param settings Protocol settings for the Neo network
         */
        explicit NeoSystem(std::shared_ptr<config::ProtocolSettings> settings);
        
        /**
         * @brief Destructor
         */
        ~NeoSystem();

        /**
         * @brief Gets the blockchain instance.
         * @return Shared pointer to the blockchain
         */
        std::shared_ptr<Blockchain> GetBlockchain() const;

        /**
         * @brief Gets the memory pool instance.
         * @return Shared pointer to the memory pool
         */
        std::shared_ptr<MemoryPool> GetMemoryPool() const;

        /**
         * @brief Gets the local network node.
         * @return Shared pointer to the local node
         */
        std::shared_ptr<network::p2p::LocalNode> GetLocalNode() const;

        /**
         * @brief Gets the protocol settings.
         * @return Shared pointer to the protocol settings
         */
        std::shared_ptr<config::ProtocolSettings> GetSettings() const;

        /**
         * @brief Gets a snapshot of the current store state.
         * @return Shared pointer to the data cache snapshot
         */
        std::shared_ptr<persistence::DataCache> GetStoreView() const;

        /**
         * @brief Gets the ledger contract instance.
         * @return Shared pointer to the ledger contract
         */
        std::shared_ptr<smartcontract::native::LedgerContract> GetLedgerContract() const;

        /**
         * @brief Starts the Neo system.
         */
        void Start();

        /**
         * @brief Stops the Neo system.
         */
        void Stop();

        /**
         * @brief Checks if the system is running.
         * @return True if running, false otherwise
         */
        bool IsRunning() const;

        /**
         * @brief Disposes of system resources.
         */
        void Dispose();

    private:
        std::shared_ptr<config::ProtocolSettings> settings_;
        std::shared_ptr<Blockchain> blockchain_;
        std::shared_ptr<MemoryPool> memory_pool_;
        std::shared_ptr<network::p2p::LocalNode> local_node_;
        std::shared_ptr<smartcontract::native::LedgerContract> ledger_contract_;
        bool is_running_;
        bool is_disposed_;

        // Initialize components
        void InitializeComponents();
        void InitializeBlockchain();
        void InitializeMemoryPool();
        void InitializeLocalNode();
        void InitializeLedgerContract();
    };

} // namespace neo::ledger 