# C# Neo Node Compatibility Guide

## Overview

This document outlines the compatibility features implemented in the Neo C++ implementation to ensure consistency with the official C# Neo node.

## Transaction Pool Architecture

### Core Design Principles

The Transaction Pool Manager has been designed to maintain compatibility with the C# Neo implementation while providing additional monitoring and optimization features.

### Memory Pool Structure (C# Compatible)

The implementation follows the C# Neo three-pool architecture:

1. **Sorted Pool** (`sorted_transactions_`)
   - Contains verified transactions sorted by fee per byte
   - Matches C# `SortedSet<PoolItem>` behavior
   - Used for block creation with priority ordering

2. **Unsorted Pool** (`unsorted_transactions_`)
   - Quick lookup dictionary for verified transactions
   - Matches C# `Dictionary<UInt256, PoolItem>` structure
   - Provides O(1) transaction retrieval

3. **Unverified Pool** (`unverified_transactions_`)
   - Holds transactions awaiting verification
   - Matches C# `Dictionary<UInt256, PoolItem>` structure
   - Supports reverification with retry limits

### Key Compatibility Features

#### 1. PoolItem Structure
```cpp
class PoolItem {
    std::shared_ptr<Transaction> transaction_;
    std::chrono::system_clock::time_point timestamp_;
    uint64_t fee_per_byte_;  // C# priority metric
    io::UInt256 hash_;
};
```

#### 2. Event System
- Static event system matching C# pattern
- `TransactionRemovedEventArgs` with reason enum
- Event firing through `MemoryPoolEvents` static class

#### 3. Transaction Verification Flow
```cpp
// C# pattern: ReverifyTransactions
memory_pool_->ReverifyTransactions(max_count);

// Move from unverified to verified
memory_pool_->MoveToVerified(item);
```

#### 4. Priority Calculation
- Fee per byte as primary ordering metric
- Higher fee per byte = higher priority
- Consistent with C# `CompareTo` implementation

## RPC Interface Compatibility

### Mempool RPC Methods

#### `getrawmempool`
Returns array of transaction hashes in the memory pool.

**C# Response Format:**
```json
[
  "0xhash1...",
  "0xhash2...",
  "0xhash3..."
]
```

#### `getmempoolinfo`
Returns detailed memory pool statistics.

**C# Response Format:**
```json
{
  "size": 150,
  "bytes": 163270,
  "usage": 163270,
  "total_fee": 15000000,
  "minfee": 1000000,
  "maxmempool": 50000,
  "unverified": 10,
  "verified": 140
}
```

### Standard RPC Methods

All standard Neo RPC methods are implemented following the C# specification:

- `getblockcount` - Current block height
- `getbestblockhash` - Latest block hash
- `getblock` - Block information by hash/index
- `getconnectioncount` - Connected peer count
- `getversion` - Node version information

## Data Structure Alignment

### Transaction Structure
```cpp
class Neo3Transaction {
    // C# compatible fields
    uint8_t version_;
    uint32_t nonce_;
    int64_t system_fee_;
    int64_t network_fee_;
    uint32_t valid_until_block_;
    std::vector<Signer> signers_;
    std::vector<TransactionAttribute> attributes_;
    std::vector<byte> script_;
    std::vector<Witness> witnesses_;
};
```

### Block Structure
```cpp
class Block : public BlockHeader {
    // C# compatible structure
    std::vector<Transaction> transactions_;
    // ConsensusData handled in header
};
```

## Transaction Processing Flow

### C# Compatible Processing Steps

1. **Reception**
   - Transaction received from network or RPC
   - Initial validation (size, format)
   - Add to unverified pool

2. **Verification**
   - Script verification
   - Signature validation
   - Balance checks
   - Move to verified pool on success

3. **Prioritization**
   - Calculate fee per byte
   - Sort by priority in sorted pool
   - Ready for block inclusion

4. **Block Creation**
   - Select highest priority transactions
   - Respect size and count limits
   - Ensure dependency ordering

5. **Cleanup**
   - Remove expired transactions
   - Evict low-priority when full
   - Handle conflicts (double-spend)

