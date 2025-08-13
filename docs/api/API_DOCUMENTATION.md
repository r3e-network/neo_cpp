# Neo C++ API Documentation

## Table of Contents
1. [Core Module](#core-module)
2. [Network Module](#network-module)
3. [Ledger Module](#ledger-module)
4. [Smart Contract Module](#smart-contract-module)
5. [RPC Module](#rpc-module)
6. [Monitoring Module](#monitoring-module)

---

## Core Module

### NeoSystem

The central coordination point for the Neo blockchain implementation.

```cpp
namespace neo::core {
    class NeoSystem {
    public:
        NeoSystem(std::unique_ptr<ProtocolSettings> settings, 
                 const std::string& storage_engine);
        
        void start();
        void stop();
        
        std::shared_ptr<Blockchain> GetBlockchain() const;
        std::shared_ptr<MemoryPool> GetMemoryPool() const;
    };
}
```

#### Methods

- **`NeoSystem(settings, storage_engine)`** - Constructs a new Neo system instance
  - `settings`: Protocol configuration
  - `storage_engine`: Storage backend ("memory", "rocksdb")

- **`start()`** - Starts all system components
- **`stop()`** - Gracefully stops all components
- **`GetBlockchain()`** - Returns the blockchain instance
- **`GetMemoryPool()`** - Returns the memory pool for pending transactions

### ProtocolSettings

Configuration for the Neo protocol.

```cpp
namespace neo {
    class ProtocolSettings {
    public:
        uint32_t Magic = 0x4e454f4e;  // N3 MainNet
        uint32_t AddressVersion = 0x35;
        uint32_t MaxTransactionsPerBlock = 512;
        uint32_t MemoryPoolMaxTransactions = 50000;
        uint32_t MaxTraceableBlocks = 2102400;
        uint32_t InitialGasDistribution = 52000000;
        uint32_t MaxIteratorResultItems = 100;
    };
}
```

---

## Network Module

### ConnectionPool

Thread-safe connection pooling for network efficiency.

```cpp
namespace neo::network {
    class ConnectionPool {
    public:
        struct Config {
            size_t min_connections = 5;
            size_t max_connections = 50;
            size_t max_idle_connections = 20;
            std::chrono::seconds idle_timeout{300};
            std::chrono::seconds connection_timeout{30};
            bool enable_keep_alive = true;
        };
        
        ConnectionPool(const Config& config = Config());
        
        std::shared_ptr<TcpConnection> GetConnection(
            const std::string& host, uint16_t port);
        void ReturnConnection(std::shared_ptr<TcpConnection> connection);
        Stats GetStats() const;
    };
}
```

#### Usage Example

```cpp
ConnectionPool pool;
pool.Start();

// Get a connection
auto conn = pool.GetConnection("seed1.neo.org", 10333);
if (conn) {
    // Use connection...
    pool.ReturnConnection(conn);
}

// Or use RAII handle
{
    PooledConnectionHandle handle(
        pool.GetConnection("seed2.neo.org", 10333), &pool);
    if (handle) {
        // Connection auto-returns when handle goes out of scope
    }
}
```

### P2P Module

#### Message

Base class for P2P network messages.

```cpp
namespace neo::network::p2p {
    class Message {
    public:
        static constexpr uint32_t PayloadMaxSize = 0x02000000;
        
        MessageCommand GetCommand() const;
        MessageFlags GetFlags() const;
        io::ByteVector GetPayload() const;
        
        template<typename T>
        static Message Create(const T& payload, MessageFlags flags = None);
    };
}
```

#### RemoteNode

Represents a remote peer in the P2P network.

```cpp
namespace neo::network::p2p {
    class RemoteNode {
    public:
        virtual uint32_t GetLastBlockIndex() const;
        virtual bool IsConnected() const;
        virtual bool Send(const Message& message, 
                         bool enableCompression = true);
    };
}
```

---

## Ledger Module

### Block

Represents a block in the blockchain.

```cpp
namespace neo::ledger {
    class Block : public ISerializable {
    public:
        uint32_t GetVersion() const;
        io::UInt256 GetHash() const;
        io::UInt256 GetPreviousHash() const;
        io::UInt256 GetMerkleRoot() const;
        uint64_t GetTimestamp() const;
        uint32_t GetIndex() const;
        uint64_t GetNonce() const;
        io::UInt160 GetNextConsensus() const;
        
        const std::vector<std::shared_ptr<Transaction>>& GetTransactions() const;
        void AddTransaction(std::shared_ptr<Transaction> tx);
        
        bool Verify(const ProtocolSettings& settings) const;
    };
}
```

### Transaction

Represents a transaction.

```cpp
namespace neo::ledger {
    class Transaction : public ISerializable {
    public:
        uint8_t GetVersion() const;
        uint32_t GetNonce() const;
        int64_t GetSystemFee() const;
        int64_t GetNetworkFee() const;
        uint32_t GetValidUntilBlock() const;
        io::UInt160 GetSender() const;
        
        const std::vector<Signer>& GetSigners() const;
        const std::vector<TransactionAttribute>& GetAttributes() const;
        const std::vector<Witness>& GetWitnesses() const;
        
        io::UInt256 GetHash() const;
        bool Verify(const ProtocolSettings& settings, 
                   const DataCache& snapshot) const;
    };
}
```

### BlockchainCache

High-performance caching layer for blockchain data.

```cpp
namespace neo::ledger {
    class BlockchainCache {
    public:
        struct Config {
            size_t block_cache_size = 1000;
            size_t transaction_cache_size = 10000;
            size_t header_cache_size = 5000;
            std::chrono::seconds ttl{3600};
        };
        
        BlockchainCache(const Config& config = Config());
        
        std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const;
        std::shared_ptr<Block> GetBlock(uint32_t height) const;
        void CacheBlock(const std::shared_ptr<Block>& block);
        
        std::shared_ptr<Transaction> GetTransaction(
            const io::UInt256& hash) const;
        void CacheTransaction(const std::shared_ptr<Transaction>& tx);
        
        CacheStats GetStats() const;
    };
}
```

---

## Smart Contract Module

### ApplicationEngine

Execution engine for smart contracts.

```cpp
namespace neo::smartcontract {
    class ApplicationEngine : public vm::ExecutionEngine {
    public:
        ApplicationEngine(TriggerType trigger, 
                         IVerifiable* container,
                         DataCache* snapshot,
                         Block* persistingBlock,
                         const ProtocolSettings& settings,
                         int64_t gas);
        
        VMState Execute();
        int64_t GetGasConsumed() const;
        std::vector<Notification> GetNotifications() const;
    };
}
```

### ContractManifest

Smart contract metadata and permissions.

```cpp
namespace neo::smartcontract {
    class ContractManifest {
    public:
        std::string GetName() const;
        std::vector<ContractGroup> GetGroups() const;
        ContractFeatures GetSupportedStandards() const;
        ContractAbi GetAbi() const;
        ContractPermission GetPermissions() const;
        std::vector<std::string> GetTrusts() const;
        
        static ContractManifest Parse(const std::string& json);
        std::string ToJson() const;
    };
}
```

---

## RPC Module

### RpcServer

HTTP-based RPC server for blockchain interaction.

```cpp
namespace neo::rpc {
    class RpcServer {
    public:
        struct Settings {
            uint16_t port = 10332;
            std::string bind_address = "127.0.0.1";
            size_t max_concurrent_connections = 40;
            size_t max_request_body_size = 5 * 1024 * 1024;
            bool enable_cors = false;
        };
        
        RpcServer(std::shared_ptr<NeoSystem> system, 
                 const Settings& settings = Settings());
        
        void Start();
        void Stop();
        
        // RPC Methods
        json GetBlock(const std::string& hash_or_index, bool verbose = true);
        json GetBlockCount();
        json GetBlockHash(uint32_t index);
        json GetTransaction(const std::string& txid, bool verbose = true);
        json SendRawTransaction(const std::string& hex);
        json GetMemoryPool(bool verbose = false);
    };
}
```

### RateLimiter

Token bucket rate limiter for DoS protection.

```cpp
namespace neo::rpc {
    class RateLimiter {
    public:
        struct Config {
            size_t requests_per_second = 10;
            size_t burst_size = 20;
        };
        
        RateLimiter(const Config& config = Config());
        
        bool AllowRequest(const std::string& ip_address, 
                         const std::string& method);
    };
}
```

---

## Monitoring Module

### PerformanceMonitor

Comprehensive performance monitoring and metrics collection.

```cpp
namespace neo::monitoring {
    class PerformanceMonitor {
    public:
        static PerformanceMonitor& GetInstance();
        
        void Start();
        void Stop();
        
        void RecordOperation(const std::string& operation_name,
                           uint64_t duration_ms,
                           bool success = true);
        
        std::unique_ptr<ScopedTimer> CreateTimer(
            const std::string& operation_name);
        
        void RecordMetric(const std::string& metric_name, double value);
        
        SystemMetrics GetSystemMetrics() const;
        std::string ExportPrometheusMetrics() const;
        std::string ExportJsonMetrics() const;
    };
}
```

#### Usage Example

```cpp
// Start monitoring
auto& monitor = PerformanceMonitor::GetInstance();
monitor.Start();

// Time an operation
{
    MONITOR_OPERATION("block_validation");
    // ... perform block validation
}

// Record custom metric
RECORD_METRIC("pending_transactions", mempool.Size());

// Manual timing
auto timer = monitor.CreateTimer("custom_operation");
// ... do work
timer->Stop();

// Get metrics
auto metrics = monitor.ExportJsonMetrics();
```

### Metrics Endpoint

RPC endpoints for metrics exposure.

```cpp
namespace neo::rpc {
    class MetricsEndpoint {
    public:
        static std::string GetPrometheusMetrics();
        static std::string GetJsonMetrics();
        static std::string GetHealthStatus();
        
        template<typename RpcServer>
        static void RegisterEndpoints(RpcServer& server);
    };
}
```

#### Available Endpoints

- **GET /metrics** - Prometheus-format metrics
- **GET /metrics/json** - JSON-format metrics  
- **GET /health** - Health check status

---

## Error Handling

All modules use exceptions for error handling:

```cpp
namespace neo {
    class NeoException : public std::exception {
    public:
        explicit NeoException(const std::string& message);
        const char* what() const noexcept override;
    };
    
    class InvalidOperationException : public NeoException {};
    class ArgumentException : public NeoException {};
    class FormatException : public NeoException {};
    class NotSupportedException : public NeoException {};
}
```

---

## Best Practices

### Resource Management

1. **Use RAII** - All resources are managed with RAII patterns
2. **Smart Pointers** - Prefer `std::shared_ptr` and `std::unique_ptr`
3. **Connection Pooling** - Always use ConnectionPool for network operations
4. **Caching** - Use BlockchainCache for frequently accessed data

### Performance

1. **Monitoring** - Use PerformanceMonitor to track operations
2. **Batching** - Batch operations when possible
3. **Async Operations** - Use async methods for I/O operations
4. **Rate Limiting** - Apply rate limiting to prevent DoS

### Security

1. **Input Validation** - Always validate external input
2. **Rate Limiting** - Use RateLimiter for RPC endpoints
3. **Secure Defaults** - All configurations have secure defaults
4. **Principle of Least Privilege** - Minimal permissions by default

---

## Migration Guide

### From Neo Legacy to Neo N3

1. Update protocol settings for N3 compatibility
2. Migrate smart contracts to N3 format
3. Update RPC calls to new endpoints
4. Implement new witness system

### From Other Neo Implementations

1. Map equivalent classes and methods
2. Update serialization format
3. Adapt to C++ patterns and idioms
4. Integrate monitoring and metrics

---

## Examples

See the `/examples` directory for complete working examples:

- `simple_node.cpp` - Basic Neo node
- `rpc_client.cpp` - RPC client example
- `smart_contract.cpp` - Smart contract deployment
- `monitoring_example.cpp` - Performance monitoring setup