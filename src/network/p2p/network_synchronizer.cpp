#include <neo/cryptography/hash.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/network_synchronizer.h>
#include <neo/network/p2p/payloads/get_data_payload.h>

#include <algorithm>
#include <chrono>
#include <iostream>

namespace neo::network::p2p
{
NetworkSynchronizer::NetworkSynchronizer(LocalNode& localNode, std::shared_ptr<ledger::Blockchain> blockchain)
    : localNode_(localNode),
      blockchain_(blockchain),
      state_(SynchronizationState::NotSynchronizing),
      currentBlockIndex_(0),
      targetBlockIndex_(0),
      running_(false)
{
    // Register callbacks
    localNode_.SetInvMessageReceivedCallback([this](RemoteNode* remoteNode, const payloads::InvPayload& payload)
                                             { OnInvMessageReceived(remoteNode, payload); });

    localNode_.SetHeadersMessageReceivedCallback([this](RemoteNode* remoteNode, const payloads::HeadersPayload& payload)
                                                 { OnHeadersMessageReceived(remoteNode, payload); });
}

NetworkSynchronizer::~NetworkSynchronizer() { Stop(); }

void NetworkSynchronizer::Start()
{
    if (running_) return;

    running_ = true;
    syncThread_ = std::thread(&NetworkSynchronizer::RunSync, this);

    // Set initial state
    SetState(SynchronizationState::NotSynchronizing);

    // Get current block index
    currentBlockIndex_ = blockchain_->GetHeight();

    // Start synchronization
    if (localNode_.GetConnectedCount() > 0)
    {
        SetState(SynchronizationState::SynchronizingHeaders);
        RequestHeaders();
    }
}

void NetworkSynchronizer::Stop()
{
    if (!running_) return;

    running_ = false;

    if (syncThread_.joinable()) syncThread_.join();

    SetState(SynchronizationState::NotSynchronizing);
}

SynchronizationState NetworkSynchronizer::GetState() const { return state_; }

uint32_t NetworkSynchronizer::GetCurrentBlockIndex() const { return currentBlockIndex_; }

uint32_t NetworkSynchronizer::GetTargetBlockIndex() const { return targetBlockIndex_; }

void NetworkSynchronizer::SetBlockReceivedCallback(std::function<void(const std::shared_ptr<ledger::Block>&)> callback)
{
    blockReceivedCallback_ = callback;
}

void NetworkSynchronizer::SetTransactionReceivedCallback(
    std::function<void(const std::shared_ptr<ledger::Transaction>&)> callback)
{
    transactionReceivedCallback_ = callback;
}

void NetworkSynchronizer::SetStateChangedCallback(std::function<void(SynchronizationState)> callback)
{
    stateChangedCallback_ = callback;
}

void NetworkSynchronizer::OnInvMessageReceived(RemoteNode* remoteNode, const payloads::InvPayload& payload)
{
    // Process inventory
    std::vector<io::UInt256> unknownHashes;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& hash : payload.GetHashes())
        {
            // Skip known hashes
            if (knownHashes_.find(hash) != knownHashes_.end()) continue;

            // Add to known hashes
            knownHashes_.insert(hash);

            // Add to unknown hashes
            unknownHashes.push_back(hash);
        }
    }

    // Request unknown hashes
    if (!unknownHashes.empty())
    {
        auto getDataPayload = std::make_shared<payloads::GetDataPayload>(payload.GetType(), unknownHashes);
        Message message(MessageCommand::GetData, getDataPayload);
        remoteNode->Send(message);
    }
}

void NetworkSynchronizer::OnBlockMessageReceived(RemoteNode* remoteNode, const std::shared_ptr<ledger::Block>& block)
{
    // Add to pending blocks
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingBlocks_[block->GetHash()] = block;
    }

    // Process pending blocks
    ProcessPendingBlocks();

    // Notify block received
    if (blockReceivedCallback_) blockReceivedCallback_(block);
}

void NetworkSynchronizer::OnTransactionMessageReceived(RemoteNode* remoteNode,
                                                       const std::shared_ptr<ledger::Transaction>& transaction)
{
    // Add to pending transactions
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingTransactions_[transaction->GetHash()] = transaction;
    }

    // Process pending transactions
    ProcessPendingTransactions();

    // Notify transaction received
    if (transactionReceivedCallback_) transactionReceivedCallback_(transaction);
}

