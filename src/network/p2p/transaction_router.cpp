#include <algorithm>
#include <chrono>
#include <iostream>
#include <neo/io/uint256.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/network/p2p/transaction_router.h>
#include <unordered_set>

namespace neo::network::p2p
{
neo::network::p2p::TransactionRouter::TransactionRouter(std::shared_ptr<ledger::Blockchain> blockchain,
                                                        std::shared_ptr<ledger::MemoryPool> memPool)
    : blockchain_(blockchain), memPool_(memPool), running_(false)
{
}

neo::network::p2p::TransactionRouter::~TransactionRouter()
{
    Stop();
}

void neo::network::p2p::TransactionRouter::Start()
{
    if (running_)
        return;

    running_ = true;
    routerThread_ = std::thread(&TransactionRouter::ProcessTransactions, this);
}

void neo::network::p2p::TransactionRouter::Stop()
{
    if (!running_)
        return;

    running_ = false;

    {
        std::lock_guard<std::mutex> lock(routerMutex_);
        routerCondition_.notify_all();
    }

    if (routerThread_.joinable())
        routerThread_.join();
}

bool neo::network::p2p::TransactionRouter::IsRunning() const
{
    return running_;
}

bool neo::network::p2p::TransactionRouter::AddTransaction(std::shared_ptr<Neo3Transaction> transaction)
{
    // Check if the transaction already exists
    if (memPool_->ContainsKey(transaction->GetHash()) || blockchain_->ContainsTransaction(transaction->GetHash()))
        return false;

    // Add the transaction
    {
        std::lock_guard<std::mutex> lock(transactionsMutex_);

        // Check if the transaction already exists
        if (transactions_.find(transaction->GetHash()) != transactions_.end())
            return false;

        // Add the transaction
        transactions_[transaction->GetHash()] = transaction;
    }

    // Notify the router thread
    {
        std::lock_guard<std::mutex> lock(routerMutex_);
        routerCondition_.notify_all();
    }

    return true;
}

std::vector<std::shared_ptr<Neo3Transaction>> neo::network::p2p::TransactionRouter::GetTransactions() const
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);

    std::vector<std::shared_ptr<Neo3Transaction>> transactions;
    transactions.reserve(transactions_.size());

    for (const auto& pair : transactions_)
    {
        transactions.push_back(pair.second);
    }

    return transactions;
}

bool neo::network::p2p::TransactionRouter::RemoveTransaction(const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(transactionsMutex_);

    auto it = transactions_.find(hash);
    if (it == transactions_.end())
        return false;

    transactions_.erase(it);
    return true;
}

