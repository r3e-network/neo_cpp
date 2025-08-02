#include <neo/network/p2p/block_sync_manager.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/inventory_vector.h>
#include <neo/network/p2p/message.h>
#include <neo/core/neo_system.h>
#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>
#include <algorithm>

namespace neo::network::p2p
{

BlockSyncManager::BlockSyncManager(std::shared_ptr<NeoSystem> system, LocalNode& localNode)
    : system_(system)
    , localNode_(localNode)
{
    syncStartTime_ = std::chrono::steady_clock::now();
}

BlockSyncManager::~BlockSyncManager()
{
    Stop();
}

void BlockSyncManager::Start()
{
    if (running_)
    {
        return;
    }

    LOG_INFO("Starting block synchronization manager");
    
    running_ = true;
    processingRunning_ = true;
    syncState_ = SyncState::SyncingHeaders;
    
    // Start processing threads for parallel block processing
    processingThreads_.reserve(PROCESSING_THREADS);
    for (size_t i = 0; i < PROCESSING_THREADS; ++i)
    {
        processingThreads_.emplace_back([this]() {
            ProcessingThreadWorker();
        });
    }
    
    // Start sync thread
    syncThread_ = std::thread([this]() {
        SyncLoop();
    });
    
    // Set up callbacks with LocalNode
    localNode_.SetHeadersMessageReceivedCallback(
        [this](RemoteNode* node, const payloads::HeadersPayload& payload) {
            OnHeadersReceived(node, payload.GetHeaders());
        });
    
    localNode_.SetRemoteNodeHandshakedCallback(
        [this](RemoteNode* node) {
            OnPeerConnected(node);
        });
    
    localNode_.SetRemoteNodeDisconnectedCallback(
        [this](RemoteNode* node) {
            OnPeerDisconnected(node);
        });
    
    LOG_INFO("Block synchronization manager started");
}

void BlockSyncManager::Stop()
{
    if (!running_)
    {
        return;
    }

    LOG_INFO("Stopping block synchronization manager");
    
    running_ = false;
    processingRunning_ = false;
    syncCv_.notify_all();
    batchCv_.notify_all();
    
    if (syncThread_.joinable())
    {
        syncThread_.join();
    }
    
    // Stop all processing threads
    for (auto& thread : processingThreads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    processingThreads_.clear();
    
    syncState_ = SyncState::Idle;
    
    LOG_INFO("Block synchronization manager stopped");
}

BlockSyncManager::SyncState BlockSyncManager::GetSyncState() const
{
    return syncState_.load();
}

uint8_t BlockSyncManager::GetSyncProgress() const
{
    uint32_t current = currentHeight_.load();
    uint32_t target = targetHeight_.load();
    
    if (target == 0 || current >= target)
    {
        return 100;
    }
    
    return static_cast<uint8_t>((current * 100) / target);
}

void BlockSyncManager::OnHeadersReceived(RemoteNode* node, const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers)
{
    if (headers.empty())
    {
        return;
    }
    
    LOG_DEBUG("Received " + std::to_string(headers.size()) + " headers from peer");
    
    {
        std::lock_guard<std::mutex> lock(headerMutex_);
        pendingHeaders_.insert(pendingHeaders_.end(), headers.begin(), headers.end());
    }
    
    // Update peer info with latest header
    UpdatePeerInfo(node, headers.back()->GetIndex());
    
    // Wake up sync thread to process headers
    syncCv_.notify_one();
}

void BlockSyncManager::OnBlockReceived(RemoteNode* node, std::shared_ptr<ledger::Block> block)
{
    if (!block)
    {
        return;
    }
    
    io::UInt256 hash = block->GetHash();
    
    LOG_DEBUG("Received block " + std::to_string(block->GetIndex()) + " from peer");
    
    // Mark block as received
    MarkBlockReceived(hash);
    
    bool shouldProcess = false;
    std::vector<std::shared_ptr<ledger::Block>> batchToProcess;
    
    {
        std::lock_guard<std::mutex> lock(pendingBlocksMutex_);
        
        // Add to pending blocks
        pendingBlocks_.push_back(block);
        
        // Check if we should process the batch
        if (pendingBlocks_.size() >= BATCH_COLLECTION_SIZE)
        {
            // Move blocks to local batch for processing
            batchToProcess = std::move(pendingBlocks_);
            pendingBlocks_.clear();
            pendingBlocks_.reserve(BATCH_COLLECTION_SIZE);
            shouldProcess = true;
        }
    }
    
    if (shouldProcess)
    {
        // Sort blocks by height for sequential processing
        std::sort(batchToProcess.begin(), batchToProcess.end(),
                  [](const auto& a, const auto& b) {
                      return a->GetIndex() < b->GetIndex();
                  });
        
        // Enqueue batch for parallel processing
        EnqueueBlockBatch(std::move(batchToProcess));
        
        LOG_INFO("Enqueued batch of " + std::to_string(BATCH_COLLECTION_SIZE) + " blocks for processing");
    }
    
    // Handle orphan blocks
    if (block->GetIndex() > currentHeight_ + BATCH_COLLECTION_SIZE * 2)
    {
        // Block is too far ahead, save as orphan
        std::lock_guard<std::mutex> lock(blockMutex_);
        if (orphanBlocks_.size() < maxOrphanBlocks_)
        {
            orphanBlocks_[hash] = block;
            LOG_DEBUG("Stored orphan block " + std::to_string(block->GetIndex()));
        }
    }
    
    // Process any orphan blocks that might now be valid
    ProcessOrphanBlocks();
}

void BlockSyncManager::OnBlockInventory(RemoteNode* node, const std::vector<io::UInt256>& hashes)
{
    if (hashes.empty())
    {
        return;
    }
    
    LOG_DEBUG("Received inventory with " + std::to_string(hashes.size()) + " block hashes");
    
    // TODO: Implement blockchain->ContainsBlock() check when blockchain interface is complete
    // For now, assume all blocks are needed
    std::lock_guard<std::mutex> lock(blockMutex_);
    for (const auto& hash : hashes)
    {
        if (!IsBlockRequested(hash))
        {
            blockDownloadQueue_.push(hash);
        }
    }
    
    // Wake up sync thread to download blocks
    syncCv_.notify_one();
}

void BlockSyncManager::OnPeerConnected(RemoteNode* node)
{
    LOG_INFO("Peer connected for block sync");
    
    // Add peer to tracking
    {
        std::lock_guard<std::mutex> lock(peerMutex_);
        peers_[node] = PeerInfo{
            node->GetLastBlockIndex(),
            std::chrono::steady_clock::now(),
            0,
            false
        };
    }
    
    // Update target height
    uint32_t peerHeight = node->GetLastBlockIndex();
    uint32_t currentTarget = targetHeight_.load();
    if (peerHeight > currentTarget)
    {
        targetHeight_ = peerHeight;
    }
    
    // Start syncing if we're behind
    if (GetLocalHeight() < peerHeight)
    {
        syncState_ = SyncState::SyncingHeaders;
        syncCv_.notify_one();
    }
}

void BlockSyncManager::OnPeerDisconnected(RemoteNode* node)
{
    LOG_INFO("Peer disconnected from block sync");
    
    // Remove peer from tracking
    std::lock_guard<std::mutex> lock(peerMutex_);
    peers_.erase(node);
}

void BlockSyncManager::SetMaxConcurrentDownloads(uint32_t maxBlocks)
{
    maxConcurrentDownloads_ = maxBlocks;
}

BlockSyncManager::SyncStats BlockSyncManager::GetStats() const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - syncStartTime_).count();
    
