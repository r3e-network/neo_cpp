# Neo C++ API Reference

## Overview

This document provides comprehensive API reference for the Neo C++ blockchain node implementation. The API is organized into logical modules corresponding to the system architecture.

## Core API Modules

### 1. Blockchain API

#### Class: `Blockchain`
**Namespace**: `neo::ledger`  
**Header**: `include/neo/ledger/blockchain.h`

Main blockchain management class.

```cpp
class Blockchain {
public:
    // Constructor
    Blockchain(std::shared_ptr<ProtocolSettings> settings,
               std::shared_ptr<IStore> store);
    
    // Initialization
    bool Initialize();
    
    // Block Operations
    bool ProcessBlock(std::shared_ptr<Block> block);
    std::shared_ptr<Block> GetBlock(const UInt256& hash);
    std::shared_ptr<Block> GetBlock(uint32_t index);
    UInt256 GetBlockHash(uint32_t index);
    
    // Chain State
    uint32_t GetHeight() const;
    UInt256 GetCurrentBlockHash() const;
    std::shared_ptr<Block> GetCurrentBlock() const;
    
    // Transaction Operations
    std::shared_ptr<Transaction> GetTransaction(const UInt256& hash);
    uint32_t GetTransactionHeight(const UInt256& hash);
    
    // Snapshot Management
    std::shared_ptr<DataCache> GetSnapshot();
    
    // Events
    std::function<void(std::shared_ptr<Block>)> OnBlockAdded;
    std::function<void(std::shared_ptr<Transaction>)> OnTransactionAdded;
};
```

**Usage Example:**
```cpp
auto settings = ProtocolSettings::GetDefault();
auto store = std::make_shared<LevelDBStore>("blockchain_data");
auto blockchain = std::make_shared<Blockchain>(settings, store);

if (blockchain->Initialize()) {
    uint32_t height = blockchain->GetHeight();
    auto currentBlock = blockchain->GetCurrentBlock();
}
```

#### Class: `Block`
**Namespace**: `neo::ledger`  
**Header**: `include/neo/ledger/block.h`

Represents a blockchain block.

```cpp
class Block {
public:
    // Constructors
    Block();
    Block(const BlockHeader& header, 
          const std::vector<std::shared_ptr<Transaction>>& transactions);
    
    // Properties
    const BlockHeader& GetHeader() const;
    const std::vector<std::shared_ptr<Transaction>>& GetTransactions() const;
    UInt256 GetHash() const;
    uint32_t GetIndex() const;
    uint64_t GetTimestamp() const;
    
    // Validation
    bool Verify(std::shared_ptr<ProtocolSettings> settings) const;
    UInt256 CalculateMerkleRoot() const;
    
    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    
    // Size calculation
    size_t GetSize() const;
};
```

#### Class: `Transaction`
**Namespace**: `neo::ledger`  
**Header**: `include/neo/ledger/transaction.h`

Represents a Neo N3 transaction.

```cpp
class Transaction {
public:
    // Constructor
    Transaction();
    
    // Properties
    uint8_t GetVersion() const;
    uint32_t GetNonce() const;
    uint64_t GetSystemFee() const;
    uint64_t GetNetworkFee() const;
    uint32_t GetValidUntilBlock() const;
    const std::vector<Signer>& GetSigners() const;
    const std::vector<uint8_t>& GetScript() const;
    const std::vector<Witness>& GetWitnesses() const;
    
    // Derived properties
    UInt256 GetHash() const;
    UInt160 GetSender() const;
    
    // Validation
    bool Verify(std::shared_ptr<ProtocolSettings> settings,
               std::shared_ptr<DataCache> snapshot,
               std::shared_ptr<MemoryPool> mempool,
               const std::unordered_set<Transaction*>& conflicts = {}) const;
    
    // Setters
    void SetVersion(uint8_t version);
    void SetNonce(uint32_t nonce);
    void SetSystemFee(uint64_t fee);
    void SetNetworkFee(uint64_t fee);
    void SetValidUntilBlock(uint32_t block);
    void SetSigners(const std::vector<Signer>& signers);
    void SetScript(const std::vector<uint8_t>& script);
    void SetWitnesses(const std::vector<Witness>& witnesses);
    
    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    
    // Size and Fee calculation
    size_t GetSize() const;
    uint64_t GetTotalFee() const;
};
```

### 2. Virtual Machine API

#### Class: `ExecutionEngine`
**Namespace**: `neo::vm`  
**Header**: `include/neo/vm/execution_engine.h`

Neo Virtual Machine execution engine.

