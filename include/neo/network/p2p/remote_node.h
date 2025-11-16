/**
 * @file remote_node.h
 * @brief Remote Node
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/connection.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace neo::network::p2p
{
class LocalNode;

/**
 * @brief Represents a remote node in the P2P network.
 */
class RemoteNode
{
   public:
    /**
     * @brief Constructs a RemoteNode.
     * @param localNode The local node.
     * @param connection The connection to the remote node.
     */
    RemoteNode(LocalNode* localNode, std::shared_ptr<Connection> connection);

    /**
     * @brief Destructor.
     */
    virtual ~RemoteNode();

    /**
     * @brief Gets the connection to the remote node.
     * @return The connection.
     */
    std::shared_ptr<Connection> GetConnection() const;

    /**
     * @brief Gets the remote endpoint.
     * @return The remote endpoint.
     */
    IPEndPoint GetRemoteEndPoint() const;

    /**
     * @brief Gets the local endpoint.
     * @return The local endpoint.
     */
    IPEndPoint GetLocalEndPoint() const;

    /**
     * @brief Gets the version of the remote node.
     * @return The version.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Gets the user agent of the remote node.
     * @return The user agent.
     */
    const std::string& GetUserAgent() const;

    /**
     * @brief Gets the capabilities of the remote node.
     * @return The capabilities.
     */
    const std::vector<NodeCapability>& GetCapabilities() const;

    /**
     * @brief Gets the last block index of the remote node.
     * @return The last block index.
     */
    virtual uint32_t GetLastBlockIndex() const;

    /**
     * @brief Gets whether the remote node is connected.
     * @return True if the remote node is connected, false otherwise.
     */
    virtual bool IsConnected() const;

    /**
     * @brief Gets whether the remote node is handshaked.
     * @return True if the remote node is handshaked, false otherwise.
     */
    bool IsHandshaked() const;

    /**
     * @brief Disconnects from the remote node.
     */
    void Disconnect();

    /**
     * @brief Sends a message to the remote node.
     * @param message The message to send.
     * @param enableCompression Whether to enable compression.
     * @return True if the message was sent successfully, false otherwise.
     */
    virtual bool Send(const Message& message, bool enableCompression = true);

    /**
     * @brief Sends a version message to the remote node.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendVersion();

    /**
     * @brief Sends a verack message to the remote node.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendVerack();

    /**
     * @brief Sends a ping message to the remote node.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendPing();

    /**
     * @brief Sends a pong message to the remote node.
     * @param payload The ping payload to respond to.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendPong(const payloads::PingPayload& payload);

    /**
     * @brief Sends a getaddr message to the remote node.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendGetAddr();

    /**
     * @brief Sends an addr message to the remote node.
     * @param addresses The addresses to send.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendAddr(const std::vector<payloads::NetworkAddressWithTime>& addresses);

    /**
     * @brief Sends an inv message to the remote node.
     * @param type The type of inventory.
     * @param hashes The hashes of the inventory items.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendInv(InventoryType type, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Sends a getdata message to the remote node.
     * @param type The type of inventory.
     * @param hashes The hashes of the inventory items.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendGetData(InventoryType type, const std::vector<io::UInt256>& hashes);

    /**
     * @brief Sends a getblocks message to the remote node.
     * @param hashStart The hash to start from.
     * @param count The number of blocks to request.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendGetBlocks(const io::UInt256& hashStart, int16_t count = -1);

    /**
     * @brief Sends a getblockbyindex message to the remote node.
     * @param indexStart The index to start from.
     * @param count The number of blocks to request.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendGetBlockByIndex(uint32_t indexStart, uint16_t count = 500);

    /**
     * @brief Sends a getheaders message to the remote node.
     * @param hashStart The hash to start from.
     * @param count The number of headers to request.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendGetHeaders(const io::UInt256& hashStart, int16_t count = -1);

    /**
     * @brief Sends a headers message to the remote node.
     * @param headers The headers to send.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendHeaders(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers);

    /**
     * @brief Sends a mempool message to the remote node.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool SendMempool();

   private:
    LocalNode* localNode_;
    std::shared_ptr<Connection> connection_;
    bool handshaked_;
    uint32_t version_;
    std::string userAgent_;
    std::vector<NodeCapability> capabilities_;
    uint32_t lastBlockIndex_;
    bool remoteAllowsCompression_;

    void OnMessageReceived(const Message& message);
    void OnDisconnected();

    void ProcessVersionMessage(const Message& message);
    void ProcessVerackMessage(const Message& message);
    void ProcessPingMessage(const Message& message);
    void ProcessPongMessage(const Message& message);
    void ProcessAddrMessage(const Message& message);
    void ProcessInvMessage(const Message& message);
    void ProcessGetDataMessage(const Message& message);
    void ProcessGetBlocksMessage(const Message& message);
    void ProcessGetBlockByIndexMessage(const Message& message);
    void ProcessGetHeadersMessage(const Message& message);
    void ProcessHeadersMessage(const Message& message);
    void ProcessMempoolMessage(const Message& message);
    void ProcessFilterAddMessage(const Message& message);
    void ProcessFilterClearMessage(const Message& message);
    void ProcessFilterLoadMessage(const Message& message);
    void ProcessGetAddrMessage(const Message& message);
    void ProcessRejectMessage(const Message& message);
    void ProcessNotFoundMessage(const Message& message);
    void ProcessTransactionMessage(const Message& message);
    void ProcessBlockMessage(const Message& message);
};
}  // namespace neo::network::p2p
