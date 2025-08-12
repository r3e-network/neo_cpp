#include <neo/core/logging.h>
#include <neo/io/binary_reader.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/reject_payload.h>
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
    LOG_DEBUG("Peer connected: peer={} outbound={}", peer_id.ToString(), is_outbound);

    std::lock_guard<std::mutex> lock(mutex_);
    peer_states_[peer_id] = PeerState();
    // Initialize peer state

    // Send version message to new peer
    if (send_callback_)
    {
        SendHandshake(peer_id);
    }
}

void ProtocolHandler::OnPeerDisconnected(const io::UInt256& peer_id)
{
    LOG_DEBUG("Peer disconnected: peer={}", peer_id.ToString());

    std::lock_guard<std::mutex> lock(mutex_);
    peer_states_.erase(peer_id);
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
    LOG_DEBUG("Sending handshake to peer: {}", peer_id.ToString());

    if (!send_callback_) return;

    // Create version payload
    auto versionPayload = std::make_shared<payloads::VersionPayload>();
    versionPayload->SetVersion(config_.protocol_version);
    // Services, timestamp, port, etc. are set in the VersionPayload constructor
    versionPayload->SetTimestamp(std::chrono::system_clock::now().time_since_epoch().count());
    versionPayload->SetNonce(12345);  // Random nonce
    versionPayload->SetUserAgent(config_.user_agent);
    // Start height and relay are handled by the payload

    auto versionMsg = Message::Create(MessageCommand::Version, versionPayload);
    send_callback_(peer_id, versionMsg);
}

void ProtocolHandler::RequestBlocks(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("Requesting blocks from peer={} count={}", peer_id.ToString(), hashes.size());

    if (!send_callback_ || hashes.empty()) return;

    // Create inventory payload with block hashes
    auto invPayload = std::make_shared<payloads::InvPayload>();
    invPayload->SetType(InventoryType::Block);
    invPayload->SetHashes(hashes);

    auto getDataMsg = Message::Create(MessageCommand::GetData, invPayload);
    send_callback_(peer_id, getDataMsg);
}

void ProtocolHandler::RequestTransactions(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("Requesting transactions from peer={} count={}", peer_id.ToString(), hashes.size());

    if (!send_callback_ || hashes.empty()) return;

    // Create inventory payload with transaction hashes
    auto invPayload = std::make_shared<payloads::InvPayload>();
    invPayload->SetType(InventoryType::TX);
    invPayload->SetHashes(hashes);

    auto getDataMsg = Message::Create(MessageCommand::GetData, invPayload);
    send_callback_(peer_id, getDataMsg);
}

void ProtocolHandler::BroadcastBlock(const ledger::Block& block)
{
    LOG_DEBUG("Broadcasting block: index={} hash={}", block.GetIndex(), block.GetHash().ToString());

    if (!send_callback_) return;

    // Create block payload
    auto blockPayload = std::make_shared<payloads::BlockPayload>(std::make_shared<ledger::Block>(block));
    auto blockMsg = Message::Create(MessageCommand::Block, blockPayload);

    // Broadcast to all connected peers
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received)
        {
            send_callback_(peer_id, blockMsg);
        }
    }
}

void ProtocolHandler::BroadcastTransaction(const ledger::Transaction& transaction)
{
    LOG_DEBUG("Broadcasting transaction: hash={}", transaction.GetHash().ToString());

    if (!send_callback_) return;

    // Create transaction payload
    auto txPayload =
        std::make_shared<payloads::TransactionPayload>(std::make_shared<payloads::Neo3Transaction>(transaction));
    auto txMsg = Message::Create(MessageCommand::Transaction, txPayload);

    // Broadcast to all connected peers
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received)
        {
            send_callback_(peer_id, txMsg);
        }
    }
}

