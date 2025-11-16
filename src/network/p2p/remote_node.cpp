/**
 * @file remote_node.cpp
 * @brief Remote Node
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/io/byte_vector.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/network_address.h>
#include <neo/network/p2p/payloads/addr_payload.h>
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
#include <neo/network/p2p/payloads/reject_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/remote_node.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace neo::network::p2p
{
RemoteNode::RemoteNode(LocalNode* localNode, std::shared_ptr<Connection> connection)
    : localNode_(localNode),
      connection_(connection),
      handshaked_(false),
      version_(0),
      lastBlockIndex_(0),
      remoteAllowsCompression_(false)
{
    // Set up callbacks
    if (connection_)
    {
        connection_->SetMessageReceivedCallback(std::bind(&RemoteNode::OnMessageReceived, this, std::placeholders::_1));
        connection_->SetDisconnectedCallback(std::bind(&RemoteNode::OnDisconnected, this));
    }
}

RemoteNode::~RemoteNode() { Disconnect(); }

std::shared_ptr<Connection> RemoteNode::GetConnection() const { return connection_; }

IPEndPoint RemoteNode::GetRemoteEndPoint() const { return connection_->GetRemoteEndPoint(); }

IPEndPoint RemoteNode::GetLocalEndPoint() const { return connection_->GetLocalEndPoint(); }

uint32_t RemoteNode::GetVersion() const { return version_; }

const std::string& RemoteNode::GetUserAgent() const { return userAgent_; }

const std::vector<NodeCapability>& RemoteNode::GetCapabilities() const { return capabilities_; }

uint32_t RemoteNode::GetLastBlockIndex() const { return lastBlockIndex_; }

bool RemoteNode::IsConnected() const { return connection_ != nullptr; }

bool RemoteNode::IsHandshaked() const { return handshaked_; }

void RemoteNode::Disconnect()
{
    if (connection_)
    {
        connection_->Disconnect();
        connection_ = nullptr;
    }
}

bool RemoteNode::Send(const Message& message, bool enableCompression)
{
    if (!IsConnected()) return false;

    bool allowCompression = enableCompression && remoteAllowsCompression_;
    if (localNode_ && !localNode_->IsCompressionEnabled())
    {
        allowCompression = false;
    }

    return connection_->Send(message, allowCompression);
}

bool RemoteNode::SendVersion()
{
    if (!IsConnected()) return false;

    // Create a version payload
    auto payload = localNode_->CreateVersionPayload();

    // Send the version message - use explicit p2p namespace
    LOG_INFO("About to call neo::network::p2p::Message::Create for Version command");
    auto message =
        neo::network::p2p::Message::Create(MessageCommand::Version, std::static_pointer_cast<IPayload>(payload));
    LOG_INFO("neo::network::p2p::Message::Create returned message with command: " +
             std::to_string(static_cast<int>(message.GetCommand())));
    return Send(message);
}

bool RemoteNode::SendVerack()
{
    if (!IsConnected()) return false;

    // Send the verack message
    return Send(Message::Create(MessageCommand::Verack));
}

bool RemoteNode::SendPing()
{
    if (!IsConnected()) return false;

    // Create a ping payload
    auto payload =
        std::make_shared<payloads::PingPayload>(payloads::PingPayload::Create(localNode_->GetLastBlockIndex()));

    // Send the ping message
    return Send(Message::Create(MessageCommand::Ping, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendPong(const payloads::PingPayload& payload)
{
    if (!IsConnected()) return false;

    // Create a pong payload (same as ping payload)
    auto pongPayload = std::make_shared<payloads::PingPayload>();
    pongPayload->SetLastBlockIndex(localNode_->GetLastBlockIndex());
    pongPayload->SetNonce(payload.GetNonce());
    pongPayload->SetTimestamp(payload.GetTimestamp());

    // Send the pong message
    return Send(Message::Create(MessageCommand::Pong, std::static_pointer_cast<IPayload>(pongPayload)));
}

bool RemoteNode::SendGetAddr()
{
    if (!IsConnected()) return false;

    // Send the getaddr message
    return Send(Message::Create(MessageCommand::GetAddr));
}

bool RemoteNode::SendAddr(const std::vector<payloads::NetworkAddressWithTime>& addresses)
{
    if (!IsConnected()) return false;

    // Create an addr payload
    auto payload = std::make_shared<payloads::AddrPayload>(addresses);

    // Send the addr message
    return Send(Message::Create(MessageCommand::Addr, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendInv(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    if (!IsConnected()) return false;

    // Create an inv payload
    auto payload = std::make_shared<payloads::InvPayload>();
    payload->SetType(static_cast<payloads::InventoryType>(type));
    payload->SetHashes(hashes);

    // Send the inv message
    return Send(Message::Create(MessageCommand::Inv, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendGetData(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    if (!IsConnected()) return false;

    // Create a getdata payload
    auto payload = std::make_shared<payloads::GetDataPayload>();
    // Note: GetDataPayload uses inventory vectors, not simple type/hashes
    std::vector<InventoryVector> inventories;
    for (const auto& hash : hashes)
    {
        inventories.emplace_back(static_cast<InventoryType>(type), hash);
    }
    payload->SetInventories(inventories);

    // Send the getdata message
    return Send(Message::Create(MessageCommand::GetData, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendGetBlocks(const io::UInt256& hashStart, int16_t count)
{
    if (!IsConnected()) return false;

    // Create a getblocks payload
    auto payload = std::make_shared<payloads::GetBlocksPayload>();
    payload->SetHashStart(hashStart);
    payload->SetCount(count);

    // Send the getblocks message
    return Send(Message::Create(MessageCommand::GetBlocks, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendGetBlockByIndex(uint32_t indexStart, uint16_t count)
{
    if (!IsConnected()) return false;

    // Create a getblockbyindex payload
    auto payload = std::make_shared<payloads::GetBlockByIndexPayload>(indexStart, count);

    // Send the getblockbyindex message
    return Send(Message::Create(MessageCommand::GetBlockByIndex, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendGetHeaders(const io::UInt256& hashStart, int16_t count)
{
    if (!IsConnected()) return false;

    // Create a getheaders payload
    auto payload = std::make_shared<payloads::GetHeadersPayload>();
    payload->SetHashStart(hashStart);
    payload->SetCount(count);

    // Send the getheaders message
    return Send(Message::Create(MessageCommand::GetHeaders, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendMempool()
{
    if (!IsConnected()) return false;

    // Create a mempool payload
    auto payload = std::make_shared<payloads::MempoolPayload>();

    // Send the mempool message
    return Send(Message::Create(MessageCommand::Mempool, std::static_pointer_cast<IPayload>(payload)));
}

bool RemoteNode::SendHeaders(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers)
{
    if (!IsConnected()) return false;

    // Create a headers payload
    auto payload = std::make_shared<payloads::HeadersPayload>(headers);

    // Send the headers message
    return Send(Message::Create(MessageCommand::Headers, std::static_pointer_cast<IPayload>(payload)));
}

void RemoteNode::OnMessageReceived(const Message& message)
{
    LOG_DEBUG("RemoteNode::OnMessageReceived - Command: " + std::to_string(static_cast<int>(message.GetCommand())));

    // Process the message based on its command
    switch (message.GetCommand())
    {
        case MessageCommand::Version:
            ProcessVersionMessage(message);
            break;
        case MessageCommand::Verack:
            ProcessVerackMessage(message);
            break;
        case MessageCommand::Ping:
            ProcessPingMessage(message);
            break;
        case MessageCommand::Pong:
            ProcessPongMessage(message);
            break;
        case MessageCommand::Addr:
            ProcessAddrMessage(message);
            break;
        case MessageCommand::Inv:
            ProcessInvMessage(message);
            break;
        case MessageCommand::GetData:
            ProcessGetDataMessage(message);
            break;
        case MessageCommand::GetBlocks:
            ProcessGetBlocksMessage(message);
            break;
        case MessageCommand::GetBlockByIndex:
            ProcessGetBlockByIndexMessage(message);
            break;
        case MessageCommand::GetHeaders:
            ProcessGetHeadersMessage(message);
            break;
        case MessageCommand::Headers:
            ProcessHeadersMessage(message);
            break;
        case MessageCommand::Mempool:
            ProcessMempoolMessage(message);
            break;
        case MessageCommand::FilterAdd:
            ProcessFilterAddMessage(message);
            break;
        case MessageCommand::FilterClear:
            ProcessFilterClearMessage(message);
            break;
        case MessageCommand::FilterLoad:
            ProcessFilterLoadMessage(message);
            break;
        // Handle additional P2P message types for complete protocol support
        case MessageCommand::GetAddr:
            // GetAddr message requests known peer addresses
            ProcessGetAddrMessage(message);
            break;
        case MessageCommand::Reject:
            // Reject message indicates why a message was rejected
            ProcessRejectMessage(message);
            break;
        case MessageCommand::NotFound:
            // NotFound message indicates requested objects were not found
            ProcessNotFoundMessage(message);
            break;
        case MessageCommand::Transaction:
            // Transaction message contains transaction data
            ProcessTransactionMessage(message);
            break;
        case MessageCommand::Block:
            // Block message contains block data
            ProcessBlockMessage(message);
            break;
        default:
            // Log unhandled message types for debugging
            LOG_DEBUG("RemoteNode received unhandled message type: {}", static_cast<int>(message.GetCommand()));
            break;
    }
}

void RemoteNode::OnDisconnected()
{
    // Notify the local node that this remote node has disconnected
    if (localNode_)
    {
        localNode_->OnRemoteNodeDisconnected(this);
    }
}

void RemoteNode::ProcessVersionMessage(const Message& message)
{
    LOG_DEBUG("ProcessVersionMessage called");

    // Only process version message if we haven't handshaked yet
    if (handshaked_)
    {
        LOG_DEBUG("Already handshaked, ignoring version message");
        return;
    }

    // Get the version payload
    auto payload = std::dynamic_pointer_cast<payloads::VersionPayload>(message.GetPayload());
    if (!payload)
    {
        LOG_WARNING("Failed to get VersionPayload from message");
        return;
    }

    LOG_INFO("Received Version message from peer");

    // Validate network magic to ensure we're on the same network
    if (localNode_ && payload->GetNetwork() != localNode_->GetNetworkMagic())
    {
        LOG_WARNING("Rejecting peer due to network magic mismatch (remote=" +
                    std::to_string(payload->GetNetwork()) + ", local=" +
                    std::to_string(localNode_->GetNetworkMagic()) + ")");
        Disconnect();
        return;
    }

    // Prevent self-connection by comparing nonces
    if (localNode_ && payload->GetNonce() == localNode_->GetNonce())
    {
        LOG_WARNING("Rejecting peer " + GetRemoteEndPoint().ToString() +
                    " due to identical nonce (self-connection detected)");
        Disconnect();
        return;
    }

    // Store the version information
    version_ = payload->GetVersion();
    userAgent_ = payload->GetUserAgent();
    capabilities_ = payload->GetCapabilities();
    lastBlockIndex_ = payload->GetStartHeight();
    remoteAllowsCompression_ = payload->GetAllowCompression();
    if (!remoteAllowsCompression_)
    {
        LOG_INFO("Remote peer {} does not support compression", GetRemoteEndPoint().ToString());
    }

    // Send verack message
    LOG_DEBUG("Sending VerAck message");
    SendVerack();

    // If we haven't sent our version yet, send it
    if (!handshaked_)
    {
        LOG_DEBUG("Sending Version message in response");
        SendVersion();
    }

    // Notify the local node that we've received a version message
    if (localNode_)
    {
        localNode_->OnVersionMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessVerackMessage(const Message& /* message */)
{
    LOG_DEBUG("ProcessVerackMessage called");

    // Only process verack message if we haven't handshaked yet
    if (handshaked_)
    {
        LOG_DEBUG("Already handshaked, ignoring verack message");
        return;
    }

    LOG_INFO("Received VerAck message from peer");

    // Mark as handshaked
    handshaked_ = true;

    LOG_INFO("P2P handshake completed successfully");

    // Notify the local node that we've handshaked
    if (localNode_)
    {
        localNode_->OnRemoteNodeHandshaked(this);
    }
}

