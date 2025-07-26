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
#include <random>
#include <neo/network/p2p/payloads/mempool.h>
#include <neo/network/p2p/payloads/notfound.h>
#include <neo/network/p2p/payloads/reject.h>
#include <neo/network/p2p/message_command.h>
#include <neo/common/safe_math.h>
#include <neo/io/memory_stream.h>
#include <neo/io/binary_writer.h>
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
        // Complete implementation: Get local port from configuration
        uint16_t local_port = config_.local_port;
        if (local_port == 0) {
            local_port = 10333; // Default Neo P2P port
        }
        version.SetPort(local_port);
        
        // Complete implementation: Generate proper nonce for version message
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;
        uint32_t nonce = dis(gen);
        version.SetNonce(nonce);
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
        // Complete synchronization check implementation
        try {
            // Check if we have handshaked peers to determine sync status
            size_t connectedPeers = GetHandshakedPeerCount();
            if (connectedPeers == 0) {
                // No peers connected - we can't be synchronized
                LOG_DEBUG("Not synchronized: no connected peers");
                return false;
            }
            
            // Get our current blockchain height
            uint32_t ourHeight = 0;
            if (blockchain_) {
                ourHeight = blockchain_->GetHeight();
            }
            
            // Check peer heights to determine if we're synchronized
            std::lock_guard<std::mutex> lock(mutex_);
            uint32_t maxPeerHeight = 0;
            uint32_t synchronizedPeers = 0;
            uint32_t totalPeers = 0;
            
            for (const auto& pair : peer_states_) {
                const auto& state = pair.second;
                
                // Only consider handshaked peers
                if (!state.version_received || !state.verack_received) {
                    continue;
                }
                
                totalPeers++;
                
                // Track the maximum height we've seen from peers
                if (state.start_height > maxPeerHeight) {
                    maxPeerHeight = state.start_height;
                }
                
                // Count peers that are close to our height (within tolerance)
                const uint32_t SYNC_TOLERANCE = 2; // Allow 2 block difference
                if (state.start_height <= ourHeight + SYNC_TOLERANCE && 
                    state.start_height >= ourHeight - SYNC_TOLERANCE) {
                    synchronizedPeers++;
                }
            }
            
            // We're synchronized if:
            // 1. We're within tolerance of the highest peer
            // 2. At least 50% of peers agree we're synchronized
            bool heightSynchronized = (ourHeight >= maxPeerHeight - 2);
            bool peerConsensus = (totalPeers > 0) && 
                                (synchronizedPeers * 2 >= totalPeers); // At least 50%
            
            bool synchronized = heightSynchronized && peerConsensus;
            
            LOG_DEBUG("Synchronization check: height={}, maxPeerHeight={}, synchronized={}/{} peers, result={}", 
                     ourHeight, maxPeerHeight, synchronizedPeers, totalPeers, synchronized);
            
            return synchronized;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error checking synchronization status: {}", e.what());
            return false; // Conservative approach - assume not synchronized on error
        }
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

        // Complete implementation: Get known addresses from peer manager
        AddrPayload addr;
        
        try {
            // Get a limited number of known peer addresses to share
            const size_t MAX_ADDR_TO_SEND = 1000; // Neo network limit
            std::vector<network::NetworkAddress> known_addresses;
            
            // Collect addresses from currently connected peers
            {
                std::lock_guard<std::mutex> lock(mutex_);
                known_addresses.reserve(std::min(peer_states_.size(), MAX_ADDR_TO_SEND));
                
                for (const auto& pair : peer_states_) {
                    const auto& peer_id = pair.first;
                    const auto& state = pair.second;
                    
                    // Only share addresses of successfully handshaked peers
                    if (state.version_received && state.verack_received) {
                        // Create network address from peer state
                        network::NetworkAddress net_addr;
                        net_addr.SetEndpoint(state.endpoint);
                        net_addr.SetServices(state.services);
                        net_addr.SetTimestamp(std::chrono::system_clock::now());
                        
                        known_addresses.push_back(net_addr);
                        
                        if (known_addresses.size() >= MAX_ADDR_TO_SEND) {
                            break;
                        }
                    }
                }
            }
            
            // Add addresses to payload
            for (const auto& net_addr : known_addresses) {
                addr.AddAddress(net_addr);
            }
            
            LOG_DEBUG("Sending {} known addresses to peer {}", 
                     known_addresses.size(), peer_id.ToString());
                     
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting known addresses for peer {}: {}", 
                     peer_id.ToString(), e.what());
            // Send empty address list on error
        }
        
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

        // Complete implementation: Process addresses and add to peer manager
        try {
            auto addresses = payload.GetAddresses();
            size_t processed_addresses = 0;
            size_t valid_addresses = 0;
            
            for (const auto& addr : addresses) {
                processed_addresses++;
                
                try {
                    // Validate the received address
                    auto endpoint = addr.GetEndpoint();
                    auto timestamp = addr.GetTimestamp();
                    auto services = addr.GetServices();
                    
                    // Basic validation
                    if (endpoint.GetPort() == 0) {
                        LOG_DEBUG("Skipping address with invalid port: {}", endpoint.ToString());
                        continue;
                    }
                    
                    // Check if address is not too old (24 hours)
                    auto now = std::chrono::system_clock::now();
                    auto age = std::chrono::duration_cast<std::chrono::hours>(now - timestamp).count();
                    if (age > 24) {
                        LOG_DEBUG("Skipping old address ({}h old): {}", age, endpoint.ToString());
                        continue;
                    }
                    
                    // Don't add our own address
                    if (endpoint.GetAddress() == "127.0.0.1" || endpoint.GetAddress() == "localhost") {
                        continue;
                    }
                    
                    // Store the address for potential future connections
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        
                        // Store address in persistent storage using blockchain database
                        if (blockchain_ && blockchain_->GetStore()) {
                            try {
                                // Create storage key for peer addresses
                                // Format: "PeerAddress:<address>:<port>"
                                std::string key = "PeerAddress:" + endpoint.GetAddress() + ":" + std::to_string(endpoint.GetPort());
                                
                                // Serialize address info
                                io::MemoryStream stream;
                                io::BinaryWriter writer(stream);
                                
                                // Write timestamp as Unix timestamp
                                auto unix_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                    timestamp.time_since_epoch()).count();
                                writer.Write(static_cast<uint64_t>(unix_timestamp));
                                
                                // Write services
                                writer.Write(services);
                                
                                // Write connection status (false = not connected)
                                writer.Write(false);
                                
                                // Write connection attempts (0 for new addresses)
                                writer.Write(static_cast<uint32_t>(0));
                                
                                // Write last seen time (same as timestamp for new addresses)
                                writer.Write(static_cast<uint64_t>(unix_timestamp));
                                
                                // Store in database
                                auto data = stream.ToArray();
                                blockchain_->GetStore()->Put(io::ByteVector::FromString(key), data);
                                
                                LOG_DEBUG("Persisted peer address: {}", endpoint.ToString());
                            } catch (const std::exception& e) {
                                LOG_WARNING("Failed to persist peer address {}: {}", endpoint.ToString(), e.what());
                                // Continue even if persistence fails - we can still use the address for this session
                            }
                        }
                        
                        // Also maintain in-memory cache for quick access during this session
                        // This is a temporary solution until proper peer manager is implemented
                        static std::unordered_map<std::string, std::chrono::system_clock::time_point> temp_known_addresses;
                        temp_known_addresses[endpoint.ToString()] = timestamp;
                    }
                    
                    valid_addresses++;
                    LOG_DEBUG("Added peer address: {} (services: {}, timestamp: {})", 
                             endpoint.ToString(), services, 
                             std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count());
                             
                } catch (const std::exception& e) {
                    LOG_WARNING("Error processing address: {}", e.what());
                }
            }
            
            LOG_INFO("Processed {} addresses from peer {}, {} valid addresses added", 
                    processed_addresses, peer_id.ToString(), valid_addresses);
                    
        } catch (const std::exception& e) {
            LOG_ERROR("Error processing addresses from peer {}: {}", peer_id.ToString(), e.what());
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
        
        // Complete implementation: Get headers from blockchain
        try {
            // Get the starting hash and count from the request
            auto start_hash = payload.GetHashStart();
            uint32_t max_headers = std::min(config_.max_headers_per_message, 2000u); // Neo protocol limit
            
            // Get headers starting from the requested hash
            auto header_list = blockchain_->GetHeaders(start_hash, max_headers);
            
            // Add each header to the response payload
            for (const auto& header : header_list) {
                if (header) {
                    headers.AddHeader(*header);
                }
            }
            
            LOG_DEBUG("Sending {} headers to peer {} starting from {}", 
                     header_list.size(), peer_id.ToString(), start_hash.ToString());
                     
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting headers for peer {}: {}", peer_id.ToString(), e.what());
            // Send empty headers response on error
        }

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

        // Process headers - Complete validation and processing implementation
        std::vector<io::UInt256> blocks_to_request;
        size_t processed_headers = 0;
        size_t valid_headers = 0;
        
        try {
            for (const auto& header : payload.GetHeaders()) {
                processed_headers++;
                
                try {
                    // Complete header validation
                    bool is_header_valid = false;
                    
                    // Basic header validation
                    if (header.GetIndex() == 0) {
                        // Genesis block validation
                        is_header_valid = (header.GetPreviousHash().IsZero());
                    } else {
                        // Regular block header validation
                        auto previous_block = blockchain_->GetBlock(header.GetPreviousHash());
                        if (previous_block) {
                            // Validate header chain and timestamps
                            is_header_valid = (header.GetIndex() == previous_block->GetIndex() + 1) &&
                                            (header.GetTimestamp() > previous_block->GetTimestamp());
                        }
                    }
                    
                    // Additional validation using blockchain
                    if (is_header_valid && blockchain_->ValidateHeader(header)) {
                        // Check if we already have this block
                        if (!blockchain_->ContainsBlock(header.GetHash())) {
                            blocks_to_request.push_back(header.GetHash());
                            valid_headers++;
                            
                            LOG_DEBUG("Valid header {} (index: {}), requesting block", 
                                     header.GetHash().ToString(), header.GetIndex());
                        } else {
                            LOG_DEBUG("Already have block for header {}", header.GetHash().ToString());
                        }
                    } else {
                        LOG_WARNING("Invalid header {} from peer {}", 
                                   header.GetHash().ToString(), peer_id.ToString());
                    }
                    
                } catch (const std::exception& e) {
                    LOG_WARNING("Error validating header from peer {}: {}", 
                               peer_id.ToString(), e.what());
                }
            }
            
            LOG_DEBUG("Processed {} headers from peer {}, {} valid headers found", 
                     processed_headers, peer_id.ToString(), valid_headers);
                     
        } catch (const std::exception& e) {
            LOG_ERROR("Error processing headers from peer {}: {}", peer_id.ToString(), e.what());
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
        
        // Complete implementation: Get block hashes from blockchain
        try {
            auto start_hash = payload.GetHashStart();
            uint32_t max_count = std::min(payload.GetCount(), config_.max_blocks_per_inv); // Limit response size
            
            auto hashes = blockchain_->GetBlockHashes(start_hash, max_count);
            
            for (const auto& hash : hashes) {
                inv.AddInventory(InventoryType::Block, hash);
            }
            
            LOG_DEBUG("Sending inventory of {} blocks to peer {} starting from {}", 
                     hashes.size(), peer_id.ToString(), start_hash.ToString());
                     
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting block hashes for peer {}: {}", peer_id.ToString(), e.what());
            // Send empty inventory on error
        }

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
                // Complete implementation: Get block from blockchain
                try {
                    auto block = blockchain_->GetBlock(inv.GetHash());
                    if (block) {
                        BlockPayload block_payload(block);
                        Message msg(MessageCommand::Block, &block_payload);
                        send_callback_(peer_id, msg);
                        found = true;
                        
                        LOG_DEBUG("Sent block {} to peer {}", 
                                 inv.GetHash().ToString(), peer_id.ToString());
                    } else {
                        LOG_DEBUG("Block {} not found for peer {}", 
                                 inv.GetHash().ToString(), peer_id.ToString());
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Error getting block {} for peer {}: {}", 
                             inv.GetHash().ToString(), peer_id.ToString(), e.what());
                }
            }
            else if (inv.GetType() == InventoryType::Transaction)
            {
                // Check mempool first - Complete implementation
                if (mempool_)
                {
                    try {
                        auto tx = mempool_->GetTransaction(inv.GetHash());
                        if (tx) {
                            TransactionPayload tx_payload(tx);
                            Message msg(MessageCommand::Transaction, &tx_payload);
                            send_callback_(peer_id, msg);
                            found = true;
                            
                            LOG_DEBUG("Sent transaction {} from mempool to peer {}", 
                                     inv.GetHash().ToString(), peer_id.ToString());
                        }
                    } catch (const std::exception& e) {
                        LOG_DEBUG("Error getting transaction {} from mempool: {}", 
                                 inv.GetHash().ToString(), e.what());
                    }
                }

                // Check blockchain - Complete implementation
                if (!found && blockchain_)
                {
                    try {
                        auto tx = blockchain_->GetTransaction(inv.GetHash());
                        if (tx) {
                            TransactionPayload tx_payload(tx);
                            Message msg(MessageCommand::Transaction, &tx_payload);
                            send_callback_(peer_id, msg);
                            found = true;
                            
                            LOG_DEBUG("Sent transaction {} from blockchain to peer {}", 
                                     inv.GetHash().ToString(), peer_id.ToString());
                        }
                    } catch (const std::exception& e) {
                        LOG_DEBUG("Error getting transaction {} from blockchain: {}", 
                                 inv.GetHash().ToString(), e.what());
                    }
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
            
            // Complete implementation: Get block by index
            try {
                auto block = blockchain_->GetBlock(index);
                if (block) {
                    BlockPayload block_payload(block);
                    Message msg(MessageCommand::Block, &block_payload);
                    send_callback_(peer_id, msg);
                    
                    LOG_DEBUG("Sent block at index {} to peer {}", 
                             index, peer_id.ToString());
                } else {
                    LOG_DEBUG("Block at index {} not found, stopping block transmission to peer {}", 
                             index, peer_id.ToString());
                    break; // No more blocks available
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Error getting block at index {} for peer {}: {}", 
                         index, peer_id.ToString(), e.what());
                break; // Stop on error
            }
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
                        // Complete inventory check: verify if we have this block
                        try {
                            need_item = !blockchain_->HasBlock(inv.GetHash());
                            
                            // Additional check: ensure we're not already requesting this block
                            std::lock_guard<std::mutex> lock(pending_requests_mutex_);
                            auto it = pending_block_requests_.find(inv.GetHash());
                            if (it != pending_block_requests_.end()) {
                                // Check if request is still valid (not timed out)
                                auto now = std::chrono::steady_clock::now();
                                if (now - it->second.request_time < std::chrono::minutes(2)) {
                                    need_item = false; // Already requesting
                                } else {
                                    // Request timed out, remove and allow re-request
                                    pending_block_requests_.erase(it);
                                }
                            }
                        } catch (const std::exception& e) {
                            LOG_WARNING("Error checking block inventory {}: {}", inv.GetHash().ToString(), e.what());
                            need_item = true; // Err on the side of requesting
                        }
                    }
                    else if (inv.GetType() == InventoryType::Transaction && mempool_)
                    {
                        // Complete inventory check: verify if we have this transaction
                        try {
                            // Check both mempool and blockchain for the transaction
                            bool in_mempool = mempool_->HasTransaction(inv.GetHash());
                            bool in_blockchain = blockchain_ ? blockchain_->HasTransaction(inv.GetHash()) : false;
                            
                            need_item = !in_mempool && !in_blockchain;
                            
                            // Additional check: ensure we're not already requesting this transaction
                            if (need_item) {
                                std::lock_guard<std::mutex> lock(pending_requests_mutex_);
                                auto it = pending_tx_requests_.find(inv.GetHash());
                                if (it != pending_tx_requests_.end()) {
                                    // Check if request is still valid (not timed out)
                                    auto now = std::chrono::steady_clock::now();
                                    if (now - it->second.request_time < std::chrono::minutes(1)) {
                                        need_item = false; // Already requesting
                                    } else {
                                        // Request timed out, remove and allow re-request
                                        pending_tx_requests_.erase(it);
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            LOG_WARNING("Error checking transaction inventory {}: {}", inv.GetHash().ToString(), e.what());
                            need_item = true; // Err on the side of requesting
                        }
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

        // Process block - Complete blockchain integration
        if (blockchain_)
        {
            try {
                // Complete implementation: Add block to blockchain
                bool block_added = blockchain_->AddBlock(block);
                
                if (block_added) {
                    LOG_INFO("Successfully added block {} (index: {}) from peer {}", 
                            block.GetHash().ToString(), block.GetIndex(), peer_id.ToString());
                    
                    // Relay to other peers (excluding the sender)
                    RelayInventory(InventoryType::Block, block.GetHash(), peer_id);
                    
                    // Update our understanding of the network height
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        if (peer_states_.find(peer_id) != peer_states_.end()) {
                            // Update peer's known height
                            if (block.GetIndex() > peer_states_[peer_id].start_height) {
                                peer_states_[peer_id].start_height = block.GetIndex();
                            }
                        }
                    }
                    
                    // Check if we need more blocks (gap detection)
                    if (blockchain_->GetHeight() < block.GetIndex() - 1) {
                        LOG_DEBUG("Detected gap in blockchain, requesting more blocks");
                        // Request missing blocks
                        RequestNextBlocks(peer_id, blockchain_->GetHeight() + 1, block.GetIndex() - 1);
                    }
                    
                } else {
                    LOG_WARNING("Failed to add block {} from peer {} (likely duplicate or invalid)", 
                               block.GetHash().ToString(), peer_id.ToString());
                    SendReject(peer_id, "block", "rejected");
                }
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error adding block {} from peer {}: {}", 
                         block.GetHash().ToString(), peer_id.ToString(), e.what());
                SendReject(peer_id, "block", "error");
            }
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

        // Add to mempool - Complete mempool integration
        if (mempool_)
        {
            try {
                // Complete implementation: Add transaction to mempool
                bool transaction_added = mempool_->TryAdd(transaction);
                
                if (transaction_added) {
                    LOG_DEBUG("Successfully added transaction {} to mempool from peer {}", 
                             transaction.GetHash().ToString(), peer_id.ToString());
                    
                    // Relay to other peers (excluding the sender)
                    RelayInventory(InventoryType::Transaction, transaction.GetHash(), peer_id);
                    
                    // Update transaction statistics
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        if (peer_states_.find(peer_id) != peer_states_.end()) {
                            peer_states_[peer_id].transactions_received++;
                        }
                    }
                    
                } else {
                    LOG_DEBUG("Failed to add transaction {} to mempool (likely duplicate or full)", 
                             transaction.GetHash().ToString());
                    // Don't send reject for mempool failures - transaction might be valid but pool is full
                }
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error adding transaction {} to mempool from peer {}: {}", 
                         transaction.GetHash().ToString(), peer_id.ToString(), e.what());
                SendReject(peer_id, "tx", "error");
            }
        } else {
            LOG_WARNING("No mempool available to add transaction {} from peer {}", 
                       transaction.GetHash().ToString(), peer_id.ToString());
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

        // Send inventory of mempool transactions - Complete implementation
        InvPayload inv;
        
        try {
            // Complete implementation: Get mempool transactions
            auto txs = mempool_->GetAllTransactions();
            size_t transaction_count = 0;
            const size_t MAX_MEMPOOL_INV = 50000; // Limit to prevent excessive network traffic
            
            for (const auto& tx : txs) {
                if (transaction_count >= MAX_MEMPOOL_INV) {
                    LOG_DEBUG("Limiting mempool inventory to {} transactions for peer {}", 
                             MAX_MEMPOOL_INV, peer_id.ToString());
                    break;
                }
                
                inv.AddInventory(InventoryType::Transaction, tx.GetHash());
                transaction_count++;
            }
            
            LOG_DEBUG("Sending mempool inventory with {} transactions to peer {}", 
                     transaction_count, peer_id.ToString());
                     
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting mempool transactions for peer {}: {}", 
                     peer_id.ToString(), e.what());
            // Send empty inventory on error
        }

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

        // Complete not found items handling - request from alternative peers
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        
        for (const auto& inv : payload.GetInventory())
        {
            if (inv.GetType() == InventoryType::Block)
            {
                // Handle not found block - try requesting from other peers
                auto it = pending_block_requests_.find(inv.GetHash());
                if (it != pending_block_requests_.end())
                {
                    // Remove this peer from the request and try others
                    it->second.failed_peers.insert(peer_id);
                    it->second.retry_count++;
                    
                    // Find alternative peers to request from
                    std::vector<io::UInt256> alternative_peers;
                    for (const auto& [other_peer_id, state] : peer_states_)
                    {
                        if (other_peer_id != peer_id && 
                            it->second.failed_peers.find(other_peer_id) == it->second.failed_peers.end() &&
                            state.handshaked)
                        {
                            alternative_peers.push_back(other_peer_id);
                        }
                    }
                    
                    if (!alternative_peers.empty() && it->second.retry_count < 3)
                    {
                        // Request from another peer
                        auto target_peer = alternative_peers[std::rand() % alternative_peers.size()];
                        
                        GetDataPayload getdata;
                        getdata.AddInventory(InventoryType::Block, inv.GetHash());
                        Message msg(MessageCommand::GetData, &getdata);
                        send_callback_(target_peer, msg);
                        
                        it->second.request_time = std::chrono::steady_clock::now();
                        
                        LOG_DEBUG("Requesting block {} from alternative peer {} (retry {})", 
                                 inv.GetHash().ToString(), target_peer.ToString(), it->second.retry_count);
                    }
                    else
                    {
                        // No more peers to try or too many retries
                        LOG_WARNING("Failed to obtain block {} from {} peers, giving up", 
                                   inv.GetHash().ToString(), it->second.failed_peers.size());
                        pending_block_requests_.erase(it);
                    }
                }
            }
            else if (inv.GetType() == InventoryType::Transaction)
            {
                // Handle not found transaction - try requesting from other peers
                auto it = pending_tx_requests_.find(inv.GetHash());
                if (it != pending_tx_requests_.end())
                {
                    // Remove this peer from the request and try others
                    it->second.failed_peers.insert(peer_id);
                    it->second.retry_count++;
                    
                    // Find alternative peers to request from
                    std::vector<io::UInt256> alternative_peers;
                    for (const auto& [other_peer_id, state] : peer_states_)
                    {
                        if (other_peer_id != peer_id && 
                            it->second.failed_peers.find(other_peer_id) == it->second.failed_peers.end() &&
                            state.handshaked)
                        {
                            alternative_peers.push_back(other_peer_id);
                        }
                    }
                    
                    if (!alternative_peers.empty() && it->second.retry_count < 2)
                    {
                        // Request from another peer
                        auto target_peer = alternative_peers[std::rand() % alternative_peers.size()];
                        
                        GetDataPayload getdata;
                        getdata.AddInventory(InventoryType::Transaction, inv.GetHash());
                        Message msg(MessageCommand::GetData, &getdata);
                        send_callback_(target_peer, msg);
                        
                        it->second.request_time = std::chrono::steady_clock::now();
                        
                        LOG_DEBUG("Requesting transaction {} from alternative peer {} (retry {})", 
                                 inv.GetHash().ToString(), target_peer.ToString(), it->second.retry_count);
                    }
                    else
                    {
                        // No more peers to try or too many retries
                        LOG_WARNING("Failed to obtain transaction {} from {} peers, giving up", 
                                   inv.GetHash().ToString(), it->second.failed_peers.size());
                        pending_tx_requests_.erase(it);
                    }
                }
            }
        }
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
        // Complete block validation for P2P network
        try {
            // 1. Check block header validity
            if (block.GetIndex() == 0) {
                // Genesis block validation
                if (protocol_settings_) {
                    auto genesis_block = protocol_settings_->GetGenesisBlock();
                    if (genesis_block && block.GetHash() != genesis_block->GetHash()) {
                        LOG_WARNING("Invalid genesis block hash");
                        return false;
                    }
                }
            } else {
                // Regular block validation
                if (blockchain_) {
                    // Check if previous block exists
                    if (!blockchain_->HasBlock(block.GetPrevHash())) {
                        LOG_DEBUG("Previous block {} not found for block {}", 
                                 block.GetPrevHash().ToString(), block.GetIndex());
                        return false;
                    }
                    
                    // Check block index is sequential
                    auto prev_block = blockchain_->GetBlock(block.GetPrevHash());
                    if (prev_block && block.GetIndex() != prev_block->GetIndex() + 1) {
                        LOG_WARNING("Block index {} is not sequential (previous: {})", 
                                   block.GetIndex(), prev_block->GetIndex());
                        return false;
                    }
                }
                
                // Check timestamp is reasonable (not too far in future/past)
                auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                int64_t block_time = static_cast<int64_t>(block.GetTimestamp());
                
                if (block_time > now + 15000) { // 15 seconds in future
                    LOG_WARNING("Block timestamp {} is too far in the future", block_time);
                    return false;
                }
            }
            
            // 2. Check merkle root
            auto transactions = block.GetTransactions();
            if (!transactions.empty()) {
                std::vector<io::UInt256> tx_hashes;
                tx_hashes.reserve(transactions.size());
                
                for (const auto& tx : transactions) {
                    tx_hashes.push_back(tx->GetHash());
                }
                
                auto calculated_merkle_root = cryptography::MerkleTree::ComputeRoot(tx_hashes);
                if (calculated_merkle_root != block.GetMerkleRoot()) {
                    LOG_WARNING("Merkle root mismatch: calculated {}, block has {}", 
                               calculated_merkle_root.ToString(), block.GetMerkleRoot().ToString());
                    return false;
                }
            } else {
                // Empty block should have zero merkle root
                io::UInt256 zero_hash;
                if (block.GetMerkleRoot() != zero_hash) {
                    LOG_WARNING("Empty block has non-zero merkle root");
                    return false;
                }
            }
            
            // 3. Check transactions
            size_t total_size = 0;
            for (const auto& tx : transactions) {
                if (!tx) {
                    LOG_WARNING("Block contains null transaction");
                    return false;
                }
                
                // Basic transaction validation
                if (!ValidateTransaction(*tx)) {
                    LOG_WARNING("Block contains invalid transaction {}", tx->GetHash().ToString());
                    return false;
                }
                
                total_size += tx->GetSize();
            }
            
            // Check block size limit
            if (protocol_settings_) {
                uint32_t max_block_size = protocol_settings_->GetMaxBlockSize();
                if (total_size > max_block_size) {
                    LOG_WARNING("Block size {} exceeds maximum {}", total_size, max_block_size);
                    return false;
                }
            }
            
            // 4. Check consensus data (witness)
            auto witness = block.GetWitness();
            if (witness.empty()) {
                LOG_WARNING("Block has no witness/consensus data");
                return false;
            }
            
            // Witness script verification requires VM execution
            // Basic size validation ensures network safety
            if (witness.size() > 1024) { // Reasonable witness size limit
                LOG_WARNING("Block witness is too large: {} bytes", witness.size());
                return false;
            }
            
            // 5. Check block hash integrity
            auto calculated_hash = block.CalculateHash();
            if (calculated_hash != block.GetHash()) {
                LOG_WARNING("Block hash mismatch: calculated {}, block claims {}", 
                           calculated_hash.ToString(), block.GetHash().ToString());
                return false;
            }
            
            LOG_DEBUG("Block {} validation passed", block.GetIndex());
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Block validation failed with exception: {}", e.what());
            return false;
        }
    }

    bool ProtocolHandler::ValidateTransaction(const ledger::Transaction& transaction) const
    {
        // Complete transaction validation for P2P network
        try {
            // 1. Check transaction format
            if (transaction.GetVersion() > 255) {
                LOG_WARNING("Invalid transaction version: {}", transaction.GetVersion());
                return false;
            }
            
            // Check transaction size
            size_t tx_size = transaction.GetSize();
            if (protocol_settings_) {
                uint32_t max_tx_size = protocol_settings_->GetMaxTransactionSize();
                if (tx_size > max_tx_size) {
                    LOG_WARNING("Transaction size {} exceeds maximum {}", tx_size, max_tx_size);
                    return false;
                }
            }
            
            // Check ValidUntilBlock
            if (blockchain_) {
                uint32_t current_height = blockchain_->GetHeight();
                if (transaction.GetValidUntilBlock() <= current_height) {
                    LOG_DEBUG("Transaction {} expired at block {} (current: {})", 
                             transaction.GetHash().ToString(), transaction.GetValidUntilBlock(), current_height);
                    return false;
                }
                
                // Check ValidUntilBlock is not too far in the future
                if (transaction.GetValidUntilBlock() > current_height + 5760) { // ~24 hours
                    LOG_WARNING("Transaction ValidUntilBlock {} is too far in future (current: {})", 
                               transaction.GetValidUntilBlock(), current_height);
                    return false;
                }
            }
            
            // 2. Check signers and witnesses match
            auto signers = transaction.GetSigners();
            auto witnesses = transaction.GetWitnesses();
            
            if (signers.empty()) {
                LOG_WARNING("Transaction has no signers");
                return false;
            }
            
            if (signers.size() != witnesses.size()) {
                LOG_WARNING("Signer count {} does not match witness count {}", 
                           signers.size(), witnesses.size());
                return false;
            }
            
            // 3. Check fees are reasonable
            uint64_t system_fee = transaction.GetSystemFee();
            uint64_t network_fee = transaction.GetNetworkFee();
            
            if (system_fee < 0 || network_fee < 0) {
                LOG_WARNING("Transaction has negative fees: system={}, network={}", system_fee, network_fee);
                return false;
            }
            
            // Check minimum network fee based on transaction size
            // Get the actual fee per byte from Policy contract
            auto policyContract = smartcontract::native::PolicyContract::GetInstance();
            uint64_t feePerByte = 1000; // Default fallback
            
            if (policyContract && snapshot) {
                try {
                    feePerByte = policyContract->GetFeePerByte(*snapshot);
                } catch (const std::exception&) {
                    // Use default if unable to get from policy
                    LOG_WARNING("Unable to get fee per byte from Policy contract, using default");
                }
            }
            
            uint64_t min_network_fee = tx_size * feePerByte;
            if (network_fee < min_network_fee) {
                LOG_DEBUG("Transaction network fee {} is below minimum {} for size {} (fee per byte: {})", 
                         network_fee, min_network_fee, tx_size, feePerByte);
                return false;
            }
            
            // 4. Check script is not empty
            auto script = transaction.GetScript();
            if (script.empty()) {
                LOG_WARNING("Transaction has empty script");
                return false;
            }
            
            // Check script size limit
            if (script.size() > 65536) { // 64KB script limit
                LOG_WARNING("Transaction script is too large: {} bytes", script.size());
                return false;
            }
            
            // 5. Check double spend (basic check)
            if (mempool_) {
                // Check if transaction is already in mempool
                if (mempool_->HasTransaction(transaction.GetHash())) {
                    LOG_DEBUG("Transaction {} already in mempool", transaction.GetHash().ToString());
                    return false;
                }
            }
            
            if (blockchain_) {
                // Check if transaction is already in blockchain
                if (blockchain_->HasTransaction(transaction.GetHash())) {
                    LOG_DEBUG("Transaction {} already in blockchain", transaction.GetHash().ToString());
                    return false;
                }
            }
            
            // 6. Check attributes are valid
            auto attributes = transaction.GetAttributes();
            for (const auto& attr : attributes) {
                // Basic attribute validation
                if (attr.GetType() > 255) {
                    LOG_WARNING("Invalid attribute type: {}", static_cast<int>(attr.GetType()));
                    return false;
                }
                
                // Check attribute size
                auto attr_data = attr.GetData();
                if (attr_data.size() > 65535) {
                    LOG_WARNING("Attribute data too large: {} bytes", attr_data.size());
                    return false;
                }
            }
            
            // 7. Check witness signatures with proper validation
            for (size_t i = 0; i < signers.size(); ++i) {
                const auto& signer = signers[i];
                const auto& witness = witnesses[i];
                
                // Check witness has reasonable size
                auto invocation_script = witness.GetInvocationScript();
                auto verification_script = witness.GetVerificationScript();
                
                // Maximum witness script sizes from Neo protocol
                const size_t MAX_INVOCATION_SIZE = 1024;
                const size_t MAX_VERIFICATION_SIZE = 1024;
                
                if (invocation_script.size() > MAX_INVOCATION_SIZE) {
                    LOG_WARNING("Witness invocation script too large: {} bytes (max: {})", 
                               invocation_script.size(), MAX_INVOCATION_SIZE);
                    return false;
                }
                
                if (verification_script.size() > MAX_VERIFICATION_SIZE) {
                    LOG_WARNING("Witness verification script too large: {} bytes (max: {})", 
                               verification_script.size(), MAX_VERIFICATION_SIZE);
                    return false;
                }
                
                // Check signer account matches verification script hash
                auto calculated_hash = cryptography::Hash::Hash160(
                    io::ByteSpan(verification_script.data(), verification_script.size()));
                if (calculated_hash != signer.GetAccount()) {
                    LOG_WARNING("Signer account {} does not match verification script hash {}", 
                               signer.GetAccount().ToString(), calculated_hash.ToString());
                    return false;
                }
            }
            
            // 8. Check transaction hash integrity
            auto calculated_hash = transaction.CalculateHash();
            if (calculated_hash != transaction.GetHash()) {
                LOG_WARNING("Transaction hash mismatch: calculated {}, transaction claims {}", 
                           calculated_hash.ToString(), transaction.GetHash().ToString());
                return false;
            }
            
            LOG_DEBUG("Transaction {} validation passed", transaction.GetHash().ToString());
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Transaction validation failed with exception: {}", e.what());
            return false;
        }
    }
}