/**
 * @file local_node.h
 * @brief Local Node
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/peer_list.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/tcp_connection.h>

#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::consensus
{
class ConsensusService;
}

namespace neo::network::p2p
{
/**
 * @brief Represents the local node in the P2P network.
 */
class LocalNode
{
   public:
    /**
     * @brief Gets the singleton instance of the LocalNode.
     * @return The singleton instance.
     */
    static LocalNode& GetInstance();

    /**
     * @brief Destructor.
     */
    virtual ~LocalNode();

    /**
     * @brief Gets the user agent of the local node.
     * @return The user agent.
     */
    const std::string& GetUserAgent() const;

    /**
     * @brief Sets the user agent of the local node.
     * @param userAgent The user agent.
     */
    void SetUserAgent(const std::string& userAgent);

    /**
     * @brief Gets the capabilities of the local node.
     * @return The capabilities.
     */
    const std::vector<NodeCapability>& GetCapabilities() const;

    /**
     * @brief Sets the capabilities of the local node.
     * @param capabilities The capabilities.
     */
    void SetCapabilities(const std::vector<NodeCapability>& capabilities);

    /**
     * @brief Gets the last block index of the local node.
     * @return The last block index.
     */
    uint32_t GetLastBlockIndex() const;

    /**
     * @brief Sets the last block index of the local node.
     * @param lastBlockIndex The last block index.
     */
    void SetLastBlockIndex(uint32_t lastBlockIndex);

    /**
     * @brief Gets the nonce of the local node.
     * @return The nonce.
     */
    uint32_t GetNonce() const;

    /**
     * @brief Gets the listening port for the local node.
     * @return The TCP listening port if available, otherwise 0.
     */
    uint16_t GetPort() const;

    /**
     * @brief Gets the connected remote nodes.
     * @return The connected remote nodes.
     */
    std::vector<RemoteNode*> GetConnectedNodes() const;

    /**
     * @brief Gets the number of connected remote nodes.
     * @return The number of connected remote nodes.
     */
    size_t GetConnectedCount() const;

    /**
     * @brief Gets the number of connected remote nodes (compatibility alias).
     * @return The number of connected remote nodes.
     */
    size_t GetConnectedPeersCount() const { return GetConnectedCount(); }

    /**
     * @brief Gets the connected peers as shared pointers.
     * @return Vector of connected peers.
     */
    std::vector<std::shared_ptr<RemoteNode>> GetConnectedPeers() const;

    /**
     * @brief Creates a version payload for the local node.
     * @return The version payload.
     */
    std::shared_ptr<payloads::VersionPayload> CreateVersionPayload() const;

    /**
     * @brief Starts the local node.
     * @param port The port to listen on.
     * @param maxConnections The maximum number of connections.
     * @return True if the local node was started successfully, false otherwise.
     */
    bool Start(uint16_t port, size_t maxConnections = 10);

    /**
     * @brief Starts the local node.
     * @param config The channels configuration.
     * @return True if the local node was started successfully, false otherwise.
     */
    bool Start(const ChannelsConfig& config);

    /**
     * @brief Stops the local node.
     */
    void Stop();

    /**
     * @brief Checks whether the local node is currently running.
     */
    bool IsRunning() const { return running_.load(); }

    /**
     * @brief Connects to a remote node.
     * @param endpoint The endpoint of the remote node.
     * @return True if the connection was established successfully, false otherwise.
     */
    bool Connect(const IPEndPoint& endpoint);

    /**
     * @brief Broadcasts a message to all connected remote nodes.
     * @param message The message to broadcast.
     * @param enableCompression Whether to enable compression.
     */
    void Broadcast(const Message& message, bool enableCompression = true);

