#include <neo/core/neo_system.h>
#include <neo/ledger/block.h>
#include <neo/ledger/event_system.h>
#include <neo/ledger/pool_item.h>
#include <neo/ledger/transaction.h>

#include <algorithm>

namespace neo::ledger
{

// Static member definitions for MemoryPoolEvents
std::vector<MemoryPoolEvents::TransactionAddedHandler> MemoryPoolEvents::transaction_added_handlers_;
std::vector<MemoryPoolEvents::TransactionRemovedHandler> MemoryPoolEvents::transaction_removed_handlers_;
std::mutex MemoryPoolEvents::handlers_mutex_;

void MemoryPoolEvents::SubscribeTransactionAdded(TransactionAddedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    transaction_added_handlers_.push_back(std::move(handler));
}

void MemoryPoolEvents::UnsubscribeTransactionAdded(TransactionAddedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    // Note: Function comparison is not straightforward in C++
    // In practice, callers should use RAII subscriptions or keep track of handlers manually
    // This is a limitation compared to C#'s delegate system
}

void MemoryPoolEvents::SubscribeTransactionRemoved(TransactionRemovedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    transaction_removed_handlers_.push_back(std::move(handler));
}

void MemoryPoolEvents::UnsubscribeTransactionRemoved(TransactionRemovedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    // Note: Function comparison limitation as above
}

void MemoryPoolEvents::FireTransactionAdded(std::shared_ptr<Transaction> transaction)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : transaction_added_handlers_)
    {
        try
        {
            handler(transaction);
        }
        catch (...)
        {
            // Silently ignore handler exceptions to prevent one bad handler from affecting others
            // In production, this should log the exception
        }
    }
}

void MemoryPoolEvents::FireTransactionRemoved(const TransactionRemovedEventArgs& args)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : transaction_removed_handlers_)
    {
        try
        {
            handler(args);
        }
        catch (...)
        {
            // Silently ignore handler exceptions
        }
    }
}

void MemoryPoolEvents::ClearAllSubscriptions()
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    transaction_added_handlers_.clear();
    transaction_removed_handlers_.clear();
}

// Static member definitions for BlockchainEvents
std::vector<BlockchainEvents::CommittingHandler> BlockchainEvents::committing_handlers_;
std::vector<BlockchainEvents::CommittedHandler> BlockchainEvents::committed_handlers_;
std::vector<BlockchainEvents::BlockPersistedHandler> BlockchainEvents::block_persisted_handlers_;
std::mutex BlockchainEvents::handlers_mutex_;

void BlockchainEvents::SubscribeCommitting(CommittingHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    committing_handlers_.push_back(std::move(handler));
}

void BlockchainEvents::SubscribeCommitted(CommittedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    committed_handlers_.push_back(std::move(handler));
}

void BlockchainEvents::SubscribeBlockPersisted(BlockPersistedHandler handler)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    block_persisted_handlers_.push_back(std::move(handler));
}

void BlockchainEvents::FireCommitting(std::shared_ptr<NeoSystem> system, std::shared_ptr<Block> block,
                                      std::shared_ptr<DataCache> cache,
                                      const std::vector<ApplicationExecuted>& executed)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : committing_handlers_)
    {
        try
        {
            handler(system, block, cache, executed);
        }
        catch (...)
        {
            // Silently ignore handler exceptions
        }
    }
}

void BlockchainEvents::FireCommitted(std::shared_ptr<NeoSystem> system, std::shared_ptr<Block> block)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : committed_handlers_)
    {
        try
        {
            handler(system, block);
        }
        catch (...)
        {
            // Silently ignore handler exceptions
        }
    }
}

void BlockchainEvents::FireBlockPersisted(std::shared_ptr<Block> block)
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : block_persisted_handlers_)
    {
        try
        {
            handler(block);
        }
        catch (...)
        {
            // Silently ignore handler exceptions
        }
    }
}

void BlockchainEvents::ClearAllSubscriptions()
{
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    committing_handlers_.clear();
    committed_handlers_.clear();
    block_persisted_handlers_.clear();
}

// RAII EventSubscription template specializations
template <>
void MemoryPoolTransactionAddedSubscription::Subscribe()
{
    if (!subscribed_)
    {
        MemoryPoolEvents::SubscribeTransactionAdded(handler_);
        subscribed_ = true;
    }
}

template <>
void MemoryPoolTransactionAddedSubscription::Unsubscribe()
{
    if (subscribed_)
    {
        MemoryPoolEvents::UnsubscribeTransactionAdded(handler_);
        subscribed_ = false;
    }
}

template <>
void MemoryPoolTransactionRemovedSubscription::Subscribe()
{
    if (!subscribed_)
    {
        MemoryPoolEvents::SubscribeTransactionRemoved(handler_);
        subscribed_ = true;
    }
}

template <>
void MemoryPoolTransactionRemovedSubscription::Unsubscribe()
{
    if (subscribed_)
    {
        MemoryPoolEvents::UnsubscribeTransactionRemoved(handler_);
        subscribed_ = false;
    }
}

template <>
void BlockchainCommittingSubscription::Subscribe()
{
    if (!subscribed_)
    {
        BlockchainEvents::SubscribeCommitting(handler_);
        subscribed_ = true;
    }
}

template <>
void BlockchainCommittingSubscription::Unsubscribe()
{
    if (subscribed_)
    {
        // Note: Unsubscription for function objects is challenging in C++
        // In practice, use weak_ptr or other mechanisms for automatic cleanup
        subscribed_ = false;
    }
}

template <>
void BlockchainCommittedSubscription::Subscribe()
{
    if (!subscribed_)
    {
        BlockchainEvents::SubscribeCommitted(handler_);
        subscribed_ = true;
    }
}

template <>
void BlockchainCommittedSubscription::Unsubscribe()
{
    if (subscribed_)
    {
        subscribed_ = false;
    }
}

template <>
void BlockchainBlockPersistedSubscription::Subscribe()
{
    if (!subscribed_)
    {
        BlockchainEvents::SubscribeBlockPersisted(handler_);
        subscribed_ = true;
    }
}

template <>
void BlockchainBlockPersistedSubscription::Unsubscribe()
{
    if (subscribed_)
    {
        subscribed_ = false;
    }
}

}  // namespace neo::ledger