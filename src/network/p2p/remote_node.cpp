#include <cstdint>
#include <functional>
#include <memory>
#include <neo/core/logging.h>
#include <neo/io/byte_vector.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/message.h>
#include <neo/network/p2p/local_node.h>
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
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/remote_node.h>
#include <string>
#include <vector>

namespace neo::network::p2p
{
RemoteNode::RemoteNode(LocalNode* localNode, std::shared_ptr<Connection> connection)
    : localNode_(localNode), connection_(connection), handshaked_(false), version_(0), lastBlockIndex_(0)
{
    // Set up callbacks
    connection_->SetMessageReceivedCallback(std::bind(&RemoteNode::OnMessageReceived, this, std::placeholders::_1));
    connection_->SetDisconnectedCallback(std::bind(&RemoteNode::OnDisconnected, this));
}

RemoteNode::~RemoteNode()
{
    Disconnect();
}

std::shared_ptr<Connection> RemoteNode::GetConnection() const
{
    return connection_;
}

IPEndPoint RemoteNode::GetRemoteEndPoint() const
{
    return connection_->GetRemoteEndPoint();
}

IPEndPoint RemoteNode::GetLocalEndPoint() const
{
    return connection_->GetLocalEndPoint();
}

uint32_t RemoteNode::GetVersion() const
{
    return version_;
}

const std::string& RemoteNode::GetUserAgent() const
{
    return userAgent_;
}

const std::vector<NodeCapability>& RemoteNode::GetCapabilities() const
{
    return capabilities_;
}

uint32_t RemoteNode::GetLastBlockIndex() const
{
    return lastBlockIndex_;
}

bool RemoteNode::IsConnected() const
{
    return connection_ != nullptr;
}

bool RemoteNode::IsHandshaked() const
{
    return handshaked_;
}

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
    if (!IsConnected())
        return false;

    return connection_->Send(message, enableCompression);
}

bool RemoteNode::SendVersion()
{
    if (!IsConnected())
        return false;

    // Create a version payload
    auto payload = localNode_->CreateVersionPayload();

    // Send the version message
    return Send(Message::Create(MessageCommand::Version, payload));
}

bool RemoteNode::SendVerack()
{
    if (!IsConnected())
        return false;

    // Send the verack message
    return Send(Message::Create(MessageCommand::Verack));
}

bool RemoteNode::SendPing()
{
    if (!IsConnected())
        return false;

    // Create a ping payload
    auto payload =
        std::make_shared<payloads::PingPayload>(payloads::PingPayload::Create(localNode_->GetLastBlockIndex()));

    // Send the ping message
    return Send(Message::Create(MessageCommand::Ping, payload));
}

bool RemoteNode::SendPong(const payloads::PingPayload& payload)
{
    if (!IsConnected())
        return false;

    // Create a pong payload (same as ping payload)
    auto pongPayload = std::make_shared<payloads::PingPayload>();
    pongPayload->SetLastBlockIndex(localNode_->GetLastBlockIndex());
    pongPayload->SetNonce(payload.GetNonce());
    pongPayload->SetTimestamp(payload.GetTimestamp());

    // Send the pong message
    return Send(Message::Create(MessageCommand::Pong, pongPayload));
}

bool RemoteNode::SendGetAddr()
{
    if (!IsConnected())
        return false;

    // Send the getaddr message
    return Send(Message::Create(MessageCommand::GetAddr));
}

bool RemoteNode::SendAddr(const std::vector<NetworkAddressWithTime>& addresses)
{
    if (!IsConnected())
        return false;

    // Create an addr payload
    auto payload = std::make_shared<payloads::AddrPayload>(addresses);

    // Send the addr message
    return Send(Message::Create(MessageCommand::Addr, payload));
}

bool RemoteNode::SendInv(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    if (!IsConnected())
        return false;

    // Create an inv payload
    auto payload = std::make_shared<payloads::InvPayload>(type, hashes);

    // Send the inv message
    return Send(Message::Create(MessageCommand::Inv, payload));
}