## Configuration Compatibility

### Default Settings (C# Neo)

```cpp
Configuration {
    max_pool_size = 50000;        // MemoryPoolMaxTransactions
    max_unverified_size = 5000;   // C# default
    transaction_timeout = 300s;    // 5 minutes
    max_transaction_size = 102400; // 100KB
    cleanup_interval = 60s;        // Periodic maintenance
}
```

## Event System Compatibility

### C# Event Pattern Implementation

```cpp
// C# pattern: Static event system
class MemoryPoolEvents {
    static event<TransactionAddedHandler> TransactionAdded;
    static event<TransactionRemovedHandler> TransactionRemoved;
};

// Event args matching C#
struct TransactionRemovedEventArgs {
    enum class Reason {
        Expired,
        LowPriority,
        Replaced,
        InvalidTransaction,
        InsufficientFunds,
        PolicyViolation,
        Included
    };
};
```

## Network Protocol Compatibility

### P2P Message Handling

- `inv` - Inventory message with transaction hashes
- `getdata` - Request for transaction data
- `tx` - Transaction broadcast
- `mempool` - Request for memory pool contents

### Message Format
All messages follow the C# Neo binary serialization format for network compatibility.

## Performance Optimizations (Beyond C#)

While maintaining C# compatibility, the C++ implementation adds:

1. **Additional Monitoring Layer**
   - Transaction metadata tracking
   - Source peer tracking
   - Detailed statistics collection

2. **Priority Queue Optimization**
   - Optional priority queue for O(log n) insertion
   - Faster block creation with pre-sorted transactions

3. **Conflict Detection Engine**
   - Automatic double-spend detection
   - Fee-based conflict resolution

4. **Callback System**
   - Transaction lifecycle callbacks
   - Statistics update notifications
   - Validation hooks

## Testing Compatibility

### Test Coverage

- Unit tests verify C# behavior compatibility
- Integration tests with C# nodes
- RPC response format validation
- Network message compatibility tests

### Compatibility Test Suite

```cpp
TEST(Compatibility, MemoryPoolBehavior) {
    // Verify sorted pool ordering matches C#
    // Verify eviction behavior
    // Verify reverification process
}

TEST(Compatibility, RpcResponses) {
    // Verify JSON format matches C#
    // Verify field names and types
    // Verify error codes
}
```

## Migration Guide

### From C# to C++

1. **Configuration**
   - Map C# config values to C++ Configuration struct
   - Adjust paths and logging settings

2. **RPC Clients**
   - No changes needed - same API
   - Same request/response format

3. **Network Peers**
   - Fully compatible P2P protocol
   - No changes to network configuration

## Known Differences

### Intentional Enhancements

1. **Monitoring**
   - Additional statistics not in C#
   - Performance metrics collection
   - Detailed transaction metadata

2. **Callbacks**
   - Extended callback system
   - More granular event notifications

3. **Configuration**
   - Additional tuning parameters
   - Performance optimization options

### Implementation Details

1. **Threading Model**
   - C++ uses std::thread vs C# Task
   - Different async patterns
   - Same external behavior

2. **Memory Management**
   - C++ explicit memory management
   - Smart pointers vs C# GC
   - Same memory usage patterns

## Compliance Verification

### Checklist

- [x] Three-pool architecture (sorted/unsorted/unverified)
- [x] Fee per byte priority ordering
- [x] Transaction reverification process
- [x] Event system with static events
- [x] RPC method compatibility
- [x] Network message format
- [x] Configuration defaults
- [x] Error codes and responses
- [x] Transaction lifecycle
- [x] Block creation process

## Future Compatibility Work

### Planned Enhancements

1. **State Management**
   - MPT (Merkle Patricia Trie) compatibility
   - State root calculation

2. **Consensus**
   - dBFT consensus message handling
   - Validator behavior compatibility

3. **Smart Contracts**
   - NeoVM execution compatibility
   - System contract interfaces

## References

- [Neo C# Implementation](https://github.com/neo-project/neo)
- [Neo Protocol Documentation](https://docs.neo.org)
- [Neo Improvement Proposals](https://github.com/neo-project/proposals)