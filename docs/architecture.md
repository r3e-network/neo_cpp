# Neo C++ Architecture Documentation

## Overview

The Neo C++ blockchain node is a complete reimplementation of the Neo N3 protocol in modern C++20. This document provides a comprehensive overview of the system architecture, design patterns, and implementation details.

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Neo C++ Node                             │
├─────────────────────────────────────────────────────────────┤
│  Application Layer                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│  │   Neo CLI   │  │   RPC API   │  │  Plugins    │       │
│  └─────────────┘  └─────────────┘  └─────────────┘       │
├─────────────────────────────────────────────────────────────┤
│  Core Services Layer                                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│  │ Consensus   │  │   P2P Net   │  │  Mempool    │       │
│  │   (dBFT)    │  │             │  │             │       │
│  └─────────────┘  └─────────────┘  └─────────────┘       │
├─────────────────────────────────────────────────────────────┤
│  Blockchain Layer                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│  │ Blockchain  │  │   VM        │  │Smart        │       │
│  │ Ledger      │  │ Engine      │  │Contracts    │       │
│  └─────────────┘  └─────────────┘  └─────────────┘       │
├─────────────────────────────────────────────────────────────┤
│  Storage Layer                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│  │  LevelDB    │  │  RocksDB    │  │   Memory    │       │
│  │   Store     │  │   Store     │  │   Store     │       │
│  └─────────────┘  └─────────────┘  └─────────────┘       │
├─────────────────────────────────────────────────────────────┤
│  Foundation Layer                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│  │Cryptography │  │  I/O Utils  │  │ Extensions  │       │
│  └─────────────┘  └─────────────┘  └─────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Blockchain Layer

#### Blockchain Manager
**Location**: `src/ledger/blockchain.cpp`
**Purpose**: Central coordinator for blockchain operations

```cpp
class Blockchain {
public:
    bool ProcessBlock(std::shared_ptr<Block> block);
    std::shared_ptr<Block> GetBlock(const UInt256& hash);
    uint32_t GetHeight() const;
    bool Initialize();
    
private:
    std::shared_ptr<IStore> store_;
    std::shared_ptr<MemoryPool> mempool_;
    std::shared_ptr<ProtocolSettings> settings_;
};
```

**Key Features:**
- Thread-safe block processing
- ACID transaction support
- Merkle tree validation
- Genesis block handling

#### Block Structure
**Location**: `src/ledger/block.cpp`

```cpp
class Block {
    BlockHeader header_;
    std::vector<std::shared_ptr<Transaction>> transactions_;
    
public:
    bool Verify(std::shared_ptr<ProtocolSettings> settings) const;
    UInt256 CalculateMerkleRoot() const;
    void Serialize(BinaryWriter& writer) const;
};
```

#### Transaction Processing
**Location**: `src/ledger/transaction.cpp`

```cpp
class Transaction {
    uint8_t version_;
    uint32_t nonce_;
    uint64_t systemFee_;
    uint64_t networkFee_;
    uint32_t validUntilBlock_;
    std::vector<Signer> signers_;
    std::vector<uint8_t> script_;
    std::vector<Witness> witnesses_;
    
public:
    bool Verify(std::shared_ptr<ProtocolSettings> settings,
               std::shared_ptr<DataCache> snapshot,
               std::shared_ptr<MemoryPool> mempool) const;
};
```

### 2. Consensus Layer (dBFT)

#### Consensus Context
**Location**: `src/consensus/consensus_context.cpp`

The consensus implementation follows the Delegated Byzantine Fault Tolerance (dBFT) algorithm:

```cpp
class ConsensusContext {
    std::vector<ECPoint> validators_;
    uint32_t blockIndex_;
    uint8_t viewNumber_;
    ConsensusState state_;
    
public:
    void ProcessMessage(std::shared_ptr<ConsensusMessage> message);
    bool CheckPolicy(std::shared_ptr<Transaction> tx);
    void CreateBlock();
};
```

#### Message Types
- **PrepareRequest**: Initial block proposal
- **PrepareResponse**: Validator agreement
- **ChangeView**: View change request
- **Commit**: Final commitment
- **RecoveryMessage**: State synchronization

#### Consensus Flow
```
Speaker Selection → Block Proposal → Validation → Commitment → Block Finalization
```

### 3. Virtual Machine

#### Execution Engine
**Location**: `src/vm/execution_engine.cpp`

```cpp
class ExecutionEngine {
    EvaluationStack stack_;
    std::vector<ExecutionContext> contexts_;
    VMState state_;
    
public:
    VMState Execute();
    void LoadScript(const std::vector<uint8_t>& script);
    std::vector<StackItem> GetResultStack() const;
};
```

#### Opcode Implementation
**Location**: `src/vm/jump_table_*.cpp`

Each opcode category has dedicated implementation:
- **Arithmetic**: ADD, SUB, MUL, DIV, MOD
- **Bitwise**: AND, OR, XOR, NOT, SHL, SHR
- **Control**: JMP, JMPIF, CALL, RET
- **Stack**: PUSH, POP, DUP, SWAP
- **Type**: CONVERT, ISNULL, SIZE