bool RemoteNode::SendGetData(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    if (!IsConnected())
        return false;

    // Create a getdata payload
    auto payload = std::make_shared<payloads::GetDataPayload>(type, hashes);

    // Send the getdata message
    return Send(Message::Create(MessageCommand::GetData, payload));
}

bool RemoteNode::SendGetBlocks(const io::UInt256& hashStart, int16_t count)
{
    if (!IsConnected())
        return false;

    // Create a getblocks payload
    auto payload = std::make_shared<payloads::GetBlocksPayload>();
    payload->SetHashStart(hashStart);
    payload->SetCount(count);

    // Send the getblocks message
    return Send(Message::Create(MessageCommand::GetBlocks, payload));
}

bool RemoteNode::SendGetBlockByIndex(uint32_t indexStart, uint16_t count)
{
    if (!IsConnected())
        return false;

    // Create a getblockbyindex payload
    auto payload = std::make_shared<payloads::GetBlockByIndexPayload>(indexStart, count);

    // Send the getblockbyindex message
    return Send(Message::Create(MessageCommand::GetBlockByIndex, payload));
}

bool RemoteNode::SendGetHeaders(const io::UInt256& hashStart, int16_t count)
{
    if (!IsConnected())
        return false;

    // Create a getheaders payload
    auto payload = std::make_shared<payloads::GetHeadersPayload>();
    payload->SetHashStart(hashStart);
    payload->SetCount(count);

    // Send the getheaders message
    return Send(Message::Create(MessageCommand::GetHeaders, payload));
}

bool RemoteNode::SendMempool()
{
    if (!IsConnected())
        return false;

    // Create a mempool payload
    auto payload = std::make_shared<payloads::MempoolPayload>();

    // Send the mempool message
    return Send(Message::Create(MessageCommand::Mempool, payload));
}

bool RemoteNode::SendHeaders(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers)
{
    if (!IsConnected())
        return false;

    // Create a headers payload
    auto payload = std::make_shared<payloads::HeadersPayload>(headers);

    // Send the headers message
    return Send(Message::Create(MessageCommand::Headers, payload));
}