    /**
     * @brief Broadcasts an inv message to all connected remote nodes.
     * @param type The type of inventory.
     * @param hashes The hashes of the inventory items.
     */
    void BroadcastInv(InventoryType type, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Sets the version message received callback.
     * @param callback The callback.
     */
    void SetVersionMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::VersionPayload&)> callback);

    /**
     * @brief Sets the ping message received callback.
     * @param callback The callback.
     */
    void SetPingMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback);

    /**
     * @brief Sets the pong message received callback.
     * @param callback The callback.
     */
    void SetPongMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback);

    /**
     * @brief Sets the addr message received callback.
     * @param callback The callback.
     */
    void SetAddrMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::AddrPayload&)> callback);

    /**
     * @brief Sets the inv message received callback.
     * @param callback The callback.
     */
    void SetInvMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::InvPayload&)> callback);

    /**
     * @brief Sets the getdata message received callback.
     * @param callback The callback.
     */
    void SetGetDataMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetDataPayload&)> callback);

    /**
     * @brief Sets the getblocks message received callback.
     * @param callback The callback.
     */
    void SetGetBlocksMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback);

    /**
     * @brief Sets the getblockbyindex message received callback.
     * @param callback The callback.
     */
    void SetGetBlockByIndexMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::GetBlockByIndexPayload&)> callback);

    /**
     * @brief Sets the getheaders message received callback.
     * @param callback The callback.
     */
    void SetGetHeadersMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback);

    /**
     * @brief Sets the headers message received callback.
     * @param callback The callback.
     */
    void SetHeadersMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::HeadersPayload&)> callback);

    /**
     * @brief Sets the mempool message received callback.
     * @param callback The callback.
     */
    void SetMempoolMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::MempoolPayload&)> callback);

    /**
     * @brief Sets the filteradd message received callback.
     * @param callback The callback.
     */
    void SetFilterAddMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::FilterAddPayload&)> callback);

    /**
     * @brief Sets the filterclear message received callback.
     * @param callback The callback.
     */
    void SetFilterClearMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::FilterClearPayload&)> callback);

    /**
     * @brief Sets the filterload message received callback.
     * @param callback The callback.
     */
    void SetFilterLoadMessageReceivedCallback(
        std::function<void(RemoteNode*, const payloads::FilterLoadPayload&)> callback);

    /**
     * @brief Sets the remote node connected callback.
     * @param callback The callback.
     */
    void SetRemoteNodeConnectedCallback(std::function<void(RemoteNode*)> callback);

    /**
     * @brief Sets the remote node disconnected callback.
     * @param callback The callback.
     */
    void SetRemoteNodeDisconnectedCallback(std::function<void(RemoteNode*)> callback);

    /**
     * @brief Sets the remote node handshaked callback.
     * @param callback The callback.
     */
    void SetRemoteNodeHandshakedCallback(std::function<void(RemoteNode*)> callback);

    /**
     * @brief Sets the block message received callback.
     * @param callback The callback.
     */
    void SetBlockMessageReceivedCallback(std::function<void(RemoteNode*, std::shared_ptr<ledger::Block>)> callback);

    /**
     * @brief Called when a version message is received.
     * @param remoteNode The remote node.
     * @param payload The version payload.
     */
    void OnVersionMessageReceived(RemoteNode* remoteNode, const payloads::VersionPayload& payload);

    /**
     * @brief Called when a ping message is received.
     * @param remoteNode The remote node.
     * @param payload The ping payload.
     */
    void OnPingMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload);

    /**
     * @brief Called when a pong message is received.
     * @param remoteNode The remote node.
     * @param payload The pong payload.
     */
    void OnPongMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload);

    /**
     * @brief Called when an addr message is received.
     * @param remoteNode The remote node.
     * @param payload The addr payload.
     */
    void OnAddrMessageReceived(RemoteNode* remoteNode, const payloads::AddrPayload& payload);

    /**
     * @brief Called when an inv message is received.
     * @param remoteNode The remote node.
     * @param payload The inv payload.
     */
    void OnInvMessageReceived(RemoteNode* remoteNode, const payloads::InvPayload& payload);

    /**
     * @brief Called when a getdata message is received.
     * @param remoteNode The remote node.
     * @param payload The getdata payload.
     */
    void OnGetDataMessageReceived(RemoteNode* remoteNode, const payloads::GetDataPayload& payload);

    /**
     * @brief Called when a getblocks message is received.
     * @param remoteNode The remote node.
     * @param payload The getblocks payload.
     */
    void OnGetBlocksMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload);

    /**
     * @brief Called when a getblockbyindex message is received.
     * @param remoteNode The remote node.
     * @param payload The getblockbyindex payload.
     */
    void OnGetBlockByIndexMessageReceived(RemoteNode* remoteNode, const payloads::GetBlockByIndexPayload& payload);

    /**
     * @brief Called when a getheaders message is received.
     * @param remoteNode The remote node.
     * @param payload The getheaders payload.
     */
    void OnGetHeadersMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload);

    /**
     * @brief Called when a headers message is received.
     * @param remoteNode The remote node.
     * @param payload The headers payload.
     */
    void OnHeadersMessageReceived(RemoteNode* remoteNode, const payloads::HeadersPayload& payload);

    /**
     * @brief Called when a mempool message is received.
     * @param remoteNode The remote node.
     * @param payload The mempool payload.
     */
    void OnMempoolMessageReceived(RemoteNode* remoteNode, const payloads::MempoolPayload& payload);

    /**
     * @brief Called when a filteradd message is received.
     * @param remoteNode The remote node.
     * @param payload The filteradd payload.
     */
    void OnFilterAddMessageReceived(RemoteNode* remoteNode, const payloads::FilterAddPayload& payload);

    /**
     * @brief Called when a filterclear message is received.
     * @param remoteNode The remote node.
     * @param payload The filterclear payload.
     */
    void OnFilterClearMessageReceived(RemoteNode* remoteNode, const payloads::FilterClearPayload& payload);

    /**
     * @brief Called when a filterload message is received.
     * @param remoteNode The remote node.
     * @param payload The filterload payload.
     */
    void OnFilterLoadMessageReceived(RemoteNode* remoteNode, const payloads::FilterLoadPayload& payload);

    /**
     * @brief Called when a block message is received.
     * @param remoteNode The remote node.
     * @param payload The block payload.
     */
    void OnBlockMessageReceived(RemoteNode* remoteNode, const payloads::BlockPayload& payload);

    /**
     * @brief Called when a transaction message is received.
     * @param remoteNode The remote node.
     * @param payload The transaction payload.
     */
    void OnTransactionMessageReceived(RemoteNode* remoteNode, const payloads::TransactionPayload& payload);

    /**
     * @brief Called when an extensible message is received.
     * @param remoteNode The remote node.
     * @param payload The extensible payload.
     */
    void OnExtensibleMessageReceived(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload);

    /**
     * @brief Called when a GetAddr message is received.
     * @param remoteNode The remote node.
     */
    void OnGetAddrMessageReceived(RemoteNode* remoteNode);

    /**
     * @brief Called when a Verack message is received.
     * @param remoteNode The remote node.
     */
    void OnVerackMessageReceived(RemoteNode* remoteNode);

    /**
     * @brief Called when a remote node is connected.
     * @param remoteNode The remote node.
     */
    void OnRemoteNodeConnected(RemoteNode* remoteNode);

    /**
     * @brief Called when a remote node is disconnected.
     * @param remoteNode The remote node.
     */
    void OnRemoteNodeDisconnected(RemoteNode* remoteNode);

    /**
     * @brief Called when a remote node is handshaked.
     * @param remoteNode The remote node.
     */
    void OnRemoteNodeHandshaked(RemoteNode* remoteNode);

    /**
     * @brief Called when a transaction is received from a remote node.
     * @param payload The transaction payload.
     */
    void OnTransactionReceived(std::shared_ptr<IPayload> payload);

    /**
     * @brief Called when a block is received from a remote node.
     * @param remoteNode The remote node that sent the block.
     * @param block The block received.
     */
    void OnBlockReceived(RemoteNode* remoteNode, std::shared_ptr<ledger::Block> block);

    /**
     * @brief Sets the peer list file path.
     * @param path The path to the peer list file.
     */
    void SetPeerListPath(const std::string& path);

    /**
     * @brief Gets the peer list.
     * @return The peer list.
     */
    PeerList& GetPeerList();

    /**
     * @brief Saves the peer list to a file.
     * @return True if the peer list was saved, false otherwise.
     */
    bool SavePeerList();

    /**
     * @brief Loads the peer list from a file.
     * @return True if the peer list was loaded, false otherwise.
     */
    bool LoadPeerList();

    /**
     * @brief Adds a peer to the peer list.
     * @param endpoint The endpoint of the peer.
     * @return True if the peer was added, false otherwise.
     */
    bool AddPeer(const IPEndPoint& endpoint);

    /**
     * @brief Adds a peer to the peer list.
     * @param peer The peer.
     * @return True if the peer was added, false otherwise.
     */
    bool AddPeer(const Peer& peer);

    /**
     * @brief Adds peers to the peer list.
     * @param endpoints The endpoints of the peers.
     */
    void AddPeers(const std::vector<IPEndPoint>& endpoints);

    /**
     * @brief Removes a peer from the peer list.
     * @param endpoint The endpoint of the peer.
     * @return True if the peer was removed, false otherwise.
     */
    bool RemovePeer(const IPEndPoint& endpoint);

    /**
     * @brief Marks a peer as connected.
     * @param endpoint The endpoint of the peer.
     * @return True if the peer was marked as connected, false otherwise.
     */
    bool MarkPeerConnected(const IPEndPoint& endpoint);

    /**
     * @brief Marks a peer as disconnected.
     * @param endpoint The endpoint of the peer.
     * @return True if the peer was marked as disconnected, false otherwise.
     */
    bool MarkPeerDisconnected(const IPEndPoint& endpoint);

    /**
     * @brief Marks a peer as bad.
     * @param endpoint The endpoint of the peer.
     * @return True if the peer was marked as bad, false otherwise.
     */
    bool MarkPeerBad(const IPEndPoint& endpoint);

    /**
     * @brief Starts the connection lifecycle management.
     */
    void StartConnectionLifecycle();

    /**
     * @brief Stops the connection lifecycle management.
     */
    void StopConnectionLifecycle();

    /**
     * @brief Manages the connection lifecycle.
     */
    void ManageConnectionLifecycle();

    /**
     * @brief Set callbacks for block and transaction events
     */
    void SetBlockReceivedCallback(std::function<void(std::shared_ptr<ledger::Block>)> callback);
    void SetTransactionReceivedCallback(std::function<void(std::shared_ptr<ledger::Transaction>)> callback);

    /**
     * @brief Get blockchain and mempool instances
     */
    std::shared_ptr<ledger::Blockchain> GetBlockchain() const;
    std::shared_ptr<ledger::MemoryPool> GetMemoryPool() const;
    void SetBlockchain(std::shared_ptr<ledger::Blockchain> blockchain);
    void SetMemoryPool(std::shared_ptr<ledger::MemoryPool> mempool);

    /**
     * @brief Relay methods for block and transaction propagation
     */
    void RelayBlock(std::shared_ptr<ledger::Block> block);
    void RelayTransaction(std::shared_ptr<ledger::Transaction> transaction);
    void RelayExtensiblePayload(std::shared_ptr<payloads::ExtensiblePayload> payload);

    /**
     * @brief Request blocks from a remote node
     */
    void RequestBlocks(RemoteNode* remoteNode);

    /**
     * @brief Process consensus and state service messages
     */
    void ProcessConsensusMessage(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload);
    void ProcessStateServiceMessage(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload);

    /**
     * @brief Get consensus and state services
     */
    std::shared_ptr<consensus::ConsensusService> GetConsensusService() const { return consensusService_; }
    std::shared_ptr<class StateService> GetStateService() const { return stateService_; }

    void SetConsensusService(std::shared_ptr<consensus::ConsensusService> service) { consensusService_ = service; }

   private:
    LocalNode();

    std::string userAgent_;
    std::vector<NodeCapability> capabilities_;
    uint32_t lastBlockIndex_;
    uint32_t nonce_;

    boost::asio::io_context ioContext_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::thread ioThread_;

    std::unordered_map<std::string, std::unique_ptr<RemoteNode>> connectedNodes_;
    mutable std::mutex connectedNodesMutex_;
    size_t maxConnections_;
    uint16_t listeningPort_{0};

    std::shared_ptr<consensus::ConsensusService> consensusService_;
    std::shared_ptr<class StateService> stateService_;
    std::atomic<bool> running_;

    PeerList peerList_;
    std::string peerListPath_;
    std::thread connectionLifecycleThread_;
    std::atomic<bool> connectionLifecycleRunning_;

    std::function<void(RemoteNode*, const payloads::VersionPayload&)> versionMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::PingPayload&)> pingMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::PingPayload&)> pongMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::AddrPayload&)> addrMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::InvPayload&)> invMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::GetDataPayload&)> getDataMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> getBlocksMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::GetBlockByIndexPayload&)> getBlockByIndexMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> getHeadersMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::HeadersPayload&)> headersMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::MempoolPayload&)> mempoolMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::FilterAddPayload&)> filterAddMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::FilterClearPayload&)> filterClearMessageReceivedCallback_;
    std::function<void(RemoteNode*, const payloads::FilterLoadPayload&)> filterLoadMessageReceivedCallback_;
    std::function<void(RemoteNode*)> remoteNodeConnectedCallback_;
    std::function<void(RemoteNode*)> remoteNodeDisconnectedCallback_;
    std::function<void(RemoteNode*)> remoteNodeHandshakedCallback_;
    std::function<void(RemoteNode*, std::shared_ptr<ledger::Block>)> blockMessageReceivedCallback_;

    void StartAccept();
    void HandleAccept(const std::error_code& error, boost::asio::ip::tcp::socket socket);
    void HandleConnect(const std::error_code& error, boost::asio::ip::tcp::socket socket, const IPEndPoint& endpoint);
    void AddConnectedNode(std::unique_ptr<RemoteNode> remoteNode);
    void RemoveConnectedNode(const std::string& key);

    // Additional member variables for production support
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> mempool_;
    std::function<void(std::shared_ptr<ledger::Block>)> blockReceivedCallback_;
    std::function<void(std::shared_ptr<ledger::Transaction>)> transactionReceivedCallback_;
};
}  // namespace neo::network::p2p
