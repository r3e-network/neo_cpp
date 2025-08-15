# Neo C++ SDK Documentation

## Overview

The Neo C++ SDK is a comprehensive toolkit for building blockchain applications on the Neo network. It provides full support for wallet management, transaction creation, smart contract interaction, and all Neo RPC methods.

## Features

### Core Capabilities
- 🔗 **Full RPC Support** - Complete implementation of all Neo RPC methods
- 👛 **Wallet Management** - NEP-6 compliant wallet creation and management
- 📝 **Transaction Builder** - Comprehensive transaction creation and signing
- 🪙 **NEP-17 Tokens** - Full support for fungible token standard
- 📜 **Smart Contracts** - Deploy and invoke smart contracts
- 🔐 **Cryptography** - ECDSA signing, key generation, encryption

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp

# Build with SDK enabled
mkdir build && cd build
cmake .. -DNEO_BUILD_SDK=ON
make neo-sdk
```

### Basic Usage

```cpp
#include <neo/sdk/rpc/rpc_client.h>
#include <neo/sdk/wallet/wallet_manager.h>
#include <neo/sdk/transaction/transaction_manager.h>

// Connect to Neo node
auto rpcClient = std::make_shared<neo::sdk::rpc::RpcClient>("http://localhost:10332");

// Create wallet
auto wallet = neo::sdk::wallet::WalletManager::Create("MyWallet", "password");
auto account = wallet->CreateAccount("Main Account");

// Check balance
auto balance = rpcClient->GetNep17Balances(account->GetAddress());

// Create and send transaction
neo::sdk::transaction::TransactionManager txManager(rpcClient);
auto tx = txManager.CreateTransferTransaction(
    account->GetAddress(),
    "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq",
    neo::sdk::transaction::TokenHash::NEO,
    "10"
);

wallet->SignTransaction(*tx, account->GetAddress());
auto txId = txManager.SendTransaction(*tx);
```

## Components

### 1. RPC Client
[Full RPC Client Documentation](rpc_client.md)

The RPC client provides complete access to Neo node functionality:
- Node information queries
- Block and transaction queries
- Smart contract invocation
- NEP-17 token operations
- State queries

### 2. Wallet Manager
[Wallet Manager Documentation](wallet_manager.md)

Complete wallet management system:
- NEP-6 wallet format support
- Account creation and import
- Multi-signature accounts
- Mnemonic phrase support
- Transaction signing

### 3. Transaction Manager
[Transaction Manager Documentation](transaction_manager.md)

Comprehensive transaction handling:
- Transfer transactions
- Contract invocations
- Multi-transfer support
- Fee calculation
- Transaction tracking

### 4. NEP-17 Token Interface
[NEP-17 Documentation](nep17.md)

Full NEP-17 token standard implementation:
- Token information queries
- Balance checking
- Transfer operations
- Transfer history

## Examples

### Example 1: Connect to Node and Get Information
```cpp
auto rpcClient = std::make_shared<neo::sdk::rpc::RpcClient>("http://localhost:10332");

// Test connection
if (rpcClient->TestConnection()) {
    std::cout << "Node Version: " << rpcClient->GetVersion() << std::endl;
    std::cout << "Block Height: " << rpcClient->GetBlockCount() << std::endl;
    std::cout << "Connected Peers: " << rpcClient->GetConnectionCount() << std::endl;
}
```

### Example 2: Create and Manage Wallet
```cpp
// Create new wallet
auto wallet = neo::sdk::wallet::WalletManager::Create("MyWallet", "SecurePassword123!");

// Create accounts
auto account1 = wallet->CreateAccount("Savings");
auto account2 = wallet->CreateAccount("Trading");

// Import existing account
auto imported = wallet->ImportAccount("L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g", "Imported");

// Save wallet
wallet->Save("mywallet.json");
```

### Example 3: Send NEO/GAS
```cpp
neo::sdk::transaction::TransactionManager txManager(rpcClient);

// Create transfer
auto tx = txManager.CreateTransferTransaction(
    fromAddress,
    toAddress,
    neo::sdk::transaction::TokenHash::GAS,
    "100000000"  // 1 GAS (8 decimals)
);

// Calculate and set fees
txManager.SetOptimalFees(*tx);

// Sign transaction
wallet->SignTransaction(*tx, fromAddress);

// Send transaction
auto txId = txManager.SendTransaction(*tx);
std::cout << "Transaction sent: " << txId << std::endl;

// Wait for confirmation
if (txManager.WaitForTransaction(txId, std::chrono::seconds(60))) {
    std::cout << "Transaction confirmed!" << std::endl;
}
```

### Example 4: Interact with Smart Contract
```cpp
// Invoke contract method (test invocation)
auto result = rpcClient->InvokeFunction(
    "0xd2a4cff31913016155e38e474a2c06d08be276cf",  // GAS contract
    "balanceOf",
    {address}
);

std::cout << "GAS Balance: " << result["stack"][0]["value"] << std::endl;