```cpp
class ExecutionEngine {
public:
    // Constructor
    ExecutionEngine();
    
    // Script execution
    VMState Execute();
    void LoadScript(const std::vector<uint8_t>& script);
    void LoadScript(const std::vector<uint8_t>& script, 
                   CallFlags callFlags);
    
    // Stack operations
    void Push(std::shared_ptr<StackItem> item);
    std::shared_ptr<StackItem> Pop();
    std::shared_ptr<StackItem> Peek(int index = 0);
    
    // State management
    VMState GetState() const;
    std::vector<std::shared_ptr<StackItem>> GetResultStack() const;
    uint64_t GetGasConsumed() const;
    
    // Context management
    std::shared_ptr<ExecutionContext> CurrentContext();
    void PushContext(std::shared_ptr<ExecutionContext> context);
    std::shared_ptr<ExecutionContext> PopContext();
    
    // Event handlers
    std::function<void(const std::string&)> OnLog;
    std::function<void(const std::vector<uint8_t>&)> OnNotify;
};
```

#### Class: `ApplicationEngine`
**Namespace**: `neo::smartcontract`  
**Header**: `include/neo/smartcontract/application_engine.h`

Enhanced VM for smart contract execution.

```cpp
class ApplicationEngine : public ExecutionEngine {
public:
    // Factory methods
    static std::shared_ptr<ApplicationEngine> Create(
        TriggerType trigger,
        std::shared_ptr<IVerifiable> container,
        std::shared_ptr<DataCache> snapshot,
        std::shared_ptr<Block> persistingBlock,
        std::shared_ptr<ProtocolSettings> settings,
        uint64_t gas);
    
    // Contract invocation
    void CallContract(const UInt160& scriptHash,
                     const std::string& method,
                     const std::vector<std::shared_ptr<StackItem>>& args);
    
    // System calls
    void SystemCall(uint32_t method);
    
    // Storage operations
    std::shared_ptr<StorageItem> GetStorageItem(const StorageKey& key);
    void PutStorageItem(const StorageKey& key, 
                       std::shared_ptr<StorageItem> item);
    void DeleteStorageItem(const StorageKey& key);
    
    // Contract management
    std::shared_ptr<ContractState> GetContract(const UInt160& hash);
    void DeployContract(std::shared_ptr<ContractState> contract);
    void UpdateContract(const UInt160& hash, 
                       std::shared_ptr<ContractState> contract);
    void DestroyContract(const UInt160& hash);
    
    // Events and logs
    void Log(const std::string& message);
    void Notify(const std::vector<std::shared_ptr<StackItem>>& state);
    
    // Gas management
    void AddGas(uint64_t gas);
    bool CheckGas(uint64_t gas);
    
    // Snapshot access
    std::shared_ptr<DataCache> GetSnapshot() const;
};
```

### 3. Consensus API

#### Class: `ConsensusContext`
**Namespace**: `neo::consensus`  
**Header**: `include/neo/consensus/consensus_context.h`

dBFT consensus management.

```cpp
class ConsensusContext {
public:
    // Constructor
    ConsensusContext(std::shared_ptr<NeoSystem> system,
                    std::shared_ptr<ProtocolSettings> settings,
                    std::shared_ptr<ISigner> signer);
    
    // Consensus operations
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Message processing
    void ProcessMessage(std::shared_ptr<ConsensusMessage> message);
    void OnTimer();
    
    // Block creation
    std::shared_ptr<Block> CreateBlock();
    void FillContext();
    
    // State queries
    ConsensusState GetState() const;
    uint32_t GetBlockIndex() const;
    uint8_t GetViewNumber() const;
    const std::vector<ECPoint>& GetValidators() const;
    
    // Policy checking
    bool CheckPolicy(std::shared_ptr<Transaction> tx);
    bool CheckWitness(const UInt160& hash);
    
    // Events
    std::function<void(std::shared_ptr<Block>)> OnBlockCreated;
    std::function<void(std::shared_ptr<ConsensusMessage>)> OnMessageToSend;
};
```

#### Class: `ConsensusMessage`
**Namespace**: `neo::consensus`  
**Header**: `include/neo/consensus/consensus_message.h`

Base class for consensus messages.

```cpp
class ConsensusMessage {
public:
    // Properties
    ConsensusMessageType GetType() const;
    uint32_t GetBlockIndex() const;
    uint8_t GetValidatorIndex() const;
    uint8_t GetViewNumber() const;
    
    // Validation
    virtual bool Verify(std::shared_ptr<ProtocolSettings> settings) const;
    
    // Serialization
    virtual void Serialize(BinaryWriter& writer) const = 0;
    virtual void Deserialize(BinaryReader& reader) = 0;
    
    // Factory method
    static std::shared_ptr<ConsensusMessage> DeserializeFrom(
        const std::vector<uint8_t>& data);
};
```