void neo::network::p2p::TransactionRouter::ProcessTransactions()
{
    while (running_)
    {
        // Get the transactions
        std::vector<std::shared_ptr<ledger::Transaction>> transactions;
        {
            std::lock_guard<std::mutex> lock(transactionsMutex_);
            transactions.reserve(transactions_.size());

            for (const auto& pair : transactions_)
            {
                transactions.push_back(pair.second);
            }
        }

        // Process the transactions
        for (const auto& transaction : transactions)
        {
            // Check if the transaction already exists
            if (memPool_->ContainsTransaction(transaction->GetHash()) ||
                blockchain_->ContainsTransaction(transaction->GetHash()))
            {
                RemoveTransaction(transaction->GetHash());
                continue;
            }

            // Verify the transaction
            if (!transaction->Verify())
            {
                RemoveTransaction(transaction->GetHash());
                continue;
            }

            // Check if the transaction is expired based on ValidUntilBlock
            auto currentBlockHeight = blockchain_->GetHeight();
            if (transaction->GetValidUntilBlock() <= currentBlockHeight)
            {
                // Transaction is expired, remove it
                RemoveTransaction(transaction->GetHash());
                continue;
            }

            // Add the transaction to the memory pool
            if (memPool_->AddTransaction(transaction))
            {
                // Relay the transaction to peers matching C# implementation
                try
                {
                    // Create Inv payload to announce the transaction
                    auto invPayload = std::make_shared<payloads::InvPayload>();
                    invPayload->SetType(payloads::InventoryType::TX);
                    invPayload->AddHash(transaction->GetHash());

                    // Create Inv message
                    auto invMessage = std::make_shared<Message>();
                    invMessage->SetCommand(MessageCommand::Inv);
                    invMessage->SetPayload(invPayload);

                    // Complete peer relay logic - broadcast transaction to connected peers
                    if (p2pServer_)
                    {
                        // Get list of connected peers
                        auto connected_peers = p2pServer_->GetConnectedPeers();

                        // Track relay statistics
                        int relay_count = 0;
                        int max_relays = std::min(static_cast<int>(connected_peers.size()), 8);  // Limit to 8 peers

                        // Create a set to track already relayed transactions to prevent loops
                        static std::unordered_set<io::UInt256> recently_relayed;
                        static std::mutex relay_mutex;

                        {
                            std::lock_guard<std::mutex> lock(relay_mutex);

                            // Check if already relayed recently
                            if (recently_relayed.find(transaction->GetHash()) != recently_relayed.end())
                            {
                                LOG_DEBUG("Transaction {} already relayed recently, skipping",
                                          transaction->GetHash().ToString());
                            }
                            else
                            {
                                // Add to recently relayed set
                                recently_relayed.insert(transaction->GetHash());

                                // Clean up old entries (keep only last 1000)
                                if (recently_relayed.size() > 1000)
                                {
                                    auto it = recently_relayed.begin();
                                    std::advance(it, recently_relayed.size() - 800);
                                    recently_relayed.erase(recently_relayed.begin(), it);
                                }

                                // Relay to selected peers
                                std::vector<io::UInt256> selected_peers;

                                // Randomly select peers to relay to (avoid flooding)
                                if (connected_peers.size() <= max_relays)
                                {
                                    selected_peers = connected_peers;
                                }
                                else
                                {
                                    // Randomly sample peers
                                    std::random_device rd;
                                    std::mt19937 gen(rd());
                                    std::shuffle(connected_peers.begin(), connected_peers.end(), gen);
                                    selected_peers.assign(connected_peers.begin(),
                                                          connected_peers.begin() + max_relays);
                                }

                                // Send Inv message to selected peers
                                for (const auto& peer_id : selected_peers)
                                {
                                    try
                                    {
                                        // Check if peer is still connected and handshaked
                                        if (p2pServer_->IsPeerHandshaked(peer_id))
                                        {
                                            // Send the Inv message
                                            p2pServer_->SendMessage(peer_id, *invMessage);
                                            relay_count++;

                                            LOG_DEBUG("Relayed transaction {} to peer {}",
                                                      transaction->GetHash().ToString(), peer_id.ToString());
                                        }
                                    }
                                    catch (const std::exception& e)
                                    {
                                        LOG_WARNING("Failed to relay transaction {} to peer {}: {}",
                                                    transaction->GetHash().ToString(), peer_id.ToString(), e.what());
                                    }
                                }

                                LOG_INFO("Successfully relayed transaction {} to {} peers",
                                         transaction->GetHash().ToString(), relay_count);
                            }
                        }
                    }
                    else
                    {
                        LOG_WARNING("P2P server not available for transaction relay");
                    }

                    // Remove from pending queue since we successfully relayed it
                    this->RemoveTransaction(transaction->GetHash());
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error relaying transaction: " << e.what() << std::endl;
                    this->RemoveTransaction(transaction->GetHash());
                }
            }
        }

        // Clean up expired transactions
        CleanupExpiredTransactions();

        // Wait for a notification or a timeout
        std::unique_lock<std::mutex> lock(routerMutex_);
        routerCondition_.wait_for(lock, std::chrono::seconds(5));
    }
}

void neo::network::p2p::TransactionRouter::CleanupExpiredTransactions()
{
    auto now = std::chrono::system_clock::now();
    auto expiration = std::chrono::seconds(60);

    // Clean up expired transactions
    {
        std::lock_guard<std::mutex> lock(transactionsMutex_);

        for (auto it = transactions_.begin(); it != transactions_.end();)
        {
            // Check if the transaction is expired based on ValidUntilBlock
            auto currentBlockHeight = blockchain_->GetHeight();
            if (it->second->GetValidUntilBlock() <= currentBlockHeight)
            {
                // Transaction is expired, remove it
                it = transactions_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
}  // namespace neo::network::p2p
