#include <neo/network/p2p/transaction_router.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/ledger/transaction.h>
#include <neo/io/uint256.h>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace neo::network::p2p
{
    TransactionRouter::TransactionRouter(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memPool)
        : blockchain_(blockchain), memPool_(memPool), running_(false)
    {
    }

    TransactionRouter::~TransactionRouter()
    {
        Stop();
    }

    void TransactionRouter::Start()
    {
        if (running_)
            return;

        running_ = true;
        routerThread_ = std::thread(&TransactionRouter::ProcessTransactions, this);
    }

    void TransactionRouter::Stop()
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

    bool TransactionRouter::IsRunning() const
    {
        return running_;
    }

    bool TransactionRouter::AddTransaction(std::shared_ptr<ledger::Transaction> transaction)
    {
        // Check if the transaction already exists
        if (memPool_->ContainsTransaction(transaction->GetHash()) || blockchain_->ContainsTransaction(transaction->GetHash()))
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

    std::vector<std::shared_ptr<ledger::Transaction>> TransactionRouter::GetTransactions() const
    {
        std::lock_guard<std::mutex> lock(transactionsMutex_);

        std::vector<std::shared_ptr<ledger::Transaction>> transactions;
        transactions.reserve(transactions_.size());

        for (const auto& pair : transactions_)
        {
            transactions.push_back(pair.second);
        }

        return transactions;
    }

    bool TransactionRouter::RemoveTransaction(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(transactionsMutex_);

        auto it = transactions_.find(hash);
        if (it == transactions_.end())
            return false;

        transactions_.erase(it);
        return true;
    }

    void TransactionRouter::ProcessTransactions()
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
                if (memPool_->ContainsTransaction(transaction->GetHash()) || blockchain_->ContainsTransaction(transaction->GetHash()))
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

                // Add the transaction to the memory pool
                if (memPool_->AddTransaction(transaction))
                {
                    // TODO: Relay the transaction to peers
                    RemoveTransaction(transaction->GetHash());
                }
            }

            // Clean up expired transactions
            CleanupExpiredTransactions();

            // Wait for a notification or a timeout
            std::unique_lock<std::mutex> lock(routerMutex_);
            routerCondition_.wait_for(lock, std::chrono::seconds(5));
        }
    }

    void TransactionRouter::CleanupExpiredTransactions()
    {
        auto now = std::chrono::system_clock::now();
        auto expiration = std::chrono::seconds(60);

        // Clean up expired transactions
        {
            std::lock_guard<std::mutex> lock(transactionsMutex_);

            for (auto it = transactions_.begin(); it != transactions_.end();)
            {
                // TODO: Check if the transaction is expired
                ++it;
            }
        }
    }
}
