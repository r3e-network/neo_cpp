#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/remote_node.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Represents the synchronization state.
 */
enum class SynchronizationState
{
    /**
     * @brief Not synchronizing.
     */
    NotSynchronizing,

    /**
     * @brief Synchronizing headers.
     */
    SynchronizingHeaders,

    /**
     * @brief Synchronizing blocks.
     */
    SynchronizingBlocks,

    /**
     * @brief Synchronized.
     */
    Synchronized
};

/**
 * @brief Handles network synchronization.
 */
class NetworkSynchronizer
{
  public:
    /**
     * @brief Constructs a NetworkSynchronizer.
     * @param localNode The local node.
     * @param blockchain The blockchain.
     */
    NetworkSynchronizer(LocalNode& localNode, std::shared_ptr<ledger::Blockchain> blockchain);

    /**
     * @brief Destructor.
     */
    ~NetworkSynchronizer();

    /**
     * @brief Starts the synchronizer.
     */
    void Start();

    /**
     * @brief Stops the synchronizer.
     */
    void Stop();

    /**
     * @brief Gets the synchronization state.
     * @return The synchronization state.
     */
    SynchronizationState GetState() const;

    /**
     * @brief Gets the current block index.
     * @return The current block index.
     */
    uint32_t GetCurrentBlockIndex() const;

    /**
     * @brief Gets the target block index.
     * @return The target block index.
     */
    uint32_t GetTargetBlockIndex() const;

    /**
     * @brief Sets the block received callback.
     * @param callback The callback.
     */
    void SetBlockReceivedCallback(std::function<void(const std::shared_ptr<ledger::Block>&)> callback);

    /**
     * @brief Sets the transaction received callback.
     * @param callback The callback.
     */
    void SetTransactionReceivedCallback(std::function<void(const std::shared_ptr<ledger::Transaction>&)> callback);

    /**
     * @brief Sets the synchronization state changed callback.
     * @param callback The callback.
     */
    void SetStateChangedCallback(std::function<void(SynchronizationState)> callback);

    /**
     * @brief Called when an inv message is received.
     * @param remoteNode The remote node.
     * @param payload The inv payload.
     */
    void OnInvMessageReceived(RemoteNode* remoteNode, const payloads::InvPayload& payload);

    /**
     * @brief Called when a block message is received.
     * @param remoteNode The remote node.
     * @param block The block.
     */
    void OnBlockMessageReceived(RemoteNode* remoteNode, const std::shared_ptr<ledger::Block>& block);

    /**
     * @brief Called when a transaction message is received.
     * @param remoteNode The remote node.
     * @param transaction The transaction.
     */
    void OnTransactionMessageReceived(RemoteNode* remoteNode, const std::shared_ptr<ledger::Transaction>& transaction);

    /**
     * @brief Called when a headers message is received.
     * @param remoteNode The remote node.
     * @param payload The headers payload.
     */
    void OnHeadersMessageReceived(RemoteNode* remoteNode, const payloads::HeadersPayload& payload);

  private:
    LocalNode& localNode_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::atomic<SynchronizationState> state_;
    std::atomic<uint32_t> currentBlockIndex_;
    std::atomic<uint32_t> targetBlockIndex_;
    std::atomic<bool> running_;
    std::thread syncThread_;

    std::unordered_set<io::UInt256> knownHashes_;
    std::unordered_map<io::UInt256, std::shared_ptr<ledger::Block>> pendingBlocks_;
    std::unordered_map<io::UInt256, std::shared_ptr<ledger::Transaction>> pendingTransactions_;
    mutable std::mutex mutex_;

    std::function<void(const std::shared_ptr<ledger::Block>&)> blockReceivedCallback_;
    std::function<void(const std::shared_ptr<ledger::Transaction>&)> transactionReceivedCallback_;
    std::function<void(SynchronizationState)> stateChangedCallback_;

    void RunSync();
    void SyncHeaders();
    void SyncBlocks();
    void ProcessPendingBlocks();
    void ProcessPendingTransactions();
    void RequestHeaders();
    void RequestBlocks();
    void SetState(SynchronizationState state);
};
}  // namespace neo::network::p2p