void NetworkSynchronizer::OnHeadersMessageReceived(RemoteNode* remoteNode, const payloads::HeadersPayload& payload)
{
    // Process headers
    const auto& headers = payload.GetHeaders();

    if (headers.empty())
    {
        // No more headers, start syncing blocks
        SetState(SynchronizationState::SynchronizingBlocks);
        RequestBlocks();
        return;
    }

    // Add headers to blockchain
    for (const auto& header : headers)
    {
        if (!blockchain_->AddHeader(header))
        {
            std::cerr << "Failed to add header: " << header->GetHash().ToString() << std::endl;
            continue;
        }

        // Update target block index
        targetBlockIndex_ = std::max(targetBlockIndex_, header->GetIndex());
    }

    // Request more headers
    RequestHeaders();
}

void NetworkSynchronizer::RunSync()
{
    while (running_)
    {
        // Check if we need to sync
        if (state_ == SynchronizationState::NotSynchronizing && localNode_.GetConnectedCount() > 0)
        {
            SetState(SynchronizationState::SynchronizingHeaders);
            RequestHeaders();
        }

        // Sleep for a while
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void NetworkSynchronizer::SyncHeaders()
{
    // Request headers
    RequestHeaders();
}

void NetworkSynchronizer::SyncBlocks()
{
    // Request blocks
    RequestBlocks();
}

void NetworkSynchronizer::ProcessPendingBlocks()
{
    std::vector<std::shared_ptr<ledger::Block>> blocksToProcess;

    // Get blocks to process
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = pendingBlocks_.begin(); it != pendingBlocks_.end();)
        {
            blocksToProcess.push_back(it->second);
            it = pendingBlocks_.erase(it);
        }
    }

    // Sort blocks by index
    std::sort(blocksToProcess.begin(), blocksToProcess.end(),
              [](const auto& a, const auto& b) { return a->GetIndex() < b->GetIndex(); });

    // Process blocks
    for (const auto& block : blocksToProcess)
    {
        if (!blockchain_->AddBlock(block))
        {
            std::cerr << "Failed to add block: " << block->GetHash().ToString() << std::endl;
            continue;
        }

        // Update current block index
        currentBlockIndex_ = block->GetIndex();

        // Check if we're synchronized
        if (currentBlockIndex_ >= targetBlockIndex_)
        {
            SetState(SynchronizationState::Synchronized);
        }
    }
}

void NetworkSynchronizer::ProcessPendingTransactions()
{
    std::vector<std::shared_ptr<ledger::Transaction>> transactionsToProcess;

    // Get transactions to process
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = pendingTransactions_.begin(); it != pendingTransactions_.end();)
        {
            transactionsToProcess.push_back(it->second);
            it = pendingTransactions_.erase(it);
        }
    }

    // Process transactions
    for (const auto& transaction : transactionsToProcess)
    {
        if (!blockchain_->AddTransaction(transaction))
        {
            std::cerr << "Failed to add transaction: " << transaction->GetHash().ToString() << std::endl;
            continue;
        }
    }
}

void NetworkSynchronizer::RequestHeaders()
{
    // Get connected nodes
    auto connectedNodes = localNode_.GetConnectedNodes();

    if (connectedNodes.empty()) return;

    // Get a random node
    auto randomNode = connectedNodes[rand() % connectedNodes.size()];

    // Create get headers payload
    auto getHeadersPayload = std::make_shared<payloads::GetBlocksPayload>(
        std::vector<io::UInt256>{blockchain_->GetCurrentHeaderHash()}, io::UInt256());

    // Send get headers message
    Message message(MessageCommand::GetHeaders, getHeadersPayload);
    randomNode->Send(message);
}

void NetworkSynchronizer::RequestBlocks()
{
    // Get connected nodes
    auto connectedNodes = localNode_.GetConnectedNodes();

    if (connectedNodes.empty()) return;

    // Get a random node
    auto randomNode = connectedNodes[rand() % connectedNodes.size()];

    // Create get blocks payload
    auto getBlocksPayload = std::make_shared<payloads::GetBlockByIndexPayload>(
        currentBlockIndex_ + 1, std::min(currentBlockIndex_ + 500, targetBlockIndex_));

    // Send get blocks message
    Message message(MessageCommand::GetBlockByIndex, getBlocksPayload);
    randomNode->Send(message);
}

void NetworkSynchronizer::SetState(SynchronizationState state)
{
    state_ = state;

    // Notify state changed
    if (stateChangedCallback_) stateChangedCallback_(state);
}
}  // namespace neo::network::p2p
