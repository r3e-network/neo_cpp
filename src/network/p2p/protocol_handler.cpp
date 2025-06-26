#include <neo/network/p2p/protocol_handler.h>
#include <neo/network/p2p/payloads/version.h>
#include <neo/network/p2p/payloads/verack.h>
#include <neo/network/p2p/payloads/getaddr.h>
#include <neo/network/p2p/payloads/addr.h>
#include <neo/network/p2p/payloads/ping.h>
#include <neo/network/p2p/payloads/pong.h>
#include <neo/network/p2p/payloads/getheaders.h>
#include <neo/network/p2p/payloads/headers.h>
#include <neo/network/p2p/payloads/getblocks.h>
#include <neo/network/p2p/payloads/getdata.h>
#include <neo/network/p2p/payloads/getblockbyindex.h>
#include <neo/network/p2p/payloads/inv.h>
#include <neo/network/p2p/payloads/block.h>
#include <neo/network/p2p/payloads/transaction.h>
#include <neo/network/p2p/payloads/mempool.h>
#include <neo/network/p2p/payloads/notfound.h>
#include <neo/network/p2p/payloads/reject.h>
#include <neo/network/p2p/message_command.h>
#include <neo/common/safe_math.h>
#include <algorithm>

namespace neo::network::p2p
{
    ProtocolHandler::ProtocolHandler(const Config& config,
                                   std::shared_ptr<persistence::DataCache> blockchain,
                                   std::shared_ptr<ledger::MemoryPool> mempool)
        : config_(config),
          logger_(core::Logger::GetInstance()),
          blockchain_(blockchain),
          mempool_(mempool)
    {
    }

    void ProtocolHandler::OnPeerConnected(const io::UInt256& peer_id, bool is_outbound)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        LOG_DEBUG("Peer connected: {} ({})", peer_id.ToString(), is_outbound ? "outbound" : "inbound");
        
        peer_states_[peer_id] = PeerState();
        peer_states_[peer_id].last_ping = std::chrono::steady_clock::now();
        