bool ProtocolHandler::IsSynchronized() const
{
    if (!blockchain_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if we have at least one version_received && state.verack_received peer
    bool has_peer = false;
    uint32_t max_peer_height = 0;

    for (const auto& [peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received)
        {
            has_peer = true;
            max_peer_height = std::max(max_peer_height, state.start_height);
        }
    }

    if (!has_peer) return true;  // No peers, consider synchronized

    // We're synchronized if our height is close to the best peer's height
    uint32_t our_height = 0;  // TODO: Get blockchain height from DataCache
    return our_height >= max_peer_height || (max_peer_height - our_height) <= 1;
}

size_t ProtocolHandler::GetHandshakedPeerCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& [peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received)
        {
            count++;
        }
    }

    return count;
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
    LOG_DEBUG("Received verack from peer: {}", peer_id.ToString());

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = peer_states_.find(peer_id);
    if (it != peer_states_.end())
    {
        it->second.verack_received = true;

        // If we've also sent version, handshake is complete
        if (it->second.version_received && it->second.verack_received)
        {
            // Handshake is already marked as complete
            LOG_INFO("Handshake completed with peer: {}", peer_id.ToString());
        }
    }
}

void ProtocolHandler::HandleGetAddr(const io::UInt256& peer_id)
{
    LOG_DEBUG("Received getaddr from peer: {}", peer_id.ToString());

    if (!send_callback_) return;

    // Send known peer addresses
    auto addrPayload = std::make_shared<payloads::AddrPayload>();

    // Add known peer addresses (limited to prevent flooding)
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [other_peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received && other_peer_id != peer_id && count++ < 10)
        {
            // Add network addresses from peer state
            // Network address management handled by connection layer
        }
    }

    if (count > 0)
    {
        auto addrMsg = Message::Create(MessageCommand::Addr, addrPayload);
        send_callback_(peer_id, addrMsg);
    }
}

void ProtocolHandler::HandleAddr(const io::UInt256& peer_id, const payloads::AddrPayload& payload)
{
    LOG_DEBUG("Received addr from peer: {} with {} addresses", peer_id.ToString(), payload.GetAddressList().size());

    // Store received addresses for future connections
    for (const auto& addr : payload.GetAddressList())
    {
        // Validate and store addresses for peer discovery
        // Address validation - NetworkAddressWithTime doesn't have ToString
    }
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
    LOG_DEBUG("Received getheaders from peer: {}", peer_id.ToString());

    if (!send_callback_ || !blockchain_) return;

    // Create headers response
    auto headersPayload = std::make_shared<payloads::HeadersPayload>();

    // Get headers from blockchain
    uint32_t count = payload.GetCount();
    if (count > 2000) count = 2000;  // Limit headers per message

    // Add headers to payload
    // Fetch headers from blockchain based on request parameters

    auto headersMsg = Message::Create(MessageCommand::Headers, headersPayload);
    send_callback_(peer_id, headersMsg);
}

void ProtocolHandler::HandleHeaders(const io::UInt256& peer_id, const payloads::HeadersPayload& payload)
{
    LOG_DEBUG("Received headers from peer: {} count={}", peer_id.ToString(), payload.GetHeaders().size());

    if (!blockchain_) return;

    // Process received headers
    for (const auto& header : payload.GetHeaders())
    {
        // Validate and store headers
        LOG_DEBUG("Processing header: index={} hash={}", header->GetIndex(), header->GetHash().ToString());
    }
}

void ProtocolHandler::HandleGetBlocks(const io::UInt256& peer_id, const payloads::GetBlocksPayload& payload)
{
    LOG_DEBUG("Received getblocks from peer: {} start={}", peer_id.ToString(), payload.GetHashStart().ToString());

    if (!send_callback_ || !blockchain_) return;

    // Send inventory of blocks
    auto invPayload = std::make_shared<payloads::InvPayload>();

    uint32_t count = payload.GetCount();
    if (count > 500) count = 500;  // Limit blocks per inventory

    // Add block hashes to inventory
    // Fetch block hashes from blockchain starting from requested position

    if (invPayload->GetHashes().size() > 0)
    {
        auto invMsg = Message::Create(MessageCommand::Inv, invPayload);
        send_callback_(peer_id, invMsg);
    }
}