    SyncStats stats;
    stats.currentHeight = currentHeight_.load();
    stats.targetHeight = targetHeight_.load();
    stats.headerHeight = headerHeight_.load();
    stats.downloadedBlocks = downloadedBlocks_.load();
    
    {
        std::lock_guard<std::mutex> lock(blockMutex_);
        stats.pendingBlocks = blockDownloadQueue_.size();
        stats.orphanBlocks = orphanBlocks_.size();
    }
    
    stats.startTime = syncStartTime_;
    stats.blocksPerSecond = elapsed > 0 ? static_cast<double>(stats.downloadedBlocks) / elapsed : 0;
    
    return stats;
}

void BlockSyncManager::SyncLoop()
{
    LOG_INFO("Block sync loop started");
    
    while (running_)
    {
        std::unique_lock<std::mutex> lock(syncMutex_);
        
        // High-performance: check for work every 10ms instead of 1 second
        syncCv_.wait_for(lock, std::chrono::milliseconds(10), [this] {
            return !running_ || 
                   syncState_ == SyncState::SyncingHeaders ||
                   syncState_ == SyncState::SyncingBlocks ||
                   !blockDownloadQueue_.empty();
        });
        
        if (!running_)
        {
            break;
        }
        
        // Process based on current state
        switch (syncState_.load())
        {
            case SyncState::SyncingHeaders:
                RequestHeaders();
                ProcessPendingHeaders();
                break;
                
            case SyncState::SyncingBlocks:
                RequestBlocks();
                ProcessOrphanBlocks();
                break;
                
            case SyncState::Synced:
                // Still process any new blocks that arrive
                RequestBlocks();
                break;
                
            case SyncState::Idle:
                break;
        }
        
        // Timeout old requests
        TimeoutRequests();
        
        // Flush pending blocks if they've been waiting too long
        {
            std::lock_guard<std::mutex> lock(pendingBlocksMutex_);
            if (!pendingBlocks_.empty())
            {
                // Sort and process pending blocks
                std::sort(pendingBlocks_.begin(), pendingBlocks_.end(),
                          [](const auto& a, const auto& b) {
                              return a->GetIndex() < b->GetIndex();
                          });
                
                EnqueueBlockBatch(std::move(pendingBlocks_));
                pendingBlocks_.clear();
                pendingBlocks_.reserve(BATCH_COLLECTION_SIZE);
            }
        }
    }
    
    LOG_INFO("Block sync loop stopped");
}