### 4. Networking API

#### Class: `P2PServer`
**Namespace**: `neo::network`  
**Header**: `include/neo/network/p2p_server.h`

P2P networking server.

```cpp
class P2PServer {
public:
    // Constructor
    P2PServer(std::shared_ptr<ProtocolSettings> settings,
             std::shared_ptr<Blockchain> blockchain,
             std::shared_ptr<MemoryPool> mempool);
    
    // Server lifecycle
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Connection management
    void ConnectToPeers();
    void DisconnectPeer(std::shared_ptr<RemoteNode> peer);
    std::vector<std::shared_ptr<RemoteNode>> GetConnectedPeers() const;
    size_t GetConnectionCount() const;
    
    // Message broadcasting
    void BroadcastMessage(std::shared_ptr<IPayload> payload);
    void RelayTransaction(std::shared_ptr<Transaction> transaction);
    void RelayBlock(std::shared_ptr<Block> block);
    
    // Event handlers
    std::function<void(std::shared_ptr<RemoteNode>)> OnPeerConnected;
    std::function<void(std::shared_ptr<RemoteNode>)> OnPeerDisconnected;
    std::function<void(std::shared_ptr<Message>, std::shared_ptr<RemoteNode>)> OnMessageReceived;
};
```

#### Class: `RemoteNode`
**Namespace**: `neo::network`  
**Header**: `include/neo/network/remote_node.h`

Represents a remote network peer.

```cpp
class RemoteNode {
public:
    // Connection info
    const std::string& GetAddress() const;
    uint16_t GetPort() const;
    bool IsConnected() const;
    
    // Protocol info
    uint32_t GetVersion() const;
    uint64_t GetServices() const;
    const std::string& GetUserAgent() const;
    uint32_t GetStartHeight() const;
    
    // Communication
    void SendMessage(std::shared_ptr<IPayload> payload);
    void Disconnect();
    
    // Statistics
    uint64_t GetBytesReceived() const;
    uint64_t GetBytesSent() const;
    std::chrono::steady_clock::time_point GetLastSeen() const;
};
```

### 5. Storage API

#### Interface: `IStore`
**Namespace**: `neo::persistence`  
**Header**: `include/neo/persistence/istore.h`

Abstract storage interface.

```cpp
class IStore {
public:
    virtual ~IStore() = default;
    
    // Snapshot management
    virtual std::shared_ptr<ISnapshot> GetSnapshot() = 0;
    
    // Basic operations
    virtual void Put(const StorageKey& key, const StorageItem& value) = 0;
    virtual std::optional<StorageItem> TryGet(const StorageKey& key) = 0;
    virtual bool Contains(const StorageKey& key) = 0;
    virtual void Delete(const StorageKey& key) = 0;
    
    // Batch operations
    virtual void PutBatch(const std::vector<std::pair<StorageKey, StorageItem>>& batch) = 0;
    virtual void DeleteBatch(const std::vector<StorageKey>& keys) = 0;
    
    // Iteration
    virtual std::unique_ptr<IIterator> Seek(const StorageKey& key) = 0;
    virtual std::unique_ptr<IIterator> SeekAll() = 0;
};
```

#### Class: `StorageKey`
**Namespace**: `neo::persistence`  
**Header**: `include/neo/persistence/storage_key.h`

Neo N3 storage key implementation.

```cpp
class StorageKey {
public:
    // Constructors
    StorageKey(int32_t id, const std::vector<uint8_t>& key);
    
    // Factory methods
    static StorageKey Create(int32_t id, uint8_t prefix);
    static StorageKey Create(int32_t id, uint8_t prefix, const UInt160& hash);
    static StorageKey Create(int32_t id, uint8_t prefix, const UInt256& hash);
    
    // Properties
    int32_t GetId() const;
    const std::vector<uint8_t>& GetKey() const;
    
    // Serialization
    std::vector<uint8_t> ToArray() const;
    static StorageKey FromArray(const std::vector<uint8_t>& data);
    
    // Comparison
    bool operator==(const StorageKey& other) const;
    bool operator<(const StorageKey& other) const;
};
```

### 6. RPC API

#### Class: `RpcServer`
**Namespace**: `neo::rpc`  
**Header**: `include/neo/rpc/rpc_server.h`