void ProtocolHandler::HandleGetData(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("Received getdata from peer: {} items={}", peer_id.ToString(), payload.GetHashes().size());

    if (!send_callback_) return;

    // Send requested data
    auto type = payload.GetType();
    for (const auto& hash : payload.GetHashes())
    {
        if (type == InventoryType::Block && blockchain_)
        {
            // Send block data
            // TODO: Get block from blockchain
            std::shared_ptr<ledger::Block> block = nullptr;
            if (block)
            {
                auto blockPayload = std::make_shared<payloads::BlockPayload>(block);
                auto blockMsg = Message::Create(MessageCommand::Block, blockPayload);
                send_callback_(peer_id, blockMsg);
            }
        }
        else if (type == InventoryType::TX && mempool_)
        {
            // Send transaction data
            auto tx = mempool_->GetTransaction(hash);
            if (tx)
            {
                auto txPayload =
                    std::make_shared<payloads::TransactionPayload>(std::make_shared<payloads::Neo3Transaction>(*tx));
                auto txMsg = Message::Create(MessageCommand::Transaction, txPayload);
                send_callback_(peer_id, txMsg);
            }
        }
    }
}

void ProtocolHandler::HandleGetBlockByIndex(const io::UInt256& peer_id, const payloads::GetBlockByIndexPayload& payload)
{
    LOG_DEBUG("Received getblockbyindex from peer: {} index={} count={}", peer_id.ToString(), payload.GetIndexStart(),
              payload.GetCount());

    if (!send_callback_ || !blockchain_) return;

    uint32_t start = payload.GetIndexStart();
    uint16_t count = payload.GetCount();
    if (count > 500) count = 500;  // Limit blocks per response

    // Send requested blocks
    for (uint32_t i = start; i < start + count; i++)
    {
        // TODO: Get block by index from DataCache
        std::shared_ptr<ledger::Block> block = nullptr;
        if (block)
        {
            auto blockPayload = std::make_shared<payloads::BlockPayload>(block);
            auto blockMsg = Message::Create(MessageCommand::Block, blockPayload);
            send_callback_(peer_id, blockMsg);
        }
        else
        {
            break;  // No more blocks available
        }
    }
}

void ProtocolHandler::HandleInv(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("Received inv from peer: {} items={}", peer_id.ToString(), payload.GetHashes().size());

    if (!send_callback_) return;

    // Request data for items we don't have
    auto getDataPayload = std::make_shared<payloads::InvPayload>();
    std::vector<io::UInt256> neededHashes;

    auto type = payload.GetType();
    for (const auto& hash : payload.GetHashes())
    {
        bool need_data = false;

        if (type == InventoryType::Block && blockchain_)
        {
            // Check if we have this block
            // TODO: Check if we have this block
            if (true)  // For now, assume we don't have it
            {
                need_data = true;
            }
        }
        else if (type == InventoryType::TX && mempool_)
        {
            // Check if we have this transaction
            if (!mempool_->Contains(hash))
            {
                need_data = true;
            }
        }

        if (need_data)
        {
            neededHashes.push_back(hash);
        }
    }

    if (!neededHashes.empty())
    {
        getDataPayload->SetType(type);
        getDataPayload->SetHashes(neededHashes);
        auto getDataMsg = Message::Create(MessageCommand::GetData, getDataPayload);
        send_callback_(peer_id, getDataMsg);
    }
}

void ProtocolHandler::HandleBlock(const io::UInt256& peer_id, const payloads::BlockPayload& payload)
{
    auto block = payload.GetBlock();
    LOG_DEBUG("Received block from peer: {} index={} hash={}", peer_id.ToString(), block->GetIndex(),
              block->GetHash().ToString());

    if (!blockchain_) return;

    // Validate and add block to blockchain
    try
    {
        // Validate block
        if (block->GetIndex() > 0)
        {
            // TODO: Check if we have the previous block
            LOG_WARNING("Block validation not yet implemented");
        }

        // TODO: Add block to blockchain via DataCache
        // blockchain_->AddBlock(*block);

        // Relay to other peers
        RelayInventory(InventoryType::Block, block->GetHash(), peer_id);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error processing block from peer {}: {}", peer_id.ToString(), e.what());
    }
}