void BlockSyncManager::RequestHeaders()
{
    auto peer = SelectBestPeer();
    if (!peer)
    {
        return;
    }
    
    uint32_t localHeight = GetLocalHeight();
    uint32_t targetHeight = targetHeight_.load();
    
    if (localHeight >= targetHeight)
    {
        // We have all headers
        syncState_ = SyncState::SyncingBlocks;
        return;
    }
    
    // Create get headers request
    // TODO: Implement blockchain->GetBlockHash() when blockchain interface is complete
    // For now, use zero hash to request from genesis
    io::UInt256 startHash; // Zero hash
    
    // Request headers
    auto payload = std::make_shared<payloads::GetBlocksPayload>();
    payload->SetHashStart(startHash);
    payload->SetCount(-1); // Request maximum headers
    
    Message message(MessageCommand::GetHeaders, payload);
    peer->Send(message);
    
    LOG_DEBUG("Requested headers starting from height " + std::to_string(localHeight));
}

void BlockSyncManager::RequestBlocks()
{
    std::lock_guard<std::mutex> lock(blockMutex_);
    
    // Check if we have capacity for more downloads
    if (requestedBlocks_.size() >= maxConcurrentDownloads_)
    {
        return;
    }
    
    // Prefetch: ensure download queue has enough blocks
    if (blockDownloadQueue_.size() < maxConcurrentDownloads_ * 2)
    {
        // Add more blocks to download queue based on current height
        uint32_t currentH = currentHeight_.load();
        uint32_t targetH = targetHeight_.load();
        
        for (uint32_t h = currentH + 1; h <= targetH && blockDownloadQueue_.size() < maxConcurrentDownloads_ * 2; ++h)
        {
            // TODO: Get block hash for height when blockchain interface is complete
            // For now, this prefetching logic is prepared for future use
        }
    }
    
    // Get blocks to download in batches for optimal network utilization
    std::vector<io::UInt256> toRequest;
    const size_t BATCH_SIZE = 500;  // Request 500 blocks per message for efficiency
    while (!blockDownloadQueue_.empty() && 
           requestedBlocks_.size() + toRequest.size() < maxConcurrentDownloads_ &&
           toRequest.size() < BATCH_SIZE)
    {
        io::UInt256 hash = blockDownloadQueue_.front();
        blockDownloadQueue_.pop();
        
        if (!IsBlockRequested(hash))
        {
            toRequest.push_back(hash);
            MarkBlockRequested(hash);
        }
    }
    
    if (toRequest.empty())
    {
        return;
    }
    
    // Select peer to request from
    auto peer = SelectBestPeer();
    if (!peer)
    {
        // Put blocks back in queue
        for (const auto& hash : toRequest)
        {
            blockDownloadQueue_.push(hash);
            requestedBlocks_.erase(hash);
        }
        return;
    }
    
    // Send getdata request
    try
    {
        // Create inventory vectors for the blocks we want
        std::vector<InventoryVector> inventories;
        for (const auto& hash : toRequest)
        {
            inventories.emplace_back(InventoryType::Block, hash);
        }
        
        auto payload = std::make_shared<payloads::GetDataPayload>(inventories);
        Message message(MessageCommand::GetData, payload);
        peer->Send(message);
        
        LOG_INFO("Requested " + std::to_string(toRequest.size()) + " blocks from peer");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to send GetData request: " + std::string(e.what()));
        // Put blocks back in queue
        for (const auto& hash : toRequest)
        {
            blockDownloadQueue_.push(hash);
            requestedBlocks_.erase(hash);
        }
    }
}