JSON-RPC server implementation.

```cpp
class RpcServer {
public:
    // Constructor
    RpcServer(std::shared_ptr<NeoSystem> system,
             const std::string& bind_address,
             uint16_t port);
    
    // Server lifecycle
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Configuration
    void SetMaxConnections(size_t max_connections);
    void SetTimeout(std::chrono::seconds timeout);
    void AddDisabledMethod(const std::string& method);
    
    // Request processing
    nlohmann::json ProcessRequest(const nlohmann::json& request);
    
private:
    // Method handlers
    nlohmann::json HandleGetBestBlockHash();
    nlohmann::json HandleGetBlock(const nlohmann::json& params);
    nlohmann::json HandleGetBlockCount();
    nlohmann::json HandleSendRawTransaction(const nlohmann::json& params);
    // ... other method handlers
};
```

#### Class: `RPCMethods`
**Namespace**: `neo::rpc`  
**Header**: `include/neo/rpc/rpc_methods.h`

Static RPC method implementations.

```cpp
class RPCMethods {
public:
    // Blockchain methods
    static nlohmann::json GetBestBlockHash(std::shared_ptr<NeoSystem> system,
                                          const nlohmann::json& params);
    static nlohmann::json GetBlock(std::shared_ptr<NeoSystem> system,
                                  const nlohmann::json& params);
    static nlohmann::json GetBlockCount(std::shared_ptr<NeoSystem> system,
                                       const nlohmann::json& params);
    
    // Transaction methods
    static nlohmann::json GetRawTransaction(std::shared_ptr<NeoSystem> system,
                                           const nlohmann::json& params);
    static nlohmann::json SendRawTransaction(std::shared_ptr<NeoSystem> system,
                                            const nlohmann::json& params);
    
    // Smart contract methods
    static nlohmann::json InvokeFunction(std::shared_ptr<NeoSystem> system,
                                        const nlohmann::json& params);
    static nlohmann::json InvokeScript(std::shared_ptr<NeoSystem> system,
                                      const nlohmann::json& params);
    
    // Node methods
    static nlohmann::json GetVersion(std::shared_ptr<NeoSystem> system,
                                    const nlohmann::json& params);
    static nlohmann::json GetConnectionCount(std::shared_ptr<NeoSystem> system,
                                            const nlohmann::json& params);
};
```

### 7. Cryptography API

#### Class: `Hash`
**Namespace**: `neo::cryptography`  
**Header**: `include/neo/cryptography/hash.h`

Cryptographic hash functions.

```cpp
class Hash {
public:
    // Hash functions
    static UInt256 SHA256(const std::vector<uint8_t>& data);
    static UInt160 RIPEMD160(const std::vector<uint8_t>& data);
    static UInt160 Hash160(const std::vector<uint8_t>& data);
    static UInt256 Hash256(const std::vector<uint8_t>& data);
    
    // HMAC functions
    static std::vector<uint8_t> HMACSHA256(const std::vector<uint8_t>& key,
                                          const std::vector<uint8_t>& data);
    
    // Merkle tree operations
    static UInt256 ComputeMerkleRoot(const std::vector<UInt256>& hashes);
};
```

#### Class: `ECPoint`
**Namespace**: `neo::cryptography`  
**Header**: `include/neo/cryptography/ecc/ecpoint.h`

Elliptic curve point operations.

```cpp
class ECPoint {
public:
    // Constructors
    ECPoint();
    ECPoint(const std::vector<uint8_t>& data);
    
    // Encoding/Decoding
    std::vector<uint8_t> EncodePoint(bool compressed) const;
    static ECPoint DecodePoint(const std::vector<uint8_t>& data);
    
    // Cryptographic operations
    bool VerifySignature(const std::vector<uint8_t>& message,
                        const std::vector<uint8_t>& signature) const;
    
    // Properties
    bool IsInfinity() const;
    bool IsValid() const;
    
    // Comparison
    bool operator==(const ECPoint& other) const;
    bool operator!=(const ECPoint& other) const;
    
    // Arithmetic operations
    ECPoint operator+(const ECPoint& other) const;
    ECPoint operator*(const BigInteger& scalar) const;
};
```

#### Class: `KeyPair`
**Namespace**: `neo::cryptography`  
**Header**: `include/neo/cryptography/key_pair.h`

Cryptographic key pair management.