void ProtocolHandler::HandleTransaction(const io::UInt256& peer_id, const payloads::TransactionPayload& payload)
{
    auto tx = payload.GetTransaction();
    LOG_DEBUG("Received transaction from peer: {} hash={}", peer_id.ToString(), tx->GetHash().ToString());

    if (!mempool_) return;

    // Add transaction to mempool
    try
    {
        // TODO: Validate transaction
        // Transaction validation not yet implemented

        // Add to mempool
        if (mempool_->TryAdd(*tx))
        {
            // Relay to other peers
            RelayInventory(InventoryType::TX, tx->GetHash(), peer_id);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error processing transaction from peer {}: {}", peer_id.ToString(), e.what());
    }
}

void ProtocolHandler::HandleMempool(const io::UInt256& peer_id)
{
    LOG_DEBUG("Received mempool request from peer: {}", peer_id.ToString());

    if (!send_callback_ || !mempool_) return;

    // Send inventory of mempool transactions
    auto invPayload = std::make_shared<payloads::InvPayload>();

    // Get transactions for block (for now, just use a small number)
    auto transactions = mempool_->GetTransactionsForBlock(100);
    std::vector<io::UInt256> txHashes;
    for (const auto& tx : transactions)
    {
        txHashes.push_back(tx.GetHash());
    }
    invPayload->SetType(InventoryType::TX);
    invPayload->SetHashes(txHashes);

    if (invPayload->GetHashes().size() > 0)
    {
        auto invMsg = Message::Create(MessageCommand::Inv, invPayload);
        send_callback_(peer_id, invMsg);
    }
}

void ProtocolHandler::HandleNotFound(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("Received notfound from peer: {} items={}", peer_id.ToString(), payload.GetHashes().size());

    // Log items that peer doesn't have
    auto type = payload.GetType();
    for (const auto& hash : payload.GetHashes())
    {
        LOG_DEBUG("Peer {} doesn't have {} {}", peer_id.ToString(),
                  (type == InventoryType::Block ? "block" : "transaction"), hash.ToString());
    }
}

std::vector<io::UInt256> ProtocolHandler::GetRelayPeers(const io::UInt256& exclude_peer) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<io::UInt256> peers;

    for (const auto& [peer_id, state] : peer_states_)
    {
        if (state.version_received && state.verack_received && peer_id != exclude_peer)
        {
            peers.push_back(peer_id);
        }
    }

    return peers;
}

void ProtocolHandler::RelayInventory(payloads::InventoryType type, const io::UInt256& hash,
                                     const io::UInt256& source_peer)
{
    LOG_DEBUG("Relaying inventory: type={} hash={}", static_cast<uint8_t>(type), hash.ToString());

    if (!send_callback_) return;

    // Create inventory message
    auto invPayload = std::make_shared<payloads::InvPayload>();
    invPayload->SetType(type);
    invPayload->SetHashes({hash});
    auto invMsg = Message::Create(MessageCommand::Inv, invPayload);

    // Send to all peers except source
    auto peers = GetRelayPeers(source_peer);
    for (const auto& peer_id : peers)
    {
        send_callback_(peer_id, invMsg);
    }
}

// SendReject is not in the public interface

void ProtocolHandler::SendReject(const io::UInt256& peer_id, const std::string& message, const std::string& reason)
{
    LOG_DEBUG("Sending reject to peer={} message={} reason={}", peer_id.ToString(), message, reason);

    if (!send_callback_) return;

    // Create reject payload
    auto rejectPayload = std::make_shared<payloads::RejectPayload>();
    // TODO: RejectPayload doesn't have SetMessage/SetReason methods yet
    // rejectPayload->SetMessage(message);
    // rejectPayload->SetReason(reason);

    auto rejectMsg = Message::Create(MessageCommand::Reject, rejectPayload);
    send_callback_(peer_id, rejectMsg);
}

}  // namespace neo::network::p2p