### 4. Smart Contracts

#### Native Contracts
**Location**: `src/smartcontract/native/`

```cpp
class NativeContract {
protected:
    int32_t id_;
    std::string name_;
    
public:
    virtual void Initialize(ApplicationEngine& engine) = 0;
    virtual void PostPersist(ApplicationEngine& engine) = 0;
    virtual std::vector<ContractMethodDescriptor> GetMethods() const = 0;
};
```

**Implemented Native Contracts:**
- **NeoToken** (ID: 1): Governance token
- **GasToken** (ID: 2): Utility token
- **PolicyContract** (ID: 3): Network policies
- **RoleManagement** (ID: 4): Role assignments
- **OracleContract** (ID: 5): Oracle services

#### Application Engine
**Location**: `src/smartcontract/application_engine.cpp`

```cpp
class ApplicationEngine : public ExecutionEngine {
    std::shared_ptr<DataCache> snapshot_;
    std::vector<LogEventArgs> logs_;
    uint64_t gasConsumed_;
    
public:
    ApplicationEngine(TriggerType trigger,
                     std::shared_ptr<IVerifiable> container,
                     std::shared_ptr<DataCache> snapshot,
                     uint64_t gas);
    
    void CallContract(const UInt160& scriptHash,
                     const std::string& method,
                     const std::vector<StackItem>& args);
};
```

### 5. Networking Layer

#### P2P Server
**Location**: `src/network/p2p_server.cpp`

```cpp
class P2PServer {
    std::vector<std::shared_ptr<RemoteNode>> connectedPeers_;
    std::shared_ptr<TcpServer> tcpServer_;
    std::shared_ptr<TaskManager> taskManager_;
    
public:
    void Start();
    void Stop();
    void BroadcastMessage(std::shared_ptr<IPayload> payload);
    void OnMessageReceived(std::shared_ptr<Message> message,
                          std::shared_ptr<RemoteNode> sender);
};
```

#### Message Protocol
**Location**: `src/network/message.cpp`

Neo N3 message format:
```
┌─────────────┬─────────────┬─────────────┬─────────────┬─────────────┐
│   Magic     │  Command    │   Length    │  Checksum   │   Payload   │
│  4 bytes    │  1 byte     │  4 bytes    │  4 bytes    │  Variable   │
└─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘
```

**Message Types:**
- Version/Verack: Handshake
- Ping/Pong: Keep-alive
- GetAddr/Addr: Peer discovery
- GetBlocks/Inv: Block synchronization
- GetData/Block: Data retrieval

### 6. Storage Layer

#### Storage Interface
**Location**: `src/persistence/istore.h`

```cpp
class IStore {
public:
    virtual ~IStore() = default;
    virtual std::shared_ptr<ISnapshot> GetSnapshot() = 0;
    virtual void Put(const StorageKey& key, const StorageItem& value) = 0;
    virtual std::optional<StorageItem> TryGet(const StorageKey& key) = 0;
    virtual bool Contains(const StorageKey& key) = 0;
    virtual void Delete(const StorageKey& key) = 0;
};
```

#### LevelDB Implementation
**Location**: `src/persistence/leveldb_store.cpp`

```cpp
class LevelDBStore : public IStore {
    std::unique_ptr<leveldb::DB> db_;
    
public:
    LevelDBStore(const std::string& path);
    void Put(const StorageKey& key, const StorageItem& value) override;
    std::optional<StorageItem> TryGet(const StorageKey& key) override;
};
```

#### Storage Key Format
Neo N3 uses a contract-based storage key format:

```cpp
struct StorageKey {
    int32_t id_;              // Contract ID (4 bytes, little-endian)
    std::vector<uint8_t> key_; // Storage key data
    
    static StorageKey Create(int32_t id, uint8_t prefix);
    static StorageKey Create(int32_t id, uint8_t prefix, const UInt160& hash);
};
```

### 7. RPC Interface

#### RPC Server
**Location**: `src/rpc/rpc_server.cpp`

```cpp
class RpcServer {
    std::shared_ptr<HttpServer> httpServer_;
    std::shared_ptr<NeoSystem> neoSystem_;
    
public:
    void Start(const std::string& bind_address, uint16_t port);
    nlohmann::json ProcessRequest(const nlohmann::json& request);
};
```

#### Implemented Methods (29 total)

**Blockchain Methods:**
- `getbestblockhash`: Get latest block hash
- `getblock`: Get block by hash/index
- `getblockcount`: Get current block height
- `getblockhash`: Get block hash by index
- `getblockheader`: Get block header

**Node Methods:**
- `getconnectioncount`: Get peer count
- `getpeers`: Get connected peers
- `getversion`: Get node version

**Transaction Methods:**
- `getrawtransaction`: Get transaction
- `sendrawtransaction`: Submit transaction
- `gettransactionheight`: Get transaction block height

**Smart Contract Methods:**
- `invokefunction`: Call contract method
- `invokescript`: Execute script
- `getcontractstate`: Get contract information

### 8. Cryptography

#### Hash Functions
**Location**: `src/cryptography/hash.cpp`