        if (is_outbound)
        {
            // Send version message for outbound connections
            SendHandshake(peer_id);
        }
    }

    void ProtocolHandler::OnPeerDisconnected(const io::UInt256& peer_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        LOG_DEBUG("Peer disconnected: {}", peer_id.ToString());
        
        peer_states_.erase(peer_id);
    }

    void ProtocolHandler::HandleMessage(const io::UInt256& peer_id, const Message& message)
    {
        LOG_DEBUG("Handling {} message from {}", message.GetCommand(), peer_id.ToString());
        
        try
        {
            switch (message.GetCommand())
            {
            case MessageCommand::Version:
                HandleVersion(peer_id, *static_cast<const VersionPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Verack:
                HandleVerack(peer_id);
                break;
                
            case MessageCommand::GetAddr:
                HandleGetAddr(peer_id);
                break;
                
            case MessageCommand::Addr:
                HandleAddr(peer_id, *static_cast<const AddrPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Ping:
                HandlePing(peer_id, *static_cast<const PingPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Pong:
                HandlePong(peer_id, *static_cast<const PongPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::GetHeaders:
                HandleGetHeaders(peer_id, *static_cast<const GetHeadersPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Headers:
                HandleHeaders(peer_id, *static_cast<const HeadersPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::GetBlocks:
                HandleGetBlocks(peer_id, *static_cast<const GetBlocksPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::GetData:
                HandleGetData(peer_id, *static_cast<const GetDataPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::GetBlockByIndex:
                HandleGetBlockByIndex(peer_id, *static_cast<const GetBlockByIndexPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Inv:
                HandleInv(peer_id, *static_cast<const InvPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Block:
                HandleBlock(peer_id, *static_cast<const BlockPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Transaction:
                HandleTransaction(peer_id, *static_cast<const TransactionPayload*>(message.GetPayload()));
                break;
                
            case MessageCommand::Mempool:
                HandleMempool(peer_id);
                break;
                
            case MessageCommand::NotFound:
                HandleNotFound(peer_id, *static_cast<const NotFoundPayload*>(message.GetPayload()));
                break;
                
            default:
                LOG_WARNING("Unknown message command: {}", static_cast<uint8_t>(message.GetCommand()));
                break;
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error handling message from {}: {}", peer_id.ToString(), e.what());
            if (disconnect_callback_)
            {
                disconnect_callback_(peer_id, "Protocol error");
            }
        }
    }

    void ProtocolHandler::SendHandshake(const io::UInt256& peer_id)
    {
        if (!send_callback_)
        {
            LOG_ERROR("Send callback not set");
            return;
        }

        // Create version message
        VersionPayload version;
        version.SetVersion(config_.protocol_version);
        version.SetServices(1); // NODE_NETWORK
        version.SetTimestamp(std::chrono::system_clock::now());
        version.SetPort(0); // TODO: Get local port
        version.SetNonce(0); // TODO: Generate nonce
        version.SetUserAgent(config_.user_agent);
        version.SetStartHeight(blockchain_ ? blockchain_->GetHeight() : 0);
        version.SetRelay(true);

        Message msg(MessageCommand::Version, &version);
        send_callback_(peer_id, msg);
    }

    void ProtocolHandler::RequestBlocks(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Cannot request blocks from non-handshaked peer");
            return;
        }

        GetDataPayload getdata;
        for (const auto& hash : hashes)
        {
            getdata.AddInventory(InventoryType::Block, hash);
        }

        Message msg(MessageCommand::GetData, &getdata);
        send_callback_(peer_id, msg);

        // Track requested blocks
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = peer_states_.find(peer_id);
            if (it != peer_states_.end())
            {
                for (const auto& hash : hashes)
                {
                    it->second.requested_blocks.push(hash);
                }
            }
        }
    }

    void ProtocolHandler::RequestTransactions(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Cannot request transactions from non-handshaked peer");
            return;
        }

        GetDataPayload getdata;
        for (const auto& hash : hashes)
        {
            getdata.AddInventory(InventoryType::Transaction, hash);
        }

        Message msg(MessageCommand::GetData, &getdata);
        send_callback_(peer_id, msg);

        // Track requested transactions
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = peer_states_.find(peer_id);
            if (it != peer_states_.end())
            {
                for (const auto& hash : hashes)
                {
                    it->second.requested_transactions.push(hash);
                }
            }
        }
    }

    void ProtocolHandler::BroadcastBlock(const ledger::Block& block)
    {
        auto hash = block.GetHash();
        RelayInventory(InventoryType::Block, hash, io::UInt256());
    }

    void ProtocolHandler::BroadcastTransaction(const ledger::Transaction& transaction)
    {
        auto hash = transaction.GetHash();
        RelayInventory(InventoryType::Transaction, hash, io::UInt256());
    }

    bool ProtocolHandler::IsSynchronized() const
    {
        // TODO: Implement synchronization check
        return true;
    }

    size_t ProtocolHandler::GetHandshakedPeerCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        return std::count_if(peer_states_.begin(), peer_states_.end(),
            [](const auto& pair) {
                return pair.second.version_received && pair.second.verack_received;
            });
    }

    // Message handler implementations

    void ProtocolHandler::HandleVersion(const io::UInt256& peer_id, const VersionPayload& payload)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = peer_states_.find(peer_id);
        if (it == peer_states_.end())
        {
            LOG_WARNING("Version from unknown peer");
            return;
        }

        if (it->second.version_received)
        {
            LOG_WARNING("Duplicate version message");
            return;
        }

        // Validate version
        if (payload.GetVersion() < config_.protocol_version)
        {
            LOG_WARNING("Peer version too old: {}", payload.GetVersion());
            if (disconnect_callback_)
            {
                disconnect_callback_(peer_id, "Version too old");
            }
            return;
        }

        it->second.version_received = true;
        it->second.start_height = payload.GetStartHeight();

        // Send verack
        VerackPayload verack;
        Message verack_msg(MessageCommand::Verack, &verack);
        send_callback_(peer_id, verack_msg);

        // Send version if we haven't already (inbound connection)
        if (!it->second.verack_received)
        {
            SendHandshake(peer_id);
        }
    }

    void ProtocolHandler::HandleVerack(const io::UInt256& peer_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = peer_states_.find(peer_id);
        if (it == peer_states_.end())
        {
            LOG_WARNING("Verack from unknown peer");
            return;
        }

        if (!it->second.version_received)
        {
            LOG_WARNING("Verack before version");
            return;
        }

        it->second.verack_received = true;
        
        LOG_INFO("Handshake completed with peer {}", peer_id.ToString());

        // Request addresses
        GetAddrPayload getaddr;
        Message getaddr_msg(MessageCommand::GetAddr, &getaddr);
        send_callback_(peer_id, getaddr_msg);

        // Start synchronization if needed
        if (blockchain_ && it->second.start_height > blockchain_->GetHeight())
        {
            // Request headers
            GetHeadersPayload getheaders;
            getheaders.SetHashStart({blockchain_->GetCurrentBlockHash()});
            Message getheaders_msg(MessageCommand::GetHeaders, &getheaders);
            send_callback_(peer_id, getheaders_msg);
        }
    }

    void ProtocolHandler::HandleGetAddr(const io::UInt256& peer_id)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("GetAddr from non-handshaked peer");
            return;
        }

        // TODO: Get known addresses from peer manager
        AddrPayload addr;
        // addr.AddAddress(...);
        
        Message msg(MessageCommand::Addr, &addr);
        send_callback_(peer_id, msg);
    }

    void ProtocolHandler::HandleAddr(const io::UInt256& peer_id, const AddrPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Addr from non-handshaked peer");
            return;
        }

        // TODO: Process addresses and add to peer manager
        for (const auto& addr : payload.GetAddresses())
        {
            LOG_DEBUG("Received address: {}", addr.GetEndpoint().ToString());
        }
    }

    void ProtocolHandler::HandlePing(const io::UInt256& peer_id, const PingPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Ping from non-handshaked peer");
            return;
        }

        // Send pong with same payload
        PongPayload pong;
        pong.SetPayload(payload.GetPayload());
        
        Message msg(MessageCommand::Pong, &pong);
        send_callback_(peer_id, msg);
    }

    void ProtocolHandler::HandlePong(const io::UInt256& peer_id, const PongPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Pong from non-handshaked peer");
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = peer_states_.find(peer_id);
        if (it != peer_states_.end())
        {
            it->second.last_pong = std::chrono::steady_clock::now();
        }
    }

    void ProtocolHandler::HandleGetHeaders(const io::UInt256& peer_id, const GetHeadersPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("GetHeaders from non-handshaked peer");
            return;
        }

        if (!blockchain_)
        {
            return;
        }

        HeadersPayload headers;
        
        // TODO: Get headers from blockchain
        // auto header_list = blockchain_->GetHeaders(payload.GetHashStart(), config_.max_headers_per_message);
        // for (const auto& header : header_list)
        // {
        //     headers.AddHeader(header);
        // }

        Message msg(MessageCommand::Headers, &headers);
        send_callback_(peer_id, msg);
    }

    void ProtocolHandler::HandleHeaders(const io::UInt256& peer_id, const HeadersPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Headers from non-handshaked peer");
            return;
        }

        if (!blockchain_)
        {
            return;
        }

        // Process headers
        std::vector<io::UInt256> blocks_to_request;
        
        for (const auto& header : payload.GetHeaders())
        {
            // TODO: Validate and process header
            // if (blockchain_->ValidateHeader(header))
            // {
            //     blocks_to_request.push_back(header.GetHash());
            // }
        }

        // Request blocks
        if (!blocks_to_request.empty())
        {
            RequestBlocks(peer_id, blocks_to_request);
        }
    }

    void ProtocolHandler::HandleGetBlocks(const io::UInt256& peer_id, const GetBlocksPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("GetBlocks from non-handshaked peer");
            return;
        }

        if (!blockchain_)
        {
            return;
        }

        // Send inventory of blocks
        InvPayload inv;
        
        // TODO: Get block hashes from blockchain
        // auto hashes = blockchain_->GetBlockHashes(payload.GetHashStart(), payload.GetCount());
        // for (const auto& hash : hashes)
        // {
        //     inv.AddInventory(InventoryType::Block, hash);
        // }

        Message msg(MessageCommand::Inv, &inv);
        send_callback_(peer_id, msg);
    }

    void ProtocolHandler::HandleGetData(const io::UInt256& peer_id, const GetDataPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("GetData from non-handshaked peer");
            return;
        }

        std::vector<InventoryVector> not_found;

        for (const auto& inv : payload.GetInventory())
        {
            bool found = false;

            if (inv.GetType() == InventoryType::Block && blockchain_)
            {
                // TODO: Get block from blockchain
                // auto block = blockchain_->GetBlock(inv.GetHash());
                // if (block)
                // {
                //     BlockPayload block_payload(block);
                //     Message msg(MessageCommand::Block, &block_payload);
                //     send_callback_(peer_id, msg);
                //     found = true;
                // }
            }
            else if (inv.GetType() == InventoryType::Transaction)
            {
                // Check mempool first
                if (mempool_)
                {
                    // TODO: Get transaction from mempool
                    // auto tx = mempool_->GetTransaction(inv.GetHash());
                    // if (tx)
                    // {
                    //     TransactionPayload tx_payload(tx);
                    //     Message msg(MessageCommand::Transaction, &tx_payload);
                    //     send_callback_(peer_id, msg);
                    //     found = true;
                    // }
                }

                // Check blockchain
                if (!found && blockchain_)
                {
                    // TODO: Get transaction from blockchain
                    // auto tx = blockchain_->GetTransaction(inv.GetHash());
                    // if (tx)
                    // {
                    //     TransactionPayload tx_payload(tx);
                    //     Message msg(MessageCommand::Transaction, &tx_payload);
                    //     send_callback_(peer_id, msg);
                    //     found = true;
                    // }
                }
            }

            if (!found)
            {
                not_found.push_back(inv);
            }
        }

        // Send not found for missing items
        if (!not_found.empty())
        {
            NotFoundPayload notfound;
            for (const auto& inv : not_found)
            {
                notfound.AddInventory(inv.GetType(), inv.GetHash());
            }
            
            Message msg(MessageCommand::NotFound, &notfound);
            send_callback_(peer_id, msg);
        }
    }

    void ProtocolHandler::HandleGetBlockByIndex(const io::UInt256& peer_id, const GetBlockByIndexPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("GetBlockByIndex from non-handshaked peer");
            return;
        }

        if (!blockchain_)
        {
            return;
        }

        uint32_t count = std::min(payload.GetCount(), config_.max_blocks_per_message);
        
        for (uint32_t i = 0; i < count; i++)
        {
            uint32_t index = payload.GetIndexStart() + i;
            
            // TODO: Get block by index
            // auto block = blockchain_->GetBlock(index);
            // if (block)
            // {
            //     BlockPayload block_payload(block);
            //     Message msg(MessageCommand::Block, &block_payload);
            //     send_callback_(peer_id, msg);
            // }
            // else
            // {
            //     break; // No more blocks
            // }
        }
    }

    void ProtocolHandler::HandleInv(const io::UInt256& peer_id, const InvPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Inv from non-handshaked peer");
            return;
        }

        std::vector<InventoryVector> to_request;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = peer_states_.find(peer_id);
            if (it != peer_states_.end())
            {
                for (const auto& inv : payload.GetInventory())
                {
                    // Update known hashes
                    it->second.known_hashes.insert(inv.GetHash());

                    // Check if we need this item
                    bool need_item = false;

                    if (inv.GetType() == InventoryType::Block && blockchain_)
                    {
                        // TODO: Check if we have this block
                        // need_item = !blockchain_->HasBlock(inv.GetHash());
                    }
                    else if (inv.GetType() == InventoryType::Transaction && mempool_)
                    {
                        // TODO: Check if we have this transaction
                        // need_item = !mempool_->HasTransaction(inv.GetHash()) && 
                        //            !blockchain_->HasTransaction(inv.GetHash());
                    }

                    if (need_item)
                    {
                        to_request.push_back(inv);
                    }
                }
            }
        }

        // Request items we don't have
        if (!to_request.empty())
        {
            GetDataPayload getdata;
            for (const auto& inv : to_request)
            {
                getdata.AddInventory(inv.GetType(), inv.GetHash());
            }

            Message msg(MessageCommand::GetData, &getdata);
            send_callback_(peer_id, msg);
        }
    }

    void ProtocolHandler::HandleBlock(const io::UInt256& peer_id, const BlockPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Block from non-handshaked peer");
            return;
        }

        const auto& block = payload.GetBlock();
        
        // Validate block
        if (!ValidateBlock(block))
        {
            LOG_WARNING("Invalid block from peer {}", peer_id.ToString());
            SendReject(peer_id, "block", "invalid");
            return;
        }

        // Process block
        if (blockchain_)
        {
            // TODO: Add block to blockchain
            // if (blockchain_->AddBlock(block))
            // {
            //     // Relay to other peers
            //     RelayInventory(InventoryType::Block, block.GetHash(), peer_id);
            // }
        }

        // Update peer state
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = peer_states_.find(peer_id);
            if (it != peer_states_.end() && !it->second.requested_blocks.empty())
            {
                if (it->second.requested_blocks.front() == block.GetHash())
                {
                    it->second.requested_blocks.pop();
                }
            }
        }
    }

    void ProtocolHandler::HandleTransaction(const io::UInt256& peer_id, const TransactionPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Transaction from non-handshaked peer");
            return;
        }

        const auto& transaction = payload.GetTransaction();

        // Validate transaction
        if (!ValidateTransaction(transaction))
        {
            LOG_WARNING("Invalid transaction from peer {}", peer_id.ToString());
            SendReject(peer_id, "tx", "invalid");
            return;
        }

        // Add to mempool
        if (mempool_)
        {
            // TODO: Add transaction to mempool
            // if (mempool_->TryAdd(transaction))
            // {
            //     // Relay to other peers
            //     RelayInventory(InventoryType::Transaction, transaction.GetHash(), peer_id);
            // }
        }

        // Update peer state
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = peer_states_.find(peer_id);
            if (it != peer_states_.end() && !it->second.requested_transactions.empty())
            {
                if (it->second.requested_transactions.front() == transaction.GetHash())
                {
                    it->second.requested_transactions.pop();
                }
            }
        }
    }

    void ProtocolHandler::HandleMempool(const io::UInt256& peer_id)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("Mempool from non-handshaked peer");
            return;
        }

        if (!mempool_)
        {
            return;
        }

        // Send inventory of mempool transactions
        InvPayload inv;
        
        // TODO: Get mempool transactions
        // auto txs = mempool_->GetAllTransactions();
        // for (const auto& tx : txs)
        // {
        //     inv.AddInventory(InventoryType::Transaction, tx.GetHash());
        // }

        if (inv.GetInventory().size() > 0)
        {
            Message msg(MessageCommand::Inv, &inv);
            send_callback_(peer_id, msg);
        }
    }

    void ProtocolHandler::HandleNotFound(const io::UInt256& peer_id, const NotFoundPayload& payload)
    {
        if (!IsPeerHandshaked(peer_id))
        {
            LOG_WARNING("NotFound from non-handshaked peer");
            return;
        }

        // Log not found items
        for (const auto& inv : payload.GetInventory())
        {
            LOG_DEBUG("Peer {} does not have {} {}", 
                     peer_id.ToString(),
                     inv.GetType() == InventoryType::Block ? "block" : "transaction",
                     inv.GetHash().ToString());
        }

        // TODO: Handle not found items (e.g., request from another peer)
    }

    bool ProtocolHandler::IsPeerHandshaked(const io::UInt256& peer_id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = peer_states_.find(peer_id);
        return it != peer_states_.end() && 
               it->second.version_received && 
               it->second.verack_received;
    }

    void ProtocolHandler::UpdateKnownHashes(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = peer_states_.find(peer_id);
        if (it != peer_states_.end())
        {
            for (const auto& hash : hashes)
            {
                it->second.known_hashes.insert(hash);
            }
        }
    }

    bool ProtocolHandler::PeerKnowsHash(const io::UInt256& peer_id, const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = peer_states_.find(peer_id);
        return it != peer_states_.end() && 
               it->second.known_hashes.count(hash) > 0;
    }

    std::vector<io::UInt256> ProtocolHandler::GetRelayPeers(const io::UInt256& exclude_peer) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<io::UInt256> peers;
        
        for (const auto& [peer_id, state] : peer_states_)
        {
            if (peer_id != exclude_peer && 
                state.version_received && 
                state.verack_received)
            {
                peers.push_back(peer_id);
            }
        }
        
        return peers;
    }

    void ProtocolHandler::RelayInventory(InventoryType type, const io::UInt256& hash, const io::UInt256& source_peer)
    {
        auto peers = GetRelayPeers(source_peer);
        
        if (peers.empty())
        {
            return;
        }

        // Create inventory message
        InvPayload inv;
        inv.AddInventory(type, hash);
        Message msg(MessageCommand::Inv, &inv);

        // Send to all relay peers that don't know about this item
        for (const auto& peer_id : peers)
        {
            if (!PeerKnowsHash(peer_id, hash))
            {
                send_callback_(peer_id, msg);
                UpdateKnownHashes(peer_id, {hash});
            }
        }
    }

    void ProtocolHandler::SendReject(const io::UInt256& peer_id, const std::string& message, const std::string& reason)
    {
        RejectPayload reject;
        reject.SetMessage(message);
        reject.SetReason(reason);
        
        Message msg(MessageCommand::Reject, &reject);
        send_callback_(peer_id, msg);
    }

    bool ProtocolHandler::ValidateBlock(const ledger::Block& block) const
    {
        // TODO: Implement block validation
        // - Check block header validity
        // - Check merkle root
        // - Check transactions
        // - Check consensus data
        return true;
    }

    bool ProtocolHandler::ValidateTransaction(const ledger::Transaction& transaction) const
    {
        // TODO: Implement transaction validation
        // - Check transaction format
        // - Check signatures
        // - Check fees
        // - Check double spend
        return true;
    }
}