void RemoteNode::ProcessPingMessage(const Message& message)
{
    // Only process ping message if we've handshaked
    if (!handshaked_) return;

    // Get the ping payload
    auto payload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
    if (!payload) return;

    // Update the last block index
    lastBlockIndex_ = payload->GetLastBlockIndex();

    // Send pong message
    SendPong(*payload);

    // Notify the local node that we've received a ping message
    if (localNode_)
    {
        localNode_->OnPingMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessPongMessage(const Message& message)
{
    // Only process pong message if we've handshaked
    if (!handshaked_) return;

    // Get the pong payload
    auto payload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
    if (!payload) return;

    // Update the last block index
    lastBlockIndex_ = payload->GetLastBlockIndex();

    // Calculate the ping time
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    uint64_t pingTime = now - connection_->GetLastPingSent();

    // Update the ping time
    connection_->UpdatePingTime(static_cast<uint32_t>(pingTime));

    // Notify the local node that we've received a pong message
    if (localNode_)
    {
        localNode_->OnPongMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessAddrMessage(const Message& message)
{
    // Only process addr message if we've handshaked
    if (!handshaked_) return;

    // Get the addr payload
    auto payload = std::dynamic_pointer_cast<payloads::AddrPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received an addr message
    if (localNode_)
    {
        localNode_->OnAddrMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessInvMessage(const Message& message)
{
    // Only process inv message if we've handshaked
    if (!handshaked_) return;

    // Get the inv payload
    auto payload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received an inv message
    if (localNode_)
    {
        localNode_->OnInvMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetDataMessage(const Message& message)
{
    // Only process getdata message if we've handshaked
    if (!handshaked_) return;

    // Get the getdata payload
    auto payload = std::dynamic_pointer_cast<payloads::GetDataPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a getdata message
    if (localNode_)
    {
        localNode_->OnGetDataMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetBlocksMessage(const Message& message)
{
    // Only process getblocks message if we've handshaked
    if (!handshaked_) return;

    // Get the getblocks payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlocksPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a getblocks message
    if (localNode_)
    {
        localNode_->OnGetBlocksMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetBlockByIndexMessage(const Message& message)
{
    // Only process getblockbyindex message if we've handshaked
    if (!handshaked_) return;

    // Get the getblockbyindex payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlockByIndexPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a getblockbyindex message
    if (localNode_)
    {
        localNode_->OnGetBlockByIndexMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetHeadersMessage(const Message& message)
{
    // Only process getheaders message if we've handshaked
    if (!handshaked_) return;

    // Get the getheaders payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlocksPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a getheaders message
    if (localNode_)
    {
        localNode_->OnGetHeadersMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessHeadersMessage(const Message& message)
{
    // Only process headers message if we've handshaked
    if (!handshaked_) return;

    // Get the headers payload
    auto payload = std::dynamic_pointer_cast<payloads::HeadersPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a headers message
    if (localNode_)
    {
        localNode_->OnHeadersMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessMempoolMessage(const Message& message)
{
    // Only process mempool message if we've handshaked
    if (!handshaked_) return;

    // Get the mempool payload
    auto payload = std::dynamic_pointer_cast<payloads::MempoolPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a mempool message
    if (localNode_)
    {
        localNode_->OnMempoolMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterAddMessage(const Message& message)
{
    // Only process filteradd message if we've handshaked
    if (!handshaked_) return;

    // Get the filteradd payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterAddPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a filteradd message
    if (localNode_)
    {
        localNode_->OnFilterAddMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterClearMessage(const Message& message)
{
    // Only process filterclear message if we've handshaked
    if (!handshaked_) return;

    // Get the filterclear payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterClearPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a filterclear message
    if (localNode_)
    {
        localNode_->OnFilterClearMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterLoadMessage(const Message& message)
{
    // Only process filterload message if we've handshaked
    if (!handshaked_) return;

    // Get the filterload payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterLoadPayload>(message.GetPayload());
    if (!payload) return;

    // Notify the local node that we've received a filterload message
    if (localNode_)
    {
        localNode_->OnFilterLoadMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetAddrMessage(const Message& message)
{
    // Only process getaddr message if we've handshaked
    if (!handshaked_) return;

    // GetAddr message has no payload, just respond with addr message
    // containing known peer addresses
    if (!localNode_) return;

    std::vector<payloads::NetworkAddressWithTime> addresses;
    constexpr size_t maxAddressCount = payloads::AddrPayload::MaxCountToSend;

    auto peerAddresses = localNode_->GetPeerList().GetPeers();
    std::shuffle(peerAddresses.begin(), peerAddresses.end(), std::mt19937{std::random_device{}()});
    auto selfEndpoint = GetRemoteEndPoint();

    const auto now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    for (const auto& peer : peerAddresses)
    {
        if (addresses.size() >= maxAddressCount) break;

        const auto& endpoint = peer.GetEndPoint();
        if (endpoint == selfEndpoint) continue;

        auto caps = peer.GetCapabilities();
        if (caps.empty())
        {
            NodeCapability tcpCapability(NodeCapabilityType::TcpServer);
            tcpCapability.SetPort(endpoint.GetPort());
            caps.push_back(tcpCapability);
        }

        payloads::NetworkAddressWithTime addr(now, endpoint.GetAddress(), caps);
        if (addr.GetPort() == 0 && endpoint.GetPort() != 0)
        {
            addr.SetPort(endpoint.GetPort());
        }
        addresses.push_back(addr);
    }

    if (!addresses.empty())
    {
        Send(Message(MessageCommand::Addr, std::make_shared<payloads::AddrPayload>(addresses)));
    }
}

void RemoteNode::ProcessRejectMessage(const Message& message)
{
    // Only process reject message if we've handshaked
    if (!handshaked_) return;

    // Get the reject payload
    auto payload = std::dynamic_pointer_cast<payloads::RejectPayload>(message.GetPayload());
    if (!payload) return;

    // Log the rejection for debugging
    LOG_WARNING("RemoteNode received reject message from peer");

    // Notify the local node if needed
    if (localNode_)
    {
        // Handle specific rejection types if needed
        // For example, if a transaction was rejected, remove it from relay queue
    }
}

void RemoteNode::ProcessNotFoundMessage(const Message& message)
{
    // Only process notfound message if we've handshaked
    if (!handshaked_) return;

    // Get the notfound payload (similar to inv payload)
    auto payload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
    if (!payload) return;

    // Log which items were not found
    LOG_DEBUG("RemoteNode received notfound message with {} items", payload->GetHashes().size());

    // Notify the local node if needed
    if (localNode_)
    {
        // Handle missing items - for example, request from another peer
        // or mark these items as unavailable from this peer
    }
}

void RemoteNode::ProcessTransactionMessage(const Message& message)
{
    // Only process transaction message if we've handshaked
    if (!handshaked_) return;

    // Get the transaction payload
    auto payload = message.GetPayload();
    if (!payload) return;

    // Cast to transaction - in full implementation, this would be the actual transaction object
    // Handle this as a generic payload and forward to the local node
    LOG_DEBUG("RemoteNode received transaction message");

    // Notify the local node about the new transaction
    if (localNode_)
    {
        // Forward transaction to local node for processing
        // In full implementation, this would involve:
        // 1. Deserializing the transaction
        // 2. Validating the transaction
        // 3. Adding to memory pool if valid
        // 4. Relaying to other peers if needed
        localNode_->OnTransactionReceived(payload);
    }
}

void RemoteNode::ProcessBlockMessage(const Message& message)
{
    // Only process block message if we've handshaked
    if (!handshaked_) return;

    // Get the block payload
    auto payload = message.GetPayload();
    if (!payload) return;

    // Deserialize the block from the payload
    try
    {
        auto blockPayload = std::dynamic_pointer_cast<ledger::Block>(payload);
        if (!blockPayload)
        {
            LOG_ERROR("Failed to cast payload to Block");
            return;
        }

        LOG_DEBUG("RemoteNode received block " + std::to_string(blockPayload->GetIndex()) + " with hash " +
                  blockPayload->GetHash().ToString());

        // Notify the local node about the new block
        if (localNode_)
        {
            localNode_->OnBlockReceived(this, blockPayload);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to process block message: " + std::string(e.what()));
    }
}

}  // namespace neo::network::p2p
