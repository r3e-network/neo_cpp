/**
 * @file network_synchronizer.cpp
 * @brief Minimal stub that keeps the LocalNode/Blockchain wiring alive until the full synchronizer lands.
 */

#include <neo/network/p2p/network_synchronizer.h>

#include <algorithm>

namespace neo::network::p2p
{
namespace
{
uint32_t TryGetHeight(const std::shared_ptr<ledger::Blockchain>& blockchain)
{
    return blockchain ? static_cast<uint32_t>(blockchain->GetHeight()) : 0U;
}
}

NetworkSynchronizer::NetworkSynchronizer(LocalNode& localNode, std::shared_ptr<ledger::Blockchain> blockchain)
    : localNode_(localNode),
      blockchain_(std::move(blockchain)),
      state_(SynchronizationState::NotSynchronizing),
      currentBlockIndex_(0),
      targetBlockIndex_(0),
      running_(false)
{
    // The full synchronizer wires LocalNode callbacks; we leave the hooks to the caller for now.
}

NetworkSynchronizer::~NetworkSynchronizer() { Stop(); }

void NetworkSynchronizer::Start()
{
    if (running_.exchange(true)) return;

    currentBlockIndex_ = TryGetHeight(blockchain_);
    targetBlockIndex_ = currentBlockIndex_.load();
    SetState(SynchronizationState::Synchronized);
}

void NetworkSynchronizer::Stop()
{
    if (!running_.exchange(false)) return;

    if (syncThread_.joinable()) syncThread_.join();
    SetState(SynchronizationState::NotSynchronizing);
}

SynchronizationState NetworkSynchronizer::GetState() const { return state_.load(); }

uint32_t NetworkSynchronizer::GetCurrentBlockIndex() const { return currentBlockIndex_.load(); }

uint32_t NetworkSynchronizer::GetTargetBlockIndex() const { return targetBlockIndex_.load(); }

void NetworkSynchronizer::SetBlockReceivedCallback(std::function<void(const std::shared_ptr<ledger::Block>&)> callback)
{
    blockReceivedCallback_ = std::move(callback);
}

void NetworkSynchronizer::SetTransactionReceivedCallback(
    std::function<void(const std::shared_ptr<ledger::Transaction>&)> callback)
{
    transactionReceivedCallback_ = std::move(callback);
}

void NetworkSynchronizer::SetStateChangedCallback(std::function<void(SynchronizationState)> callback)
{
    stateChangedCallback_ = std::move(callback);
}

void NetworkSynchronizer::OnInvMessageReceived(RemoteNode*, const payloads::InvPayload& payload)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& hash : payload.GetHashes())
    {
        knownHashes_.insert(hash);
    }
}

void NetworkSynchronizer::OnBlockMessageReceived(RemoteNode*, const std::shared_ptr<ledger::Block>& block)
{
    if (!block) return;

    currentBlockIndex_ = std::max<uint32_t>(currentBlockIndex_.load(), block->GetIndex());
    targetBlockIndex_ = std::max(targetBlockIndex_.load(), block->GetIndex());

    if (blockReceivedCallback_) blockReceivedCallback_(block);
}

void NetworkSynchronizer::OnTransactionMessageReceived(RemoteNode*,
                                                       const std::shared_ptr<ledger::Transaction>& transaction)
{
    if (transactionReceivedCallback_) transactionReceivedCallback_(transaction);
}

void NetworkSynchronizer::OnHeadersMessageReceived(RemoteNode*, const payloads::HeadersPayload& payload)
{
    uint32_t maxIndex = targetBlockIndex_.load();
    for (const auto& header : payload.GetHeaders())
    {
        maxIndex = std::max<uint32_t>(maxIndex, header->GetIndex());
    }
    targetBlockIndex_ = maxIndex;
}

void NetworkSynchronizer::RunSync() {}

void NetworkSynchronizer::SyncHeaders() {}

void NetworkSynchronizer::SyncBlocks() {}

void NetworkSynchronizer::ProcessPendingBlocks() {}

void NetworkSynchronizer::ProcessPendingTransactions() {}

void NetworkSynchronizer::RequestHeaders() {}

void NetworkSynchronizer::RequestBlocks() {}

void NetworkSynchronizer::SetState(SynchronizationState state)
{
    state_.store(state);
    if (stateChangedCallback_) stateChangedCallback_(state);
}
}  // namespace neo::network::p2p
