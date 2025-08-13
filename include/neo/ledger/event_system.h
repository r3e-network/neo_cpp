/**
 * @file event_system.h
 * @brief Event handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

// Include transaction type
#include <neo/ledger/transaction.h>

// Forward declarations
namespace neo
{
class NeoSystem;
}

namespace neo::ledger
{
// Forward declarations
class Block;
struct TransactionRemovedEventArgs;

/**
 * @brief Static event system that mimics C#'s static event pattern.
 * This provides compatibility with C# Neo's event handling while using C++ callbacks internally.
 */
class MemoryPoolEvents
{
   public:
    // Event handler types
    using TransactionAddedHandler = std::function<void(std::shared_ptr<Transaction>)>;
    using TransactionRemovedHandler = std::function<void(const TransactionRemovedEventArgs&)>;

    /**
     * @brief Subscribes to the TransactionAdded event.
     * @param handler The handler function to call when a transaction is added.
     */
    static void SubscribeTransactionAdded(TransactionAddedHandler handler);

    /**
     * @brief Unsubscribes from the TransactionAdded event.
     * @param handler The handler function to remove.
     */
    static void UnsubscribeTransactionAdded(TransactionAddedHandler handler);

    /**
     * @brief Subscribes to the TransactionRemoved event.
     * @param handler The handler function to call when a transaction is removed.
     */
    static void SubscribeTransactionRemoved(TransactionRemovedHandler handler);

    /**
     * @brief Unsubscribes from the TransactionRemoved event.
     * @param handler The handler function to remove.
     */
    static void UnsubscribeTransactionRemoved(TransactionRemovedHandler handler);

    /**
     * @brief Fires the TransactionAdded event to all subscribers.
     * @param transaction The transaction that was added.
     */
    static void FireTransactionAdded(std::shared_ptr<Transaction> transaction);

    /**
     * @brief Fires the TransactionRemoved event to all subscribers.
     * @param args The event arguments containing the transaction and removal reason.
     */
    static void FireTransactionRemoved(const TransactionRemovedEventArgs& args);

    /**
     * @brief Clears all event subscriptions (useful for testing).
     */
    static void ClearAllSubscriptions();

   private:
    static std::vector<TransactionAddedHandler> transaction_added_handlers_;
    static std::vector<TransactionRemovedHandler> transaction_removed_handlers_;
    static std::mutex handlers_mutex_;
};

/**
 * @brief Static event system for Blockchain events.
 */
class BlockchainEvents
{
   public:
    // Forward declarations
    class ApplicationExecuted;
    class DataCache;

    // Event handler types
    using CommittingHandler = std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>,
                                                 std::shared_ptr<DataCache>, const std::vector<ApplicationExecuted>&)>;
    using CommittedHandler = std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>)>;
    using BlockPersistedHandler = std::function<void(std::shared_ptr<Block>)>;

    /**
     * @brief Subscribes to the Committing event.
     */
    static void SubscribeCommitting(CommittingHandler handler);

    /**
     * @brief Subscribes to the Committed event.
     */
    static void SubscribeCommitted(CommittedHandler handler);

    /**
     * @brief Subscribes to the BlockPersisted event.
     */
    static void SubscribeBlockPersisted(BlockPersistedHandler handler);

    /**
     * @brief Fires the Committing event.
     */
    static void FireCommitting(std::shared_ptr<NeoSystem> system, std::shared_ptr<Block> block,
                               std::shared_ptr<DataCache> cache, const std::vector<ApplicationExecuted>& executed);

    /**
     * @brief Fires the Committed event.
     */
    static void FireCommitted(std::shared_ptr<NeoSystem> system, std::shared_ptr<Block> block);

    /**
     * @brief Fires the BlockPersisted event.
     */
    static void FireBlockPersisted(std::shared_ptr<Block> block);

    /**
     * @brief Clears all event subscriptions.
     */
    static void ClearAllSubscriptions();

   private:
    static std::vector<CommittingHandler> committing_handlers_;
    static std::vector<CommittedHandler> committed_handlers_;
    static std::vector<BlockPersistedHandler> block_persisted_handlers_;
    static std::mutex handlers_mutex_;
};

/**
 * @brief RAII-style event subscription helper.
 * Automatically unsubscribes when the object is destroyed.
 */
template <typename EventClass, typename Handler>
class EventSubscription
{
   public:
    EventSubscription(Handler handler) : handler_(handler), subscribed_(false) {}

    ~EventSubscription()
    {
        if (subscribed_)
        {
            Unsubscribe();
        }
    }

    // Disable copy and move to prevent double unsubscription
    EventSubscription(const EventSubscription&) = delete;
    EventSubscription& operator=(const EventSubscription&) = delete;
    EventSubscription(EventSubscription&&) = delete;
    EventSubscription& operator=(EventSubscription&&) = delete;

    void Subscribe();
    void Unsubscribe();
    bool IsSubscribed() const { return subscribed_; }

   private:
    Handler handler_;
    bool subscribed_;
};

// Convenience type aliases for RAII subscriptions
using MemoryPoolTransactionAddedSubscription =
    EventSubscription<MemoryPoolEvents, MemoryPoolEvents::TransactionAddedHandler>;
using MemoryPoolTransactionRemovedSubscription =
    EventSubscription<MemoryPoolEvents, MemoryPoolEvents::TransactionRemovedHandler>;
using BlockchainCommittingSubscription = EventSubscription<BlockchainEvents, BlockchainEvents::CommittingHandler>;
using BlockchainCommittedSubscription = EventSubscription<BlockchainEvents, BlockchainEvents::CommittedHandler>;
using BlockchainBlockPersistedSubscription =
    EventSubscription<BlockchainEvents, BlockchainEvents::BlockPersistedHandler>;

}  // namespace neo::ledger