void BlockSyncManager::ProcessPendingHeaders()
{
    std::vector<std::shared_ptr<ledger::BlockHeader>> headers;
    {
        std::lock_guard<std::mutex> lock(headerMutex_);
        headers = std::move(pendingHeaders_);
        pendingHeaders_.clear();
    }
    
    if (headers.empty())
    {
        return;
    }
    
    // TODO: Implement blockchain->OnNewHeaders() when blockchain interface is complete
    LOG_DEBUG("Processing " + std::to_string(headers.size()) + " headers (blockchain integration pending)");
    
    // Update header height
    if (!headers.empty())
    {
        headerHeight_ = headers.back()->GetIndex();
    }
    
    // Add blocks to download queue
    {
        std::lock_guard<std::mutex> lock(blockMutex_);
        for (const auto& header : headers)
        {
            io::UInt256 hash = header->GetHash();
            // TODO: Add blockchain->ContainsBlock() check when available
            blockDownloadQueue_.push(hash);
        }
    }
    
    // Switch to block syncing if we have headers
    if (syncState_ == SyncState::SyncingHeaders && !blockDownloadQueue_.empty())
    {
        syncState_ = SyncState::SyncingBlocks;
    }
}

void BlockSyncManager::ProcessOrphanBlocks()
{
    // TODO: Implement proper orphan block processing when blockchain interface is complete
    std::lock_guard<std::mutex> lock(blockMutex_);
    
    if (!orphanBlocks_.empty())
    {
        LOG_DEBUG("Processing " + std::to_string(orphanBlocks_.size()) + " orphan blocks (blockchain integration pending)");
        
        // For now, just clear orphan blocks periodically to prevent memory growth
        if (orphanBlocks_.size() > maxOrphanBlocks_ / 2)
        {
            auto it = orphanBlocks_.begin();
            std::advance(it, orphanBlocks_.size() / 4);
            orphanBlocks_.erase(orphanBlocks_.begin(), it);
            LOG_DEBUG("Cleared old orphan blocks to manage memory");
        }
    }
}

