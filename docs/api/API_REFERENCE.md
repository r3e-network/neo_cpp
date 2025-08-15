# Neo C++ API Reference

## Overview

The Neo C++ API provides comprehensive access to blockchain functionality through a well-designed, type-safe interface. This reference documents all public APIs available in the SDK.

## Table of Contents

1. [Blockchain API](#blockchain-api)
2. [Wallet API](#wallet-api)
3. [Transaction API](#transaction-api)
4. [Smart Contract API](#smart-contract-api)
5. [RPC Client API](#rpc-client-api)
6. [Network API](#network-api)
7. [Cryptography API](#cryptography-api)
8. [Storage API](#storage-api)

## Blockchain API

### `neo::sdk::blockchain::BlockchainClient`

Primary interface for blockchain interaction.

#### Constructor
```cpp
BlockchainClient(const std::string& rpcEndpoint = "http://localhost:10332");
```

#### Methods

##### `getBlockHeight()`
```cpp
uint32_t getBlockHeight() const;
```
Returns the current blockchain height.

**Returns:** Current block height  
**Throws:** `std::runtime_error` if RPC connection fails

##### `getBlock()`
```cpp
std::shared_ptr<Block> getBlock(uint32_t index) const;
std::shared_ptr<Block> getBlock(const std::string& hash) const;
```
Retrieves a block by index or hash.

**Parameters:**
- `index`: Block height
- `hash`: Block hash (hex string)

**Returns:** Block object or nullptr if not found  
**Throws:** `std::invalid_argument` for invalid hash format

##### `getTransaction()`
```cpp
std::shared_ptr<Transaction> getTransaction(const std::string& txid) const;
```
Retrieves a transaction by ID.

**Parameters:**
- `txid`: Transaction ID (hex string)

**Returns:** Transaction object or nullptr if not found

##### `getAccountState()`
```cpp
AccountState getAccountState(const std::string& address) const;
```
Gets the state of an account including balances.

**Parameters:**
- `address`: Neo address

**Returns:** Account state with balances  
**Throws:** `std::invalid_argument` for invalid address

## Wallet API

### `neo::sdk::wallet::Wallet`

Manages cryptographic keys and signs transactions.

#### Factory Methods
```cpp
static std::unique_ptr<Wallet> Create(
    const std::string& path,
    const std::string& password
);

static std::unique_ptr<Wallet> Open(
    const std::string& path,
    const std::string& password
);
```

#### Methods

##### `CreateAccount()`
```cpp
std::shared_ptr<Account> CreateAccount(const std::string& label = "");
```
Creates a new account in the wallet.

**Parameters:**
- `label`: Optional account label

**Returns:** New account object

##### `GetAccount()`
```cpp
std::shared_ptr<Account> GetAccount(const std::string& address) const;
```
Retrieves an account by address.

##### `GetAccounts()`
```cpp
std::vector<std::shared_ptr<Account>> GetAccounts() const;
```
Returns all accounts in the wallet.

##### `Sign()`
```cpp
void Sign(Transaction& tx, const std::string& address);
```
Signs a transaction with the specified account.

**Parameters:**
- `tx`: Transaction to sign
- `address`: Address of signing account

**Throws:** `std::runtime_error` if account not found or locked

### `neo::sdk::wallet::Account`

Represents a wallet account.

#### Properties
```cpp
std::string GetAddress() const;
std::string GetLabel() const;
bool IsDefault() const;
bool IsLocked() const;
std::string GetPublicKey() const;
```

## Transaction API

### `neo::sdk::transaction::TransactionBuilder`

Fluent interface for building transactions.

#### Example Usage
```cpp
auto tx = TransactionBuilder()
    .SetSender(senderAddress)
    .AddTransfer(tokenHash, fromAddress, toAddress, amount)
    .SetSystemFee(1000000)
    .SetNetworkFee(500000)
    .SetValidUntilBlock(currentHeight + 100)
    .Build();
```

#### Methods

##### `SetSender()`
```cpp
TransactionBuilder& SetSender(const std::string& address);
```
Sets the transaction sender (fee payer).

##### `AddTransfer()`
```cpp
TransactionBuilder& AddTransfer(
    const std::string& tokenHash,
    const std::string& from,
    const std::string& to,
    const BigInteger& amount
);
```
Adds a token transfer to the transaction.

##### `AddAttribute()`
```cpp
TransactionBuilder& AddAttribute(const TransactionAttribute& attr);
```
Adds a transaction attribute.

##### `SetSystemFee()`
```cpp
TransactionBuilder& SetSystemFee(uint64_t fee);
```
Sets the system fee (GAS).

##### `SetNetworkFee()`
```cpp
TransactionBuilder& SetNetworkFee(uint64_t fee);
```
Sets the network fee (GAS).

##### `SetValidUntilBlock()`
```cpp
TransactionBuilder& SetValidUntilBlock(uint32_t blockIndex);
```
Sets transaction expiration block.

##### `Build()`
```cpp
std::shared_ptr<Transaction> Build();
```
Builds and returns the transaction.

## Smart Contract API

### `neo::sdk::contract::ContractDeployer`

Deploys smart contracts to the blockchain.

#### Methods

##### `Deploy()`
```cpp
std::shared_ptr<Transaction> Deploy(
    const std::vector<uint8_t>& nefFile,
    const std::string& manifest,
    const std::string& senderAddress
);
```
Creates a deployment transaction.

**Parameters:**
- `nefFile`: Compiled NEF file bytes
- `manifest`: Contract manifest JSON
- `senderAddress`: Deployer address

**Returns:** Deployment transaction

### `neo::sdk::contract::ContractInvoker`

Invokes smart contract methods.

#### Methods

##### `Invoke()`
```cpp
InvocationResult Invoke(
    const std::string& contractHash,
    const std::string& method,
    const std::vector<ContractParameter>& params = {}
);
```
Invokes a contract method.

**Parameters:**
- `contractHash`: Contract script hash
- `method`: Method name
- `params`: Method parameters

**Returns:** Invocation result with stack items

##### `TestInvoke()`
```cpp
InvocationResult TestInvoke(
    const std::string& contractHash,
    const std::string& method,
    const std::vector<ContractParameter>& params = {}
);
```
Tests contract invocation without blockchain modification.

### `neo::sdk::contract::NEP17Token`

Interface for NEP-17 token contracts.

#### Methods

##### `Symbol()`
```cpp
std::string Symbol() const;
```
Returns token symbol.

##### `Decimals()`
```cpp
uint8_t Decimals() const;
```
Returns token decimals.

##### `TotalSupply()`
```cpp
BigInteger TotalSupply() const;
```
Returns total token supply.

##### `BalanceOf()`
```cpp
BigInteger BalanceOf(const std::string& address) const;
```
Returns token balance for address.

##### `Transfer()`
```cpp
std::shared_ptr<Transaction> Transfer(
    const std::string& from,
    const std::string& to,
    const BigInteger& amount,
    const std::string& data = ""
);
```
Creates a transfer transaction.

## RPC Client API

### `neo::sdk::rpc::RPCClient`

Low-level RPC communication client.

#### Methods

##### `Call()`
```cpp
json Call(const std::string& method, const json& params = {});
```
Makes an RPC call.

**Parameters:**
- `method`: RPC method name
- `params`: Method parameters

**Returns:** JSON response

##### `GetVersion()`
```cpp
RPCVersion GetVersion();
```
Returns RPC server version information.

##### `GetConnectionCount()`
```cpp
uint32_t GetConnectionCount();
```
Returns number of connected peers.

##### `GetPeers()`
```cpp
PeerInfo GetPeers();
```
Returns connected and disconnected peer information.

## Network API

### `neo::sdk::network::NetworkClient`

P2P network interaction client.

#### Methods

##### `Connect()`
```cpp
void Connect(const std::string& host, uint16_t port = 10333);
```
Connects to a Neo node.

##### `Disconnect()`
```cpp
void Disconnect();
```
Disconnects from the network.

##### `SendMessage()`
```cpp
void SendMessage(const P2PMessage& message);
```
Sends a P2P protocol message.

##### `OnMessage()`
```cpp
void OnMessage(std::function<void(const P2PMessage&)> handler);
```
Sets message received callback.

## Cryptography API

### `neo::sdk::crypto::KeyPair`

Manages cryptographic key pairs.

#### Factory Methods
```cpp
static KeyPair Generate();
static KeyPair FromPrivateKey(const std::vector<uint8_t>& privateKey);
static KeyPair FromWIF(const std::string& wif);
```

#### Methods

##### `GetPrivateKey()`
```cpp
std::vector<uint8_t> GetPrivateKey() const;
```
Returns private key bytes.

##### `GetPublicKey()`
```cpp
std::vector<uint8_t> GetPublicKey() const;
```
Returns public key bytes.

##### `GetAddress()`
```cpp
std::string GetAddress() const;
```
Returns Neo address.

##### `Sign()`
```cpp
std::vector<uint8_t> Sign(const std::vector<uint8_t>& message) const;
```
Signs a message.

### `neo::sdk::crypto::Hash`

Cryptographic hash functions.

#### Static Methods

##### `SHA256()`
```cpp
static std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data);
```
Computes SHA-256 hash.

##### `RIPEMD160()`
```cpp
static std::vector<uint8_t> RIPEMD160(const std::vector<uint8_t>& data);
```
Computes RIPEMD-160 hash.

##### `Hash256()`
```cpp
static std::vector<uint8_t> Hash256(const std::vector<uint8_t>& data);
```
Computes double SHA-256 hash.

##### `Hash160()`
```cpp
static std::vector<uint8_t> Hash160(const std::vector<uint8_t>& data);
```
Computes SHA-256 then RIPEMD-160 hash.

## Storage API

### `neo::sdk::storage::BlockchainStorage`

Blockchain data storage interface.

#### Methods

##### `GetBlock()`
```cpp
std::optional<Block> GetBlock(uint32_t height) const;
std::optional<Block> GetBlock(const Hash256& hash) const;
```
Retrieves stored blocks.

##### `StoreBlock()`
```cpp
void StoreBlock(const Block& block);
```
Stores a block.

##### `GetTransaction()`
```cpp
std::optional<Transaction> GetTransaction(const Hash256& hash) const;
```
Retrieves stored transactions.

##### `GetState()`
```cpp
std::optional<std::vector<uint8_t>> GetState(
    const std::vector<uint8_t>& key
) const;
```
Retrieves state data.

## Error Handling

All SDK methods follow consistent error handling patterns:

### Exception Types

- `neo::sdk::InvalidArgumentException`: Invalid input parameters
- `neo::sdk::NetworkException`: Network communication errors
- `neo::sdk::CryptoException`: Cryptographic operation failures
- `neo::sdk::StorageException`: Storage access errors
- `neo::sdk::ContractException`: Smart contract execution errors

### Example Error Handling

```cpp
try {
    auto wallet = Wallet::Open("wallet.json", password);
    auto account = wallet->CreateAccount("Main");
    
    auto tx = TransactionBuilder()
        .SetSender(account->GetAddress())
        .AddTransfer(NEO_HASH, from, to, 100)
        .Build();
        
    wallet->Sign(*tx, account->GetAddress());
    
    auto client = std::make_unique<RPCClient>("http://localhost:10332");
    auto txid = client->SendTransaction(*tx);
    
    std::cout << "Transaction sent: " << txid << std::endl;
    
} catch (const neo::sdk::InvalidArgumentException& e) {
    std::cerr << "Invalid input: " << e.what() << std::endl;
} catch (const neo::sdk::NetworkException& e) {
    std::cerr << "Network error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Thread Safety

### Thread-Safe Classes
- `BlockchainClient`: All methods are thread-safe
- `RPCClient`: Thread-safe for concurrent calls
- `Hash`: All static methods are thread-safe

### Non Thread-Safe Classes
- `Wallet`: Requires external synchronization
- `TransactionBuilder`: Create separate instances per thread
- `NetworkClient`: Single-threaded usage only

## Performance Considerations

### Connection Pooling
```cpp
// RPC client maintains connection pool
auto client = std::make_unique<RPCClient>(
    "http://localhost:10332",
    RPCOptions{
        .connectionPoolSize = 10,
        .requestTimeout = std::chrono::seconds(30),
        .keepAlive = true
    }
);
```

### Batch Operations
```cpp
// Batch RPC calls for efficiency
auto batch = client->CreateBatch();
batch.Add("getblockcount");
batch.Add("getbestblockhash");
batch.Add("getconnectioncount");
auto results = batch.Execute();
```

### Caching
```cpp
// Enable caching for frequently accessed data
BlockchainClient client(endpoint, CacheOptions{
    .blockCacheSize = 1000,
    .txCacheSize = 10000,
    .stateCacheSizeMB = 100
});
```

## Version Compatibility

This API reference covers Neo C++ SDK version 1.0.0, compatible with:
- Neo N3 Protocol
- Neo-CLI 3.6.0+
- RPC API 3.6.0+

## Additional Resources

- [SDK Examples](./SDK_EXAMPLES.md)
- [Architecture Guide](./ARCHITECTURE.md)
- [Developer Guide](./DEVELOPER_GUIDE.md)
- [Neo Protocol Documentation](https://docs.neo.org)

---

*Last Updated: August 15, 2025*