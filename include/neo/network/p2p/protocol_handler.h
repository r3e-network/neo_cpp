#pragma once

#include <neo/core/logging.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/iinventory.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/persistence/data_cache.h>

#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>

namespace neo::network::p2p
{
/**
 * @brief Protocol handler for Neo P2P messages
 *
 * Implements the Neo network protocol message handling logic
 */
class ProtocolHandler
{
   public:
    using SendMessageCallback = std::function<void(const io::UInt256& peer_id, const Message& message)>;
    using BroadcastCallback = std::function<void(const Message& message, const std::vector<io::UInt256>& exclude)>;
    using DisconnectCallback = std::function<void(const io::UInt256& peer_id, const std::string& reason)>;

    struct Config
    {
        uint32_t protocol_version{0};
        uint32_t network_id{0};
        std::string user_agent{"neo-cpp/1.0"};
        uint32_t max_blocks_per_message{500};
        uint32_t max_headers_per_message{2000};
        uint32_t max_inventory_per_message{500};
        uint32_t max_addr_per_message{200};
        std::chrono::seconds ping_interval{30};
        std::chrono::seconds ping_timeout{60};
    };

   private:
    Config config_;
    std::shared_ptr<core::Logger> logger_;
    std::shared_ptr<persistence::DataCache> blockchain_;
    std::shared_ptr<ledger::MemoryPool> mempool_;

    // Callbacks
    SendMessageCallback send_callback_;
    BroadcastCallback broadcast_callback_;
    DisconnectCallback disconnect_callback_;

    // Peer state tracking
    struct PeerState
    {
        bool version_received{false};
        bool verack_received{false};
        uint32_t start_height{0};
        std::chrono::steady_clock::time_point last_ping;
        std::chrono::steady_clock::time_point last_pong;
        std::unordered_set<io::UInt256> known_hashes;
        std::queue<io::UInt256> requested_blocks;
        std::queue<io::UInt256> requested_transactions;
    };

    std::unordered_map<io::UInt256, PeerState> peer_states_;
    mutable std::mutex mutex_;

   public:
    ProtocolHandler(const Config& config, std::shared_ptr<persistence::DataCache> blockchain,
                    std::shared_ptr<ledger::MemoryPool> mempool);

    /**
     * @brief Set callbacks
     */
    void SetSendCallback(SendMessageCallback callback) { send_callback_ = callback; }
    void SetBroadcastCallback(BroadcastCallback callback) { broadcast_callback_ = callback; }
    void SetDisconnectCallback(DisconnectCallback callback) { disconnect_callback_ = callback; }

    /**
     * @brief Handle new peer connection
     */
    void OnPeerConnected(const io::UInt256& peer_id, bool is_outbound);

    /**
     * @brief Handle peer disconnection
     */
    void OnPeerDisconnected(const io::UInt256& peer_id);

    /**
     * @brief Handle received message
     */
    void HandleMessage(const io::UInt256& peer_id, const Message& message);

    /**
     * @brief Send initial handshake
     */
    void SendHandshake(const io::UInt256& peer_id);

    /**
     * @brief Request blocks from peer
     */
    void RequestBlocks(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Request transactions from peer
     */
    void RequestTransactions(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Broadcast block to network
     */
    void BroadcastBlock(const ledger::Block& block);

    /**
     * @brief Broadcast transaction to network
     */
    void BroadcastTransaction(const ledger::Transaction& transaction);

    /**
     * @brief Get synchronization status
     */
    bool IsSynchronized() const;

    /**
     * @brief Get peer count by state
     */
    size_t GetHandshakedPeerCount() const;

   private:
    // Message handlers
    void HandleVersion(const io::UInt256& peer_id, const payloads::VersionPayload& payload);
    void HandleVerack(const io::UInt256& peer_id);
    void HandleGetAddr(const io::UInt256& peer_id);
    void HandleAddr(const io::UInt256& peer_id, const payloads::AddrPayload& payload);
    void HandlePing(const io::UInt256& peer_id, const payloads::PingPayload& payload);
    void HandlePong(const io::UInt256& peer_id, const payloads::PingPayload& payload);  // Note: Pong uses PingPayload
    void HandleGetHeaders(const io::UInt256& peer_id, const payloads::GetHeadersPayload& payload);
    void HandleHeaders(const io::UInt256& peer_id, const payloads::HeadersPayload& payload);
    void HandleGetBlocks(const io::UInt256& peer_id, const payloads::GetBlocksPayload& payload);
    void HandleGetData(const io::UInt256& peer_id,
                       const payloads::InvPayload& payload);  // GetData uses InvPayload format
    void HandleGetBlockByIndex(const io::UInt256& peer_id, const payloads::GetBlockByIndexPayload& payload);
    void HandleInv(const io::UInt256& peer_id, const payloads::InvPayload& payload);
    void HandleBlock(const io::UInt256& peer_id, const payloads::BlockPayload& payload);
    void HandleTransaction(const io::UInt256& peer_id, const payloads::TransactionPayload& payload);
    void HandleMempool(const io::UInt256& peer_id);
    void HandleNotFound(const io::UInt256& peer_id,
                        const payloads::InvPayload& payload);  // NotFound uses InvPayload format

    /**
     * @brief Check if peer is handshaked
     */
    bool IsPeerHandshaked(const io::UInt256& peer_id) const;

    /**
     * @brief Update peer known hashes
     */
    void UpdateKnownHashes(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Check if peer knows hash
     */
    bool PeerKnowsHash(const io::UInt256& peer_id, const io::UInt256& hash) const;

    /**
     * @brief Get peers for relay
     */
    std::vector<io::UInt256> GetRelayPeers(const io::UInt256& exclude_peer) const;

    /**
     * @brief Relay inventory
     */
    void RelayInventory(payloads::InventoryType type, const io::UInt256& hash, const io::UInt256& source_peer);

    /**
     * @brief Send reject message
     */
    void SendReject(const io::UInt256& peer_id, const std::string& message, const std::string& reason);

    /**
     * @brief Validate block
     */
    bool ValidateBlock(const ledger::Block& block) const;

    /**
     * @brief Validate transaction
     */
    bool ValidateTransaction(const ledger::Transaction& transaction) const;
};
}  // namespace neo::network::p2p