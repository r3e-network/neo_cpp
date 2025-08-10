#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/network/p2p/remote_node.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace neo
{
class NeoSystem;
}

namespace neo::network::p2p
{
class LocalNode;

/**
 * @brief Manages block synchronization from network peers
 *
 * This class handles:
 * - Initial block download (IBD) from peers
 * - Header synchronization
 * - Block download and validation
 * - Orphan block management
 * - Sync state tracking
 */
class BlockSyncManager
{
   public:
    enum class SyncState
    {
        Idle,
        SyncingHeaders,
        SyncingBlocks,
        Synced
    };

    /**
     * @brief Constructs a BlockSyncManager
     * @param system The Neo system instance
     * @param localNode The local node instance
     */
    BlockSyncManager(std::shared_ptr<NeoSystem> system, LocalNode& localNode);

    /**
     * @brief Destructor
     */
    ~BlockSyncManager();

    /**
     * @brief Starts the synchronization process
     */
    void Start();

    /**
     * @brief Stops the synchronization process
     */
    void Stop();

    /**
     * @brief Gets the current sync state
     * @return The current synchronization state
     */
    SyncState GetSyncState() const;

    /**
     * @brief Gets the synchronization progress percentage
     * @return Progress from 0 to 100
     */
    uint8_t GetSyncProgress() const;

    /**
     * @brief Handles headers received from a peer
     * @param node The remote node that sent the headers
     * @param headers The received headers
     */
    void OnHeadersReceived(RemoteNode* node, const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers);

    /**
     * @brief Handles a block received from a peer
     * @param node The remote node that sent the block
     * @param block The received block
     */
    void OnBlockReceived(RemoteNode* node, std::shared_ptr<ledger::Block> block);

    /**
     * @brief Handles inventory message for blocks
     * @param node The remote node that sent the inventory
     * @param hashes The block hashes in the inventory
     */
    void OnBlockInventory(RemoteNode* node, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Called when a new peer connects
     * @param node The newly connected peer
     */
    void OnPeerConnected(RemoteNode* node);

    /**
     * @brief Called when a peer disconnects
     * @param node The disconnected peer
     */
    void OnPeerDisconnected(RemoteNode* node);

    /**
     * @brief Sets the maximum number of blocks to download concurrently
     * @param maxBlocks Maximum concurrent block downloads
     */
    void SetMaxConcurrentDownloads(uint32_t maxBlocks);

    /**
     * @brief Gets statistics about the sync process
     */
    struct SyncStats
    {
        uint32_t currentHeight;
        uint32_t targetHeight;
        uint32_t headerHeight;
        uint32_t downloadedBlocks;
        uint32_t pendingBlocks;
        uint32_t orphanBlocks;
        std::chrono::steady_clock::time_point startTime;
        double blocksPerSecond;
    };

    /**
     * @brief Gets current synchronization statistics
     * @return Current sync statistics
     */
    SyncStats GetStats() const;

   private:
    // Core components
    std::shared_ptr<NeoSystem> system_;
    LocalNode& localNode_;

    // Sync state
    std::atomic<SyncState> syncState_{SyncState::Idle};
    std::atomic<uint32_t> currentHeight_{0};
    std::atomic<uint32_t> targetHeight_{0};
    std::atomic<uint32_t> headerHeight_{0};

    // Thread management
    std::thread syncThread_;
    std::atomic<bool> running_{false};
    std::condition_variable syncCv_;
    mutable std::mutex syncMutex_;

    // Parallel processing
    static constexpr size_t PROCESSING_THREADS = 8;
    std::vector<std::thread> processingThreads_;
    std::queue<std::vector<std::shared_ptr<ledger::Block>>> blockBatches_;
    mutable std::mutex batchMutex_;
    std::condition_variable batchCv_;
    std::atomic<bool> processingRunning_{false};

    // Block management
    std::unordered_map<io::UInt256, std::shared_ptr<ledger::Block>> orphanBlocks_;
    std::unordered_set<io::UInt256> requestedBlocks_;
    std::unordered_map<io::UInt256, std::chrono::steady_clock::time_point> requestTimestamps_;
    std::queue<io::UInt256> blockDownloadQueue_;
    mutable std::mutex blockMutex_;

    // Block collection for batching
    std::vector<std::shared_ptr<ledger::Block>> pendingBlocks_;
    mutable std::mutex pendingBlocksMutex_;
    static constexpr size_t BATCH_COLLECTION_SIZE = 500;

    // Header management
    std::vector<std::shared_ptr<ledger::BlockHeader>> pendingHeaders_;
    mutable std::mutex headerMutex_;

    // Peer tracking
    struct PeerInfo
    {
        uint32_t lastBlockIndex;
        std::chrono::steady_clock::time_point lastUpdate;
        uint32_t downloadSpeed;  // blocks per second
        bool syncing;
    };
    std::unordered_map<RemoteNode*, PeerInfo> peers_;
    mutable std::mutex peerMutex_;

    // Configuration
    uint32_t maxConcurrentDownloads_{2000};  // Production: download 2000 blocks concurrently
    uint32_t maxOrphanBlocks_{100};
    std::chrono::seconds requestTimeout_{30};

    // Statistics
    std::chrono::steady_clock::time_point syncStartTime_;
    std::atomic<uint32_t> downloadedBlocks_{0};

    // Private methods
    void SyncLoop();
    void RequestHeaders();
    void RequestBlocks();
    void ProcessPendingHeaders();
    void ProcessOrphanBlocks();
    RemoteNode* SelectBestPeer();
    void TimeoutRequests();
    bool IsBlockRequested(const io::UInt256& hash) const;
    void MarkBlockRequested(const io::UInt256& hash);
    void MarkBlockReceived(const io::UInt256& hash);
    void UpdatePeerInfo(RemoteNode* node, uint32_t lastBlockIndex);
    uint32_t GetLocalHeight() const;
    void UpdateSyncState();

    // Parallel processing methods
    void ProcessingThreadWorker();
    void EnqueueBlockBatch(std::vector<std::shared_ptr<ledger::Block>>&& batch);
};

}  // namespace neo::network::p2p