```cpp
class KeyPair {
public:
    // Constructors
    KeyPair();
    KeyPair(const std::vector<uint8_t>& privateKey);
    
    // Key generation
    static std::shared_ptr<KeyPair> Generate();
    static std::shared_ptr<KeyPair> FromPrivateKey(const std::vector<uint8_t>& privateKey);
    static std::shared_ptr<KeyPair> FromWIF(const std::string& wif);
    
    // Key access
    const std::vector<uint8_t>& GetPrivateKey() const;
    const ECPoint& GetPublicKey() const;
    
    // Address generation
    UInt160 GetScriptHash() const;
    std::string GetAddress() const;
    
    // Signing
    std::vector<uint8_t> Sign(const std::vector<uint8_t>& message) const;
    
    // Export
    std::string ExportWIF() const;
};
```

### 8. Data Types

#### Class: `UInt160`
**Namespace**: `neo::io`  
**Header**: `include/neo/io/uint160.h`

160-bit unsigned integer (20 bytes).

```cpp
class UInt160 {
public:
    // Constructors
    UInt160();
    UInt160(const std::vector<uint8_t>& data);
    
    // Factory methods
    static UInt160 Zero();
    static UInt160 Parse(const std::string& hex);
    
    // Properties
    static constexpr size_t Size() { return 20; }
    bool IsZero() const;
    
    // Conversion
    std::string ToString() const;
    std::vector<uint8_t> ToArray() const;
    
    // Comparison
    bool operator==(const UInt160& other) const;
    bool operator!=(const UInt160& other) const;
    bool operator<(const UInt160& other) const;
};
```

#### Class: `UInt256`
**Namespace**: `neo::io`  
**Header**: `include/neo/io/uint256.h`

256-bit unsigned integer (32 bytes).

```cpp
class UInt256 {
public:
    // Constructors
    UInt256();
    UInt256(const std::vector<uint8_t>& data);
    
    // Factory methods
    static UInt256 Zero();
    static UInt256 Parse(const std::string& hex);
    
    // Properties
    static constexpr size_t Size() { return 32; }
    bool IsZero() const;
    
    // Conversion
    std::string ToString() const;
    std::vector<uint8_t> ToArray() const;
    
    // Comparison
    bool operator==(const UInt256& other) const;
    bool operator!=(const UInt256& other) const;
    bool operator<(const UInt256& other) const;
};
```

## Usage Examples

### Basic Node Setup

```cpp
#include <neo/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/persistence/leveldb_store.h>
#include <neo/network/p2p_server.h>

// Initialize node
auto settings = ProtocolSettings::GetDefault();
auto store = std::make_shared<LevelDBStore>("blockchain_data");
auto blockchain = std::make_shared<Blockchain>(settings, store);
auto mempool = std::make_shared<MemoryPool>(settings);
auto p2pServer = std::make_shared<P2PServer>(settings, blockchain, mempool);

// Start services
blockchain->Initialize();
p2pServer->Start();
```

### Smart Contract Execution

```cpp
#include <neo/smartcontract/application_engine.h>

// Create execution environment
auto engine = ApplicationEngine::Create(
    TriggerType::Application,
    nullptr,
    blockchain->GetSnapshot(),
    nullptr,
    settings,
    10000000 // 10 GAS
);

// Execute contract method
std::vector<std::shared_ptr<StackItem>> args = {
    std::make_shared<ByteArrayStackItem>(std::vector<uint8_t>{1, 2, 3})
};

engine->CallContract(contractHash, "transfer", args);
auto result = engine->Execute();

if (result == VMState::HALT) {
    auto returnValue = engine->GetResultStack()[0];
    // Process return value
}
```

### RPC Server Setup

```cpp
#include <neo/rpc/rpc_server.h>

// Create RPC server
auto neoSystem = std::make_shared<NeoSystem>(settings, store);
auto rpcServer = std::make_shared<RpcServer>(neoSystem, "127.0.0.1", 10332);

// Start RPC service
rpcServer->Start();

// Server will handle JSON-RPC requests automatically
```

## Error Handling

All API methods use standard C++ exception handling. Common exceptions include:

- `std::invalid_argument`: Invalid input parameters
- `std::runtime_error`: Runtime execution errors
- `ValidationException`: Validation failures
- `VMException`: Virtual machine execution errors
- `NetworkException`: Network communication errors

## Thread Safety

Most API classes are thread-safe for read operations but require external synchronization for write operations. Specific thread safety guarantees are documented in individual class headers.

## Performance Considerations

- Use smart pointers for automatic memory management
- Prefer const references for large objects
- Use move semantics where appropriate
- Consider batching operations for better performance
- Cache frequently accessed data

This API reference provides the foundation for building applications on top of the Neo C++ blockchain node.