void RemoteNode::OnMessageReceived(const Message& message)
{
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
            ProcessGetAddrMessage(message);
            break;
        case MessageCommand::Reject:
            ProcessRejectMessage(message);
            break;
        case MessageCommand::NotFound:
            ProcessNotFoundMessage(message);
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
    // Only process version message if we haven't handshaked yet
    if (handshaked_)
        return;

    // Get the version payload
    auto payload = std::dynamic_pointer_cast<payloads::VersionPayload>(message.GetPayload());
    if (!payload)
        return;

    // Store the version information
    version_ = payload->GetVersion();
    userAgent_ = payload->GetUserAgent();
    capabilities_ = payload->GetCapabilities();

    // Send verack message
    SendVerack();

    // If we haven't sent our version yet, send it
    if (!handshaked_)
    {
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
    // Only process verack message if we haven't handshaked yet
    if (handshaked_)
        return;

    // Mark as handshaked
    handshaked_ = true;

    // Notify the local node that we've handshaked
    if (localNode_)
    {
        localNode_->OnRemoteNodeHandshaked(this);
    }
}

void RemoteNode::ProcessPingMessage(const Message& message)
{
    // Only process ping message if we've handshaked
    if (!handshaked_)
        return;

    // Get the ping payload
    auto payload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
    if (!payload)
        return;

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
    if (!handshaked_)
        return;

    // Get the pong payload
    auto payload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
    if (!payload)
        return;

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
    if (!handshaked_)
        return;

    // Get the addr payload
    auto payload = std::dynamic_pointer_cast<payloads::AddrPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received an addr message
    if (localNode_)
    {
        localNode_->OnAddrMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessInvMessage(const Message& message)
{
    // Only process inv message if we've handshaked
    if (!handshaked_)
        return;

    // Get the inv payload
    auto payload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received an inv message
    if (localNode_)
    {
        localNode_->OnInvMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetDataMessage(const Message& message)
{
    // Only process getdata message if we've handshaked
    if (!handshaked_)
        return;

    // Get the getdata payload
    auto payload = std::dynamic_pointer_cast<payloads::GetDataPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a getdata message
    if (localNode_)
    {
        localNode_->OnGetDataMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetBlocksMessage(const Message& message)
{
    // Only process getblocks message if we've handshaked
    if (!handshaked_)
        return;

    // Get the getblocks payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlocksPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a getblocks message
    if (localNode_)
    {
        localNode_->OnGetBlocksMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetBlockByIndexMessage(const Message& message)
{
    // Only process getblockbyindex message if we've handshaked
    if (!handshaked_)
        return;

    // Get the getblockbyindex payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlockByIndexPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a getblockbyindex message
    if (localNode_)
    {
        localNode_->OnGetBlockByIndexMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetHeadersMessage(const Message& message)
{
    // Only process getheaders message if we've handshaked
    if (!handshaked_)
        return;

    // Get the getheaders payload
    auto payload = std::dynamic_pointer_cast<payloads::GetBlocksPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a getheaders message
    if (localNode_)
    {
        localNode_->OnGetHeadersMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessHeadersMessage(const Message& message)
{
    // Only process headers message if we've handshaked
    if (!handshaked_)
        return;

    // Get the headers payload
    auto payload = std::dynamic_pointer_cast<payloads::HeadersPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a headers message
    if (localNode_)
    {
        localNode_->OnHeadersMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessMempoolMessage(const Message& message)
{
    // Only process mempool message if we've handshaked
    if (!handshaked_)
        return;

    // Get the mempool payload
    auto payload = std::dynamic_pointer_cast<payloads::MempoolPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a mempool message
    if (localNode_)
    {
        localNode_->OnMempoolMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterAddMessage(const Message& message)
{
    // Only process filteradd message if we've handshaked
    if (!handshaked_)
        return;

    // Get the filteradd payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterAddPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a filteradd message
    if (localNode_)
    {
        localNode_->OnFilterAddMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterClearMessage(const Message& message)
{
    // Only process filterclear message if we've handshaked
    if (!handshaked_)
        return;

    // Get the filterclear payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterClearPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a filterclear message
    if (localNode_)
    {
        localNode_->OnFilterClearMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessFilterLoadMessage(const Message& message)
{
    // Only process filterload message if we've handshaked
    if (!handshaked_)
        return;

    // Get the filterload payload
    auto payload = std::dynamic_pointer_cast<payloads::FilterLoadPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a filterload message
    if (localNode_)
    {
        localNode_->OnFilterLoadMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessGetAddrMessage(const Message& message)
{
    // Only process getaddr message if we've handshaked
    if (!handshaked_)
        return;

    // GetAddr message has no payload, just respond with addr message
    // containing known peer addresses
    if (localNode_)
    {
        localNode_->OnGetAddrMessageReceived(this);
    }
}

void RemoteNode::ProcessRejectMessage(const Message& message)
{
    // Only process reject message if we've handshaked
    if (!handshaked_)
        return;

    // Get the reject payload
    auto payload = std::dynamic_pointer_cast<payloads::RejectPayload>(message.GetPayload());
    if (!payload)
        return;

    // Log the rejection for debugging
    LOG_WARNING("RemoteNode received reject message: {}", payload->GetReason());

    // Notify the local node that we've received a reject message
    if (localNode_)
    {
        localNode_->OnRejectMessageReceived(this, *payload);
    }
}

void RemoteNode::ProcessNotFoundMessage(const Message& message)
{
    // Only process notfound message if we've handshaked
    if (!handshaked_)
        return;

    // Get the notfound payload (similar to inv payload)
    auto payload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
    if (!payload)
        return;

    // Notify the local node that we've received a notfound message
    if (localNode_)
    {
        localNode_->OnNotFoundMessageReceived(this, *payload);
    }
}

}  // namespace neo::network::p2p
