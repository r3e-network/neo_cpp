# Neo C++ Complete API Reference

## Version: 1.2.0
## Status: Production Ready

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Core API](#core-api)
3. [Network API](#network-api)
4. [Ledger API](#ledger-api)
5. [Smart Contract API](#smart-contract-api)
6. [RPC API](#rpc-api)
7. [Monitoring API](#monitoring-api)
8. [Wallet API](#wallet-api)
9. [Consensus API](#consensus-api)
10. [Error Handling](#error-handling)
11. [Performance Optimization](#performance-optimization)
12. [Security](#security)

---

## Getting Started

### Installation

```bash
# Clone the repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp

# Build the project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# Run tests
ctest --output-on-failure
```

### Basic Usage

```cpp
#include <neo/core/neo_system.h>
#include <neo/network/p2p_server.h>
#include <neo/rpc/rpc_server.h>

int main() {
    // Initialize Neo system
    neo::core::SystemConfig config;
    config.network_id = 860833102;  // N3 MainNet
    config.data_path = "./data";
    
    auto system = std::make_unique<neo::core::NeoSystem>(config);
    
    // Start P2P networking
    neo::network::P2PConfig p2p_config;
    p2p_config.port = 10333;
    auto p2p = std::make_unique<neo::network::P2PServer>(p2p_config);
    
    // Start RPC server
    neo::rpc::RpcConfig rpc_config;
    rpc_config.port = 10332;
    auto rpc = std::make_unique<neo::rpc::RpcServer>(rpc_config);
    
    // Start services
    system->Start();
    p2p->Start();
    rpc->Start();
    
    // Run until shutdown
    system->WaitForShutdown();
    
    return 0;
}
```

---

## Core API

### NeoSystem

The central coordination point for the Neo blockchain.

```cpp
namespace neo::core {

class NeoSystem {
public:
    struct SystemConfig {
        uint32_t network_id = 860833102;      // Network identifier
        std::string data_path = "./data";     // Data storage path
        size_t max_memory_pool_size = 50000;  // Max mempool transactions
        uint32_t max_block_size = 262144;     // Max block size in bytes
        uint32_t max_block_system_fee = 900000000000; // Max system fee per block
    };
    
    explicit NeoSystem(const SystemConfig& config);
    ~NeoSystem();
    
    // Lifecycle management
    void Start();
    void Shutdown();
    bool IsRunning() const;
    void WaitForShutdown();
    
    // Component access
    std::shared_ptr<ledger::Blockchain> GetBlockchain() const;
    std::shared_ptr<ledger::MemoryPool> GetMemoryPool() const;
    std::shared_ptr<consensus::ConsensusService> GetConsensus() const;
    
    // System information
    uint32_t GetNetworkId() const;
    std::string GetDataPath() const;
    Version GetVersion() const;
    
    // Event handling
    using BlockAddedHandler = std::function<void(const ledger::Block&)>;
    void OnBlockAdded(BlockAddedHandler handler);
};

} // namespace neo::core
```

### Error Handling Framework

```cpp
namespace neo::error {

// Result type for operations that can fail
template<typename T, typename E = NeoException>
class Result {
public:
    bool IsSuccess() const;
    bool IsError() const;
    
    T& Value();  // Throws if error
    E& Error();  // Throws if success
    
    T ValueOr(const T& default_value) const;
    
    // Monadic operations
    template<typename F>
    auto Map(F&& func);
    
    template<typename F>
    auto AndThen(F&& func);
};

// Error codes
enum class ErrorCode {
    Success = 0,
    InvalidArgument = 2,
    NetworkTimeout = 1000,
    InvalidBlock = 2001,
    InvalidTransaction = 4001,
    ContractExecutionFailed = 6000
};

// Exception class
class NeoException : public std::exception {
public:
    NeoException(ErrorCode code, const std::string& message);
    ErrorCode code() const;
};

} // namespace neo::error
```

---

## Network API

### Connection Pool

High-performance connection pooling for network operations.

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
    
    explicit ConnectionPool(const Config& config = Config());
    
    // Connection management
    std::shared_ptr<TcpConnection> GetConnection(
        const std::string& host, 
        uint16_t port);
    void ReturnConnection(std::shared_ptr<TcpConnection> connection);
    
    // Pool management
    void Start();
    void Stop();
    void HealthCheck();
    
    // Statistics
    struct Stats {
        size_t total_connections;
        size_t active_connections;
        size_t idle_connections;
        size_t failed_connections;
        size_t reused_connections;
        uint64_t total_bytes_sent;
        uint64_t total_bytes_received;
    };
    Stats GetStats() const;
};

// RAII connection handle
class PooledConnectionHandle {
public:
    PooledConnectionHandle(std::shared_ptr<TcpConnection> conn, 
                          ConnectionPool* pool);
    ~PooledConnectionHandle();  // Auto-returns to pool
    
    TcpConnection* operator->() const;
    explicit operator bool() const;
    void Return();  // Manual return
};

} // namespace neo::network
```

### P2P Server

Peer-to-peer networking implementation.

```cpp
namespace neo::network {

class P2PServer {
public:
    struct P2PConfig {
        uint16_t port = 10333;
        size_t max_peers = 100;
        size_t min_desired_peers = 10;
        std::chrono::seconds peer_timeout{60};
        bool enable_upnp = false;
    };
    
    explicit P2PServer(const P2PConfig& config);
    
    // Server lifecycle
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Peer management
    void ConnectToPeer(const std::string& host, uint16_t port);
    void DisconnectPeer(const std::string& peer_id);
    size_t GetPeerCount() const;
    std::vector<PeerInfo> GetPeers() const;
    
    // Message handling
    void BroadcastMessage(const Message& message);
    void SendMessage(const std::string& peer_id, const Message& message);
    
    // Event handlers
    using MessageHandler = std::function<void(const std::string&, const Message&)>;
    void OnMessage(MessageType type, MessageHandler handler);
};

} // namespace neo::network
```

---

## Ledger API

### Blockchain

Core blockchain data structure and operations.

```cpp
namespace neo::ledger {

class Blockchain {
public:
    // Block operations
    error::Result<void> AddBlock(const Block& block);
    std::shared_ptr<Block> GetBlock(uint32_t height) const;
    std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const;
    uint32_t GetHeight() const;
    
    // Transaction operations
    std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;
    bool ContainsTransaction(const io::UInt256& hash) const;
    
    // State queries
    io::UInt256 GetCurrentHeaderHash() const;
    std::shared_ptr<Block> GetCurrentBlock() const;
    
    // Validation
    error::Result<void> ValidateBlock(const Block& block) const;
    error::Result<void> ValidateTransaction(const Transaction& tx) const;
};

} // namespace neo::ledger
```

### Blockchain Cache

High-performance caching layer for blockchain data.

```cpp
namespace neo::ledger {

class BlockchainCache {
public:
    struct Config {
        size_t block_cache_size = 1000;
        size_t transaction_cache_size = 10000;
        size_t header_cache_size = 5000;
        size_t contract_cache_size = 500;
        std::chrono::seconds ttl{3600};
        bool enable_metrics = true;
    };
    
    explicit BlockchainCache(const Config& config = Config());
    
    // Cache operations
    std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const;
    std::shared_ptr<Block> GetBlock(uint32_t height) const;
    void CacheBlock(const std::shared_ptr<Block>& block);
    
    std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;
    void CacheTransaction(const std::shared_ptr<Transaction>& tx);
    
    // Cache management
    void Clear();
    void WarmCache(const std::vector<std::shared_ptr<Block>>& blocks);
    
    // Statistics
    struct CacheStats {
        double hit_rate;
        uint64_t total_requests;
        uint64_t cache_hits;
        uint64_t cache_misses;
        std::chrono::seconds uptime;
    };
    CacheStats GetStats() const;
};

} // namespace neo::ledger
```

### Memory Pool

Transaction memory pool for pending transactions.

```cpp
namespace neo::ledger {

class MemoryPool {
public:
    MemoryPool(size_t capacity, size_t payload_max_size);
    
    // Transaction management
    bool TryAdd(const Transaction& tx);
    bool Remove(const io::UInt256& hash);
    std::optional<PoolItem> Get(const io::UInt256& hash) const;
    bool Contains(const io::UInt256& hash) const;
    
    // Pool queries
    size_t GetCount() const;
    std::vector<Transaction> GetSortedTransactions() const;
    std::vector<io::UInt256> GetTransactionHashes() const;
    
    // Pool management
    void Clear();
    void UpdatePoolForBlockPersisted(const Block& block);
    
    // Verification
    VerifyResult GetVerificationResult(const io::UInt256& hash) const;
    void UpdateVerificationResult(const io::UInt256& hash, VerifyResult result);
};

} // namespace neo::ledger
```

---

## Smart Contract API

### Contract Execution

```cpp
namespace neo::smartcontract {

class ApplicationEngine {
public:
    ApplicationEngine(TriggerType trigger, 
                     const IVerifiable& container,
                     const DataCache& snapshot,
                     const Block& persistingBlock,
                     int64_t gas);
    
    // Execution
    VMState Execute();
    void LoadScript(const Script& script);
    
    // Gas management
    int64_t GetGasConsumed() const;
    void AddGas(int64_t gas);
    
    // Stack operations
    void Push(const StackItem& item);
    StackItem Pop();
    
    // System calls
    void System_Contract_Call();
    void System_Storage_Get();
    void System_Storage_Put();
    
    // Events
    using NotificationHandler = std::function<void(const Notification&)>;
    void OnNotification(NotificationHandler handler);
};

} // namespace neo::smartcontract
```

### Native Contracts

```cpp
namespace neo::smartcontract::native {

class NeoToken : public NativeContract {
public:
    static constexpr auto Name = "NeoToken";
    static constexpr auto Symbol = "NEO";
    static constexpr uint8_t Decimals = 0;
    
    BigInteger TotalSupply(const DataCache& snapshot) const;
    BigInteger BalanceOf(const DataCache& snapshot, const UInt160& account) const;
    bool Transfer(ApplicationEngine& engine, const UInt160& from, 
                 const UInt160& to, const BigInteger& amount);
    
    // Governance
    bool Vote(ApplicationEngine& engine, const UInt160& account, 
             const ECPoint& voteTo);
    std::vector<ECPoint> GetCandidates(const DataCache& snapshot) const;
};

class GasToken : public NativeContract {
public:
    static constexpr auto Name = "GasToken";
    static constexpr auto Symbol = "GAS";
    static constexpr uint8_t Decimals = 8;
    
    BigInteger TotalSupply(const DataCache& snapshot) const;
    BigInteger BalanceOf(const DataCache& snapshot, const UInt160& account) const;
    bool Transfer(ApplicationEngine& engine, const UInt160& from,
                 const UInt160& to, const BigInteger& amount);
};

} // namespace neo::smartcontract::native
```

---

## RPC API

### RPC Server

JSON-RPC 2.0 server implementation.

```cpp
namespace neo::rpc {

class RpcServer {
public:
    struct RpcConfig {
        uint16_t port = 10332;
        std::string bind_address = "0.0.0.0";
        size_t max_concurrent_requests = 100;
        std::chrono::seconds request_timeout{30};
        bool enable_cors = true;
        std::vector<std::string> allowed_origins{"*"};
    };
    
    explicit RpcServer(const RpcConfig& config);
    
    // Server lifecycle
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Method registration
    using MethodHandler = std::function<json(const json&)>;
    void RegisterMethod(const std::string& name, MethodHandler handler);
    
    // Built-in methods
    json GetBlockCount();
    json GetBlock(const json& params);
    json GetTransaction(const json& params);
    json SendRawTransaction(const json& params);
    json GetConnectionCount();
    json GetPeers();
    json GetVersion();
    json GetApplicationLog(const json& params);
};

} // namespace neo::rpc
```

### RPC Methods

Standard RPC methods implementation.

```cpp
// Block methods
getblockcount() -> number
getblock(hash/height, verbose) -> Block
getblockheader(hash/height, verbose) -> Header
getblockhash(height) -> hash

// Transaction methods
getrawtransaction(txid, verbose) -> Transaction
sendrawtransaction(hex) -> hash
gettransactionheight(txid) -> height

// Contract methods
invokefunction(scripthash, method, params) -> Result
invokescript(script) -> Result
getcontractstate(scripthash) -> ContractState

// Wallet methods (when wallet is open)
getnewaddress() -> address
getbalance(asset_id) -> Balance
sendfrom(asset_id, from, to, amount) -> Transaction
sendtoaddress(asset_id, address, amount) -> Transaction
sendmany(transfers) -> Transaction

// Node methods
getconnectioncount() -> number
getpeers() -> PeerList
getversion() -> Version

// Plugin methods
getapplicationlog(txid) -> ApplicationLog
getnep17balances(address) -> Balances
getnep17transfers(address, timestamp) -> Transfers
```

---

## Monitoring API

### Performance Monitor

Comprehensive performance monitoring system.

```cpp
namespace neo::monitoring {

class PerformanceMonitor {
public:
    struct Config {
        bool enable_cpu_monitoring = true;
        bool enable_memory_monitoring = true;
        bool enable_disk_monitoring = true;
        bool enable_network_monitoring = true;
        std::chrono::seconds collection_interval{10};
        size_t history_size = 1000;
    };
    
    explicit PerformanceMonitor(const Config& config = Config());
    
    // Monitoring control
    void Start();
    void Stop();
    
    // Metric recording
    void RecordOperation(const std::string& operation, double duration_ms);
    void RecordEvent(const std::string& event, const json& metadata = {});
    void IncrementCounter(const std::string& counter, int64_t value = 1);
    void SetGauge(const std::string& gauge, double value);
    
    // Alert management
    void SetAlertThreshold(const std::string& metric, double threshold);
    std::vector<Alert> GetActiveAlerts() const;
    
    // Metrics retrieval
    struct Metrics {
        double cpu_usage_percent;
        double memory_usage_mb;
        double disk_usage_gb;
        double network_bandwidth_mbps;
        uint64_t total_requests;
        double average_response_time_ms;
        std::chrono::seconds uptime_seconds;
    };
    Metrics GetMetrics() const;
    
    // Export formats
    json ExportPrometheus() const;
    json ExportJSON() const;
    std::string ExportCSV() const;
};

} // namespace neo::monitoring
```

---

## Wallet API

### Wallet Management

```cpp
namespace neo::wallets {

class Wallet {
public:
    Wallet(const std::string& path, const std::string& password);
    
    // Account management
    WalletAccount CreateAccount();
    WalletAccount CreateAccount(const PrivateKey& key);
    WalletAccount GetAccount(const UInt160& script_hash);
    std::vector<WalletAccount> GetAccounts() const;
    bool DeleteAccount(const UInt160& script_hash);
    
    // Balance queries
    BigInteger GetBalance(const UInt160& asset_id, 
                         const UInt160& account) const;
    std::vector<Coin> GetCoins() const;
    
    // Transaction creation
    Transaction MakeTransaction(const std::vector<TransferOutput>& outputs);
    
    // Wallet operations
    void Save();
    void ChangePassword(const std::string& old_password, 
                       const std::string& new_password);
    bool VerifyPassword(const std::string& password) const;
};

class NEP6Wallet : public Wallet {
public:
    struct ScryptParameters {
        int n = 16384;
        int r = 8;
        int p = 8;
    };
    
    NEP6Wallet(const std::string& path, 
              const std::string& name,
              const ScryptParameters& scrypt = ScryptParameters());
    
    // NEP-6 specific
    json Export() const;
    static std::unique_ptr<NEP6Wallet> Import(const json& wallet_json,
                                              const std::string& password);
};

} // namespace neo::wallets
```

---

## Consensus API

### Consensus Service

dBFT 2.0 consensus implementation.

```cpp
namespace neo::consensus {

class ConsensusService {
public:
    ConsensusService(NeoSystem& system, Wallet& wallet);
    
    // Consensus control
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Validator management
    void SetValidatorIndex(int index);
    int GetValidatorIndex() const;
    bool IsValidator() const;
    bool IsPrimary() const;
    
    // Consensus state
    uint32_t GetViewNumber() const;
    ConsensusState GetState() const;
    
    // Message handling
    void OnConsensusMessage(const ConsensusMessage& message);
    void BroadcastMessage(const ConsensusMessage& message);
    
    // Events
    using BlockGeneratedHandler = std::function<void(const Block&)>;
    void OnBlockGenerated(BlockGeneratedHandler handler);
};

} // namespace neo::consensus
```

---

## Performance Optimization

### Best Practices

```cpp
// Use connection pooling for network operations
auto pool = std::make_unique<ConnectionPool>(config);
auto conn = pool->GetConnection(host, port);  // Reuses existing connections

// Enable blockchain caching
BlockchainCache::Config cache_config;
cache_config.block_cache_size = 1000;
auto cache = std::make_unique<BlockchainCache>(cache_config);

// Use Result types for error handling
auto result = blockchain->AddBlock(block);
if (result.IsError()) {
    // Handle error efficiently without exceptions
}

// Batch operations when possible
std::vector<Transaction> txs;
for (const auto& tx : transactions) {
    txs.push_back(tx);
}
mempool->AddBatch(txs);  // More efficient than individual adds

// Use performance monitoring
PerformanceMonitor monitor;
monitor.RecordOperation("block_processing", duration_ms);
```

---

## Security

### Security Best Practices

```cpp
// Input validation
NEO_REQUIRE(!address.empty(), ErrorCode::InvalidArgument, "Address cannot be empty");
NEO_REQUIRE(amount > 0, ErrorCode::InvalidArgument, "Amount must be positive");

// Use secure random generation
auto key = cryptography::Crypto::GenerateRandomBytes(32);

// Rate limiting
RateLimiter limiter(1000, std::chrono::seconds(1));  // 1000 req/sec
if (!limiter.TryAcquire()) {
    return Err<void>(NeoException(ErrorCode::RateLimitExceeded, "Too many requests"));
}

// Secure error handling
ErrorGuard guard([&]() {
    // Cleanup on error
    connection->Close();
    transaction.Rollback();
});

// Validate all external data
auto validation_result = ValidateTransaction(tx);
if (!validation_result.IsSuccess()) {
    return validation_result.Error();
}
```

---

## Examples

### Complete Node Example

```cpp
#include <neo/neo.h>

int main() {
    try {
        // Initialize configuration
        neo::core::SystemConfig sys_config;
        sys_config.network_id = 860833102;  // N3 MainNet
        sys_config.data_path = "./mainnet_data";
        
        // Create system
        auto system = std::make_unique<neo::core::NeoSystem>(sys_config);
        
        // Setup networking with connection pool
        neo::network::ConnectionPool::Config pool_config;
        pool_config.max_connections = 100;
        auto connection_pool = std::make_unique<neo::network::ConnectionPool>(pool_config);
        
        neo::network::P2PConfig p2p_config;
        p2p_config.port = 10333;
        auto p2p_server = std::make_unique<neo::network::P2PServer>(p2p_config);
        
        // Setup RPC
        neo::rpc::RpcConfig rpc_config;
        rpc_config.port = 10332;
        auto rpc_server = std::make_unique<neo::rpc::RpcServer>(rpc_config);
        
        // Setup monitoring
        neo::monitoring::PerformanceMonitor monitor;
        monitor.SetAlertThreshold("memory", 4096);  // 4GB
        monitor.SetAlertThreshold("cpu", 80);       // 80%
        
        // Start services
        monitor.Start();
        system->Start();
        connection_pool->Start();
        p2p_server->Start();
        rpc_server->Start();
        
        // Setup signal handlers
        std::signal(SIGINT, [](int) {
            std::cout << "Shutting down..." << std::endl;
            // Trigger shutdown
        });
        
        // Run until shutdown
        system->WaitForShutdown();
        
        // Cleanup
        rpc_server->Stop();
        p2p_server->Stop();
        connection_pool->Stop();
        system->Shutdown();
        monitor.Stop();
        
        // Export final metrics
        auto metrics = monitor.GetMetrics();
        std::cout << "Final metrics:" << std::endl;
        std::cout << monitor.ExportJSON().dump(2) << std::endl;
        
        return 0;
        
    } catch (const neo::error::NeoException& e) {
        std::cerr << "Neo error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

---

## Migration Guide

### Migrating from Neo C# to Neo C++

```cpp
// C# Code:
// var system = new NeoSystem(ProtocolSettings.Default, storageEngine);

// C++ Equivalent:
auto config = neo::core::SystemConfig();
auto system = std::make_unique<neo::core::NeoSystem>(config);

// C# Code:
// await system.StartAsync();

// C++ Equivalent:
system->Start();  // Synchronous, but non-blocking

// C# Code:
// var block = Blockchain.Singleton.GetBlock(hash);

// C++ Equivalent:
auto block = system->GetBlockchain()->GetBlock(hash);

// C# Code:
// using (var snapshot = Blockchain.Singleton.GetSnapshot())

// C++ Equivalent:
auto snapshot = system->GetBlockchain()->GetSnapshot();
// RAII handles cleanup automatically
```

---

## Support

For questions and support:
- GitHub Issues: https://github.com/neo-project/neo-cpp/issues
- Documentation: https://docs.neo.org
- Community: https://discord.gg/neo

---

## License

MIT License - See LICENSE file for details