```cpp
class Hash {
public:
    static UInt256 SHA256(const std::vector<uint8_t>& data);
    static UInt160 RIPEMD160(const std::vector<uint8_t>& data);
    static UInt160 Hash160(const std::vector<uint8_t>& data);
    static UInt256 Hash256(const std::vector<uint8_t>& data);
};
```

#### Elliptic Curve Cryptography
**Location**: `src/cryptography/ecc.cpp`

```cpp
class ECPoint {
    std::vector<uint8_t> data_;
    
public:
    bool Verify(const std::vector<uint8_t>& message,
               const std::vector<uint8_t>& signature) const;
    static ECPoint DecodePoint(const std::vector<uint8_t>& data);
    std::vector<uint8_t> EncodePoint(bool compressed) const;
};
```

#### BLS12-381 Support
**Location**: `src/cryptography/bls12_381.cpp`

Support for BLS signature aggregation and verification for advanced cryptographic operations.

## Design Patterns

### 1. RAII (Resource Acquisition Is Initialization)
All resources are managed through smart pointers and automatic destruction.

### 2. Observer Pattern
Event-driven architecture for blockchain events, consensus messages, and network notifications.

### 3. Strategy Pattern
Pluggable storage backends, different consensus strategies, and various cryptographic implementations.

### 4. Factory Pattern
Creation of transactions, blocks, and messages through factory classes.

### 5. Singleton Pattern
Protocol settings, native contract instances, and global configuration.

## Thread Safety

### Concurrent Access
- **Read-Heavy Operations**: Shared locks for blockchain queries
- **Write Operations**: Exclusive locks for block processing
- **Network I/O**: Separate thread pool for P2P operations
- **Consensus**: Dedicated consensus thread

### Synchronization Primitives
```cpp
std::shared_mutex blockchain_mutex_;      // Reader-writer lock
std::mutex mempool_mutex_;               // Exclusive mempool access
std::atomic<uint32_t> block_height_;     // Atomic height updates
```

## Performance Optimizations

### 1. Memory Management
- Object pooling for frequently created objects
- Custom allocators for specific data structures
- RAII-based automatic memory management

### 2. I/O Optimizations
- Asynchronous I/O for network operations
- Batch operations for database writes
- Memory-mapped files for large data sets

### 3. Caching
- LRU cache for frequently accessed blocks
- Transaction cache for validation
- State cache for smart contract execution

### 4. Parallel Processing
- Multi-threaded transaction validation
- Concurrent signature verification
- Parallel merkle tree computation

## Security Considerations

### 1. Input Validation
All external inputs are validated before processing:
- Transaction format validation
- Block structure verification
- Script execution limits
- Network message validation

### 2. Memory Safety
- Smart pointer usage throughout
- Bounds checking for all array access
- RAII for automatic resource cleanup
- No manual memory management

### 3. Cryptographic Security
- Industry-standard cryptographic libraries
- Secure random number generation
- Proper key management
- Side-channel attack resistance

## Configuration

### Protocol Settings
**Location**: `src/protocol_settings.cpp`

```cpp
class ProtocolSettings {
    uint32_t network_;
    uint8_t addressVersion_;
    uint32_t millisecondsPerBlock_;
    uint32_t maxTransactionsPerBlock_;
    std::vector<ECPoint> standbyCommittee_;
    
public:
    static std::shared_ptr<ProtocolSettings> LoadFrom(const std::string& path);
    static std::shared_ptr<ProtocolSettings> GetDefault();
};
```

### Network Configuration
```json
{
  "Network": 860833102,
  "AddressVersion": 53,
  "MillisecondsPerBlock": 15000,
  "MaxTransactionsPerBlock": 512,
  "ValidatorsCount": 7,
  "CommitteeMembersCount": 21
}
```

## Error Handling

### Exception Hierarchy
```cpp
class NeoException : public std::exception {};
class InvalidFormatException : public NeoException {};
class ValidationException : public NeoException {};
class ConsensusException : public NeoException {};
class VMException : public NeoException {};
```

### Error Recovery
- Graceful degradation for non-critical errors
- Automatic retry mechanisms for network operations
- State recovery for consensus failures
- Transaction rollback for validation errors

## Monitoring and Metrics

### Performance Metrics
- Transaction throughput (TPS)
- Block processing time
- Memory usage
- Network latency
- Consensus round time

### Health Checks
- Database connectivity
- Network connectivity
- Consensus participation
- Memory usage thresholds
- Disk space availability

## Future Enhancements

### Planned Features
1. **Enhanced RPC API**: Additional wallet and utility methods
2. **Performance Optimizations**: Further database and networking improvements
3. **Plugin Architecture**: Extensible plugin system
4. **Advanced Monitoring**: Comprehensive metrics and alerting
5. **Multi-signature Wallets**: Enhanced wallet functionality

### Scalability Improvements
1. **Sharding Support**: Horizontal scaling capabilities
2. **State Channels**: Off-chain transaction processing
3. **Optimized Storage**: Compressed and efficient storage formats
4. **Network Optimizations**: Improved P2P protocol efficiency

This architecture provides a solid foundation for a production-ready Neo N3 blockchain node with excellent performance, security, and maintainability characteristics.