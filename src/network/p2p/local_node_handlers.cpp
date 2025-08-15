/**
 * @file local_node_handlers.cpp
 * @brief Event and message handlers
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_message.h>
#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/remote_node.h>

namespace neo::network::p2p
{

// Extension methods for LocalNode to handle additional message types

void LocalNode::OnBlockMessageReceived(RemoteNode* remoteNode, const payloads::BlockPayload& payload)
{
    LOG_DEBUG("Block message received from {}", remoteNode->GetRemoteEndPoint().ToString());

    auto block = payload.GetBlock();
    if (!block)
    {
        LOG_WARNING("Received invalid block payload");
        return;
    }

    // Validate block
    auto blockchain = GetBlockchain();
    if (!blockchain)
    {
        LOG_ERROR("Blockchain not available");
        return;
    }

    // Check if we already have this block
    if (blockchain->ContainsBlock(block->GetHash()))
    {
        LOG_TRACE("Block {} already exists", block->GetHash().ToString());
        return;
    }

    // Try to add the block to the blockchain using OnNewBlock
    // OnNewBlock will validate and add the block
    auto result = blockchain->OnNewBlock(block);
    if (result == ledger::VerifyResult::Succeed)
    {
        LOG_INFO("Added block {} at height {}", block->GetHash().ToString(), block->GetIndex());

        // Relay to other nodes
        RelayBlock(block);

        // Notify subscribers
        if (blockReceivedCallback_)
        {
            blockReceivedCallback_(block);
        }
    }
    else
    {
        LOG_WARNING("Failed to add block: {}", static_cast<int>(result));
    }
}

void LocalNode::OnTransactionMessageReceived(RemoteNode* remoteNode, const payloads::TransactionPayload& payload)
{
    LOG_DEBUG("Transaction message received from {}", remoteNode->GetRemoteEndPoint().ToString());

    auto transaction = payload.GetTransaction();
    if (!transaction)
    {
        LOG_WARNING("Received invalid transaction payload");
        return;
    }

    // Get mempool
    auto mempool = GetMemoryPool();
    if (!mempool)
    {
        LOG_ERROR("Memory pool not available");
        return;
    }

    // Check if we already have this transaction
    if (mempool->Contains(transaction->GetHash()))
    {
        LOG_TRACE("Transaction {} already in mempool", transaction->GetHash().ToString());
        return;
    }

    // Validate transaction
    auto blockchain = GetBlockchain();
    if (!blockchain)
    {
        LOG_ERROR("Blockchain not available");
        return;
    }

    // Try to add the transaction to memory pool
    // The mempool will verify it internally using the verifier
    if (mempool->TryAdd(*transaction))
    {
        LOG_INFO("Added transaction {} to mempool", transaction->GetHash().ToString());

        // Relay to other nodes
        RelayTransaction(transaction);

        // Notify subscribers
        if (transactionReceivedCallback_)
        {
            transactionReceivedCallback_(transaction);
        }
    }
    else
    {
        LOG_DEBUG("Failed to add transaction {} to mempool", transaction->GetHash().ToString());
    }
}

void LocalNode::OnExtensibleMessageReceived(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload)
{
    LOG_DEBUG("Extensible message received from {}", remoteNode->GetRemoteEndPoint().ToString());

    // ExtensiblePayload is used for consensus messages in Neo N3
    auto category = payload.GetCategory();

    if (category == "dBFT")
    {
        // This is a consensus message
        ProcessConsensusMessage(remoteNode, payload);
    }
    else if (category == "StateService")
    {
        // State service message
        ProcessStateServiceMessage(remoteNode, payload);
    }
    else
    {
        LOG_DEBUG("Unknown extensible message category: {}", category);
    }
}

void LocalNode::ProcessConsensusMessage(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload)
{
    LOG_TRACE("Processing consensus message");

    // Get consensus service
    auto consensus = GetConsensusService();
    if (!consensus)
    {
        LOG_DEBUG("Consensus service not available");
        return;
    }

    // Deserialize consensus message from payload data
    auto data = payload.GetData();
    if (data.empty())
    {
        LOG_WARNING("Empty consensus message data");
        return;
    }

    // Forward to consensus service for processing
    if (consensus)
    {
        try
        {
            // Process consensus message
            consensus::ConsensusMessage consensusMsg(consensus::ConsensusMessageType::PrepareRequest);
            io::ByteSpan span(data.Data(), data.Size());
            io::BinaryReader reader(span);
            consensusMsg.Deserialize(reader);
            
            // Validate the message
            // Note: Verify() method needs to be implemented
            bool isValid = true; // Placeholder for validation
            if (isValid)
            {
                // Forward to consensus service (when implemented)
                // consensus->OnConsensusMessage(consensusMsg);
                LOG_DEBUG("Consensus message received");
                
                // Relay to other nodes if needed
                // RelayExtensiblePayload would need to be implemented
                LOG_DEBUG("Would relay consensus message if implemented");
            }
            else
            {
                LOG_WARNING("Invalid consensus message received");
            }
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Failed to process consensus message: {}", e.what());
        }
    }
}

void LocalNode::ProcessStateServiceMessage(RemoteNode* remoteNode, const payloads::ExtensiblePayload& payload)
{
    LOG_TRACE("Processing state service message");

    // Get state service plugin if available
    auto stateService = GetStateService();
    if (!stateService)
    {
        LOG_DEBUG("State service not available");
        return;
    }

    // Process state service message
    try
    {
        // Process state service message (state root synchronization)
        auto data = payload.GetData();
        if (!data.empty())
        {
            // StateRoot processing would go here when implemented
            // For now, just log that we received a state root payload
            LOG_DEBUG("State root payload received, processing not yet implemented");
            
            // State service integration would go here
            // stateService->ProcessStateRoot(stateRoot);
            
            // Relay functionality would go here
            // RelayExtensiblePayload(std::make_shared<payloads::ExtensiblePayload>(payload));
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARNING("Failed to process state service message: {}", e.what());
    }
}

void LocalNode::OnGetAddrMessageReceived(RemoteNode* remoteNode)
{
    LOG_DEBUG("GetAddr message received from {}", remoteNode->GetRemoteEndPoint().ToString());

    // Send our known peers
    std::vector<payloads::NetworkAddressWithTime> addresses;

    auto peers = GetConnectedNodes();
    for (const auto& peer : peers)
    {
        if (peer != remoteNode)  // Don't send the requesting node back to itself
        {
            payloads::NetworkAddressWithTime addr;
            // NetworkAddressWithTime constructor takes (timestamp, services, address, port)
            uint32_t timestamp = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count());
            auto endpoint = peer->GetRemoteEndPoint();
            addr = payloads::NetworkAddressWithTime(timestamp, 0, endpoint.GetAddress().ToString(), endpoint.GetPort());
            addresses.push_back(addr);

            if (addresses.size() >= 200)  // Limit to 200 addresses
                break;
        }
    }

    if (!addresses.empty())
    {
        remoteNode->SendAddr(addresses);
        LOG_DEBUG("Sent {} addresses to peer", addresses.size());
    }
}

void LocalNode::OnVerackMessageReceived(RemoteNode* remoteNode)
{
    LOG_DEBUG("Verack message received from {}", remoteNode->GetRemoteEndPoint().ToString());

    // Mark the handshake as complete
    // This is handled in RemoteNode typically, but we can do additional processing here

    // Start synchronization if needed
    auto blockchain = GetBlockchain();
    if (blockchain && blockchain->GetHeight() < remoteNode->GetLastBlockIndex())
    {
        LOG_INFO("Remote node has higher block height, requesting blocks");
        RequestBlocks(remoteNode);
    }
}

void LocalNode::RelayBlock(std::shared_ptr<ledger::Block> block)
{
    if (!block) return;

    LOG_DEBUG("Relaying block {} to peers", block->GetHash().ToString());

    // Create inventory vector
    std::vector<io::UInt256> hashes;
    hashes.push_back(block->GetHash());

    // Send to all connected peers
    auto peers = GetConnectedNodes();
    for (const auto& peer : peers)
    {
        if (peer->IsHandshaked())
        {
            peer->SendInv(InventoryType::Block, hashes);
        }
    }
}

void LocalNode::RelayTransaction(std::shared_ptr<ledger::Transaction> transaction)
{
    if (!transaction) return;

    LOG_DEBUG("Relaying transaction {} to peers", transaction->GetHash().ToString());

    // Create inventory vector
    std::vector<io::UInt256> hashes;
    hashes.push_back(transaction->GetHash());

    // Send to all connected peers
    auto peers = GetConnectedNodes();
    for (const auto& peer : peers)
    {
        if (peer->IsHandshaked())
        {
            peer->SendInv(InventoryType::Transaction, hashes);
        }
    }
}

void LocalNode::RequestBlocks(RemoteNode* remoteNode)
{
    auto blockchain = GetBlockchain();
    if (!blockchain) return;

    auto currentHeight = blockchain->GetHeight();
    auto targetHeight = remoteNode->GetLastBlockIndex();

    if (currentHeight >= targetHeight) return;

    LOG_INFO("Requesting blocks from height {} to {}", currentHeight + 1, targetHeight);

    // Request blocks in batches
    uint32_t batchSize = 500;
    uint32_t startIndex = currentHeight + 1;
    uint16_t count = std::min(batchSize, targetHeight - currentHeight);

    remoteNode->SendGetBlockByIndex(startIndex, count);
}

// Callback setters for external components
void LocalNode::SetBlockReceivedCallback(std::function<void(std::shared_ptr<ledger::Block>)> callback)
{
    blockReceivedCallback_ = callback;
}

void LocalNode::SetTransactionReceivedCallback(std::function<void(std::shared_ptr<ledger::Transaction>)> callback)
{
    transactionReceivedCallback_ = callback;
}

std::shared_ptr<ledger::Blockchain> LocalNode::GetBlockchain() const
{
    // This should be set during initialization
    return blockchain_;
}

std::shared_ptr<ledger::MemoryPool> LocalNode::GetMemoryPool() const
{
    // This should be set during initialization
    return mempool_;
}

void LocalNode::SetBlockchain(std::shared_ptr<ledger::Blockchain> blockchain) { blockchain_ = blockchain; }

void LocalNode::SetMemoryPool(std::shared_ptr<ledger::MemoryPool> mempool) { mempool_ = mempool; }

}  // namespace neo::network::p2p