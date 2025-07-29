#include <neo/core/logging.h>
#include <neo/io/binary_reader.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/protocol_handler.h>

namespace neo::network::p2p
{
ProtocolHandler::ProtocolHandler(const Config& config, std::shared_ptr<persistence::DataCache> blockchain,
                                 std::shared_ptr<ledger::MemoryPool> mempool)
    : config_(config), blockchain_(blockchain), mempool_(mempool)
{
}

// Destructor is implicitly defined

void ProtocolHandler::OnPeerConnected(const io::UInt256& peer_id, bool is_outbound)
{
    LOG_DEBUG("OnPeerConnected stub: peer={} outbound={}", peer_id.ToString(), is_outbound);
}

void ProtocolHandler::OnPeerDisconnected(const io::UInt256& peer_id)
{
    LOG_DEBUG("OnPeerDisconnected stub: peer={}", peer_id.ToString());
}

void ProtocolHandler::HandleMessage(const io::UInt256& peer_id, const Message& message)
{
    const std::string command = GetCommandName(message.GetCommand());
    LOG_DEBUG("HandleMessage: peer={} command={}", peer_id.ToString(), command);

    try
    {
        if (command == "version")
        {
            auto versionPayload = std::dynamic_pointer_cast<payloads::VersionPayload>(message.GetPayload());
            if (versionPayload)
            {
                HandleVersion(peer_id, *versionPayload);
            }
        }
        else if (command == "verack")
        {
            HandleVerack(peer_id);
        }
        else if (command == "getaddr")
        {
            HandleGetAddr(peer_id);
        }
        else if (command == "addr")
        {
            auto addrPayload = std::dynamic_pointer_cast<payloads::AddrPayload>(message.GetPayload());
            if (addrPayload)
            {
                HandleAddr(peer_id, *addrPayload);
            }
        }
        else if (command == "ping")
        {
            auto pingPayload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
            if (pingPayload)
            {
                HandlePing(peer_id, *pingPayload);
            }
        }
        else if (command == "pong")
        {
            auto pongPayload = std::dynamic_pointer_cast<payloads::PingPayload>(message.GetPayload());
            if (pongPayload)
            {
                HandlePong(peer_id, *pongPayload);
            }
        }
        else if (command == "getheaders")
        {
            auto getHeadersPayload = std::dynamic_pointer_cast<payloads::GetHeadersPayload>(message.GetPayload());
            if (getHeadersPayload)
            {
                HandleGetHeaders(peer_id, *getHeadersPayload);
            }
        }
        else if (command == "headers")
        {
            auto headersPayload = std::dynamic_pointer_cast<payloads::HeadersPayload>(message.GetPayload());
            if (headersPayload)
            {
                HandleHeaders(peer_id, *headersPayload);
            }
        }
        else if (command == "getblocks")
        {
            auto getBlocksPayload = std::dynamic_pointer_cast<payloads::GetBlocksPayload>(message.GetPayload());
            if (getBlocksPayload)
            {
                HandleGetBlocks(peer_id, *getBlocksPayload);
            }
        }
        else if (command == "getdata")
        {
            auto invPayload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
            if (invPayload)
            {
                HandleGetData(peer_id, *invPayload);
            }
        }
        else if (command == "getblockbyindex")
        {
            auto getBlockByIndexPayload =
                std::dynamic_pointer_cast<payloads::GetBlockByIndexPayload>(message.GetPayload());
            if (getBlockByIndexPayload)
            {
                HandleGetBlockByIndex(peer_id, *getBlockByIndexPayload);
            }
        }
        else if (command == "inv")
        {
            auto invPayload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
            if (invPayload)
            {
                HandleInv(peer_id, *invPayload);
            }
        }
        else if (command == "block")
        {
            auto blockPayload = std::dynamic_pointer_cast<payloads::BlockPayload>(message.GetPayload());
            if (blockPayload)
            {
                HandleBlock(peer_id, *blockPayload);
            }
        }
        else if (command == "tx")
        {
            auto txPayload = std::dynamic_pointer_cast<payloads::TransactionPayload>(message.GetPayload());
            if (txPayload)
            {
                HandleTransaction(peer_id, *txPayload);
            }
        }
        else if (command == "mempool")
        {
            HandleMempool(peer_id);
        }
        else if (command == "notfound")
        {
            auto invPayload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
            if (invPayload)
            {
                HandleNotFound(peer_id, *invPayload);
            }
        }
        else
        {
            LOG_WARNING("Unknown message command: {}", command);
            SendReject(peer_id, command, "Unknown command");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error handling message from peer {}: {}", peer_id.ToString(), e.what());
        SendReject(peer_id, GetCommandName(message.GetCommand()), "Message processing error");
    }
}

void ProtocolHandler::SendHandshake(const io::UInt256& peer_id)
{
    LOG_DEBUG("SendHandshake stub: peer={}", peer_id.ToString());
}

void ProtocolHandler::RequestBlocks(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("RequestBlocks stub: peer={} count={}", peer_id.ToString(), hashes.size());
}

void ProtocolHandler::RequestTransactions(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("RequestTransactions stub: peer={} count={}", peer_id.ToString(), hashes.size());
}

void ProtocolHandler::BroadcastBlock(const ledger::Block& block)
{
    LOG_DEBUG("BroadcastBlock stub");
}

void ProtocolHandler::BroadcastTransaction(const ledger::Transaction& transaction)
{
    LOG_DEBUG("BroadcastTransaction stub");
}

bool ProtocolHandler::IsSynchronized() const
{
    return true;  // Stub - always synchronized
}

size_t ProtocolHandler::GetHandshakedPeerCount() const
{
    return 0;  // Stub
}

// Private method stubs
void ProtocolHandler::HandleVersion(const io::UInt256& peer_id, const payloads::VersionPayload& payload)
{
    LOG_DEBUG("HandleVersion: peer={} version={} user_agent={}", peer_id.ToString(), payload.GetVersion(),
              payload.GetUserAgent());

    std::lock_guard<std::mutex> lock(mutex_);
    auto& peer_state = peer_states_[peer_id];

    // Check if already received version
    if (peer_state.version_received)
    {
        LOG_WARNING("Duplicate version message from peer {}", peer_id.ToString());
        if (disconnect_callback_)
        {
            disconnect_callback_(peer_id, "Duplicate version message");
        }
        return;
    }

    // Validate version compatibility
    if (payload.GetVersion() < config_.protocol_version)
    {
        LOG_WARNING("Incompatible protocol version from peer {}: {} < {}", peer_id.ToString(), payload.GetVersion(),
                    config_.protocol_version);
        if (disconnect_callback_)
        {
            disconnect_callback_(peer_id, "Incompatible protocol version");
        }
        return;
    }

    // Store peer state
    peer_state.version_received = true;
    peer_state.start_height = payload.GetStartHeight();

    // Send verack response
    if (send_callback_)
    {
        auto verackMsg = Message::Create(MessageCommand::Verack);
        send_callback_(peer_id, verackMsg);
    }

    LOG_INFO("Version handshake from peer {} completed", peer_id.ToString());
}

void ProtocolHandler::HandleVerack(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleVerack stub");
}

void ProtocolHandler::HandleGetAddr(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleGetAddr stub");
}

void ProtocolHandler::HandleAddr(const io::UInt256& peer_id, const payloads::AddrPayload& payload)
{
    LOG_DEBUG("HandleAddr stub");
}

void ProtocolHandler::HandlePing(const io::UInt256& peer_id, const payloads::PingPayload& payload)
{
    LOG_DEBUG("HandlePing: peer={} nonce={}", peer_id.ToString(), payload.GetNonce());

    // Respond with pong message containing same nonce
    if (send_callback_)
    {
        auto pongPayload = std::make_shared<payloads::PingPayload>();
        pongPayload->SetNonce(payload.GetNonce());
        pongPayload->SetTimestamp(payload.GetTimestamp());
        pongPayload->SetLastBlockIndex(payload.GetLastBlockIndex());
        auto pongMsg = Message::Create(MessageCommand::Pong, pongPayload);
        send_callback_(peer_id, pongMsg);
    }
}

void ProtocolHandler::HandlePong(const io::UInt256& peer_id, const payloads::PingPayload& payload)
{
    LOG_DEBUG("HandlePong: peer={} nonce={}", peer_id.ToString(), payload.GetNonce());

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = peer_states_.find(peer_id);
    if (it != peer_states_.end())
    {
        it->second.last_pong = std::chrono::steady_clock::now();
        LOG_DEBUG("Updated pong timestamp for peer {}", peer_id.ToString());
    }
}

void ProtocolHandler::HandleGetHeaders(const io::UInt256& peer_id, const payloads::GetHeadersPayload& payload)
{
    LOG_DEBUG("HandleGetHeaders stub");
}

void ProtocolHandler::HandleHeaders(const io::UInt256& peer_id, const payloads::HeadersPayload& payload)
{
    LOG_DEBUG("HandleHeaders stub");
}

void ProtocolHandler::HandleGetBlocks(const io::UInt256& peer_id, const payloads::GetBlocksPayload& payload)
{
    LOG_DEBUG("HandleGetBlocks stub");
}

void ProtocolHandler::HandleGetData(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleGetData stub");
}

void ProtocolHandler::HandleGetBlockByIndex(const io::UInt256& peer_id, const payloads::GetBlockByIndexPayload& payload)
{
    LOG_DEBUG("HandleGetBlockByIndex stub");
}

void ProtocolHandler::HandleInv(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleInv stub");
}

void ProtocolHandler::HandleBlock(const io::UInt256& peer_id, const payloads::BlockPayload& payload)
{
    LOG_DEBUG("HandleBlock stub");
}

void ProtocolHandler::HandleTransaction(const io::UInt256& peer_id, const payloads::TransactionPayload& payload)
{
    LOG_DEBUG("HandleTransaction stub");
}

void ProtocolHandler::HandleMempool(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleMempool stub");
}

void ProtocolHandler::HandleNotFound(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleNotFound stub");
}

std::vector<io::UInt256> ProtocolHandler::GetRelayPeers(const io::UInt256& exclude_peer) const
{
    return std::vector<io::UInt256>();  // Stub - no peers
}

void ProtocolHandler::RelayInventory(payloads::InventoryType type, const io::UInt256& hash,
                                     const io::UInt256& source_peer)
{
    LOG_DEBUG("RelayInventory stub: type={} hash={}", static_cast<uint8_t>(type), hash.ToString());
}

// SendReject is not in the public interface

}  // namespace neo::network::p2p