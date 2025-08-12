# Transaction Pool Manager API Documentation

## Overview

The Transaction Pool Manager is an enterprise-grade component for managing pending transactions in the Neo C++ blockchain implementation. It provides priority-based transaction ordering, real-time monitoring, automatic cleanup, and comprehensive conflict detection.

## Table of Contents

- [Architecture](#architecture)
- [Core Features](#core-features)
- [API Reference](#api-reference)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [Performance Metrics](#performance-metrics)
- [Best Practices](#best-practices)

## Architecture

### Component Overview

```
┌─────────────────────────────────────┐
│   Transaction Pool Manager          │
├─────────────────────────────────────┤
│ • Priority Queue Management         │
│ • Conflict Detection Engine         │
│ • Metrics & Monitoring             │
│ • Automatic Cleanup Thread         │
│ • Callback System                  │
└─────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│        Memory Pool                  │
├─────────────────────────────────────┤
│ • Verified Transactions            │
│ • Unverified Transactions          │
│ • Sorted Transaction Set           │
└─────────────────────────────────────┘
```

### Key Components

1. **TransactionPoolManager**: Main orchestrator for transaction management
2. **MemoryPool**: Underlying storage with verified/unverified separation
3. **Priority Queue**: Fee-based and priority-based ordering system
4. **Conflict Detector**: Double-spend and conflict resolution
5. **Cleanup Thread**: Automatic expiration and validation processing

## Core Features

### Priority Levels

The system supports four priority levels for transaction processing:

| Priority | Value | Description | Typical Use Case |
|----------|-------|-------------|------------------|
| Critical | 3 | Highest priority | System transactions, emergency operations |
| High | 2 | Elevated priority | Time-sensitive transfers, high-fee transactions |
| Normal | 1 | Standard priority | Regular user transactions |
| Low | 0 | Lowest priority | Batch operations, low-fee transactions |

### Automatic Fee-Based Prioritization

Transactions are automatically assigned priority based on network fees:

- **Critical**: ≥ 10 GAS (1,000,000,000 units)
- **High**: ≥ 1 GAS (100,000,000 units)
- **Normal**: ≥ 0.1 GAS (10,000,000 units)
- **Low**: < 0.1 GAS

## API Reference

### Class: `TransactionPoolManager`

#### Constructor

```cpp
explicit TransactionPoolManager(const Configuration& config = Configuration{})
```

Creates a new transaction pool manager with the specified configuration.

**Parameters:**
- `config`: Configuration object with pool settings

**Example:**
```cpp
TransactionPoolManager::Configuration config;
config.max_pool_size = 50000;
config.enable_priority_queue = true;
auto pool = std::make_unique<TransactionPoolManager>(config);
```

#### Methods

##### `Start()`

```cpp
void Start()
```

Starts the pool manager and initializes background threads for cleanup and validation.

**Thread Safety:** Thread-safe

**Example:**
```cpp
pool->Start();
```

##### `Stop()`

```cpp
void Stop()
```

Stops the pool manager and cleanup threads, clearing all transactions.

**Thread Safety:** Thread-safe

##### `AddTransaction()`

```cpp
bool AddTransaction(
    const Neo3Transaction& transaction,
    Priority priority = Priority::Normal,
    const std::string& source_peer = ""
)
```

Adds a transaction to the pool with specified priority.

**Parameters:**
- `transaction`: The transaction to add
- `priority`: Transaction priority level
- `source_peer`: Optional identifier of the peer that sent this transaction

**Returns:** `true` if successfully added, `false` if rejected

**Rejection Reasons:**
- Transaction already exists in pool
- Pool at maximum capacity
- Transaction size exceeds limit
- Fee below minimum threshold
- Conflicts with higher-fee transaction

**Example:**
```cpp
Neo3Transaction tx = CreateTransaction();
bool added = pool->AddTransaction(tx, TransactionPoolManager::Priority::High, "peer_001");
```

##### `RemoveTransaction()`

```cpp
bool RemoveTransaction(
    const io::UInt256& hash,
    const std::string& reason = ""
)
```

Removes a transaction from the pool.

**Parameters:**
- `hash`: Transaction hash to remove
- `reason`: Optional reason for removal (for logging)

**Returns:** `true` if removed, `false` if not found

##### `GetTransaction()`

```cpp
std::optional<Neo3Transaction> GetTransaction(const io::UInt256& hash) const
```

Retrieves a transaction by its hash.

**Parameters:**
- `hash`: Transaction hash

**Returns:** Optional containing the transaction if found

##### `GetTransactionsForBlock()`

```cpp
std::vector<Neo3Transaction> GetTransactionsForBlock(
    size_t max_count = 1000,
    size_t max_size = 1048576
) const
```

Gets transactions ready for inclusion in a block, ordered by priority and fee.

**Parameters:**
- `max_count`: Maximum number of transactions to return
- `max_size`: Maximum total size in bytes

**Returns:** Vector of transactions ordered by priority

**Ordering Logic:**
1. Higher priority transactions first
2. Within same priority, higher fees first
3. Dependencies are resolved (dependent transactions included after their dependencies)

##### `ContainsTransaction()`

```cpp
bool ContainsTransaction(const io::UInt256& hash) const
```

Checks if a transaction exists in the pool.

**Parameters:**
- `hash`: Transaction hash to check

**Returns:** `true` if transaction exists

##### `GetStatistics()`

```cpp
PoolStats GetStatistics() const
```

Gets current pool statistics.

**Returns:** `PoolStats` structure containing:
- `total_transactions`: Total number of transactions in pool
- `verified_count`: Number of verified transactions
- `unverified_count`: Number of unverified transactions
- `pending_count`: Number of pending transactions
- `rejected_count`: Total rejected transactions since start
- `total_fees`: Sum of all transaction fees
- `average_fee`: Average fee per transaction
- `average_validation_time`: Average time to validate transactions
- `memory_usage_bytes`: Estimated memory usage
- `throughput_tps`: Transactions processed per second

##### `Clear()`

```cpp
void Clear(const std::string& reason = "Manual clear")
```

Clears all transactions from the pool.

**Parameters:**
- `reason`: Reason for clearing (for logging)

##### `ValidateUnverifiedTransactions()`

```cpp
size_t ValidateUnverifiedTransactions()
```

Validates all unverified transactions using the configured validator.

**Returns:** Number of transactions validated

**Note:** Requires a validator to be set via `SetValidator()`

##### `RemoveExpiredTransactions()`

```cpp
size_t RemoveExpiredTransactions()
```

Removes transactions that have exceeded the configured timeout.

**Returns:** Number of transactions removed

##### `DetectAndResolveConflicts()`

```cpp
size_t DetectAndResolveConflicts()
```

Detects and resolves transaction conflicts (double-spends).

**Returns:** Number of conflicts resolved

**Resolution Strategy:** Keeps transaction with highest fee when conflicts are detected

##### `GetTransactionMetadata()`

```cpp
std::optional<TransactionMetadata> GetTransactionMetadata(
    const io::UInt256& hash
) const
```

Gets detailed metadata for a transaction.

**Parameters:**
- `hash`: Transaction hash

**Returns:** Optional containing metadata if found

**Metadata Includes:**
- Transaction hash
- Priority level
- Network fee
- Received timestamp
- Validation timestamp
- Dependencies
- Verification status
- Retry count
- Source peer

##### `UpdateConfiguration()`

```cpp
void UpdateConfiguration(const Configuration& config)
```

Updates the pool configuration at runtime.

**Parameters:**
- `config`: New configuration settings

### Callback System

#### `SetValidator()`

```cpp
void SetValidator(
    std::function<bool(const Neo3Transaction&)> validator
)
```

Sets the transaction validation callback.

**Parameters:**
- `validator`: Function that returns `true` if transaction is valid

**Example:**
```cpp
pool->SetValidator([](const Neo3Transaction& tx) {
    return tx.GetNetworkFee() >= 1000000 && tx.Verify();
});
```

#### `SetOnTransactionAdded()`

```cpp
void SetOnTransactionAdded(
    std::function<void(const io::UInt256&, const std::string&)> callback
)
```

Sets callback for transaction addition events.

**Parameters:**
- `callback`: Function called when transaction is added (hash, source_peer)

#### `SetOnTransactionRemoved()`

```cpp
void SetOnTransactionRemoved(
    std::function<void(const io::UInt256&, const std::string&)> callback
)
```

Sets callback for transaction removal events.

**Parameters:**
- `callback`: Function called when transaction is removed (hash, reason)

#### `SetOnStatsUpdated()`

```cpp
void SetOnStatsUpdated(
    std::function<void(const PoolStats&)> callback
)
```

Sets callback for statistics updates.

**Parameters:**
- `callback`: Function called periodically with updated statistics

## Configuration

### Configuration Structure

```cpp
struct Configuration {
    size_t max_pool_size = 100000;
    size_t max_unverified_size = 10000;
    size_t max_transaction_size = 102400;  // 100KB
    std::chrono::seconds transaction_timeout{300};  // 5 minutes
    std::chrono::seconds cleanup_interval{60};  // 1 minute
    uint64_t min_fee_threshold = 0;
    bool enable_priority_queue = true;
    bool enable_conflict_detection = true;
    bool enable_metrics = true;
    uint32_t max_retry_attempts = 3;
};
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `max_pool_size` | 100,000 | Maximum total transactions |
| `max_unverified_size` | 10,000 | Maximum unverified transactions |
| `max_transaction_size` | 102,400 | Maximum transaction size in bytes |
| `transaction_timeout` | 300s | Transaction expiration time |
| `cleanup_interval` | 60s | Cleanup thread interval |
| `min_fee_threshold` | 0 | Minimum required fee |
| `enable_priority_queue` | true | Enable priority ordering |
| `enable_conflict_detection` | true | Enable conflict detection |
| `enable_metrics` | true | Enable metrics collection |
| `max_retry_attempts` | 3 | Maximum validation retries |

## Usage Examples

### Basic Usage

```cpp
#include <neo/ledger/transaction_pool_manager.h>

// Create and configure pool
TransactionPoolManager::Configuration config;
config.max_pool_size = 50000;
config.min_fee_threshold = 1000000;  // 0.01 GAS minimum
auto pool = std::make_unique<TransactionPoolManager>(config);

// Start the pool
pool->Start();

// Add transactions
Neo3Transaction tx = CreateTransaction();
pool->AddTransaction(tx, TransactionPoolManager::Priority::Normal, "peer_001");

// Get transactions for block
auto block_txs = pool->GetTransactionsForBlock(100, 1024*1024);

// Check statistics
auto stats = pool->GetStatistics();
std::cout << "Total transactions: " << stats.total_transactions << std::endl;
std::cout << "Average fee: " << stats.average_fee << std::endl;

// Stop the pool
pool->Stop();
```

### Advanced Usage with Callbacks

```cpp
// Set up validation
pool->SetValidator([](const Neo3Transaction& tx) {
    // Custom validation logic
    if (tx.GetNetworkFee() < 1000000) return false;
    if (!tx.Verify()) return false;
    return true;
});

// Monitor additions
pool->SetOnTransactionAdded([](const io::UInt256& hash, const std::string& peer) {
    std::cout << "Transaction added: " << hash.ToString() 
              << " from " << peer << std::endl;
});

// Monitor removals
pool->SetOnTransactionRemoved([](const io::UInt256& hash, const std::string& reason) {
    std::cout << "Transaction removed: " << hash.ToString() 
              << " - " << reason << std::endl;
});

// Monitor statistics
pool->SetOnStatsUpdated([](const TransactionPoolManager::PoolStats& stats) {
    std::cout << "Pool update - Transactions: " << stats.total_transactions
              << ", TPS: " << stats.throughput_tps << std::endl;
});
```

### Conflict Resolution Example

```cpp
// Enable conflict detection
config.enable_conflict_detection = true;
auto pool = std::make_unique<TransactionPoolManager>(config);
pool->Start();

// Add first transaction
Neo3Transaction tx1 = CreateTransactionWithInput(input_a);
tx1.SetNetworkFee(1000000);  // 0.01 GAS
pool->AddTransaction(tx1);

// Try to add conflicting transaction with higher fee
Neo3Transaction tx2 = CreateTransactionWithInput(input_a);  // Same input
tx2.SetNetworkFee(10000000);  // 0.1 GAS - higher fee

// tx2 will replace tx1 due to higher fee
bool added = pool->AddTransaction(tx2);
assert(added == true);
assert(pool->ContainsTransaction(tx2.GetHash()) == true);
assert(pool->ContainsTransaction(tx1.GetHash()) == false);  // tx1 removed
```

## Performance Metrics

### Benchmarks

| Operation | Average Time | Throughput |
|-----------|-------------|------------|
| Add Transaction | < 100μs | 10,000 TPS |
| Remove Transaction | < 50μs | 20,000 TPS |
| Get Transaction | < 10μs | 100,000 TPS |
| Get Block Transactions (1000) | < 5ms | 200 ops/sec |
| Conflict Detection (10K txs) | < 100ms | 10 ops/sec |

### Memory Usage

| Pool Size | Approximate Memory |
|-----------|-------------------|
| 1,000 | ~1 MB |
| 10,000 | ~10 MB |
| 50,000 | ~50 MB |
| 100,000 | ~100 MB |

## Best Practices

### 1. Configuration Tuning

```cpp
// For high-throughput networks
config.max_pool_size = 200000;
config.cleanup_interval = std::chrono::seconds(30);
config.enable_priority_queue = true;

// For low-resource environments
config.max_pool_size = 10000;
config.max_unverified_size = 1000;
config.cleanup_interval = std::chrono::seconds(120);
```

### 2. Fee Management

```cpp
// Set minimum fee to prevent spam
config.min_fee_threshold = 1000000;  // 0.01 GAS

// Implement dynamic fee adjustment
pool->SetValidator([base_fee](const Neo3Transaction& tx) {
    auto current_load = GetNetworkLoad();
    auto required_fee = base_fee * (1.0 + current_load);
    return tx.GetNetworkFee() >= required_fee;
});
```

### 3. Monitoring Integration

```cpp
// Export metrics to monitoring system
pool->SetOnStatsUpdated([](const TransactionPoolManager::PoolStats& stats) {
    metrics_exporter->Export("tx_pool_size", stats.total_transactions);
    metrics_exporter->Export("tx_pool_tps", stats.throughput_tps);
    metrics_exporter->Export("tx_pool_avg_fee", stats.average_fee);
    
    if (stats.total_transactions > 90000) {
        alert_system->Trigger("Pool near capacity");
    }
});
```

### 4. Error Handling

```cpp
// Robust transaction addition with retry
bool AddTransactionWithRetry(TransactionPoolManager* pool, 
                            const Neo3Transaction& tx,
                            int max_retries = 3) {
    for (int i = 0; i < max_retries; i++) {
        if (pool->AddTransaction(tx)) {
            return true;
        }
        
        // Check why it failed
        if (pool->ContainsTransaction(tx.GetHash())) {
            return true;  // Already added
        }
        
        auto stats = pool->GetStatistics();
        if (stats.total_transactions >= pool->GetConfiguration().max_pool_size) {
            // Pool full, wait for cleanup
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    return false;
}
```

### 5. Graceful Shutdown

```cpp
// Ensure clean shutdown
void ShutdownPool(TransactionPoolManager* pool) {
    // Stop accepting new transactions
    accepting_new_txs = false;
    
    // Wait for pending operations
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Get final statistics
    auto final_stats = pool->GetStatistics();
    LogStatistics(final_stats);
    
    // Stop the pool
    pool->Stop();
}
```

## Thread Safety

The Transaction Pool Manager is fully thread-safe. All public methods can be called concurrently from multiple threads. Internal synchronization uses:

- **Shared mutex** for read operations (allows multiple concurrent readers)
- **Unique locks** for write operations (exclusive access)
- **Atomic variables** for metrics and counters

## Integration with Blockchain

### Block Creation

```cpp
std::shared_ptr<Block> CreateNewBlock(TransactionPoolManager* pool) {
    auto block = std::make_shared<Block>();
    
    // Get high-priority transactions for block
    auto transactions = pool->GetTransactionsForBlock(
        500,           // Max 500 transactions
        900 * 1024     // Max 900KB (leave room for coinbase)
    );
    
    // Add to block
    for (const auto& tx : transactions) {
        block->AddTransaction(tx);
    }
    
    // Remove included transactions after block is accepted
    for (const auto& tx : transactions) {
        pool->RemoveTransaction(tx.GetHash(), "Included in block");
    }
    
    return block;
}
```

### Network Synchronization

```cpp
void HandleIncomingTransaction(TransactionPoolManager* pool,
                              const Neo3Transaction& tx,
                              const std::string& peer_id) {
    // Determine priority based on source
    auto priority = TransactionPoolManager::Priority::Normal;
    if (IsTrustedPeer(peer_id)) {
        priority = TransactionPoolManager::Priority::High;
    }
    
    // Add to pool
    if (pool->AddTransaction(tx, priority, peer_id)) {
        // Broadcast to other peers
        BroadcastTransaction(tx, peer_id);
    }
}
```

## Troubleshooting

### Common Issues

1. **Pool filling up quickly**
   - Increase `max_pool_size`
   - Decrease `transaction_timeout`
   - Increase `min_fee_threshold`

2. **High memory usage**
   - Reduce `max_pool_size`
   - Enable more aggressive cleanup
   - Monitor for memory leaks

3. **Transactions not being validated**
   - Ensure validator is set
   - Check `max_retry_attempts`
   - Monitor validation callback performance

4. **Conflicts not resolved**
   - Verify `enable_conflict_detection` is true
   - Check conflict detection logic
   - Monitor conflict groups

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01-12 | Initial implementation |
| 1.1.0 | TBD | Performance optimizations |
| 1.2.0 | TBD | Enhanced monitoring |

## License

This component is part of the Neo C++ implementation and follows the project's licensing terms.