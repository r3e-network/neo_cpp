/**
 * @file event_system_usage.cpp
 * @brief Example demonstrating C# compatible event system usage
 *
 * This example shows how to use the static event system that mimics
 * C# Neo's event handling pattern.
 */

#include <iostream>
#include <memory>
#include <neo/ledger/event_system.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/core/neo_system.h>

using namespace neo::ledger;
using neo::ledger::Block;
using neo::NeoSystem;

// Example event handlers
void OnTransactionAdded(std::shared_ptr<Transaction> transaction)
{
    std::cout << "Transaction added: " << transaction->GetHash().ToString() << std::endl;
}

void OnTransactionRemoved(const TransactionRemovedEventArgs& args)
{
    std::cout << "Transaction removed: " << args.transaction->GetHash().ToString()
              << " Reason: " << static_cast<int>(args.reason) << std::endl;
}

void OnBlockCommitted(std::shared_ptr<NeoSystem> system, std::shared_ptr<Block> block)
{
    std::cout << "Block committed: " << block->GetHash().ToString() << std::endl;
}

int main()
{
    std::cout << "Neo C++ Event System Demo - C# Compatibility" << std::endl;
    std::cout << "=============================================" << std::endl;

    // Method 1: Direct static event subscription (matches C# pattern)
    std::cout << "\n1. Subscribing to events using static methods:" << std::endl;

    MemoryPoolEvents::SubscribeTransactionAdded(OnTransactionAdded);
    MemoryPoolEvents::SubscribeTransactionRemoved(OnTransactionRemoved);
    BlockchainEvents::SubscribeCommitted(OnBlockCommitted);

    std::cout << "✓ Subscribed to MemoryPool.TransactionAdded event" << std::endl;
    std::cout << "✓ Subscribed to MemoryPool.TransactionRemoved event" << std::endl;
    std::cout << "✓ Subscribed to Blockchain.Committed event" << std::endl;

    // Method 2: RAII-style subscription (C++ enhancement)
    std::cout << "\n2. Using RAII-style event subscriptions:" << std::endl;

    {
        auto transaction_subscription = std::make_unique<MemoryPoolTransactionAddedSubscription>(
            [](std::shared_ptr<Transaction> tx)
            { std::cout << "RAII handler: Transaction added " << tx->GetHash().ToString() << std::endl; });
        transaction_subscription->Subscribe();
        std::cout << "✓ RAII subscription active" << std::endl;

        // Simulate some events
        std::cout << "\n3. Simulating events:" << std::endl;

        // Create a mock transaction
        auto mock_transaction = std::make_shared<Transaction>();

        // Fire events through static system (this is what MemoryPool does internally)
        MemoryPoolEvents::FireTransactionAdded(mock_transaction);

        TransactionRemovedEventArgs remove_args(mock_transaction, TransactionRemovedEventArgs::Reason::LowPriority);
        MemoryPoolEvents::FireTransactionRemoved(remove_args);

    }  // RAII subscription automatically unsubscribes here

    std::cout << "\n4. RAII subscription automatically unsubscribed" << std::endl;

    // Method 3: Lambda expressions (modern C++ style)
    std::cout << "\n5. Using lambda expressions:" << std::endl;

    MemoryPoolEvents::SubscribeTransactionAdded(
        [](std::shared_ptr<Transaction> tx) { std::cout << "Lambda handler: New transaction in pool" << std::endl; });

    // Demonstrate that multiple handlers can be registered
    std::cout << "\n6. Multiple handlers demonstration:" << std::endl;

    MemoryPoolEvents::SubscribeTransactionAdded(
        [](std::shared_ptr<Transaction> tx)
        { std::cout << "Second lambda handler: Processing transaction..." << std::endl; });

    // Fire another event to show multiple handlers
    auto another_transaction = std::make_shared<Transaction>();
    MemoryPoolEvents::FireTransactionAdded(another_transaction);

    // Clean up
    std::cout << "\n7. Cleaning up subscriptions:" << std::endl;
    MemoryPoolEvents::ClearAllSubscriptions();
    BlockchainEvents::ClearAllSubscriptions();
    std::cout << "✓ All event subscriptions cleared" << std::endl;

    std::cout << "\nDemo completed successfully!" << std::endl;
    std::cout << "\nC# Compatibility Notes:" << std::endl;
    std::cout << "- Static event subscription matches C# Neo pattern" << std::endl;
    std::cout << "- Event firing is automatic from MemoryPool and Blockchain" << std::endl;
    std::cout << "- RAII subscriptions provide automatic cleanup" << std::endl;
    std::cout << "- Multiple handlers per event are supported" << std::endl;

    return 0;
}

/**
 * Example C# equivalent code that this system emulates:
 *
 * // C# Neo event subscription
 * MemoryPool.TransactionAdded += OnTransactionAdded;
 * MemoryPool.TransactionRemoved += OnTransactionRemoved;
 * Blockchain.Committed += OnBlockCommitted;
 *
 * // C# Neo event handlers
 * private void OnTransactionAdded(Transaction transaction)
 * {
 *     Console.WriteLine($"Transaction added: {transaction.Hash}");
 * }
 *
 * private void OnTransactionRemoved(TransactionRemovedEventArgs args)
 * {
 *     Console.WriteLine($"Transaction removed: {args.Transaction.Hash} Reason: {args.Reason}");
 * }
 *
 * private void OnBlockCommitted(NeoSystem system, Block block)
 * {
 *     Console.WriteLine($"Block committed: {block.Hash}");
 * }
 */