RemoteNode* BlockSyncManager::SelectBestPeer()
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    
    RemoteNode* bestPeer = nullptr;
    uint32_t bestScore = 0;
    
    for (auto& [node, info] : peers_)
    {
        if (!node->IsConnected() || info.syncing)
        {
            continue;
        }
        
        // Score based on height and speed
        uint32_t score = info.lastBlockIndex * 1000 + info.downloadSpeed;
        if (score > bestScore)
        {
            bestScore = score;
            bestPeer = node;
        }
    }
    
    if (bestPeer)
    {
        peers_[bestPeer].syncing = true;
    }
    
    return bestPeer;
}

void BlockSyncManager::TimeoutRequests()
{
    // TODO: Implement request timeout logic
    // For now, we'll rely on peer disconnection to clean up
}

bool BlockSyncManager::IsBlockRequested(const io::UInt256& hash) const
{
    return requestedBlocks_.find(hash) != requestedBlocks_.end();
}

void BlockSyncManager::MarkBlockRequested(const io::UInt256& hash)
{
    requestedBlocks_.insert(hash);
}

void BlockSyncManager::MarkBlockReceived(const io::UInt256& hash)
{
    requestedBlocks_.erase(hash);
}

void BlockSyncManager::UpdatePeerInfo(RemoteNode* node, uint32_t lastBlockIndex)
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    
    auto it = peers_.find(node);
    if (it != peers_.end())
    {
        it->second.lastBlockIndex = lastBlockIndex;
        it->second.lastUpdate = std::chrono::steady_clock::now();
        it->second.syncing = false;
    }
}

uint32_t BlockSyncManager::GetLocalHeight() const
{
    return system_->GetCurrentBlockHeight();
}

void BlockSyncManager::UpdateSyncState()
{
    uint32_t current = currentHeight_.load();
    uint32_t target = targetHeight_.load();
    
    if (current >= target)
    {
        if (syncState_ != SyncState::Synced)
        {
            syncState_ = SyncState::Synced;
            LOG_INFO("Blockchain synchronization complete at height " + std::to_string(current));
        }
    }
}

void BlockSyncManager::ProcessingThreadWorker()
{
    LOG_INFO("Block processing thread started");
    
    while (processingRunning_)
    {
        std::vector<std::shared_ptr<ledger::Block>> batch;
        
        {
            std::unique_lock<std::mutex> lock(batchMutex_);
            batchCv_.wait(lock, [this] {
                return !processingRunning_ || !blockBatches_.empty();
            });
            
            if (!processingRunning_)
            {
                break;
            }
            
            if (!blockBatches_.empty())
            {
                batch = std::move(blockBatches_.front());
                blockBatches_.pop();
            }
        }
        
        if (!batch.empty())
        {
            try
            {
                // Process batch using high-performance batch processing
                size_t processed = system_->ProcessBlocksBatch(batch);
                
                if (processed > 0)
                {
                    downloadedBlocks_ += processed;
                    
                    // Update current height
                    uint32_t maxHeight = 0;
                    for (const auto& block : batch)
                    {
                        if (block && block->GetIndex() > maxHeight)
                        {
                            maxHeight = block->GetIndex();
                        }
                    }
                    currentHeight_ = maxHeight;
                    
                    UpdateSyncState();
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Batch processing failed: " + std::string(e.what()));
            }
        }
    }
    
    LOG_INFO("Block processing thread stopped");
}

void BlockSyncManager::EnqueueBlockBatch(std::vector<std::shared_ptr<ledger::Block>>&& batch)
{
    if (batch.empty())
    {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(batchMutex_);
        blockBatches_.push(std::move(batch));
    }
    
    batchCv_.notify_one();
}

} // namespace neo::network::p2p