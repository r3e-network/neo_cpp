# State Synchronization Manager API Documentation

## Overview

The State Synchronization Manager provides comprehensive blockchain state synchronization capabilities for the Neo C++ implementation. It ensures consistency of state roots, MPT (Merkle Patricia Trie) data, and state transitions across the distributed network while maintaining full compatibility with the C# Neo reference implementation.

## Table of Contents

- [Architecture](#architecture)
- [Core Features](#core-features)
- [API Reference](#api-reference)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [Network Protocol](#network-protocol)
- [Performance Metrics](#performance-metrics)
- [Best Practices](#best-practices)

## Architecture

### Component Overview

```
┌─────────────────────────────────────┐
│   State Synchronization Manager     │
├─────────────────────────────────────┤
│ • Peer State Management             │
│ • State Root Validation             │
│ • Chunk-based Synchronization      │
│ • Consensus Calculation            │
│ • Persistence Layer                │
│ • Recovery Mechanisms              │
└─────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│        State Root System            │
├─────────────────────────────────────┤
│ • MPT Root Hash Tracking           │
│ • Witness Signatures               │
│ • Height-based Indexing           │
└─────────────────────────────────────┘
```

### Key Components

1. **StateSynchronizationManager**: Main orchestrator for state sync operations
2. **StateRoot**: Represents blockchain state at specific height
3. **StateChunk**: Batch of states for efficient transmission
4. **PeerState**: Tracks synchronization status with individual peers
5. **ValidationResult**: Contains state validation outcome and metrics

## Core Features

### Synchronization Modes

The system supports four distinct synchronization modes:

| Mode | Description | Use Case |
|------|-------------|----------|
| Full | Complete state history synchronization | Archive nodes |
| Fast | State roots with selective data | Standard nodes |
| Light | Minimal state for verification | Light clients |
| Archive | Full history with all state transitions | Historical analysis |

### Consensus Mechanism

- **Peer Agreement**: Requires 66% agreement among peers (configurable)
- **Trusted Peers**: Priority given to designated trusted peers
- **Validation**: Multi-level validation with caching

### Recovery Features

- **Automatic Recovery**: Self-healing from synchronization failures
- **Retry Logic**: Configurable retry attempts with exponential backoff
- **Stale Peer Detection**: Automatic removal of unresponsive peers

## API Reference

### Class: `StateSynchronizationManager`

#### Constructor

```cpp
explicit StateSynchronizationManager(
    const Configuration& config = Configuration{},
    std::shared_ptr<persistence::DataCache> data_cache = nullptr
)
```

Creates a new state synchronization manager.

**Parameters:**
- `config`: Configuration settings for synchronization
- `data_cache`: Optional data cache for state persistence

**Example:**
```cpp
StateSynchronizationManager::Configuration config;
config.sync_mode = SyncMode::Fast;
config.chunk_size = 1000;
auto sync_manager = std::make_unique<StateSynchronizationManager>(config);
```

#### Methods

##### `Start()`

```cpp
void Start()
```

Starts the synchronization service and background threads.

**Thread Safety:** Thread-safe

**Example:**
```cpp
sync_manager->Start();
```

##### `Stop()`

```cpp
void Stop()
```

Stops the synchronization service and cleans up resources.

**Thread Safety:** Thread-safe

##### `RequestStateSync()`

```cpp
bool RequestStateSync(
    const std::string& peer_id,
    uint32_t start_height,
    uint32_t count
)
```

Requests state synchronization from a specific peer.

**Parameters:**
- `peer_id`: Identifier of the target peer
- `start_height`: Starting block height for sync
- `count`: Number of states to synchronize

**Returns:** `true` if request was initiated successfully

**Example:**
```cpp
bool requested = sync_manager->RequestStateSync("peer_001", 1000, 500);
if (requested) {
    std::cout << "Sync requested from peer_001" << std::endl;
}
```

##### `ProcessStateRoot()`

```cpp
ValidationResult ProcessStateRoot(
    uint32_t height,
    const io::UInt256& state_root,
    const std::string& peer_id
)
```

Processes a received state root from a peer.

**Parameters:**
- `height`: Block height of the state root
- `state_root`: The state root hash
- `peer_id`: Source peer identifier

**Returns:** Validation result with consensus information

##### `ProcessStateChunk()`

```cpp
bool ProcessStateChunk(
    const StateChunk& chunk,
    const std::string& peer_id
)
```

Processes a batch of states received from a peer.

**Parameters:**
- `chunk`: State chunk containing multiple states
- `peer_id`: Source peer identifier

**Returns:** `true` if chunk was processed successfully

##### `ValidateStateAtHeight()`

```cpp
ValidationResult ValidateStateAtHeight(uint32_t height)
```

Validates the state at a specific block height.

**Parameters:**
- `height`: Block height to validate

**Returns:** Validation result with expected vs actual comparison

##### `GetSyncStatus()`

```cpp
SyncStatus GetSyncStatus() const
```

Gets the current synchronization status.

**Returns:** Current status (Idle, Syncing, Validating, Synchronized, Failed, Recovering)

##### `GetStatistics()`

```cpp
SyncStats GetStatistics() const
```

Gets comprehensive synchronization statistics.

**Returns:** Statistics including progress, validation counts, and timing

**Example:**
```cpp
auto stats = sync_manager->GetStatistics();
std::cout << "Sync progress: " << stats.sync_progress_percent << "%" << std::endl;
std::cout << "States validated: " << stats.states_validated << std::endl;
```

##### `AddTrustedPeer()`

```cpp
void AddTrustedPeer(const std::string& peer_id)
```

Adds a peer to the trusted peer list.

**Parameters:**
- `peer_id`: Peer identifier to trust

##### `RemoveTrustedPeer()`

```cpp
void RemoveTrustedPeer(const std::string& peer_id)
```

Removes a peer from the trusted peer list.

**Parameters:**
- `peer_id`: Peer identifier to remove

##### `IsSynchronized()`

```cpp
bool IsSynchronized() const
```

Checks if the node is fully synchronized.

**Returns:** `true` if synchronized

##### `ForceValidation()`

```cpp
size_t ForceValidation(uint32_t start_height, uint32_t end_height)
```

Forces validation of states in a height range.

**Parameters:**
- `start_height`: Starting height
- `end_height`: Ending height

**Returns:** Number of states validated

##### `Reset()`

```cpp
void Reset(bool clear_cache = false)
```

Resets the synchronization state.

**Parameters:**
- `clear_cache`: Whether to clear the validation cache

### Callback System

#### Setting Callbacks

```cpp
// State validated callback
sync_manager->SetOnStateValidated(
    [](uint32_t height, const io::UInt256& root) {
        std::cout << "State validated at height " << height << std::endl;
    });

// Validation failed callback
sync_manager->SetOnValidationFailed(
    [](const ValidationResult& result) {
        std::cerr << "Validation failed: " << result.error_message << std::endl;
    });

// Progress update callback
sync_manager->SetOnSyncProgress(
    [](const SyncStats& stats) {
        std::cout << "Progress: " << stats.sync_progress_percent << "%" << std::endl;
    });

// Status change callback
sync_manager->SetOnStatusChanged(
    [](SyncStatus status) {
        std::cout << "Status changed to: " << static_cast<int>(status) << std::endl;
    });
```

## Configuration

### Configuration Structure

```cpp
struct Configuration {
    SyncMode sync_mode = SyncMode::Fast;
    uint32_t chunk_size = 1000;
    uint32_t max_concurrent_chunks = 5;
    std::chrono::seconds sync_interval{30};
    std::chrono::seconds peer_timeout{60};
    std::chrono::seconds validation_timeout{10};
    uint32_t max_retry_attempts = 3;
    bool enable_parallel_validation = true;
    bool enable_state_persistence = true;
    bool enable_auto_recovery = true;
    size_t max_state_cache_size = 10000;
    double min_peer_agreement = 0.66;
};
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `sync_mode` | Fast | Synchronization mode |
| `chunk_size` | 1000 | States per chunk |
| `max_concurrent_chunks` | 5 | Maximum parallel chunks |
| `sync_interval` | 30s | Sync cycle interval |
| `peer_timeout` | 60s | Peer timeout duration |
| `validation_timeout` | 10s | Validation timeout |
| `max_retry_attempts` | 3 | Maximum retries |
| `enable_parallel_validation` | true | Parallel validation |
| `enable_state_persistence` | true | Persist states |
| `enable_auto_recovery` | true | Auto-recovery |
| `max_state_cache_size` | 10000 | Cache size limit |
| `min_peer_agreement` | 0.66 | Consensus threshold |

## Usage Examples

### Basic Synchronization

```cpp
#include <neo/ledger/state_synchronization_manager.h>

// Configure sync manager
StateSynchronizationManager::Configuration config;
config.sync_mode = StateSynchronizationManager::SyncMode::Fast;
config.chunk_size = 500;

// Create with data cache for persistence
auto data_cache = std::make_shared<persistence::DataCache>();
auto sync_manager = std::make_unique<StateSynchronizationManager>(config, data_cache);

// Set up callbacks
sync_manager->SetOnSyncProgress([](const auto& stats) {
    std::cout << "Synced: " << stats.current_height 
              << "/" << stats.target_height << std::endl;
});

// Start synchronization
sync_manager->Start();

// Add trusted peers
sync_manager->AddTrustedPeer("node1.neo.org");
sync_manager->AddTrustedPeer("node2.neo.org");

// Request initial sync
sync_manager->RequestStateSync("node1.neo.org", 0, 1000);
```

### Advanced Monitoring

```cpp
// Monitor synchronization with detailed callbacks
sync_manager->SetOnStateValidated(
    [](uint32_t height, const io::UInt256& root) {
        // Log successful validation
        Logger::Info("Validated state at height " + std::to_string(height));
    });

sync_manager->SetOnValidationFailed(
    [](const ValidationResult& result) {
        // Handle validation failure
        Logger::Error("Validation failed at height " + 
                     std::to_string(result.validation_height) +
                     ": " + result.error_message);
        
        // Trigger alert if critical height
        if (result.validation_height % 1000 == 0) {
            AlertSystem::Trigger("Critical validation failure");
        }
    });

// Periodic statistics logging
std::thread monitor_thread([&sync_manager]() {
    while (sync_manager->GetSyncStatus() != SyncStatus::Idle) {
        auto stats = sync_manager->GetStatistics();
        
        Logger::Info("Sync Statistics:");
        Logger::Info("  Progress: " + std::to_string(stats.sync_progress_percent) + "%");
        Logger::Info("  Validated: " + std::to_string(stats.states_validated));
        Logger::Info("  Failed: " + std::to_string(stats.states_failed));
        Logger::Info("  Peers: " + std::to_string(stats.peer_count));
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
});
```

### Handling Network Events

```cpp
// Process incoming state root from network
void HandleStateRootMessage(const StateRootPayload& payload, const std::string& peer_id) {
    auto result = sync_manager->ProcessStateRoot(
        payload.GetStateRoot().GetIndex(),
        payload.GetStateRoot().GetRoot(),
        peer_id
    );
    
    if (!result.is_valid) {
        // Handle invalid state
        PeerManager::DecreaseTrust(peer_id);
    }
}

// Process state chunk from network
void HandleStateChunkMessage(const StateChunk& chunk, const std::string& peer_id) {
    if (!sync_manager->ProcessStateChunk(chunk, peer_id)) {
        // Chunk processing failed
        Logger::Warning("Failed to process chunk from " + peer_id);
        
        // Request chunk from different peer
        auto peers = sync_manager->GetAllPeerStates();
        for (const auto& [other_peer_id, state] : peers) {
            if (other_peer_id != peer_id && state.is_trusted) {
                sync_manager->RequestStateSync(
                    other_peer_id, 
                    chunk.start_height, 
                    chunk.end_height - chunk.start_height + 1
                );
                break;
            }
        }
    }
}
```

## Network Protocol

### Message Types

#### GetStateRoots Request
```cpp
struct GetStateRootsPayload {
    uint32_t request_id;
    uint32_t start_height;
    uint32_t count;
};
```

#### StateRoot Response
```cpp
struct StateRootPayload {
    StateRoot state_root;
    uint32_t request_id;
    bool is_response;
};
```

### Protocol Flow

1. **Discovery Phase**
   - Node broadcasts state height to peers
   - Peers respond with their current state

2. **Synchronization Phase**
   - Request state chunks from selected peers
   - Process and validate received chunks
   - Update local state

3. **Validation Phase**
   - Calculate consensus among peers
   - Validate state roots
   - Persist validated states

## Performance Metrics

### Benchmarks

| Operation | Average Time | Throughput |
|-----------|-------------|------------|
| Process State Root | < 10ms | 100/sec |
| Process State Chunk (1000 states) | < 500ms | 2000 states/sec |
| Validate State | < 5ms | 200/sec |
| Consensus Calculation | < 20ms | 50/sec |
| Persistence (per state) | < 2ms | 500/sec |

### Memory Usage

| Component | Memory Usage |
|-----------|-------------|
| Per Peer State | ~1 KB |
| Per State Root | ~100 bytes |
| Validation Cache (10K entries) | ~1 MB |
| State Chunk (1000 states) | ~100 KB |

## Best Practices

### 1. Configuration Tuning

```cpp
// For fast sync on high-bandwidth network
config.chunk_size = 5000;
config.max_concurrent_chunks = 10;
config.sync_interval = std::chrono::seconds(10);

// For limited bandwidth
config.chunk_size = 100;
config.max_concurrent_chunks = 2;
config.sync_interval = std::chrono::seconds(60);

// For archive node
config.sync_mode = SyncMode::Archive;
config.enable_state_persistence = true;
config.max_state_cache_size = 100000;
```

### 2. Error Handling

```cpp
// Robust error handling
sync_manager->SetOnValidationFailed(
    [&sync_manager](const ValidationResult& result) {
        // Log detailed error
        LogValidationError(result);
        
        // Attempt recovery if critical
        if (result.validation_height < GetCurrentBlockHeight() - 100) {
            sync_manager->ForceValidation(
                result.validation_height - 10,
                result.validation_height + 10
            );
        }
        
        // Reset if too many failures
        static int failure_count = 0;
        if (++failure_count > 100) {
            sync_manager->Reset(true);
            failure_count = 0;
        }
    });
```

### 3. Peer Management

```cpp
// Dynamic peer trust management
class PeerTrustManager {
    std::unordered_map<std::string, int> trust_scores_;
    
    void UpdateTrust(const std::string& peer_id, bool success) {
        if (success) {
            trust_scores_[peer_id]++;
        } else {
            trust_scores_[peer_id]--;
        }
        
        // Update sync manager
        if (trust_scores_[peer_id] > 10) {
            sync_manager->AddTrustedPeer(peer_id);
        } else if (trust_scores_[peer_id] < -5) {
            sync_manager->RemoveTrustedPeer(peer_id);
        }
    }
};
```

### 4. Monitoring and Alerting

```cpp
// Comprehensive monitoring
void MonitorSynchronization(StateSynchronizationManager* manager) {
    auto stats = manager->GetStatistics();
    
    // Check sync progress
    if (stats.sync_progress_percent < 50 && 
        std::chrono::steady_clock::now() - stats.sync_start_time > std::chrono::hours(1)) {
        Alert("Slow synchronization detected");
    }
    
    // Check validation rate
    double validation_rate = static_cast<double>(stats.states_validated) / 
                           stats.states_processed;
    if (validation_rate < 0.95) {
        Alert("High validation failure rate: " + std::to_string(validation_rate));
    }
    
    // Check peer availability
    if (stats.peer_count < 3) {
        Alert("Insufficient peers for consensus");
    }
}
```

## Thread Safety

The State Synchronization Manager is fully thread-safe:
- All public methods can be called concurrently
- Internal synchronization uses read-write locks
- Atomic operations for counters and status

## Integration with Blockchain

### Block Processing Integration

```cpp
void ProcessNewBlock(const Block& block) {
    // Update state root
    auto state_root = CalculateStateRoot(block);
    
    // Validate with sync manager
    auto result = sync_manager->ProcessStateRoot(
        block.GetIndex(),
        state_root,
        "local"
    );
    
    if (!result.is_valid) {
        // State mismatch - investigate
        RequestStateResync(block.GetIndex());
    }
}
```

### Consensus Integration

```cpp
bool ValidateConsensusState(uint32_t height) {
    auto state_root = sync_manager->GetStateRootAtHeight(height);
    if (!state_root.has_value()) {
        return false;
    }
    
    // Verify with consensus nodes
    return VerifyWithConsensusNodes(state_root.value());
}
```

## Troubleshooting

### Common Issues

1. **Slow Synchronization**
   - Increase `chunk_size` and `max_concurrent_chunks`
   - Reduce `sync_interval`
   - Add more trusted peers

2. **High Validation Failures**
   - Check network connectivity
   - Verify peer trust settings
   - Increase `min_peer_agreement` threshold

3. **Memory Usage**
   - Reduce `max_state_cache_size`
   - Disable `enable_state_persistence` if not needed
   - Decrease `chunk_size`

4. **Consensus Failures**
   - Ensure minimum 3 peers available
   - Check peer state consistency
   - Verify time synchronization

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01-12 | Initial implementation |
| 1.1.0 | TBD | Performance optimizations |
| 1.2.0 | TBD | Enhanced recovery mechanisms |

## License

This component is part of the Neo C++ implementation and follows the project's licensing terms.