// Create contract invocation transaction
auto tx = txManager.CreateContractTransaction(
    contractHash,
    "transfer",
    {fromAddress, toAddress, amount},
    senderAddress
);
```

### Example 5: NEP-17 Token Operations
```cpp
// Create NEP-17 token interface
neo::sdk::contract::NEP17Token gasToken(rpcClient, neo::sdk::transaction::TokenHash::GAS);

// Get token info
std::cout << "Symbol: " << gasToken.Symbol() << std::endl;
std::cout << "Decimals: " << (int)gasToken.Decimals() << std::endl;
std::cout << "Total Supply: " << gasToken.TotalSupply().ToString() << std::endl;

// Check balance
auto balance = gasToken.BalanceOf(address);
std::cout << "Balance: " << balance.ToString() << std::endl;

// Transfer tokens
auto txId = gasToken.Transfer(fromAddress, toAddress, amount, *wallet);
```

## API Reference

### RPC Client Methods
| Method | Description |
|--------|-------------|
| `GetVersion()` | Get node version information |
| `GetBlockCount()` | Get current block height |
| `GetBlock()` | Get block by hash or index |
| `GetRawTransaction()` | Get transaction details |
| `SendRawTransaction()` | Send signed transaction |
| `InvokeFunction()` | Invoke contract method (test) |
| `GetNep17Balances()` | Get NEP-17 token balances |
| `ValidateAddress()` | Validate Neo address |

### Wallet Manager Methods
| Method | Description |
|--------|-------------|
| `Create()` | Create new wallet |
| `Open()` | Open existing wallet |
| `CreateAccount()` | Create new account |
| `ImportAccount()` | Import account from WIF |
| `SignTransaction()` | Sign transaction |
| `Save()` | Save wallet to file |
| `GenerateMnemonic()` | Generate mnemonic phrase |

### Transaction Manager Methods
| Method | Description |
|--------|-------------|
| `CreateTransferTransaction()` | Create token transfer |
| `CreateContractTransaction()` | Create contract invocation |
| `CreateMultiTransferTransaction()` | Create multiple transfers |
| `SetOptimalFees()` | Calculate and set fees |
| `SendTransaction()` | Send transaction to network |
| `WaitForTransaction()` | Wait for confirmation |

## Advanced Topics

### Multi-Signature Accounts
```cpp
// Create 2-of-3 multi-sig account
std::vector<std::string> publicKeys = {pubKey1, pubKey2, pubKey3};
auto multiSig = wallet->CreateMultiSigAccount(2, publicKeys, "Treasury");
```

### Batch Transactions
```cpp
std::vector<neo::sdk::transaction::Transaction> transactions;
// ... create multiple transactions
auto txIds = txManager.SendBatchTransactions(transactions);
```

### Custom RPC Calls
```cpp
// Make custom RPC call
auto result = rpcClient->Call("custommethod", {param1, param2});
```

## Error Handling

The SDK uses exceptions for error handling:

```cpp
try {
    auto tx = txManager.CreateTransferTransaction(...);
    auto txId = txManager.SendTransaction(*tx);
} catch (const std::runtime_error& e) {
    std::cerr << "Transaction failed: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Configuration

### RPC Client Configuration
```cpp
rpcClient->SetTimeout(30000);  // 30 seconds
auto endpoint = rpcClient->GetEndpoint();
```

### Wallet Configuration
```cpp
// Set scrypt parameters for encryption
wallet->scryptParams_.n = 16384;
wallet->scryptParams_.r = 8;
wallet->scryptParams_.p = 8;
```

## Dependencies

- **CURL** - HTTP/HTTPS communication
- **OpenSSL** - Cryptographic operations
- **nlohmann/json** - JSON parsing
- **libscrypt** (optional) - Key derivation

## Building from Source

### Requirements
- CMake 3.16+
- C++20 compiler
- OpenSSL development libraries
- CURL development libraries

### Build Commands
```bash
mkdir build && cd build
cmake .. -DNEO_BUILD_SDK=ON
make neo-sdk
make install
```

## Testing

Run SDK tests:
```bash
make neo-sdk-tests
./neo-sdk-tests
```

## Performance

- **RPC Calls**: < 100ms typical latency
- **Transaction Creation**: < 10ms
- **Signing**: < 5ms per signature
- **Memory Usage**: < 50MB typical

## Security Considerations

1. **Private Key Storage**: Never store private keys in plain text
2. **Password Security**: Use strong passwords for wallet encryption
3. **Network Security**: Use HTTPS for RPC connections in production
4. **Input Validation**: Always validate addresses and amounts
5. **Error Handling**: Properly handle all error conditions

## Support

- [GitHub Issues](https://github.com/neo-project/neo-cpp/issues)
- [Neo Discord](https://discord.gg/neo)
- [Developer Forum](https://forum.neo.org)

## License

MIT License - See LICENSE file for details

---

*SDK Version: 1.0.0*
*Compatible with Neo N3*
*Last Updated: August 2025*